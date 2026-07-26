#pragma once

namespace acus::sugar::impl {
  
  template <typename Derived, typename FunctionType>
  struct LibraryFunction;

  template <typename Derived, typename ReturnType, typename ... SignatureArgs>
  struct LibraryFunction<Derived, ReturnType(SignatureArgs...)> {

    template <typename T> using ExprArg = Expr const &;
    
    static auto operator()(ExprArg<SignatureArgs> ... args, SUGAR_FUNC) {
      if constexpr (std::is_void_v<ReturnType>) {
	Derived::emit(args ..., LOC_FWD);
	return;
      } else {
	auto result = let_<ReturnType>(impl::nextVarName());
	__assembler.scope().begin();
	Derived::emit(result, args ..., LOC_FWD);
	__assembler.endScope();
	return result;
      }
      std::unreachable();
    }


    static std::string functionName() {
      static std::string const name = std::string("__sugar_") + typeid(Derived).name();
      return name;
    }

    static auto outline(SUGAR_FUNC) {
      
      return std::apply([&](auto const & ... varNames) {
	return function_<ReturnType(SignatureArgs...)>(functionName(), varNames ...) | define {
	  if constexpr (std::is_void_v<ReturnType>) {
	    Derived::emit(var_(varNames)..., LOC_FWD);
	    return_;
	  } else {
	    auto result = let_<ReturnType>(impl::nextVarName());
	    Derived::emit(result, var_(varNames)..., LOC_FWD);
	    return__(result, LOC_FWD);
	  }
	};
      }, std::tuple{((void)std::type_identity<SignatureArgs>{}, impl::nextVarName()) ... });

    }

  }; // LibraryFunction

} // acus::sugar::impl

