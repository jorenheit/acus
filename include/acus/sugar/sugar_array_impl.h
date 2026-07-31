#pragma once
#include "acus/sugar/sugar_array.h"

namespace acus::sugar::impl {

  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::Assignable<typename ArrayType::ElementType>
  void FillArray<ArrayType, Unroll>::emit(Expr const &arr, Expr const &val) {
    using ElementType = typename ArrayType::ElementType;
    if constexpr (Unroll) {
      for (size_t i = 0; i != ArrayType::Size; ++i) {
	operations::Assign<ElementType>::apply(arr[i], val);
      }
    } else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 0);
      while_(i < ArrayType::Size) {
	operations::Assign<ElementType>::apply(arr[i++], val);
      };
    }
  }

  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  void FindArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {
    using ElementType = typename ArrayType::ElementType;

    // Valid indices are 0 ... N - 1, so N represents "not found".
    result = ArrayType::Size;

    if constexpr (Unroll) {
      auto searching = (let_<u8>(nextVarName()) = 1);

      for (size_t i = 0; i != ArrayType::Size; ++i) {
        if_(searching) {
          if_(operations::Equal<ElementType>::apply(arr[i], val)) {
            result = i;
            searching = 0;
          };
        };
      }
    } else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 0);
      while_(i < ArrayType::Size) {
        if_(operations::Equal<ElementType>::apply(arr[i], val)) {
          result = i;
          break_;
        };
        ++i;
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  void ContainsArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {
    result = ArrayType::find(arr, val) != ArrayType::Size;
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  void EqualArray<ArrayType, Unroll>::emit(Expr &result, Expr const &lhs, Expr const &rhs) {

    using ElementType = typename ArrayType::ElementType;

    result = 1;

    if constexpr (Unroll) {
      for (size_t i = 0; i != ArrayType::Size; ++i) {
        if_(result) {
          if_(not operations::Equal<ElementType>::apply(lhs[i], rhs[i])) {
            result = 0;
          };
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < ArrayType::Size) {
	if_(not operations::Equal<ElementType>::apply(lhs[i], rhs[i])) {
          result = 0;
          break_;
        };

        ++i;
      };
    }
  }

  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::Integer<typename ArrayType::ElementType>
  void SumArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = ElementType{0};

    if constexpr (Unroll) {
      for (size_t i = 0; i != ArrayType::Size; ++i) {
        result += arr[i];
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < ArrayType::Size) {
        result += arr[i++];
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::LessComparable<typename ArrayType::ElementType>
  void MinArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = arr[0];

    if constexpr (Unroll) {
      for (size_t i = 1; i != ArrayType::Size; ++i) {
        if_(operations::Less<ElementType>(arr[i], result)) {
          result = arr[i];
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<ElementType>(nextVarName());

      while_(i < ArrayType::Size) {
        current = arr[i++];

        if_(operations::Less<ElementType>(current, result)) {
          result = current;
        };
      };
    }
  }

  // TODO: depend on LessComparable and reverse args
  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  void MaxArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = arr[0];

    if constexpr (Unroll) {
      for (size_t i = 1; i != ArrayType::Size; ++i) {
        if_(operations::Greater<ElementType>(arr[i], result)) {
          result = arr[i];
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<ElementType>(nextVarName());

      while_(i < ArrayType::Size) {
        current = arr[i++];

        if_(operations::Greater<ElementType>(current,result)) {
          result = current;
        };
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::LessComparable<typename ArrayType::ElementType>
  void MinIndexArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = 0;
    auto minimum = (let_<ElementType>(nextVarName()) = arr[0]);

    if constexpr (Unroll) {
      for (size_t i = 1; i != ArrayType::Size; ++i) {
        if_(operations::Less<ElementType>(arr[i], minimum)) {
          minimum = arr[i];
          result = i;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<ElementType>(nextVarName());

      while_(i < ArrayType::Size) {
	current = arr[i];
	
        if_(operations::Less<ElementType>(arr[i], minimum)) {
          minimum = arr[i];
          result = i;
        };

        ++i;
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  void MaxIndexArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = 0;
    auto maximum = (let_<ElementType>(nextVarName()) = arr[0]);

    if constexpr (Unroll) {
      for (size_t i = 1; i != ArrayType::Size; ++i) {
        if_(operations::Greater<ElementType>(arr[i], maximum)) {
          maximum = arr[i];
          result = i;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 1);
      auto current = let_<ElementType>(nextVarName());

      while_(i < ArrayType::Size) {
        current = arr[i];

        if_(operations::Greater<ElementType>(current, maximum)) {
          maximum = current;
          result = i;
        };

        ++i;
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  void CountArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr, Expr const &val) {

    using ElementType = typename ArrayType::ElementType;
    result = 0;

    if constexpr (Unroll) {
      for (size_t i = 0; i != ArrayType::Size; ++i) {
        if_(operations::Equal<ElementType>(arr[i], val)) {
          ++result;
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 0);

      while_(i < ArrayType::Size) {
        if_(operations::Equal<ElementType>(arr[i++], val)) {
          ++result;
        };
      };
    }
  }


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  void SortArray<ArrayType, Unroll>::emit(Expr const &arr) {
    if constexpr (ArrayType::Size < 2) {
      return;
    }

    using ElementType = typename ArrayType::ElementType;
    
    if constexpr (Unroll) {
      /*
       * Bubble sort with compile-time indices. After every pass, the greatest
       * remaining value has moved to the end of the unsorted region.
       */
      for (size_t end = ArrayType::Size; end > 1; --end) {
        for (size_t i = 1; i < end; ++i) {
          auto lhs = arr[i - 1];
          auto rhs = arr[i];

          if_(operations::Greater<ElementType>::apply(lhs, rhs)) {
	    algorithm::swap<ElementType>(lhs, rhs);
          };
        }
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;

      auto end = (let_<IndexType>(nextVarName()) = ArrayType::Size);
      auto i = let_<IndexType>(nextVarName());
      auto swapped = let_<u8>(nextVarName());

      while_(end > 1) {
        i = 1;
        swapped = 0;

        while_(i < end) {
          auto lhs = arr[i - 1];
          auto rhs = arr[i];

          if_(operations::Greater<ElementType>::apply(lhs, rhs)) {
	    algorithm::swap<ElementType>(lhs, rhs);
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


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  void IsSortedArray<ArrayType, Unroll>::emit(Expr &result, Expr const &arr) {

    using ElementType = typename ArrayType::ElementType;
    result = 1;

    if constexpr (ArrayType::Size < 2) {
      return;
    }

    if constexpr (Unroll) {
      for (size_t i = 1; i != ArrayType::Size; ++i) {
        if_(result) {
          if_(operations::Greater<ElementType>(arr[i - 1], arr[i])) {
            result = 0;
          };
        };
      }
    }
    else {
      using IndexType = ArrayIndexType<ArrayType>;
      auto i = (let_<IndexType>(nextVarName()) = 1);

      while_(i < ArrayType::Size) {
        if_(operations::Greater<ElementType>(arr[i - 1], arr[i])) {
          result = 0;
          break_;
        };

        ++i;
      };
    }
  }

} // namespace acus::sugar::impl
