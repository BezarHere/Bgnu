#include "StringTools.hpp"



uint8_t string_tools::MBCLength(const string_char character) {
	constexpr uint8_t bits_c = sizeof(string_char) * 8;
	for (uint8_t i = 1; i < bits_c; i++)
	{
		// is the i-th bit from the end on?
		if ((character >> (bits_c - i)) & 1)
		{
			continue;
		}

		return i;
	}
	return 7U;
}

size_t string_tools::MBStrLength(const string_char *str, const size_t max_length) {
	size_t offset = 0;
	for (size_t i = 0; i < (max_length - offset); i++)
	{
		if (str[i + offset] == 0)
		{
			Logger::verbose("MBStrLength: found length=%llu [%llu]\n", i, offset);
			return i;
		}

		offset += MBCLength(str[i + offset]) - 1;
	}
	Logger::verbose("MBStrLength: reached max=%llu\n", max_length - offset);
	return max_length - offset;
}