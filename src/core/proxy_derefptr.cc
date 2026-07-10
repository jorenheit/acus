// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"


namespace acus::proxy {

  impl::DereferencedPointer::DereferencedPointer(SlotProxy ptr):
    Base(types::cast<types::PointerType>(ptr.type())->pointeeType()),
    _ptr(std::move(ptr))
  {}

  Kind impl::DereferencedPointer::kind() const {
    return Kind::DereferencedPointer;
  }
  
  bool impl::DereferencedPointer::dependsOnDereferencedPointer() const {
    return true;
  }
      
  bool impl::DereferencedPointer::directRelative() const {
    return false;
  }

  bool impl::DereferencedPointer::directAbsolute() const {
    return false;
  }

  std::string impl::DereferencedPointer::name() const {
    return std::string("deref<") + _ptr.name() + ">";
  }

  std::string impl::DereferencedPointer::uniqueName() const {
    return std::string("deref<") + _ptr.uniqueName() + ">";
  }
      
  std::optional<SlotProxy> impl::DereferencedPointer::enclosingProxy() const {
    return {};
  }
      
  bool impl::DereferencedPointer::dependsOn(SlotProxy other) const {
    return other == _ptr;
  }

  Slot impl::DereferencedPointer::materialize(Assembler &) const {
    assert(false && "dereferenced pointer materialization always requires a target slot");
    std::unreachable();
  }

  void impl::DereferencedPointer::materialize(Assembler &a, Slot target) const {
    Slot const ptrSlot = a.materialize(_ptr);
    a.dereferencePointerIntoSlot(ptrSlot, target);
  }

  void impl::DereferencedPointer::write(Assembler &a, SlotProxy src, TransferMode mode) const {
    Slot const ptrSlot = a.materialize(_ptr);
    Slot const srcSlot = a.materialize(src);
    a.writeSlotThroughDereferencedPointer(ptrSlot, srcSlot, mode);
  }

  void impl::DereferencedPointer::write(Assembler &a, literal::Literal src) const {
    Slot const ptrSlot = a.materialize(_ptr);
    a.writeConstThroughDereferencedPointer(ptrSlot, src);
  }

  void impl::DereferencedPointer::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    Slot tmp = a.getTemp(this->type());
    writeInto(tmp);
    write(a, tmp, TransferMode::Move);
    a.freeTempSlot(tmp);
  }
  
  Slot impl::DereferencedPointer::addressOf(Assembler &a, API_CTX) const {
    return a.materialize(_ptr);
  }

} // namespace proxy
