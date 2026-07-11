// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace acus::defer {

  template <typename T, typename Context>
  class Type {
    std::function<T(Context const &)> _get;
  public:
    Type(T value):
      _get([value](Context const &){ return value; })
    {}

    template <typename Getter> requires std::is_invocable_r_v<T, Getter, Context const&>
    Type(Getter&& func):
      _get(std::forward<Getter>(func))
    {}

    T resolve(Context const &ctx) const { return _get(ctx); }
  };

  template <typename Context>
  using Int = Type<int, Context>;

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator+(Type<T, C> const &lhs, U const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) + static_cast<T>(rhs); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator+(U const &lhs, Type<T, C> const &rhs) { return rhs + lhs; }

  template <typename T, typename C>
  Type<T, C> operator+(Type<T, C> const &lhs, Type<T, C> const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) + rhs.resolve(ctx); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator-(Type<T, C> const &lhs, U const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) - static_cast<T>(rhs); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator-(U const &lhs, Type<T, C> const &rhs) {
    return [lhs, rhs](C const &ctx) { return static_cast<T>(lhs) - rhs.resolve(ctx); };
  }

  template <typename T, typename C>
  Type<T, C> operator-(Type<T, C> const &lhs, Type<T, C> const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) - rhs.resolve(ctx); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator*(Type<T, C> const &lhs, U const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) * static_cast<T>(rhs); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator*(U const &lhs, Type<T, C> const &rhs) { return rhs * lhs; }

  template <typename T, typename C>
  Type<T, C> operator*(Type<T, C> const &lhs, Type<T, C> const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) * rhs.resolve(ctx); };
  }

  template <typename T, typename U, typename C> requires std::convertible_to<U, T>
  Type<T, C> operator==(U const &lhs, Type<T, C> const &rhs) { return rhs == lhs; }
  
  template <typename T, typename C>
  Type<T, C> operator==(Type<T, C> const &lhs, Type<T, C> const &rhs) {
    return [lhs, rhs](C const &ctx) { return lhs.resolve(ctx) == rhs.resolve(ctx); };
  }
  
  template <typename T, typename C>
  Type<T, C> operator-(Type<T, C> const &op) {
    return [op](C const &ctx) { return -op.resolve(ctx); };
  }

  template <typename T, typename C>
  Type<T, C> operator!(Type<T, C> const &op) {
    return [op](C const &ctx) { return !op.resolve(ctx); };
  }

  
  template <typename C, typename ... DeferPack>
  auto resolve(C const &ctx, DeferPack&& ... args) {
    return std::tuple{ args.resolve(ctx)... };
  }
} // namespace acus::defer
