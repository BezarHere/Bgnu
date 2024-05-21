#pragma once
#include "ProjVar.hpp"

class ProjFile
{
public:
	static ProjVar load(const string &filepath);
	static ProjVar read(const char *source, size_t length);


};
