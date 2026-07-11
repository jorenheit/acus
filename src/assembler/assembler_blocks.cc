// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::beginBlock(std::string const &name) {
  
  Function::Block &block = _currentFunction->createBlock(name);
  _program.registerBlock(block);

  if (_currentFunction->blocks.size() == 1) {
    _currentFunction->entryBlockIndex = block.globalBlockIndex;
  }

  _currentBlock = &block;
  setTargetSequence(&block.code);

  // To start a block, we need to check 2 conditions:
  // 1. Does the block-index match the value stored in the TargetBlock cell?
  // 2. Is the Run-cell still set?
  //
  // If both are true, the Flag field of the TargetBlock cell is set to 1 and
  // used as the conditional cell upon which it is decided whether or not to enter
  // the block.
  
  moveTo(FrameLayout::TargetBlock, MacroCell::Value0);
  compare16ToConstConstructive(/* value =    */ _currentBlock->globalBlockIndex,
			       /* highByte = */ Cell{FrameLayout::TargetBlock, MacroCell::Value1},
			       /* result =   */ Cell{FrameLayout::TargetBlock, MacroCell::Flag},
			       Temps<2>::select(FrameLayout::TargetBlock, MacroCell::Scratch0,
						FrameLayout::TargetBlock, MacroCell::Scratch1));

  moveTo(FrameLayout::RunState, MacroCell::Value0);
  andConstructive(/* result = */ Cell{FrameLayout::RunState, MacroCell::Flag},
		  /* other  = */ Cell{FrameLayout::TargetBlock, MacroCell::Flag},
		  Temps<2>::select(FrameLayout::RunState, MacroCell::Scratch0,
				   FrameLayout::RunState, MacroCell::Scratch1));

  // Clear the targetblock flag
  moveTo(FrameLayout::TargetBlock, MacroCell::Flag);
  zeroCell();

  // Open the block, conditional on the run-flag
  moveTo(FrameLayout::RunState, MacroCell::Flag);
  loopOpen();
  zeroCell();
  moveToOrigin();
}

void Assembler::endBlock() {
  assert(_currentBlock != nullptr);

  // We move back to the Flag stored in the RunState cell (guaranteed zero)
  moveTo(FrameLayout::RunState, MacroCell::Flag);
  loopClose();
  moveToOrigin();
  _currentBlock = nullptr;
}

std::string Assembler::generateUniqueBlockName() {
  static int blockID = 0;
  return "__block_" + std::to_string(blockID++);
}

void Assembler::label(std::string const &labelName, API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  _cache.controlBoundary();  
  if (_currentBlock != nullptr) {
    setNextBlock(_currentFunction->name, labelName);    
    endBlock();
  }

  beginBlock(labelName);
}

void Assembler::setNextBlock(std::string const &f, std::string const &b) {

  if (_currentBlock != nullptr) {
    _currentBlock->children.emplace_back(f, b);
  }
  
  pushPtr();

  moveTo(FrameLayout::TargetBlock, MacroCell::Value0);
  zeroCell();
  emit<primitive::ChangeBy>([f, b](primitive::Context const &ctx) -> int {
    return ctx.getBlockIndex(f, b) & 0xff;
  });
  
  moveTo(FrameLayout::TargetBlock, MacroCell::Value1);
  zeroCell();
  emit<primitive::ChangeBy>([f, b](primitive::Context const &ctx) -> int {
    return (ctx.getBlockIndex(f, b) >> 8) & 0xff;
  });
  
  popPtr();
}

void Assembler::setNextBlock(Expression obj) {
  assert(types::isFunctionPointer(obj.type()));
  
  auto const targetSlot  = Slot {
    SlotData{
      .name = "target_block",
      .type = obj.type(),
      .kind = Slot::Dummy,
      .offset = FrameLayout::TargetBlock
    }
  };
  
  if (obj.hasSlot()) {
    Slot const ptrSlot = materialize(obj.slot());
    assignSlot(targetSlot, ptrSlot);
  } else {
    assignSlot(targetSlot, obj.literal());
  }
}

void Assembler::jump(std::string const &jumpLabel, API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  deferLabelCheck(_currentFunction->name, jumpLabel, API_FWD);
  
  // A jump ends the current block and sets the target to the indicated jump label.
  // After a jump, a label is expected to prevent unreachable code.

  assert(_currentBlock != nullptr);
  _cache.controlBoundary();
  setNextBlock(_currentFunction->name, jumpLabel);
  endBlock();

  API_EXPECT_NEXT("label");
}

void Assembler::jumpIfImpl(Expression obj, std::string const &trueLabel,
		       std::string const &falseLabel, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_INTEGER(obj.type());

  if (obj.hasSlot()) {
    branchIfSlot(materialize(obj.slot()), trueLabel, falseLabel);
  } else {  
    bool const value = literal::cast<types::IntegerType>(obj.literal())->encodedValue();
    setNextBlock(_currentFunction->name, value ? trueLabel : falseLabel);
  }

  deferLabelCheck(_currentFunction->name, trueLabel, API_FWD);
  deferLabelCheck(_currentFunction->name, falseLabel, API_FWD);
  _cache.controlBoundary();  
  endBlock();
  
  API_EXPECT_NEXT("label");
}

void Assembler::constructMetaBlocks() {
  assert(_currentFunction == nullptr);
  assert(_currentBlock == nullptr);

  for (size_t idx = 0; idx != _metaBlocks.size(); ++idx) {
    MetaBlock const &m = _metaBlocks[idx];
    types::TypeHandle returnType = std::holds_alternative<std::string>(m.callee)
      ? _program.function(std::get<std::string>(m.callee)).type->returnType()
      : std::get<types::FunctionType const *>(m.callee)->returnType();

    // Set current function to caller (owner of metablock) and construct block
    _currentFunction = &_program.function(m.caller);    
    beginBlock(m.name); {

      if (returnType == ts::void_t() || not m.returnSlot){
	fetchReturnData();
      }
      else {
	assert(m.returnSlot.has_value());
	SlotProxy returnSlot = *m.returnSlot;
	assert(returnType == returnSlot.type());

	_cache.write(returnSlot, [&](Slot const &slot){
	  fetchReturnData(slot);
	});
	_cache.controlBoundary();
      }

      // Check if the run-state has become 0. If so, unwind the stack
      moveTo(FrameLayout::RunState, MacroCell::Value0);
      copyField(Cell{FrameLayout::RunState, MacroCell::Scratch0},
		Temps<1>::select(_dp.current().offset, MacroCell::Scratch1)); 
      moveTo(FrameLayout::RunState, MacroCell::Scratch1);
      setToValue(1);
      
      switchField(MacroCell::Scratch0);
      loopOpen(); { // if run: sync globals and set next block
	zeroCell();
	switchField(MacroCell::Scratch1);
	zeroCell();
	setNextBlock(m.caller, m.nextBlockName);
	switchField(MacroCell::Scratch0);	
      } loopClose();
      
      switchField(MacroCell::Scratch1);
      loopOpen(); { // else: pop frame
	zeroCell();
	popFrame(); // This leaves us at the Scratch1 cell in another frame: guaranteed 0
      } loopClose();
      switchField(MacroCell::Value0);
    } endBlock();
  }

  _currentFunction = nullptr;
  assert(_currentBlock == nullptr);
  _metaBlocks.clear();
}

void Assembler::unreachable(API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  _currentBlock->reachable = false;
}
