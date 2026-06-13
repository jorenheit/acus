# Acus SlotProxy Materialization Cache Strategy

## Purpose

The `SlotProxy` abstraction describes logical storage locations. Some proxies can be resolved directly to a concrete `Slot`, while others require runtime work before they can be accessed. Examples include dynamic array elements, nested array elements, global references, struct fields of dynamic objects, and dereferenced pointers.

The materialization cache exists to avoid repeatedly resolving the same logical object and immediately writing it back after each small subobject modification.

For example:

```cpp
arr[i][j].x = 1;
arr[i][j].y = 2;
```

should be able to materialize `arr[i][j]` once, modify both fields in the cached slot, and flush the result back only when required.

This document describes the current conservative cache strategy, including the special cases needed for direct-relative proxies, nested cache entries, globals, pointer dereferences, alias-sensitive writes, and source protection.

---

## High-level model

The cache is owned by `Assembler::Cache`. A `SlotProxy` describes a logical location; it does not own cache state.

A cache entry represents one materialized logical object:

```text
proxy           logical object represented by this entry
slot            cache slot containing the materialized value
dirty           cached value must be committed before deletion
parent          nearest enclosing cached entry, if any
children        cached entries nested below this entry
markedForDelete temporary deletion marker used during invalidation
```

The cache uses `SlotProxy` identity/equality internally. Callers should not manually construct cache keys or depend on string names for cache management.

---

## Proxy categories

The cache logic depends heavily on the kind of proxy being handled.

### Direct-absolute proxies

A direct-absolute proxy resolves to canonical storage by itself. Local variables are typical examples.

```text
x -> actual local slot for x
```

Writes to these locations are direct writes to canonical storage. They may still invalidate other cached entries that depend on the modified proxy.

### Direct-relative proxies

A direct-relative proxy resolves to a direct subslot relative to its parent. Struct fields are the most common example.

```text
h.x        direct relative to h
arr[0]     direct relative to arr, if the index is constant
```

A direct-relative proxy is not necessarily direct in the absolute sense. If the parent is cached, the direct-relative child is located inside the cached parent slot.

Example:

```text
parent h is cached in __cache_0
h.x is direct relative to __cache_0, not necessarily direct canonical storage
```

This distinction matters when writing to direct-relative proxies. A write to a direct-relative child must mark the nearest cached owner dirty if such an owner exists.

### Non-direct proxies

A non-direct proxy requires runtime work to resolve and is normally cacheable.

Examples:

```text
arr[i]
arr[i][j]
global g accessed through GlobalReference
deref<p>
```

These proxies can have cache entries.

### Alias-sensitive proxies

A proxy is alias-sensitive if it depends on a dereferenced pointer.

Examples:

```text
*p
(*p).x
(*p)[i]
arr[*p]
```

The exact naming in code is currently `dependsOnDereferencedPointer()`. This property must be recursive: wrapper proxies such as `StructField` and `ArrayElement` must report true if any relevant parent/index/source proxy depends on a dereferenced pointer.

Alias-sensitive proxies are handled conservatively as cache boundaries.

---

## Parent links versus dependencies

Parent links and dependency relations are different.

A parent link is a cache-internal relation used for flushing dirty data upward:

```text
arr[i][j] -> parent arr[i]
arr[i]    -> parent arr, if arr is cached
```

A dependency relation records which proxy modifications make a cache entry stale:

```text
arr[i] depends on arr and i
arr[i][j] depends on arr[i] and j
*deref<p> / deref<p> depends on p
```

These relations are used differently:

```text
parent links:
  determine flush order and write-back target inside the cache tree

dependencies:
  determine which entries must be invalidated before an external write
```

A cache-internal flush is not an external write and must not run the ordinary dependency invalidation path.

---

## Dirty state

A cache entry is dirty if its slot differs from its immediate backing store.

Dirty state is local to one cache level. If a dirty child is flushed into its parent, the child becomes clean and the parent becomes dirty.

Example before flushing:

