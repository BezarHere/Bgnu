#include "FilePath.hpp"
#include "Logger.hpp"
#include "misc/Error.hpp"
#include <direct.h>

// as Mathias Begert on https://stackoverflow.com/questions/32173890
// dot files can have extensions those the prefix dot is a part of the filename
#define DOT_FILE_EXTENSIONLESS

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

	if (FilePath::validate(text, text_length))
	{
		//? do something
	}

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

	if (FilePath::validate(text, text_length))
	{
		//? do something
	}

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
	if (std::addressof(assign) == this)
	{
		return *this;
	}

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

StrBlob FilePath::to_string() const {
	return {m_internal->text.data(), m_internal->text_length};
}

Blob<const FilePath::segment_type> FilePath::get_segments() const {
	return {m_internal->segments.data(), m_internal->segments_count};
}

FilePath::iterator FilePath::create_iterator() const {
	return iterator(m_internal->text.data());
}

bool FilePath::exists() const {
	return std::filesystem::exists(string(m_internal->text.data(), m_internal->text_length));
}

bool FilePath::is_file() const {
	return std::filesystem::is_regular_file(string(m_internal->text.data(), m_internal->text_length));
}

bool FilePath::is_directory() const {
	return std::filesystem::is_directory(string(m_internal->text.data(), m_internal->text_length));
}

bool FilePath::is_valid() const {
	return m_internal && m_internal->segments_count && m_internal->text_length;
}

const FilePath &FilePath::working_directory() {
	static const FilePath path{_get_working_directory()};
	return path;
}

const FilePath &FilePath::parent_directory() {
	static const FilePath path{_get_parent_directory()};
	return path;
}

const FilePath &FilePath::executable_path() {
	static const FilePath path{_get_executable_path()};
	return path;
}

FilePath::string_type FilePath::_get_working_directory() {
	char_type buffer[MaxPathLength];

	if (_getcwd(buffer, std::size(buffer) - 1) == nullptr)
	{
		const errno_t error = errno;
		Logger::error("Failed to retrieve current working directory: %s", errno_str(error));

		return "";
	}

	buffer[std::size(buffer) - 1] = char_type();

	return string_type(buffer);
}

FilePath::string_type FilePath::_get_parent_directory() {
	string_type exc_path = _get_executable_path();
	string_type exc_parent = _get_parent({exc_path.data(), exc_path.length()});

	// FIXME: return drive character in windows
	return exc_parent.empty() ? "/" : exc_parent;
}

FilePath::string_type FilePath::_get_executable_path() {
	char_type *cstr = nullptr;

	//* docs do not mention anything about freeing the output ptr
	errno_t error = _get_pgmptr(&cstr);

	if (error != 0)
	{
		Logger::error("Failed to retrieve executable path: %s", errno_str(error));
		return "";
	}

	return string_type(cstr);
}

FilePath::string_type FilePath::_get_parent(const StrBlob &source) {
	size_t anchor = npos;

	for (size_t i = source.size; i > 0; i--)
	{
		if (is_directory_separator(source[i - 1]))
		{
			anchor = i - 1;
			break;
		}
	}

	// no separator found, this path is at base-level and has no parent
	if (anchor == npos)
	{
		return "";
	}

	return string_type(source.data, anchor);
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

bool FilePath::validate(TextBlock &text, size_t size) {
	bool driver_column = false;

	for (size_t i = 0; i < size; i++)
	{
		if (text[i] == '\\')
		{
			text[i] = '/';
		}

		if (!FilePath::is_valid_path_char(text[i]))
		{
			if (text[i] == ':' && !driver_column)
			{
				driver_column = true;
				continue;
			}

			Logger::error(
				"FilePath: Invalid path \"%.*s\", Bad char '%c' [%llu]",
				size,
				text.data(),
				text[i],
				i
			);

			return false;
		}
	}
	return true;
}

