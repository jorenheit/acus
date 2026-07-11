// tests/comparison_le_unsigned_max_edge.cc
// Expected: BBBB

TEST_BEGIN

c.function("main").begin(); {
  c.declareLocal("a", ts::u8());
  c.declareLocal("x", ts::u16());

  c.assign("a", literal::u8(123));
  c.write(c.add(c.le("a", literal::u8(255)), literal::u8('A')));

  c.assign("a", literal::u8(255));
  c.write(c.add(c.le("a", literal::u8(255)), literal::u8('A')));

  c.assign("x", literal::u16(12345));
  c.write(c.add(c.le("x", literal::u16(65535)), literal::u8('A')));

  c.assign("x", literal::u16(65535));
  c.write(c.add(c.le("x", literal::u16(65535)), literal::u8('A')));

  c.returnFromFunction();
} c.endFunction();

TEST_END
