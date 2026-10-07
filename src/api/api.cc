// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/api/context.h"
#include "acus/assembler/assembler.h"

namespace Impl {
  std::string expected;
}


std::string acus::api::impl::expected() {
  return Impl::expected;
}

void acus::api::impl::clearExpected() {
  Impl::expected = "";
}

void acus::api::impl::expectNext(std::string const &name) {
  Impl::expected = name;
}

acus::api::impl::ExpResult acus::api::impl::isExpected(std::string const &name, bool strict) {
  std::string const &expected = Impl::expected;

  if (expected == name || (!strict && expected.empty())) {
    return { true, "" };
  }

  if (expected.empty()) {
    return {
      .expected = false,
      .msg = "unexpected '" + name + "'."
    };
  }

  return {
    .expected = false,
    .msg = "expected '" + expected + "', but got '" + name + "'."
  };
}

acus::api::impl::Context::Context(Assembler const &a, std::string const &name, std::source_location loc):
  _assembler(&a),
  _name(name),
  _loc(std::move(loc))
{}

acus::api::impl::Context::Context(std::string const &name, std::source_location loc):
  _assembler(nullptr),
  _name(name),
  _loc(std::move(loc))
{}

bool acus::api::impl::Context::blockDispatchMode() const {
  assert(_assembler);
  return !_assembler->programStarted() ||
         _assembler->_program.mode == Program::Mode::BlockDispatch;
}

bool acus::api::impl::Context::programStarted() const {
  assert(_assembler);
  return _assembler->programStarted();
}

std::string acus::api::impl::Context::currentFunction() const {
  assert(_assembler);
  return _assembler->currentFunction();
}

int acus::api::impl::Context::currentScopeDepth() const {
  assert(_assembler);
  return _assembler->currentScopeDepth();
}

bool acus::api::impl::Context::declaredAsGlobal(std::string const &name) const {
  assert(_assembler);
  return _assembler->declaredAsGlobal(name);
}

std::string acus::api::impl::Context::apiName() const {
  assert(_assembler);
  return _name;
}

bool acus::api::impl::Context::globalDeclarationsAllowed() const {
  assert(_assembler);
  return _assembler->globalDeclarationsAllowed();
}

bool acus::api::impl::Context::inScope(std::string const &name) const {
  assert(_assembler);
  return _assembler->inScope(name);
}

bool acus::api::impl::Context::inCurrentScope(std::string const &name) const {
  assert(_assembler);
  return _assembler->inCurrentScope(name);
}
