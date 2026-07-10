// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"
#include "acus/util/util.h"

Assembler::Cache::Entry* Assembler::Cache::findEntry(SlotProxy proxy) const {
  auto it = std::find_if(_entries.begin(), _entries.end(), [&](EntryPtr const &entry){
    return entry->proxy == proxy;
  });
  return it == _entries.end() ? nullptr : it->get();
}

Assembler::Cache::Entry *Assembler::Cache::findCachedOwner(SlotProxy proxy) const {
  std::optional<SlotProxy> parent = proxy.enclosingProxy();
  while (parent) {
    if (Entry *entry = findEntry(*parent)) return entry;
    parent = (*parent).enclosingProxy();
  }
  return nullptr;
}

Assembler::Cache::Entry* Assembler::Cache::ensureParentEntry(SlotProxy proxy) {
  // Make sure that the proxy has a parent cached if it needs to be written
  // back indirectly; no dangling cached children.
  
  auto parentProxy = proxy.enclosingProxy();

  // Direct-relative proxies are not cache entries themselves. They are views
  // into their enclosing storage owner, so climb past them.
  while (parentProxy && (*parentProxy).direct()) {
    parentProxy = (*parentProxy).enclosingProxy();
  }

  // No cacheable enclosing proxy. The proxy is rooted in canonical storage.
  if (!parentProxy) return nullptr;

  // The parent must be fully materialized, even if the child itself is being
  // created with skipMaterialization=true. The child may later need to flush
  // into this parent while preserving the rest of the parent value.
  return &findOrCreateEntry(*parentProxy, false);
}

Assembler::Cache::Entry& Assembler::Cache::findOrCreateEntry(SlotProxy proxy, bool const skipMaterialization) {
  // Direct proxies should never be cached.
  assert(proxy.direct() == false);

  // If this proxy was already cached, return the cached entry.
  if (Entry *entry = findEntry(proxy)) {
    return *entry;
  }

  // Establish ownership before creating/materializing the child entry.
  // This is needed for nested write-intent entries. For example, when creating
  // a dirty entry for m[row][col] without reading the old cell value (skipMaterialization
  // is set to true), m[row] must still exist as the parent entry so the cell can later flush into it.

  Entry *parentEntry = ensureParentEntry(proxy);
  Slot const cacheSlot = _self.getCache(proxy.type());
  if (not skipMaterialization) {
    proxy.materialize(_self, cacheSlot);
  }

  auto newEntry = std::make_unique<Entry>(Entry{
      .proxy = proxy,
      .slot = cacheSlot,
      .dirty = _aliasWriteMode || skipMaterialization,
      .pendingWrite = skipMaterialization,
      .markedForDelete = false,
      .parent = parentEntry,
      .children = {}
    });

  Entry *raw = newEntry.get();    
  if (parentEntry != nullptr) {
    parentEntry->children.push_back(raw);    
  }

  _entries.emplace_back(std::move(newEntry));
  return *raw;
}

Slot Assembler::Cache::materialize(SlotProxy proxy) {
  if (proxy.dependsOnDereferencedPointer() && not _flushing && not _aliasWriteMode) {
    flushAndClearRoots();
  }
  
  if (proxy.direct()) {
    if (not _flushing) flushSubtree(proxy);
    return proxy.materialize(_self);
  }
  
  Entry &entry = findOrCreateEntry(proxy);
  if (not _flushing) flushSubtree(entry, false);
  return entry.slot;
}


