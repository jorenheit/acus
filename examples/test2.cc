#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {
      c.declareLocal("x", ts::u8());
      c.declareLocal("y", ts::u8());

      c.assign("x", literal::u8('A'));
      c.callFunction("foo").into("y").arg("x").arg(literal::u8('B')).done();
      c.write("x");
      c.write("y");
      c.returnFromFunction();
    } c.endFunction();

    c.function("foo").param("arg1", ts::u8()).param("arg2", ts::u8()).ret(ts::u8()).begin(); {
      c.returnFromFunction("arg2");
    } c.endFunction();
    
  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
