// tests/unsigned_widening_clears_high_byte.cc
// Expected: 66

TEST_BEGIN

c.function("main").begin(); {
  c.declareLocal("x", ts::u16());
  c.declareLocal("a", ts::u8());

  // Leave a nonzero high byte in x.
  c.assign("x", literal::u16(0x1200));

  // Assigning u8 -> u16 should produce 66, not 0x1242.
  c.assign("a", literal::u8(66));
  c.assign("x", "a");

  c.print("x");

  c.returnFromFunction();
} c.endFunction();

TEST_END
