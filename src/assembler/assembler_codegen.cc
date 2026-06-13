// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

std::string Assembler::defaultOpenTag() {
  static int count = 0;
  return std::string("open_loop_") + std::to_string(count++);
} 

std::string Assembler::defaultCloseTag() {
  static int count = 0;
  return std::string("close_loop_") + std::to_string(count++);
}

int Assembler::getFieldIndex(int offset, int field) {
  return offset * MacroCell::FieldCount + field;
}

int Assembler::getFieldIndex(Cell cell) {
  return getFieldIndex(cell.offset, cell.field);
}

void Assembler::setTargetSequence(primitive::Sequence *seq) {
  _currentSeq = seq;
}

std::string Assembler::builtinFunctionName(BuiltinFunction func) {
  switch (func) {
  case BuiltinFunction::PrintUnsigned8:   return "__print_u8";
  case BuiltinFunction::PrintUnsigned16:  return "__print_u16";
  case BuiltinFunction::PrintSigned8:     return "__print_s8";
  case BuiltinFunction::PrintSigned16:    return "__print_s16";
  }
  std::unreachable();
}

void Assembler::constructBuiltinFunctions() {
  assert(_currentFunction == nullptr);
  assert(_currentBlock == nullptr);

  for (auto func: _usedBuiltinFunctions) {

    types::TypeHandle const paramType = [&] {
      switch (func) {
      case BuiltinFunction::PrintUnsigned8:  return ts::u8();
      case BuiltinFunction::PrintUnsigned16: return ts::u16();
      case BuiltinFunction::PrintSigned8:    return ts::s8();
      case BuiltinFunction::PrintSigned16:   return ts::s16();
      }
      std::unreachable();
    }();

    std::string const funcName = builtinFunctionName(func);
    
    function(funcName).param("x", paramType).begin(); {
      std::optional<Slot> slot = localSlot("x");
      assert(slot.has_value());
      printDecimal(Expression{ *slot });
      returnFromFunction();
    } endFunction();
    
  }

  _usedBuiltinFunctions.clear();
}

primitive::Context Assembler::constructContext() const {

  auto const constructBlockIDtoIndexMap = [&](){
    std::unordered_map<std::string, int> result;
    for (auto const &f: _program.functions) {
      // Special block ID equal to the first block of a function to indicate
      // the entry-point of this function (easy lookup).
      assert(f.blocks.size() > 0);
      result[f.name] = f.blocks[0]->globalBlockIndex;
      for (auto const &b: f.blocks) {
	std::string const id = f.name + "." + b->name;
	result[id] = b->globalBlockIndex;
      }
    }
    return result;
  };
  
  auto const constructStackFrameSizeMap = [&](){
    std::unordered_map<std::string, int> result;
    for (auto const &f: _program.functions) {
      result[f.name] = f.frame.totalLogicalCells();
    }
    return result;
  };

  auto const constructLocalBaseOffsetMap = [&](){
    std::unordered_map<std::string, int> result;
    for (auto const &f: _program.functions) {
      result[f.name] = f.frame.localBase();
    }
    return result;
  };
  
  return primitive::Context {
    .fieldCount = MacroCell::FieldCount,
    .blockIDtoIndex = constructBlockIDtoIndexMap(),
    .stackFrameSize = constructStackFrameSizeMap(),
    .localBaseOffset = constructLocalBaseOffsetMap()
  };
}


primitive::Sequence Assembler::compilePrimitives(API_CTX) {
  primitive::Sequence result = _program.bootstrap;
  for (auto &fn: _program.functions) {
    checkFunctionFlowValidity(fn, API_FWD);  
    for (auto const &block:  fn.blocks) {
      if (block->reachable) result.append(block->code);
    }
  }
  result.append(_program.hatstrap);
  return result;
}

std::string Assembler::brainfuck(std::string const &name) const {
  // TODO: API_REQUIRE
  return simplifyBrainfuck(_bf.at(name));
}

std::string Assembler::primitives(std::string const &name) const {
  // TODO: API_REQUIRE
  return _txt.at(name);
}



primitive::Sequence Assembler::simplifySequence(primitive::Sequence const &seq) {
  primitive::Sequence result;
  if (seq.nodes.empty()) return result;

  result.nodes.push_back(seq.nodes[0]);
  for (size_t next = 1; next != seq.nodes.size(); ++next) {
    auto n0 = result.nodes.back();
    auto n1 = seq.nodes[next];

    if (auto merged = n0->merge(n1.get())) {
      result.nodes.back() = merged;
    }
    else {
      result.nodes.push_back(n1);
    }
  }
  return result;
}


// Compress BF string by cancelling opposite commands
std::string Assembler::simplifyBrainfuck(std::string const &bf) {
  auto cancel = [](std::string const &input, char const up, char const down) -> std::string {
    std::string result;
    int count = 0;

    auto flush = [&]() {
      if (count > 0) result += std::string( count, up);
      if (count < 0) result += std::string(-count, down);
      count = 0;
    };

    for (char c: input) {
      if (c == up)   ++count;
      else if (c == down) --count;
      else {
	flush();
	result += c;
      }
    }
    
    flush();
    return result;
  };
  
  return cancel(cancel(bf, '>', '<'), '+', '-');
}


void Assembler::checkFunctionFlowValidity(Function &fn, API_CTX) {
  
  // Check if all blocks of the function can be reached and if all paths
  // end up at a return

  auto getBlock = [&fn](std::string const &name) -> Function::Block* {
    for (auto &b: fn.blocks) {
      if (b->name == name) return b.get();
    }
    return nullptr;
  };
  
  auto markReachabilityAndCheckReturnPaths = [&](Function::Block &b) -> void {
    auto recurse = [&](auto&& self, Function::Block* b) -> void {
      if (b == nullptr) return;
      if (b->reached) return;
      b->reached = true;
  
      if (b->children.size() == 0) {
	API_REQUIRE(b->returns,
		    error::ErrorCode::ExecutionPathWithoutReturn,
		    "function '", fn.name, "' terminates in block labeled '", b->name, "' without a return-statement.");
	return;
      }

      for (auto const &child: b->children) {
	self(self, getBlock(child.blockName));
      }
    };

    recurse(recurse, &b);
  };

  assert(fn.blocks.size() > 0);
  assert(fn.blocks[0].get() != nullptr);
  markReachabilityAndCheckReturnPaths(*fn.blocks[0]);

  for (auto const &b: fn.blocks) {
    if (not b->name.starts_with("__")) { // Skip auto-generated blocks that may be empty
      // TODO: only error when option is active
      API_REQUIRE(b->reached || not b->reachable, error::ErrorCode::UnreachableCodeSection,
		  "function '", fn.name, "' contains an unreachable code section labeled '", b->name, "'.");
    }
  }
}
