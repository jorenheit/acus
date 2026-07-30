#pragma once

namespace acus::sugar::impl {

  template <concepts::String StringType>
  struct ReadLine<StringType>: LibraryFunction<ReadLine<StringType>,
					       StringType()> {
    static void emit(Expr &result);
  }; // ReadLine

} // namespace acus::sugar::impl

// io


#define ACUS_IO_IMPL_read_line ReadLine



#define ACUS_IO_IMPL_EXPAND(name) ACUS_IO_IMPL_##name
#define IO_FUNCTION(name, comment, ...)                            \
  inline impl::ACUS_IO_IMPL_EXPAND(name)                           \
  __VA_OPT__(<__VA_ARGS__>) name
