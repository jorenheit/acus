#pragma once

namespace acus::sugar::impl {

  template <size_t MaxSize>
  struct ClearString: LibraryFunction<ClearString<MaxSize>,
                                      void(String<MaxSize>),
                                      Outlinable<false>> {
    static void emit(Expr const &str);
  }; // ClearString


  template <concepts::Integer IntType, size_t MaxSize>
  struct StrLen: LibraryFunction<StrLen<IntType, MaxSize>,
                                 IntType(String<MaxSize>)> {
    static void emit(Expr &result, Expr const &str);
  }; // StrLen


  template <size_t DestSize, size_t SrcSize>
  struct AppendString: LibraryFunction<AppendString<DestSize, SrcSize>,
                                       void(String<DestSize>, String<SrcSize>),
                                       Outlinable<false>> {
    static void emit(Expr const &dest, Expr const &src);
  }; // AppendString


  template <size_t DestSize, size_t SrcSize>
  struct AppendStringCopy: LibraryFunction<AppendStringCopy<DestSize, SrcSize>,
                                           String<DestSize>(String<DestSize>, String<SrcSize>)> {
    static void emit(Expr &result, Expr const &dest, Expr const &src);
  }; // AppendStringCopy


  template <size_t MaxSize>
  void emitStringLength(Expr &result, Expr const &str);


  template <size_t LhsSize, size_t RhsSize>
  struct StringCompare: LibraryFunction<StringCompare<LhsSize, RhsSize>,
                                        s8(String<LhsSize>, String<RhsSize>)> {
    static void emit(Expr &result, Expr const &lhs, Expr const &rhs);
  }; // StringCompare


  template <size_t StringSize, size_t PrefixSize>
  struct StartsWith: LibraryFunction<StartsWith<StringSize, PrefixSize>,
                                     u8(String<StringSize>, String<PrefixSize>)> {
    static void emit(Expr &result, Expr const &str, Expr const &prefix);
  }; // StartsWith


  template <size_t StringSize, size_t SuffixSize>
  struct EndsWith: LibraryFunction<EndsWith<StringSize, SuffixSize>,
                                   u8(String<StringSize>, String<SuffixSize>)> {
    static void emit(Expr &result, Expr const &str, Expr const &suffix);
  }; // EndsWith


  template <size_t StringSize>
  struct FindChar: LibraryFunction<FindChar<StringSize>,
                                   u16(String<StringSize>, u8)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // FindChar


  template <size_t StringSize, size_t NeedleSize>
  struct FindString: LibraryFunction<FindString<StringSize, NeedleSize>,
                                     u16(String<StringSize>, String<NeedleSize>)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // FindString


  template <size_t StringSize, size_t NeedleSize>
  struct ContainsString: LibraryFunction<ContainsString<StringSize, NeedleSize>,
                                         u8(String<StringSize>, String<NeedleSize>)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // ContainsString


  template <concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 36)
  struct IntToString: LibraryFunction<IntToString<IntType, Base>,
                                      String<IntType::Bits>(IntType)> {
    static void emit(Expr &result, Expr const &val);
  }; // IntToString


  template <concepts::Integer IntType, size_t Base, size_t MaxSize>
  requires (Base >= 2 && Base <= 36)
  struct StringToInt: LibraryFunction<StringToInt<IntType, Base, MaxSize>,
                                      IntType(String<MaxSize>)> {
    static void emit(Expr &result, Expr const &str);
  }; // StringToInt

} // namespace acus::sugar::impl