void Assembler::Cache::forEntireSubtree(SlotProxy root, auto&& action) {
  // Apply an action to the entire cache-subtree of the proxy. This proxy
  // must not already live in cache, otherwise we should have called the
  // other overload of this function.
  assert(not findEntry(root));

  auto const isPartOfSemanticSubtree = [&](SlotProxy proxy) -> bool {
    if (proxy == root) return true;

    auto current = proxy.enclosingProxy();
    while (current) {
      if (*current == root) return true;
      current = (*current).enclosingProxy();
    }
    return false;
  };


  // Find entries that are part of the semantic subtree of this proxy.
  // Call the other overload of forEntireSubtree on the cache-root
  // of that tree.
  for (auto const &ptr: _entries) {
    Entry &entry = *ptr;
    if (not isPartOfSemanticSubtree(entry.proxy)) continue;
    if (entry.parent && isPartOfSemanticSubtree(entry.parent->proxy)) continue; // not the cache-root
    forEntireSubtree(entry, true, action);
  }
}

void Assembler::Cache::forEntireSubtree(Entry& root, bool const includeRoot, auto&& action) {
  
  auto descendants = [&] {
    std::vector<Entry*> result;
    auto impl = [&](auto&& self, Entry const &root) -> void {
      for (Entry *child: root.children) {
	self(self, *child);
	result.emplace_back(child);	
      }
    };

    impl(impl, root);
    return result;
  }();

  // Descendents are already sorted -> safe for flushing actions that need the child entry
  // to be flushed into its parent before its parent is flushed further.
  for (Entry *entry: descendants) action(*entry);
  if (includeRoot) action(root);
}

void Assembler::Cache::flushEntryIfDirty(Entry &entry, TransferMode mode) {
  if (not entry.dirty || entry.pendingWrite) return;
  entry.dirty = false;

  entry.proxy.write(_self, entry.slot, mode);
  if (entry.parent != nullptr) entry.parent->dirty = true;
}
      

void Assembler::Cache::flushSubtree(SlotProxy proxy, TransferMode mode) {
  assert(not _flushing);

  util::BoolGuard guard(_flushing, true);
  forEntireSubtree(proxy, [&](Entry &entry) {
    flushEntryIfDirty(entry, mode);
  });
}

void Assembler::Cache::flushSubtree(Entry &root, bool const includeRoot, TransferMode mode) {
  assert(not _flushing);  
  util::BoolGuard guard(_flushing, true);
  forEntireSubtree(root, includeRoot, [&](Entry &entry){
    flushEntryIfDirty(entry, mode);
  });
}

void Assembler::Cache::markEntryForDelete(Entry &entry) {
  entry.markedForDelete = true;
}

void Assembler::Cache::markSubtreeForDelete(SlotProxy proxy) {
  forEntireSubtree(proxy, [&](Entry &entry){
    markEntryForDelete(entry);
  });
}

void Assembler::Cache::markSubtreeForDelete(Entry &root, bool const includeRoot) {
  forEntireSubtree(root, includeRoot, [&](Entry &entry){
    markEntryForDelete(entry);
  });
}

void Assembler::Cache::flushAndDeleteSubtree(SlotProxy proxy) {
  assert(not _flushing);
  util::BoolGuard guard(_flushing, true);
  forEntireSubtree(proxy, [&](Entry &entry) {
    flushEntryIfDirty(entry, TransferMode::Move);
    markEntryForDelete(entry);
  });
  deleteMarkedEntries();
}

void Assembler::Cache::flushAndDeleteSubtree(Entry &root, bool const includeRoot) {
  assert(not _flushing);
  util::BoolGuard guard(_flushing, true);
  forEntireSubtree(root, includeRoot, [&](Entry &entry) {
    flushEntryIfDirty(entry, TransferMode::Move);
    markEntryForDelete(entry);
  });
  deleteMarkedEntries();
}

void Assembler::Cache::deleteMarkedEntries() {
  for (EntryPtr const &entry : _entries) {
    if (not entry->markedForDelete) continue;

    if (entry->parent != nullptr && not entry->parent->markedForDelete) {
      std::erase(entry->parent->children, entry.get());
      entry->parent = nullptr;
    }

    // Free the cache slot
    _self.freeCacheSlot(entry->slot);
  }

  std::erase_if(_entries, [](EntryPtr const &entry){ return entry->markedForDelete; });
}      

