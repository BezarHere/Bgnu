#pragma once

#include <cmath>
#include <inttypes.h>

// constant math
struct Math
{

	// rotates an integral to the right by an offset
	template <typename T>
	static constexpr inline T rotr(T value, uint8_t offset) {
		constexpr size_t t_bits = sizeof(T) * 8;
		return (value >> offset) | (value << (t_bits - offset));
	}

	// rotates an integral to the left by an offset
	template <typename T>
	static constexpr inline T rotl(T value, uint8_t offset) {
		constexpr size_t t_bits = sizeof(T) * 8;
		return (value << offset) | (value >> (t_bits - offset));
	}

};
