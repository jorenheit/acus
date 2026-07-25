#pragma once

namespace acus::sugar::impl {

  template <typename T> requires impl::IsSugarType<T>
  std::string toString() {
    return T::type()->str();
  }

  template <typename T>
  std::string toString(T&& val) {
    return std::to_string(std::forward<T>(val));
  }


  
  template <typename FunctionType>
  struct GetReturnType_;

  template <typename FunctionType>
  struct MakeParamTypeTuple_;


  template <typename ReturnType, typename ... Args>
  struct GetReturnType_<ReturnType(Args...)> {
    using Type = ReturnType;
  };

  template <typename ReturnType, typename ... Args>
  struct MakeParamTypeTuple_<ReturnType(Args...)> {
    using Type = std::tuple<Args...>;
  };

  template <typename T>
  using GetReturnType = typename GetReturnType_<T>::Type;

  template <typename T>
  using MakeParamTypeTuple = typename MakeParamTypeTuple_<T>::Type;
  

  template <typename FunctionType>
  struct BuildFunctionName;

  
  template <typename ReturnType, typename ... Args>
  struct BuildFunctionName<ReturnType(Args...)>  {
    static std::string build(std::string const &base) {

      std::ostringstream oss;
      std::string_view separator;
      
      oss << base << "<" << ReturnType::type()->str() + "(";
      ((oss << std::exchange(separator, ", ") << Args::type()->str()), ...);      
      oss << ")>";

      return oss.str();
    }
  };

  template <impl::FixedString Name, typename Derived, typename FunctionType>
  struct LibraryFunction {

    using ReturnType = GetReturnType<FunctionType>;
    using ParamTypeTuple = MakeParamTypeTuple<FunctionType>;
    static constexpr size_t ArgCount = std::tuple_size_v<ParamTypeTuple>;

    // TODO: how should I pass SUGAR_LOC here?! emit should check if runtime types match the
    // compiletime types
    template <typename ... Args> 
    static auto operator()(Args&& ... args) {
      static_assert(sizeof ... (args) ==  ArgCount);
      return Derived::emit(std::forward<Args>(args) ...);
    }

    
    static auto outline(SUGAR_FUNC) {
      static constexpr size_t N = std::tuple_size_v<ParamTypeTuple>;

      auto makeVarNames = [&]<size_t ... Indices>(std::index_sequence<Indices...>) {
	return std::tuple{
	  ((void)Indices, impl::nextVarName())...
	};
      };

      return std::apply([&](auto const & ... varNames) {
	std::string const functionName = BuildFunctionName<FunctionType>::build(Name);
	return function_<FunctionType>(functionName, varNames ...) | define {
	  if constexpr (std::is_void_v<ReturnType>) {
	    Derived::emit(var_(varNames)...);
	    return_;
	  } else {
	    return__(Derived::emit(var_(varNames)...), LOC_FWD);
	  }
	};
      }, makeVarNames(std::make_index_sequence<N>{}));
    }

  }; // LibraryFunction

} // acus::sugar::impl

