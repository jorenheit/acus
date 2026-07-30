// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later


/*
 * Acus Sugar standard library
 * ---------------------------
 *
 * This header intentionally collects the complete user-facing standard-library
 * interface in one place. Library functions are stateless callable objects.
 * Operations that naturally belong to a String or Array type are exposed
 * through that type, while free-standing operations are grouped into namespaces
 * such as io, algorithm, ascii, string, math and ansi.
 *
 * For example:
 *
 *   using Line = String<40>;
 *   using Lines = Array<Line, 5>;
 *
 *   auto line = io::read_line<Line>();
 *   auto length = Line::length(line);
 *
 *   if_(Line::contains<String<5>>(line, "hello")) {
 *     println(length);
 *   };
 *
 *   Lines::sort(lines);
 *
 * The String and Array entries are member-like library objects, not ordinary
 * C++ member functions of an Expr. The value being operated on is therefore
 * still passed explicitly:
 *
 *   Line::find_char(line, 'x');
 *   Lines::contains(lines, line);
 *
 * Template arguments select the Sugar types, capacities or other compile-time
 * settings required by a function. The expected Acus signature is documented
 * beside every public object:
 *
 *   template <typename Int>
 *   MATH_FUNCTION(sqrt, "(Int) -> Int", Int) {};
 *
 * This means that `math::sqrt<u16>` behaves as an Acus function with signature
 *
 *   (u16) -> u16
 *
 * Calling a library object returns an Expr when the function has a result. A
 * C++ variable storing that result is only a handle to the generated Acus
 * expression or storage:
 *
 *   auto length = Line::length(line);
 *   println(length);
 *
 *
 * Inline and outlined use
 * -----------------------
 *
 * By default, a library function is emitted inline at its call site. Inline
 * calls avoid Acus function-call overhead, but repeated calls duplicate the
 * generated implementation.
 *
 * Outlinable functions can instead be emitted once as ordinary Acus functions:
 *
 *   using Line = String<40>;
 *   using Lines = Array<Line, 5>;
 *
 *   auto read_line = io::read_line<Line>.outline();
 *   auto contains_line = Lines::contains.outline();
 *   auto square_root = math::sqrt<u16>.outline();
 *
 * The returned handles can then be called like regular function handles:
 *
 *   let_<Line>("line") = read_line();
 *   println(contains_line(lines, var_("line")));
 *
 * Outlining can substantially reduce generated program size when a function is
 * used repeatedly, but introduces function-call and argument-copying overhead.
 * Whether inline or outlined code is preferable therefore depends on the
 * function and its usage.
 *
 * Functions marked "inline only" modify storage supplied by the caller and do
 * not provide outline(). Examples include:
 *
 *   algorithm::clear<T>
 *   algorithm::swap<T>
 *   String<N>::append<Other>
 *   Array<T, N>::fill
 *   Array<T, N>::sort
 *
 */

#pragma once
#include <tuple>

