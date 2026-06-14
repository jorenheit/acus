// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  impl::Direct::Direct(Slot const &slot):
    Base(slot.type),
    _slot(slot)
  {} 

  Kind impl::Direct::kind() const {
    return Kind::Direct;
  }     
  
  bool impl::Direct::directAbsolute() const {
    return true;
  }

  bool impl::Direct::directRelative() const {
    return true;
  }
  
  Slot impl::Direct::materialize(Assembler &) const {
    return _slot;
  }

  void impl::Direct::materialize(Assembler &, Slot const &) const {
    assert(false && "trying to materialize a direct proxy into another slot");
  }
  
  void impl::Direct::write(Assembler &a, SlotProxy src) const {
    a.assignSlot(_slot, a.materialize(src));
  }

  void impl::Direct::write(Assembler &a, literal::Literal src) const {
    a.assignSlot(_slot, src);
  }

  void impl::Direct::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    writeInto(_slot);
  }

  Slot impl::Direct::addressOf(Assembler &a) const {
    return a.addressOfSlot(_slot);
  }

  bool impl::Direct::dependsOnDereferencedPointer() const {
    return false;
  }

  std::string impl::Direct::name() const {
    return _slot.name;
  }

  std::string impl::Direct::uniqueName() const {
    return _slot.uniqueName;
  }

  std::optional<SlotProxy> impl::Direct::enclosingProxy() const {
    return {};
  }

  bool impl::Direct::dependsOn(SlotProxy) const {
    return false;
  }

} // namespace proxy
