#pragma once
#include "base.hpp"
#include "misc/Error.hpp"
#include "Logger.hpp"
#include "FieldVar.hpp"


struct FieldDataReader
{
public:
	static inline const FieldVar Default = FieldVar(std::nullptr_t());
	static inline const FieldVar DefaultArray = FieldVar(FieldVarType::Array);

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
																			 const FieldVar &default_value = DefaultArray) const;

	// will throw an exception if the reader can't be created
	// branching will fail if the value at 'name' isn't a dict (something else or does not exist)
	// also, not specifying the new_context parameter will result in a reader with context equal to
	// "<current context>::<sub-reader data name (i.e name parameter)>" (ex: foo.branch_reader("bar") -> foo::bar)
	FieldDataReader branch_reader(const FieldVar::String &name, const string &new_context = "") const;

	inline const string &get_context() const noexcept { return _context; }
	inline const FieldVar::Dict &get_data() const noexcept { return _data; }

	const string _context;
	const FieldVar::Dict &_data;

private:
	template <typename... _Args>
	inline void _error(const char *format, _Args &&...args) const {
		const string new_format = _context + ": " + format;
		Logger::error(
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
			FieldVar::get_name_for_type(FType),
			name.c_str()
		);
		return default_value;
	}

	const FieldVar &var = _data.at(name);

	if (!var.is_convertible_to(FType))
	{
		_error(
			"Expecting type '%s' for '%s', got type '%s'",
			FieldVar::get_name_for_type(FType),
			name.c_str(),
			FieldVar::get_name_for_type(var.get_type())
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
				FieldVar::get_name_for_type(ElementFType),
				i,
				FieldVar::get_name_for_type(var.get_type())
			);
			return default_value;
		}
	}

	return array_var;
}
