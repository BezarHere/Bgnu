#pragma once
#include "base.hpp"
#include "Console.hpp"
#include "FieldVar.hpp"

struct ParsingResult
{
	bool success = false;
	string reason = "";
	uint32_t code = 0;
};

struct FieldDataReader
{
public:
	static inline const FieldVar Default = FieldVar(std::nullptr_t());

	inline FieldDataReader(const string &context, const FieldVar::Dict &data)
		: _context{context}, _data{data} {
	}

	template <FieldVarType FType>
	inline const FieldVar &try_get_value(const FieldVar::String &name,
																			 const FieldVar &default_value = Default) const;

	// tries to get an array with the name 'name'.
	// reports an error and returns null if the array doesn't exist or
	// if any element of the array isn't of the template type 
	template <FieldVarType ElementFType>
	inline const FieldVar &try_get_array(const FieldVar::String &name,
																			 const FieldVar &default_value = Default) const;

	// will throw an exception if the reader can't be created
	// branching will fail if the value at 'name' isn't a dict (something else or does not exist)
	FieldDataReader branch_reader(const FieldVar::String &name, const string &new_context = "") const;

	const string _context;
	const FieldVar::Dict &_data;

private:
	template <typename... _Args>
	inline void _error(const char *format, _Args &&...args) const {
		const string new_format = _context + ": " + format;
		Console::error(
			new_format.c_str(), std::forward<_Args>(args)...
		);
	}
};

template<FieldVarType FType>
inline const FieldVar &FieldDataReader::try_get_value(const FieldVar::String &name,
																											const FieldVar &default_value) const {
	if (!_data.contains(name))
	{
		_error(
			"Expecting a value of type %s named '%s'",
			FieldVar::get_type_name(FType),
			name.c_str()
		);
		return default_value;
	}

	const FieldVar &var = _data.at(name);

	if (!var.is_convertible_to(FType))
	{
		_error(
			"Expecting type '%s' for '%s', got type '%s'",
			FieldVar::get_type_name(FType),
			name.c_str(),
			FieldVar::get_type_name(var.get_type())
		);
		return default_value;
	}

	return var;
}

template<FieldVarType ElementFType>
inline const FieldVar &FieldDataReader::try_get_array(const FieldVar::String &name, const FieldVar &default_value) const {
	const FieldVar &array_var = try_get_value<FieldVarType::Array>(name);

	if (array_var.is_null())
	{
		return default_value;
	}

	for (size_t i = 0; i < array_var.get_array().size(); ++i)
	{
		const auto &var = array_var.get_array()[i];
		if (!var.is_convertible_to(ElementFType))
		{
			_error(
				"The array '%s' should only contain %s types, but element at %llu is a/an %s type",
				name.c_str(),
				FieldVar::get_type_name(ElementFType),
				i,
				FieldVar::get_type_name(var.get_type())
			);
			return default_value;
		}
	}

	return array_var;
}
