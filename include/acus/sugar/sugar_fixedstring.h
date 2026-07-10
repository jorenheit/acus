// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cstdint>
#include <algorithm>

namespace acus::sugar::impl {

  template <std::size_t N>
  struct FixedString {
    char data[N];

    constexpr FixedString(char const (&str)[N]) {
      std::copy_n(str, N, data);
    }
  };

}
