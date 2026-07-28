#pragma once
#include "acus/sugar/sugar_io.h"

namespace acus::sugar::impl {

  template <size_t MaxSize>
  void ReadLine<MaxSize>::emit(Expr &result) {
    auto i = (let_<u8>("i") = 0);
    while_(i < MaxSize) {
      read(result[i]);
      if_(result[i++] == '\n') { break_; };
    };
    result[i - 1] = 0;
  }

} // namespace acus::sugar::impl
