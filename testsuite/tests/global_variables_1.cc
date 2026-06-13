// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests global variable access across a call: callee reads and mutates a referenced global, caller observes the updated value.
// Expected: AF

TEST_BEGIN

c.declareGlobal("g", ts::u8());

c.function("main").begin(); {
  c.declareLocal("x", ts::u8());

  c.assign("g", literal::u8('A'));
  c.callFunction("foo").done();
  c.write("g");          // should print 'F'
  c.returnFromFunction();
} c.endFunction();

c.function("foo").begin(); {

  c.write("g");          // should print 'A'
  c.assign("g", literal::u8('F'));  // modify global shadow
  c.returnFromFunction();
} c.endFunction();


TEST_END

