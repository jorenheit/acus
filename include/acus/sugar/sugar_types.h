#pragma once
#include "acus/types/literal.h"
#include "acus/sugar/sugar_fixedstring.h"

namespace acus::sugar {

  namespace impl {
    struct SugarType {};
    
    template <auto typeFactory, auto literalFactory>
    class Int: public SugarType {
      int x;
    public:
      Int(int val): x(val) {};

      template <auto tf, auto lf>
      Int(Int<tf, lf> const &other):
	x(other.getInt())
      {}
      
      static types::IntegerType const *type() { return typeFactory({}); }
      literal::Literal toLiteral() const { return literalFactory(x, {}); }

      int getInt() const { return x; }
    };
  }

  // Integer types
  struct u8:  impl::Int<ts::u8,  literal::u8>  {};
  struct u16: impl::Int<ts::u16, literal::u16> {};
  struct s8:  impl::Int<ts::s8,  literal::s8>  {};
  struct s16: impl::Int<ts::s16, literal::s16> {};

  // String
  template <size_t N>
  class string: public impl::SugarType {
    std::string _str;
  public:
    string(std::string const &str): _str(str) {}
    static types::StringType const *type() { return ts::string(N); }
    literal::Literal toLiteral() const { return literal::string(_str); }
  };

  // Struct
  template <impl::FixedString Name, typename T>
  struct Field {
    static_assert(std::is_base_of_v<impl::SugarType, T>,
		  "Struct-fields can only contain types from the sugar API");
    using type = T;
    static constexpr std::string_view name{ Name.data, sizeof(Name.data) - 1 };
  };

  template <impl::FixedString Name, typename ... Fields>
  struct Struct: impl::SugarType, std::tuple<typename Fields::type ...> {
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

  template <typename T, size_t N>
  struct Array: impl::SugarType, std::array<T, N> {
    static_assert(std::is_base_of_v<impl::SugarType, T>, "Arrays can only contain types from the sugar API");
    
    using Base = std::array<T, N>;
    using Base::Base;

    constexpr Array() = default;

    template <typename... Args> requires (sizeof...(Args) == N && (std::constructible_from<T, Args&&> && ...))
    constexpr Array(Args&&... args):
      Base{ T{std::forward<Args>(args)}... }
    {}
    
    static types::TypeHandle type() {
      return ts::array(T::type(), N);
    }
    
    literal::Literal toLiteral() const {
      auto builder = literal::array(type());

      // Initialize fields from the base array
      [&]<size_t... Is>(std::index_sequence<Is...>) {
	(builder.push((*this)[Is].toLiteral()), ...);
      }(std::make_index_sequence<N>{});
      
      return builder.done();
    }
  };
  
}
