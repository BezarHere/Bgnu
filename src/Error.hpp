#pragma once
#include <string>

enum class Error
{
	Ok,
	Failure,

	NoData,
	InvalidType,
};

struct ErrorReport
{
	Error code = Error::Ok;
	std::string message;
};
