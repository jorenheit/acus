// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests reading through a pointer to a global variable in the same function.
// Expected: GG

TEST_BEGIN

auto u8  = ts::u8();
auto u8p = ts::pointer(u8);

c.declareGlobal("g", u8);

c.function("main").begin(); {
  c.declareLocal("p", u8p);

  c.assign("g", literal::u8('G'));
  c.assign("p", c.addressOf("g"));

  auto pDeref = c.dereferencePointer("p");
  c.write("g");
  c.write(pDeref);

  c.returnFromFunction();
} c.endFunction();

TEST_END
