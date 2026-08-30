// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Materializing an aggregate through a pointer must rebase runtime pointers
// stored inside that aggregate.
// Expected: A

TEST_BEGIN

auto u8 = ts::u8();
auto u8p = ts::pointer(u8);
auto holder = ts::defineStruct("DereferencedHolder").field("p", u8p).done();
auto holderp = ts::pointer(holder);

c.function("main").begin(); {
  c.declareLocal("x", u8);
  c.declareLocal("h", holder);

  c.assign("x", literal::u8('A'));
  c.assign(c.structField("h", "p"), c.addressOf("x"));
  c.callFunction("readHolder").arg(c.addressOf("h")).done();
  c.returnFromFunction();
} c.endFunction();

c.function("readHolder").param("hp", holderp).ret(ts::void_t()).begin(); {
  auto h = c.dereferencePointer("hp");
  auto p = c.structField(h, "p");
  c.write(c.dereferencePointer(p));
  c.returnFromFunction();
} c.endFunction();

TEST_END
