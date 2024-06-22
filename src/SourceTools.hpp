#pragma once
#include "base.hpp"
#include "FilePath.hpp"

enum class SourceFileType
{
	None = 0,

	C,
	CPP,
};

class SourceTools
{
public:
	static void get_dependencies(const StrBlob &file, SourceFileType type, vector<string> &out);
};

