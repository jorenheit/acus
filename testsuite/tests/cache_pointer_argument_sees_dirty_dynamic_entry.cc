// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Dirty a dynamic array field, pass a pointer to it, and read it in the callee.
// Expected: "X"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);
auto arrT = ts::array(pairT, 3);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);
  c.declareLocal("p", pairP);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("i", literal::u8(1));
  c.assign(c.structField(c.arrayElement("arr", "i"), "x"), literal::u8('X'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 1)));
  c.callFunction("foo").arg("p").done();

  c.returnFromFunction();
} c.endFunction();

c.function("foo").param("p", pairP).ret(ts::void_t()).begin(); {
  c.write(c.structField(c.dereferencePointer("p"), "x"));
  c.returnFromFunction();
} c.endFunction();

TEST_END
