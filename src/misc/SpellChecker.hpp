#pragma once
#include "../base.hpp"

namespace spell
{
	typedef uint32_t edit_distance_t;

	inline edit_distance_t wagner_fischer_distance(const string &left, const string &right) {

		if (left.empty())
		{
			return right.size();
		}

		if (right.empty())
		{
			return left.size();
		}

		const size_t edit_matrix_size = right.size();
		edit_distance_t *edit_matrix = \
			reinterpret_cast<edit_distance_t *>(alloca(edit_matrix_size * sizeof(*edit_matrix)));

		edit_distance_t last_left_side = 0;

		for (size_t x = 0; x < left.size(); x++)
		{
			for (size_t y = 0; y < right.size(); y++)
			{
				const edit_distance_t above = (y == 0) ? (x + 1) : edit_matrix[y - 1];
				const edit_distance_t left_side = (x == 0) ? (y + 1) : edit_matrix[y];
				const edit_distance_t diag = last_left_side;

				edit_matrix[y] = std::min(std::min(above, left_side), diag);


				if (left[x] != right[y])
				{
					edit_matrix[y]++;
				}

				last_left_side = left_side;
			}
		}

		edit_distance_t result = edit_matrix[edit_matrix_size - 1];

		return result;
	}

}
