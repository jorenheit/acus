#pragma once

namespace acus::sugar::impl {

  template <concepts::Array ArrayType>
  using ArrayIndexType = std::conditional_t<(ArrayType::Size < 256), u8, u16>;
  
  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::Assignable<typename ArrayType::ElementType>
  struct FillArray<ArrayType, Unroll>: LibraryFunction<FillArray<ArrayType, Unroll>,
						       void(ArrayType, typename ArrayType::ElementType),
						       Outlinable<false>> {

    static void emit(Expr const &arr, Expr const &val);
  }; // FillArray;

  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  struct FindArray<ArrayType, Unroll>: LibraryFunction<FindArray<ArrayType, Unroll>,
						       ArrayIndexType<ArrayType> (ArrayType, typename ArrayType::ElementType)> {
    
    static void emit(Expr &result, Expr const &arr, Expr const &val);
  }; // FindArray


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  struct ContainsArray<ArrayType, Unroll>: LibraryFunction<ContainsArray<ArrayType, Unroll>,
							   u8(ArrayType, typename ArrayType::ElementType)> {
    
    static void emit(Expr &result, Expr const &arr, Expr const &val);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  struct EqualArray<ArrayType, Unroll>: LibraryFunction<EqualArray<ArrayType, Unroll>,
							u8(ArrayType, ArrayType)> {

    static void emit(Expr &result, Expr const &lhs, Expr const &rhs);
  };

  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::Integer<typename ArrayType::ElementType>
  struct SumArray<ArrayType, Unroll>: LibraryFunction<SumArray<ArrayType, Unroll>,
						      typename ArrayType::ElementType (ArrayType)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::LessComparable<typename ArrayType::ElementType>
  struct MinArray<ArrayType, Unroll>: LibraryFunction<MinArray<ArrayType, Unroll>,
						      typename ArrayType::ElementType (ArrayType)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  struct MaxArray<ArrayType, Unroll>: LibraryFunction<MaxArray<ArrayType, Unroll>,
						      typename ArrayType::ElementType (ArrayType)> {
    
    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::LessComparable<typename ArrayType::ElementType>
  struct MinIndexArray<ArrayType, Unroll>: LibraryFunction<MinIndexArray<ArrayType, Unroll>,
							   ArrayIndexType<ArrayType> (ArrayType)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  struct MaxIndexArray<ArrayType, Unroll>: LibraryFunction<MaxIndexArray<ArrayType, Unroll>,
							   ArrayIndexType<ArrayType> (ArrayType)> {

    static void emit(Expr &result, Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::EqualComparable<typename ArrayType::ElementType>
  struct CountArray<ArrayType, Unroll>: LibraryFunction<CountArray<ArrayType, Unroll>,
							ArrayIndexType<ArrayType> (ArrayType, typename ArrayType::ElementType)> {

    static void emit(Expr &result, Expr const &arr, Expr const &val);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  struct SortArray<ArrayType, Unroll>: LibraryFunction<SortArray<ArrayType, Unroll>,
						       void(ArrayType),
						       Outlinable<false>> {

    static void emit(Expr const &arr);
  };


  template <concepts::Array ArrayType, bool Unroll>
  requires concepts::GreaterComparable<typename ArrayType::ElementType>
  struct IsSortedArray<ArrayType, Unroll>: LibraryFunction<IsSortedArray<ArrayType, Unroll>,
							    u8(ArrayType)> {

    static void emit(Expr &result, Expr const &arr);
  };

} // namespace acus::sugar::impl
  

#define ACUS_ARRAY_IMPL_fill FillArray
#define ACUS_ARRAY_IMPL_find FindArray
#define ACUS_ARRAY_IMPL_contains ContainsArray
#define ACUS_ARRAY_IMPL_equal EqualArray
#define ACUS_ARRAY_IMPL_sum SumArray
#define ACUS_ARRAY_IMPL_min MinArray
#define ACUS_ARRAY_IMPL_max MaxArray
#define ACUS_ARRAY_IMPL_min_index MinIndexArray
#define ACUS_ARRAY_IMPL_max_index MaxIndexArray
#define ACUS_ARRAY_IMPL_count CountArray
#define ACUS_ARRAY_IMPL_sort SortArray
#define ACUS_ARRAY_IMPL_is_sorted IsSortedArray

#define ACUS_ARRAY_IMPL_EXPAND(name) ACUS_ARRAY_IMPL_##name
#define ARRAY_MEMBER(name, comment)\
  static impl::ACUS_ARRAY_IMPL_EXPAND(name)<This> name;

#define ARRAY_IMPLEMENTATION			\
  Array() = default;				\
  using Base = impl::ArrayBase<T, N>;		\
  using Base::Base;				\
  using This = Array<T, N>;
