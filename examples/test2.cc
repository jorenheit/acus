#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {

      c.declareLocal("idx", ts::u8());
      c.assign("idx", literal::u8(1));

      auto tmpArray =
	literal::array(ts::array(ts::u8(), 3))
	.push(literal::u8('A'))
	.push(literal::u8('B'))
	.push(literal::u8('C'))
	.done();

      auto elem = c.arrayElement(tmpArray, "idx");

      c.addressOf(elem);

      c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
