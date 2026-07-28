#pragma once
#include "acus/sugar/sugar_math.h"

namespace acus::sugar::impl {

  template <concepts::Integer IntType>
  void Min<IntType>::emit(Expr &result, Expr const &x1, Expr const &x2) {
    if_(x1 <= x2) { result = x1; }
    else_ { result = x2; };
  }


  template <concepts::Integer IntType>
  void Max<IntType>::emit(Expr &result, Expr const &x1, Expr const &x2) {
    if_(x1 >= x2) { result = x1; }
    else_ { result = x2; };
  }


  template <concepts::Integer IntType>
  void Abs<IntType>::emit(Expr &result, Expr const &x) {
    result = __assembler.abs(x.get());
  }


  template <concepts::Integer IntType>
  void Pow<IntType>::emit(Expr &result, Expr const &x, Expr const &p) {
    auto const compute = [&]{
      result = IntType{1};
      for_(let_<IntType>("i") = IntType{0}, var_("i") != p, ++var_("i")) {
        result *= x;
      };
    };

    if constexpr (concepts::SignedInteger<IntType>) {
      if_(p < IntType{0}) {
        result = IntType{0};
      } else_ {
        compute();
      };
    }
    else {
      compute();
    }
  }


  template <concepts::Integer IntType>
  void Sqrt<IntType>::emit(Expr &result, Expr const &val) {
    result = IntType{0};
    auto value = (let_<IntType>(impl::nextVarName()) = val);
    auto odd = (let_<IntType>(impl::nextVarName()) = IntType{1});

    while_(value >= odd) {
      value -= odd;
      odd += IntType{2};
      ++result;
    };
  }


  template <size_t Base, concepts::Integer IntType>
  requires (Base >= 2)
  void Log<Base, IntType>::emit(Expr &result, Expr const &val) {
    result = u8{0};
    auto value = (let_<IntType>(impl::nextVarName()) = val);
    while_(value >= IntType{Base}) {
      value /= IntType{Base};
      ++result;
    };
  }


  template <concepts::Integer IntType>
  void Gcd<IntType>::emit(Expr &result, Expr const &a, Expr const &b) {
    auto aCopy = let_<IntType>(nextVarName()) = a;
    auto bCopy = let_<IntType>(nextVarName()) = b;
    auto tmp = (let_<IntType>(nextVarName()) = b);

    while_(bCopy) {
      tmp = bCopy;
      bCopy = aCopy % bCopy;
      aCopy = tmp;
    };
    result = aCopy;
  }


  template <concepts::Integer IntType>
  void Clamp<IntType>::emit(Expr &result, Expr const &val, Expr const &min, Expr const &max) {
    if_(val < min) {
      result = min;
    } else_ {
      if_(val > max) {
        result = max;
      } else_ {
        result = val;
      };
    };
  }

} // namespace acus::sugar::impl
