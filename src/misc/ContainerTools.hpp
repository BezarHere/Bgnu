#pragma once
#include <map>
#include <set>
#include <vector>

namespace container_tools
{
	template <typename T, typename E>
	static inline std::set<T> keys(const std::map<T, E> &_map) {
		std::set<T> set_{};

		for (const auto &[key, _] : _map)
		{
			set_.insert(key);
		}

		return set_;
	}
	
	template <typename T, typename E>
	static inline std::vector<E> values(const std::map<T, E> &_map) {
		std::vector<E> list{};

		for (const auto &[_, value] : _map)
		{
			list.emplace_back(value);
		}

		return list;
	}
	
	template <typename T, typename E>
	static inline std::set<E> values_set(const std::map<T, E> &_map) {
		std::set<E> list{};

		for (const auto &[_, value] : _map)
		{
			list.insert(value);
		}

		return list;
	}

	template <typename CNT, typename Processor, typename Comparator = std::equal_to<typename CNT::value_type>>
	static inline void intersection_apply(const CNT &left, const CNT &right, Processor &&processor) {
		for (const auto &i : left)
		{

		}
	}
}
