// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Copy from one dynamic array element to another using different index variables.
// Expected: "CBCD"

TEST_BEGIN

auto u8 = ts::u8();
auto arrT = ts::array(u8, 4);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);
  c.declareLocal("j", u8);

  c.assign(c.arrayElement("arr", 0), literal::u8('A'));
  c.assign(c.arrayElement("arr", 1), literal::u8('B'));
  c.assign(c.arrayElement("arr", 2), literal::u8('C'));
  c.assign(c.arrayElement("arr", 3), literal::u8('D'));

  c.assign("i", literal::u8(0));
  c.assign("j", literal::u8(2));

  c.assign(c.arrayElement("arr", "i"), c.arrayElement("arr", "j"));

  c.write(c.arrayElement("arr", 0));
  c.write(c.arrayElement("arr", 1));
  c.write(c.arrayElement("arr", 2));
  c.write(c.arrayElement("arr", 3));

  c.returnFromFunction();
} c.endFunction();

TEST_END
