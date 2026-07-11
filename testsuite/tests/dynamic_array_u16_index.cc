// tests/dynamic_array_u16_index.cc
// Expected: Z

TEST_BEGIN

auto arrT = ts::array(ts::u8(), 4);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("idx", ts::u16());

  c.assign(c.arrayElement("arr", 0), literal::u8('A'));
  c.assign(c.arrayElement("arr", 1), literal::u8('B'));
  c.assign(c.arrayElement("arr", 2), literal::u8('C'));
  c.assign(c.arrayElement("arr", 3), literal::u8('Z'));

  c.assign("idx", literal::u16(3));
  c.write(c.arrayElement("arr", "idx"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
