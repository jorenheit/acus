#pragma once

namespace acus::sugar::impl {
  
  template <concepts::Clearable Type>
  struct Clear: LibraryFunction<Clear<Type>,
				void(Type),
				Outlinable<false>> {
    
    static void emit(Expr &result);
  }; // Clear

  template <concepts::Assignable Type>
  struct Swap: LibraryFunction<Swap<Type>,
			       void(Type, Type),
			       Outlinable<false>> {
    
    static void emit(Expr x1, Expr x2);
  }; // Clear
  
  

} // impl


#define ACUS_ALGORITHM_IMPL_clear Clear
#define ACUS_ALGORITHM_IMPL_swap  Swap

#define ACUS_ALGORITHM_IMPL_EXPAND(name) ACUS_ALGORITHM_IMPL_##name

#define ALGORITHM_FUNCTION(name, comment, ...)                     \
  inline impl::ACUS_ALGORITHM_IMPL_EXPAND(name)                    \
  __VA_OPT__(<__VA_ARGS__>) name
