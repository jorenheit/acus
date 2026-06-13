// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/types/typesystem.h"

namespace acus::ts::impl {
  
  extern std::unique_ptr<types::VoidType> _void;
  extern std::unique_ptr<types::IntegerType> _i8;
  extern std::unique_ptr<types::IntegerType> _i16;
  extern std::unique_ptr<types::FunctionType> _voidFunction;
  extern std::vector<std::unique_ptr<types::RawType>> _rawTypes;    
  extern std::vector<std::unique_ptr<types::ArrayType>> _arrayTypes;
  extern std::vector<std::unique_ptr<types::StringType>> _stringTypes;
  extern std::vector<std::unique_ptr<types::StructType>> _structTypes;
  extern std::vector<std::unique_ptr<types::PointerType>> _pointerTypes;
  extern std::vector<std::unique_ptr<types::FunctionType>> _functionTypes;
  extern std::vector<std::unique_ptr<types::FunctionPointerType>> _functionPointerTypes;
}


// FunctionTypeBuilder
namespace acus::ts {

  FunctionTypeBuilder & FunctionTypeBuilder::ret(types::TypeHandle returnType) & {
    BuilderBase::check();
    API_REQUIRE(_ret == types::null,
		error::ErrorCode::ReturnTypeSpecifiedMultipleTimes,
		"return-type was already specified.");
    _ret = returnType;
    return *this;
  }

  FunctionTypeBuilder && FunctionTypeBuilder::ret(types::TypeHandle returnType) && {
    BuilderBase::check();
    return std::move(this->ret(returnType));
  }
  
  FunctionTypeBuilder & FunctionTypeBuilder::param(types::TypeHandle paramType) & {
    BuilderBase::check();
    _paramTypes.push_back(paramType);
    return *this;
  }

  FunctionTypeBuilder && FunctionTypeBuilder::param(types::TypeHandle paramType) && {
    BuilderBase::check();
    return std::move(this->param(paramType));
  }

  types::FunctionType const *FunctionTypeBuilder::done() {
    BuilderBase::check();
    _finalized.done();
    
    if (_ret == types::null) _ret = ts::void_t();
    if (_ret == ts::void_t() && _paramTypes.size() == 0) {
      return ts::void_function();
    }
    
    for (auto const &ptr: impl::_functionTypes) {
      if (_ret == ptr->returnType()) {
	if (ptr->paramTypes().size() != _paramTypes.size()) continue;

	bool match = true;
	if (_paramTypes.size() == 0) break;
	for (size_t i = 0; i != _paramTypes.size(); ++i) {
	  if (_paramTypes[i] != ptr->paramTypes()[i]) {
	    match = false;
	    break;
	  }
	}
	if (match) return ptr.get();
      }
    }
    impl::_functionTypes.emplace_back(std::make_unique<types::FunctionType>(_ret, _paramTypes));
    return impl::_functionTypes.back().get();
  }

  FunctionTypeBuilder::FunctionTypeBuilder(api::impl::Context const &ctx):
    BuilderBase("FunctionTypeBuilder", "done", ctx),
    _ret(types::null)
  {}
}


// StructTypeBuilder
namespace acus::ts {

  StructTypeBuilder::StructTypeBuilder(std::string const &structName, api::impl::Context const &ctx):
    BuilderBase("StructTypeBuilder", "done", ctx),
    _structName(structName)
  {}

  StructTypeBuilder & StructTypeBuilder::field(std::string const &name, types::TypeHandle type) &{
    BuilderBase::check();
    _fields.emplace_back(name, type);
    return *this;
  }

  StructTypeBuilder && StructTypeBuilder::field(std::string const &name, types::TypeHandle type) && {
    BuilderBase::check();
    return std::move(this->field(name, type));
  }
  
  types::StructType const *StructTypeBuilder::done() {
    BuilderBase::check();
    _finalized.done();
    if (struct_t(_structName) != nullptr) return nullptr; // already defined a struct with this name
    impl::_structTypes.emplace_back(std::make_unique<types::StructType>(_structName, _fields));
    return impl::_structTypes.back().get();
  }

}
