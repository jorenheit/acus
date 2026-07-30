#pragma once
#include "acus/sugar/sugar_string.h"

namespace acus::sugar::impl {


  template <concepts::String StringType>
  void StrLen<StringType>::emit(Expr &result, Expr const &str) {
    using SizeType = std::conditional_t<StringType::Size < 256, u8, u16>;
    result = SizeType{0};
    while_(str[result++] != 0) { };
    --result;
  }


  template <concepts::String DestString, concepts::String SrcString>
  void AppendString<DestString, SrcString>::emit(Expr const &dest, Expr const &src) {

    static constexpr size_t DestSize = DestString::Size;
    static constexpr size_t SrcSize = SrcString::Size;    
    
    // Find end of dest string
    auto i = let_<u16>(nextVarName()) = 0;
    while_(i <= DestSize) {
      if_(dest[i] == 0) { break_; };
      ++i;
    };

    // Copy source to dest
    auto j = let_<u16>(nextVarName()) = 0;
    while_(i < DestSize && j < SrcSize) {
      if_(src[j] == 0) { break_; };
      dest[i++] = src[j++];
    };

    // Set null terminator beyond
    dest[i] = 0;
  }


  template <concepts::String DestString, concepts::String SrcString>
  void AppendStringCopy<DestString, SrcString>::emit(Expr &result, Expr const &dest,
						     Expr const &src) {
    result = dest;
    AppendString<DestString, SrcString>::emit(result, src);
  }


  template <concepts::String StringType>
  void emitStringLength(Expr &result, Expr const &str) {
    result = 0;

    while_(result <= StringType::Size) {
      if_(str[result] == 0) {
        break_;
      };

      ++result;
    };
  }


  template <concepts::String Lhs, concepts::String Rhs>
  void StringCompare<Lhs, Rhs>::emit(Expr &result, Expr const &lhs,
				     Expr const &rhs) {

    result = s8{0};
    auto i = let_<u16>(nextVarName()) = 0;

    while_(i <= Lhs::Size && i <= Rhs::Size) {
      auto lhsChar = lhs[i];
      auto rhsChar = rhs[i];

      if_(lhsChar < rhsChar) {
        result = -1;
        break_;
      };

      if_(lhsChar > rhsChar) {
        result = s8{1};
        break_;
      };

      if_(lhsChar == 0) {
        break_;
      };

      ++i;
    };
  }


  template <concepts::String StringType, concepts::String PrefixStringType>
  void StartsWith<StringType, PrefixStringType>::emit(Expr &result, Expr const &str,
						      Expr const &prefix) {
    result = 1;
    auto i = let_<u16>(nextVarName()) = 0;

    while_(i <= PrefixStringType::Size) {
      auto prefixChar = prefix[i];

      if_(prefixChar == 0) {
        break_;
      };

      if_(i > StringType::Size) {
        result = 0;
        break_;
      }
      else_ {
        if_(str[i] != prefixChar) {
          result = 0;
          break_;
        };

        ++i;
      };
    };
  }


  template <concepts::String StringType, concepts::String SuffixStringType>
  void EndsWith<StringType, SuffixStringType>::emit(Expr &result, Expr const &str,
                                              Expr const &suffix) {
    auto strLength = let_<u16>(nextVarName());
    auto suffixLength = let_<u16>(nextVarName());

    emitStringLength<StringType>(strLength, str);
    emitStringLength<SuffixStringType>(suffixLength, suffix);

    result = 0;

    if_(suffixLength <= strLength) {
      result = 1;

      auto start = let_<u16>(nextVarName()) = strLength - suffixLength;
      auto i = let_<u16>(nextVarName()) = 0;

      while_(i < suffixLength) {
        if_(str[start + i] != suffix[i]) {
          result = 0;
          break_;
        };

        ++i;
      };
    };
  }


  template <concepts::String StringType>
  void FindChar<StringType>::emit(Expr &result, Expr const &str, Expr const &needle) {
    result = u16{StringType::Size + 1};
    auto i = let_<u16>(nextVarName()) = 0;

    while_(i <= StringType::Size) {
      auto current = (let_<u8>(nextVarName()) = str[i]);

      if_(current == needle) {
        result = i;
        break_;
      };

      if_(current == 0) {
        break_;
      };

      ++i;
    };
  }


