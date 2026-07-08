#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

int main() try {
  Assembler c;

  c.program("test", "main").begin(); {
    c.declareGlobal("g", ts::u8());

    c.function("main").begin(); {

      c.callFunction("foo").into("g").done();
      c.write("g");
      c.callFunction("bar").done();
      c.returnFromFunction();
    } c.endFunction();

    c.function("foo").ret(ts::u8()).begin(); {
      c.declareLocal("y", ts::u8());
      c.assign("y", literal::u8('Y'));
      c.returnFromFunction("y");
    } c.endFunction();

    c.function("bar").begin(); {
      c.write("g");
      c.returnFromFunction();
    } c.endFunction();
    
  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
