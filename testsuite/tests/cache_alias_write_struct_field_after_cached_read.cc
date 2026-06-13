// Cache a dereferenced struct, then write a field through the same pointer and read it back.
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
  c.declareLocal("p", pairP);
  c.declareLocal("arr", arrT);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 1)));

  auto obj = c.dereferencePointer("p");

  c.write(c.structField(obj, "x"));
  c.assign(c.structField(obj, "x"), literal::u8('X'));
  c.write(c.structField(obj, "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
