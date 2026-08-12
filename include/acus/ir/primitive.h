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
    std::unordered_map<std::string, int> blockIDtoIndex;
    std::unordered_map<std::string, int> stackFrameSize;
    std::unordered_map<std::string, int> localBaseOffset;

    int getBlockIndex(std::string const &f, std::string const &b = "") const {      
      std::string const id = f + (b.empty() ? "" : (std::string(".") + b));
      assert(blockIDtoIndex.contains(id));
      return blockIDtoIndex.at(id);
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

    DInt value, current, scratch;
    
    /*
      Sets the value of the current cell a constant using the most
      optimal algorithm.
      Assumed initial pointer position: current
      Assumed empty: tmp
      Invariants: ptr, tmp
     */


    explicit ConstructConstant(DInt val, DInt current, DInt scratch):
      value(std::move(val)),
      current(std::move(current)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  
  struct ChangeBy: Node {
    DInt delta;
    /*
      Changes the value in the current cell by an amount 'delta'.
      
      Assumed initial pointer position: -
      Assumed empty: -      
      Invariants: -
    */
    
    inline explicit ChangeBy(DInt d): delta(std::move(d)) {}
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


  struct Boolean: Node {
    DInt current, scratch;
    /*
      Converts 'current' to bool (destructive).

      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
     */

    inline explicit Boolean(DInt current, DInt scratch):
      current(std::move(current)),
      scratch(std::move(scratch))
    {}
    
    COMMON_INTERFACE;
    MERGABLE;
  };
  
  struct Not: Node {
    DInt current, scratch;

    /*
      Not operator applied to the value stored at 'current'. Overwrites
      'current' with 0 if nonzero, 1 if zero (destructive).

      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
     */
    
    inline explicit Not(DInt current, DInt scratch):
      current(std::move(current)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };
  

  struct Or: Node {
    DInt current, other, scratch;

    /*
      OR operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current || other'.
      Consumes other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
      Clears: other
     */
    
    inline explicit Or(DInt current, DInt other, DInt scratch):
      current(std::move(current)),
      other(std::move(other)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct Xor: Node {
    DInt current, other, scratch1, scratch2;

    /*
      XOR operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current ^ other'.
      Consumes other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch1, scratch2
      Invariants: ptr, scratch1, scratch2
      Clears: other
     */
    
    inline explicit Xor(DInt current, DInt other, DInt scratch1, DInt scratch2):
      current(std::move(current)),
      other(std::move(other)),
      scratch1(std::move(scratch1)),
      scratch2(std::move(scratch2))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };
  
  struct And: Node {
    DInt current, other, scratch;

    /*
      AND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current && other'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
      Clears: other
      
     */
    
    inline explicit And(DInt current, DInt other, DInt scratch):
      current(std::move(current)),
      other(std::move(other)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct Less: Node {
    DInt current, other, scratch1, scratch2;

    /*
      NAND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current < other)'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch1, scratch2
      Invariants: ptr, scratch1, scratch2
      Clears: other
      
     */
    
    inline explicit Less(DInt current, DInt other, DInt scratch1, DInt scratch2):
      current(std::move(current)),
      other(std::move(other)),
      scratch1(std::move(scratch1)),
      scratch2(std::move(scratch2))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct LessOrEqual: Node {
    DInt current, other, scratch1, scratch2;

    /*
      NAND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current <= other)'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch1, scratch2
      Invariants: ptr, scratch1, scratch2
      Clears: other
      
     */
    
    inline explicit LessOrEqual(DInt current, DInt other, DInt scratch1, DInt scratch2):
      current(std::move(current)),
      other(std::move(other)),
      scratch1(std::move(scratch1)),
      scratch2(std::move(scratch2))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct Greater: Node {
    DInt current, other, scratch1, scratch2;

    /*
      NAND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current > other)'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch1, scratch2
      Invariants: ptr, scratch1, scratch2
      Clears: other
      
     */
    
    inline explicit Greater(DInt current, DInt other, DInt scratch1, DInt scratch2):
      current(std::move(current)),
      other(std::move(other)),
      scratch1(std::move(scratch1)),
      scratch2(std::move(scratch2))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct GreaterOrEqual: Node {
    DInt current, other, scratch1, scratch2;

    /*
      NAND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current >= other)'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Assumed empty: scratch1, scratch2
      Invariants: ptr, scratch1, scratch2
      Clears: other
      
     */
    
    inline explicit GreaterOrEqual(DInt current, DInt other, DInt scratch1, DInt scratch2):
      current(std::move(current)),
      other(std::move(other)),
      scratch1(std::move(scratch1)),
      scratch2(std::move(scratch2))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };


  struct Equal: Node {
    DInt current, other;

    /*
      NAND operator applied to the values stored at current and other. Overwrites
      the value at 'current' with the result of 'current == other)'. Consumes
      other as well (other == 0 afterwards).
      
      Assumed initial pointer position: current
      Invariants: ptr
      Clears: other
      
     */
    
    inline explicit Equal(DInt current, DInt other):
      current(std::move(current)),
      other(std::move(other))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };
    
  
  struct Cmp: Node {
    DInt value, current, scratch; 
    
    /*
      Compares the value stored at 'current' to 'value'. Stores 1 in 'result'
      if equal, 0 otherwise.
      
      Assumed initial pointer position: current
      Assumed empty: scratch
      Invariants: ptr, scratch
     */
    
    inline explicit Cmp(DInt value, DInt current, DInt scratch):
      value(std::move(value)),
      current(std::move(current)),
      scratch(std::move(scratch))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };


  struct Add: Node {
    DInt current, other;

    /*
      ADD operator: add the value stored at other to current. Overwrites
      the value at 'current' with the result of 'current + other' while
      consuming other (left 0).
      
      Assumed initial pointer position: current
      Clears: other
     */
    
    inline explicit Add(DInt current, DInt other):
      current(std::move(current)),
      other(std::move(other))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };

  struct Subtract: Node {
    DInt current, other;

    /*
      SUB operator: subtract the value stored at other from current. Overwrites
      the value at 'current' with the result of 'current - other' while consuming
      other (left 0).
      
      Assumed initial pointer position: current
      Clears: other
     */
    
    inline explicit Subtract(DInt current, DInt other):
      current(std::move(current)),
      other(std::move(other))
    {}

    COMMON_INTERFACE;
    MERGABLE;
  };
  
} // namespace acus::ir



