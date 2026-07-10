// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"

namespace acus::sugar::io {
  // Builtin functions
  
  void print(char c, SUGAR_FUNC);
  void print(int x, SUGAR_FUNC);
  void print(Expr const &expr, SUGAR_FUNC);

  void println(SUGAR_FUNC);
  void println(auto&& arg, SUGAR_FUNC) {
    print(std::forward<decltype(arg)>(arg), LOC_FWD);
    print('\n', LOC_FWD);
  }
  
  void read(Expr const &expr, SUGAR_FUNC);

  
  template <size_t MaxSize>
  struct ReadLine {
    static Expr operator()(Expr const &result) {
      __assembler.scope().begin();
      {
	auto i = (let<u8>("i") = 0);
	while_(i < MaxSize) {
	  read(result[i]);
	  if_(result[i++] == '\n') { break_; };
	};
	result[i - 1] = 0;
      }
      __assembler.endScope();
      return result;
    }
    
    static Expr operator()() {
      auto result = let<string<MaxSize>>(impl::nextVarName());
      return ReadLine<MaxSize>::operator()(result);
    }

    static auto outline(std::string const &name = "readLine", SUGAR_FUNC) {
      return function_<string<MaxSize>()>(name) | define {
	return__(ReadLine<MaxSize>::operator()(), LOC_FWD);
      };
    }
  };

  template <size_t MaxSize>
  ReadLine<MaxSize> readLine{};


  template <typename IntType, size_t MaxSize> requires impl::IsSugarType<IntType>
  struct StrLen {
    
    static Expr operator()(Expr const &str) {
      assert(types::isString(str.get().type()));

      std::string result = impl::nextVarName();
      let<IntType>(result) = 0;
      while_(str[var(result)++] != 0) { };
      return var(result) - 1;
    }

    static auto outline(std::string const &name = "strlen", SUGAR_FUNC) {
      return function_<IntType(string<MaxSize>)>(name, "str") | define {
	return__(StrLen<IntType, MaxSize>::operator()(var("str")));
      };
    }

  };


  template <typename IntType = u8, size_t MaxSize = 64>
  StrLen<IntType, MaxSize> strlen;
  
  // Expr stringToInt(Expr const &str) {
  //   assert(types::isString(str.get().type()));

  //   let<u16>("base") = 1;
  //   for_(let<u8>("i") = 0, var<"i"> != str.get().type()->size(), 
    
  // }
}
