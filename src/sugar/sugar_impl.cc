// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/sugar/sugar_impl.h"

namespace acus::sugar::impl {
  std::stack<std::string> ControlStack::_continueStack;
  std::stack<std::string> ControlStack::_breakStack;

  std::string nextLabel() {
    static size_t count = 0;
    return "__sugar_label_" + std::to_string(count++);
  }

  std::string nextVarName() {
    static size_t count = 0;
    return "__sugar_var_" + std::to_string(count++);
  }
  
}
