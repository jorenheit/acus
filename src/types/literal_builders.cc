// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/types/literal.h"

namespace acus::literal {
  
  StructLiteralBuilder & StructLiteralBuilder::init(std::string const &name, Literal val) & {
    BuilderBase::check();
    API_REQUIRE(not _fields.contains(name),
		error::ErrorCode::MultipleInitializationsOfSameField,
		"field '", name, "' was initialized multiple times.");
    _fields[name] = val;
    return *this;
  }

  StructLiteralBuilder && StructLiteralBuilder::init(std::string const &name, Literal val) && {
    BuilderBase::check();
    return std::move(this->init(name, val));
  }
  
  Literal StructLiteralBuilder::done() {
    BuilderBase::check();
    _finalized.done();
    return std::make_shared<impl::structT>(_structType, _fields, API_FWD);
  }

  StructLiteralBuilder::StructLiteralBuilder(types::TypeHandle structType, api::impl::Context const &ctx):
    BuilderBase("StructLiteralBuilder", "done", ctx),
    _structType(structType)
  {}
    
  ArrayLiteralBuilder &ArrayLiteralBuilder::push(Literal value) & {
    BuilderBase::check();
    API_REQUIRE(_elements.size() < _length,
		error::ErrorCode::TooManyElementsInArrayInitialization,
		"too many elements for this array-type.");
    API_EXPECT_TYPE(value->type(), _elementType);
    _elements.push_back(value);
    return *this;
  }

  ArrayLiteralBuilder && ArrayLiteralBuilder::push(Literal value) && {
    BuilderBase::check();
    return std::move(this->push(value));
  }
    
  Literal ArrayLiteralBuilder::done() {
    BuilderBase::check();
    API_REQUIRE(_elements.size() == _length,
		error::ErrorCode::TooFewElementsInArrayInitialization,
		"too few elements for this array-type.");
    _finalized.done();
    return std::make_shared<impl::array>(_elementType, _elements, API_FWD);
  }

  ArrayLiteralBuilder::ArrayLiteralBuilder(types::TypeHandle arrayType, api::impl::Context const &ctx):
    BuilderBase("ArrayLiteralBuilder", "done", ctx)
  {
    API_REQUIRE_IS_ARRAY(arrayType);
    _arrayType = types::cast<types::ArrayType>(arrayType);
    _elementType = _arrayType->elementType();
    _length = _arrayType->length();
  }
}
