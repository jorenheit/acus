// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Do pointer arithmetic on an array of structs and verify scaling by struct size
// Expected: ACBCBA2

TEST_BEGIN

auto u8    = ts::u8();
auto pairT = ts::defineStruct("Pair").field("x", u8).field("y", u8).done();
auto pairP = ts::pointer(pairT);
auto arrT  = ts::array(pairT, 3);

c.function("main").begin(); {
  c.declareLocal("p", pairP);
  c.declareLocal("q", pairP);
  c.declareLocal("arr", arrT);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 0)));

  auto pObj = c.dereferencePointer("p");
  c.write(c.structField(pObj, "x"));        // A

  c.assign("q", c.add("p", literal::u16(2)));
  auto qObj = c.dereferencePointer("q");
  c.write(c.structField(qObj, "x"));        // C

  c.subAssign("q", literal::u16(1));
  auto qPrev = c.dereferencePointer("q");
  c.write(c.structField(qPrev, "x"));       // B

  c.declareLocal("i", ts::u16());
  c.assign("i", literal::u16(2));
  c.addAssign("p", c.add("i", literal::u16(0)));
  c.write(c.structField(c.dereferencePointer("p"), "x")); // C
  c.subAssign("p", c.sub("i", literal::u16(1)));
  c.write(c.structField(c.dereferencePointer("p"), "x")); // B
  c.addAssign("p", literal::u16(1));
  c.subAssign("p", "i");
  c.write(c.structField(c.dereferencePointer("p"), "x")); // A
  c.write(c.add(c.cast("i", ts::u8()), literal::u8('0'))); // 2
  c.returnFromFunction();
} c.endFunction();

TEST_END
