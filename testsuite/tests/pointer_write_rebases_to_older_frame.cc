// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Writing a runtime pointer through a pointer into an older frame must rebase
// the stored pointer to that destination frame.
// Expected: A

TEST_BEGIN

auto u8   = ts::u8();
auto u8p  = ts::pointer(u8);
auto u8pp = ts::pointer(u8p);

c.function("main").begin(); {
  c.declareLocal("x", u8);
  c.declareLocal("p", u8p);

  c.assign("x", literal::u8('A'));
  c.callFunction("store")
    .arg(c.addressOf("p"))
    .arg(c.addressOf("x"))
    .done();

  c.write(c.dereferencePointer("p"));
  c.returnFromFunction();
} c.endFunction();

c.function("store").param("pp", u8pp).param("q", u8p).ret(ts::void_t()).begin(); {
  c.assign(c.dereferencePointer("pp"), "q");
  c.returnFromFunction();
} c.endFunction();

TEST_END