```text
cache[arr[i][j]] dirty
cache[arr[i]]    clean
```

After flushing the child into the parent:

```text
cache[arr[i][j]] clean
cache[arr[i]]    dirty
```

A dirty parent and dirty child can coexist. The flush algorithm must therefore flush deepest-first.

Important rule:

```text
Dirty entries may never be deleted or freed without first being committed.
```

---

## Entry creation and `ensureParentEntry`

When a non-direct proxy gets a cache entry, its nearest cacheable enclosing parent must also exist, unless there is no such parent.

This is the job of `ensureParentEntry`.

The need for this comes from the write-materialization optimization where a destination cache entry can be created with `skipMaterialization=true`. In that case the entry is dirty immediately, but its parent may not have been materialized naturally.

Example:

```cpp
m[row][col] = 'X';
write(m);
```

Both `m[row]` and `m[row][col]` are non-direct/cacheable. If only `m[row][col]` is created, the cache contains an orphan dirty entry:

```text
Entry: m[row][col]
parent = nullptr
```

Then a subtree flush rooted at `m` or `m[row]` cannot find the dirty grandchild.

Correct structure:

```text
Entry: m[row]
  child -> Entry: m[row][col]
```

`ensureParentEntry` therefore creates or finds the nearest cacheable enclosing parent first and links the new child under it.

Direct-relative parents are skipped while searching for the nearest cacheable owner. Example:

```cpp
h.data[i] = 'X';
```

Here `h.data` may be direct-relative and not cacheable as its own entry, while `h.data[i]` is non-direct. The cache should climb past `h.data` to find the nearest cacheable enclosing owner.

Invariant:

```text
Every cached non-direct subobject is linked under its nearest cached/cacheable enclosing owner.
```

---

## Materialization

Materialization must go through the cache-aware interface.

### Read materialization

For a read materialization:

1. If the proxy is alias-sensitive, perform the conservative alias-read boundary first: flush and clear the entire cache.
2. If the proxy is direct-relative, resolve it relative to its parent.
3. Otherwise find or create a cache entry.
4. If an entry exists, flush dirty descendants into it before returning its slot.
5. If no entry exists, allocate a cache slot, materialize the proxy into it, register the entry, and return the slot.

Read materialization does not mark the entry dirty.

Alias-read boundary motivation:

```cpp
g = 'G';      // dirty cached global
write(*pg);   // *pg reads canonical memory
```

If dirty cache entries are not flushed first, the pointer read can observe stale canonical storage.

### Write materialization

Write materialization prepares a cache entry to be mutated.

For non-direct entries:

1. Find or create the exact entry.
2. Flush dirty descendants into it.
3. Delete descendants, because the parent slot is about to become mutable and descendants may become stale.
4. Return the entry slot.

Whether the entry is marked dirty depends on context. Ordinary `Cache::write` marks the destination entry dirty after assigning into it. During alias-sensitive writes, `_aliasWriteMode` ensures that entries materialized with write intent are treated as dirty, because a proxy such as `StructField` may mutate a subslot of the materialized parent without explicitly marking the parent dirty.

This case was the important example:

```cpp
(*p).x = 'X';
```

`StructField::write` materializes `*p` for write, modifies the `.x` subslot, and returns. The cache must treat the materialized `deref<p>` entry as dirty so that the subsequent alias write boundary flush commits it through the pointer.

---

## External writes

`Cache::write` is the external/user-level write entry point. It should not be called while the cache is flushing.

Recommended invariant:

```cpp
assert(!_flushing);
```

Dirty-entry flushing should use an internal commit/write-back path and must not re-enter external `Cache::write`.

The write logic has three main paths:

```text
alias-sensitive write
  hard cache boundary around the semantic write

direct-relative write
  write directly into parent/canonical storage and mark cached owner dirty if needed

cached/non-direct write
  write into destination cache entry and mark that entry dirty
```

### Alias-sensitive writes

If the destination or source depends on a dereferenced pointer, the write is alias-sensitive.

Conservative strategy:

