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

	explicit NField(const NameType &name, NField_NoInit)
		: m_name{name} {
	}

	NField(const NameType &name)
		: m_name{name}, m_field{} {
	}

	NField(const NameType &name, FieldType &&move_f)
		: m_name{name}, m_field{std::forward<FieldType>(move_f)} {
	}

	NField(const NameType &name, const FieldType &copy_f)
		: m_name{name}, m_field{copy_f} {
	}

	inline FieldType *operator->() {
		return &m_field;
	}

	inline const FieldType *operator->() const {
		return &m_field;
	}

	inline FieldType &operator*() {
		return m_field;
	}

	inline const FieldType &operator*() const {
		return m_field;
	}

	// inline FieldType *operator&() {
	// 	return &m_field;
	// }

	// inline const FieldType *operator&() const {
	// 	return &m_field;
	// }

	inline const NameType &name() const {
		return m_name;
	}

	inline FieldType &field() {
		return m_field;
	}

	inline const FieldType &field() const {
		return m_field;
	}

private:
	NameType m_name;
	FieldType m_field;
};
