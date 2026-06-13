// Move a struct pointer through an array, write through it, then read the array fields.
// Expected: "AXC"

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

  c.assign("p", c.addressOf(c.arrayElement("arr", 0)));
  c.addAssign("p", literal::u16(1));

  auto obj = c.dereferencePointer("p");
  c.assign(c.structField(obj, "x"), literal::u8('X'));

  c.write(c.structField(c.arrayElement("arr", 0), "x"));
  c.write(c.structField(c.arrayElement("arr", 1), "x"));
  c.write(c.structField(c.arrayElement("arr", 2), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
