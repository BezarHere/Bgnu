#pragma once
#include "FieldVar.hpp"
#include "FilePath.hpp"

class FieldFile
{
public:
	static FieldVar load(const FilePath &filepath);
	static FieldVar read(const string_char *source, size_t length);
	
	static void dump(const FilePath &filepath, const FieldVar::Dict &data);
	static string write(const FieldVar::Dict &data);

};
