// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests returning a global value into a caller local while preserving the global for later reads.
// GG
TEST_BEGIN

c.declareGlobal("g", ts::u8());

c.function("main").begin(); {
  c.declareLocal("x", ts::u8());

  c.callFunction("foo").into("x").done();
  c.write("x");
  c.callFunction("bar").done();
  c.returnFromFunction();
} c.endFunction();

c.function("foo").ret(ts::u8()).begin(); {
  c.assign("g", literal::u8('G'));
  c.returnFromFunction("g");
} c.endFunction();

c.function("bar").begin(); {
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

TEST_END
