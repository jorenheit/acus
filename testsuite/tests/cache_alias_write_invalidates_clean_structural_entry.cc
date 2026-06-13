// Cache a structural dynamic field, write the same field through a pointer, then read structurally again.
// Expected: "BX"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);
auto arrT = ts::array(pairT, 3);

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);
  c.declareLocal("p", pairP);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("i", literal::u8(1));
  c.write(c.structField(c.arrayElement("arr", "i"), "x"));

  c.assign("p", c.addressOf(c.arrayElement("arr", 1)));
  c.assign(c.structField(c.dereferencePointer("p"), "x"), literal::u8('X'));

  c.write(c.structField(c.arrayElement("arr", "i"), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
