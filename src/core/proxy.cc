// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  impl::SlotProxy::SlotProxy(Slot const &slot):
    BasePtr(proxy::direct(slot))
  {}
  
  bool impl::SlotProxy::operator==(SlotProxy const &other) const {
    return (*this)->uniqueName() == other->uniqueName();
  }

} // namespace proxy
