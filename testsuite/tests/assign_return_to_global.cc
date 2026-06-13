// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests assigning a function return value directly into a global and reading it back in a later call.
// Expected result: YY

TEST_BEGIN
c.declareGlobal("g", ts::u8());

c.function("main").begin(); {

  c.callFunction("foo").into("g").done();
  c.write("g");
  c.callFunction("bar").done();
  c.returnFromFunction();
} c.endFunction();

c.function("foo").ret(ts::u8()).begin(); {
  c.declareLocal("y", ts::u8());
  c.assign("y", literal::u8('Y'));
  c.returnFromFunction("y");
} c.endFunction();

c.function("bar").begin(); {
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

TEST_END
