#pragma once
#include "base.hpp"
#include <iostream>
#include <unordered_map>

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

	inline FieldVarType get_type() const noexcept { return m_type; }

	inline Bool get_bool() const noexcept { return m_bool; }
	inline Int get_int() const noexcept { return m_int; }
	inline Real get_real() const noexcept { return m_real; }
	inline const String &get_string() const noexcept { return m_string; }
	inline const Array &get_array() const noexcept { return m_array; }
	inline const Dict &get_dict() const noexcept { return m_dict; }

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




namespace std
{
	inline ostream &operator<<(ostream &stream, const FieldVar &project_variable) {
		switch (project_variable.get_type())
		{
		case FieldVarType::Null:
			return stream << "Null";
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

