#pragma once
#include "misc/StaticString.hpp"

struct NField_NoInit
{
  inline constexpr explicit NField_NoInit() : _{} {}

  char _;
};

// named field
template <typename FieldType, typename _NameType = StaticString<StaticStr_LongestWordLength>>
class NField
{
public:
  typedef std::remove_cv_t<_NameType> NameType;

  explicit NField(const NameType &name, NField_NoInit) : m_name{ name } {}

  NField(const NameType &name) : m_name{ name }, m_field{} {}

  NField(const NameType &name, FieldType &&move_f)
      : m_name{ name }, m_field{ std::forward<FieldType>(move_f) } {}

  NField(const NameType &name, const FieldType &copy_f) : m_name{ name }, m_field{ copy_f } {}

  inline FieldType *operator->() { return &m_field; }

  inline const FieldType *operator->() const { return &m_field; }

  inline FieldType &operator*() { return m_field; }

  inline const FieldType &operator*() const { return m_field; }

  // inline FieldType *operator&() {
  // 	return &m_field;
  // }

  // inline const FieldType *operator&() const {
  // 	return &m_field;
  // }

  inline const NameType &name() const { return m_name; }

  inline FieldType &field() { return m_field; }

  inline const FieldType &field() const { return m_field; }

private:
  NameType m_name;
  FieldType m_field;
};

enum class NSerializationStance : uint8_t {
  Required,
  Optional,
};

template <typename FieldType, typename _NameType = StaticString<StaticStr_LongestWordLength>>
class NSerializable : public NField<FieldType, _NameType>
{
public:
  typedef NField<FieldType, _NameType> base_type;
  typedef base_type::NameType NameType;
  typedef NSerializationStance Stance;

  static constexpr Stance Required = Stance::Required;
  static constexpr Stance Optional = Stance::Optional;

  explicit NSerializable(const NameType &name, NField_NoInit, Stance required = Stance::Required)
      : base_type{ name, NField_NoInit{} }, m_required{ required } {}

  NSerializable(const NameType &name, Stance required = Stance::Required)
      : base_type{ name }, m_required{ required } {}

  NSerializable(const NameType &name, FieldType &&move_f, Stance required = Stance::Required)
      : base_type{ name, std::forward<FieldType>(move_f) }, m_required{ required } {}

  NSerializable(const NameType &name, const FieldType &copy_f, Stance required = Stance::Required)
      : base_type{ name, copy_f }, m_required{ required } {}

  inline bool is_required() const noexcept { return m_required == Stance::Required; }
  inline bool is_optional() const noexcept { return m_required == Stance::Optional; }

private:
  Stance m_required;
};
