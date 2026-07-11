// tests/dynamic_array_scaled_offset_overflow.cc
// Expected: ZZCD

TEST_BEGIN

auto pairT = ts::array(ts::u8(), 2);
auto arrT  = ts::array(pairT, 129);

auto pairLiteral = [](char a, char b) {
  return literal::array(ts::array(ts::u8(), 2))
    .push(literal::u8(a))
    .push(literal::u8(b))
    .done();
};

c.function("main").begin(); {
  c.declareLocal("arr", arrT);
  c.declareLocal("idx", ts::u8());

  c.assign(c.arrayElement("arr", 0), pairLiteral('C', 'D'));
  c.assign(c.arrayElement("arr", 128), pairLiteral('A', 'B'));

  c.assign("idx", literal::u8(128));

  // Correct target: arr[128].
  // If scaled offset is stored as u8, 128 * 2 wraps to 0 and writes arr[0].
  c.assign(c.arrayElement("arr", "idx"), pairLiteral('Z', 'Z'));

  c.write(c.arrayElement("arr", 128));
  c.write(c.arrayElement("arr", 0));

  c.returnFromFunction();
} c.endFunction();

TEST_END
