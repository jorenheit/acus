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

    static auto outline(SUGAR_FUNC) {
      std::string const name = "__readLine<" + std::to_string(MaxSize) + ">";
      return function_<string<MaxSize>()>(name) | define {
	return__(ReadLine<MaxSize>::operator()(), LOC_FWD);
      };
    }
  };

  template <size_t MaxSize>
  ReadLine<MaxSize> readLine {};


  template <typename IntType, size_t MaxSize = 64> requires impl::IsSugarType<IntType>
  struct StrLen {
    
    static Expr operator()(Expr const &str) {
      assert(types::isString(str.get().type()));

      std::string result = impl::nextVarName();
      let<IntType>(result) = 0;
      while_(str[var(result)++] != 0) { };
      return var(result) - 1;
    }

    static auto outline(SUGAR_FUNC) {
      std::string const name = "__strlen<" + IntType::type()->str() + ", " + std::to_string(MaxSize) + ">";
      return function_<IntType(string<MaxSize>)>(name, "str") | define {
	return__(StrLen<IntType>::operator()(var("str")));
      };
    }

  };

  template <typename IntType = u8, size_t MaxSize = 16>
  StrLen<IntType, MaxSize> strlen {};
  
  
  template <typename IntType, size_t MaxSize> requires impl::IsSugarType<IntType>
  struct StringToInt {
    static_assert(IsInteger<IntType>, "StringToInt requires an integer result type");
    
    static Expr parse(Expr const &str) {
      std::string const resultName = impl::nextVarName();
      std::string const indexName = impl::nextVarName();

      let<IntType>(resultName) = IntType{0};
      let<u8>(indexName) = 0;

      std::string negativeName;
      if constexpr (IsSignedInteger<IntType>) {
        negativeName = impl::nextVarName();
        let<u8>(negativeName) = 0;

        if_(str[var(indexName)] == '-') {
          var(negativeName) = 1;
          ++var(indexName);
        } else_ {
          if_(str[var(indexName)] == '+') {
            ++var(indexName);
          };
        };
      }
      else {
        if_(str[var(indexName)] == '+') {
          ++var(indexName);
        };
      }

      while_(str[var(indexName)] >= '0' &&
             str[var(indexName)] <= '9') {
        var(resultName) *= IntType{10};
        var(resultName) +=
          (str[var(indexName)] - '0').template cast<IntType>();
        ++var(indexName);
      };

      if constexpr (IsSignedInteger<IntType>) {
        if_(var(negativeName)) {
          var(resultName) = -var(resultName);
        };
      }

      return var(resultName);
    }

    static Expr operator()(Expr const &str) {
      assert(types::isString(str.get().type()));

      auto const stringType = types::cast<types::StringType>(str.get().type());
      assert(stringType->length() - 1 <= 255 &&
             "StringToInt currently supports strings up to 255 characters");
      return parse(str);
    }

    static auto outline(std::string const &name = "stringToInt", SUGAR_FUNC) {
      return function_<IntType(string<MaxSize>)>(name, "str") | define {
        return__(StringToInt<IntType, MaxSize>::operator()(var("str")), LOC_FWD);
      };
    }
  };

  template <typename IntType = u8, size_t MaxSize = 16>
  StringToInt<IntType, MaxSize> stringToInt {};
}
