#pragma once
#include "base.hpp"

struct PropertyInfo
{
	enum Flags : uint8_t
	{
		ePFlag_None = 0x00,
	};

	string name;
	string desc;
	
	std::vector<string> valid_values;
};
