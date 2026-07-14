// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/types/typesystem.h"
#include "acus/types/literal_impl.h"
#include "acus/core/builder.h"

#define API_HEADER
#include "acus/api/api.h"

// Public literal API (implemented in literal.cc)
namespace acus::literal {  
  class StructLiteralBuilder;
  class ArrayLiteralBuilder;

  using Literal = impl::Literal; 
  
  Literal u8(int val, API_FUNC);
  Literal s8(int val, API_FUNC);
  Literal u16(int val, API_FUNC);
  Literal s16(int val, API_FUNC);
  Literal string(std::string const &str, API_FUNC);
  Literal function_pointer(types::TypeHandle fType, std::string const &fName, API_FUNC);

  StructLiteralBuilder struct_t(types::TypeHandle structType, API_FUNC);
  ArrayLiteralBuilder array(types::TypeHandle arrayType, API_FUNC);

  template <typename T> requires std::derived_from<T, types::Type>
  auto cast(Literal const &v);
} // namespace acus::literal

#include "literal_cast.tpp"

// Builder classes for arrays and structs (implemented in literal_builders.cc)
namespace acus::literal {
  
  struct StructLiteralBuilder: builder::BuilderBase {
    StructLiteralBuilder &  init(std::string const &name, Literal val) &;
    StructLiteralBuilder && init(std::string const &name, Literal val) &&;
    Literal done();
    StructLiteralBuilder(types::TypeHandle structType, api::impl::Context const &ctx);
  private:
    types::TypeHandle _structType;
    std::unordered_map<std::string, Literal> _fields;
  };

  struct ArrayLiteralBuilder: builder::BuilderBase {
    ArrayLiteralBuilder &  push(Literal value) &;
    ArrayLiteralBuilder && push(Literal value) &&;
    Literal done();
    ArrayLiteralBuilder(types::TypeHandle arrayType, api::impl::Context const &ctx);
  private:
    types::ArrayType const *_arrayType;
    types::TypeHandle _elementType;
    size_t _length;
    std::vector<Literal> _elements;
  };

} // namespace acus::literal

