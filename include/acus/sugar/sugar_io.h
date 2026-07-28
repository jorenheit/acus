#pragma once

namespace acus::sugar::impl {

  template <size_t MaxSize>
  struct ReadLine: LibraryFunction<ReadLine<MaxSize>,
                                   String<MaxSize>()> {
    static void emit(Expr &result);
  }; // ReadLine

} // namespace acus::sugar::impl
