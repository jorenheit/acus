#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {

      c.declareLocal("x", ts::s8());
      c.assign("x", literal::s8(10));
      
      //c.print(c.add("x", literal::s8(2)));
      c.print(c.add(literal::s8(2), "x"));

     c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
