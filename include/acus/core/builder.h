// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cassert>
#include <exception>

#define API_HEADER
#include "acus/api/api.h"

namespace acus::builder {

  struct FinalizeGuard {
    bool finalized = false;

    FinalizeGuard() = default;
    FinalizeGuard(FinalizeGuard const&) = delete;
    FinalizeGuard& operator=(FinalizeGuard const&) = delete;

    FinalizeGuard(FinalizeGuard&& other) noexcept {
      finalized = other.finalized;
      other.finalized = true;
    }

    FinalizeGuard& operator=(FinalizeGuard&& other) noexcept {
      finalized = other.finalized;
      other.finalized = true;
      return *this;
    }

    ~FinalizeGuard() {
      if (std::uncaught_exceptions() == 0) assert(finalized);
    }

    void done() { finalized = true; }
    operator bool() const { return finalized; }
  };


  struct BuilderBase {
    std::string _builderName;
    std::string _finalizeMethod;
    
    api::impl::Context API_CTX_NAME;
    builder::FinalizeGuard _finalized;

    BuilderBase(std::string const &builderName, std::string const &finalMethod, api::impl::Context const &ctx):
      _builderName(builderName),
      _finalizeMethod(finalMethod),
      API_CTX_NAME(ctx)
    {}
    
    ~BuilderBase() noexcept(false) {
      if (std::uncaught_exceptions() == 0)
	API_REQUIRE(_finalized, error::ErrorCode::BuilderNotFinalized,
		    "'", _finalizeMethod, "' was never called the ", _builderName , ".");
    }

    void check() {
      API_REQUIRE(not _finalized, error::ErrorCode::BuilderUsedAfterFinalize,
		  "builder object '", _builderName, "' can not be used after calling '", _finalizeMethod, "'.");
    }
    
    BuilderBase(BuilderBase const&) = delete;
    BuilderBase& operator=(BuilderBase const&) = delete;

    BuilderBase& operator=(BuilderBase&&) = default;
    BuilderBase(BuilderBase&&) = default;
  };
  
}
