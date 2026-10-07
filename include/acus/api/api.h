// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/api/error.h"
#include "acus/types/type_rules.h"
#include "acus/types/types.h"
#include "acus/api/context.h"

#ifndef API_LOC
#define API_LOC api_loc
#endif

#if !defined(API_HEADER) && !defined(API_SOURCE)
#define API_HEADER
#endif

#ifdef API_FUNC
#undef API_FUNC
#endif

#ifdef API_HEADER
#define API_FUNC API_FUNC_HEADER
#undef API_HEADER
#elif defined(API_SOURCE)
#define API_FUNC API_FUNC_SOURCE
#undef API_SOURCE
#else
#error __FILE__ ": define API_HEADER or API_SOURCE before including this file"
#endif

#ifndef API_H_INCLUDED
#define API_H_INCLUDED

namespace acus::api::impl {
  struct ExpResult {
    bool expected;
    std::string msg;
  };
  
  void expectNext(std::string const &name);
  ExpResult isExpected(std::string const &name, bool strict = false);
  std::string expected();
  void clearExpected();
}

#define API_FUNC_HEADER std::source_location API_LOC = std::source_location::current()
#define API_FUNC_SOURCE std::source_location API_LOC
#define API_CTX_NAME api_ctx
#define API_CTX api::impl::Context const & API_CTX_NAME
#define API_CTX_IGNORE api::impl::Context const &
#define API_FWD API_CTX_NAME

#define API_FUNC_BEGIN()					\
  api::impl::Context API_CTX_NAME{*this, __func__, API_LOC};

#define API_FUNC_BEGIN_FREE()				\
  api::impl::Context API_CTX_NAME{__func__, API_LOC};

#define API_EXPECT_NEXT(name) api::impl::expectNext((name));			

#define API_CHECK_EXPECTED_IMPL(strict) do {				\
    std::string const apiName = (API_CTX_NAME).apiName();		\
    auto expResult = api::impl::isExpected(apiName, strict);		\
    error::throw_if(not expResult.expected,				\
		    error::ErrorCode::UnexpectedApiCall,		\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    expResult.msg);					\
    if (api::impl::expected() == apiName) api::impl::clearExpected();	\
  } while (false);

#define API_CHECK_EXPECTED() API_CHECK_EXPECTED_IMPL(false);
#define API_CHECK_EXPECTED_STRICT() API_CHECK_EXPECTED_IMPL(true);
#define API_CLEAR_EXPECTED() api::impl::clearExpected();

// TODO: type-checks should accept types, not expression objects

#define API_REQUIRE(condition, errorCode, ...)				\
  error::throw_if(not (condition),					\
		  (errorCode),						\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  __VA_ARGS__);