```text
flush and clear entire cache
enter alias-write mode
perform the semantic proxy write
leave alias-write mode
flush and clear entire cache
```

This handles both direct pointer writes and subobject writes through pointers:

```cpp
*p = value;
(*p).x = value;
(*p)[i] = value;
x = *p;
```

The cache is cleared before the write so pending dirty cache entries cannot be written back later in the wrong order. The cache is cleared after the write so no clean cached entries remain stale after an alias mutation.

`_aliasWriteMode` is needed because semantic proxy writes may materialize parent objects with write intent and mutate their subslots without explicitly marking them dirty. During alias write mode, such write-intent materializations must be committed before clearing.

### Direct-relative writes

For direct-relative destinations:

```cpp
if (dest->directRelative()) {
  flushAndDeleteSubtree(dest);
  invalidateDependencies(dest);
  dest->write(_self, src);

  if (Entry *owner = findCachedOwner(dest))
    owner->dirty = true;
}
```

This is necessary because a direct-relative proxy may be a subslot inside a cached parent.

Example:

```cpp
g.x = 'A';
```

If `g` is cached, `g.x` writes into the cached `g` slot. The cache entry for `g` must be marked dirty, otherwise the later flush/clear will discard the modified cached object instead of committing it.

### Cached/non-direct writes

For non-direct destinations:

```cpp
Entry &entry = findOrCreateEntry(dest, true);
flushAndDeleteSubtree(entry, false);
invalidateDependencies(dest);
assign(entry.slot);
entry.dirty = true;
```

The destination entry is created with materialization skipped because the old value is not needed when overwriting it. This is the optimization that makes `ensureParentEntry` necessary.

---

## Dependency invalidation

Before a structural external write modifies a proxy, cached entries that depend on that proxy must be invalidated.

Example:

```cpp
arr[i] = 'A';  // may cache arr[i]
i = i + 1;     // must flush/delete arr[i] before changing i
```

If invalidation were delayed until after `i` changed, flushing `arr[i]` could write back to the wrong element.

`invalidateDependencies(modifiedProxy)` now assumes that `modifiedProxy` itself is not alias-sensitive:

```cpp
assert(!modifiedProxy->dependsOnDereferencedPointer());
```

Alias-sensitive writes are handled before this function is reached.

However, dependent cached entries may themselves be pointer-resolved. This is still important.

Example:

```cpp
write(*p);   // cache deref<p>
p += 1;      // modifiedProxy = p
write(*p);   // must not reuse old deref<p>
```

Here `p` does not depend on a dereferenced pointer, but the existing cached entry `deref<p>` does. If an invalidated dependent entry is pointer-resolved, the conservative strategy is to flush and clear the entire cache.

So this check should remain:

```text
if any dependent entry is storage-resolved through a pointer dereference:
  flush and clear entire cache
```

This prevents dirty pointer-resolved entries from being committed after the pointer/index they depend on has changed.

---

## Flushing and committing

Flushing means committing dirty entries one level toward their backing storage.

If a dirty entry has a cached parent:

```text
flush child into cached parent
mark child clean
mark parent dirty
```

If a dirty entry has no cached parent:

```text
write entry back to its represented storage
mark entry clean
```

Dirty entries must be flushed deepest-first.

External writes should not be called during flushing. Internal cache commits should call proxy-level writeback or a raw commit helper, not the external `Cache::write` entry point.

This avoids reentrant cache maintenance:

```text
flushAndClearEntireCache
  flush dirty deref<p>
    external Cache::write(deref<p>, ...)
      flushAndClearEntireCache again  // not allowed
```

`_flushing` is therefore a guard for internal maintenance. It is not a mode in which user-level writes should run.

---

## Subtree traversal and direct-relative gaps

Entry-based subtree traversal can rely on cache parent/child links.

Proxy-rooted subtree traversal must be more careful, because the semantic proxy tree may contain direct-relative gaps that have no cache entries.

Example:

```cpp
h.data[i] = 'X';
```

Semantic tree:

```text
h
└── h.data       direct-relative, no cache entry
    └── h.data[i] cached entry
```

A proxy-rooted flush/delete for `h` must still find `h.data[i]`. Therefore proxy-rooted subtree logic should determine whether a cached entry is part of the semantic subtree by walking its `enclosingProxy()` chain, not only by checking the immediate parent.

A safe approach:

```text
collect top-level cached entries whose proxy has root in its enclosing-proxy chain
for each collected root:
  traverse the entry subtree deepest-first
```

Collect roots before mutating/deleting entries.

---

## Globals

Globals are represented through `GlobalReference` proxies rather than through the old local-mirror global system.

A global reference behaves like a cacheable proxy whose backing storage is canonical global storage.

Important rules:

```text
writes to global references may cache and dirty the global value
control boundaries must flush dirty global entries before calls/returns as required
direct-relative writes into cached global subobjects must mark the global owner dirty
```

The call setup order matters. Dirty caller state must be flushed before switching/preparing frames in a way that would make global writes invisible or stale.

For return values written into globals, copying return data into a cached global slot is not enough unless the cache is flushed at the meta/control boundary before jumping onward.

---

## Pointers and aliasing

Pointer dereference introduces runtime aliasing. The proxy tree alone cannot tell which canonical object `*p` will touch.

Therefore the current conservative rule is:

```text
any read or write involving a dereferenced pointer is a hard cache boundary
```

### Pointer reads

Before reading through a pointer, dirty cache entries that may be observed by the pointer must be committed.

Current conservative implementation:

```text
if proxy depends on dereferenced pointer:
  flush and clear entire cache
```

### Pointer writes

Before writing through a pointer, dirty cache entries must be committed so that the pointer write is not later overwritten by an old dirty cache value.

After writing through a pointer, the cache must be empty or invalidated so that no clean entry remains stale.

Current conservative implementation:

```text
flush and clear entire cache
perform write in alias-write mode
flush and clear entire cache
```

### Subobject writes through pointers

Subobject writes through pointers are the tricky case:

```cpp
(*p).x = 'X';
```

`StructField::write` may materialize `*p`, modify only its `.x` subslot, and return. Without `_aliasWriteMode`, the materialized `deref<p>` entry may remain clean and then be discarded without writeback.

During alias write mode, entries materialized with write intent must be treated as dirty. This makes the final flush write the modified object back through the dereferenced pointer.

### Pointer arithmetic

Pointer arithmetic must be scaled by the pointee type size:

```text
p + n means p.offset + n * sizeof(*p)
```

The pointer offset must use the same unit as the runtime pointer representation.

### Pointer dependency invalidation

When a pointer variable changes, cached dereferences depending on that pointer must be invalidated before the change takes effect.

Example:

```cpp
write(*p);  // may cache deref<p>
p += 1;     // invalidates deref<p>
```

This is why `invalidateDependencies` must still treat pointer-resolved dependent entries conservatively, even though the modified proxy itself is not alias-sensitive.

---

## Source protection during writes

A write operation has two conceptual sides:

```text
destination preparation
source materialization/copy
```

Preparing the destination may flush, invalidate, delete, and reuse cache slots. If the source was already stored in a cache slot that gets deleted during destination preparation, the final copy can read the wrong value.

Dangerous examples:

```cpp
arr[i] = arr[j];
arr[i].x = arr[j].x;
(*p).x = arr[i].x;
arr[i].x = (*p).x;
```

Invariant:

```text
During Cache::write(dest, src), any materialized source slot must remain valid until the final copy into dest has completed.
```

Two possible strategies:

1. Copy the source into a protected scratch slot before destructive destination preparation.
2. Pin the source cache entry so it may be flushed but not deleted/freed/reused until the write completes.

Pinning avoids unnecessary generated copies but requires extra cache-entry state such as `pinCount` or a temporary protected state. A pinned entry must not be freed or have its cache slot reused. If it is marked for deletion while pinned, deletion should be deferred until unpinning.

This is planned/required work if not already implemented.

---

## Self-assignment

