#pragma once
#include "acus/sugar/sugar_array.h"

namespace acus::sugar::impl {

  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void FillArray<IntType, N, Unroll>::emit(Expr const &arr, Expr const &val) {
    if constexpr (Unroll) {
      for (size_t i = 0; i != N; ++i) {
	arr[i] = val;
      }
    } else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 0);
      while_(i < N) {
	arr[i++] = val;
      };
    }
  }

  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void FindArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {

    // Valid indices are 0 ... N - 1, so N represents "not found".
    result = N;

    if constexpr (Unroll) {
      auto searching = (let_<u8>(nextVarName()) = 1);

      for (size_t i = 0; i != N; ++i) {
        if_(searching) {
          if_(arr[i] == val) {
            result = i;
            searching = 0;
          };
        };
      }
    } else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 0);
      while_(i < N) {
        if_(arr[i] == val) {
          result = i;
          break_;
        };

        ++i;
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void ContainsArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {
    result = array::find<IntType, N, Unroll>(arr, val) != N;
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void EqualArray<IntType, N, Unroll>::emit(Expr &result, Expr const &lhs, Expr const &rhs) {
    result = 1;

    if constexpr (Unroll) {
      for (size_t i = 0; i != N; ++i) {
        if_(result) {
          if_(lhs[i] != rhs[i]) {
            result = 0;
          };
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < N) {
        if_(lhs[i] != rhs[i]) {
          result = 0;
          break_;
        };

        ++i;
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void ClearArray<IntType, N, Unroll>::emit(Expr const &arr) {
    array::fill<IntType, N, Unroll>(arr, IntType{0});
  }

  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void SumArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = IntType{0};

    if constexpr (Unroll) {
      for (size_t i = 0; i != N; ++i) {
        result += arr[i];
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < N) {
        result += arr[i++];
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void MinArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = arr[0];

    if constexpr (Unroll) {
      for (size_t i = 1; i != N; ++i) {
        if_(arr[i] < result) {
          result = arr[i];
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<IntType>(nextVarName());

      while_(i < N) {
        current = arr[i++];

        if_(current < result) {
          result = current;
        };
      };
    }
  }

 
  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void MaxArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = arr[0];

    if constexpr (Unroll) {
      for (size_t i = 1; i != N; ++i) {
        if_(arr[i] > result) {
          result = arr[i];
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<IntType>(nextVarName());

      while_(i < N) {
        current = arr[i++];

        if_(current > result) {
          result = current;
        };
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void MinIndexArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = 0;
    auto minimum = (let_<IntType>(nextVarName()) = arr[0]);

    if constexpr (Unroll) {
      for (size_t i = 1; i != N; ++i) {
        if_(arr[i] < minimum) {
          minimum = arr[i];
          result = i;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<IntType>(nextVarName());

      while_(i < N) {
        current = arr[i];

        if_(current < minimum) {
          minimum = current;
          result = i;
        };

        ++i;
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void MaxIndexArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = 0;
    auto maximum = (let_<IntType>(nextVarName()) = arr[0]);

    if constexpr (Unroll) {
      for (size_t i = 1; i != N; ++i) {
        if_(arr[i] > maximum) {
          maximum = arr[i];
          result = i;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<IntType>(nextVarName());

      while_(i < N) {
        current = arr[i];

        if_(current > maximum) {
          maximum = current;
          result = i;
        };

        ++i;
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void CountArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {
    result = 0;

    if constexpr (Unroll) {
      for (size_t i = 0; i != N; ++i) {
        if_(arr[i] == val) {
          ++result;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < N) {
        if_(arr[i++] == val) {
          ++result;
        };
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void SortArray<IntType, N, Unroll>::emit(Expr const &arr) {
    if constexpr (N < 2) {
      return;
    }

    auto tmp = let_<IntType>(nextVarName());

    if constexpr (Unroll) {
      /*
       * Bubble sort with compile-time indices. After every pass, the greatest
       * remaining value has moved to the end of the unsorted region.
       */
      for (size_t end = N; end > 1; --end) {
        for (size_t i = 1; i < end; ++i) {
          auto lhs = arr[i - 1];
          auto rhs = arr[i];

          if_(lhs > rhs) {
            tmp = lhs;
            lhs = rhs;
            rhs = tmp;
          };
        }
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;

      auto end = (let_<IndexType>(nextVarName()) = N);
      auto i = let_<IndexType>(nextVarName());
      auto swapped = let_<u8>(nextVarName());

      while_(end > 1) {
        i = 1;
        swapped = 0;

        while_(i < end) {
          auto lhs = arr[i - 1];
          auto rhs = arr[i];

          if_(lhs > rhs) {
            tmp = lhs;
            lhs = rhs;
            rhs = tmp;
            swapped = 1;
          };

          ++i;
        };

        if_(!swapped) {
          break_;
        };

        --end;
      };
    }
  }


  template <concepts::Integer IntType, size_t N, bool Unroll> requires (N < 0xFFFF)
  void IsSortedArray<IntType, N, Unroll>::emit(Expr &result, Expr const &arr) {
    result = 1;

    if constexpr (N < 2) {
      return;
    }

    if constexpr (Unroll) {
      for (size_t i = 1; i != N; ++i) {
        if_(result) {
          if_(arr[i - 1] > arr[i]) {
            result = 0;
          };
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<N>;
      auto i = (let_<IndexType>(nextVarName()) = 1);

      while_(i < N) {
        if_(arr[i - 1] > arr[i]) {
          result = 0;
          break_;
        };

        ++i;
      };
    }
  }

} // namespace acus::sugar::impl
