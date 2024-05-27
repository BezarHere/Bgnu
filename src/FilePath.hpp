#pragma once
#include "Range.hpp"
#include <array>
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

	typedef std::array<char_type, MaxPathLength> TextBlock;
	typedef std::array<segment_type, MaxPathSegCount> SegmentBlock;

	FilePath(const string_type &str);
	~FilePath();

	FilePath(const FilePath &copy);
	FilePath(FilePath &&move) noexcept;
	FilePath &operator=(const FilePath &assign);
	FilePath &operator=(FilePath &&assign) noexcept;

	operator string_type() const;
	
	FilePath get_parent() const;
	string_type get_filename() const;

	string_type get_name() const;
	string_type get_extension() const;

	bool is_valid() const;

private:
	struct Internal;
	FilePath(Internal *data, size_t start, size_t end);

	static void build_segments(Internal &internals);
	static void resolve(Internal &internals, const string_type &base);

private:
	Internal *m_internal;
};
