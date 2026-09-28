// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Filename: integer_modulo_edgecases_u16.cc
// Modulo edge cases for u16: x%0, 0%x, 0%0, and assign variants
// Expected: BBEE

TEST_BEGIN

auto u16 = ts::u16();

c.function("main").begin(); {
  c.declareLocal("x", u16);
  c.declareLocal("y", u16);


  // x % 0 -> 0, mapped to "AA"
  c.assign("x", literal::u16(0x1234));
  c.assign("y", literal::u16(0));
  c.write(c.add(c.mod("x", "y"), literal::u16(0x4141)));

  // 0 % x -> 0, mapped to "BB"
  c.assign("x", literal::u16(0));
  c.assign("y", literal::u16(0x1111));
  c.write(c.add(c.mod("x", "y"), literal::u16(0x4242)));

  // 0 % 0 -> 0, mapped to "CC"
  c.assign("x", literal::u16(0));
  c.assign("y", literal::u16(0));
  c.write(c.add(c.mod("x", "y"), literal::u16(0x4343)));

  // x %= 0 -> 0, mapped to "DD"
  c.assign("x", literal::u16(0x1234));
  c.assign("y", literal::u16(0));
  c.modAssign("x", "y");
  c.write(c.add("x", literal::u16(0x4444)));

  // 0 %= x -> 0, mapped to "EE"
  c.assign("x", literal::u16(0));
  c.assign("y", literal::u16(0x1111));
  c.modAssign("x", "y");
  c.write(c.add("x", literal::u16(0x4545)));

  c.returnFromFunction();
} c.endFunction();

TEST_END
