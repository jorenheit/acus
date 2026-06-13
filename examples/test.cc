// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))

int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {
      c.declareLocal("x", ts::s16());
      c.declareLocal("y", ts::s16());
      c.declareLocal("z", ts::s16());

      // -7 / 3 = -2; CAT('C', 'B') - 2 = CAT('A', 'B') -> AB
      c.assign("x", literal::s16(-7));
      c.assign("y", literal::s16(3));
      c.assign("z", c.div("x", "y"));
      c.addAssign("z", literal::s16(CAT('C', 'B')));
      c.write("z"); // AB

      // 7 / -3 = -2; CAT('E', 'D') - 2 = CAT('C', 'D') -> CD
      c.assign("x", literal::s16(7));
      c.assign("y", literal::s16(-3));
      c.assign("z", c.div("x", "y"));
      c.addAssign("z", literal::s16(CAT('E', 'D')));
      c.write("z"); // CD

      // -7 / -3 = 2; CAT('C', 'F') + 2 = CAT('E', 'F') -> EF
      c.assign("x", literal::s16(-7));
      c.assign("y", literal::s16(-3));
      c.assign("z", c.div("x", "y"));
      c.addAssign("z", literal::s16(CAT('C', 'F')));
      c.write("z"); // EF

      // 7 / 3 = 2; CAT('E', 'H') + 2 = CAT('G', 'H') -> GH
      c.assign("x", literal::s16(7));
      c.assign("y", literal::s16(3));
      c.assign("z", c.div("x", "y"));
      c.addAssign("z", literal::s16(CAT('E', 'H')));
      c.write("z"); // GH

      // -7 % 3 = -1; CAT('J', 'J') - 1 = CAT('I', 'J') -> IJ
      c.assign("x", literal::s16(-7));
      c.assign("y", literal::s16(3));
      c.assign("z", c.mod("x", "y"));
      c.addAssign("z", literal::s16(CAT('J', 'J')));
      c.write("z"); // IJ

      // 7 % -3 = 1; CAT('J', 'L') + 1 = CAT('K', 'L') -> KL
      c.assign("x", literal::s16(7));
      c.assign("y", literal::s16(-3));
      c.assign("z", c.mod("x", "y"));
      c.addAssign("z", literal::s16(CAT('J', 'L')));
      c.write("z"); // KL

      // -7 % -3 = -1; CAT('N', 'N') - 1 = CAT('M', 'N') -> MN
      c.assign("x", literal::s16(-7));
      c.assign("y", literal::s16(-3));
      c.assign("z", c.mod("x", "y"));
      c.addAssign("z", literal::s16(CAT('N', 'N')));
      c.write("z"); // MN

      // 7 % 3 = 1; CAT('N', 'P') + 1 = CAT('O', 'P') -> OP
      c.assign("x", literal::s16(7));
      c.assign("y", literal::s16(3));
      c.assign("z", c.mod("x", "y"));
      c.addAssign("z", literal::s16(CAT('N', 'P')));
      c.write("z"); // OP

      c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();
  
  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
