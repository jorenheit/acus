#pragma once

namespace acus::sugar::impl {

  struct IsDigit: LibraryFunction<IsDigit,
				  u8(u8)> {
    static void emit(Expr &result, Expr const &val, SUGAR_LOC) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      result = (val <= '9' && val >= '0');
    };
    
  }; // IsDigit


  struct IsAlpha: LibraryFunction<IsAlpha,
				  u8(u8)> {

    static void emit(Expr &result, Expr const &val, SUGAR_LOC) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      result = (val <= 'z' && val >= 'A');
    };
    
  }; // IsAlpha


  struct IsAlphanumeric: LibraryFunction<IsAlphanumeric,
					 u8(u8)> {

    static void emit(Expr &result, Expr const &val, SUGAR_LOC) {
      // TODO: exception
      assert(types::isU8(val.get().type()));
      result = (val <= '9' && val >= '0') || (val <= 'z' && val >= 'A');
    }
    
  }; // IsAlphanumeric
  
} // acus::sugar::impl
