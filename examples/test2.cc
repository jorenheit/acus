#include <iostream>
#include "acus/assembler/assembler.h"
using namespace acus;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))


int main() try {
  Assembler c;

  c.program("test", "main").begin(); {
    auto array4 = ts::array(ts::u8(), 4);

    c.function("main").begin(); {
      c.declareLocal("arr", array4);
      c.declareLocal("idx", ts::u8());

      c.assign("arr", literal::array(ts::array(ts::u8(), 4)).push(literal::u8('A')).push(literal::u8('B')).push(literal::u8('C')).push(literal::u8('D')).done());
      c.assign("idx", literal::u8(3));

      auto dest = c.arrayElement("arr", "idx");
      c.callFunction("makeZ").into(dest).done();
      c.write("arr");
      c.returnFromFunction();
    } c.endFunction();

    c.function("makeZ").ret(ts::u8()).begin(); {
      c.returnFromFunction(literal::u8('Z'));
    } c.endFunction();



  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
