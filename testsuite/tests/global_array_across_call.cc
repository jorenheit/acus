// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Tests referenced global array access across a call, including element-wise writes in caller and reads in callee.
// Expected output: ABCD

TEST_BEGIN

c.declareGlobal("x", ts::array(ts::u8(), 10));

c.function("main").begin(); {

  auto x0 = c.arrayElement("x", 0);
  auto x1 = c.arrayElement("x", 1);
  auto x2 = c.arrayElement("x", 2);
  auto x3 = c.arrayElement("x", 3);

  c.assign(x0, literal::u8('A'));
  c.assign(x1, literal::u8('B'));
  c.assign(x2, literal::u8('C'));
  c.assign(x3, literal::u8('D'));

  c.callFunction("foo").done();
  c.returnFromFunction();
} c.endFunction();

c.function("foo").begin(); {

  auto x0 = c.arrayElement("x", literal::u8(0));
  auto x1 = c.arrayElement("x", literal::u8(1));
  auto x2 = c.arrayElement("x", literal::u8(2));
  auto x3 = c.arrayElement("x", literal::u8(3));

  c.write(x0);
  c.write(x1);
  c.write(x2);
  c.write(x3);

  c.returnFromFunction();
} c.endFunction();

TEST_END
