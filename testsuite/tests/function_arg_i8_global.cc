// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests passing a global u8 by value as a function argument, verifying that modifying the parameter does not mutate the global.
// Expected: GHG

TEST_BEGIN

c.declareGlobal("g", ts::u8());

c.function("main").begin(); {

  c.assign("g", literal::u8('G'));
  c.callFunction("foo").arg("g").done();
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

c.function("foo").param("arg1", ts::u8()).ret(ts::void_t()).begin(); {
  c.write("arg1");
  c.assign("arg1", literal::u8('H'));
  c.write("arg1");
  c.returnFromFunction();
} c.endFunction();

TEST_END
