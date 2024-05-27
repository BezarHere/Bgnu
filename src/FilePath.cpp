#include "FilePath.hpp"
#include "Logger.hpp"

// is a dot file an nameless extension or a extension-less name?
// #define DOT_FILE_EXTENSIONLESS

static constexpr FilePath::char_type DirectorySeparator = '/';

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

/// @brief finds where the extension starts, if there is an extension
/// @param filename the filename (string) to search
/// @param length the length of `filename`
/// @returns where the extension starts, or the end (`length`) if no extension is found
static inline size_t find_extension(const FilePath::char_type *filename, size_t length) {
	constexpr FilePath::char_type ExtensionChar{'.'};
	if (length < 3)
	{
		return length;
	}

	for (size_t i = length - 2; i > 0; i--)
	{
		if (filename[i] == ExtensionChar)
		{
			return i + 1;
		}
	}


#ifdef DOT_FILE_EXTENSIONLESS
	// dot files are just names without extensions
	return length;
#else
	// dot files are just extensions without names
	return 1;
#endif
}

struct FilePath::Internal
{
	Internal(const string_type &str);
	Internal(const Internal &from, const IndexRange &segments_range);

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

	string_type::traits_type::copy(text.data(), str.c_str(), text_length);
	text[text_length] = char_type{};

	// build the segments
	FilePath::build_segments(*this);
}

FilePath::Internal::Internal(const Internal &from, const IndexRange &segments_range)
	: segments_count{}, text_length{} {


	for (size_t i = segments_range.begin; i < segments_range.end; ++i)
	{

		// checks for too many segments
		if (segments_count >= MaxPathSegCount)
		{
			Logger::error(
				"FilePath::Internal: Too many segments to copy from \"%.*s\", range = %llu to %llu",
				from.text_length,
				from.text,
				segments_range.begin,
				segments_range.end
			);
			break;
		}


		// current segment to process
		const IndexRange &segment = from.segments[i];

		// copy segment
		segments[segments_count++] = segment;

		// check if we should add a directory separator
		if (text_length)
		{
			text[text_length++] = DirectorySeparator;
		}

		// checks for text overflow
		if (text_length + segment.length() >= MaxPathLength)
		{
			Logger::error(
				("FilePath::Internal: Text overflow, can not copy %llu [%llu] chars"
				 " from \"%.*s\", range = %llu to %llu"),

				segment.length(),
				text_length + segment.length(),
				from.text_length,
				from.text,
				segments_range.begin,
				segments_range.end
			);
			break;
		}

		// copy the segment text
		string_type::traits_type::copy(
			&text[text_length],
			&from.text[segment.begin],
			segment.length()
		);
		text_length += segment.length();
	}

	text[text_length] = char_type{};

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

FilePath::operator string_type() const {
	return string_type(m_internal->text.data(), m_internal->text_length);
}

FilePath FilePath::get_parent() const {
	if (m_internal->segments_count == 0)
	{
		return *this;
	}

	return FilePath(m_internal, 0, m_internal->segments_count - 1);
}

FilePath::string_type FilePath::get_filename() const {
	if (!is_valid())
	{
		return "";
	}

	const IndexRange &last_segment = m_internal->segments[m_internal->segments_count - 1];

	return string_type(m_internal->text.data() + last_segment.begin, last_segment.length());
}

FilePath::string_type FilePath::get_name() const {
	if (!is_valid())
	{
		return "";
	}

	const IndexRange &last_segment = m_internal->segments[m_internal->segments_count - 1];

	const char_type *last_seg_text = m_internal->text.data() + last_segment.begin;

	const size_t extension_start = find_extension(
		last_seg_text,
		last_segment.length()
	);

	return string_type(
		last_seg_text,
		extension_start - 1
	);
}

FilePath::string_type FilePath::get_extension() const {
	if (!is_valid())
	{
		return "";
	}

	const IndexRange &last_segment = m_internal->segments[m_internal->segments_count - 1];

	const char_type *last_seg_text = m_internal->text.data() + last_segment.begin;

	const size_t extension_start = find_extension(
		last_seg_text,
		last_segment.length()
	);

	return string_type(
		last_seg_text + extension_start,
		last_segment.length() - extension_start
	);
}

bool FilePath::is_valid() const {
	return m_internal && m_internal->segments_count && m_internal->text_length;
}

FilePath::FilePath(Internal *data, size_t start, size_t end)
	: m_internal{new Internal(*data, IndexRange(start, end))} {
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

void FilePath::resolve(Internal &internals, const string_type &base) {
	// TODO: substitute '..' & '.' to make the path contained in `internals` absolute
}


