#pragma once

namespace acus::sugar::impl {

  template <size_t N>
  using ArrayIndexType = std::conditional_t<(N < 256), u8, u16>;
  
  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct FillArray: LibraryFunction<FillArray<IntType, N, Unroll>,
				    void(Array<IntType, N>, IntType),
				    Outlinable<false>> {

    static void emit(Expr const &arr, Expr const &val);
  }; // FillArray;

  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct FindArray:
    LibraryFunction<FindArray<IntType, N, Unroll>,
                    ArrayIndexType<N>(Array<IntType, N>, IntType)> {

    static void emit(Expr &result, Expr const &arr, Expr const &val);
  }; // FindArray


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct ContainsArray:
    LibraryFunction<ContainsArray<IntType, N, Unroll>,
                    u8(Array<IntType, N>, IntType)> {

    static void emit(Expr &result, Expr const &arr, Expr const &val);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct EqualArray:
    LibraryFunction<EqualArray<IntType, N, Unroll>,
                    u8(Array<IntType, N>, Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &lhs, Expr const &rhs);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct ClearArray:
    LibraryFunction<ClearArray<IntType, N, Unroll>,
                    void(Array<IntType, N>),
                    Outlinable<false>> {

    static void emit(Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct SumArray:
    LibraryFunction<SumArray<IntType, N, Unroll>,
                    IntType(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct MinArray:
    LibraryFunction<MinArray<IntType, N, Unroll>,
                    IntType(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct MaxArray:
    LibraryFunction<MaxArray<IntType, N, Unroll>,
                    IntType(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct MinIndexArray:
    LibraryFunction<MinIndexArray<IntType, N, Unroll>,
                    ArrayIndexType<N>(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct MaxIndexArray:
    LibraryFunction<MaxIndexArray<IntType, N, Unroll>,
                    ArrayIndexType<N>(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct CountArray:
    LibraryFunction<CountArray<IntType, N, Unroll>,
                    ArrayIndexType<N>(Array<IntType, N>, IntType)> {

    static void emit(Expr &result, Expr const &arr, Expr const &val);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct SortArray:
    LibraryFunction<SortArray<IntType, N, Unroll>,
                    void(Array<IntType, N>),
                    Outlinable<false>> {

    static void emit(Expr const &arr);
  };


  template <concepts::Integer IntType, size_t N, bool Unroll>
  requires (N < 0xFFFF)
  struct IsSortedArray:
    LibraryFunction<IsSortedArray<IntType, N, Unroll>,
                    u8(Array<IntType, N>)> {

    static void emit(Expr &result, Expr const &arr);
  };

} // namespace acus::sugar::impl
  
