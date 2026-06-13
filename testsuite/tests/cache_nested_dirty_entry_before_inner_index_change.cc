// Write to a nested dynamic element, then change the inner index; the dirty grandchild must be flushed.
// Expected: "ABXD"

TEST_BEGIN

auto u8 = ts::u8();
auto rowT = ts::array(u8, 2);
auto matrixT = ts::array(rowT, 2);

c.function("main").begin(); {
  c.declareLocal("m", matrixT);
  c.declareLocal("row", u8);
  c.declareLocal("col", u8);

  c.assign(c.arrayElement(c.arrayElement("m", 0), 0), literal::u8('A'));
  c.assign(c.arrayElement(c.arrayElement("m", 0), 1), literal::u8('B'));
  c.assign(c.arrayElement(c.arrayElement("m", 1), 0), literal::u8('C'));
  c.assign(c.arrayElement(c.arrayElement("m", 1), 1), literal::u8('D'));

  c.assign("row", literal::u8(1));
  c.assign("col", literal::u8(0));

  c.assign(c.arrayElement(c.arrayElement("m", "row"), "col"), literal::u8('X'));

  c.assign("col", literal::u8(1));

  c.write(c.arrayElement(c.arrayElement("m", 0), 0));
  c.write(c.arrayElement(c.arrayElement("m", 0), 1));
  c.write(c.arrayElement(c.arrayElement("m", 1), 0));
  c.write(c.arrayElement(c.arrayElement("m", 1), 1));

  c.returnFromFunction();
} c.endFunction();

TEST_END
