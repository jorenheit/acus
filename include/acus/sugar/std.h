#pragma once
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"

namespace acus::sugar::io {
  // Builtin functions
  
  void print(char c, SUGAR_FUNC);
  void print(int x, SUGAR_FUNC);
  void print(Expr const &expr, SUGAR_FUNC);

  void println(SUGAR_FUNC);
  void println(auto&& arg, SUGAR_FUNC) {
    print(std::forward<decltype(arg)>(arg), LOC_FWD);
    print('\n', LOC_FWD);
  }
  
  void read(Expr const &expr, SUGAR_FUNC);


  template <size_t MaxSize>
  Expr readLine(Expr const &result) {

    __assembler.scope().begin();
    {
      auto i = (let<u8>("i") = 0);
      while_(i < MaxSize) {
	auto elem = result[i];
	read(elem);
	if_(elem == '\n') { break_; };
	++i;
      };
      result[i] = 0;
    }
    __assembler.endScope();
    return result;
  };

  template <size_t MaxSize>
  Expr readLine() {

    auto result = let<string<MaxSize>>(impl::nextVarName());
    __assembler.scope().begin();
    {
      auto i = (let<u8>("i") = 0);
      while_(i < MaxSize) {
	auto elem = result[i];
	read(elem);
	if_(elem == '\n') { break_; };
	++i;
      };
      result[i] = 0;
    }
    __assembler.endScope();
    return result;
  };
  
  
  
}
