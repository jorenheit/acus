// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"
#include <iostream>

Assembler::FunctionCallBuilder Assembler::callFunction(std::string const &functionName, API_FUNC) {
  API_FUNC_BEGIN();
  return FunctionCallBuilder { *this, functionName, API_FWD };
}

void Assembler::callFunctionImpl(std::string const &functionName, std::optional<Expression> const &returnSlot,
				 std::vector<Expression> const &args, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  deferFunctionCallTypeCheck(functionName, args, API_FWD);

  std::string const nextBlockName = generateUniqueBlockName();

  // Schedule metablock
  std::string const metaBlockName = std::string("__ret_meta_")
    + _currentFunction->name + "_"
    + std::to_string(_metaBlocks.size());

  _metaBlocks.push_back(MetaBlock{
      .name = metaBlockName,
      .caller = _currentFunction->name,
      .callee = functionName,
      .returnSlot = returnSlot ? std::optional<SlotProxy>(returnSlot->slot()) : std::nullopt,
      .nextBlockName = nextBlockName,
    });

  setNextBlock(_currentFunction->name, metaBlockName);

  // Prepare frame (set target, copy args) and push next frame onto the stack
  _cache.callBoundary();  
  prepareNextFrame(functionName, args, API_FWD);
  pushFrame();
  _cache.reset();
  

  // Start the next block
  assert(_currentBlock != nullptr);
  endBlock();

  beginBlock(nextBlockName);
}

void Assembler::callFunctionImpl(Expression fPtr, std::optional<Expression> const &returnSlot,
				 std::vector<Expression> const &args, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_FUNCTION_POINTER(fPtr.type());

  auto functionType = types::cast<types::FunctionPointerType>(fPtr.type())->functionType();  
  functionCallTypeCheck(functionType, args, API_FWD);

  std::string const nextBlockName = generateUniqueBlockName();

  // Schedule metablock
  std::string const metaBlockName = std::string("__ret_meta_")
    + _currentFunction->name + "_"
    + std::to_string(_metaBlocks.size());
  setNextBlock(_currentFunction->name, metaBlockName);

  // Build MetaBlock
  _metaBlocks.push_back(MetaBlock{
      .name = metaBlockName,
      .caller = _currentFunction->name,
      .callee = functionType,
      .returnSlot = returnSlot ? std::optional<SlotProxy>(returnSlot->slot()) : std::nullopt,
      .nextBlockName = nextBlockName,
    });

  // Prepare frame (set target, copy args) and push next frame onto the stack
  _cache.callBoundary();
  prepareNextFrame(fPtr, args, API_FWD);
  pushFrame();
  _cache.reset();

  // Start next block
  assert(_currentBlock != nullptr);
  endBlock();
  beginBlock(nextBlockName);
}

void Assembler::functionCallTypeCheck(types::FunctionType const *functionType, std::vector<Expression> const &args, API_CTX) {
  auto const &paramTypes = functionType->paramTypes();
  API_REQUIRE(paramTypes.size() == args.size(),
	      error::ErrorCode::InvalidFunctionPointerCall,
	      "invalid number of arguments in function-call through function-pointer: " 
	      "expected ", paramTypes.size(), ", got ", args.size(), ".");
  for (size_t i = 0; i != args.size(); ++i) {
    API_REQUIRE_ASSIGNABLE(paramTypes[i], args[i].type());
  }
}

void Assembler::deferFunctionCallTypeCheck(std::string const &callee, std::vector<Expression> const &args, API_CTX) {
    _deferredFunctionCallTypeChecks.emplace_back(FunctionCallInfo {
      .API_CTX_NAME = API_FWD,
      .callee = callee,
      .args = args
    });
}

void Assembler::deferredFunctionCallTypeChecks() {
  assert(_currentFunction == nullptr);
  for (auto const &[API_CTX_NAME, callee, args]: _deferredFunctionCallTypeChecks) {
    API_REQUIRE_FUNCTION_DEFINED(callee);
    functionCallTypeCheck(_program.function(callee).type, args, API_FWD);
  }

  _deferredFunctionCallTypeChecks.clear();
}

void Assembler::labelCheck(std::string const &functionName, std::string const &labelName, API_CTX) {
  API_REQUIRE_FUNCTION_DEFINED(functionName);
  API_REQUIRE_LABEL_DEFINED(functionName, labelName);
}

void Assembler::deferLabelCheck(std::string const &functionName, std::string const &labelName, API_CTX) {
  _deferredLabelChecks.emplace_back( LabelCheck {
      .API_CTX_NAME = API_FWD,
      .functionName = functionName,
      .labelName = labelName
    });
}

void Assembler::deferredLabelChecks() {
  assert(_currentFunction == nullptr);

  for (auto const &[API_CTX_NAME, functionName, labelName]: _deferredLabelChecks) {
    labelCheck(functionName, labelName, API_FWD);
  }

  _deferredLabelChecks.clear();
}

