// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  impl::GlobalReference::GlobalReference(Slot slot):
    Base(slot.type()),
    _slot(slot)
  {}

  Kind impl::GlobalReference::kind() const {
    return Kind::GlobalReference;
  }     

  Slot impl::GlobalReference::addressOf(Assembler &a) const {
    return a.addressOfSlot(_slot);
  }

  bool impl::GlobalReference::directAbsolute() const {
    return false;
  }

  bool impl::GlobalReference::directRelative() const {
    return false;
  }

  std::optional<SlotProxy> impl::GlobalReference::enclosingProxy() const  {
    return {};
  }

  bool impl::GlobalReference::dependsOn(SlotProxy) const {
    return false;
  }
  
  bool impl::GlobalReference::dependsOnDereferencedPointer() const {
    return false;
  }
  
  std::string impl::GlobalReference::name() const  {
    return "global<" + _slot.name() + ">";
  }

  std::string impl::GlobalReference::uniqueName() const {
    return "global<" + _slot.uniqueName() + ">";
  }
  
  Slot impl::GlobalReference::materialize(Assembler &a) const {
    assert(false && "global reference materialization always requires a target slot");
  }
  
  void impl::GlobalReference::materialize(Assembler &a, Slot dest) const {
    a.fetchGlobal(_slot, dest);
  }
      
  void impl::GlobalReference::write(Assembler &a, SlotProxy src) const {
    Slot const srcSlot = a.materialize(src);
    a.putGlobal(_slot, srcSlot);
  }
      
  void impl::GlobalReference::write(Assembler &a, acus::literal::Literal src) const {
    a.putGlobal(_slot, src);
  }

  void impl::GlobalReference::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    Slot tmp = a.getTemp(this->type());
    writeInto(tmp);
    write(a, tmp);
    a.freeTempSlot(tmp);
  }
  
  
} // namespace proxy
