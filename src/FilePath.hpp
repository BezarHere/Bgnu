#pragma once
#include <ostream>
#include "Range.hpp"
#include <array>
#include <string>
#include <memory>
#include <filesystem>
#include "StringUtils.hpp"

struct FilePath;

struct FilePath
{
public:
	typedef std::filesystem::directory_iterator iterator;
	typedef iterator::value_type iterator_entry;

	static constexpr size_t MaxPathLength = 256;
	static constexpr size_t MaxPathSegCount = MaxPathLength / 8;

	using string_type = std::string;
	using char_type = typename string_type::value_type;
	

	static constexpr FilePath::char_type DirectorySeparator = '/';

	typedef IndexRange segment_type;

	typedef Blob<const char_type> string_blob; 

	typedef std::array<char_type, MaxPathLength> TextBlock;
	typedef std::array<segment_type, MaxPathSegCount> SegmentBlock;

	FilePath(const string_type &str);
	inline FilePath(const char_type *cstr) : FilePath(string_type(cstr)) {}
	// narrowing WILL have gonky behavior
	inline FilePath(const iterator_entry &entry)
		: FilePath(StringUtils::narrow(entry.path().c_str(), npos)) {
	}
	~FilePath();

	FilePath(const FilePath &copy);
	FilePath(FilePath &&move) noexcept;
	FilePath &operator=(const FilePath &assign);
	FilePath &operator=(FilePath &&move) noexcept;

	operator string_type() const;

	FilePath get_parent() const;
	string_type get_filename() const;

	string_type get_name() const;
	string_type get_extension() const;

	StrBlob get_text() const;

	Blob<const segment_type> get_segments() const;

	iterator create_iterator() const;

	FilePath join_path(const StrBlob &path) const;

	inline FilePath join_path(const FilePath &path) const;
	inline FilePath join_path(const string_type &path) const;
	inline FilePath join_path(const char_type *path) const {
		return join_path(StrBlob(path, string_type::traits_type::length(path)));
	}

	errno_t create_file() const;
	errno_t create_directory() const;

	bool exists() const;
	bool is_file() const;
	bool is_directory() const;

	bool is_valid() const;

	static const FilePath &get_working_directory();
	static const FilePath &get_parent_directory();
	static const FilePath &get_executable_path();

	static string_type _working_directory();
	static string_type _parent_directory();
	static string_type _executable_path();

	static constexpr bool is_directory_separator(const char_type character);
	static constexpr bool is_valid_filename_char(const char_type character);
	static constexpr bool is_valid_path_char(const char_type character);

	static string_type _get_parent(const StrBlob &source);

private:
	struct Internal;
	FilePath(Internal *data, size_t start, size_t end);

	static void build_segments(Internal &internals);
	static void resolve(Internal &internals, const string_type &base);
	static bool preprocess(Blob<TextBlock::value_type> &text);

	static inline bool preprocess(TextBlock::value_type *text, size_t &length) {
		Blob<TextBlock::value_type> blob{text, length};
		bool result = preprocess(blob);
		length = blob.length();
		return result;
	}

private:
	Internal *m_internal;
};

inline constexpr bool FilePath::is_directory_separator(const char_type character) {
	return character == '\\' || character == '/';
}

inline constexpr bool FilePath::is_valid_filename_char(const char_type character) {
	switch (character)
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

inline constexpr bool FilePath::is_valid_path_char(const char_type character) {
	return is_valid_filename_char(character) || is_directory_separator(character);
}

inline FilePath FilePath::join_path(const FilePath &path) const {
	return join_path(path.get_text());
}

inline FilePath FilePath::join_path(const string_type &path) const {
	return join_path(StrBlob(path.data(), path.length()));
}

namespace std
{
	inline ostream &operator<<(ostream &stream, const FilePath &filepath) {
		return stream << '"' << filepath.get_text().data << '"';
	}
}
