// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Exercise every binary operator for which a literal lhs may be moved to the
// rhs. Each check prints 'B' on success and 'A' on failure.
//
// The comparison equality cases are intentional: they distinguish operand
// swapping (< -> >) from result negation (< -> >=).
// Expected: 38 times 'B'

TEST_BEGIN

c.function("main").begin(); {
  c.declareLocal("x", ts::u8());

  auto check = [&](Expression actual, literal::Literal expected) {
    c.write(c.add(c.eq(actual, expected), literal::u8('A')));
  };

  // Arithmetic -------------------------------------------------------------
  c.assign("x", literal::u8(7));

  check(c.add("x", literal::u8(5)), literal::u8(12));
  check(c.add(literal::u8(5), "x"), literal::u8(12));

  check(c.sub("x", literal::u8(5)), literal::u8(2));
  check(c.sub(literal::u8(5), "x"), literal::u8(254));

  check(c.mul("x", literal::u8(5)), literal::u8(35));
  check(c.mul(literal::u8(5), "x"), literal::u8(35));

  // Logical ---------------------------------------------------------------
  // x is true and the literal is false.
  check(c.land("x", literal::u8(0)), literal::u8(0));
  check(c.land(literal::u8(0), "x"), literal::u8(0));

  check(c.lnand("x", literal::u8(0)), literal::u8(1));
  check(c.lnand(literal::u8(0), "x"), literal::u8(1));

  check(c.lor("x", literal::u8(0)), literal::u8(1));
  check(c.lor(literal::u8(0), "x"), literal::u8(1));

  check(c.lnor("x", literal::u8(0)), literal::u8(0));
  check(c.lnor(literal::u8(0), "x"), literal::u8(0));

  check(c.lxor("x", literal::u8(0)), literal::u8(1));
  check(c.lxor(literal::u8(0), "x"), literal::u8(1));

  check(c.lxnor("x", literal::u8(0)), literal::u8(0));
  check(c.lxnor(literal::u8(0), "x"), literal::u8(0));

  // Equality --------------------------------------------------------------
  check(c.eq("x", literal::u8(5)), literal::u8(0));
  check(c.eq(literal::u8(5), "x"), literal::u8(0));

  check(c.neq("x", literal::u8(5)), literal::u8(1));
  check(c.neq(literal::u8(5), "x"), literal::u8(1));

  // Ordered comparisons: unequal operands --------------------------------
  check(c.lt("x", literal::u8(5)), literal::u8(0));
  check(c.lt(literal::u8(5), "x"), literal::u8(1));

  check(c.le("x", literal::u8(5)), literal::u8(0));
  check(c.le(literal::u8(5), "x"), literal::u8(1));

  check(c.gt("x", literal::u8(5)), literal::u8(1));
  check(c.gt(literal::u8(5), "x"), literal::u8(0));

  check(c.ge("x", literal::u8(5)), literal::u8(1));
  check(c.ge(literal::u8(5), "x"), literal::u8(0));

  // Ordered comparisons: equality boundary -------------------------------
  c.assign("x", literal::u8(5));

  check(c.lt("x", literal::u8(5)), literal::u8(0));
  check(c.lt(literal::u8(5), "x"), literal::u8(0));

  check(c.le("x", literal::u8(5)), literal::u8(1));
  check(c.le(literal::u8(5), "x"), literal::u8(1));

  check(c.gt("x", literal::u8(5)), literal::u8(0));
  check(c.gt(literal::u8(5), "x"), literal::u8(0));

  check(c.ge("x", literal::u8(5)), literal::u8(1));
  check(c.ge(literal::u8(5), "x"), literal::u8(1));

  c.returnFromFunction();
} c.endFunction();

TEST_END
