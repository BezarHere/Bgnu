#pragma once
#include "base.hpp"
#include "Range.hpp"
#include "FilePath.hpp"

struct Glob
{
public:
	struct Segment;

	typedef Blob<Segment> SegmentCollection;

	Glob(const string &str);

private:
	static SegmentCollection parse(const StrBlob &blob);

	static inline constexpr bool is_char_reserved(string_char character) {
		return character == string_char('*') ||
			FilePath::is_directory_separator(character) ||
			character == string_char('[');
	}

private:
	string m_source;
	SegmentCollection m_segments;
};
