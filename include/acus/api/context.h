// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <source_location>
#include <string>

namespace acus {
  class Assembler;
}

namespace acus::api::impl {

  class Context {
    Assembler const *_assembler = nullptr;
    std::string _name;
    std::source_location _loc;
    
  public:
    Context() = default;
    Context(std::string const &name, std::source_location loc);    
    Context(Assembler const &a, std::string const &name, std::source_location loc);

    bool programStarted() const;
    std::string currentFunction() const;
    int currentScopeDepth() const;
    bool declaredAsGlobal(std::string const &name) const;
    bool inScope(std::string const &name) const;
    bool inCurrentScope(std::string const &name) const;
    std::string apiName() const;
    bool globalDeclarationsAllowed() const;

    inline auto file_name() const { return _loc.file_name(); }
    inline auto function_name() const { return _loc.function_name(); }
    inline auto line() const { return _loc.line(); }
    inline auto column() const { return _loc.column(); }
  };
  
} // namespace api::impl