Self-assignment should be detected early:

```cpp
if (dest == src) return;
```

This avoids attempting to copy a slot onto itself through lower-level primitives that require distinct source/target/tmp locations.

It is still useful to have a lower-level `assignSlot` guard for identical concrete slots as a second line of defense.

---

## Control boundaries

The conservative strategy is to flush/clear at control boundaries where cache state must become visible to canonical storage or where aliasing/control-flow reasoning becomes difficult.

Examples:

```text
function calls
function pointer calls
returns
branch boundaries
block transitions
function end
other control-flow boundaries
```

The named function-call path should flush before preparing/switching frames, matching the safer function-pointer call order.

After a boundary, blocks that assume a clean cache may assert that the cache is empty.

---

## Expected invariants

The cache should maintain these invariants:

1. A cache entry owns one materialized logical object.
2. Dirty means the cached slot must be committed before deletion.
3. Dirty data is flushed deepest-first, child before parent.
4. A cache entry handed out for writing has no live cached descendants afterward.
5. Every cached non-direct subobject is linked under its nearest cached/cacheable enclosing owner.
6. Direct-relative writes into cached owners mark those owners dirty.
7. External writes invalidate affected dependencies before modifying the proxy.
8. Alias-sensitive external writes are hard cache boundaries.
9. During alias write mode, write-intent materializations are treated as dirty.
10. Dependency invalidation is not run for cache-internal flushes.
11. External `Cache::write` is not called while `_flushing` is true.
12. Dirty entries are never deleted without being flushed/committed.
13. Clean entries may be deleted/cleared without writeback.
14. Deleting an entry also deletes or detaches its descendants safely.
15. Parent/child links are maintained only by the cache system.
16. Source slots are protected during writes once source protection is implemented.

---

## Suggested stress tests

The cache should have focused tests for the following patterns.

### Nested dynamic subobjects

```cpp
m[row][col] = 'X';
write(m);
```

Checks `ensureParentEntry` and deepest-first flushing.

### Direct-relative gap

```cpp
h.data[i] = 'X';
write(h);
```

Checks proxy-rooted subtree traversal across direct-relative proxies with no cache entry.

### Direct-relative write into cached owner

```cpp
g.x = 'A';
g.y = 'B';
controlBoundary();
write(g);
```

Checks cached owner dirty marking.

### Global scalar and global struct across calls

```cpp
g = 'A';
foo();
write(g);
```

and

```cpp
g.x = 'A';
g.y = 'B';
foo();
write(g);
```

Checks global flush and call-boundary ordering.

### Pointer read after dirty cache

```cpp
p = &g;
g = 'G';
write(*p);
```

Checks alias-read boundary.

### Pointer write invalidates clean cache

```cpp
p = &x;
write(x);
*p = 'Y';
write(x);
```

Checks alias-write boundary.

### Subobject write through pointer

```cpp
p = &arr[0];
p += 1;
(*p).x = 'X';
write(arr[1].x);
```

Checks `_aliasWriteMode` and write-intent dirty marking.

### Pointer variable modification invalidates deref cache

```cpp
write(*p);
p += 1;
write(*p);
```

Checks pointer-resolved dependent entries in `invalidateDependencies`.

### Source protection

```cpp
arr[i] = arr[j];
arr[i].x = arr[j].x;
```

Run with `i == j` and `i != j`. Checks source pin/copy protection.

### Self-assignment

```cpp
arr[i] = arr[i];
arr[i].x = arr[i].x;
```

Checks early no-op and low-level same-slot protection.

---

## Current conservative stance

The current design deliberately favors correctness over optimal pointer-heavy code generation.

In particular:

```text
dereferenced pointer access acts as a cache boundary
```

This means sequences such as:

```cpp
*p = 3;
*p += x;
```

will not benefit much from caching yet. Supporting that efficiently would require an alias-aware/pointer-region optimization where the compiler can prove that the pointer expression remains stable and no other aliasing operation intervenes.

That is a future optimization. The current goal is a simple, robust, conservative cache.