void Assembler::Cache::flushAndClearRoots(auto&& condition) {
  std::vector<Entry*> roots;
  for (auto const& entry : _entries) {
    if (entry->parent == nullptr && condition(entry))
      roots.push_back(entry.get());
  }
  
  for (Entry* root: roots) {
    flushAndDeleteSubtree(*root, true);
  }
}

void Assembler::Cache::flushAndClearRoots() {
  flushAndClearRoots([](EntryPtr const &entry) { return true; });
}

void Assembler::Cache::internalBoundary() {
  // At internal control-flow boundaries, we don't need to synchronize globals
  flushAndClearRoots([&](EntryPtr const &entry) {
    return entry->proxy.kind() != proxy::Kind::GlobalReference;
  });
}

void Assembler::Cache::callBoundary() {
  // At call boundaries, we flush the entire cache to make sure all aliased slots are in sync
  flushAndClearRoots();;
  assert(_entries.empty());
}


void Assembler::Cache::returnBoundary() {
  // When returning from a function, we only need to flush dirty global data. Invalidation is
  // not necessary, since all local data (except for the return value) will be invalid anyway after the return.
  // We only need to flush entries that might outlive this frame, i.e. global references and dereferenced pointers.

  // Find all cached entries (roots) that (could) refer to data outside this frame
  for (auto const &entry: _entries) {
    if (entry->parent != nullptr) continue;
    if (entry->proxy.kind() == proxy::Kind::GlobalReference ||
	entry->proxy.kind() == proxy::Kind::DereferencedPointer) {

      // Entries can be flushed using move-semantics, since the cache tree
      // will be abandoned anyway.
      flushSubtree(*entry, true, TransferMode::Move);
    }
  }
  
  // Free all cache slots (should not be necessary but good practice anyway).
  for (auto const &entry: _entries) {
    _self.freeCacheSlot(entry->slot);
  }

  reset();
}

void Assembler::Cache::reset() {
  _entries.clear();
}


void Assembler::Cache::invalidateDependencies(SlotProxy modifiedProxy) {
  // The proxy is about to be modified. This means that all other cache
  // entries that depend on this proxy should be flushed (if dirty) and then
  // invalidated (deleted) to prevent them becoming stale. 

  // The proxy may not depend on a dereferenced pointer.
  assert(not modifiedProxy.dependsOnDereferencedPointer());
  
  // The cache should not contain any delete-marks at this point
  for (auto const &entryPtr: _entries) assert(not entryPtr->markedForDelete);
  
  // Go through all the cached entries and find proxies that depend (directly)
  // on the modified proxy. These dependent entries must be flushed and marked for
  // deletion. Also every entry that depends on the entry that was just marked,
  // should be invalidated in the same way (hence the recursive lambda below).

  auto dependentEntries = [&] -> std::vector<Entry*> {
    std::vector<Entry*> result;      
    auto impl = [&](auto&& self, SlotProxy dependency) -> void {
      for (auto const &entryPtr: _entries) {
	Entry& entry = *entryPtr;
	if (entry.markedForDelete) continue;
	if (entry.proxy.dependsOn(dependency)) {
	  markSubtreeForDelete(entry, true);
	  self(self, entry.proxy);
	  result.push_back(&entry);
	}
      }
    };
    impl(impl, modifiedProxy);
    return result;
  }();


  // modifiedProxy itself is not alias-derived; alias-derived writes are handled
  // at the top of Cache::write. However, existing cached entries may depend on
  // modifiedProxy and may themselves be alias-derived, e.g. deref<p> depends on p.
  // If such an entry exists, a normal structural invalidation is not enough:
  // dirty alias entries must be committed before the dependency changes, and
  // clean alias entries must be invalidated. Conservatively clear the cache.  
  for (Entry *entry: dependentEntries) {
    if (entry->proxy.dependsOnDereferencedPointer()) {
      flushAndClearRoots();
      return;
    }
  }

  // Go through the list of dependent entries in the order they were added to the vector
  // (deepest first) and flush, then delete.
  for (Entry *entry: dependentEntries) {
    flushSubtree(*entry, true, TransferMode::Move);
  }
  
  deleteMarkedEntries();
}


