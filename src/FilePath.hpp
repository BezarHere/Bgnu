#pragma once
#include <ostream>
#include "Range.hpp"
#include <array>
#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include "StringTools.hpp"

#include "misc/ArrayList.hpp"

struct FilePath;

struct FilePath
{
public:
	typedef std::filesystem::directory_iterator iterator;
	typedef iterator::value_type iterator_entry;

	static constexpr size_t MaxPathLength = 512;
	static constexpr size_t MaxPathSegCount = MaxPathLength / 8;

	using string_type = string;
	using char_type = typename string_type::value_type;

	static constexpr char_type DirectorySeparator = '/';
	static constexpr char_type NullChar = '\0';
	static constexpr const char_type *EmptyString = &NullChar;

	typedef Blob<char_type> mutable_string_blob;
	typedef Blob<const char_type> string_blob;

	typedef size_t separator_index;

	typedef ArrayList<char_type, MaxPathLength> TextArray;
	typedef ArrayList<separator_index, MaxPathSegCount> SeparatorArray;

public:
	static const FilePath &get_working_directory();
	static const FilePath &get_parent_directory();
	static const FilePath &get_executable_path();

	FilePath(const string_blob &str);
	inline FilePath(const string_type &str) : FilePath(string_blob(str.data(), str.length())) {}
	inline FilePath(const char_type *cstr)
		: FilePath(string_blob(cstr, StringTools::length(cstr, MaxPathLength - 1))) {
	}
	// narrowing will have gonky behavior
	inline FilePath(const iterator_entry &entry)
		: FilePath(StringTools::convert(entry.path().c_str(), npos)) {
	}
	~FilePath();

	FilePath(const FilePath &copy) = default;
	FilePath(FilePath &&move) noexcept = default;
	FilePath &operator=(const FilePath &assign) = default;
	FilePath &operator=(FilePath &&move) noexcept = default;

	bool operator<(const FilePath &other) const;
	bool operator==(const FilePath &other) const;

	FilePath &operator+=(const FilePath &right);
	FilePath operator+(const FilePath &right) const;

	operator string_type() const;

	FilePath parent() const;
	string_type filename() const;

	string_type name() const;
	string_type extension() const;

	string_blob get_text() const;

	Blob<const separator_index> get_separators() const;

	iterator create_iterator() const;

	FilePath join_path(const string_blob &path) const;

	inline FilePath join_path(const FilePath &path) const;
	inline FilePath join_path(const string_type &path) const;
	inline FilePath join_path(const char_type *path) const {
		return join_path(string_blob(path, string_type::traits_type::length(path)));
	}

	FilePath &add_path(const string_blob &path);

	inline FilePath &add_path(const FilePath &path) {
		return add_path(path.get_text());
	}

	inline FilePath &add_path(const string_type &path) {
		return add_path(string_blob(path.data(), path.size()));
	}

	inline FilePath &add_path(const char_type *path) {
		return add_path(string_blob(path, string_type::traits_type::length(path)));
	}

	FilePath &pop_path();

	std::ifstream stream_read(bool binary = true) const;
	std::ofstream stream_write(bool binary = true) const;

	string read_string(streamsize max_size = streamsize_max) const;
	void write(const StrBlob &value) const;

	errno_t create_file() const;
	errno_t create_directory() const;

	bool exists() const;
	bool is_file() const;
	bool is_directory() const;

	bool is_valid() const;

	void resolve(const FilePath &base);
	inline void resolve() { resolve(get_working_directory()); }

	inline FilePath resolved_copy(const FilePath &base) const {
		FilePath copy{*this};
		copy.resolve(base);
		return copy;
	}
	inline FilePath resolved_copy() const { return resolved_copy(get_working_directory()); }


	static string_type _working_directory();
	static string_type _parent_directory();
	static string_type _executable_path();

	static constexpr bool is_valid_filename_char(const char_type character);
	static constexpr bool is_valid_path_char(const char_type character);

	// to be used in windows or any OS using drive letters (like 'C:/' or 'F:/')
	static constexpr bool is_valid_drive_root(const string_blob &path);

	static string_blob _get_parent(const string_blob &source);

	// index of the last directory separator, skips trailing separators (e.g. "some/dir/", returns 4)
	// returns `npos` on failure
	static constexpr size_t _get_last_separator(const string_blob &source);

private:

	static void calculate_separators(const string_blob &text, SeparatorArray &out);
	static string_type _resolve_path(const string_blob &text, const string_blob &base);
	static bool preprocess(Blob<TextArray::value_type> &text);

	static inline bool preprocess(TextArray::value_type *text, size_t &length) {
		Blob<TextArray::value_type> blob{text, length};
		bool result = preprocess(blob);
		length = blob.length();
		return result;
	}

private:
	TextArray m_text;
	SeparatorArray m_separators;
};

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
	return is_valid_filename_char(character) || StringTools::is_directory_separator(character);
}

inline constexpr bool FilePath::is_valid_drive_root(const string_blob &path) {
	if (path.size < 2)
	{
		return false;
	}

	if (path[1] != ':')
	{
		return false;
	}

	const char_type letter = path[0];

	return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z');
}

inline constexpr size_t FilePath::_get_last_separator(const string_blob &source) {
	/*
	*	won't check the first and last characters (if not, will fail on "/file" or "path/dir/")
	*/

	for (size_t r = 2; r < source.size; r++)
	{
		const size_t i = source.size - r;
		if (StringTools::is_directory_separator(source[i]))
		{
			return i;
		}
	}

	return npos;
}

inline FilePath FilePath::join_path(const FilePath &path) const {
	return join_path(path.get_text());
}

inline FilePath FilePath::join_path(const string_type &path) const {
	return join_path(string_blob(path.data(), path.length()));
}

namespace std
{
	inline ostream &operator<<(ostream &stream, const FilePath &filepath) {
		return stream << '"' << filepath.get_text().data << '"';
	}
}
