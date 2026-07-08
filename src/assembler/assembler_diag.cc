// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

std::string Assembler::currentFunction() const {
  return _currentFunction ? (*_currentFunction).name : "";
}

bool Assembler::programStarted() const {
  return _state.begun;
}

bool Assembler::declaredAsGlobal(std::string const &name) const {
  return _program.isGlobal(name);
}

bool Assembler::globalDeclarationsAllowed() const {
  return _state.allowGlobalDeclarations;
}

bool Assembler::inScope(std::string const &name) const {
  return localSlot(name).has_value();
}

bool Assembler::inCurrentScope(std::string const &name) const {
  std::optional<Slot> slot = localSlot(name);
  return slot.has_value() && slot->scope() == _currentScope;
}

int Assembler::currentScopeDepth() const {
  int depth = 0;
  Function::Scope *scope = _currentScope;
  while (scope != nullptr) {
    scope = scope->parent;
    ++depth;
  }
  return depth;
}
