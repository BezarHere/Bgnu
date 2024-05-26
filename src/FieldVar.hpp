#pragma once
#include "base.hpp"
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

enum class FieldVarType : char
{
	Null,
	Boolean,
	Integer,
	Real,
	
	String,
	Array,
	Dict,
};

struct FieldVar
{
public:
	using Null = std::nullptr_t;
	using Bool = bool;
	using Int = int64_t;
	using Real = float;
	using String = string;
	using Array = vector<FieldVar>;
	using Dict = std::unordered_map<String, FieldVar>;

	// returns the name of the given type
	static constexpr const char *get_name_for_type(FieldVarType type);
	static constexpr bool is_convertible(FieldVarType from, FieldVarType to);
	static constexpr bool is_type_simple(FieldVarType type);

	inline FieldVar(FieldVarType type = FieldVarType::Null);

	inline explicit FieldVar(Null) : m_type{FieldVarType::Null} {}
	inline FieldVar(Bool boolean) : m_type{FieldVarType::Boolean}, m_bool{boolean} {}
	inline FieldVar(Int integer) : m_type{FieldVarType::Integer}, m_int{integer} {}
	inline FieldVar(Real number) : m_type{FieldVarType::Real}, m_real{number} {}
	inline FieldVar(const String &string) : m_type{FieldVarType::String}, m_string{string} {}
	inline explicit FieldVar(const Array &array) : m_type{FieldVarType::Array}, m_array{array} { std::cout << this << '\n'; }
	inline explicit FieldVar(const Dict &dict) : m_type{FieldVarType::Dict}, m_dict{dict} {}

	inline FieldVar(const String::value_type *cstring) : FieldVar(String(cstring)) {}

	inline ~FieldVar();
	inline FieldVar(const FieldVar &copy);

	inline FieldVar &operator=(const FieldVar &copy);
	inline bool operator==(const FieldVar &other) const;

	inline FieldVarType get_type() const noexcept { return m_type; }

	/// @brief transforms the value to it's string representation
	/// @brief (e.g. FieldVar(3.14F).string() == FieldVar("3.14"))
	inline void stringify();

	/// @brief creates a copy, stringifies it and returns it
	inline FieldVar copy_stringified() const {
		FieldVar copy{*this};
		copy.stringify();
		return copy;
	}

	inline Bool get_bool() const;
	inline Int get_int() const;
	inline Real get_real() const;
	inline const String &get_string() const;
	inline const Array &get_array() const;
	inline const Dict &get_dict() const;

	inline operator Bool() const { return get_bool(); }
	inline operator Int() const { return get_int(); }
	inline operator Real() const { return get_real(); }
	inline operator const String &() const { return get_string(); }
	inline explicit operator const Array &() const { return get_array(); }
	inline explicit operator const Dict &() const { return get_dict(); }

	/// @brief is the value a null? like it has nothing?
	inline bool is_null() const { return get_type() != FieldVarType::Null; }

	inline bool is_convertible_to(const FieldVarType type) const {
		return is_convertible(m_type, type);
	}

	inline bool has_simple_type() const {
		return is_type_simple(m_type);
	}

	// get the name of the field var's type 
	inline const char *get_type_name() const {
		return get_name_for_type(m_type);
	}

private:
	template <typename Transformer>
	inline void _handle(Transformer &&transformer) {
		switch (m_type)
		{
		case FieldVarType::Boolean:
			return transformer(m_bool);
		case FieldVarType::Integer:
			return transformer(m_int);
		case FieldVarType::Real:
			return transformer(m_real);
		case FieldVarType::String:
			return transformer(m_string);
		case FieldVarType::Array:
			return transformer(m_array);
		case FieldVarType::Dict:
			return transformer(m_dict);
		default:
			return;
		}
	}

	template <typename Transformer>
	inline void _handle() { _handle(Transformer()); }

	inline void *_union_ptr() { return &m_bool; }
	inline const void *_union_ptr() const { return &m_bool; }

private:
	FieldVarType m_type;

	union
	{
		Bool m_bool;
		Int m_int;
		Real m_real;
		String m_string;
		Array m_array;
		Dict m_dict;
	};
};

class fieldvar_access_violation : public std::runtime_error
{
	static string _generate_msg(const FieldVar &fieldvar, const FieldVarType access_type) {
		char buffer[inner::ExceptionBufferSz]{0};
		sprintf_s(
			buffer,
			"Trying to access a fieldvar [%p] of type '%s' as type '%s'",
			&fieldvar,
			FieldVar::get_name_for_type(fieldvar.get_type()),
			FieldVar::get_name_for_type(access_type)
		);

		return buffer;
	}

public:
	explicit fieldvar_access_violation(const FieldVar &fieldvar, const FieldVarType access_type)
		: runtime_error(_generate_msg(fieldvar, access_type)),
		_access_type{access_type}, _fieldvar{fieldvar} {
	}

	const FieldVarType _access_type;
	const FieldVar &_fieldvar;
};

