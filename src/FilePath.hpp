#pragma once
#include "Range.hpp"
#include <string>
#include <memory>

struct FilePath
{
public:
	static constexpr size_t MaxPathLength = 256;
	static constexpr size_t MaxPathSegCount = MaxPathLength / 8;

	using string_type = std::string;
	using char_type = typename string_type::value_type;

	typedef IndexRange segment_type;

	typedef char_type TextBlock[MaxPathLength];
	typedef segment_type SegmentBlock[MaxPathSegCount];

	FilePath(const string_type &str);
	~FilePath();

	FilePath(const FilePath &copy);
	FilePath(FilePath &&move) noexcept;
	FilePath &operator=(const FilePath &assign);
	FilePath &operator=(FilePath &&assign) noexcept;


private:
	struct Internal;

	static void build_segments(Internal &internals);

private:
	Internal *m_internal;
};
