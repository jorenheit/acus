#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {

      c.declareLocal("arr", ts::array(ts::u16(), 10));
      c.declareLocal("i", ts::u8());

      for (int n = 0; n != 10; ++n) {
	c.assign(c.arrayElement("arr", n), literal::u16(1000 + n));
      }

      for (int rep = 0; rep != 100; ++rep) {
	for (int n = 0; n != 10; ++n) {
	  c.assign("i", literal::u8(n));

	  // Dirty cached dynamic element.
	  c.addAssign(c.arrayElement("arr", "i"), literal::u16(1));

	  // Direct write to same array should invalidate/flush dynamic children.
	  // Add zero-ish rewrite preserving the current intended value is hard here,
	  // so use a known value after the dirty flush has happened.
	  c.assign(c.arrayElement("arr", n), literal::u16(1000 + n + rep + 1));
	}
      }

      c.print(c.arrayElement("arr", 9));

     c.returnFromFunction();
    } c.endFunction();

  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
