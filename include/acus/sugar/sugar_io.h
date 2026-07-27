#pragma once

namespace acus::sugar::impl {
  
  template <size_t MaxSize>
  struct ReadLine: LibraryFunction<ReadLine<MaxSize>,
				   String<MaxSize>()> {
    static void emit(Expr &result) {
      auto i = (let_<u8>("i") = 0);
      while_(i < MaxSize) {
	read(result[i]);
	if_(result[i++] == '\n') { break_; };
      };
      result[i - 1] = 0;
    }
  }; // ReadLine

  
  
} // acus::sugar::impl