  template <concepts::String StringType, concepts::String NeedleStringType>
  void FindString<StringType, NeedleStringType>::emit(Expr &result, Expr const &str,
						      Expr const &needle) {
    auto strLength = let_<u16>(nextVarName());
    auto needleLength = let_<u16>(nextVarName());

    emitStringLength<StringType::Size>(strLength, str);
    emitStringLength<NeedleStringType::Size>(needleLength, needle);

    result = u16{StringType::Size + 1};

    if_(needleLength == 0) {
      result = 0;
    }
    else_ {
      if_(needleLength <= strLength) {
        auto i = let_<u16>(nextVarName()) = 0;

        while_(i + needleLength <= strLength) {
          auto matches = let_<u8>(nextVarName()) = 1;
          auto j = let_<u16>(nextVarName()) = 0;

          while_(j < needleLength) {
            if_(str[i + j] != needle[j]) {
              matches = 0;
              break_;
            };

            ++j;
          };

          if_(matches) {
            result = i;
            break_;
          };

          ++i;
        };
      };
    };
  }


  template <concepts::String StringType, concepts::String NeedleStringType>
  void ContainsString<StringType, NeedleStringType>::emit(Expr &result, Expr const &str,
							  Expr const &needle) {
    auto position = let_<u16>(nextVarName());
    FindString<StringType, NeedleStringType>::emit(position, str, needle);

    result = position < StringType::Size;
  }


  template <concepts::String StringType, concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 36)
  void IntToString<StringType, IntType, Base>::emit(Expr &result, Expr const &val) {
    auto bits = (let_<u16>(nextVarName()) = val);
    auto buffer = let_<String<IntType::Bits>>(nextVarName());
    auto count = (let_<u8>(nextVarName()) = 0);

    if_(bits == 0) {
      result[0] = '0';
      result[1] = 0;
    }
    else_ {
      while_(bits && count < IntType::Bits) {
        auto value = (let_<u8>(nextVarName()) =
                      sugar_cast<u8>(bits % IntType{Base}));

        auto digit = let_<u8>(nextVarName());

        if constexpr (Base <= 10) {
          digit = '0' + value;
        }
        else {
          if_(value < 10) { digit = '0' + value; }
          else_ { digit = 'a' + value - 10; };
        }

        buffer[IntType::Bits - count - 1] = digit;
        bits /= IntType{Base};
        ++count;
      };

      auto i = let_<u8>(nextVarName()) = 0;
      while_(i != count) {
        result[i] = buffer[IntType::Bits - count + i];
        ++i;
      };

      result[count] = 0;
    };
  }


  template <concepts::String StringType, concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 36)
  void StringToInt<StringType, IntType, Base>::emit(Expr &result, Expr const &str) {
    result = IntType{0};
    auto index = (let_<u8>(impl::nextVarName()) = 0);
    auto isNegative = (let_<u8>(impl::nextVarName()) = 0);

    if constexpr (concepts::SignedInteger<IntType>) {
      if_(str[0] == '-') {
        isNegative = 1;
        ++index;
      } else_ {
        if_(str[0] == '+') {
          ++index;
        };
      };
    }
    else {
      if_(str[index] == '+') {
        ++index;
      };
    }

    if constexpr (Base <= 10) {
      while_(index < StringType::Size && ascii::is_digit(str[index])) {
        result *= Base;
        result += str[index] - '0';
        ++index;
      };
    }
    else {
      while_(index < StringType::Size && str[index]) {
        if_(ascii::is_digit(str[index])) {
          result *= Base;
          result += str[index] - '0';
        } else_ {
          auto lower = (let_<u8>(nextVarName()) = ascii::to_lower(str[index]));
          if_(lower >= 'a' + Base - 10) { break_; };
          result *= Base;
          result += lower - 'a' + 10;
        };
        ++index;
      };
    }

    if constexpr (concepts::SignedInteger<IntType>) {
      if_(isNegative) { result *= -1; };
    }
  }

} // namespace acus::sugar::impl
