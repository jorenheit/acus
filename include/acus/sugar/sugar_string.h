#pragma once

namespace acus::sugar::impl {

  template <size_t MaxSize>
  struct ClearString: LibraryFunction<ClearString<MaxSize>,
				      void(String<MaxSize>),
				      Outlinable<false>> {
    
    static void emit(Expr const &str) {
      str[0] = 0;
    }
    
  };


  template <concepts::Integer IntType, size_t MaxSize>
  struct StrLen: LibraryFunction<StrLen<IntType, MaxSize>,
				 IntType(String<MaxSize>)> {

    static void emit(Expr &result, Expr const &str) {
      result = IntType{0};
      while_(str[result++] != 0) { };
      --result;
    }

  }; // StrLen
  
  template <size_t DestSize, size_t SrcSize> 
  struct AppendString: LibraryFunction<AppendString<DestSize, SrcSize>,
				       void (String<DestSize>, String<SrcSize>),
				       Outlinable<false>> {
    
    static void emit(Expr const &dest, Expr const &src) {
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
  };


  template <size_t DestSize, size_t SrcSize> 
  struct AppendStringCopy: LibraryFunction<AppendStringCopy<DestSize, SrcSize>,
					   String<DestSize> (String<DestSize>, String<SrcSize>)> {
    
    static void emit(Expr &result, Expr const &dest, Expr const &src) {
      result = dest;
      AppendString<DestSize, SrcSize>::emit(result, src);
    }
  };


  template <size_t MaxSize>
  static void emitStringLength(Expr &result, Expr const &str) {
    result = 0;

    while_(result <= MaxSize) {
      if_(str[result] == 0) {
        break_;
      };

      ++result;
    };
  }


  template <size_t LhsSize, size_t RhsSize>
  struct StringCompare:
    LibraryFunction<StringCompare<LhsSize, RhsSize>,
                    s8(String<LhsSize>, String<RhsSize>)> {

    static void emit(Expr &result, Expr const &lhs, Expr const &rhs) {
      result = s8{0};
      auto i = let_<u16>(nextVarName()) = 0;

      while_(i <= LhsSize && i <= RhsSize) {
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

  }; // StringCompare


  template <size_t StringSize, size_t PrefixSize>
  struct StartsWith:
    LibraryFunction<StartsWith<StringSize, PrefixSize>,
                    u8(String<StringSize>, String<PrefixSize>)> {

    static void emit(Expr &result, Expr const &str, Expr const &prefix) {
      result = 1;
      auto i = let_<u16>(nextVarName()) = 0;

      while_(i <= PrefixSize) {
        auto prefixChar = prefix[i];

        if_(prefixChar == 0) {
          break_;
        };

        if_(i > StringSize) {
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

  }; // StartsWith


  template <size_t StringSize, size_t SuffixSize>
  struct EndsWith:
    LibraryFunction<EndsWith<StringSize, SuffixSize>,
                    u8(String<StringSize>, String<SuffixSize>)> {

    static void emit(Expr &result, Expr const &str, Expr const &suffix) {
      auto strLength = let_<u16>(nextVarName());
      auto suffixLength = let_<u16>(nextVarName());

      emitStringLength<StringSize>(strLength, str);
      emitStringLength<SuffixSize>(suffixLength, suffix);

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

  }; // EndsWith


  template <size_t StringSize>
  struct FindChar:
    LibraryFunction<FindChar<StringSize>,
                    u16(String<StringSize>, u8)> {

    static void emit(Expr &result, Expr const &str, Expr const &needle) {
      result = u16{StringSize + 1};
      auto i = let_<u16>(nextVarName()) = 0;

      while_(i <= StringSize) {
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

  }; // FindChar


  template <size_t StringSize, size_t NeedleSize>
  struct FindString:
    LibraryFunction<FindString<StringSize, NeedleSize>,
                    u16(String<StringSize>, String<NeedleSize>)> {

    static void emit(Expr &result, Expr const &str, Expr const &needle) {
      auto strLength = let_<u16>(nextVarName());
      auto needleLength = let_<u16>(nextVarName());

      emitStringLength<StringSize>(strLength, str);
      emitStringLength<NeedleSize>(needleLength, needle);

      result = u16{StringSize + 1};

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

  }; // FindString


  template <size_t StringSize, size_t NeedleSize>
  struct ContainsString: LibraryFunction<ContainsString<StringSize, NeedleSize>,
					 u8(String<StringSize>, String<NeedleSize>)> {

    static void emit(Expr &result, Expr const &str, Expr const &needle) {
      auto position = let_<u16>(nextVarName());
      FindString<StringSize, NeedleSize>::emit(position, str, needle);

      result = (position < StringSize);
    }

  }; // ContainsString

  template <concepts::Integer IntType, size_t Base>
  requires (Base >= 2 && Base <= 10 + 26)
  struct IntToString: LibraryFunction<IntToString<IntType, Base>,
				      String<IntType::Bits> (IntType)> {

    static void emit(Expr &result, Expr const &val) {
      static_assert(Base >= 2 && Base <= 36);

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
	  } else {
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
    
  }; // IntToString


  template <concepts::Integer IntType, size_t Base, size_t MaxSize>
  requires (Base >= 2 && Base <= 10 + 26)
  struct StringToInt: LibraryFunction<StringToInt<IntType, Base, MaxSize>,
				      IntType(String<MaxSize>)>{

    static void emit(Expr &result, Expr const &str) {
      result = IntType{0};
      auto index  = (let_<u8>(impl::nextVarName()) = 0);
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
      } else {
	if_(str[index] == '+') {
	  ++index;
	};
      }

      if constexpr (Base <= 10) {
	while_(index < MaxSize && IsDigit{}(str[index])) {
	  result *= Base;
	  result += str[index] - '0';	  
	  ++index;
	};
      } else {
	while_(index < MaxSize && str[index]) {
	  if_(IsDigit{}(str[index])) {
	    result *= Base;
	    result += str[index] - '0';
	  } else_ {
	    auto lower = (let_<u8>(nextVarName()) = ToLower{}(str[index]));
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
  }; // StringToInt
  
} // impl
