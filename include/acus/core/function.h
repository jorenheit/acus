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
#include <memory>

#include "acus/core/frame.h"
#include "acus/ir/primitive.h"
#include "acus/types/types_fwd.h"

namespace acus {

// ============================================================
// Function
// ============================================================

struct Function {
  
  struct Block {
    size_t globalBlockIndex = 0;
    size_t parentFunctionIndex = 0;
    bool isEntryPoint = false;
    std::string name;
    std::string id;
    primitive::Sequence code;

    bool returns = false;
    bool reached = false;
    bool reachable = true;
    
    struct Child {
      std::string functionName;
      std::string blockName;
    };

    std::vector<Child> children;
  };

  struct Scope {
    Scope *parent = nullptr;
    size_t id = -1;
  };
  
  size_t functionIndex = 0;
  std::string name;
  FrameLayout frame;
  types::FunctionType const *type;
  
  std::vector<std::unique_ptr<Block>> blocks;
  std::unordered_map<std::string, size_t> blockByName;
  std::vector<std::unique_ptr<Scope>> scopes;
  
  size_t entryBlockIndex = 0;

  inline Block &createBlock(std::string blockName) {
    assert(!blockByName.contains(blockName));
    blockByName[blockName] = blocks.size();
    blocks.push_back(std::make_unique<Block>());

    
    Block &b = *blocks.back();
    b.globalBlockIndex = -1; 
    b.parentFunctionIndex = functionIndex;
    b.name = std::move(blockName);
    b.id = name + "." + b.name;
    b.isEntryPoint = (blocks.size() == 1);
    return b;
  }

  inline Block &block(std::string const& name) {
    return *blocks.at(blockByName.at(name));
  }

  inline Block const &block(std::string const& name) const {
    return *blocks.at(blockByName.at(name));
  }

  inline bool isBlockDefined(std::string const &b) const {
    return blockByName.find(b) != blockByName.end();
  }

  inline Scope &createScope(Scope *parent, size_t scopeID) {
    bool valid = false;
    for (auto const &scope: scopes) {
      if (scope.get() == parent) {
	valid = true;
	break;
      }
    }
    assert(valid || parent == nullptr);
    scopes.emplace_back(std::make_unique<Scope>(parent, scopeID));
    return *scopes.back();
  }
};

} // namespace acus
