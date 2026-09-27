// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <memory>
#include <utility>
#include "acus/ir/defer.h"

namespace acus::primitive {

  struct Context {
    
    int fieldCount;

    std::unordered_map<std::string, int> blockIDToDispatchIndex;
    std::unordered_map<std::string, int> stackFrameSize;
    std::unordered_map<std::string, int> localBaseOffset;

    int getDispatchIndex(std::string const &f, std::string const &b = "") const {      
      std::string const id = f + (b.empty() ? "" : (std::string(".") + b));
      assert(blockIDToDispatchIndex.contains(id));
      return blockIDToDispatchIndex.at(id);
    }

    int getStackFrameSize(std::string const &f) const {
      assert(stackFrameSize.contains(f));
      return stackFrameSize.at(f);
    }

    int getLocalBaseOffset(std::string const &f) const {
      assert(localBaseOffset.contains(f));
      return localBaseOffset.at(f);
    }
  };

  using DInt = defer::Int<Context>;
  
  struct Node {
    virtual ~Node() = default;
    virtual std::string text(Context const&) const = 0;
    virtual std::string generate(Context const&) const = 0;
    virtual std::shared_ptr<Node> merge(Node const *) const { return nullptr; }
  };

  struct Sequence {
    std::vector<std::shared_ptr<Node>> nodes;

    template <typename T, typename... Args>
    void emplace(Args&&... args) {
      nodes.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }

    inline void append(Sequence const &other) {
      for (auto const &n : other.nodes) {
	nodes.push_back(n);
      }
    }
    
    inline size_t size() const { return nodes.size(); }
    
    std::string dumpText(Context const &ctx);
    std::string dumpCode(Context const &ctx);
  };


  enum Direction { Left, Right };
  
  // ========= Primitive Nodes =============

#define COMMON_INTERFACE				 \
  virtual std::string text(Context const&) const override;	 \
  virtual std::string generate(Context const&) const override;	 \

#define MERGABLE \
  virtual std::shared_ptr<Node> merge(Node const *other) const override;
  
  
  struct Comment: Node {
    std::string txt;
    inline explicit Comment(std::string txt): txt(std::move(txt)) {}
    COMMON_INTERFACE;
  };
  
  struct LoopOpen: Node {
    std::string tag;
    inline explicit LoopOpen(std::string tag): tag(std::move(tag)) {}
    COMMON_INTERFACE;
  };

  struct LoopClose: Node {
    std::string tag;
    inline explicit LoopClose(std::string tag): tag(std::move(tag)) {}
    COMMON_INTERFACE;
    MERGABLE;
  };

  struct Inline : Node {
    std::string code;
    inline explicit Inline(std::string code) : code(std::move(code)) {}
    COMMON_INTERFACE;
  };

  struct MovePointerRelative: Node {
    DInt amount = 0;

    /*
      Moves the pointer by a fixed amount.
      .
      Assumed initial pointer position: -
      Assumed empty: -
      Invariants: -
     */
    
    inline explicit MovePointerRelative(DInt amount): amount(std::move(amount)) {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  
  struct ZeroCell: Node {

    /*
      Sets the value of the current cell to 0.
      Assumed initial pointer position: -
      Assumed empty: -      
      Invariants: -
     */

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct ZeroCellPlus: Node {

    /*
      Sets the value of the current cell to 0 by incrementing it.
      Assumed initial pointer position: -
      Assumed empty: -      
      Invariants: -
     */
    
    COMMON_INTERFACE;
    MERGABLE;
  };

  struct ConstructConstant: Node {

    bool naive;
    DInt value, current, scratch;
    
    /*
      Sets the value of the current cell a constant using the most
      optimal algorithm.
      Assumed initial pointer position: current
      Assumed empty: tmp
      Invariants: ptr, tmp
     */


    explicit ConstructConstant(DInt val):
      naive(true),
      value(std::move(val)),
      current(0),
      scratch(0)
    {}
    
    explicit ConstructConstant(DInt val, DInt current, DInt scratch):
      naive(false),
      value(std::move(val)),
      current(std::move(current)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  
  struct ChangeBy: Node {

    bool naive;
    DInt delta, current, scratch;

    /*
      Changes the value in the current cell by an amount 'delta'.
      
      Assumed initial pointer position: -
      Assumed empty: -      
      Invariants: -
    */
    
    explicit ChangeBy(DInt delta):
      naive(true),
      delta(std::move(delta)),
      current(0),
      scratch(0)
    {}
    
    explicit ChangeBy(DInt delta, DInt current, DInt scratch):
      naive(false),
      delta(std::move(delta)),
      current(std::move(current)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct MoveData: Node {
    DInt current, dest;

    /*
      Moves the value stored in 'current' into 'dest'.
      This leaves 'current' empty!
      
      Assumed initial pointer position: current
      Assumed empty: -      
      Invariants: ptr
     */
    
    inline explicit MoveData(DInt diff):
      current(0),
      dest(std::move(diff))
    {}
    
    inline explicit MoveData(DInt current, DInt dest):
      current(std::move(current)),
      dest(std::move(dest))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };
  
  struct CopyData: Node {
    DInt current, dest, scratch;

    /*
      Copies the value stored in 'current' into 'dest'.
      
      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
     */
    
    inline explicit CopyData(DInt current, DInt dest, DInt scratch):
      current(std::move(current)),
      dest(std::move(dest)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct In: Node {
    COMMON_INTERFACE;
  };
  
  struct Out: Node {
    COMMON_INTERFACE;
  };


  
} // namespace acus::ir



