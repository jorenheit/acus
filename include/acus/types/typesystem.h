// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/types/types_fwd.h"
#include "acus/core/builder.h"

#define API_HEADER
#include "acus/api/api.h"

// Public typesystem API (implemented in typesystem.cc)
namespace acus::ts {
  using TypeHandle = types::TypeHandle;

  struct FunctionTypeBuilder;
  struct StructTypeBuilder;  
  
  types::VoidType const *void_t(API_FUNC);
  types::IntegerType const *u8(API_FUNC);
  types::IntegerType const *s8(API_FUNC);
  types::IntegerType const *u16(API_FUNC);
  types::IntegerType const *s16(API_FUNC);
  
  types::ArrayType const *array(types::TypeHandle elem, int length, API_FUNC);
  types::StringType const *string(int maxLen, API_FUNC);    
  types::RawType const *raw(int n, API_FUNC);
  types::StructType const *struct_t(std::string const &name, API_FUNC);
  types::PointerType const *pointer(types::TypeHandle pointee, API_FUNC);
  types::FunctionPointerType const *function_pointer(types::TypeHandle functionType, API_FUNC);
  types::FunctionType const *void_function(API_FUNC);

  FunctionTypeBuilder function(API_FUNC);
  StructTypeBuilder defineStruct(std::string const &name, API_FUNC);  
}

// Builder Classes (implemented in type_builders.cc)
namespace acus::ts {
  
  struct FunctionTypeBuilder: builder::BuilderBase {
    FunctionTypeBuilder&  ret(types::TypeHandle returnType)  &;    
    FunctionTypeBuilder&& ret(types::TypeHandle returnType)  &&;    
    FunctionTypeBuilder&  param(types::TypeHandle paramType) &;
    FunctionTypeBuilder&& param(types::TypeHandle paramType) &&;
    types::FunctionType const *done();
    
    FunctionTypeBuilder(api::impl::Context const &ctx);      
    
  private:
    types::TypeHandle _ret = types::null;
    std::vector<types::TypeHandle> _paramTypes;
  };  

  struct StructTypeBuilder: builder::BuilderBase {
    
    StructTypeBuilder(std::string const &structName, api::impl::Context const &ctx);
    StructTypeBuilder &  field(std::string const &name, types::TypeHandle type) &;
    StructTypeBuilder && field(std::string const &name, types::TypeHandle type) &&;
    types::StructType const *done();
    
  private:
    std::string _structName;
    using NameTypePair = std::pair<std::string, types::TypeHandle>;
    std::vector<types::NameTypePair> _fields;
  };

  
} // namespace impl
