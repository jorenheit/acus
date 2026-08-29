// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

Assembler::ProgramBuilder Assembler::program(std::string const &name, std::string const &entry, API_FUNC) {
  API_FUNC_BEGIN();
  return ProgramBuilder{ *this, name, entry, API_FWD };
}

void Assembler::beginProgramImpl(std::string const &name, std::string const &entry, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_OUTSIDE_PROGRAM_BLOCK();

  _program = {};
  _program.name = name;
  _program.entryFunctionName = entry;  
  _state.begun = true;
  _state.allowGlobalDeclarations = true;
  _counters.tmpID = 0;
  _counters.cacheID = 0;
  _counters.scopeID = 0;
  _dp.set(0, MacroCell::Value0);
  
  
  // Globals should start at same frame offset as locals for consistency -> pad with raw
  declareGlobal("__pad__", ts::raw(FrameLayout::ReturnValueStart));
}

void Assembler::endProgram(API_FUNC) {

  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_PROGRAM_BLOCK();
  API_REQUIRE_OUTSIDE_FUNCTION_BLOCK();
  API_REQUIRE(_program.functions.size() > 0,
	      error::ErrorCode::EmptyProgram,
	      "a program should contain at least one function.");

  API_REQUIRE(_program.isFunctionDefined(_program.entryFunctionName),
	      error::ErrorCode::EntryFunctionNotDefined,
	      "entry function '", _program.entryFunctionName, "' was never defined.");

  auto const entryFunctionType = _program.function(_program.entryFunctionName).type;
  API_REQUIRE(entryFunctionType == ts::void_function(),
	      error::ErrorCode::WrongEntryFunctionType,
	      "entry function must be of type 'void()', but is of type '", entryFunctionType->str(), "'."); 
	      
  
  // Generate the metablocks and builtin functions
  constructBuiltinFunctions();
  constructMetaBlocks();

  // Check if all calls and jumps can be resolved properly
  deferredFunctionCallTypeChecks();
  deferredLabelChecks();

  // Mark reachability for all blocks
  for (auto &fn: _program.functions) {
    checkFunctionFlowValidity(fn, API_FWD);
  }

  // Gather reachable blocks
  std::vector<Function::Block *> dispatchBlocks;
  for (auto *block : _program.globalBlockOrder) {
    if (block->reachable) {
      dispatchBlocks.push_back(block);
    }
  }

  API_REQUIRE(dispatchBlocks.size() <= 0xfeff, error::ErrorCode::TooManyBlocks,
	      "Number of blocks exceeds maximum (0xfeff)");

  // Start building result-sequence
  primitive::Sequence result;
  
  // Find optimal switch-shape
  auto const [outerSwitchCaseCount, innerSwitchCaseCount] = [&] -> std::pair<int, int> {

    struct Result {
      int outer;
      int inner;
      int sum;
      int diff;
      Result(int i, int j):
	outer(std::min(i, j)),
	inner(std::max(i, j)),
	sum(i + j),
	diff(inner - outer)
      {}
    };

    Result best{0xfe, 0xff};

    for (int inner = 1; inner <= 0x0100; ++inner) {
      int outer = (dispatchBlocks.size() + inner - 1) / inner;
      if (outer > 0x00ff) continue;

      Result result{outer, inner};
      if (result.sum <= best.sum) {
	if (result.sum < best.sum || result.diff < best.diff) {
	  best = result;
	}
      }
    }
    
    return {best.outer, best.inner};
  }();

 
  // TODO: document how the switch works 
  auto constructSwitches = [&](MacroCell::Field outerValueField,
			       MacroCell::Field outerFlagField,
			       MacroCell::Field innerValueField,
			       MacroCell::Field innerFlagField) -> void {
    
    auto impl = [&](auto &&self,
		    MacroCell::Field valueField,
		    MacroCell::Field flagField,
		    int caseCount,
		    int caseIndex,
		    auto &&caseBody) -> void
    {
      assert(caseCount > 0);
      assert(caseIndex >= 0 && caseIndex < caseCount);
      
      switchField(valueField);
      loopOpen(); {
	if (caseIndex + 1 == caseCount) {
	  // Default case: abort program	  
	  // 1. Flush remainder of the valueField	  
	  zeroCell();

	  // 2. Clear TargetBlock high-byte
	  switchField(MacroCell::Value1);
	  zeroCell();

	  // 3. Clear switch-flag
	  switchField(flagField);
	  dec();

	  switchField(valueField);
	}
	else {
	  // Recursive call to build all cases up to the default case above
	  dec();      
	  self(self, valueField, flagField, caseCount, caseIndex + 1, caseBody);
	}
      } loopClose();

      // Case implementations
      switchField(flagField);
      loopOpen(); {
	dec();

	moveToOrigin();
	caseBody(caseIndex);

	// Make sure we end on the flag again to exit the case
	moveTo(FrameLayout::TargetBlock, flagField);
      } loopClose();
      
      switchField(valueField);
    };

    // Set flag
    switchField(outerFlagField);
    inc();
    
    // Initial decrement for the outer switch.    
    switchField(outerValueField);
    dec();

    // Outer switch
    impl(impl,
	 outerValueField,
	 outerFlagField,
	 outerSwitchCaseCount,
	 0,
	 [&](int outerCaseIndex){

	   int const thisInnerSwitchCaseCount =
	     (outerCaseIndex + 1) < outerSwitchCaseCount
	     ? innerSwitchCaseCount
	     : ((dispatchBlocks.size() - 1) % innerSwitchCaseCount + 1);	   
	   
	   // Set flag
	   switchField(innerFlagField);
	   inc();
	   // Inner switch
	   impl(impl,
		innerValueField,
		innerFlagField,
		thisInnerSwitchCaseCount,
		0,
		[&](int innerCaseIndex) {
		  // Insert block body
		  size_t const dispatchIndex = outerCaseIndex * innerSwitchCaseCount + innerCaseIndex;
		  assert(dispatchIndex < dispatchBlocks.size());
		  result.append(dispatchBlocks[dispatchIndex]->code);    
		});
	 });
  };


  auto copyTargetBlockToScratch = [&]{
    // Use Daniel's algorithm to copy the TargetBlock into the scratch cells
    // 1: Move TargetBlock high byte value into Scratch1 and Flag
    loopOpen(); {
      switchField(MacroCell::Scratch1); inc();
      switchField(MacroCell::Flag);     inc();
      switchField(MacroCell::Value1);   dec();
    } loopClose();
    
    // 2: Move TargetBlock low byte value into Value1 and Scratch0
    switchField(MacroCell::Value0);
    loopOpen(); {
      switchField(MacroCell::Value1);   inc();
      switchField(MacroCell::Scratch0); inc();
      switchField(MacroCell::Value0);   dec();
    } loopClose();
    
    // 3: Restore low byte in Value0
    switchField(MacroCell::Value1);
    loopOpen(); {
      switchField(MacroCell::Value0); inc();
      switchField(MacroCell::Value1); dec();
    } loopClose();

    // 4: Restore high byte in Value1
    switchField(MacroCell::Flag);
    loopOpen(); {
      switchField(MacroCell::Value1); inc();
      switchField(MacroCell::Flag);   dec();
    } loopClose();
  };

  
  // Construct final result
  setTargetSequence(&result);

  // Mark the first cell (start of global frame) with a SeekMarker and a FrameMarker
  _dp.set(0, static_cast<MacroCell::Field>(0));
  switchField(MacroCell::SeekMarker);
  inc();
  switchField(MacroCell::FrameMarker);
  inc();

  // Mark the start of Frame 1 (main-function) with a FrameMarker
  moveTo(1 + _program.globalVariableFrameSize()); 
  _dp.set(0);
  switchField(MacroCell::FrameMarker);
  inc();

  // Populate the TargetBlock with the index corresponding to the entrypoint
  setTargetBlock(_program.entryFunctionName, "");

  // The high byte of the TargetBlock is the main loop-guard (Run-flag). 
  moveTo(FrameLayout::TargetBlock, MacroCell::Value1);
  loopOpen("main loop"); {

    // Make a temporary copy of the TargetBlock cells
    copyTargetBlockToScratch();

    // Construct switches that use the copied values to select the correct block
    constructSwitches(MacroCell::Scratch1, MacroCell::Flag,      // outer switch fields
		      MacroCell::Scratch0, MacroCell::Scratch1); // inner switch fields
  
    // Close main loop
    switchField(MacroCell::Value1);
  } loopClose("main loop");

  _state.begun = false;
  setTargetSequence(nullptr);

  // Primitive merging
  mergeSequence(result);
  
  // Construct context for final generation passes
  primitive::Context ctx = constructContext(dispatchBlocks, innerSwitchCaseCount);  

  _txt[_program.name] = result.dumpText(ctx);
  _bf[_program.name] = simplifyBrainfuck(result.dumpCode(ctx));
}

