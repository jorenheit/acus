// Cache a dereferenced pointer, change the pointer variable, then dereference it again.
// Expected: "AB"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);
auto arrT = ts::array(pairT, 2);

c.function("main").begin(); {
  c.declareLocal("p", pairP);
  c.declareLocal("arr", arrT);

  c.assign(c.structField(c.arrayElement("arr", 0), "x"), literal::u8('A'));
  c.assign(c.structField(c.arrayElement("arr", 1), "x"), literal::u8('B'));

  c.assign("p", c.addressOf(c.arrayElement("arr", 0)));
  c.write(c.structField(c.dereferencePointer("p"), "x"));

  c.assign("p", c.addressOf(c.arrayElement("arr", 1)));
  c.write(c.structField(c.dereferencePointer("p"), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
