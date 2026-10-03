// Exercise consumable expression temporaries and preserved local operands.
TEST_BEGIN
c.function("main").begin(); {
  auto check = [&](auto const &value, auto const &expected) {
    c.write(c.add(c.eq(value, expected), literal::u8('S'))); // T on success
  };
  for (auto type : {ts::u8(), ts::u16(), ts::s8(), ts::s16()}) {
    c.scope().begin();
    c.declareLocal("lhs", type);
    c.declareLocal("rhs", type);
    auto lit = [&](int value) -> literal::Literal {
      if (type == ts::u8()) return literal::u8(value);
      if (type == ts::u16()) return literal::u16(value);
      if (type == ts::s8()) return literal::s8(value);
      return literal::s16(value);
    };
    for (auto [a, b] : {std::pair{0, 3}, {17, 3}, {17, -3}, {-17, 3}, {-17, -3}}) {
      if (acus::types::isUnsignedInteger(type) && (a < 0 || b < 0)) continue;
      // Every operator gets both a temporary RHS and a persistent RHS.
      for (bool temporary : {false, true}) {
        auto reset = [&] {
          c.assign("lhs", lit(a));
          c.assign("rhs", lit(b));
        };
        auto operand = [&] {
          return temporary ? c.add("rhs", lit(0)) : c.expr("rhs");
        };
        auto arithmetic = [&](auto operation, int expected) {
          c.scope().begin();
          reset();
          operation(operand());
          check("lhs", lit(expected));
          check("rhs", lit(b));
          c.endScope();
        };
        arithmetic([&](auto rhs) { c.addAssign("lhs", rhs); }, a + b);
        arithmetic([&](auto rhs) { c.subAssign("lhs", rhs); }, a - b);
        arithmetic([&](auto rhs) { c.mulAssign("lhs", rhs); }, a * b);
        arithmetic([&](auto rhs) { c.divAssign("lhs", rhs); }, a / b);
        arithmetic([&](auto rhs) { c.modAssign("lhs", rhs); }, a % b);
        auto boolean = [&](auto operation, bool expected) {
          c.scope().begin();
          reset();
          check(operation(operand()), literal::u8(expected));
          check("rhs", lit(b));
          c.endScope();
        };
        boolean([&](auto rhs) { return c.eq("lhs", rhs); }, a == b);
        boolean([&](auto rhs) { return c.neq("lhs", rhs); }, a != b);
        boolean([&](auto rhs) { return c.lt("lhs", rhs); }, a < b);
        boolean([&](auto rhs) { return c.le("lhs", rhs); }, a <= b);
        boolean([&](auto rhs) { return c.gt("lhs", rhs); }, a > b);
        boolean([&](auto rhs) { return c.ge("lhs", rhs); }, a >= b);
        boolean([&](auto rhs) { return c.land("lhs", rhs); }, a && b);
        boolean([&](auto rhs) { return c.lnand("lhs", rhs); }, !(a && b));
        boolean([&](auto rhs) { return c.lor("lhs", rhs); }, a || b);
        boolean([&](auto rhs) { return c.lnor("lhs", rhs); }, !(a || b));
        boolean([&](auto rhs) { return c.lxor("lhs", rhs); }, bool(a) != bool(b));
        boolean([&](auto rhs) { return c.lxnor("lhs", rhs); }, bool(a) == bool(b));
      }
    }
    c.endScope();
  }
  // Same temporary on both sides must retain the alias-safe operation path.
  c.declareLocal("x", ts::u8());
  c.assign("x", literal::u8(7));
  auto tmp = c.add(literal::u8(0), "x");
  c.assign(tmp, literal::u8(7));
  c.addAssign(tmp, tmp);
  check(tmp, literal::u8(14));
  c.returnFromFunction();
} c.endFunction();
TEST_END
