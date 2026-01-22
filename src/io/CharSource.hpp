#pragma once
#include "DataSource.hpp"
#include "base.hpp"

class CharSource : public DataSource<string_char>
{
public:
  inline CharSource(const std::basic_string<value_type> &str)
    : DataSource(str.data(), str.length()) {}
  
  inline CharSource(const std::basic_string_view<value_type> &str)
    : DataSource(str.data(), str.length()) {}

  inline string get(length_type length) {
    if (!good())
    {
      throw std::bad_exception();
    }

    if (length == 0)
    {
      return string();
    }

    if (tell() + length > size())
    {
      throw std::out_of_range("length");
    }

    m_position += length;
    return string(m_data + m_position - length, length);
  }

  inline string_char get() { return DataSource::get(); }

  template <typename PRED>
  string get_while(PRED &&predicate) {
    auto last_pos = tell();
    skip_while(std::forward<PRED>(predicate));
    auto final_pos = tell();

    seek(last_pos, SeekOrigin::Begin);

    return get(length_type(final_pos - last_pos));
  }

};

