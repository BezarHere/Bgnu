#pragma once
#include <stdexcept>
#include "misc/Blob.hpp"

enum class SeekOrigin : unsigned char
{
  Begin,
  Current,
  End
};

template <typename T>
class DataSource
{
public:
  using value_type = T;
  using position_type = signed;
  using offset_type = signed;
  using length_type = unsigned;

  DataSource(const value_type *data, length_type length)
    : m_data{data}, m_length{length} {}

  template <typename E> requires std::is_same_v<T, std::remove_const_t<E>>
  DataSource(const Blob<E> &data)
    : DataSource(data.data, data.size) {}

  template <typename E> requires std::is_same_v<T, std::remove_const_t<E>>
  DataSource(const Blob<E> &data, length_type start, length_type end)
    : DataSource(data.slice(start, end)) {}


  static_assert(std::is_trivial_v<value_type>, "only accept trivial types");

  void seek(offset_type offset, SeekOrigin origin = SeekOrigin::Begin) {
    switch (origin)
    {
    case SeekOrigin::Begin:
      m_position = (position_type)offset;
      return;
    case SeekOrigin::Current:
      m_position += offset;
      return;
    case SeekOrigin::End:
      m_position = (position_type)(size() - offset);
      return;
    }
  }

  void skip(length_type count = 1) {
    seek(count, SeekOrigin::Current);
  }

  length_type size() const noexcept { return m_length; }
  position_type tell() const noexcept { return m_position; }
  length_type available() const noexcept {
    return std::max<length_type>(0, m_length - m_position);
  }

  bool depleted() const noexcept { return available() == 0; }
  bool invalid() const noexcept { return m_position < 0; }
  bool good() const noexcept { return !invalid() && !depleted(); }

  const value_type &current() const {
    if (!good())
    {
      throw std::bad_exception();
    }

    return m_data[m_position];
  }

  value_type get() {
    if (!good())
    {
      throw std::bad_exception();
    }

    return m_data[m_position++];
  }

  const value_type &operator[](offset_type pos) const {
    if (pos < -m_position || pos >= offset_type(m_length - m_position))
    {
      throw std::out_of_range("pos");
    }

    return (m_data + m_position)[pos];
  }

  template <typename PRED>
  void skip_while(PRED &&predicate) {
    if (invalid())
    {
      throw std::bad_exception();
    }

    while (!depleted())
    {
      if (!predicate(current()))
      {
        break;
      }

      skip(1);
    }
  }

  template <typename PRED>
  void skip_until(PRED &&predicate) {
    return skip_while([&predicate](char cur) { return !predicate(cur);});
  }


  operator bool() const noexcept { return good(); }

protected:
  const value_type *const m_data;
  const length_type m_length;
  position_type m_position = 0;
};

