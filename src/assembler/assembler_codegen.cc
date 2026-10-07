// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <regex>
#include <set>
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
    default: std::unreachable(); return "";
  }
  std::unreachable();
}

void Assembler::constructBuiltinFunctions() {
  assert(_currentFunction == nullptr);
  assert(_currentBlock == nullptr);

  for (auto func: _usedBuiltinFunctions) {

    types::TypeHandle const paramType = [&] -> types::TypeHandle {
      switch (func) {
        case BuiltinFunction::PrintUnsigned8:  return ts::u8();
        case BuiltinFunction::PrintUnsigned16: return ts::u16();
        case BuiltinFunction::PrintSigned8:    return ts::s8();
        case BuiltinFunction::PrintSigned16:   return ts::s16();
        default: std::unreachable(); return types::null;
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

primitive::Context Assembler::constructStraightLineContext() const {
  primitive::Context ctx{
    .fieldCount = MacroCell::FieldCount,
    .blockIDToDispatchIndex = {},
    .stackFrameSize = {},
    .localBaseOffset = {}
  };
  
  for (auto const &function : _program.functions) {
    ctx.stackFrameSize.emplace(function.name, function.frame.totalLogicalCells());
    ctx.localBaseOffset.emplace(function.name, function.frame.localBase());
  }

  return ctx;
}

primitive::Context Assembler::constructContext(std::vector<Function::Block *> const &dispatchBlocks, int innerSwitchCaseCount) const {

  auto const constructBlockIDToDispatchIndexMap = [&]{
    std::unordered_map<std::string, int> result;
    for (size_t i = 0; i != dispatchBlocks.size(); ++i) {
      Function::Block const *block = dispatchBlocks[i];

      int const outer = i / innerSwitchCaseCount + 1;
      int const inner = i % innerSwitchCaseCount;

      size_t const dispatchIndex = (outer << 8) | inner;
      result[block->id] = dispatchIndex;

      if (block->isEntryPoint) {
        std::string const &functionName = _program.functions[block->parentFunctionIndex].name;
        result[functionName] = dispatchIndex;
      }
    }
    return result;
  };

  auto const constructStackFrameSizeMap = [&]{
    std::unordered_map<std::string, int> result;
    for (auto const &f: _program.functions) {
      result[f.name] = f.frame.totalLogicalCells();
    }
    return result;
  };

  auto const constructLocalBaseOffsetMap = [&]{
    std::unordered_map<std::string, int> result;
    for (auto const &f: _program.functions) {
      result[f.name] = f.frame.localBase();
    }
    return result;
  };

  return primitive::Context {
    .fieldCount = MacroCell::FieldCount,
    .blockIDToDispatchIndex = constructBlockIDToDispatchIndexMap(),
    .stackFrameSize = constructStackFrameSizeMap(),
    .localBaseOffset = constructLocalBaseOffsetMap()
  };
}

std::string Assembler::brainfuck(std::string const &name, API_FUNC) const {
  API_FUNC_BEGIN();
  API_REQUIRE(_bf.contains(name), acus::error::ErrorCode::NoSuchProgram, "program '", name, "' does not exist.");
  return simplifyBrainfuck(_bf.at(name));
}

std::string Assembler::primitives(std::string const &name, API_FUNC) const {
  API_FUNC_BEGIN();
  API_REQUIRE(_bf.contains(name), acus::error::ErrorCode::NoSuchProgram, "program '", name, "' does not exist.");
  return _txt.at(name);
}

void Assembler::mergeSequence(primitive::Sequence &seq) {
  if (seq.nodes.empty()) return;
  
  primitive::Sequence merged;

  while (not seq.nodes.empty()) {
    merged.nodes.push_back(seq.nodes[0]);
    for (size_t next = 1; next != seq.nodes.size(); ++next) {
      auto n0 = merged.nodes.back();
      auto n1 = seq.nodes[next];

      if (auto mergeResult = n0->merge(n1.get())) {
        merged.nodes.back() = mergeResult;
      }
      else {
        merged.nodes.push_back(n1);
      }
    }

    std::swap(merged.nodes, seq.nodes);
    if (merged.nodes.size() == seq.nodes.size()) return;
    merged.nodes.clear();
  }

  std::unreachable();
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

  auto cleanHead = [](std::string const &input) -> std::string {
    // Remove all unnecessary [-] sequences before the state cannot be traced
    // trivially anymore (at first nontrivial [...])
    std::set<std::ptrdiff_t> unknown;
    std::ptrdiff_t current = 0;
    std::string result;
    result.reserve(input.size());
    for (size_t idx = 0; idx < input.size(); ++idx) {
      switch (input[idx]) {
        case '<': --current; break;
        case '>': ++current; break;
        case '+':
        case '-':
        case ',':
          unknown.insert(current);
          break;
        case '[':
          if (input.compare(idx, 3, "[-]") == 0) {
            if (unknown.erase(current)) result += "[-]";
            idx += 2;
            continue;
          }
          // Preserve everything from the first other loop onward.
          result.append(input, idx, std::string::npos);
          return result;
        default:
          break;
      }
      result += input[idx];
    }

    return result;
  };

  auto cleanTail = [](std::string const &input) -> std::string {
    size_t end = input.size();

    while (end > 0) {
      switch (input[end - 1]) {
        case '<':
        case '>':
        case '+':
        case '-': --end; break;
        case ']': {
          if (end >= 3 && input.compare(end - 3, 3, "[-]") == 0) {
            end -= 3;
            break;
          }
          // Fall through
        }
        default: return input.substr(0, end);
      }
    }

    return {};
  };  
  
  std::string result = cancel(cancel(bf, '>', '<'), '+', '-');
  result = std::regex_replace(result, std::regex(R"(\]\[-\])"), "]");
  result = cleanHead(result);
  result = cleanTail(result);
  return result;
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
