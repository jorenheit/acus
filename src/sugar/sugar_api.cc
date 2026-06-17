#include "acus/sugar/sugar.h"

namespace acus::sugar {

  Assembler __assembler;

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

  void break_(SUGAR_LOC) {
    __assembler.jump(impl::ControlStack::getBreakLabel(), LOC_FWD);
    __assembler.label(impl::nextLabel(), LOC_FWD);
  }

  void continue_(SUGAR_LOC) {
    __assembler.jump(impl::ControlStack::getContinueLabel(), LOC_FWD);
    __assembler.label(impl::nextLabel(), LOC_FWD);
  }
  
  void return_(SUGAR_LOC) {
    __assembler.returnFromFunction();
  }

  void return_(Expr const &expr, SUGAR_LOC) {
    __assembler.returnFromFunction(expr.get());
  }
  
  void print(char c, SUGAR_LOC) {
    __assembler.write(literal::u8(c));
  }

  void print(int x, SUGAR_LOC) {
    __assembler.print(impl::toLiteral(x));
  }
  
  void print(Expr const &expr, SUGAR_LOC) {
    try {
      __assembler.print(expr.get());
    } catch (error::Error &err) {
      if (err.errorCode == error::ErrorCode::NotPrintable) {
	__assembler.write(expr.get());
      }
      else throw;
    }
  }

  Expr var(std::string const &varName, SUGAR_LOC) {
    return Expr{ __assembler.expr(varName), LOC_FWD };
  }
  
  
}
