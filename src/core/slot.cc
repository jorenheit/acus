// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/slot.h"
#include "acus/types/typesystem.h"

using namespace acus;

int SlotData::size() const { return type->size(); }

SlotData SlotData::sub(types::TypeHandle subType, int subOffset) const {
  return SlotData {
    .name = name + "<" + std::to_string(subOffset) + ">",
    .type = subType,
    .kind = Dummy, //this->kind == Temp ? Temp : Dummy,
    .offset = offset + subOffset,
    .scope = scope
  };
}

SlotData SlotData::unsignedView() const {
  assert(types::isInteger(type));
  SlotData view = *this;
  view.type = type->usesValue1() ? ts::u16() : ts::u8();
  return view;
}