void Assembler::abortProgram(API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  moveTo(FrameLayout::RunState, MacroCell::Value0);
  zeroCell();

  // Sync and pop
  popFrame();

  // Block boundary
  assert(_currentBlock != nullptr);
  _currentBlock->returns = true;
  endBlock();
  beginBlock(generateUniqueBlockName());
}

void Assembler::returnFromFunction(API_FUNC) {
  API_FUNC_BEGIN();
  returnFromFunctionImpl({}, API_FWD);
}

void Assembler::returnFromFunctionImpl(std::optional<Expression> const &ret, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  _currentBlock->returns = true;
  
  if (ret) {
    API_REQUIRE_ASSIGNABLE(_currentFunction->type->returnType(), ret->type());

    // Copy the variable into the return-slot. 
    auto returnSlot = Slot {
      SlotData {
	.name = "__return_slot",
	.type = ret->type(),
	.kind = Slot::Dummy,
	.offset = FrameLayout::ReturnValueStart,
	.scope = nullptr
      },
      false
    };
      
    assignImpl(Expression{returnSlot}, *ret, true, API_FWD);
  }
  
  _cache.returnBoundary();
  popFrame();

  assert(_currentBlock != nullptr);
  endBlock();
  beginBlock(generateUniqueBlockName());  
}

