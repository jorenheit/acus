// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {


  impl::SlotProxy::SlotProxy(BasePtr ptr):
    _ptr(std::move(ptr))
  {}
  
  impl::SlotProxy::SlotProxy(Slot slot):
    SlotProxy(proxy::direct(slot))
  {}


  types::TypeHandle impl::SlotProxy::type() const {
      return _ptr->type();
  }

  Slot impl::SlotProxy::addressOf(Assembler &a, API_CTX) const {
    return _ptr->addressOf(a, API_FWD);
  }

  bool impl::SlotProxy::dependsOnStorage(Slot slot) const {
      return _ptr->dependsOnStorage(slot);
  }

  bool impl::SlotProxy::dependsOn(impl::SlotProxy other) const {
    return _ptr->dependsOn(other);
  }
  
  bool impl::SlotProxy::dependsOnDereferencedPointer() const {
    return _ptr->dependsOnDereferencedPointer();
  }
  
  bool impl::SlotProxy::direct() const {
    return _ptr->direct();
  }
  
  std::optional<impl::SlotProxy> impl::SlotProxy::enclosingProxy() {
    return _ptr->enclosingProxy();
  }
  
  Kind impl::SlotProxy::kind() const {
    return _ptr->kind();
  }
  
  Slot impl::SlotProxy::materialize(Assembler &a) const {
    return _ptr->materialize(a);
  }
  
  void impl::SlotProxy::materialize(Assembler &a, Slot dest) const {
    return _ptr->materialize(a, dest);
  }
  
  std::string impl::SlotProxy::name() const {
    return _ptr->name();
  }
  
  std::string impl::SlotProxy::uniqueName() const {
    return _ptr->uniqueName();
  }
  
  void impl::SlotProxy::write(Assembler &a, impl::SlotProxy src, TransferMode mode) const {
    return _ptr->write(a, src, mode);
  }
  
  void impl::SlotProxy::write(Assembler &a, acus::literal::Literal src) const {
    return _ptr->write(a, src);
  }
  
  void impl::SlotProxy::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    return _ptr->write(a, writeInto);
  }
  
  
  bool impl::SlotProxy::operator==(impl::SlotProxy const &other) const {
    return (*this).uniqueName() == other.uniqueName();
  }

} // namespace proxy
