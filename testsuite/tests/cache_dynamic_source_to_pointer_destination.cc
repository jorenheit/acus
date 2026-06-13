// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Copy from a dynamic source field into a pointer-derived destination field.
// Expected: "ABBC"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);
auto arrT = ts::array(pairT, 3);

c.function("main").begin(); {
  c.declareLocal("p", pairP);
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 0)));
  c.assign("i", literal::u8(1));

  auto src = c.structField(c.arrayElement("arr", "i"), "x");
  auto dst = c.structField(c.dereferencePointer("p"), "x");

  c.write(src);
  c.assign(dst, src);

  c.write(c.structField(c.arrayElement("arr", 0), "x"));
  c.write(c.structField(c.arrayElement("arr", 1), "x"));
  c.write(c.structField(c.arrayElement("arr", 2), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
