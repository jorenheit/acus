// tests/global_cache_path_dependent_join.cc
// Expected: I
//
// This tries to expose a global cache entry that is created in a branch
// which is compiled first, but not executed at runtime.
//
// If internalBoundary keeps global cache entries alive, the join may read
// from the cache slot created by the true branch, even though runtime took
// the false branch and never wrote that cache slot.

TEST_BEGIN

c.declareGlobal("g", ts::u8());
c.declareGlobal("poison", ts::u8());

c.function("main").begin(); {
  c.declareLocal("cond", ts::u8());

  // Put the canonical global into a known state, then force it out of cache.
  c.assign("g", literal::u8('I'));
  c.callFunction("noop").done();

  // Poison a recently freed cache slot. This increases the chance that a
  // non-executed cached-global slot contains a visible wrong value instead
  // of accidentally containing 'I'.
  c.assign("poison", literal::u8('P'));
  c.callFunction("noop").done();

  // Runtime will take the false branch.
  c.assign("cond", literal::u8(0));
  c.jumpIf("cond", "truePath", "falsePath");

  // This branch is compiled first, so it creates a dirty cached entry for g.
  // But it is not executed at runtime.
  c.label("truePath");
  c.assign("g", literal::u8('T'));
  c.jump("join");

  // Runtime comes here. This path does not touch g.
  c.label("falsePath");
  c.jump("join");

  // Correct runtime value is still canonical global g == 'I'.
  // If the compiler kept the truePath cache entry alive across the internal
  // boundary, this may read the non-executed cache slot instead.
  c.label("join");
  c.write("g");

  c.returnFromFunction();
} c.endFunction();

c.function("noop").begin(); {
  c.returnFromFunction();
} c.endFunction();

TEST_END
