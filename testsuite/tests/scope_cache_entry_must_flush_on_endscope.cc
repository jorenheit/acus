// tests/scope_cache_entry_must_flush_on_endscope.cc
// Expected: abcd
//
// This test exposes stale cache entries created inside an internal scope.
// If endScope() does not flush/clear the cache before freeing scoped locals,
// the dirty cached arr[idx] entry can survive past the scope. The storage for
// arr can then be reused by victim, and a later call boundary may flush the
// stale arr[idx] entry into victim.

TEST_BEGIN

auto array4 = ts::array(ts::u8(), 4);

c.function("main").begin(); {
  c.scope().begin(); {
    c.declareLocal("arr", array4);
    c.declareLocal("idx", ts::u8());

    c.assign("arr",
	     literal::array(array4)
	       .push(literal::u8('A'))
	       .push(literal::u8('B'))
	       .push(literal::u8('C'))
	       .push(literal::u8('D'))
	       .done());

    c.assign("idx", literal::u8(3));

    // Creates a dirty cache entry for arr[idx].
    // If endScope does not flush this entry, it survives after arr is freed.
    c.assign(c.arrayElement("arr", "idx"), literal::u8('Z'));

  } c.endScope();

  // This should reuse the same frame storage that arr used.
  // If the stale arr[idx] cache entry survives, it may later write into victim.
  c.declareLocal("victim", array4);

  c.assign("victim",
	   literal::array(array4)
	     .push(literal::u8('a'))
	     .push(literal::u8('b'))
	     .push(literal::u8('c'))
	     .push(literal::u8('d'))
	     .done());

  // Force a cache boundary after victim has been initialized.
  // Without the endScope boundary, the stale arr[idx] entry can flush here.
  c.callFunction("noop").done();

  c.write("victim");

  c.returnFromFunction();
} c.endFunction();

c.function("noop").begin(); {
  c.returnFromFunction();
} c.endFunction();

TEST_END
