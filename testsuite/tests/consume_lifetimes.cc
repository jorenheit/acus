// Regression coverage for caller-owned consumed temporary allocations.
TEST_BEGIN
c.function("main").begin(); {
  c.declareLocal("x", ts::u8());
  c.declareLocal("a", ts::array(ts::u8(), 3));
  c.assign("x", literal::u8(1));
  auto index = c.add("x", literal::u8(0));
  c.assign(c.arrayElement("a", index), index);
  c.write(c.add(c.arrayElement("a", 1), literal::u8('A'))); // B
  auto same = c.add("x", literal::u8(0));
  c.assign(same, same);
  c.addAssign(same, same);
  c.write(c.add(same, literal::u8('A'))); // C
  auto narrow = c.add("x", literal::u8(0));
  c.write(c.add(c.eq(c.add(narrow, literal::u16(1)),
                     literal::u16(2)), literal::u8('A'))); // B
  auto arg = c.add("x", literal::u8('A'));
  c.callFunction("echo2").arg(arg).arg(arg).done(); // BB
  auto idx = c.add("x", literal::u8(0));
  c.callFunction("echo2").arg(idx).arg(c.arrayElement("a", idx)).done(); // 1,1
  auto fn = ts::function().param(ts::u8()).done();
  c.declareLocal("callbacks", ts::array(ts::function_pointer(fn), 2));
  c.assign(c.arrayElement("callbacks", 1), literal::function_pointer(fn, "echo1"));
  auto callbackIndex = c.add("x", literal::u8(0));
  c.callFunctionPointer(c.arrayElement("callbacks", callbackIndex)).arg(callbackIndex).done(); // 1
  auto condition = c.add("x", literal::u8(0));
  c.jumpIf(condition, "yes", "no");
  c.label("yes"); c.write(literal::u8('T')); c.jump("end");
  c.label("no"); c.write(literal::u8('F')); c.jump("end");
  c.label("end");
  // Reusing the return destination as a consumed argument must keep its allocation.
  auto result = c.add("x", literal::u8(0));
  c.callFunction("identity").into(result).arg(result).done();
  c.write(c.add(result, literal::u8('A'))); // B
  c.returnFromFunction();
} c.endFunction();
c.function("echo2").param("first", ts::u8()).param("second", ts::u8()).begin(); {
  c.write("first"); c.write("second"); c.returnFromFunction();
} c.endFunction();
c.function("echo1").param("value", ts::u8()).begin(); {
  c.write("value"); c.returnFromFunction();
} c.endFunction();
c.function("identity").param("value", ts::u8()).ret(ts::u8()).begin(); {
  c.returnFromFunction("value");
} c.endFunction();
TEST_END
