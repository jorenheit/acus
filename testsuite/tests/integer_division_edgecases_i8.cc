// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Filename: integer_division_edgecases_u8.cc
// Division edge cases for u8, including division by zero and 0/0
// Expected: ABCD

TEST_BEGIN

auto u8 = ts::u8();

c.function("main").begin(); {
  c.declareLocal("x", u8);
  c.declareLocal("y", u8);
  
  // x / 0 -> 0xff, mapped to 'A'
  c.assign("x", literal::u8(100));
  c.assign("y", literal::u8(0));
  c.write(c.add(c.div("x", "y"), literal::u8('B')));

  // 0 / 0 -> 0, mapped to 'B'
  c.assign("x", literal::u8(0));
  c.assign("y", literal::u8(0));
  c.write(c.add(c.div("x", "y"), literal::u8('C')));

  // x /= 0 -> 0xff, mapped to 'C'
  c.assign("x", literal::u8(100));
  c.assign("y", literal::u8(0));
  c.divAssign("x", "y");
  c.write(c.add("x", literal::u8('D')));

  // 0 /= 0 -> 0, mapped to 'D'
  c.assign("x", literal::u8(0));
  c.assign("y", literal::u8(0));
  c.divAssign("x", "y");
  c.write(c.add("x", literal::u8('E')));

  c.returnFromFunction();
} c.endFunction();

TEST_END
