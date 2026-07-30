namespace acus::sugar {

  template <typename T, size_t N>
  inline impl::FillArray<Array<T, N>>
  Array<T, N>::fill {};

  template <typename T, size_t N>
  inline impl::FindArray<Array<T, N>>
  Array<T, N>::find {};

  template <typename T, size_t N>
  inline impl::ContainsArray<Array<T, N>>
  Array<T, N>::contains {};
  
  template <typename T, size_t N>
  inline impl::EqualArray<Array<T, N>>
  Array<T, N>::equal {};

  template <typename T, size_t N>
  inline impl::SumArray<Array<T, N>>
  Array<T,N>::sum {};

  template <typename T, size_t N>
  inline impl::MinArray<Array<T, N>>
  Array<T, N>::min {};

  template <typename T, size_t N>
  inline impl::MaxArray<Array<T, N>>
  Array<T, N>::max {};

  template <typename T, size_t N>
  inline impl::MinIndexArray<Array<T, N>>
  Array<T, N>::min_index {};

  template <typename T, size_t N>
  inline impl::MaxIndexArray<Array<T, N>>
  Array<T, N>::max_index {};

  template <typename T, size_t N>
  inline impl::CountArray<Array<T, N>>
  Array<T, N>::count {};

  template <typename T, size_t N>
  inline impl::SortArray<Array<T, N>>
  Array<T, N>::sort {};

  template <typename T, size_t N>
  inline impl::IsSortedArray<Array<T, N>>
  Array<T, N>::is_sorted {};

}
  
