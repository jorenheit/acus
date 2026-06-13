// Dirty a global struct field and then read it through a pointer.
// Expected: "X"

TEST_BEGIN

auto u8 = ts::u8();

auto pairT = ts::defineStruct("Pair")
  .field("x", u8)
  .field("y", u8)
  .done();

auto pairP = ts::pointer(pairT);

c.declareGlobal("g", pairT);

c.function("main").begin(); {
  c.declareLocal("p", pairP);

  c.assign(c.structField("g", "x"), literal::u8('A'));

  c.assign("p", c.addressOf("g"));
  c.assign(c.structField("g", "x"), literal::u8('X'));

  c.write(c.structField(c.dereferencePointer("p"), "x"));

  c.returnFromFunction();
} c.endFunction();

TEST_END
