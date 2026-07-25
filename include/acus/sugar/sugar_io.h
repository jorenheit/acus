#pragma once

namespace acus::sugar::impl {

  template <size_t MaxSize>
  struct ReadLine: LibraryFunction<"__readLine",
				   ReadLine<MaxSize>,
				   string<MaxSize>()> {
    static Expr emit() {
      auto result = let_<string<MaxSize>>(impl::nextVarName());
      __assembler.scope().begin();
      {
	auto i = (let_<u8>("i") = 0);
	while_(i < MaxSize) {
	  io::read(result[i]);
	  if_(result[i++] == '\n') { break_; };
	};
	result[i - 1] = 0;
      }
      __assembler.endScope();
      return result;
    }
  }; // ReadLine


  template <typename IntType, size_t MaxSize> requires impl::IsSugarType<IntType>
  struct StrLen: LibraryFunction<"__strlen",
				 StrLen<IntType, MaxSize>,
				 IntType(string<MaxSize>)> {

    static Expr emit(Expr const &str) {
      // TODO: exception
      assert(types::isString(str.get().type()));
      std::string result = impl::nextVarName();
      let_<IntType>(result) = 0;
      while_(str[var_(result)++] != 0) { };
      return var_(result) - 1;
    }

  }; // StrLen
  
  template <typename IntType, size_t MaxSize> requires impl::IsSugarType<IntType>
  struct ParseInt: LibraryFunction<"__parseInt",
				   ParseInt<IntType, MaxSize>,
				   IntType(string<MaxSize>)>{

    static Expr emit(Expr const &str) {
      // TODO: exception
      assert(types::isString(str.get().type()));
      std::string const resultName = impl::nextVarName();
      std::string const indexName = impl::nextVarName();

      let_<IntType>(resultName) = IntType{0};
      let_<u8>(indexName) = 0;

      std::string negativeName;
      if constexpr (IsSignedInteger<IntType>) {
        negativeName = impl::nextVarName();
        let_<u8>(negativeName) = 0;

        if_(str[var_(indexName)] == '-') {
          var_(negativeName) = 1;
          ++var_(indexName);
        } else_ {
          if_(str[var_(indexName)] == '+') {
            ++var_(indexName);
          };
        };
      }
      else {
        if_(str[var_(indexName)] == '+') {
          ++var_(indexName);
        };
      }

      while_(str[var_(indexName)] >= '0' &&
             str[var_(indexName)] <= '9') {
        var_(resultName) *= IntType{10};
        var_(resultName) +=
          (str[var_(indexName)] - '0').template cast<IntType>();
        ++var_(indexName);
      };

      if constexpr (IsSignedInteger<IntType>) {
        if_(var_(negativeName)) {
          var_(resultName) = -var_(resultName);
        };
      }

      return var_(resultName);
    }
  }; // ParseInt
  
} // acus::sugar::impl
