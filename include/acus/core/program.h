// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cassert>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

#include "acus/core/function.h"
#include "acus/core/slot.h"
#include "acus/ir/primitive.h"
#include "acus/types/types.h"

namespace acus {

// ============================================================
// Program
// ============================================================

struct Program {

  primitive::Sequence bootstrap;
  primitive::Sequence hatstrap;
  std::string name;
  std::string entryFunctionName;
  
  std::vector<Function> functions;
  std::unordered_map<std::string, size_t> functionByName;
  std::vector<Slot> globals;
  std::vector<Function::Block*> globalBlockOrder;

  inline Function& createFunction(std::string name, types::FunctionType const *type) {
    assert(!functionByName.contains(name));
    size_t idx = functions.size();
    functionByName[name] = idx;
    functions.push_back(Function{
	.functionIndex = idx,
	.name   = std::move(name),
	.frame  = FrameLayout{type->returnType()->size()},
	.type   = type
      });
    functions.back().createScope(nullptr);
    return functions.back();
  }
  
  inline Function &function(std::string const &name) {
    return functions.at(functionByName.at(name));
  }

  inline Function const &function(std::string const &name) const {
    return functions.at(functionByName.at(name));
  }

  inline void registerBlock(Function::Block &block) {
    block.globalBlockIndex = globalBlockOrder.size();
    globalBlockOrder.push_back(&block);
  }

  inline int globalVariableFrameSize() const {
    int result = 0;
    for (auto const &slot: globals) result += slot.size();
    return result;
  }

  inline bool isFunctionDefined(std::string const &f) const {
    return (functionByName.find(f) != functionByName.end());
  }

  inline int globalIndex(std::string const &name) const {
    for (size_t i = 0; i != globals.size(); ++i) if (globals[i].name() == name) return i;
    return -1;
  }

  inline bool isGlobal(std::string const &name) const {
    return globalIndex(name) != -1;
  }
  
  inline Slot globalSlot(std::string const &name) const {
    int idx = globalIndex(name); assert(idx != -1);
    return globals[idx];
  }
};

} // namespace acus