Assembler::FunctionBuilder Assembler::function(std::string const &name, API_FUNC) {
  API_FUNC_BEGIN();
  return FunctionBuilder { *this, name, API_FWD };
}

void Assembler::beginFunctionImpl(std::string const &name, types::TypeHandle type, std::vector<std::string> const &params, API_CTX) {
  API_CHECK_EXPECTED();  
  API_REQUIRE_INSIDE_PROGRAM_BLOCK();
  API_REQUIRE_OUTSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_FUNCTION(type);

  auto fType = types::cast<types::FunctionType>(type);
  API_REQUIRE_PARAM_COUNT_MATCHES_FUNCTION(fType, params);
  
  _state.allowGlobalDeclarations = false;
  _currentFunction = &_program.createFunction(name, fType, _counters.scopeID++);
  _cache.reset();

  std::unordered_set<std::string> paramSet;
  for (size_t i = 0; i != params.size(); ++i) {
    std::string const &name = params[i];
    auto [_, unique] = paramSet.insert(name);
    API_REQUIRE(unique,
		error::ErrorCode::DuplicateFunctionParameters,
		"parameter name '", name, "' used more than once.");
    declareLocal(name, fType->paramTypes()[i]);
  }

  beginBlock(generateUniqueBlockName());
}

void Assembler::endFunction(API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();  
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_NO_SCOPE();

  endBlock();
  freeTempSlots();
 
  _currentFunction = nullptr;
}

Assembler::ScopeBuilder Assembler::scope(API_FUNC) {
  API_FUNC_BEGIN();
  return ScopeBuilder { *this, API_FWD };
}

void Assembler::beginScopeImpl(API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  _currentScope = &_currentFunction->createScope(_currentScope, _counters.scopeID++);
}

void Assembler::endScope(API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  // TODO: should this actually be a cache boundary?
  // Rather, cache entries should not be scoped?
  //  _cache.controlBoundary();
  freeScope(_currentScope);
  _currentScope = _currentScope->parent;
}



