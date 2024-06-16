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


FilePath::FilePath(const string_blob &str) {

	// copy string
	m_text.extend(str.data, StringTools::length(str.data, str.length()));

	// can't write to the entire text region, need to place a null at back
	if (m_text.full())
	{
		m_text.pop_back();
	}

	// + null end
	m_text.emplace_back();

	size_t preprocessed_size = m_text.size() - 1;

	if (FilePath::_preprocess(m_text.data(), preprocessed_size))
	{
		//? do something
	}

	// m_text data is modified inplace by FilePath::_preprocess, no need to copy anything
	// just resize to fit
	m_text.resize(preprocessed_size);
	// + null end
	m_text.emplace_back();

	// build the separators
	FilePath::_calculate_separators(string_blob(m_text.data(), m_text.size()), m_separators);
}

FilePath::FilePath(const string_blob &str, const string_blob &base)
	: FilePath(_resolve_path(str, base)) {

}

FilePath::~FilePath() {
}

bool FilePath::operator<(const FilePath &other) const {
	const size_t search_c = std::min(m_text.size(), other.m_text.size());
	for (size_t i = 0; i < search_c; i++)
	{
		if (m_text[i] == other.m_text[i])
		{
			continue;
		}

		if (m_text[i] > other.m_text[i])
		{
			return false;
		}

		return true;
	}
	return false;
}

bool FilePath::operator==(const FilePath &other) const {
	if (m_text.size() != other.m_text.size())
	{
		return false;
	}

	return StringTools::equal(
		m_text.data(), other.m_text.data(),
		m_text.size()
	);
}

FilePath &FilePath::operator+=(const FilePath &right) {

}

FilePath FilePath::operator+(const FilePath &right) const {
	// return FilePath();
}

FilePath::operator string_type() const {
	return string_type(m_text.data(), m_text.size() - 1);
}

FilePath FilePath::parent() const {
	if (!is_valid())
	{
		return *this;
	}

	return FilePath(*this).pop_path();
}

FilePath::string_type FilePath::filename() const {
	if (!is_valid())
	{
		return "";
	}

	// position of last directory separator + 1 (e.g. after it)
	const separator_index last = m_separators.back() + 1;

	return string_type(m_text.data() + last, m_text.size() - last);
}

FilePath::string_type FilePath::name() const {
	if (!is_valid())
	{
		return "";
	}

	// position of last directory separator + 1 (e.g. after it)
	const separator_index last = m_separators.back() + 1;

	const char_type *last_seg_text = m_text.data() + last;
	const size_t last_seg_len = m_text.size() - last;

	const size_t extension_start = find_extension(
		last_seg_text,
		last_seg_len
	);

	return string_type(
		last_seg_text,
		extension_start - 1
	);
}

FilePath::string_type FilePath::extension() const {
	if (!is_valid())
	{
		return EmptyString;
	}

	// position of last directory separator + 1 (e.g. after it)
	const separator_index last = m_separators.back() + 1;

	const char_type *last_seg_text = m_text.data() + last;
	const size_t last_seg_len = m_text.size() - last;

	const size_t extension_start = find_extension(
		last_seg_text,
		last_seg_len
	);

	return string_type(
		last_seg_text + extension_start,
		last_seg_len - extension_start
	);
}

FilePath::string_blob FilePath::get_text() const {
	// excludes the null end
	return {m_text.data(), m_text.size() - 1};
}

Blob<const FilePath::separator_index> FilePath::get_separators() const {
	return {m_separators.data(), m_separators.size()};
}

FilePath::iterator FilePath::create_iterator() const {
	return iterator(m_text.data());
}

FilePath FilePath::join_path(const string_blob &path) const {
	if (path.size == 0)
	{
		return *this;
	}

	FilePath copy = *this;
	copy.add_path(path);
	return copy;
}

FilePath &FilePath::add_path(const string_blob &path) {


	if (!StringTools::is_directory_separator(path[0]))
	{
		// 'back()' should be the null end (hopefully)
		m_text.back() = DirectorySeparator;
	}

	m_text.extend(path.data, path.length());

	size_t size = m_text.size();
	_preprocess(m_text.data(), size);

	m_text.resize(size + 1);
	m_text.back() = 0;

	m_separators.clear();

	_calculate_separators({m_text.data(), m_text.size() - 1}, m_separators);

	return *this;
}

