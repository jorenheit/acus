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

      c.assign("x", literal::s16(10));
      c.assign("y", literal::s16(0));
      c.assign("z", c.mod("x", "y"));
      
      c.print("z");

     c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
