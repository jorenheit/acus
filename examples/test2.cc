#include <iostream>
#include "../include/acus/acus.h"

using namespace acus::api;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))

int main() try {
  Assembler c;

  c.program("test", "main").begin(); {

    c.function("main").begin(); {

      TypeHandle fooType = ts::function()
	.param(ts::s16())
	.ret(ts::s16())
	.done();
	
      Literal doubleFunction = literal::function_pointer(fooType, "double");
      Literal tripleFunction = literal::function_pointer(fooType, "triple");
      auto fooPtrType = ts::function_pointer(fooType);

      c.declareLocal("fPtr", fooPtrType);
      c.declareLocal("x", ts::s16());
      c.declareLocal("input", ts::s16());

      c.print(literal::string("Enter 'd' or 't': "));
      c.read("input");
      c.jumpIf(c.eq("input", literal::u8('d')), "useDouble", "useTriple");

      c.label("useDouble");
      c.assign("fPtr", doubleFunction);
      c.jump("done");
      c.label("useTriple");
      c.assign("fPtr", tripleFunction);
      c.label("done");

      c.assign("x", literal::s16(21));
      c.callFunctionPointer("fPtr")
	.arg("x")
	.into("x")
	.done();

      c.print("x");

      c.returnFromFunction();
    } c.endFunction();

    c.function("double").param("x", ts::s16()).ret(ts::s16()).begin(); {
      c.returnFromFunction(c.mul("x", literal::u8(2)));
    } c.endFunction();

    c.function("triple").param("x", ts::s16()).ret(ts::s16()).begin(); {
      c.returnFromFunction(c.mul("x", literal::u8(3)));
    } c.endFunction();
    
  } c.endProgram();

  std::cout << c.brainfuck("test") << '\n';
 } catch (error::Error const &e) {
  std::cerr << e.what() << '\n';
 }