void Assembler::Cache::writeAliasSensitive(SlotProxy dest, SlotProxy src, TransferMode mode) {
  flushAndClearRoots();
  util::BoolGuard guard(_aliasWriteMode, true);
  dest.write(_self, src, mode);
  flushAndClearRoots();
}

void Assembler::Cache::writeAliasSensitive(SlotProxy dest, auto&& src) {
  flushAndClearRoots();
  util::BoolGuard guard(_aliasWriteMode, true);
  dest.write(_self, src);   
  flushAndClearRoots();
}


void Assembler::Cache::writeDirect(SlotProxy dest, SlotProxy src, TransferMode mode) {
  assert(dest.direct());
  
  flushAndDeleteSubtree(dest);
  invalidateDependencies(dest);
  dest.write(_self, src, mode);

  if (Entry *cachedOwner = findCachedOwner(dest))
    cachedOwner->dirty = true;
}

void Assembler::Cache::writeDirect(SlotProxy dest, auto&& src) {
  assert(dest.direct());
  
  flushAndDeleteSubtree(dest);
  invalidateDependencies(dest);
  dest.write(_self, src);   

  if (Entry *cachedOwner = findCachedOwner(dest))
    cachedOwner->dirty = true;
}


void Assembler::Cache::writeIndirect(SlotProxy dest, auto&& assign) {
  assert(not dest.direct());

  // When writing to an indirect proxy, there is no need to materialize the old value
  // into the cache, so we skip the materialization when creating a cache entry.
  // If the entry was already cached, all of its descendants and dependencies
  // are invalidated before assigning the new value.
  
  Entry &entry = findOrCreateEntry(dest, true);
  flushAndDeleteSubtree(entry, false);
  invalidateDependencies(dest);

  assign(entry.slot);
  entry.dirty = true;
  entry.pendingWrite = false;
}


void Assembler::Cache::write(SlotProxy dest, SlotProxy src, TransferMode mode) {
  assert(not _flushing);
  
  if (dest == src) return;
  
  // If not in the flushing state, we need to check if the proxies are alias-sensitive.
  if ((dest.dependsOnDereferencedPointer() || src.dependsOnDereferencedPointer())) {
    return writeAliasSensitive(dest, src, mode);
  }

  if (dest.direct()) {
    return writeDirect(dest, src, mode);
  }

  writeIndirect(dest, [&](Slot destSlot){
    Slot const srcSlot = materialize(src);
    _self.assignSlot(destSlot, srcSlot, mode);
  });
}

void Assembler::Cache::write(SlotProxy dest, literal::Literal val) {
  assert(not _flushing);
  
  if (dest.dependsOnDereferencedPointer()) {
    return writeAliasSensitive(dest, val);
  }
  
  if (dest.direct()) {
    return writeDirect(dest, val);
  }

  writeIndirect(dest, [&](Slot destSlot){
    _self.assignSlot(destSlot, val);
  });
}


void Assembler::Cache::write(SlotProxy dest, std::function<void(Slot)> const &writeInto) {
  assert(not _flushing);
  
  if (dest.dependsOnDereferencedPointer()) {
    return writeAliasSensitive(dest, writeInto);
  }
  
  if (dest.direct()) {
    return writeDirect(dest, writeInto);
  }

  writeIndirect(dest, [&](Slot destSlot){
    writeInto(destSlot);
  });
}

bool Assembler::Cache::empty() const {
  return _entries.size() == 0;
}

Slot Assembler::materialize(SlotProxy proxy) {
  return _cache.materialize(proxy);
}
