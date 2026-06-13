// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/slot.h"
#include "acus/types/typesystem.h"

using namespace acus;

int Slot::size() const { return type->size(); }

Slot Slot::sub(types::TypeHandle subType, int subOffset) const {
  return Slot {
    .name = name + "<" + std::to_string(subOffset) + ">",
    .type = subType,
    .kind = this->kind == Temp ? Temp : Dummy,
    .offset = offset + subOffset,
    .scope = scope
  };
}

Slot Slot::unsignedView() const {
  assert(types::isInteger(type));
  Slot view = *this;
  view.type = type->usesValue1() ? ts::u16() : ts::u8();
  return view;
}
