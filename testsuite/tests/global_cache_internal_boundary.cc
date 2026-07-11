// tests/global_cache_internal_boundary.cc
// Expected: T

TEST_BEGIN

c.declareGlobal("g", ts::u8());

c.function("main").begin(); {
  c.declareLocal("cond", ts::u8());

  c.assign("cond", literal::u8(1));
  c.jumpIf("cond", "thenBranch", "elseBranch");

  c.label("thenBranch");
  c.assign("g", literal::u8('T'));
  c.jump("join");

  c.label("elseBranch");
  c.assign("g", literal::u8('F'));
  c.jump("join");

  c.label("join");
  c.write("g");

  c.returnFromFunction();
} c.endFunction();

TEST_END
