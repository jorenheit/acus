#pragma once
namespace acus::sugar::impl {

  struct IsDigit: LibraryFunction<IsDigit, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val >= '0' && val <= '9';
    }
  }; // IsDigit


  struct IsLower: LibraryFunction<IsLower, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val >= 'a' && val <= 'z';
    }
  }; // IsLower


  struct IsUpper: LibraryFunction<IsUpper, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val >= 'A' && val <= 'Z';
    }
  }; // IsUpper


  struct IsAlpha: LibraryFunction<IsAlpha, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = (val >= 'a' && val <= 'z') ||
               (val >= 'A' && val <= 'Z');
    }
  }; // IsAlpha


  struct IsAlphanumeric: LibraryFunction<IsAlphanumeric, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = (val >= '0' && val <= '9') ||
               (val >= 'a' && val <= 'z') ||
               (val >= 'A' && val <= 'Z');
    }
  }; // IsAlphanumeric


  struct IsWhitespace: LibraryFunction<IsWhitespace, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val == ' '  ||
               val == '\t' ||
               val == '\n' ||
               val == '\v' ||
               val == '\f' ||
               val == '\r';
    }
  }; // IsWhitespace

  struct ToLower: LibraryFunction<ToLower, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val;

      if_(val >= 'A' && val <= 'Z') {
        result += 'a' - 'A';
      };
    }
  }; // ToLower


  struct ToUpper: LibraryFunction<ToUpper, u8(u8)> {
    static void emit(Expr &result, Expr const &val) {
      result = val;

      if_(val >= 'a' && val <= 'z') {
        result -= 'a' - 'A';
      };
    }
  }; // ToUpper

} // namespace acus::sugar::impl
