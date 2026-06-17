#include "acus/sugar/sugar.h"

namespace acus::sugar {

  Assembler __assembler;

  std::string generateBrainfuck(std::string const &programName) {
    return __assembler.brainfuck(programName);
  }
  
  void program(std::string const &name, std::string const &entry) {
    __assembler.program(name, entry).begin();
  }

  void endProgram() {
    __assembler.endProgram();
  }

  void endFunction() {
    __assembler.endFunction();
  }

  void return_() {
    __assembler.returnFromFunction();
  }

  void return_(Expr const &expr) {
    __assembler.returnFromFunction(expr.get());
  }
  
  void print(char c) {
    __assembler.write(literal::u8(c));
  }

  void print(int x) {
    __assembler.print(impl::toLiteral(x));
  }
  
  void print(std::string const &str) {
    __assembler.print(literal::string(str));
  }

  void print(Expr const &expr) {
    try {
      __assembler.print(expr.get());
    } catch (error::Error &err) {
      if (err.errorCode == error::ErrorCode::NotPrintable) {
	__assembler.write(expr.get());
      }
      else throw;
    }
  }

  Expr var(std::string const &varName) {
    return Expr{ __assembler.expr(varName) };
  }
  
  
}
