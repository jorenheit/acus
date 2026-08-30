// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <vector>
#include "acus/core/slot.h"

namespace acus {

// ============================================================
// Frame layout
// ============================================================

struct FrameLayout { 

  enum Offsets {
    TargetBlock,
    ReturnValueStart
  };

  enum FrameMarkerValue {
    GlobalVariableFrameID = 1,
    FirstStackFrameID = 2
  };

  int returnValueSize = 1;
  std::vector<Slot> locals;

  FrameLayout(int retSize): returnValueSize(retSize) {}
  
  inline int localAreaSize() const {
    int result = 0;
    for (auto const &slot: locals) result += slot.size();
    return result;
  }

  inline int localBase() const {
    return ReturnValueStart + returnValueSize;
  }
  
  inline int totalLogicalCells() const {
    return localBase() + localAreaSize();
  }
  
};

} // namespace acus
