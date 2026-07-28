// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/sugar/sugar.h"
#include "acus/sugar/sugar_std.h"

namespace acus::sugar {

  void print(char c, SUGAR_LOC) {
    __assembler.write(literal::u8(c));
  }

  void print(int x, SUGAR_LOC) {
    __assembler.print(impl::toLiteral(x));
  }
  
  void print(Expr const &expr, SUGAR_LOC) {
    __assembler.print(expr.get(), LOC_FWD);
  }

  void println(SUGAR_LOC) {
    print('\n', LOC_FWD);
  }

  void put(Expr const &expr, SUGAR_LOC) {
    __assembler.write(expr.get(), LOC_FWD);
  }

  void read(Expr const &expr, SUGAR_LOC) {
    __assembler.read(expr.get(), LOC_FWD);
  }
  
}
