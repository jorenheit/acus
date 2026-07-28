#pragma once

namespace config {

#ifndef ACUS_SUGAR_LOOP_UNROLL_LIMIT
#define ACUS_SUGAR_LOOP_UNROLL_LIMIT 10
#endif

  inline constexpr size_t LoopUnrollLimit =  ACUS_SUGAR_LOOP_UNROLL_LIMIT;

  namespace array {

    template <typename T, size_t N>
    inline constexpr bool UnrollFill = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollFind = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollContains = (N <= LoopUnrollLimit);
      
    template <typename T, size_t N>
    inline constexpr bool UnrollEqual = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollClear = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollSum = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollMinMax = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollMinMaxIndex = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollCount = (N <= LoopUnrollLimit);

    template <typename T, size_t N>
    inline constexpr bool UnrollSort = (N <= 5);

    template <typename T, size_t N>
    inline constexpr bool UnrollIsSorted = (N <= LoopUnrollLimit);

    
  }

} // config  

