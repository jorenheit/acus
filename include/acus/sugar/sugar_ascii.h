#pragma once

namespace acus::sugar::impl {

  struct IsDigit: LibraryFunction<"__isDigit",
				  IsDigit,
				  u8(u8)> {
    static Expr emit(Expr const &val) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      return (val <= '9' && val >= '0');
    };
    
  }; // IsDigit


  struct IsAlpha: LibraryFunction<"__isAlpha",
				  IsAlpha,
				  u8(u8)> {

    static Expr emit(Expr const &val) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      return (val <= 'z' && val >= 'A');
    };
    
  }; // IsAlpha


  struct IsAlphanumeric: LibraryFunction<"__isAlphanumeric",
					 IsAlphanumeric,
					 u8(u8)> {

    static Expr emit(Expr const &val) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      return (val <= '9' && val >= '0') || (val <= 'z' && val >= 'A');
    }
    
  };
  
} // acus::sugar::impl