#define API_REQUIRE_OUTSIDE_PROGRAM_BLOCK()				\
  error::throw_if((API_CTX_NAME).programStarted(),			\
		  error::ErrorCode::ExpectedOutsideProgramBlock,	\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' while inside a program-block.")

#define API_REQUIRE_INSIDE_PROGRAM_BLOCK()				\
  error::throw_if(not (API_CTX_NAME).programStarted(),			\
		  error::ErrorCode::ExpectedInsideProgramBlock,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' outside a program-block.")

#define API_REQUIRE_OUTSIDE_FUNCTION_BLOCK()				\
  error::throw_if(not (API_CTX_NAME).currentFunction().empty(),		\
		  error::ErrorCode::ExpectedOutsideFunctionBlock,	\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' while inside function-block '", \
		  (API_CTX_NAME).currentFunction(), "'; this operation must be used outside a function-block.")

#define API_REQUIRE_INSIDE_FUNCTION_BLOCK()				\
  error::throw_if((API_CTX_NAME).currentFunction().empty(),		\
		  error::ErrorCode::ExpectedInsideFunctionBlock,	\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' outside a function-block.")

#define API_REQUIRE_BLOCK_DISPATCH_MODE() \
  error::throw_if(not (API_CTX_NAME).blockDispatchMode(),		\
		  error::ErrorCode::NotAvailableInStraightLineMode,	\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "function '", (API_CTX_NAME).apiName(), "' is not available in straight line mode.")

#define API_REQUIRE_NO_SCOPE()						\
  error::throw_if((API_CTX_NAME).currentScopeDepth() > 0,		\
		  error::ErrorCode::ExpectedNoScope,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' while inside a scope-block at depth ", \
		  (API_CTX_NAME).currentScopeDepth(), "; call 'endScope' first until the scope depth returns to 0.")

#define API_REQUIRE_DECLARE_GLOBAL_ALLOWED()				\
  error::throw_if(not (API_CTX_NAME).globalDeclarationsAllowed(),	\
		  error::ErrorCode::GlobalDeclarationNotAllowed,	\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "called '", (API_CTX_NAME).apiName(), "' in a context where global declarations are not allowed.")

#define API_REQUIRE_GLOBAL_NAME_AVAILABLE(name)				\
  error::throw_if((API_CTX_NAME).declaredAsGlobal(name),		\
		  error::ErrorCode::GlobalNameUnavailable,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "cannot declare global '", name, "' because that name is already in use.")

#define API_REQUIRE_IS_GLOBAL(name)					\
  error::throw_if(not (API_CTX_NAME).declaredAsGlobal(name),		\
		  error::ErrorCode::ExpectedGlobal,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "'", name, "' is not a declared global.")

#define API_REQUIRE_IN_SCOPE(name)					\
  error::throw_if(not (API_CTX_NAME).inScope(name),			\
		  error::ErrorCode::NameNotInScope,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "'", name, "' is not declared in this scope.")

#define API_REQUIRE_NOT_IN_SCOPE(name)					\
  error::throw_if((API_CTX_NAME).inScope(name),				\
		  error::ErrorCode::NameAlreadyInScope,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "'", name, "' is already declared in this scope.")

#define API_REQUIRE_NOT_IN_CURRENT_SCOPE(name)				\
  error::throw_if((API_CTX_NAME).inCurrentScope(name),			\
		  error::ErrorCode::NameAlreadyInCurrentScope,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "'", name, "' is already declared in this scope.")

#define API_REQUIRE_IS_INTEGER(type)					\
  error::throw_if(not types::isInteger(type),				\
		  error::ErrorCode::ExpectedInteger,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected an integer but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_SIGNED_INTEGER(type)				\
  error::throw_if(not types::isSignedInteger(type),			\
		  error::ErrorCode::ExpectedSignedInteger,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected an signed integer but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_UNSIGNED_INTEGER(type)				\
  error::throw_if(types::isSignedInteger(type),				\
		  error::ErrorCode::ExpectedUnsignedInteger,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected an unsigned integer but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_ARRAY(type)					\
  error::throw_if(not types::isArray(type),				\
		  error::ErrorCode::ExpectedArray,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected an array but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_STRING(type)					\
  error::throw_if(not types::isString(type),				\
		  error::ErrorCode::ExpectedString,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected a string but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_ARRAY_OR_STRING(type)				\
  error::throw_if(not (types::isArray(type) || types::isString(type)),	\
		  error::ErrorCode::ExpectedArrayOrString,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected an array or string but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_STRUCT(type)					\
  error::throw_if(not types::isStruct(type),				\
		  error::ErrorCode::ExpectedStruct,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected a struct but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_POINTER(type)					\
  error::throw_if(not types::isPointer(type),				\
		  error::ErrorCode::ExpectedPointer,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected a pointer but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_FUNCTION(type)					\
  error::throw_if(not types::isFunction(type),				\
		  error::ErrorCode::ExpectedFunctionType,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected a function-type but got '", (type)->str(), "'.")

#define API_REQUIRE_IS_FUNCTION_POINTER(type)				\
  error::throw_if(not types::isFunctionPointer(type),			\
		  error::ErrorCode::ExpectedFunctionPointer,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "expected a function-pointer but got '", (type)->str(), "'.")

#define API_REQUIRE_FIELD_INDEX_IN_BOUNDS(obj, fieldIndex)		\
  error::throw_if((fieldIndex) < 0 ||					\
		  (fieldIndex) >= types::cast<types::StructType>((obj).type())->fieldCount(), \
		  error::ErrorCode::FieldIndexOutOfBounds,		\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "field index ", fieldIndex, " is out of bounds for struct '", \
		  (obj).type()->str(), "' with ",			\
		  types::cast<types::StructType>((obj).type())->fieldCount(), " fields.")

#define API_REQUIRE_IS_FIELD(obj, fieldName)				\
  error::throw_if(-1 == types::cast<types::StructType>((obj).type())->fieldIndex(fieldName), \
		  error::ErrorCode::FieldNotFound,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "struct '", (obj).type()->str(), "' has no field named '", \
		  fieldName, "'.")

#define API_REQUIRE_INDEX_IN_BOUNDS(arr, index)				\
  error::throw_if((index) < 0 ||					\
		  (index) >= types::cast<types::ArrayLike>((arr).type())->length(), \
		  error::ErrorCode::IndexOutOfBounds,			\
		  (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		  "index ", index, " is out of bounds for '",		\
		  (arr).str(), "' of type '", (arr).type()->str(),	\
		  "' with length ",					\
		  types::cast<types::ArrayLike>((arr).type())->length(), ".")

#define API_REQUIRE_ASSIGNABLE(dest, src) do {				\
    auto result = types::rules::assignResult((dest), (src));		\
    error::throw_if(not result,						\
		    error::ErrorCode::AssignmentTypeMismatch,		\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    result.errorMsg);					\
  } while (false);


#define API_EXPECT_TYPE(t, expectedT) do {				\
    error::throw_if(t != expectedT,					\
		    error::ErrorCode::UnexpectedType,			\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    "expected type '" + (expectedT)->str() + "', got '" + (t)->str() + "'"); \
  } while (false);

#define API_REQUIRE_PARAM_COUNT_MATCHES_FUNCTION(funcType, params) do { \
    error::throw_if(funcType->paramTypes().size() != (params).size(),	\
		    error::ErrorCode::FunctionParameterCountMismatch,	\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    "number or parameters does not match function-type; expected ", \
		    funcType->paramTypes().size(), ", got ", (params).size(), "."); \
  } while (false);

    
#define API_REQUIRE_FUNCTION_DEFINED(functionName) do {			\
    error::throw_if(not _program.isFunctionDefined(functionName),	\
		    error::ErrorCode::FunctionDoesNotExist,		\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    "function '", functionName, "' was not defined.");	\
  } while (false);

#define API_REQUIRE_LABEL_DEFINED(functionName, labelName) do {		\
    error::throw_if(not _program.function(functionName).isBlockDefined(labelName), \
		    error::ErrorCode::LabelDoesNotExist,		\
		    (API_CTX_NAME).file_name(), (API_CTX_NAME).line(), (API_CTX_NAME).column(), \
		    "label '", labelName, "' not defined inside function '", functionName, "'."); \
  } while (false);
  

#endif // API_H
