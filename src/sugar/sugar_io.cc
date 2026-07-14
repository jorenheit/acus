// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/sugar/sugar.h"
#include "acus/sugar/sugar_std.h"

namespace acus::sugar::io {

  void print(char c, SUGAR_LOC) {
    __assembler.write(literal::u8(c));
  }

  void print(int x, SUGAR_LOC) {
    __assembler.print(impl::toLiteral(x));
  }
  
  void print(Expr const &expr, SUGAR_LOC) {
    try {
      __assembler.print(expr.get());
    } catch (error::Error &err) {
      if (err.errorCode == error::ErrorCode::NotPrintable) {
	__assembler.write(expr.get());
      }
      else throw;
    }
  }

  void println(SUGAR_LOC) {
    print('\n', LOC_FWD);
  }

  void read(Expr const &expr, SUGAR_LOC) {
    __assembler.read(expr.get(), LOC_FWD);
  }
  
}
