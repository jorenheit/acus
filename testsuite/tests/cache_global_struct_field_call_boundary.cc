// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Write global struct fields, cross a function-call boundary, and read the global in the callee.
// Expected: "AB"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

c.declareGlobal("g", pairT);

c.function("main").begin(); {
  c.assign(c.structField("g", "x"), literal::u8('A'));
  c.assign(c.structField("g", "y"), literal::u8('B'));

  c.callFunction("foo").done();

  c.returnFromFunction();
} c.endFunction();

c.function("foo").ret(ts::void_t()).begin(); {
  c.write(c.structField("g", "x"));
  c.write(c.structField("g", "y"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
