#pragma once
#include <chrono>

namespace t
{
	typedef std::chrono::microseconds::rep microsecond_t;

	static inline microsecond_t Now_ms() {
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch()
		).count();
	}

}

