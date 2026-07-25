// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "acus/types/typesystem.h"
#include "acus/types/literal.h"
#include "acus/assembler/assembler.h"
#include "acus/api/error.h"

namespace acus::api {
  using acus::Assembler;
  using acus::Expression;
  using acus::ts::TypeHandle;
  using acus::literal::Literal;

  // using enum acus::BinOp;
  // using enum acus::UnOp;
  
  namespace literal { using namespace acus::literal; }
  namespace ts      { using namespace acus::ts; }
  namespace error   { using namespace acus::error; }
}
