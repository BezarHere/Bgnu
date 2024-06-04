#include "FilePath.hpp"
#include "Logger.hpp"
#include "misc/Error.hpp"

#include <direct.h>
#include <algorithm>

namespace fs
{
	using namespace std::filesystem;
}

// as Mathias Begert on https://stackoverflow.com/questions/32173890
// dot files can have extensions thus the prefix dot is a part of the filename
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

enum class RelationSegmentType : uint8_t
{
	None = 0,

	// just a ordinary segment path
	Normal,
	// should be treated like normal, no effect except being deleted
	Current,

	Parent, // go one directory up if possible

	DriveLetter, // a drive letter (win32) anchor the path to said drive
};

static constexpr RelationSegmentType
get_segment_relation_type(const FilePath::string_blob &segment);

// the returned blob is either to a static pointer or to a slice in base, thus,
// the returned blob data SHOULD NOT be deleted
// also, returned blob might not have a null char at it's end
static FilePath::string_blob get_root_path(const FilePath::string_blob &base);

struct FilePath::Internal
{
	Internal(const string_type &str);
	Internal(const Internal &from, const IndexRange &segments_range);

	inline Blob<const SegmentBlock::value_type> get_segments() const {
		return {segments.data(), segments_count};
	}

	inline string_blob get_text() const {
		return {text.data(), text_length};
	}

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
		Logger::error(
			"Path source too long: \"%s\" is %llu chars, max is %llu chars; truncating.",
			to_cstr(str), text_length, MaxPathLength - 1
		);
		text_length = MaxPathLength - 1;
	}

	string_type::traits_type::copy(text.data(), str.c_str(), text_length);
	text[text_length] = char_type{};

	if (FilePath::preprocess(text.data(), text_length))
	{
		//? do something
	}

	Blob<TextBlock::value_type> blob{text.data(), text_length};

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

	if (FilePath::preprocess(text.data(), text_length))
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
		return EmptyString;
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

FilePath::string_blob FilePath::get_text() const {
	return m_internal->get_text();
}

Blob<const FilePath::segment_type> FilePath::get_segments() const {
	return m_internal->get_segments();
}

FilePath::iterator FilePath::create_iterator() const {
	return iterator(m_internal->text.data());
}

FilePath FilePath::join_path(const string_blob &path) const {
	if (path.size == 0)
	{
		return *this;
	}

	if (path.size >= MaxPathLength)
	{
		Logger::error(
			"path to join \"%s\" is too long (%llu chars), max path length is %llu chars",
			path.data,
			path.size,
			MaxPathLength - 1
		);

		return *this;
	}

	const volatile StrBlob source = get_text();

	string_type result{};
	result.resize(source.size + 1 + path.size);


	char_type *start = result.data();
	string_type::traits_type::copy(start, source.data, source.size);
	start += source.size;

	*start = DirectorySeparator;
	start++;

	string_type::traits_type::copy(start, path.data, path.size);

	return FilePath(result);
}

errno_t FilePath::create_file() const {

	return errno_t();
}

