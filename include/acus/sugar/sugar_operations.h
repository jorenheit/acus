#pragma once

namespace acus::sugar::impl::operations {

  template <impl::concepts::Integer T>
  struct Clear<T> {
    static void apply(Expr expr) {
      expr = T{0};
    }
  };

  template <size_t N>
  struct Clear<sugar::String<N>> {
    static void apply(Expr expr) {
      expr[0] = 0;
    }
  };
  
  template <impl::concepts::Clearable T, size_t N> 
  struct Clear<sugar::Array<T, N>> {

    static void apply(Expr arr) {
      if constexpr (config::array::UnrollClear<sugar::Array<T, N>>) {
	for (size_t i = 0; i != N; ++i) {
	  operations::Clear<T>::apply(arr[i]);
	}
      }
      else {
	using IndexType = std::conditional_t<(N < 256), u8, u16>;
	auto i = (let_<IndexType>(nextVarName()) = 0);
	while_(i < N) {
	  operations::Clear<T>::apply(arr[i++]);
	};
      }
    }
  };

  template <impl::FixedString Name, impl::concepts::Clearable T>
  struct Clear<Field<Name, T>> {
    static void apply(Expr expr) {
      Clear<T>::apply(expr);
    }
  };
  
  template <impl::FixedString Name, impl::concepts::Clearable ... Fields>
  struct Clear<Struct<Name, Fields ...>> {
    static void apply(Expr expr) {
      (Clear<Fields>::apply(expr.field(std::string(Fields::name))), ...);
    }
  };
  
  template <typename T>
  struct Assign<T, T> {
    static Expr apply(Expr lhs, Expr rhs) {
      return lhs = rhs;
    }
  };

  template <size_t N, size_t M> requires (N > M)
  struct Assign<sugar::String<N>, sugar::String<M>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return lhs = rhs;
    }
  };
  
  template <impl::concepts::Integer T, impl::concepts::Integer U>
  struct Less<T, U> {
    static Expr apply(Expr lhs, Expr rhs) {
      return lhs < rhs;
    }
  };

  template <size_t N, size_t M>
  struct Less<sugar::String<N>, sugar::String<M>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return string::compare<String<N>, String<M>>(lhs, rhs) < 0;
    }
  };

  template <impl::concepts::Integer T, impl::concepts::Integer U>
  struct Greater<T, U> {
    static Expr apply(Expr lhs, Expr rhs) {
      return lhs > rhs;
    }
  };

  template <size_t N, size_t M>
  struct Greater<sugar::String<N>, sugar::String<M>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return string::compare<String<N>, String<M>>(lhs, rhs) > 0;
    }
  };
  
    
  template <impl::concepts::Integer T, impl::concepts::Integer U>
  struct Equal<T, U> {
    static Expr apply(Expr lhs, Expr rhs) {
      return lhs == rhs;
    }
  };

  template <size_t N, size_t M>
  struct Equal<sugar::String<N>, sugar::String<M>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return string::compare<String<N>, String<M>>(lhs, rhs) == 0;
    }
  };


  template <impl::FixedString Name, impl::concepts::EqualComparable T>
  struct Equal<Field<Name, T>, Field<Name, T>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return Equal<T, T>::apply(lhs, rhs);
    }
  };  
  
  template <impl::FixedString Name, impl::concepts::EqualComparable ... Fields>
  struct Equal<Struct<Name, Fields ...>, Struct<Name, Fields ...>> {
    static Expr apply(Expr lhs, Expr rhs) {
      return (Equal<Fields>::apply(lhs.field(std::string(Fields::name)),
				   rhs.field(std::string(Fields::name))) && ...);
    }
  };

  
}
