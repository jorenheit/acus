// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Write through a direct-relative field containing a dynamic array element.
// Expected: "AQCD"

TEST_BEGIN

auto u8 = ts::u8();
auto dataT = ts::array(u8, 3);

auto holderT = ts::defineStruct("Holder")
  .field("tag", u8)
  .field("data", dataT)
  .done();

c.function("main").begin(); {
  c.declareLocal("h", holderT);
  c.declareLocal("idx", u8);

  c.assign(c.structField("h", "tag"), literal::u8('A'));
  c.assign(c.arrayElement(c.structField("h", "data"), 0), literal::u8('B'));
  c.assign(c.arrayElement(c.structField("h", "data"), 1), literal::u8('C'));
  c.assign(c.arrayElement(c.structField("h", "data"), 2), literal::u8('D'));

  c.assign("idx", literal::u8(0));
  c.assign(c.arrayElement(c.structField("h", "data"), "idx"), literal::u8('Q'));

  c.write(c.structField("h", "tag"));
  c.write(c.arrayElement(c.structField("h", "data"), 0));
  c.write(c.arrayElement(c.structField("h", "data"), 1));
  c.write(c.arrayElement(c.structField("h", "data"), 2));

  c.returnFromFunction();
} c.endFunction();

TEST_END
