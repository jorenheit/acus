#pragma once

namespace acus::sugar::impl {

  template <size_t MaxSize>
  struct Hello: LibraryFunction<Hello<MaxSize>, void(string<MaxSize>)> {
    static void emit(Expr const &str, SUGAR_LOC) {
      io::print("Hello, ");
      io::print(str);
      io::println("!");
    }
  };
  
  template <size_t MaxSize>
  struct ReadLine: LibraryFunction<ReadLine<MaxSize>,
				   string<MaxSize>()> {
    static void emit(Expr &result, SUGAR_LOC) {
      auto i = (let_<u8>("i") = 0);
      while_(i < MaxSize) {
	io::read(result[i]);
	if_(result[i++] == '\n') { break_; };
      };
      result[i - 1] = 0;
    }
  }; // ReadLine


  template <typename IntType, size_t MaxSize> requires impl::IsIntegerSugarType<IntType>
  struct StrLen: LibraryFunction<StrLen<IntType, MaxSize>,
				 IntType(string<MaxSize>)> {

    static void emit(Expr &result, Expr const &str, SUGAR_LOC) {
      // TODO: exception
      assert(types::isString(str.get().type()));
      result = IntType{0};
      while_(str[result++] != 0) { };
      --result;
    }

  }; // StrLen
  
  template <typename IntType, size_t MaxSize> requires impl::IsIntegerSugarType<IntType>
  struct ParseInt: LibraryFunction<ParseInt<IntType, MaxSize>,
				   IntType(string<MaxSize>)>{

    static void emit(Expr &result, Expr const &str, SUGAR_LOC) {
      // TODO: exception
      assert(types::isString(str.get().type()));

      result = IntType{0};
      auto index  = (let_<u8>(impl::nextVarName()) = 0);
      auto isNegative = (let_<u8>(impl::nextVarName()) = 0);

      if constexpr (IsSignedInteger<IntType>) {
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

      while_(str[index] >= '0' && str[index] <= '9') {
	result *= 10;
	result += str[index] - '0';
	++index;
      };

      if constexpr (IsSignedInteger<IntType>) {
	if_(isNegative) {
	  result *= -1;
	};
      }
    }
  }; // ParseInt
  
} // acus::sugar::impl
