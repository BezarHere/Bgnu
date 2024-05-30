#pragma once
#include <string>

enum class Error
{
	Ok,
	Failure,

	NoData,
	InvalidType,

	NotImplemented,
};

struct ErrorReport
{
	Error code = Error::Ok;
	std::string message;
};
