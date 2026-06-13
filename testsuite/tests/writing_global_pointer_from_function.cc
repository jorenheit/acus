// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests writing through a pointer to a global variable from inside a function.
// main initializes g to 'G', foo writes 'H' through p, then main prints g.
// Expected: HH

TEST_BEGIN

auto u8  = ts::u8();
auto u8p = ts::pointer(u8);

c.declareGlobal("g", u8);

c.function("main").begin(); {
  c.declareLocal("p", u8p);

  c.assign("g", literal::u8('G'));
  c.assign("p", c.addressOf("g"));
  c.callFunction("foo").arg("p").done();
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

c.function("foo").param("p", u8p).ret(ts::void_t()).begin(); {
  auto pDeref = c.dereferencePointer("p");
  c.assign(pDeref, literal::u8('H'));
  c.write("g");
  c.returnFromFunction();
} c.endFunction();

TEST_END
