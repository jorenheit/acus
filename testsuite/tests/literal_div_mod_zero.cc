// tests/literal_div_mod_zero.cc
// Expected: ABCD

TEST_BEGIN

c.function("main").begin(); {
  // Runtime semantics:
  // 100 / 0 -> 255, then +66 wraps to 'A'
  c.write(c.add(c.div(literal::u8(100), literal::u8(0)), literal::u8(66)));

  // 0 / 0 -> 0, then +'B'
  c.write(c.add(c.div(literal::u8(0), literal::u8(0)), literal::u8('B')));

  // 100 % 0 -> 0, then +'C'
  c.write(c.add(c.mod(literal::u8(100), literal::u8(0)), literal::u8('C')));

  // 0 % 0 -> 0, then +'D'
  c.write(c.add(c.mod(literal::u8(0), literal::u8(0)), literal::u8('D')));

  c.returnFromFunction();
} c.endFunction();

TEST_END
