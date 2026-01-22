#pragma once
#include "FieldVar.hpp"
#include "utility/NField.hpp"

class FieldIO
{
public:
	template <typename T>
	static inline std::string NestedName(const std::string &parent, const NField<T> &field) {
		return parent + ":" + (std::string)field.name();
	}
};
