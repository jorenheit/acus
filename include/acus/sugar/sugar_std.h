// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <tuple>
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"


  // Builtin functions
namespace acus::sugar::io {

  // TODO: underscores to indicate builtins
  
  void print(char c, SUGAR_FUNC);
  void print(int x, SUGAR_FUNC);
  void print(Expr const &expr, SUGAR_FUNC);

  void println(SUGAR_FUNC);

  void println(auto&& arg, SUGAR_FUNC) {
    print(std::forward<decltype(arg)>(arg), LOC_FWD);
    print('\n', LOC_FWD);
  }
  
  void read(Expr const &expr, SUGAR_FUNC);

} // sugar::acus::io

#include "acus/sugar/sugar_libraryfunction.h"
#include "acus/sugar/sugar_io.h"
#include "acus/sugar/sugar_ascii.h"
#include "acus/sugar/sugar_math.h"

namespace acus::sugar {


  namespace io {
    template <size_t MaxSize> impl::ReadLine<MaxSize>
    readLine {};

    template <typename IntType = u8, size_t MaxSize = 10> impl::StrLen<IntType, MaxSize>
    strlen {};

    template <typename IntType = u8, size_t MaxSize = 10> impl::ParseInt<IntType, MaxSize>
    parseInt {};

    // Function returning void (test)
    template <size_t MaxSize = 10> impl::Hello<MaxSize>
    hello {};
    
  } // io

  namespace ascii {

    inline impl::IsDigit
    isDigit {};

    inline impl::IsAlpha
    isAlpha {};

    inline impl::IsAlphanumeric
    isAlphanumeric {};
    
  } // ascii
  
  namespace math {

    template <typename IntType> impl::Min<IntType>
    min {};

    template <typename IntType> impl::Max<IntType>
    max {};
    
    template <typename IntType> impl::Sqrt<IntType>
    sqrt {};

    template <size_t Base, typename IntType = u16> impl::Log<Base, IntType>
    log {};

    template <typename IntType = u16> impl::Log<2, IntType>
    log2 {};

    template <typename IntType = u16> impl::Log<10, IntType>
    log10 {};
    
  } // math
  
  
} // acus::sugar