FilePath &FilePath::pop_path() {
	if (!is_valid())
	{
		return *this;
	}

	separator_index last = m_separators.back();
	m_separators.pop_back();

	m_text.resize(last);
	m_text.emplace_back();

	return *this;
}

std::ifstream FilePath::stream_read(bool binary) const {
	return std::ifstream(
		m_text.data(),
		std::ios::openmode::_S_in | std::ios::openmode(binary ? std::ios::openmode::_S_bin : 0)
	);
}

std::ofstream FilePath::stream_write(bool binary) const {
	return std::ofstream(
		m_text.data(),
		std::ios::openmode::_S_out | std::ios::openmode(binary ? std::ios::openmode::_S_bin : 0)
	);
}

string FilePath::read_string(streamsize max_size) const {
	auto stream = stream_read(false);

	string output{};

	stream.seekg(0, std::ios::end);

	const streamsize size = \
		std::min<streamsize>(stream.tellg() + std::streamoff(1), max_size);

	if (size <= 0)
	{
		return "";
	}

	output.resize((size_t)size);

	stream.seekg(0, std::ios::beg);

	stream.read(output.data(), size);
	return output;
}

void FilePath::write(const StrBlob &value) const {
	if (value.size > static_cast<size_t>(streamsize_max))
	{
		char msg[inner::ExceptionBufferSz]{0};

		sprintf_s(
			msg,
			"Can not write %llu bytes of memory, max is %llu bytes!",
			value.size,
			static_cast<size_t>(streamsize_max)
		);

		throw std::length_error(msg);
	}

	auto stream = stream_write(false);

	stream.write(value.data, static_cast<streamsize>(value.size));

	stream.close();
}

errno_t FilePath::create_file() const {

	return errno_t();
}

errno_t FilePath::create_directory() const {
	std::error_code err_code;
	fs::create_directories(fs::path(m_text.data()), err_code);

	return errno_t();
}

bool FilePath::exists() const {
	return std::filesystem::exists(string(m_text.data(), m_text.size()));
}

bool FilePath::is_file() const {
	return std::filesystem::is_regular_file(string(m_text.data(), m_text.size()));
}

bool FilePath::is_directory() const {
	return std::filesystem::is_directory(string(m_text.data(), m_text.size()));
}

bool FilePath::is_valid() const {
	return !(m_separators.empty() || m_text.empty());
}

FilePath &FilePath::resolve(const FilePath &base) {
	string_type resolved_str = _resolve_path(
		string_blob(m_text.begin(), m_text.end() - 1),
		base.get_text()
	);

	// will haunt me later, my future self wouldn't know what hit 'em
	this->~FilePath();
	new (this) FilePath(resolved_str);
	return *this;
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

void FilePath::_calculate_separators(const string_blob &text, SeparatorArray &out) {

	// out.data[out.count++] = npos; // first separator

	for (size_t i = 0; i < text.size; i++)
	{
		if (StringTools::is_directory_separator(text[i]))
		{
			// we could just break after reaching the segment limit
			// but that will leave an undefined behavior unchecked
			if (out.full())
			{
				Logger::error(
					"FilePath: Too many segments for \"%.*s\"",
					text.size,
					text.data
				);
				break;
			}

			out.push_back(i);
		}
	}
}

FilePath::string_type FilePath::_resolve_path(const string_blob &text, const string_blob &base) {
	if (text.empty())
	{
		return "";
	}

	const bool root_start = StringTools::is_directory_separator(text[0]);

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

	const size_t text_count = std::min(MaxPathLength - 1, text.size);

	// start after the first char, if it can be processed then it had been above 
	for (size_t i = 1; i <= text_count; i++)
	{
		if (StringTools::is_directory_separator(text[i]) || i == text.size)
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

bool FilePath::_preprocess(Blob<TextArray::value_type> &text) {
	bool driver_column = false;

	size_t drag_offset = 0;
	size_t last_dir_separator = npos;

	for (size_t i = 0; i < text.size; i++)
	{
		// reached end, trim text by setting drag_offset
		if (!text[i])
		{
			drag_offset = text.size - i;
			break;
		}

		if (StringTools::is_directory_separator(text[i]))
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
