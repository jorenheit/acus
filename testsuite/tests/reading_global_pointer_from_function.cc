// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Pass a pointer to a global variable to a function that outputs its pointee. Then change its value and pass it again.
// Expected: "GHX"

TEST_BEGIN

TypeHandle u8 = ts::u8();
TypeHandle u8p = ts::pointer(ts::u8());


c.declareGlobal("g", u8);

c.function("main").begin(); {
  c.declareLocal("pg", u8p);
  c.declareLocal("x", u8);

  c.assign("pg", c.addressOf("g"));
  c.assign("g", literal::u8('G'));
  c.assign("x", literal::u8('X'));
  c.callFunction("foo").arg("pg").done();
  c.assign("g", literal::u8('H'));

  c.callFunction("foo").arg("pg").done();
  c.write("x");
  c.returnFromFunction();
} c.endFunction();

c.function("foo").param("p", u8p).ret(ts::void_t()).begin(); {
  auto pDeref = c.dereferencePointer("p");
  c.write(pDeref);
  c.returnFromFunction();
} c.endFunction();


TEST_END
