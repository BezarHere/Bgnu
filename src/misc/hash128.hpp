#pragma once
#include <array>
#include <limits>

#include "utility/Math.hpp"

typedef uint64_t hash_t;

struct Hash128
{
  static constexpr uint64_t low64_mask = 0xFFFFFFFF;
  static constexpr uint64_t high64_mask = 0xFFFFFFFF'00000000;
  static constexpr uint32_t low32_mask = 0xFFFF;
  static constexpr uint32_t high32_mask = 0xFFFF'0000;
  static constexpr uint32_t low16_mask = 0xFF;
  static constexpr uint32_t high16_mask = 0xFF'00;
  static constexpr uint8_t low8_mask = 0xF;
  static constexpr uint8_t high8_mask = 0xF'0;

  static constexpr size_t numeric_size = 2;

  typedef uint64_t numeric_type;
  typedef std::array<numeric_type, numeric_size> integral_type;
  typedef std::numeric_limits<numeric_type> numeric_type_limits;

  static constexpr size_t numeric_bits_count = sizeof(numeric_type) * 8;
  static constexpr size_t hash_bits_count = numeric_bits_count * numeric_size;

  static_assert(numeric_type(-1) > 0, "only supported unsigned numeric types");

  constexpr Hash128() = default;
  constexpr Hash128(numeric_type num_l, numeric_type num_h) : ivs{ num_l, num_h } {}

  constexpr Hash128(uint32_t u32_0, uint32_t u32_1, uint32_t u32_2, uint32_t u32_3)
      : ivs{ (uint64_t(u32_0) << (numeric_bits_count / 2)) | u32_1,
             (uint64_t(u32_2) << (numeric_bits_count / 2)) | u32_3 } {}

  constexpr Hash128 &operator=(const Hash128 &value) = default;
  constexpr Hash128 &operator=(const numeric_type value) {
    ivs = { value };
    return *this;
  }

  constexpr Hash128 &operator+=(const Hash128 &value) {
    for (size_t i = 0; i < numeric_size - 1; i++)
    {
      if ((numeric_type_limits::max() - value.ivs[i]) < this->ivs[i])
      {
        this->ivs[i + 1]++;
      }

      this->ivs[i] += value.ivs[i];
    }

    // last iteration for adding
    this->ivs[numeric_size - 1] += value.ivs[numeric_size - 1];
    return *this;
  }

  constexpr Hash128 &operator-=(const Hash128 &value) {
    constexpr size_t last_index = numeric_size - 1;
    for (size_t i = 0; i < last_index; i++)
    {
      // underflow will occur, handle it
      if (this->ivs[i] < value.ivs[i])
      {
        // iterating to find the a value to borrow from
        // underflow on value i means that that value borrowed from i + 1, those we continue
        for (size_t j = i + 1; j < last_index; j++)
        {
          this->ivs[j]--;

          // value underflow-ed; continue to borrow
          if (this->ivs[j] == numeric_type_limits::max())
          {
            continue;
          }

          break;
        }
      }

      // borrow completed above, now we subtract
      this->ivs[i] -= value.ivs[i];
    }

    // last iteration for subtracting
    this->ivs[numeric_size - 1] -= value.ivs[numeric_size - 1];
    return *this;
  }

  constexpr Hash128 &operator>>=(uint32_t offset) {
    if (offset >= hash_bits_count)
    {
      *this = 0;
      return *this;
    }

    for (size_t i = 0; i < numeric_size - 1; i++)
    {
      ivs[i] = ivs[i] >> offset;
    }
  }

  // little endian
  integral_type ivs = {};
};
