#pragma once

namespace acus::sugar::impl {

  
  template <concepts::Integer IntType>
  struct Min: LibraryFunction<Min<IntType>,
			      IntType(IntType, IntType)> {

    static void emit(Expr &result, Expr const &x1, Expr const &x2, SUGAR_LOC) {
      // TODO: exception
      assert(types::isInteger(x1.get().type()));
      assert(types::isInteger(x2.get().type()));

      if_(x1 <= x2) { result = x1; }
      else_ { result = x2; };
    };
    
  }; // Min

  template <concepts::Integer IntType> 
  struct Max: LibraryFunction<Max<IntType>,
			      IntType(IntType, IntType)> {

    static void emit(Expr &result, Expr const &x1, Expr const &x2, SUGAR_LOC) {
      // TODO: exception
      assert(types::isInteger(x1.get().type()));
      assert(types::isInteger(x2.get().type()));

      if_(x1 >= x2) { result = x1; }
      else_ { result = x2; };
    };
    
  }; // Min

  

  
  template <concepts::Integer IntType>
  struct Sqrt: LibraryFunction<Sqrt<IntType>,
			       IntType(IntType)> {

    static void emit(Expr &result, Expr const &val, SUGAR_LOC) {
      assert(types::isInteger(val.get().type()));

      result = IntType{0};
      auto value = (let_<IntType>(impl::nextVarName()) = val);
      auto odd  =  (let_<IntType>(impl::nextVarName()) = IntType{1});

      while_(value >= odd) {
	value -= odd;
	odd   += IntType{2};
	++result;
      };
    }
    
  }; // Sqrt


  template <size_t Base, concepts::Integer IntType> 
  struct Log: LibraryFunction<Log<Base, IntType>,
			      u8(IntType)>{
    static_assert(Base >= 2);

    static void emit(Expr &result, Expr const &val, SUGAR_LOC) {
      result = u8{0};
      auto value  = (let_<IntType>(impl::nextVarName()) = val);      
      while_(value >= IntType{Base}) {
	value /= IntType{Base};
	++result;
      };
    }
  };  // Log

  
} // acus::sugar::impl
