#pragma once

namespace acus::sugar::impl {

  template <concepts::String Type>
  struct StrLen<Type>: LibraryFunction<StrLen<Type>,
				       std::conditional_t<(Type::Size < 256), u8, u16> (Type)> {
    static void emit(Expr &result, Expr const &str);
  }; // StrLen


  template <concepts::String DestType, concepts::String SrcType>
  struct AppendString<DestType, SrcType>: LibraryFunction<AppendString<DestType, SrcType>,
							  void(DestType, SrcType),
							  Outlinable<false>> {
    static void emit(Expr const &dest, Expr const &src);
  }; // AppendString


  template <concepts::String DestType, concepts::String SrcType>
  struct AppendStringCopy<DestType, SrcType>: LibraryFunction<AppendStringCopy<DestType, SrcType>,
							      DestType(DestType, SrcType)> {
    static void emit(Expr &result, Expr const &dest, Expr const &src);
  }; // AppendStringCopy


  // TODO: this thing should live somewhere else
  template <size_t MaxSize>
  void emitStringLength(Expr &result, Expr const &str);


  template <concepts::String LhsType, concepts::String RhsType>
  struct StringCompare<LhsType, RhsType>: LibraryFunction<StringCompare<LhsType, RhsType>,
							  s8(LhsType, RhsType)> {
    static void emit(Expr &result, Expr const &lhs, Expr const &rhs);
  }; // StringCompare


  template <concepts::String StringType, concepts::String PrefixStringType>
  struct StartsWith<StringType, PrefixStringType>: LibraryFunction<StartsWith<StringType, PrefixStringType>,
								   u8(StringType,PrefixStringType)> {
    static void emit(Expr &result, Expr const &str, Expr const &prefix);
  }; // StartsWith


  template <concepts::String StringType, concepts::String SuffixStringType>
  struct EndsWith<StringType, SuffixStringType>: LibraryFunction<EndsWith<StringType, SuffixStringType>,
								 u8(StringType, SuffixStringType)> {
    static void emit(Expr &result, Expr const &str, Expr const &suffix);
  }; // EndsWith


  template <concepts::String StringType>
  struct FindChar<StringType>: LibraryFunction<FindChar<StringType>,
					       u16(StringType, u8)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // FindChar


  template <concepts::String StringType, concepts::String NeedleStringType>
  struct FindString<StringType, NeedleStringType>: LibraryFunction<FindString<StringType, NeedleStringType>,
								   u16(StringType, NeedleStringType)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // FindString


  template <concepts::String StringType, concepts::String NeedleStringType>
  struct ContainsString<StringType, NeedleStringType>: LibraryFunction<ContainsString<StringType, NeedleStringType>,
								       u8(StringType, NeedleStringType)> {
    static void emit(Expr &result, Expr const &str, Expr const &needle);
  }; // ContainsString


  template <concepts::String StringType, concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 36)
  struct IntToString: LibraryFunction<IntToString<StringType, IntType, Base>,
				      StringType(IntType)> {
    
    static void emit(Expr &result, Expr const &val);
  }; // IntToString


  template <concepts::String StringType, concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 36)
  struct StringToInt<StringType, IntType, Base>: LibraryFunction<StringToInt<StringType, IntType, Base>,
								 IntType(StringType)> {
    
    static void emit(Expr &result, Expr const &str);
  }; // StringToInt

} // namespace acus::sugar::impl


  
#define ACUS_STRING_IMPL_length StrLen
#define ACUS_STRING_IMPL_starts_with StartsWith
#define ACUS_STRING_IMPL_ends_with EndsWith
#define ACUS_STRING_IMPL_find_char FindChar
#define ACUS_STRING_IMPL_find_str FindString
#define ACUS_STRING_IMPL_contains ContainsString
#define ACUS_STRING_IMPL_append_to_copy AppendStringCopy
#define ACUS_STRING_IMPL_to_int StringToInt
#define ACUS_STRING_IMPL_append AppendString
  
#define ACUS_STRING_IMPL_EXPAND(name) ACUS_STRING_IMPL_##name
#define STRING_MEMBER(name, signature, ...)                         \
  static impl::ACUS_STRING_IMPL_EXPAND(name)<			    \
    This __VA_OPT__(,) __VA_ARGS__                                  \
  > name
  
#define STRING_IMPLEMENTATION \
  using This = String<N>;


#define ACUS_STRING_IMPL_compare  StringCompare
#define ACUS_STRING_IMPL_from_int IntToString

#define STRING_FUNCTION(name, comment, ...)                       \
  inline impl::ACUS_STRING_IMPL_EXPAND(name)                      \
  __VA_OPT__(<__VA_ARGS__>) name  
