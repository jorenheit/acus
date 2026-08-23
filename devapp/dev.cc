#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;


int main() try {
  Assembler c;

  c.program("hello", "main").begin(); {

    c.function("main").begin(); {
      c.print(literal::string("Hello, World!\n"));
      c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();
 
  std::cout << c.brainfuck("hello") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
