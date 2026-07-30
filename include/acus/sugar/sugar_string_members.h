namespace acus::sugar {

  template <size_t N>
  inline impl::StrLen<String<N>>
  String<N>::length {};

  template <size_t N>
  template <typename Other>
  inline impl::StartsWith<String<N>, Other>
  String<N>::starts_with {};

  template <size_t N>
  template <typename Other>
  inline impl::EndsWith<String<N>, Other>
  String<N>::ends_with {};

  template <size_t N>
  inline impl::FindChar<String<N>>
  String<N>::find_char {};


  template <size_t N>
  template <typename Other>
  inline impl::FindString<String<N>, Other>
  String<N>::find_str {};


  template <size_t N>
  template <typename Other>
  inline impl::ContainsString<String<N>, Other>
  String<N>::contains {};


  template <size_t N>
  template <typename Other>
  inline impl::AppendStringCopy<String<N>, Other>
  String<N>::append_to_copy {};


  template <size_t N>
  template <typename Int, size_t Base>
  inline impl::StringToInt<String<N>, Int, Base>
  String<N>::to_int {};


  template <size_t N>
  template <typename Other>
  inline impl::AppendString<String<N>, Other>
  String<N>::append {};

}
