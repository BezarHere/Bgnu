#pragma once
#include "base.hpp"
#include <iostream>
#include <unordered_map>

enum class ProjVarType : char
{
	Null,
	Boolean,
	Number,
	String,
	Array,
	Dict,
};

struct ProjVar
{
public:
	using VarNull = std::nullptr_t;
	using VarBool = bool;
	using VarNumber = float;
	using VarString = string;
	using VarArray = vector<ProjVar>;
	using VarDict = std::unordered_map<VarString, ProjVar>;

	inline ProjVar(ProjVarType type = ProjVarType::Null);

	inline ProjVar(VarNull) : m_type{ProjVarType::Null} {}
	inline ProjVar(VarBool boolean) : m_type{ProjVarType::Boolean}, m_bool{boolean} {}
	inline ProjVar(VarNumber number) : m_type{ProjVarType::Number}, m_number{number} {}
	inline ProjVar(const VarString &string) : m_type{ProjVarType::String}, m_string{string} {}
	inline explicit ProjVar(const VarArray &array) : m_type{ProjVarType::Array}, m_array{array} { std::cout << this << '\n'; }
	inline explicit ProjVar(const VarDict &dict) : m_type{ProjVarType::Dict}, m_dict{dict} {}

	inline ProjVar(const VarString::value_type *cstring) : ProjVar(VarString(cstring)) {}

	inline ~ProjVar();
	inline ProjVar(const ProjVar &copy);

	inline ProjVar &operator=(const ProjVar &copy);

	inline ProjVarType get_type() const noexcept { return m_type; }

	inline VarBool get_bool() const noexcept { return m_bool; }
	inline VarNumber get_number() const noexcept { return m_number; }
	inline const VarString &get_string() const noexcept { return m_string; }
	inline const VarArray &get_array() const noexcept { return m_array; }
	inline const VarDict &get_dict() const noexcept { return m_dict; }

private:
	template <typename Transformer>
	inline void _handle(Transformer &&transformer) {
		switch (m_type)
		{
		case ProjVarType::Boolean:
			return transformer(m_bool);
		case ProjVarType::Number:
			return transformer(m_number);
		case ProjVarType::String:
			return transformer(m_string);
		case ProjVarType::Array:
			return transformer(m_array);
		case ProjVarType::Dict:
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
	ProjVarType m_type;

	union
	{
		VarBool m_bool;
		VarNumber m_number;
		VarString m_string;
		VarArray m_array;
		VarDict m_dict;
	};
};

inline ProjVar::ProjVar(ProjVarType type) : m_type{type} {
	_handle<inner::EmptyCTor>();
}

inline ProjVar::~ProjVar() {
	_handle<inner::DTor>();
}

ProjVar::ProjVar(const ProjVar &copy) : m_type{copy.m_type} {
	_handle(inner::TypelessCTor(copy._union_ptr()));
}

inline ProjVar &ProjVar::operator=(const ProjVar &copy) {
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
	inline ostream &operator<<(ostream &stream, const ProjVar &project_variable) {
		switch (project_variable.get_type())
		{
		case ProjVarType::Null:
			return stream << "<PVar=Null>";
		case ProjVarType::Boolean:
			return stream << "<PVar=" << (project_variable.get_bool() ? "true" : "false") << ">";
		case ProjVarType::Number:
			return stream << "<PVar=" << project_variable.get_number() << ">";
		case ProjVarType::String:
			return stream << "<PVar=\"" << project_variable.get_string() << "\">";
		case ProjVarType::Array:
			{
				stream << "<PVar=[";
				const ProjVar::VarArray &array = project_variable.get_array();

				for (size_t i = 0; i < array.size(); ++i) {
					if (i)
					{
						stream << ", ";
					}

					stream << array[i];
				}

				return stream << "]>";
			}
		case ProjVarType::Dict:
			{
				stream << "<PVar={";
				const ProjVar::VarDict &dict = project_variable.get_dict();

				size_t counter = 0;
				for (const auto &kv : dict) {
					if (counter)
					{
						stream << ", ";
					}

					++counter;

					stream << '"' << kv.first << "\": " << kv.second;
				}

				return stream << "}>";
			}
		}
		return stream;
	}
}

