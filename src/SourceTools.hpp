#pragma once
#include "base.hpp"

enum class SourceType
{
	C,
	CPP = C,

};

class SourceTools
{
public:
	static vector<string> get_dependencies(const StrBlob &file, SourceType type);
};

