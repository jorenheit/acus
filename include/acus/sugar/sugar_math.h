#pragma once

namespace acus::sugar::impl {

  template <concepts::Integer IntType>
  struct Min: LibraryFunction<Min<IntType>,
                              IntType(IntType, IntType)> {
    static void emit(Expr &result, Expr const &x1, Expr const &x2);
  }; // Min


  template <concepts::Integer IntType>
  struct Max: LibraryFunction<Max<IntType>,
                              IntType(IntType, IntType)> {
    static void emit(Expr &result, Expr const &x1, Expr const &x2);
  }; // Max


  template <concepts::Integer IntType>
  struct Abs: LibraryFunction<Abs<IntType>,
                              IntType(IntType)> {
    static void emit(Expr &result, Expr const &x);
  }; // Abs


  template <concepts::Integer IntType>
  struct Pow: LibraryFunction<Pow<IntType>,
                              IntType(IntType, IntType)> {
    static void emit(Expr &result, Expr const &x, Expr const &p);
  }; // Pow


  template <concepts::Integer IntType>
  struct Sqrt: LibraryFunction<Sqrt<IntType>,
                               IntType(IntType)> {
    static void emit(Expr &result, Expr const &val);
  }; // Sqrt


  template <size_t Base, concepts::Integer IntType>
  requires (Base >= 2)
  struct Log: LibraryFunction<Log<Base, IntType>,
                              u8(IntType)> {
    static void emit(Expr &result, Expr const &val);
  }; // Log


  template <concepts::Integer IntType>
  struct Gcd: LibraryFunction<Gcd<IntType>,
                              IntType(IntType, IntType)> {
    static void emit(Expr &result, Expr const &a, Expr const &b);
  }; // Gcd


  template <concepts::Integer IntType>
  struct Clamp: LibraryFunction<Clamp<IntType>,
                                IntType(IntType, IntType, IntType)> {
    static void emit(Expr &result, Expr const &val, Expr const &min, Expr const &max);
  }; // Clamp

} // namespace acus::sugar::impl
