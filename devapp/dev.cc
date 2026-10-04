#include <iostream>
#include <acus/acus.h>

using namespace acus::api;

int main() try {
  Assembler a;

  a.program("hello", "main").begin(); {

    a.function("main").begin(); {
      a.declareLocal("x", ts::u8());
      a.assign("x", literal::u8(78));
      a.modAssign("x", literal::u8(8));
      a.print("x");
      a.write(literal::u8('\n'));
      
      a.print(literal::string("Hello, World!\n"));
      a.returnFromFunction();
    } a.endFunction();

  } a.endProgram();

  std::cout << a.brainfuck("hello");
}
catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
