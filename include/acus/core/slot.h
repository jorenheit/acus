// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <memory>
#include <cassert>
#include "acus/types/types_fwd.h"

namespace acus {

  struct SlotData {

    enum Kind {
      Local,
      Global,
      Dummy,
      Available,
      Temp,
      Cache
    };

    std::string name;
    std::string uniqueName;
    types::TypeHandle type;
    Kind kind;
    int offset;
    void const *scope = nullptr;
    //    bool consumable = false;
    //    bool consumed = false;
  
    int size() const;
    operator int() const { return offset; }

    // void wasConsumed() {
    //   assert(consumable);
    //   consumable = false;
    //   consumed = true;
    // }

    // void allowConsumption(bool value = true) {
    //   assert(not consumed);
    //   consumable = value;
    // }

    SlotData sub(types::TypeHandle subType, int subOffset) const;
    SlotData unsignedView() const;
  };


  class Slot {
    std::shared_ptr<SlotData> _slot;
    
  public:
    using enum SlotData::Kind;
    
    Slot(SlotData const &data):
      _slot(std::make_shared<SlotData>(data))
    {}

    Slot(Slot const &other) = default;
    Slot(Slot&& other) = default;
    Slot &operator=(Slot const &other) = default;
    Slot &operator=(Slot&& other) = default;
    
    SlotData &get() { return *_slot; }
    SlotData const &get() const { return *_slot; }
    
    std::string name() const { return _slot->name; }
    std::string uniqueName() const { return _slot->uniqueName; }
    types::TypeHandle type() const { return _slot->type; }
    SlotData::Kind kind() const { return _slot->kind; }
    void const *scope() const { return _slot->scope; }
    int size() const { return _slot->size(); }
    int offset() const { return _slot->offset; }
    // bool consumable() const { return _slot->consumable; }
    // bool consumed() const { return _slot->consumed; }
    operator int() const { return offset(); }
    
    
    Slot sub(types::TypeHandle subType, int subOffset) const { return Slot{_slot->sub(subType, subOffset)}; }
    Slot unsignedView() const { return Slot{_slot->unsignedView()}; }
  };


  inline bool operator==(SlotData const &s1, SlotData const &s2) {
    return s1.offset == s2.offset && s1.size() == s2.size();
  }

  inline bool operator==(Slot const &s1, Slot const &s2) {
    return s1.get() == s2.get();
  }
  
} // namespace acus
