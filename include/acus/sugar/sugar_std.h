// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later


/*
 * Acus Sugar standard library
 * ---------------------------
 *
 * Library functions are exposed as stateless callable objects grouped into
 * namespaces such as io, ascii, string, math and ansi.
 *
 * A library function can be called directly, e.g.
 *
 *   let<u8>("c") = ascii::to_lower(var_("x"));
 *
 * Template arguments configure the function's types or capacities. The
 * expected Acus signature is documented beside each public object. For
 * example, the sqrt-function needs to know its expected type
 *
 *   template <typename Int> impl::Sqrt<Int>
 *   sqrt {}; 
 *
 * This means that `math::sqrt<u16>` behaves as a function with signature
 *
 *   u16 (u16)
 *
 * The signatures for each of the functions are visible in comments below.
 *
 * The returned Expr may be stored in a C++ variable. This variable is only a
 * handle to the generated Acus expression or storage:
 *
 *   auto length = string::strlen<u16, 20>(text);
 *   println(length);
 *
 *
 * Inline and outlined use
 * -----------------------
 *
 * By default, a library function is emitted inline at the call site.
 * Inline calls avoid Acus function-call overhead, but repeated calls duplicate
 * the generated implementation.
 *
 * Outlinable functions can instead be emitted once as ordinary Acus functions:
 *
 *   auto parse_int = string::string_to_int<s16, 10, 20>.outline();
 *   auto square_root = math::sqrt<u16>.outline();
 *
 * The returned handles can then be called like regular function handles:
 *
 *   let<u16>("x") = parse_int(text);
 *
 * Outlining can substantially reduce generated program size when a function is
 * used repeatedly, but introduces function-call and argument-copying overhead.
 * Whether inline or outlined code is preferable therefore depends on the
 * function and its usage.
 *
 * Functions marked "not outlinable" modify storage supplied by the caller:
 *
 *   string::clear
 *   string::append
 *
 * Such functions must be used inline and do not provide outline().
 *
 */


#pragma once
#include <tuple>
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"

#include "acus/sugar/sugar_libraryfunction.h"
#include "acus/sugar/sugar_ascii.h"
#include "acus/sugar/sugar_io.h"
#include "acus/sugar/sugar_math.h"
#include "acus/sugar/sugar_ansi.h"
#include "acus/sugar/sugar_string.h"
#include "acus/sugar/sugar_array.h"
#include "acus/sugar/sugar_config.h"


namespace acus::sugar {

  // Builtin functions
  void print(char c, SUGAR_FUNC);
  void print(int x, SUGAR_FUNC);
  void print(Expr const &expr, SUGAR_FUNC);
  void println(auto&& arg, SUGAR_FUNC);  
  void println(SUGAR_FUNC);
  void read(Expr const &expr, SUGAR_FUNC);
  void put(Expr const &expr, SUGAR_FUNC);

  
  // Library functions
  namespace io {
    template <size_t MaxSize> impl::ReadLine<MaxSize>
    read_line /* string<MaxSize> () */ {};
    
  } // io

  namespace ascii {

    inline impl::IsDigit
    is_digit /* u8 (u8) */ {};

    inline impl::IsAlpha
    is_alpha /* u8 (u8) */ {};

    inline impl::IsAlphanumeric
    is_alphanumeric /* u8 (u8) */ {};

    inline impl::IsLower
    is_lower /* u8 (u8) */ {};

    inline impl::IsUpper
    is_upper /* u8 (u8) */ {};

    inline impl::IsWhitespace
    is_whitespace /* u8 (u8) */ {};

    inline impl::ToLower
    to_lower /* u8 (u8) */ {};

    inline impl::ToUpper
    to_upper /* u8 (u8) */ {};
    
  } // ascii

  namespace string {


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
    contains /* u8 (string<StringSize>, string<NeedleSize>) */ {};

    template <size_t DestSize, size_t SrcSize> impl::AppendStringCopy<DestSize, SrcSize>
    append_to_copy /* string<DestSize> (string<DestSize>, string<SrcSize>) */ {};

    template <typename Int, size_t Base = 10> impl::IntToString<Int, Base>
    int_to_string /* string<#bits{Int}> (Int) */ {};
    
    template <typename Int, size_t Base = 10, size_t MaxSize = 16> impl::StringToInt<Int, Base, MaxSize>
    string_to_int /* Int (string<MaxSize>) */ {};

    template <size_t MaxSize> impl::ClearString<MaxSize>
    clear /* inline void (string<MaxSize>) */ {};

    template <size_t DestSize, size_t SrcSize> impl::AppendString<DestSize, SrcSize>
    append /* inline void (string<DestSize>, string<SrcSize>) */ {};

  } // string

  namespace array {

    template <typename T, size_t N, bool Unroll = config::array::UnrollFill<T, N>> impl::FillArray<T, N, Unroll>
    fill /* inline void (Array<T, N>, T) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollClear<T, N>> impl::ClearArray<T, N, Unroll>
    clear /* inline void (Array<T, N>) */ {};
    
