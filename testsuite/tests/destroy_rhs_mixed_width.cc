// Widening and consuming an 8-bit RHS must preserve its original local.
TEST_BEGIN
c.function("main").begin(); {
  auto check = [&](auto const &value, auto const &expected) {
    c.write(c.add(c.eq(value, expected), literal::u8('S')));
  };
  for (bool signedOperands : {false, true}) {
    c.scope().begin();
    c.declareLocal("lhs", signedOperands ? ts::s16() : ts::u16());
    c.declareLocal("rhs", signedOperands ? ts::s8() : ts::u8());
    auto wide = [&](int value) -> literal::Literal {
      return signedOperands ? literal::s16(value) : literal::u16(value);
    };
    auto narrow = [&](int value) -> literal::Literal {
      return signedOperands ? literal::s8(value) : literal::u8(value);
    };
    int const a = signedOperands ? -400 : 400;
    int b = 3;
    auto reset = [&] { c.assign("lhs", wide(a)); c.assign("rhs", narrow(b)); };
    auto operand = [&] { return c.add("rhs", narrow(0)); };
    auto arithmetic = [&](auto operation, int expected) {
      c.scope().begin();
      reset(); operation(operand());
      check("lhs", wide(expected)); check("rhs", narrow(b));
      c.endScope();
    };
    arithmetic([&](auto rhs) { c.addAssign("lhs", rhs); }, a+b);
    arithmetic([&](auto rhs) { c.subAssign("lhs", rhs); }, a-b);
    arithmetic([&](auto rhs) { c.mulAssign("lhs", rhs); }, a*b);
    arithmetic([&](auto rhs) { c.divAssign("lhs", rhs); }, a/b);
    arithmetic([&](auto rhs) { c.modAssign("lhs", rhs); }, a%b);
    auto comparison = [&](auto operation, bool expected) {
      c.scope().begin();
      reset(); check(operation(operand()), literal::u8(expected));
      check("rhs", narrow(b));
      c.endScope();
    };
    comparison([&](auto rhs) { return c.lt("lhs", rhs); }, a<b);
    comparison([&](auto rhs) { return c.le("lhs", rhs); }, a<=b);
    comparison([&](auto rhs) { return c.gt("lhs", rhs); }, a>b);
    comparison([&](auto rhs) { return c.ge("lhs", rhs); }, a>=b);
    if (signedOperands) {
      b = -3;
      arithmetic([&](auto rhs) { c.mulAssign("lhs", rhs); }, a*b);
      arithmetic([&](auto rhs) { c.divAssign("lhs", rhs); }, a/b);
      arithmetic([&](auto rhs) { c.modAssign("lhs", rhs); }, a%b);
    }
    c.endScope();
  }
  c.returnFromFunction();
} c.endFunction();
TEST_END
