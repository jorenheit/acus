// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/types/typesystem.h"
#include "acus/types/types.h"

#define API_SOURCE
#include "acus/api/api.h"
using namespace acus;

namespace acus::ts::impl {
  
  std::unique_ptr<types::VoidType> _void;
  std::unique_ptr<types::IntegerType> _u8;
  std::unique_ptr<types::IntegerType> _s8;
  std::unique_ptr<types::IntegerType> _u16;
  std::unique_ptr<types::IntegerType> _s16;
  std::unique_ptr<types::FunctionType> _voidFunction;

  std::vector<std::unique_ptr<types::RawType>> _rawTypes;    
  std::vector<std::unique_ptr<types::ArrayType>> _arrayTypes;
  std::vector<std::unique_ptr<types::StringType>> _stringTypes;
  std::vector<std::unique_ptr<types::StructType>> _structTypes;
  std::vector<std::unique_ptr<types::PointerType>> _pointerTypes;
  std::vector<std::unique_ptr<types::FunctionType>> _functionTypes;
  std::vector<std::unique_ptr<types::FunctionPointerType>> _functionPointerTypes;
}

namespace acus::ts {

  types::VoidType const *void_t(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    if (!impl::_void) impl::_void = std::make_unique<types::VoidType>();
    return impl::_void.get();
  }
  
  types::IntegerType const *u8(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    if (!impl::_u8) impl::_u8 = std::make_unique<types::IntegerType>(8);
    return impl::_u8.get();
  }

  types::IntegerType const *s8(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    if (!impl::_s8) impl::_s8 = std::make_unique<types::IntegerType>(8, types::SIGNED);
    return impl::_s8.get();
  }
  
  types::IntegerType const *u16(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    if (!impl::_u16) impl::_u16 = std::make_unique<types::IntegerType>(16);
    return impl::_u16.get();
  }

  types::IntegerType const *s16(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    if (!impl::_s16) impl::_s16 = std::make_unique<types::IntegerType>(16, types::SIGNED);
    return impl::_s16.get();
  }
  
  types::ArrayType const *array(types::TypeHandle elem, int length, API_FUNC) {
    API_FUNC_BEGIN_FREE();

    for (auto const &ptr: impl::_arrayTypes) {
      if (ptr->elementType() == elem && ptr->length() == length) return ptr.get();
    }
    impl::_arrayTypes.emplace_back(std::make_unique<types::ArrayType>(elem, length));
    return impl::_arrayTypes.back().get();
  }

  types::StringType const *string(int maxLen, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    
    for (auto const &ptr: impl::_stringTypes) {
      if (ptr->size() == maxLen + 1) {
	return ptr.get();
      }
    }
    impl::_stringTypes.emplace_back(std::make_unique<types::StringType>(u8(), maxLen));
    return impl::_stringTypes.back().get();
  }
    
  types::RawType const *raw(int n, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    
    for (auto const &ptr: impl::_rawTypes) {
      if (ptr->size() == n) return ptr.get();
    }
    impl::_rawTypes.emplace_back(std::make_unique<types::RawType>(n));
    return impl::_rawTypes.back().get();
  }


  types::StructType const *struct_t(std::string const &name, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    
    for (auto const &s: impl::_structTypes) {
      if (s->str() == name) return s.get();
    }
    return nullptr; // no struct by this name exists
  }
  
  types::PointerType const *pointer(types::TypeHandle pointee, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    
    for (auto const &ptr: impl::_pointerTypes) {
      if (ptr->pointeeType() == pointee) {
	return ptr.get();
      }
    }
    impl::_pointerTypes.emplace_back(std::make_unique<types::PointerType>(pointee));
    return impl::_pointerTypes.back().get();
  }

  types::FunctionPointerType const *function_pointer(types::TypeHandle functionType, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    API_REQUIRE_IS_FUNCTION(functionType);
    
    for (auto const &ptr: impl::_functionPointerTypes) {
      if (ptr->functionType() == functionType) {
	return ptr.get();
      }
    }

    auto type = dynamic_cast<types::FunctionType const *>(functionType);
    assert(type);
    impl::_functionPointerTypes.emplace_back(std::make_unique<types::FunctionPointerType>(type));
    return impl::_functionPointerTypes.back().get();
  }

  types::FunctionType const *void_function(API_FUNC) {
    if (!impl::_voidFunction) impl::_voidFunction = std::make_unique<types::FunctionType>(void_t());
    return impl::_voidFunction.get();
  }

  FunctionTypeBuilder function(API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return FunctionTypeBuilder { API_FWD };
  }

  StructTypeBuilder defineStruct(std::string const &name, API_FUNC) {
    API_FUNC_BEGIN_FREE();
    return StructTypeBuilder { name, API_FWD };
  }
} // namespace ts