    template <typename T, size_t N, bool Unroll = config::array::UnrollFind<T, N>> impl::FindArray<T, N, Unroll>
    find /* u8/u16 (Array<T, N>, T) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollContains<T, N>> impl::ContainsArray<T, N, Unroll>
    contains /* u8 (Array<T, N>, T) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollEqual<T, N>> impl::EqualArray<T, N, Unroll>
    equal /* u8 (Array<T, N>, Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollSum<T, N>> impl::SumArray<T, N, Unroll>
    sum /* T (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollMinMax<T, N>> impl::MinArray<T, N, Unroll>
    min /* T (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollMinMax<T, N>> impl::MaxArray<T, N, Unroll>
    max /* T (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollMinMaxIndex<T, N>> impl::MinIndexArray<T, N, Unroll>
    min_index /* u8/u16 (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollMinMaxIndex<T, N>> impl::MaxIndexArray<T, N, Unroll>
    max_index /* u8/u16 (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollCount<T, N>> impl::CountArray<T, N, Unroll>
    count /* u8/u16 (Array<T, N>, T) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollSort<T, N>> impl::SortArray<T, N, Unroll>
    sort /* inline void (Array<T, N>) */ {};

    template <typename T, size_t N, bool Unroll = config::array::UnrollIsSorted<T, N>> impl::IsSortedArray<T, N, Unroll>
    is_sorted /* u8 (Array<T, N>) */ {};

  } // array
  
  namespace math {

    template <typename Int> impl::Min<Int>
    min /* Int (Int, Int) */ {};

    template <typename Int> impl::Max<Int>
    max /* Int (Int, Int) */ {};

    template <typename Int> impl::Clamp<Int>
    clamp /* Int (Int x, Int min, Int max) */ {};
    
    template <typename Int> impl::Abs<Int>
    abs /* Int (Int) */ {};
    
    template <typename Int> impl::Pow<Int>
    pow /* Int (Int, Int) */ {};
    
    template <typename Int> impl::Sqrt<Int>
    sqrt /* Int (Int) */ {};

    template <size_t Base, typename Int> impl::Log<Base, Int>
    log /* u8 (Int) */ {};

    template <typename Int> impl::Log<2, Int>
    log2 /* u8 (Int) */ {};

    template <typename Int> impl::Log<10, Int>
    log10 /* u8 (Int) */ {};

    template <typename Int> impl::Gcd<Int>
    gcd /* Int (Int, Int) */ {};
    
  } // math


  namespace ansi {
    template <size_t Width, size_t Height, size_t Left = 1, size_t Top = 1>
    using Screen = impl::Screen<Width, Height, Left, Top>;

    /*
     * ANSI terminal screen
     * --------------------
     *
     * Screen defines a rectangular drawing region inside the terminal.
     * Width and Height specify the dimensions of the region. Left and Top
     * specify its position in the terminal using ANSI's one-based column and
     * row coordinates.
     *
     * To start, Define a new screen region:
     *   using Screen = ansi::Screen<40, 40, 10, 10>;
     *
     * Now draw to the screen using its interface:
     *
     *   Screen::begin()
     *     Signature:   void ()
     *     Description: Prepare the terminal for drawing and hide the cursor.
     *
     *   Screen::move_to(x, y)
     *     Signature:   void (u8, u8)
     *     Description: Move the terminal cursor to coordinate (x, y).
     *
     *   Screen::put(x, y, character)
     *     Signature:   void (u8, u8, u8)
     *     Description: Move to coordinate (x, y) and write one character.
     *
     *   Screen::write<N>(x, y, text)
     *     Signature:   void (u8, u8, string<N>)
     *     Description: Move to coordinate (x, y) and write a string horizontally.
     *
     *   Screen::clear()
     *     Signature:   void ()
     *     Description: Clear the screen region and return the cursor to coordinate (0, 0).
     *
     *   Screen::end()
     *     Signature:   void ()
     *     Description: Restore the cursor and move it below the screen region.
     *
     * Example:
     *
     *   using Screen = ansi::Screen<40, 20>;
     *
     *   Screen::begin();
     *   Screen::clear();
     *   Screen::put(5, 2, '@');
     *   Screen::write<13>(3, 5, "Hello, world!");
     *   Screen::end();
     *
     * Notes:
     *
     * Screen writes directly to the terminal using ANSI escape sequences; it
     * does not maintain a separate framebuffer.
     *
     * Dynamic Acus expressions may be used for coordinates and characters.
     * The caller is responsible for keeping coordinates within the configured
     * screen dimensions.
     *
     * Functions may be outlined as usual:
     *
     *     auto put_screen = Screen::put.outline();
     */    
  }
  
} // acus::sugar


// Println implementation
void acus::sugar::println(auto&& arg, SUGAR_LOC) {
  print(std::forward<decltype(arg)>(arg), LOC_FWD);
  print('\n', LOC_FWD);
}

#include "acus/sugar/sugar_ascii_impl.h"
#include "acus/sugar/sugar_io_impl.h"
#include "acus/sugar/sugar_math_impl.h"
#include "acus/sugar/sugar_ansi_impl.h"
#include "acus/sugar/sugar_string_impl.h"
#include "acus/sugar/sugar_array_impl.h"
