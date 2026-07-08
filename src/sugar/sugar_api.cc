#include "acus/sugar/sugar.h"
#include "acus/sugar/std.h"

namespace acus::sugar {

  Assembler __assembler;

  namespace io {
    
  }
  

  std::string generateBrainfuck(std::string const &programName, SUGAR_LOC) {
    return __assembler.brainfuck(programName);
  }
  
  void program(std::string const &name, std::string const &entry, SUGAR_LOC) {
    __assembler.program(name, entry).begin();
  }

  void endProgram(SUGAR_LOC) {
    __assembler.endProgram();
  }

  void endFunction(SUGAR_LOC) {
    __assembler.endFunction();
  }

  void break__(SUGAR_LOC) {
    __assembler.jump(impl::ControlStack::getBreakLabel(), LOC_FWD);
    __assembler.label(impl::nextLabel(), LOC_FWD);
  }

  void continue__(SUGAR_LOC) {
    __assembler.jump(impl::ControlStack::getContinueLabel(), LOC_FWD);
    __assembler.label(impl::nextLabel(), LOC_FWD);
  }
  
  void return__(SUGAR_LOC) {
    __assembler.returnFromFunction(LOC_FWD);
  }

  void return__(Expr const &expr, SUGAR_LOC) {
    __assembler.returnFromFunction(expr.get(), LOC_FWD);
  }
    
  Expr var(std::string const &varName, SUGAR_LOC) {
    return Expr{ __assembler.expr(varName), LOC_FWD };
  }

}
