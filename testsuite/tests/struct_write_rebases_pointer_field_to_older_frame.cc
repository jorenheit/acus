// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Writing an aggregate through a pointer into an older frame must rebase
// runtime pointers stored inside that aggregate.
// Expected: A

TEST_BEGIN

auto u8 = ts::u8();
auto u8p = ts::pointer(u8);
auto holder = ts::defineStruct("WrittenHolder").field("p", u8p).done();
auto holderp = ts::pointer(holder);

c.function("main").begin(); {
  c.declareLocal("x", u8);
  c.declareLocal("dst", holder);

  c.assign("x", literal::u8('A'));
  c.callFunction("storeHolder")
    .arg(c.addressOf("dst"))
    .arg(c.addressOf("x"))
    .done();

  auto p = c.structField("dst", "p");
  c.write(c.dereferencePointer(p));
  c.returnFromFunction();
} c.endFunction();

c.function("storeHolder").param("dst", holderp).param("q", u8p).ret(ts::void_t()).begin(); {
  c.declareLocal("src", holder);
  c.assign(c.structField("src", "p"), "q");
  c.assign(c.dereferencePointer("dst"), "src");
  c.returnFromFunction();
} c.endFunction();

TEST_END
