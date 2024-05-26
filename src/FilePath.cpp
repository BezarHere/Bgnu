#include "FilePath.hpp"
#include "Logger.hpp"

static inline bool is_directory_separator(const char value) {
	return value == '\\' || value == '/';
}

static inline bool is_valid_filename_char(const char value) {
	switch (value)
	{
	case ':':
	case '<':
	case '>':
	case '"':
	case '?':
	case '|':
		return false;
	default:
		return true;
	}
}

static inline bool is_valid_filepath_char(const char value) {
	return is_valid_filename_char(value) || is_directory_separator(value);
}

struct FilePath::Internal
{
	Internal(const string_type &str);

	size_t segments_count;
	SegmentBlock segments;

	size_t text_length;
	TextBlock text;
};

FilePath::Internal::Internal(const string_type &str)
	: segments_count{}, text_length{str.length()} {

	// is the path too long? (long strings might not be actual paths)
	if (text_length >= MaxPathLength)
	{
		//! path truncated
		text_length = MaxPathLength - 1;
	}

	string_type::traits_type::copy(text, str.c_str(), text_length);

	// build the segments
	FilePath::build_segments(*this);
}

FilePath::FilePath(const string_type &str) : m_internal{new Internal(str)} {
}

FilePath::~FilePath() {
	delete m_internal;
}

FilePath::FilePath(const FilePath &copy) : m_internal{new Internal(*copy.m_internal)} {
}

FilePath::FilePath(FilePath &&move) noexcept : m_internal{move.m_internal} {
	move.m_internal = nullptr;
}

FilePath &FilePath::operator=(const FilePath &assign) {
	*m_internal = *assign.m_internal;
	return *this;
}

FilePath &FilePath::operator=(FilePath &&move) noexcept {
	m_internal = move.m_internal;
	move.m_internal = nullptr;
	return *this;
}

void FilePath::build_segments(Internal &internals) {

	size_t anchor = 0;

	for (size_t i = 0; i < internals.text_length; i++)
	{
		if (is_directory_separator(internals.text[i]) || i == internals.text_length - 1)
		{
			// we could just break after reaching the segment limit
			// but that will leave an undefined behavior unchecked
			if (internals.segments_count >= MaxPathSegCount)
			{
				Logger::error(
					"FilePath: Too many segments for \"%.*s\"",
					internals.text_length,
					internals.text
				);
				break;
			}

			// we have some characters between the index and the anchor
			if (i - anchor > 0)
			{
				internals.segments[internals.segments_count++] = IndexRange(anchor, i);
			}

			// start anchor after this directory separator
			anchor = i + 1;
		}
	}
}