errno_t FilePath::create_directory() const {
	std::error_code err_code;
	fs::create_directories(fs::path(m_internal->text.data()), err_code);

	return errno_t();
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

void FilePath::resolve(const FilePath &base) {
	string_type resolved_str = _resolve_path(m_internal->get_text(), base.get_text());

	m_internal->~Internal();
	new (m_internal) Internal(resolved_str);
}

const FilePath &FilePath::get_working_directory() {
	static const FilePath path{_working_directory()};
	return path;
}

const FilePath &FilePath::get_parent_directory() {
	static const FilePath path{_parent_directory()};
	return path;
}

const FilePath &FilePath::get_executable_path() {
	static const FilePath path{_executable_path()};
	return path;
}

FilePath::string_type FilePath::_working_directory() {
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

FilePath::string_type FilePath::_parent_directory() {
	string_type exc_path = _executable_path();
	string_blob exc_parent = _get_parent({exc_path.data(), exc_path.length()});

	// FIXME: return drive character in windows
	return exc_parent.empty() ? "/" : string(exc_parent.data, exc_parent.size);
}

FilePath::string_type FilePath::_executable_path() {
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

FilePath::string_blob FilePath::_get_parent(const string_blob &source) {
	const size_t anchor = _get_last_separator(source);

	// no separator found or this path is at base-level and has no parent
	if (anchor == npos)
	{
		return string_blob();
	}

	return string_blob(source.data, anchor);
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

FilePath::string_type FilePath::_resolve_path(const string_blob &text, const string_blob &base) {
	const bool root_start = is_directory_separator(text[0]);

	string_type result_str{};
	result_str.reserve(MaxPathLength * 2);

	if (root_start)
	{
		string_blob root = get_root_path(base);

		result_str.append(root.data, root.size);
	}
	else
	{
		result_str.append(base.data, base.length());
	}

	size_t last_anchor = 0;

	// start after the first char, if it can be processed then it had been above 
	for (size_t i = 1; i <= text.size; i++)
	{
		if (is_directory_separator(text[i]) || i == text.size)
		{
			const string_blob segment_source = text.slice(last_anchor, i);

			RelationSegmentType rel_type = get_segment_relation_type(segment_source);

			// "some_path"
			if (rel_type == RelationSegmentType::Normal)
			{
				result_str \
					.append(1, DirectorySeparator)
					.append(segment_source.data, segment_source.length());
			}
			// ".."
			else if (rel_type == RelationSegmentType::Parent)
			{
				string_blob result_blob = {result_str.data(), result_str.length()};

				const size_t parent_sep = _get_last_separator(result_blob);

				// no parent, go back to root
				if (parent_sep == npos)
				{
					string_blob root = get_root_path(result_blob);
					string_type root_str = string_type(root.data, root.length());

					result_str.assign(root_str);
				}
				else
				{
					result_str.resize(parent_sep);
				}
			}
			// drive letters ("E:", "C:")
			else if (rel_type == RelationSegmentType::DriveLetter)
			{
				string_type drive_str = string_type(segment_source.data, segment_source.length());
				result_str.assign(drive_str);
			}
			// current ('.')
			else
			{
				// skip 'current' segments
			}

			// no need to process next char 
			i++;

			last_anchor = i;
		}
	}

	return result_str;
}

bool FilePath::preprocess(Blob<TextBlock::value_type> &text) {
	bool driver_column = false;

	size_t drag_offset = 0;
	size_t last_dir_separator = npos;

	for (size_t i = 0; i < text.size; i++)
	{
		if (is_directory_separator(text[i]))
		{
			if (last_dir_separator == i - 1)
			{
				drag_offset++;
			}

			last_dir_separator = i;
			text[i] = DirectorySeparator;
		}

		text[i - drag_offset] = text[i];

		if (!FilePath::is_valid_path_char(text[i]))
		{
			if (text[i] == ':' && !driver_column)
			{
				driver_column = true;
				continue;
			}

			Logger::error(
				"FilePath: Invalid path \"%.*s\", Bad char '%c' [%llu]",
				text.size,
				text.data,
				text[i],
				i
			);

			return false;
		}
	}

	text.size -= drag_offset;
	text[text.size] = char_type();

	return true;
}

constexpr RelationSegmentType get_segment_relation_type(const FilePath::string_blob &segment) {

	// can't be anything except normal or current
	if (segment.size <= 1)
	{
		return (segment.size == 1 && segment[0] == '.') ?
			RelationSegmentType::Current : RelationSegmentType::Normal;
	}

	if (segment.size == 2)
	{
		if (segment[0] == '.' && segment[1] == '.')
		{
			return RelationSegmentType::Parent;
		}

		if (std::isalpha(segment[0]) && segment[1] == ':')
		{
			return RelationSegmentType::DriveLetter;
		}
	}

	return RelationSegmentType::Normal;
}

FilePath::string_blob get_root_path(const FilePath::string_blob &base) {
#ifdef _WIN32
	LOG_ASSERT_V(FilePath::is_valid_drive_root(base), "Invalid base path");

	return FilePath::string_blob(base.data, 2);
#else
	return FilePath::string_blob(&FilePath::DirectorySeparator, 1);
#endif
}