#include "acus/sugar/sugar_config.h"
#include "acus/sugar/sugar_libraryfunction.h"
#include "acus/sugar/sugar_std_fwd.h"
#include "acus/sugar/sugar_ascii.h"
#include "acus/sugar/sugar_io.h"
#include "acus/sugar/sugar_math.h"
#include "acus/sugar/sugar_ansi.h"
#include "acus/sugar/sugar_string.h"
#include "acus/sugar/sugar_array.h"
#include "acus/sugar/sugar_algorithm.h"

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
    /*
     * Examples:
     *
     *   using Line = String<40>;
     *
     *   auto line = io::read_line<Line>();
     *   auto read_line = io::read_line<Line>.outline();
     *   let_<Line>("other") = read_line();
     */

    template <typename String>
    IO_FUNCTION(read_line, "() -> String", String);
  } // namespace io


  namespace algorithm {
    /*
     * Examples:
     *
     *   using Values = Array<u8, 5>;
     *
     *   algorithm::clear<Values>(values);
     *   algorithm::swap<u8>(lhs, rhs);
     */

    template <typename T> ALGORITHM_FUNCTION(clear, "(T) -> void", T);    // inline only
    template <typename T> ALGORITHM_FUNCTION(swap,  "(T, T) -> void", T); // inline only 
  } // namespace algorithm


  namespace ascii {
    /*
     * Examples:
     *
     *   if_(ascii::is_digit(character)) {
     *     println("digit");
     *   };
     *
     *   auto lower = ascii::to_lower(character);
     */

    ASCII_FUNCTION(is_digit,        "(u8) -> u8") {};
    ASCII_FUNCTION(is_alpha,        "(u8) -> u8") {};
    ASCII_FUNCTION(is_alphanumeric, "(u8) -> u8") {};
    ASCII_FUNCTION(is_lower,        "(u8) -> u8") {};
    ASCII_FUNCTION(is_upper,        "(u8) -> u8") {};
    ASCII_FUNCTION(is_whitespace,   "(u8) -> u8") {};
    ASCII_FUNCTION(to_lower,        "(u8) -> u8") {};
    ASCII_FUNCTION(to_upper,        "(u8) -> u8") {};
  } // namespace ascii


  template <size_t N>
  struct String: impl::StringBase<N> {
    STRING_IMPLEMENTATION;

    /*
     * Examples:
     *
     *   using Text = String<40>;
     *
     *   auto length = Text::length(text);
     *   auto position = Text::find_char(text, 'x');
     *
     *   auto find_char = Text::find_char.outline();
     *   println(find_char(text, 'x'));
     */
    STRING_MEMBER(length,    "(String) -> u8/u16");     // depending on string-size
    STRING_MEMBER(find_char, "(String, u8) -> u8/u16"); // N means not found

    /*
     * Examples:
     *
     *   using Text = String<40>;
     *   using Word = String<10>;
     *
     *   if_(Text::starts_with<Word>(text, word)) {
     *     println("prefix");
     *   };
     *
     *   auto contains_word = Text::contains<Word>.outline();
     *   println(contains_word(text, word));
     */
    template <typename Other> STRING_MEMBER(starts_with,    "(String, Other) -> u8", Other); 
    template <typename Other> STRING_MEMBER(ends_with,      "(String, Other) -> u8", Other);
    template <typename Other> STRING_MEMBER(find_str,       "(String, Other) -> u8/u16", Other); // N means not found
    template <typename Other> STRING_MEMBER(contains,       "(String, Other) -> u8", Other);
    template <typename Other> STRING_MEMBER(append_to_copy, "(String, Other) -> String", Other);
    template <typename Other> STRING_MEMBER(append,         "(String, Other) -> void", Other); // inline only

    /*
     * Examples:
     *
     *   auto value = String<3>::to_int<u8>("123");
     *   auto parse_hex = String<4>::to_int<u16, 16>.outline();
     */
    template <typename Int, size_t Base = 10>
    STRING_MEMBER(to_int, "(String) -> Int", Int, Base);
  }; // String


  namespace string {
    /*
     * Free-standing string functions treat neither operand as a natural
     * receiver, or construct a new string value.
     *
     * Examples:
     *
     *   auto order = string::compare<String<10>, String<20>>(lhs, rhs);
     *   auto text = string::from_int<u16>(value);
     *   auto format_hex = string::from_int<u16, 16>.outline();
     */

    template <typename Lhs, typename Rhs>
    STRING_FUNCTION(compare, "(Lhs, Rhs) -> s8", Lhs, Rhs);

    template <typename Int, size_t Base = 10>
    STRING_FUNCTION(from_int, "(Int) -> String<#bits{Int}>", String<Int::Bits>, Int, Base);
  } // namespace string


  template <typename T, size_t N>
  struct Array: impl::ArrayBase<T, N> {
    ARRAY_IMPLEMENTATION;

    /*
     * Examples:
     *
     *   using Values = Array<u8, 5>;
     *
     *   Values::fill(values, 0);
     *   auto position = Values::find(values, 42);
     *   println(Values::contains(values, 42));
     *
     *   auto find = Values::find.outline();
     *   println(find(values, 42));
     *
     * `find` returns N when no matching element exists. The index-returning
     * functions use u8 for small arrays and u16 for larger arrays.
     */

    ARRAY_MEMBER(fill,      "(Array, T) -> void");   // inline only
    ARRAY_MEMBER(find,      "(Array, T) -> u8/u16");
    ARRAY_MEMBER(contains,  "(Array, T) -> u8");
    ARRAY_MEMBER(equal,     "(Array, Array) -> u8");
    ARRAY_MEMBER(sum,       "(Array) -> T");
    ARRAY_MEMBER(min,       "(Array) -> T");
    ARRAY_MEMBER(max,       "(Array) -> T");
    ARRAY_MEMBER(min_index, "(Array) -> u8/u16");
    ARRAY_MEMBER(max_index, "(Array) -> u8/u16");
    ARRAY_MEMBER(count,     "(Array, T) -> u8/u16");
    ARRAY_MEMBER(sort,      "(Array) -> void");       // inline only
    ARRAY_MEMBER(is_sorted, "(Array) -> u8");
  }; // Array


  namespace math {
    /*
     * Examples:
     *
     *   auto root = math::sqrt<u16>(value);
     *   auto bounded = math::clamp<s16>(value, -10, 10);
     *   auto binary_digits = math::log2<u16>(value);
     *
     *   auto gcd = math::gcd<u16>.outline();
     *   println(gcd(lhs, rhs));
     */

    template <typename Int> MATH_FUNCTION(min,   "(Int, Int) -> Int",      Int) {};
    template <typename Int> MATH_FUNCTION(max,   "(Int, Int) -> Int",      Int) {};
    template <typename Int> MATH_FUNCTION(clamp, "(Int, Int, Int) -> Int", Int) {};
    template <typename Int> MATH_FUNCTION(abs,   "(Int) -> Int",           Int) {};
    template <typename Int> MATH_FUNCTION(pow,   "(Int, Int) -> Int",      Int) {};
    template <typename Int> MATH_FUNCTION(sqrt,  "(Int)	-> Int",           Int) {};
    template <typename Int> MATH_FUNCTION(log2,  "(Int)	-> u8",            2, Int) {};
    template <typename Int> MATH_FUNCTION(log10, "(Int)	-> u8",            10, Int) {};
    template <typename Int> MATH_FUNCTION(gcd,   "(Int, Int)-> Int",       Int) {};

    template <size_t Base, typename Int>
    MATH_FUNCTION(log,   "(Int) -> u8", Base, Int) {};

  } // namespace math


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
     * To start, define a new screen region:
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
     *     Signature:   void (u8, u8, String<N>)
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
  } // namespace ansi
  
} // namespace acus::sugar

#include "acus/sugar/sugar_operations.h"
#include "acus/sugar/sugar_println_impl.h"
#include "acus/sugar/sugar_ascii_impl.h"
#include "acus/sugar/sugar_io_impl.h"
#include "acus/sugar/sugar_math_impl.h"
#include "acus/sugar/sugar_ansi_impl.h"
#include "acus/sugar/sugar_string_impl.h"
#include "acus/sugar/sugar_array_impl.h"
#include "acus/sugar/sugar_algorithm_impl.h"

#include "acus/sugar/sugar_string_members.h"  
#include "acus/sugar/sugar_array_members.h"

#include "acus/sugar/sugar_undef_macros.h"
