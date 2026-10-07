#include <iostream>
#include <acus/acus.h>

using namespace acus::api;

int main() try {
  Assembler a;

  a.program("hello1", "main").begin(); {

    a.function("main").begin(); {
      a.print(literal::string("Hello, World!\n"));
      a.returnFromFunction();
    } a.endFunction();

  } a.endProgram();

  a.program("hello2", acus::Program::Mode::StraightLine).begin(); {

//    a.print(literal::string("Hello, World!\n"));

  } a.endProgram();
  
  std::cout << a.brainfuck("hello2");
}
catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
