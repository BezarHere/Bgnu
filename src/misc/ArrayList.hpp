#pragma once
#include "../base.hpp"
#include "NakedArray.hpp"

// may construct all elements of count _Sz on ctor
template <typename _T, std::size_t _Sz>
class ArrayList
{
public:
  typedef NakedArray<_T, _Sz> array_type;
  typedef size_t size_type;
  typedef _T value_type;

  constexpr ArrayList() = default;
  ~ArrayList();

  template <typename E, size_t N>
  constexpr bool operator==(const ArrayList<E, N> &other);

  template <std::size_t _N, std::enable_if_t<_N <= _Sz>>
  constexpr ArrayList(const std::array<_T, _N> &array);

  inline constexpr size_type size() const { return m_count; }
  inline constexpr size_type length() const { return m_count; }

  inline static constexpr size_type max_size() { return _Sz; }
  inline static constexpr size_type capacity() { return _Sz; }

  inline constexpr bool empty() const { return m_count == 0; }
  inline constexpr bool full() const { return m_count == capacity(); }

  inline constexpr value_type *data() { return m_array; }
  inline constexpr const value_type *data() const { return m_array; }

  inline constexpr value_type *begin() { return m_array; }
  inline constexpr const value_type *begin() const { return m_array; }

  inline constexpr value_type *end() { return begin() + m_count; }
  inline constexpr const value_type *end() const { return begin() + m_count; }

  inline constexpr const array_type &_inner() const { return m_array; }

  inline value_type &operator[](const size_type index) { return m_array[index]; }
  inline const value_type &operator[](const size_type index) const { return m_array[index]; }

  inline value_type *pointer(const size_type index = 0) { return &m_array[index]; }
  inline const value_type *pointer(const size_type index = 0) const { return &m_array[index]; }

  template <typename... _Args>
  value_type &emplace_back(_Args &&...args);

  value_type &push_back(const value_type &copy);
  value_type &push_back(value_type &&move);

  inline value_type &back() {
    if (empty())
    {
      throw std::runtime_error("back() call on an empty ArrayList");
    }

    return end()[-1];
  }

  inline const value_type &back() const {
    if (empty())
    {
      throw std::runtime_error("back() const call on an empty ArrayList");
    }

    return end()[-1];
  }

  void pop_back();

  // returns the actual amount of elements added, might even be zero if the ArrayList is full
  size_t extend(const value_type *begin, size_t count);

  // returns the actual amount of elements added, might even be zero if the ArrayList is full
  inline size_t extend(const value_type *begin, const value_type *end) {
    if (begin > end)
    {
      throw std::runtime_error("can not extend ArrayList, begin > end");
    }

    return extend(begin, end - begin);
  }

  void clear();

  inline constexpr void _set_size(size_type new_size) {
    if (new_size > max_size())
    {
      throw std::length_error("new_size too large");
    }

    m_count = new_size;
  }
  void resize(size_type new_size);

private:
  inline constexpr void _destroy(const size_type pos) { m_array[pos].~value_type(); }

  inline constexpr void _destroy_all() {
    for (size_t i = 0; i < m_count; i++)
    {
      _destroy(i);
    }
  }

  template <typename... _Args>
  inline constexpr void _construct(const size_type pos, _Args &&...args) {
    new (m_array + pos) value_type(std::forward<_Args>(args)...);
  }

  inline NORETURN void _Xfull() const {
    throw std::length_error("Can not push or emplace to a full ArrayList");
  }

private:
  size_type m_count = 0;
  array_type m_array;
};

template <typename _T, std::size_t _Sz>
template <typename E, size_t N>
inline constexpr bool ArrayList<_T, _Sz>::operator==(const ArrayList<E, N> &other) {
  if (size() != other.size())
  {
    return false;
  }

  for (size_t i = 0; i < size(); i++)
  {
    if (this->operator[](i) != other[i])
    {
      return false;
    }
  }

  return true;
}

template <typename _T, std::size_t _Sz>
template <std::size_t _N, std::enable_if_t<_N <= _Sz>>
inline constexpr ArrayList<_T, _Sz>::ArrayList(const std::array<_T, _N> &array)
    : m_count{ _N }, m_array{ { array.begin(), array.end() } } {}

template <typename _T, std::size_t _Sz>
template <typename... _Args>
inline ArrayList<_T, _Sz>::value_type &ArrayList<_T, _Sz>::emplace_back(_Args &&...args) {
  if (full())
  {
    _Xfull();
  }

  _construct(m_count, std::forward<_Args>(args)...);
  m_count++;

  return end()[-1];
}

template <typename _T, std::size_t _Sz>
inline ArrayList<_T, _Sz>::~ArrayList() {
  _destroy_all();
}

template <typename _T, std::size_t _Sz>
inline ArrayList<_T, _Sz>::value_type &ArrayList<_T, _Sz>::push_back(const value_type &copy) {
  if constexpr (std::is_copy_constructible_v<_T>)
  {
    return emplace_back(copy);
  }
  else
  {
    // use push_back(_T &&) or emplace_back(...)
    throw std::logic_error("_T is not copy constructible");
  }
}

template <typename _T, std::size_t _Sz>
inline ArrayList<_T, _Sz>::value_type &ArrayList<_T, _Sz>::push_back(value_type &&move) {
  if constexpr (std::is_move_constructible_v<_T>)
  {
    return emplace_back(std::forward<value_type>(move));
  }
  else if constexpr (std::is_copy_constructible_v<_T>)
  {
    return emplace_back(static_cast<const value_type &>(move));
  }
  else
  {
    // use emplace_back(...)
    // or redesign your type fr
    throw std::logic_error("_T is not move nor copy constructible");
  }
}

template <typename _T, std::size_t _Sz>
inline void ArrayList<_T, _Sz>::pop_back() {
  if (empty())
  {
    throw std::runtime_error("pop_back() call on an empty ArrayList");
  }

  --m_count;

  _destroy(m_count);
}

template <typename _T, std::size_t _Sz>
inline size_t ArrayList<_T, _Sz>::extend(const value_type *begin, size_t count) {
  count = std::min(capacity() - m_count, count);

  for (size_t i = 0; i < count; i++)
  {
    _construct(i + m_count, begin[i]);
  }

  m_count += count;
  return count;
}

template <typename _T, std::size_t _Sz>
inline void ArrayList<_T, _Sz>::clear() {
  _destroy_all();
  m_count = 0;
}

template <typename _T, std::size_t _Sz>
inline void ArrayList<_T, _Sz>::resize(size_type new_size) {
  if (new_size == size())
  {
    return;
  }

  if (new_size < size())
  {
    if constexpr (!std::is_trivially_destructible_v<_T>)
    {
      for (size_t i = new_size; i < size(); i++)
      {
        _destroy(i);
      }
    }

    _set_size(new_size);
  }

  for (size_t i = size(); i < new_size; i++)
  {
    _construct(i);
  }

  _set_size(new_size);
}
