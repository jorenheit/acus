#pragma once


namespace acus::sugar::impl {

  struct IsDigit: LibraryFunction<IsDigit, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsDigit
  
  struct IsLower: LibraryFunction<IsLower, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsLower


  struct IsUpper: LibraryFunction<IsUpper, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsUpper


  struct IsAlpha: LibraryFunction<IsAlpha, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsAlpha


  struct IsAlphanumeric: LibraryFunction<IsAlphanumeric, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsAlphanumeric


  struct IsWhitespace: LibraryFunction<IsWhitespace, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // IsWhitespace


  struct ToLower: LibraryFunction<ToLower, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // ToLower


  struct ToUpper: LibraryFunction<ToUpper, u8(u8)> {
    static void emit(Expr &result, Expr const &val);
  }; // ToUpper

} // namespace acus::sugar::impl