void Assembler::initializeArguments(primitive::DInt const currentFrameSize, primitive::DInt const paramStart,
				  std::vector<Expression> const &args, API_CTX) {

  auto const copySlotToNextFrame = [&](Slot slot, int &offset) {
    assert(slot.kind() != Slot::Temp);
    for (int i = 0; i != slot.type()->size(); ++i) {
      int const varIndex0 = getFieldIndex(slot + i, MacroCell::Value0);
      primitive::DInt const paramIndex0 = currentFrameSize + paramStart + offset + MacroCell::Value0;
      primitive::DInt const scratchIndex = paramIndex0 + (MacroCell::Scratch0 - MacroCell::Value0);

      moveTo(slot + i, MacroCell::Value0);
      emit<primitive::CopyData>(varIndex0, paramIndex0, scratchIndex);
      moveTo(slot + i, MacroCell::Value1);
      if (slot.type()->usesValue1()) {
	int const varIndex1 = getFieldIndex(slot + i, MacroCell::Value1);
	primitive::DInt const paramIndex1 = currentFrameSize + paramStart + offset + MacroCell::Value1;
	emit<primitive::CopyData>(varIndex1, paramIndex1, scratchIndex);
      }
      else {
	emit<primitive::ZeroCell>();
      }
      offset += MacroCell::FieldCount;
    }
  };

  auto const moveSlotToNextFrame = [&](Slot slot, int &offset) {
    assert(slot.kind() == Slot::Temp);
    for (int i = 0; i != slot.type()->size(); ++i) {
      int const varIndex0 = getFieldIndex(slot + i, MacroCell::Value0);
      primitive::DInt const paramIndex0 = currentFrameSize + paramStart + offset + MacroCell::Value0;

      moveTo(slot + i, MacroCell::Value0);
      emit<primitive::MoveData>(varIndex0, paramIndex0);
      moveTo(slot + i, MacroCell::Value1);
      if (slot.type()->usesValue1()) {
	int const varIndex1 = getFieldIndex(slot + i, MacroCell::Value1);
	primitive::DInt const paramIndex1 = currentFrameSize + paramStart + offset + MacroCell::Value1;
	emit<primitive::MoveData>(varIndex1, paramIndex1);
      }
      else {
	emit<primitive::ZeroCell>();
      }
      offset += MacroCell::FieldCount;
    }
  };

  auto const copyOrMoveSlotToNextFrame = [&](Slot slot, int &offset) {
    if (slot.kind() == Slot::Temp) moveSlotToNextFrame(slot, offset);
    else copySlotToNextFrame(slot, offset);
  };

  auto const constructInNextFrame = [&](auto&& self, int &offset, Expression const &arg) -> void {

    if (arg.hasSlot()) { // Already stored on tape -> copy to next frame
      Slot const argSlot = materialize(arg.slot());
      switch (argSlot.type()->tag()) {
      case types::U8:
      case types::U16:
      case types::S8:
      case types::S16:
      case types::STRING:
      case types::FUNCTION_POINTER: {
	copyOrMoveSlotToNextFrame(argSlot, offset);
	break;
      }
      case types::POINTER: {
	int const destOffset = offset;
	copyOrMoveSlotToNextFrame(argSlot, offset);
	primitive::DInt const distance = currentFrameSize + paramStart + destOffset + MacroCell::Value0;
	moveTo(0, MacroCell::Value0);
	emit<primitive::MovePointerRelative>(distance);
	inc();
	emit<primitive::MovePointerRelative>(-distance);
	break;
      }	
      case types::ARRAY: {
	auto arrayType = types::cast<types::ArrayType>(argSlot.type());
	auto elementType = arrayType->elementType();
	
	for (int i = 0; i != arrayType->length(); ++i) {
	  auto const elementSlot = Slot {
	    SlotData {
	      .name = "dummy",
	      .type = elementType,
	      .kind = Slot::Dummy,
	      .offset = argSlot + i * elementType->size()
	    },
	    false
	  };
	  self(self, offset, rValue(elementSlot, API_FWD));
	}
	break;
      }
      case types::STRUCT: {
	auto structType = types::cast<types::StructType>(argSlot.type());
	for (int i = 0; i != structType->fieldCount(); ++i) {
	  auto fieldType = structType->fieldType(i);
	  auto const fieldSlot = Slot {
	    SlotData {
	      .name = "dummy",
	      .type = fieldType,
	      .kind = Slot::Dummy,
	      .offset = argSlot + structType->fieldOffset(i)
	    },
	    false
	  };
	  self(self, offset, rValue(fieldSlot, API_FWD));
	}
	break;
      }
      default: {
	assert(false && "What type is this?");
	std::unreachable();
      }
      } // switch (tag)
    }
    else { // anonymous value -> construct in-place
      types::TypeHandle argType = arg.type();
      switch(argType->tag()) {
      case types::U8:
      case types::S8:
      case types::U16:
      case types::S16: {
	// Construct integer
	int const value = literal::cast<types::IntegerType>(arg.literal())->encodedValue();
	moveTo(0, MacroCell::Value0);
	primitive::DInt const diff = currentFrameSize + paramStart + offset;
	emit<primitive::MovePointerRelative>(diff);
	setToValue(value, Temps<1>::select(0, MacroCell::Scratch0));
	switchField(MacroCell::Value1);
	setToValue(argType->usesValue1() ? ((value >> 8) & 0xff) : 0, Temps<1>::select(0, MacroCell::Scratch0));
	switchField(MacroCell::Value0);
	emit<primitive::MovePointerRelative>(-diff);
	offset += MacroCell::FieldCount;
	break;
      }
      case types::FUNCTION_POINTER: {
	std::string const &functionName = literal::cast<types::FunctionPointerType>(arg.literal())->functionName();
	primitive::DInt const diff = currentFrameSize + paramStart + offset;
	emit<primitive::MovePointerRelative>(diff);
	zeroCell();
	emit<primitive::ChangeBy>([functionName](primitive::Context const &ctx) -> int {
	  return ctx.getBlockIndex(functionName) & 0xff;
	});
	switchField(MacroCell::Value1);
	zeroCell();
	emit<primitive::ChangeBy>([functionName](primitive::Context const &ctx) -> int {
	  return (ctx.getBlockIndex(functionName) >> 8) & 0xff;
	});
	switchField(MacroCell::Value0);
	emit<primitive::MovePointerRelative>(-diff);
	offset += MacroCell::FieldCount;
      }
      case types::ARRAY:
      case types::STRING: {
	// recursive call for each element
	for (int i = 0; i != types::cast<types::ArrayLike>(argType)->length(); ++i)
	  self(self, offset, rValue(literal::cast<types::ArrayLike>(arg.literal())->element(i), API_FWD));
	break;
      }
      case types::POINTER: {
	assert(false && "there should not be anonymous pointers"); 
      }
      default: assert(false && "passing this type as arg is not supported"); 
      }
    }
  };

  pushPtr();
  
  // Copy arguments
  int offset = 0;
  for (Expression const &arg: args) {
    constructInNextFrame(constructInNextFrame, offset, arg);
  }
  
  popPtr();
}


void Assembler::prepareNextFrame(std::string const &functionName, std::vector<Expression> const &args, API_CTX) {

  primitive::DInt const paramStart = [callee = functionName](primitive::Context const &ctx) {
    return ctx.getLocalBaseOffset(callee) * MacroCell::FieldCount;
  };

  primitive::DInt const currentFrameSize = [caller = _currentFunction->name](primitive::Context const &ctx){
    return ctx.getStackFrameSize(caller) * MacroCell::FieldCount;
  };
  
  initializeArguments(currentFrameSize, paramStart, args, API_FWD);

  // Set target block in next frame
  emit<primitive::MovePointerRelative>(currentFrameSize); 
  setNextBlock(functionName, "");
  emit<primitive::MovePointerRelative>(-currentFrameSize);
}

