#pragma once

namespace config {

#ifndef ACUS_SUGAR_LOOP_UNROLL_LIMIT
#define ACUS_SUGAR_LOOP_UNROLL_LIMIT 10
#endif

  inline constexpr size_t LoopUnrollLimit =  ACUS_SUGAR_LOOP_UNROLL_LIMIT;

  namespace array {

    template <typename T>
    inline constexpr bool UnrollFill = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollFind = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollContains = (T::Size <= LoopUnrollLimit);
      
    template <typename T>
    inline constexpr bool UnrollEqual = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollClear = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollSum = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollMinMax = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollMinMaxIndex = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollCount = (T::Size <= LoopUnrollLimit);

    template <typename T>
    inline constexpr bool UnrollSort = (T::Size <= 5);

    template <typename T>
    inline constexpr bool UnrollIsSorted = (T::Size <= LoopUnrollLimit);

    
  }

} // config  

