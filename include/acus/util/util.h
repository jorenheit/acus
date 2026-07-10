// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace acus::util {

  template <typename ... Offsets>
  inline bool allDifferent(Offsets ... offsets) {
    static constexpr int N = sizeof...(Offsets);
    int const array[N] = { offsets... };
    for (int i = 0 ; i != N - 1; ++i) {
      for (int j = i + 1; j != N; ++j) {
	if (array[i] == array[j]) return false;
      }
    }
    return true;
  }

  
  struct BoolGuard {
    bool &_flag;
    bool const _old;
    
    BoolGuard(bool &flag, bool value):
      _flag(flag),
      _old(flag)
    {
      _flag = value;
    }

    ~BoolGuard() {
      _flag = _old;
    }
  };
}
