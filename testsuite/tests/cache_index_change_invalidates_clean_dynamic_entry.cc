// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Read through a dynamic index, change the index, then read through it again.
// Expected: "AC"

TEST_BEGIN

auto u8 = ts::u8();
auto arrT = ts::array(u8, 3);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);

  c.assign(c.arrayElement("arr", 0), literal::u8('A'));
  c.assign(c.arrayElement("arr", 1), literal::u8('B'));
  c.assign(c.arrayElement("arr", 2), literal::u8('C'));

  c.assign("i", literal::u8(0));
  c.write(c.arrayElement("arr", "i"));

  c.assign("i", literal::u8(2));
  c.write(c.arrayElement("arr", "i"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
