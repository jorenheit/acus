#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  auto check = [&](Expression actual, literal::Literal expected) {
    c.print(c.eq(actual, expected));
    c.write(literal::u8(10));
  };
  
  c.program("test", "main").begin(); {

    c.function("main").begin(); {

      c.declareLocal("x", ts::u8());
      c.declareLocal("y", ts::u8());

      c.assign("x", literal::u8(1));
      c.assign("y", literal::u8(0));
      check(c.lxor("x", "y"), literal::u8(1));
      
     c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
