// tests/logical_negative_constants.cc
// Expected: BAB

TEST_BEGIN

c.function("main").begin(); {
  c.declareLocal("x", ts::s8());

  c.assign("x", literal::s8(0));

  // false OR true -> true
  c.write(c.add(c.lor("x", literal::s8(-1)), literal::u8('A')));

  // false NOR true -> false
  c.write(c.add(c.lnor("x", literal::s8(-1)), literal::u8('A')));

  // false XOR true -> true
  c.write(c.add(c.lxor("x", literal::s8(-1)), literal::u8('A')));

  c.returnFromFunction();
} c.endFunction();

TEST_END