namespace std
{
	inline ostream &operator<<(ostream &stream, const FieldVar &project_variable) {
		switch (project_variable.get_type())
		{
		case FieldVarType::Null:
			return stream << "null";
		case FieldVarType::Boolean:
			return stream << (project_variable.get_bool() ? "true" : "false");
		case FieldVarType::Integer:
			return stream << project_variable.get_int();
		case FieldVarType::Real:
			return stream << project_variable.get_real();
		case FieldVarType::String:
			return stream << "\"" << project_variable.get_string() << "\"";
		case FieldVarType::Array:
			{
				stream << "[";
				const FieldVar::Array &array = project_variable.get_array();

				for (size_t i = 0; i < array.size(); ++i) {
					if (i)
					{
						stream << ", ";
					}

					stream << array[i];
				}

				return stream << "]";
			}
		case FieldVarType::Dict:
			{
				stream << "{";
				const FieldVar::Dict &dict = project_variable.get_dict();

				size_t counter = 0;
				for (const auto &kv : dict) {
					if (counter)
					{
						stream << ", ";
					}

					++counter;

					stream << '"' << kv.first << "\": " << kv.second;
				}

				return stream << "}";
			}
		}
		return stream;
	}
}

#pragma region(defines)
inline constexpr const char *FieldVar::get_name_for_type(FieldVarType type) {
	constexpr const char *Names[]{
		"null",
		"boolean",
		"integer",
		"real",
		"string",
		"array",
		"dict"
	};
	return Names[(int)type];
}

inline constexpr bool FieldVar::is_convertible(FieldVarType from, FieldVarType to) {
	// self to self
	if (from == to)
	{
		return true;
	}

	switch (from)
	{
		// should't be converted to anything, null is error to be handled
	case FieldVarType::Null:
		return false;
	case FieldVarType::Integer:
		return to == FieldVarType::Real || to == FieldVarType::Boolean;
	case FieldVarType::Real:
		return to == FieldVarType::Integer;

	default:
		// string, arrays and dicts can't be converted to anything easily
		return false;
	}
}

inline constexpr bool FieldVar::is_type_simple(FieldVarType type) {
	return (int)type < (int)FieldVarType::String;
}

inline FieldVar::FieldVar(FieldVarType type) : m_type{type} {
	_handle<inner::EmptyCTor>();
}

inline FieldVar::~FieldVar() {
	_handle<inner::DTor>();
}

FieldVar::FieldVar(const FieldVar &copy) : m_type{copy.m_type} {
	_handle(inner::TypelessCTor(copy._union_ptr()));
}

inline FieldVar &FieldVar::operator=(const FieldVar &copy) {
	// type didn't change, just assign data
	if (copy.m_type == m_type)
	{
		_handle(inner::TypelessAssign(copy._union_ptr()));
		return *this;
	}

	// changed data type, destroy old data
	_handle<inner::DTor>();

	m_type = copy.m_type;

	// construct new data
	_handle(inner::TypelessCTor(copy._union_ptr()));
	return *this;
}

inline bool FieldVar::operator==(const FieldVar &other) const {
	if (!other.is_convertible_to(get_type()))
	{
		return false;
	}

	switch (m_type)
	{
	case FieldVarType::Null:
		// null always equals null
		return true;
	case FieldVarType::Boolean:
		return get_bool() == other.get_bool();
	case FieldVarType::Integer:
		return get_int() == other.get_int();
	case FieldVarType::Real:
		return get_real() == other.get_real();
	case FieldVarType::String:
		return get_string() == other.get_string();
	case FieldVarType::Array:
		return get_array() == other.get_array();
	case FieldVarType::Dict:
		return get_dict() == other.get_dict();
	default:
		return false;
	}
}

inline void FieldVar::stringify() {
	std::ostringstream stream{};
	stream << *this;
	*this = stream.str();
}

inline FieldVar::Bool FieldVar::get_bool() const {
	if (m_type == FieldVarType::Boolean)
	{
		return m_bool;
	}

	switch (m_type)
	{
	case FieldVarType::Null:
		return false;
	case FieldVarType::Integer:
		return m_int != 0;
	case FieldVarType::Real:
		return m_real != 0.0F;
	case FieldVarType::String:
		return !m_string.empty();
	case FieldVarType::Array:
		return !m_array.empty();
	case FieldVarType::Dict:
		return !m_dict.empty();

	default:
		return m_bool;
	}
}

inline FieldVar::Int FieldVar::get_int() const {
	if (m_type == FieldVarType::Integer)
	{
		return m_int;
	}

	switch (m_type)
	{
	case FieldVarType::Null:
		return 0;
	case FieldVarType::Boolean:
		return m_bool;
	case FieldVarType::Real:
		return Int(m_real);
		// case FieldVarType::String:
		// case FieldVarType::Array:
		// case FieldVarType::Dict:

	default:
		return m_bool;
	}
}

inline FieldVar::Real FieldVar::get_real() const {
	if (m_type == FieldVarType::Real)
	{
		return m_real;
	}

	switch (m_type)
	{
	case FieldVarType::Null:
		return 0.0F;
	case FieldVarType::Boolean:
		return Real(m_bool);
	case FieldVarType::Integer:
		return Real(m_int);
		// case FieldVarType::String:
		// case FieldVarType::Array:
		// case FieldVarType::Dict:

	default:
		return m_bool;
	}
}

inline const FieldVar::String &FieldVar::get_string() const {
	if (m_type == FieldVarType::String)
	{
		return m_string;
	}

	throw fieldvar_access_violation(*this, FieldVarType::String);
}

inline const FieldVar::Array &FieldVar::get_array() const {
	if (m_type == FieldVarType::Array)
	{
		return m_array;
	}

	throw fieldvar_access_violation(*this, FieldVarType::Array);
}

inline const FieldVar::Dict &FieldVar::get_dict() const {
	if (m_type == FieldVarType::Dict)
	{
		return m_dict;
	}

	throw fieldvar_access_violation(*this, FieldVarType::Dict);
}

#pragma endregion
