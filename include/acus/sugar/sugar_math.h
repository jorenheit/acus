#pragma once

namespace acus::sugar::impl {

  
  template <typename IntType> requires impl::IsSugarType<IntType>
  struct Min: LibraryFunction<"__min",
			      Min<IntType>,
			      IntType(IntType, IntType)> {

    static Expr emit(Expr const &x1, Expr const &x2) {
      // TODO: exception
      assert(types::isInteger(x1.get().type()));
      assert(types::isInteger(x2.get().type()));

      auto result = let_<IntType>(impl::nextVarName());
      if_(x1 <= x2) { result = x1; }
      else_ { result = x2; };
      return result;
    };
    
  }; // Min

  template <typename IntType> requires impl::IsSugarType<IntType>
  struct Max: LibraryFunction<"__max",
			      Max<IntType>,
			      IntType(IntType, IntType)> {

    static Expr emit(Expr const &x1, Expr const &x2) {
      // TODO: exception
      assert(types::isInteger(x1.get().type()));
      assert(types::isInteger(x2.get().type()));

      auto result = let_<IntType>(impl::nextVarName());
      if_(x1 >= x2) { result = x1; }
      else_ { result = x2; };
      return result;
    };
    
  }; // Min
  
  template <typename IntType> requires impl::IsSugarType<IntType>
  struct Sqrt: LibraryFunction<"__sqrt",
			       Sqrt<IntType>,
			       IntType(IntType)> {

    static Expr emit(Expr const &val) {
      // TODO: exception
      assert(types::isInteger(val.get().type()));
      auto value = (let_<IntType>(impl::nextVarName()) = val);
      auto root =  (let_<IntType>(impl::nextVarName()) = IntType{0});
      auto odd  =  (let_<IntType>(impl::nextVarName()) = IntType{1});

      while_(value >= odd) {
        value -= odd;
        odd   += IntType{2};
        ++root;
      };

      return root;
    }
    
  }; // Sqrt


  template <size_t Base, typename IntType>
  struct Log: LibraryFunction<"__log",
			      Log<Base, IntType>,
			      u8(IntType)>{
    static_assert(Base >= 2);

    static Expr emit(Expr const &val) {
      auto value  = (let_<IntType>(impl::nextVarName()) = val);
      auto result = (let_<u8>(impl::nextVarName()) = 0);

      while_(value >= IntType{Base}) {
	value /= IntType{Base};
	++result;
      };

      return result;
    }
  };  // Log

  
} // acus::sugar::impl
