// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/types/literal.h"
#include "acus/types/literal_impl.h"

#define API_SOURCE
#include "acus/api/api.h"

namespace acus::literal {

  Literal u8(int val, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return Literal { std::make_shared<impl::u8>(val, API_FWD) };
  }

  Literal s8(int val, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return Literal { std::make_shared<impl::s8>(val, API_FWD) };
  }
  
  Literal u16(int val, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return Literal { std::make_shared<impl::u16>(val, API_FWD) };
  }

  Literal s16(int val, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return Literal { std::make_shared<impl::s16>(val, API_FWD) };
  }
  
  Literal string(std::string const &str, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return Literal { std::make_shared<impl::string>(str, API_FWD) };
  }

  StructLiteralBuilder struct_t(types::TypeHandle structType, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    API_REQUIRE_IS_STRUCT(structType);
    return StructLiteralBuilder { structType, API_FWD };
  }

  ArrayLiteralBuilder array(types::TypeHandle elementType, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return ArrayLiteralBuilder { elementType, API_FWD };
  }
  
  Literal function_pointer(types::TypeHandle fType, std::string const &fName, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    API_REQUIRE_IS_FUNCTION(fType);
    return Literal { std::make_shared<impl::FunctionPointer>(fType, fName, API_FWD) };
  }

} // namespace acus::literal
