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

  auto u8 = ts::u8();
  auto u8p = ts::pointer(u8);
  auto arr = ts::array(u8, 5);
  
  c.program("test", "main").begin(); {

    c.function("main").begin(); {
      c.declareLocal("arr", arr);
      c.declareLocal("i", u8);
      c.declareLocal("p", u8p);

      c.assign("i", literal::u8(3));
      c.addAssign("i", "i");
      c.assign("p", c.addressOf(c.addAssign("i", "i")));

      c.write(c.dereferencePointer("p"));
      
      c.returnFromFunction();
    } c.endFunction();

    c.function("get").ret(u8).begin(); {
      c.returnFromFunction(literal::u8(42));
    } c.endFunction();
    
  } c.endProgram();
  
  std::cout << c.brainfuck("test") << '\n';
  
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
