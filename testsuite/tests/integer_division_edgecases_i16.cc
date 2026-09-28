// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Filename: integer_division_edgecases_u16.cc
// Division edge cases for u16, including division by zero and 0/0
// Expected: AABBCCDD

TEST_BEGIN

auto u16 = ts::u16();

c.function("main").begin(); {
  c.declareLocal("x", u16);
  c.declareLocal("y", u16);
  
  // x / 0 -> 0xffff, mapped to "AA"
  c.assign("x", literal::u16(0x1234));
  c.assign("y", literal::u16(0));
  c.write(c.add(c.div("x", "y"), literal::u16(0x4142)));
  
  // 0 / 0 -> 0xffff, mapped to "BB"
  c.assign("x", literal::u16(0));
  c.assign("y", literal::u16(0));
  c.write(c.add(c.div("x", "y"), literal::u16(0x4243)));

  // x /= 0 -> 0xffff, mapped to "CC"
  c.assign("x", literal::u16(0x1234));
  c.assign("y", literal::u16(0));
  c.divAssign("x", "y");
  c.write(c.add("x", literal::u16(0x4344)));
  
  // 0 /= 0 -> 0xffff, mapped to "DD"
  c.assign("x", literal::u16(0));
  c.assign("y", literal::u16(0));
  c.divAssign("x", "y");
  c.write(c.add("x", literal::u16(0x4445)));

  c.returnFromFunction();
} c.endFunction();

TEST_END
