// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <algorithm>

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

  namespace constraint {

    template <typename T>
    struct Consecutive {
      template <typename ... Args> requires (std::is_same_v<T, Args> && ...)
      static auto test(Args ... args) -> std::optional<std::array<T, sizeof ... (Args)>> {
	static constexpr size_t N = sizeof ... (Args);
	std::array<T, N> arr{args ...};
	std::sort(arr.begin(), arr.end());
	for (size_t i = 1; i != N; ++i) {
	  if (arr[i] != arr[i - 1] + 1) return std::nullopt;
	}
	
	return arr;
      }
    };
    
    


    
  }
  
  namespace math {
    inline int div(int num, int denom) {
      if (denom != 0) return num / denom;
      if (num == 0) return 0;
      return static_cast<int>(static_cast<size_t>(-1));
    }

    inline int mod(int num, int denom) {
      if (denom != 0) return num % denom;
      return 0;
    }
  }
}
