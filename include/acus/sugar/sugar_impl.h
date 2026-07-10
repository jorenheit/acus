// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/sugar/sugar_loc.h"
#include "acus/sugar/sugar_types.h"
#include <stack>
#include <functional>

namespace acus::sugar::impl {
  
  
  inline literal::Literal toLiteral(int x) {
    if (x >= 256) return literal::u16(x);
    if (x >= 0)   return literal::u8(x);
    if (x < -127) return literal::s16(x);
    return literal::s8(x);
  }

  inline literal::Literal toLiteral(int x, types::TypeHandle type) {
    if (not types::isInteger(type)) {
      assert(types::isPointer(type));
      return literal::u16(x);
    }
    
    if (type == ts::u8())  return literal::u8(x);
    if (type == ts::u16()) return literal::u16(x);
    if (type == ts::s8())  return literal::s8(x);
    if (type == ts::s16()) return literal::s16(x);
    std::unreachable();
  }

  inline literal::Literal toLiteral(literal::Literal lit) {
    return lit;
  }
  
  template <typename T> requires IsSugarType<T>
  literal::Literal toLiteral(T const &val) {
    return val.toLiteral();
  }



  std::string nextLabel();
  std::string nextVarName();

  class ControlStack {

    static std::stack<std::string> _continueStack;
    static std::stack<std::string> _breakStack;

  public:
    
    static std::string getContinueLabel() {
      assert(not _continueStack.empty());
      return _continueStack.top();
    }

    static std::string getBreakLabel() {
      assert(not _breakStack.empty());
      return _breakStack.top();
    }

    static void pop() {
      assert(not _continueStack.empty());      
      assert(not _breakStack.empty());
      _breakStack.pop();
      _continueStack.pop();
    }

    static void push(std::string const &continueLabel, std::string const &breakLabel) {
      _continueStack.push(continueLabel);
      _breakStack.push(breakLabel);
    }
    
  };


  class FunctionDefinition {
    std::source_location _loc;
    std::function<void()> _body;
  public:
    FunctionDefinition(SUGAR_FUNC): _loc(LOC_FWD) {}
    
    template <typename Body>
    FunctionDefinition &operator<<(Body&& body) {
      _body = std::forward<Body>(body);
      return *this;
    }

    void operator()() const {
      _body();
    }

    std::source_location const &loc() const {
      return _loc;
    }

  };

  class FunctionDeclaration {
    std::source_location _loc;
  public:
    FunctionDeclaration(SUGAR_FUNC): _loc(LOC_FWD) {}
  };
  
  
} // impl
