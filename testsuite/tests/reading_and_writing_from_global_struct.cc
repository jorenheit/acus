// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Reading from, writing to (mutating) fields of a global struct variable
// Expected: ABAC


TEST_BEGIN

auto point = ts::defineStruct("Point").field("x", ts::u8()).field("y", ts::u8()).done();

c.declareGlobal("g", point);

c.function("main").begin(); {

  auto gx = c.structField("g", "x");
  auto gy = c.structField("g", "y");

  c.assign(gx, literal::u8('A'));
  c.assign(gy, literal::u8('B'));
  c.write("g");

  c.callFunction("foo").done();
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

c.function("foo").begin(); {

  auto gy = c.structField("g", "y");
  c.assign(gy, literal::u8('C'));
  c.returnFromFunction();
} c.endFunction();

TEST_END
