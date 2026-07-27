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
  void write(Expr const &expr, SUGAR_FUNC);

} // sugar::acus::io

#include "acus/sugar/sugar_libraryfunction.h"
#include "acus/sugar/sugar_io.h"
#include "acus/sugar/sugar_ascii.h"
#include "acus/sugar/sugar_math.h"
#include "acus/sugar/sugar_ansi.h"
#include "acus/sugar/sugar_string.h"

namespace acus::sugar {


  namespace io {
    template <size_t MaxSize> impl::ReadLine<MaxSize>
    readLine /* string<MaxSize> () */ {};

    template <typename Int, size_t MaxSize = 10> impl::ParseInt<Int, MaxSize>
    parseInt /* Int (string<MaxSize>) */ {};
    
  } // io

  namespace ascii {

    inline impl::IsDigit
    isDigit /* u8 (u8) */ {};

    inline impl::IsAlpha
    isAlpha /* u8 (u8) */ {};

    inline impl::IsAlphanumeric
    isAlphanumeric /* u8 (u8) */ {};

    inline impl::IsLower
    isLower /* u8 (u8) */ {};

    inline impl::IsUpper
    isUpper /* u8 (u8) */ {};

    inline impl::IsWhitespace
    isWhitespace /* u8 (u8) */ {};

    inline impl::ToLower
    toLower /* u8 (u8) */ {};

    inline impl::ToUpper
    toUpper /* u8 (u8) */ {};
    
  } // ascii

  // TODO: capitalize string type (also Ptr) and rename namespace to simply string
  namespace strings {

    template <size_t MaxSize> impl::ClearString<MaxSize>
    clear /* void (string<MaxSize>) */ {};

    template <size_t DestSize, size_t SrcSize> impl::AppendString<DestSize, SrcSize>
    append /* void (string<DestSize>, string<SrcSize>) */ {};

    template <size_t DestSize, size_t SrcSize> impl::AppendStringCopy<DestSize, SrcSize>
    append_to_copy /* string<DestSize> (string<DestSize>, string<SrcSize>) */ {};

    template <typename Int, size_t MaxSize = 10> impl::StrLen<Int, MaxSize>
    strlen /* Int (string<MaxSize>) */  {};
    
    template <size_t LhsSize, size_t RhsSize> impl::StringCompare<LhsSize, RhsSize>
    strcmp /* s8 (string<LhsSize>, string<RhsSize>) */ {};

    template <size_t StringSize, size_t PrefixSize> impl::StartsWith<StringSize, PrefixSize>
    starts_with /* u8 (string<StringSize>, string<PrefixSize>) */ {};

    template <size_t StringSize, size_t SuffixSize> impl::EndsWith<StringSize, SuffixSize>
    ends_with /* u8 (string<StringSize>, string<SuffixSize>) */ {};
    
    template <size_t StringSize> impl::FindChar<StringSize>
    find_char /* u16 (string<StringSize>, u8) */ {};

    template <size_t StringSize, size_t NeedleSize> impl::FindString<StringSize, NeedleSize>
    find_str /* u16 (string<StringSize>, string<NeedleSize>) */ {};

    template <size_t StringSize, size_t NeedleSize> impl::ContainsString<StringSize, NeedleSize>
    contains_str /* u16 (string<StringSize>, string<NeedleSize>) */ {};
  }
  
  namespace math {

    template <typename Int> impl::Min<Int>
    min /* Int (Int, Int) */ {};

    template <typename Int> impl::Max<Int>
    max /* Int (Int, Int) */ {};

    template <typename Int> impl::Pow<Int>
    pow /* Int (Int, Int) */ {};
    
    template <typename Int> impl::Sqrt<Int>
    sqrt /* Int (Int) */ {};

    template <size_t Base, typename Int> impl::Log<Base, Int>
    log /* u8 (Int) */ {};

    template <typename Int = u16> impl::Log<2, Int>
    log2 /* u8 (Int) */ {};

    template <typename Int = u16> impl::Log<10, Int>
    log10 /* u8 (Int) */ {};
    
  } // math


  namespace ansi {
    template <size_t Width, size_t Height, size_t Left = 1, size_t Top = 1>
    using Screen = impl::Screen<Width, Height, Left, Top>;

    // TODO: document interface
  }
  
} // acus::sugar

