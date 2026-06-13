// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Structurally write one field, then write a sibling field through a pointer and read both.
// Expected: "XY"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);
auto arrT = ts::array(pairT, 2);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("p", pairP);
  c.declareLocal("i", u8);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 0), "y"), literal::u8('B'));

  c.assign("i", literal::u8(0));
  c.assign(c.structField(c.arrayElement("arr", "i"), "x"), literal::u8('X'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 0)));
  c.assign(c.structField(c.dereferencePointer("p"), "y"), literal::u8('Y'));

  c.write(c.structField(c.arrayElement("arr", 0), "x"));
  c.write(c.structField(c.arrayElement("arr", 0), "y"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
