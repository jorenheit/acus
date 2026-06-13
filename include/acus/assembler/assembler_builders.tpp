// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later


// FunctionCallBuilder templates
Assembler::FunctionCallBuilder &  Assembler::FunctionCallBuilder::into(auto&& result) & {
  BuilderBase::check();
  _result = _assembler.lValue(std::forward<decltype(result)>(result), API_FWD);
  return *this;
}

Assembler::FunctionCallBuilder &&  Assembler::FunctionCallBuilder::into(auto&& result) && {
  BuilderBase::check();
  return std::move(this->into(std::forward<decltype(result)>(result)));
}

Assembler::FunctionCallBuilder & Assembler::FunctionCallBuilder::arg(auto&& arg) & {
  BuilderBase::check();  
  _args.push_back(_assembler.rValue(std::forward<decltype(arg)>(arg), API_FWD));
  return *this;
}

Assembler::FunctionCallBuilder &&  Assembler::FunctionCallBuilder::arg(auto&& arg) && {
  BuilderBase::check();  
  return std::move(this->arg(std::forward<decltype(arg)>(arg)));
}
Assembler::FunctionCallBuilder::FunctionCallBuilder(Assembler &a, auto const &function, api::impl::Context const &ctx):
  BuilderBase("FunctionCallBuilder", "done", ctx),
  _assembler(a),
  _function(function)
{}

