// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/types/literal.h"
#include "acus/sugar/sugar_fixedstring.h"

namespace acus::sugar {

  namespace impl {

    struct SugarType {};
    struct StringType: SugarType {};
    struct ArrayType: SugarType {};
    struct IntType: SugarType {};
    struct StructType: SugarType {};
    struct PointerType: SugarType{};

    namespace concepts {

      template <typename T>
      inline constexpr bool IsSugarType =
	std::is_same_v<T, void> ||
	std::is_base_of_v<impl::SugarType, std::remove_cvref_t<T>>;

      template <typename T>
      concept SugarType = IsSugarType<T>;

    } // concepts
      

    
    template <concepts::SugarType T>
    types::TypeHandle getTypeHandle() {
      if constexpr (std::is_same_v<T, void>) return ts::void_t();
      else return T::type();
    }

    template <auto typeFactory, auto literalFactory, size_t Bits_>
    class Int: public IntType {
      int x;
    public:
      static constexpr size_t Bits = Bits_;
      
      Int(int val): x(val) {};

      template <auto tf, auto lf, size_t Bits>
      Int(Int<tf, lf, Bits> const &other):
	x(other.getInt())
      {}

      static types::IntegerType const *type() { return typeFactory({}); }
      literal::Literal toLiteral() const { return literalFactory(x, {}); }

      int getInt() const { return x; }
    };
  }

  // Integer types
  struct u8:  impl::Int<ts::u8,  literal::u8,  8>  {};
  struct u16: impl::Int<ts::u16, literal::u16, 16> {};
  struct s8:  impl::Int<ts::s8,  literal::s8,  8>  {};
  struct s16: impl::Int<ts::s16, literal::s16, 16> {};


  namespace impl {

    // String
    template <size_t N>
    class StringBase: public impl::StringType {
      std::string _str;
    public:
      static constexpr size_t Size = N;
      StringBase() = default;
      StringBase(std::string const &str): _str(str) {}
      static types::StringType const *type() { return ts::string(N); }
      literal::Literal toLiteral() const { return literal::string(_str, N); }    
    };

    // Array
    template <impl::concepts::SugarType T, size_t N>
    class ArrayBase: public impl::ArrayType {

      std::array<T, N> _arr;

    public:
      using ElementType = T;
      static constexpr size_t Size = N;
    
      constexpr ArrayBase() = default;

      template <typename... Args> requires (sizeof...(Args) == N && (std::constructible_from<T, Args&&> && ...))
      constexpr ArrayBase(Args&&... args):
	_arr{ T{std::forward<Args>(args)}... }
      {}
    
      static types::TypeHandle type() {
	return ts::array(impl::getTypeHandle<T>(), N);
      }

      literal::Literal toLiteral() const {
	auto builder = literal::array(type());

	// Initialize fields from the base array
	[&]<size_t... Is>(std::index_sequence<Is...>) {
	  (builder.push(_arr[Is].toLiteral()), ...);
	}(std::make_index_sequence<N>{});

	return builder.done();
      }    
    };
    
  } // impl
  
  // Struct
  template <impl::FixedString Name, impl::concepts::SugarType T>
  struct Field {
    using type = T;
    static constexpr std::string_view name{ Name.data, sizeof(Name.data) - 1 };
  };

  template <impl::FixedString Name, typename ... Fields>
  struct Struct: impl::StructType, std::tuple<typename Fields::type ...> {
    using Base = std::tuple<typename Fields::type ...>;
    
    static constexpr std::string_view name{ Name.data, sizeof(Name.data) - 1 };
    using Base::Base;

    static types::TypeHandle type() {
      types::TypeHandle type = ts::struct_t(std::string(name));
      if (type != nullptr) return type;

      auto builder = ts::defineStruct(std::string(name));
      (builder.field(std::string(Fields::name), Fields::type::type()), ...);
      return builder.done();
    }
    
    literal::Literal toLiteral() const {
      auto builder = literal::struct_t(type());

      // Initialize fields from the base tuple
      [&]<size_t... Is>(std::index_sequence<Is...>) {
	(builder.init(std::string(std::tuple_element_t<Is, std::tuple<Fields...>>::name),
		      std::get<Is>(*this).toLiteral()), ...);
      }(std::index_sequence_for<Fields...>{});
      
      return builder.done();
    }
    
  };


  template <impl::concepts::SugarType T>
  struct Ptr: impl::PointerType {
    static types::TypeHandle type() {
      return ts::pointer(impl::getTypeHandle<T>());
    }
  };


  struct Expr;

  template <size_t N> struct String;
  template <typename T, size_t N> struct Array;


  
  //Concepts
  namespace impl::concepts {
    // Helpers
    template <typename T, typename ... U>
    inline constexpr bool IsOneOf = (std::is_same_v<std::remove_cvref_t<T>, U> || ...);

    template <typename T>
    struct IsStringType: std::false_type {};

    template <size_t N>
    struct IsStringType<sugar::String<N>>: std::true_type {};
    
    template <typename T>
    struct IsArrayType: std::false_type {};

    template <typename T, size_t N>
    struct IsArrayType<sugar::Array<T, N>>: std::true_type {};
    
    template <typename T>
    struct IsPointerType: std::false_type {};

    template <typename T>
    struct IsPointerType<Ptr<T>>: std::true_type {};

    // Concepts
    template <typename T>
    concept Integer = IsOneOf<T, u8, u16, s8, s16>;

    template <typename T>
    concept UnsignedInteger = IsOneOf<T, s8, s16>;
      
    template <typename T>
    concept SignedInteger = IsOneOf<T, s8, s16>;
    
    template <typename T>
    concept Pointer = IsPointerType<std::remove_cvref_t<T>>::value;

    template <typename T>
    concept Array = IsArrayType<std::remove_cvref_t<T>>::value;

    template <typename T>
    concept String = IsStringType<std::remove_cvref_t<T>>::value;
    
  } // concepts


  // Operators
  namespace impl::operations {
    template <typename T, typename U = T> struct Assign;    
    template <typename T, typename U = T> struct Less;
    template <typename T, typename U = T> struct Greater;
    template <typename T, typename U = T> struct Equal;
    template <typename T> struct Clear;
  }

  namespace impl::concepts {

    template <template <typename, typename> typename Op, typename Ret, typename T, typename U>
    concept SupportsBinaryOperation = requires(Expr lhs, Expr rhs) {
      { Op<std::remove_cvref_t<T>, std::remove_cvref_t<U>>::apply(lhs, rhs) } -> std::convertible_to<Ret>;
    };

    template <template <typename> typename Op, typename Ret, typename T>
    concept SupportsUnaryOperation = requires(Expr expr) {
      { Op<std::remove_cvref_t<T>>::apply(expr) } -> std::convertible_to<Ret>;
    };
    
    template <typename T, typename U = T>
    concept EqualComparable = SupportsBinaryOperation<operations::Equal, Expr, T, U>;
    
    template <typename T, typename U = T>
    concept LessComparable = SupportsBinaryOperation<operations::Less, Expr, T, U>;

    template <typename T, typename U = T>
    concept GreaterComparable = SupportsBinaryOperation<operations::Greater, Expr, T, U>;
    
    template <typename T, typename U = T>
    concept Assignable = SupportsBinaryOperation<operations::Assign, Expr, T, U>;

    template <typename T>
    concept Clearable = SupportsUnaryOperation<operations::Clear, void, T>;
        
  } // concepts
  
} // sugar
