// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "acus/types/types_fwd.h"

namespace acus {

struct Slot {

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
  
	 
  int size() const;
  operator int() const { return offset; }

  Slot sub(types::TypeHandle subType, int subOffset) const;
  Slot unsignedView() const;
};

  inline bool operator==(Slot const &s1, Slot const &s2) {
    return s1.offset == s2.offset && s1.size() == s2.size();
  }

  
} // namespace acus
