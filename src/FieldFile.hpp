#pragma once
#include "FieldVar.hpp"

class FieldFile
{
public:
	static FieldVar load(const string &filepath);
	static FieldVar read(const char *source, size_t length);
	
	static void dump(const string &filepath, const FieldVar::VarDict &data);
	static string write(const FieldVar::VarDict &data);

};
