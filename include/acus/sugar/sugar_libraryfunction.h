#pragma once

namespace acus::sugar::impl {

  template <bool Enable>
  struct Outlinable: std::bool_constant<Enable> {};
  
  template <typename Derived, typename FunctionType, typename ProvideOutlineVersion = Outlinable<true>>
  struct LibraryFunction;

  template <typename Derived, typename ReturnType, typename ProvideOutlineVersion, typename ... SignatureArgs>
  struct LibraryFunction<Derived, ReturnType(SignatureArgs...), ProvideOutlineVersion> {

    template <typename T> using ExprArg = Expr;

    static auto operator()(ExprArg<SignatureArgs> ... args, SUGAR_FUNC) {

      (errorIfIncompatibleType<SignatureArgs>(args, LOC_FWD), ...);
      
      if constexpr (std::is_void_v<ReturnType>) {
	Derived::emit(args ...);
	return;
      } else {
	auto result = let_<ReturnType>(impl::nextVarName());
	__assembler.scope().begin();
	Derived::emit(result, args ...);
	__assembler.endScope();
	return result;
      }
      std::unreachable();
    }


    static std::string functionName() {
      static std::string const name = std::string("__sugar_") + typeid(Derived).name();
      return name;
    }

    static auto outline(SUGAR_FUNC) requires ProvideOutlineVersion::value {
      
      return std::apply([&](auto const & ... varNames) {
	return function_<ReturnType(SignatureArgs...)>(functionName(), varNames ...) | define {
	  if constexpr (std::is_void_v<ReturnType>) {
	    Derived::emit(var_(varNames)...);
	    return_;
	  } else {
	    auto result = let_<ReturnType>(impl::nextVarName());
	    Derived::emit(result, var_(varNames)...);
	    return__(result, LOC_FWD);
	  }
	};
      }, std::tuple{((void)std::type_identity<SignatureArgs>{}, impl::nextVarName()) ... });

    }

  }; // LibraryFunction

} // acus::sugar::impl

