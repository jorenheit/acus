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


#define ACUS_ASCII_IMPL_is_digit        IsDigit
#define ACUS_ASCII_IMPL_is_alpha        IsAlpha
#define ACUS_ASCII_IMPL_is_alphanumeric IsAlphanumeric
#define ACUS_ASCII_IMPL_is_lower        IsLower
#define ACUS_ASCII_IMPL_is_upper        IsUpper
#define ACUS_ASCII_IMPL_is_whitespace   IsWhitespace
#define ACUS_ASCII_IMPL_to_lower        ToLower
#define ACUS_ASCII_IMPL_to_upper        ToUpper

#define ACUS_ASCII_IMPL_EXPAND(name) ACUS_ASCII_IMPL_##name

#define ASCII_FUNCTION(name, comment, ...)                         \
  inline impl::ACUS_ASCII_IMPL_EXPAND(name)                        \
  __VA_OPT__(<__VA_ARGS__>) name
