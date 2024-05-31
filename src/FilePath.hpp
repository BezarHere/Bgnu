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

	static constexpr FilePath::char_type DirectorySeparator = '/';

	typedef IndexRange segment_type;

	typedef std::array<char_type, MaxPathLength> TextBlock;
	typedef std::array<segment_type, MaxPathSegCount> SegmentBlock;

	FilePath(const string_type &str);
	inline FilePath(const char_type *cstr) : FilePath(string_type(cstr)) {}
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

	StrBlob get_source() const;
	Blob<const segment_type> get_segments() const;
	
	bool exists() const;
	bool is_file() const;
	bool is_directory() const;

	bool is_valid() const;


	static constexpr bool is_directory_separator(const char_type character);
	static constexpr bool is_valid_filename_char(const char_type character);
	static constexpr bool is_valid_path_char(const char_type character);
private:
	struct Internal;
	FilePath(Internal *data, size_t start, size_t end);

	static void build_segments(Internal &internals);
	static void resolve(Internal &internals, const string_type &base);
	static bool process(TextBlock &text, size_t size);
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
