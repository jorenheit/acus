// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Pointer addition is accepted in both orders. Verify that moving a literal
// lhs to the rhs preserves pointer arithmetic rather than trying to assign the
// integer literal to a pointer temporary.
// Expected: CC

TEST_BEGIN

auto u8 = ts::u8();
auto ptr = ts::pointer(u8);
auto arr = ts::array(u8, 4);

c.function("main").begin(); {
  c.declareLocal("values", arr);
  c.declareLocal("p", ptr);
  c.declareLocal("q", ptr);

  c.assign(c.arrayElement("values", 0), literal::u8('A'));
  c.assign(c.arrayElement("values", 1), literal::u8('B'));
  c.assign(c.arrayElement("values", 2), literal::u8('C'));
  c.assign(c.arrayElement("values", 3), literal::u8('D'));

  c.assign("p", c.addressOf(c.arrayElement("values", 0)));

  c.assign("q", c.add("p", literal::u16(2)));
  c.write(c.dereferencePointer("q"));

  c.assign("q", c.add(literal::u16(2), "p"));
  c.write(c.dereferencePointer("q"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
