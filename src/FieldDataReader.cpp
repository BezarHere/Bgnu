#include "FieldDataReader.hpp"



FieldDataReader FieldDataReader::branch_reader(const FieldVar::String &name,
																							 const string &new_context) const {
	const FieldVar &dict_var = try_get_value<FieldVarType::Dict>(name);
	if (dict_var.is_null())
	{
		char buffer[inner::ExceptionBufferSz]{0};
		sprintf_s(buffer, "No dict found named '%s' to branch reader", name.c_str());

		throw std::runtime_error(buffer);
	}

	if (new_context.empty())
	{
		return {_context + ":" + name, dict_var.get_dict()};
	}

	return {new_context, dict_var.get_dict()};
}