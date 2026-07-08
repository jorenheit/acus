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
	      
  

  // Generate the metablocks, builtin functions, bootstrap and hatstrap sequences.
  constructBuiltinFunctions();
  deferredFunctionCallTypeChecks();
  constructMetaBlocks();
  deferredLabelChecks();

  
  // To bootstrap the system, we need to do the following:
  // 1. Mark cell 0 using the SeekMarker field to indicate that this is where
  //    the global data frame starts, for easy navigation to this frame. 
  // 2. Move the pointer to the first stackframe and reset the origin.
  // 3. Initialize the first stack-frame, where we set the TargetBlock to the
  //    index of the entry-function and the run-state to 1. It is assigned
  //    FrameMarker 1, which from hereon will be copied and incremented for
  //    deeper frames.
  // 4. Open the main-loop and leave the pointer in cell 0 of the frame.

  setTargetSequence(&_program.bootstrap);

  resetOrigin();
  switchField(MacroCell::SeekMarker);
  setToValue(1);
  switchField(MacroCell::FrameMarker);
  setToValue(1);

  moveTo(1 + _program.globalVariableFrameSize()); 
  resetOrigin();
  switchField(MacroCell::FrameMarker);
  setToValue(1);

  setNextBlock(_program.entryFunctionName, "");
  moveTo(FrameLayout::RunState, MacroCell::Value0);
  setToValue(1);
  loopOpen("main loop");
  moveToOrigin();
  
  // Also generate the hatstrap code. All this needs to do is move the pointer to
  // the run-cell and close the loop.

  setTargetSequence(&_program.hatstrap);
  moveTo(FrameLayout::RunState, MacroCell::Value0);
  loopClose("main loop");

  _state.begun = false;

  // Store resulting code
  auto prog = simplifySequence(compilePrimitives(API_FWD));
  primitive::Context ctx = constructContext();  
  _txt[_program.name] = prog.dumpText(ctx);
  _bf[_program.name] = simplifyBrainfuck(prog.dumpCode(ctx));
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
  _currentFunction = &_program.createFunction(name, fType);
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

  _currentScope = &_currentFunction->createScope(_currentScope);
}

void Assembler::endScope(API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  // TODO: should this actually be a cache boundary?
  // Rather, cache entries should not be scoped?
  _cache.internalBoundary();
  freeScope(_currentScope);
  _currentScope = _currentScope->parent;
}



