// Copy between two dynamic proxy expressions that resolve to the same runtime element.
// Expected: "ABC"

TEST_BEGIN

auto u8 = ts::u8();
auto arrT = ts::array(u8, 3);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);
  c.declareLocal("j", u8);

  c.assign(c.arrayElement("arr", 0), literal::u8('A'));
  c.assign(c.arrayElement("arr", 1), literal::u8('B'));
  c.assign(c.arrayElement("arr", 2), literal::u8('C'));

  c.assign("i", literal::u8(1));
  c.assign("j", literal::u8(1));

  c.assign(c.arrayElement("arr", "i"), c.arrayElement("arr", "j"));

  c.write(c.arrayElement("arr", 0));
  c.write(c.arrayElement("arr", 1));
  c.write(c.arrayElement("arr", 2));

  c.returnFromFunction();
} c.endFunction();

TEST_END
