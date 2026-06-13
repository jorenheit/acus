// Read through a pointer after a dirty dynamic array field write; the pointer read must see the flushed value.
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
  c.declareLocal("arr", arrT);
  c.declareLocal("i", u8);
  c.declareLocal("p", pairP);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));
  c.assign(c.structField(c.arrayElement("arr", 2), "x"), literal::u8('C'));

  c.assign("i", literal::u8(1));
  c.assign(c.structField(c.arrayElement("arr", "i"), "x"), literal::u8('X'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 1)));

  auto obj = c.dereferencePointer("p");

  c.write(c.structField(c.arrayElement("arr", 0), "x"));
  c.write(c.structField(obj, "x"));
  c.write(c.structField(c.arrayElement("arr", 2), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
