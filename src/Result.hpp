#pragma once
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "misc/Error.hpp"

/*
  if (result)
  {
    // has value!
  }
  
  if (!result)
  {
    // no value :(
  }
  
*/
template <typename T>
class Result
{
public:
  using value_type = T;
  using error_type = Error;
  using message_type = std::string;

  static_assert(!std::is_same_v<value_type, error_type>);

  inline Result(value_type &&value) : m_value{ std::forward<value_type>(value) } {}
  inline Result(Error error, const std::string &msg)
      : m_value{ std::nullopt }, m_error{ error }, m_message{ msg } {}

  template <typename U = value_type>
    requires(std::is_default_constructible_v<U>)
  inline Result() : m_value{ U{} } {}

  template <typename U = value_type>
    requires(!std::is_default_constructible_v<U>)
  inline Result() : Result(Error::Failure) {}

  Result(const Result &copy) = default;
  Result(Result &&move) = default;

  inline value_type *operator->() {
    if (m_value.has_value())
    {
      return &m_value.value();
    }

    throw std::runtime_error("Result has no value");
  }

  inline const value_type *operator->() const {
    if (m_value.has_value())
    {
      return &m_value.value();
    }

    throw std::runtime_error("Result has no value");
  }

  inline value_type &operator*() {
    if (m_value.has_value()) return m_value.value();
    throw std::runtime_error("Result has no value");
  }

  inline const value_type &operator*() const {
    if (m_value.has_value()) return m_value.value();
    throw std::runtime_error("Result has no value");
  }

  inline operator ErrorReport() const { return { m_error, m_message }; }

  inline operator bool() const { return m_value.has_value(); }

  inline bool has_value() const { return m_value.has_value(); }

  inline value_type &value() { return m_value.value(); }
  
  inline const value_type &value() const { return m_value.value(); }
  
  inline error_type error() const { return m_error; }
  
  inline const message_type &message() const { return m_message; }

private:
  std::optional<value_type> m_value;
  error_type m_error = Error::Ok;
  message_type m_message = "";
};
