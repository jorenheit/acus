// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Dereferencing a pointer-to-pointer across a frame boundary must rebase
// the materialized inner pointer to the current frame.
// Expected: A

TEST_BEGIN

auto u8   = ts::u8();
auto u8p  = ts::pointer(u8);
auto u8pp = ts::pointer(u8p);

c.function("main").begin(); {
  c.declareLocal("x", u8);
  c.assign("x", literal::u8('A'));
  c.callFunction("outer").arg(c.addressOf("x")).done();
  c.returnFromFunction();
} c.endFunction();

c.function("outer").param("p", u8p).ret(ts::void_t()).begin(); {
  c.callFunction("inner").arg(c.addressOf("p")).done();
  c.returnFromFunction();
} c.endFunction();

c.function("inner").param("pp", u8pp).ret(ts::void_t()).begin(); {
  auto p = c.dereferencePointer("pp");
  c.write(c.dereferencePointer(p));
  c.returnFromFunction();
} c.endFunction();

TEST_END
