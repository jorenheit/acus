// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

// ProgramBuilder
namespace acus {

  void Assembler::ProgramBuilder::begin() {
    BuilderBase::check();
    _finalized.done();
    _assembler.beginProgramImpl(_name, _entry, API_FWD);
  }

  Assembler::ProgramBuilder::ProgramBuilder(Assembler &a, std::string const &name, std::string const &entry, api::impl::Context const &ctx):
    BuilderBase("ProgramBuilder", "begin", ctx),
    _assembler(a),
    _name(name),
    _entry(entry)
  {}

} // namespace acus


// FunctionBuilder
namespace acus {

  Assembler::FunctionBuilder & Assembler::FunctionBuilder::ret(types::TypeHandle returnType) & {
    API_REQUIRE(_returnType == types::null,
		error::ErrorCode::ReturnTypeSpecifiedMultipleTimes,
		"return-type was already specified.");
    _returnType = returnType;
    return *this;
  }

  Assembler::FunctionBuilder && Assembler::FunctionBuilder::ret(types::TypeHandle returnType) && {
    BuilderBase::check();  
    return std::move(this->ret(returnType));
  }
  
  Assembler::FunctionBuilder & Assembler::FunctionBuilder::param(std::string const &varName, types::TypeHandle varType) & {
    BuilderBase::check();  
    _params.emplace_back(varName, varType);
    return *this;
  }

  Assembler::FunctionBuilder && Assembler::FunctionBuilder::param(std::string const &varName, types::TypeHandle varType) && {
    BuilderBase::check();  
    return std::move(this->param(varName, varType));
  }
  
  void Assembler::FunctionBuilder::begin() {
      BuilderBase::check();
    _finalized.done();

    std::vector<std::string> varNames;
    auto functionTypeBuilder = ts::function().ret(_returnType);
    for (auto const &[name, type]: _params) {
      varNames.push_back(name);
      functionTypeBuilder.param(type);
    }
    auto functionType = functionTypeBuilder.done();

    _assembler.beginFunctionImpl(_functionName, functionType, varNames, API_FWD);
  }

  Assembler::FunctionBuilder::FunctionBuilder(Assembler &a, std::string const &functionName, api::impl::Context const &ctx):
    BuilderBase("FunctionBuilder", "begin", ctx),
    _assembler(a),
    _functionName(functionName)
  {}

} // namespace acus


// ScopeBuilder
namespace acus {

  void Assembler::ScopeBuilder::begin() {
    _finalized.done();
    _assembler.beginScopeImpl(API_FWD);
  }

  Assembler::ScopeBuilder::ScopeBuilder(Assembler &a, api::impl::Context const &ctx):
    BuilderBase("ScopeBuilder", "begin", ctx),
    _assembler(a)
  {}
  
} // namespace acus
  

// FunctionCallBuilder
namespace acus {

  void Assembler::FunctionCallBuilder::done() {
    _finalized.done();
    if (std::holds_alternative<std::string>(_function)) {
      _assembler.callFunctionImpl(std::get<std::string>(_function), _result, _args, API_FWD);
    } else {
      _assembler.callFunctionImpl(std::get<Expression>(_function), _result, _args, API_FWD);
    }
  }

  
} // namespace acus