void Assembler::prepareNextFrame(Expression fptr, std::vector<Expression> const &args, API_CTX) {

  auto functionPointerType = types::cast<types::FunctionPointerType>(fptr.type());
  auto functionType = types::cast<types::FunctionType>(functionPointerType->functionType());
  auto returnType = functionType->returnType();
  
  int const paramStart = (FrameLayout::ReturnValueStart + returnType->size()) * MacroCell::FieldCount;

  primitive::DInt const currentFrameSize = [caller = _currentFunction->name](primitive::Context const &ctx){
    return ctx.getStackFrameSize(caller) * MacroCell::FieldCount;
  };

  initializeArguments(currentFrameSize, paramStart, args, API_FWD);
  
  
  // Set target block
  Slot fptrSlot = [&] {
    if (fptr.hasSlot()) return materialize(fptr.slot());
    return getTemp(fptr.literal());
  }();

  primitive::DInt const sourceCell0 = getFieldIndex(fptrSlot, MacroCell::Value0);
  primitive::DInt const sourceCell1 = getFieldIndex(fptrSlot, MacroCell::Value1);
  primitive::DInt const targetCell0 = getFieldIndex(FrameLayout::TargetBlock, MacroCell::Value0) + currentFrameSize;
  primitive::DInt const targetCell1 = getFieldIndex(FrameLayout::TargetBlock, MacroCell::Value1) + currentFrameSize;
  primitive::DInt const scratchCell = getFieldIndex(FrameLayout::TargetBlock, MacroCell::Scratch0) + currentFrameSize;

  pushPtr();
  moveTo(fptrSlot, MacroCell::Value0);
  emit<primitive::CopyData>(sourceCell0, targetCell0, scratchCell);
  switchField(MacroCell::Value1);   
  emit<primitive::CopyData>(sourceCell1, targetCell1, scratchCell);
  popPtr();

  if (fptrSlot.kind() == Slot::Temp) freeTempSlot(fptrSlot);
}




void Assembler::fetchReturnData() {
  assert(_currentSeq != nullptr);  
  assert(_currentFunction != nullptr);

  primitive::DInt const stackFrameSize = [caller = _currentFunction->name](primitive::Context const &ctx) -> int {
    return ctx.getStackFrameSize(caller) * MacroCell::FieldCount;
  };

  pushPtr();
  
  // Get run-state
  moveTo(FrameLayout::RunState);
  emit<primitive::MovePointerRelative>(stackFrameSize);
  emit<primitive::MoveData>(-stackFrameSize);
  emit<primitive::MovePointerRelative>(-stackFrameSize);

  popPtr();
}



void Assembler::fetchReturnData(Slot returnSlot) {

  primitive::DInt const stackFrameSize = [caller = _currentFunction->name](primitive::Context const &ctx) -> int {
    return ctx.getStackFrameSize(caller) * MacroCell::FieldCount;
  };

  pushPtr();
  for (int i = 0; i != returnSlot.type()->size(); ++i) {
    primitive::DInt const diff = stackFrameSize - (returnSlot - FrameLayout::ReturnValueStart) * MacroCell::FieldCount;
    
    moveTo(returnSlot + i, MacroCell::Value0);
    emit<primitive::MovePointerRelative>(diff);
    emit<primitive::MoveData>(-diff);
    emit<primitive::MovePointerRelative>(-diff);

    if (returnSlot.type()->usesValue1()) {
      moveTo(returnSlot + i, MacroCell::Value1);
      emit<primitive::MovePointerRelative>(diff);
      emit<primitive::MoveData>(-diff);
      emit<primitive::MovePointerRelative>(-diff);
    }
  }
  fetchReturnData(); // fetch the rest
  popPtr();
}


void Assembler::branchIfSlot(Slot slot, std::string const &trueLabel, std::string const &falseLabel) {

  pushPtr();

  Slot tmp = getTemp(ts::u8());
    
  moveTo(slot);
  if (slot.type()->usesValue1()) {
    orConstructive(Cell{tmp, MacroCell::Value0},
		   Cell{slot, MacroCell::Value1},
		   Temps<2>::select(tmp, MacroCell::Scratch0,
				    tmp, MacroCell::Scratch1));
  }
  else {
    copyField(Cell{tmp, MacroCell::Value0},
	      Temps<1>::select(tmp, MacroCell::Scratch0));
  }

  moveTo(tmp);
  switchField(MacroCell::Flag);
  setToValue(1);
  switchField(MacroCell::Value0);
  loopOpen(); {
    zeroCell();
    setNextBlock(_currentFunction->name, trueLabel);
    switchField(MacroCell::Flag);
    setToValue(0);
    switchField(MacroCell::Value0);	
  }; loopClose();

  switchField(MacroCell::Flag);
  loopOpen(); {
    zeroCell();
    setNextBlock(_currentFunction->name, falseLabel);
  } loopClose();

  popPtr();

  freeTempSlot(tmp);
}
