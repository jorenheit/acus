// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Self-multiplication through compound assignment (lhs == rhs).
// Covers both 8-bit and 16-bit paths.
// Expected: Q!!

TEST_BEGIN

c.function("main").begin(); {
  c.declareLocal("x8", ts::u8());
  c.declareLocal("x16", ts::u16());

  c.assign("x8", literal::u8(9));
  c.mulAssign("x8", "x8");       // 9 * 9 = 81 -> Q
  c.write("x8");

  c.assign("x16", literal::u16(0x1011));
  c.mulAssign("x16", "x16");     // 0x1011^2 mod 2^16 = 0x2121 -> !!
  c.write("x16");

  c.returnFromFunction();
} c.endFunction();

TEST_END
