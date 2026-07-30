#pragma once

#include "acus/sugar/sugar_config.h"

namespace acus::sugar::impl {

  // io
  template <typename T> struct ReadLine;

  // string
  template <typename T>		            struct StrLen;
  template <typename T>			    struct FindChar;
  template <typename T, typename U>	    struct AppendString;
  template <typename T, typename U>	    struct AppendStringCopy;
  template <typename T, typename U>	    struct StringCompare;
  template <typename T, typename U>	    struct StartsWith;
  template <typename T, typename U>	    struct EndsWith;
  template <typename T, typename U>	    struct FindString;
  template <typename T, typename U>	    struct ContainsString;
  template <typename T, typename U, size_t> struct StringToInt;  

  // array
  template <typename T, bool Unroll = config::array::UnrollFill<T>>	   struct FillArray;
  template <typename T, bool Unroll = config::array::UnrollFind<T>>	   struct FindArray;
  template <typename T, bool Unroll = config::array::UnrollContains<T>>	   struct ContainsArray;
  template <typename T, bool Unroll = config::array::UnrollEqual<T>>	   struct EqualArray;
  template <typename T, bool Unroll = config::array::UnrollSum<T>>	   struct SumArray;
  template <typename T, bool Unroll = config::array::UnrollMinMax<T>>	   struct MinArray;
  template <typename T, bool Unroll = config::array::UnrollMinMax<T>>	   struct MaxArray;
  template <typename T, bool Unroll = config::array::UnrollMinMaxIndex<T>> struct MinIndexArray;
  template <typename T, bool Unroll = config::array::UnrollMinMaxIndex<T>> struct MaxIndexArray;
  template <typename T, bool Unroll = config::array::UnrollCount<T>>	   struct CountArray;
  template <typename T, bool Unroll = config::array::UnrollSort<T>>	   struct SortArray;
  template <typename T, bool Unroll = config::array::UnrollIsSorted<T>>	   struct IsSortedArray;  
}
