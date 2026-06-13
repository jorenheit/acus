// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Copy from a dereferenced pointer into a dynamic destination field.
// Expected: "BBC"

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

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A')); // arr[0].x = 'A'
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B')); // arr[1].x = 'B'
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C')); // arr[2].x = 'C'

  c.assign("p", c.addressOf(c.arrayElement("arr", 1))); // p = &arr[1]
  c.assign("i", literal::u8(0)); // i = 0

  auto src = c.structField(c.dereferencePointer("p"), "x");   // src = (*p).x
  auto dst = c.structField(c.arrayElement("arr", "i"), "x");  // dst = arr[i].x -> arr[0].x

  c.assign(dst, src); // arr[0].x = (*p).x -> arr[1].x -> B

  c.write(c.structField(c.arrayElement("arr", 0), "x"));
  c.write(c.structField(c.arrayElement("arr", 1), "x"));
  c.write(c.structField(c.arrayElement("arr", 2), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
