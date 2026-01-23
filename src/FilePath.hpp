/*
  Filepath objects for utility, less verbose then the standard path class

  By zaher abdulatif babker (C) 2024-2025
*/

#pragma once
#include <ostream>
#include "Range.hpp"
#include <array>
#include <string>
#include <memory>
#include <fstream>
#include <filesystem>

#include "HashTools.hpp"
#include "StringTools.hpp"
#include "misc/ArrayList.hpp"

struct FilePath;

struct FilePath
{
public:
  /*
  * === === Aliases === ===
  */

  typedef std::filesystem::directory_iterator iterator;
  typedef iterator::value_type iterator_entry;

  using string_type = string;
  using char_type = typename string_type::value_type;

  typedef Blob<char_type> mutable_string_blob;
  typedef Blob<const char_type> string_blob;

  typedef size_t separator_index;

  /*
  * === === Class Compile-time Constants === ===
  */

  static constexpr size_t MaxPathLength = 512;
  static constexpr size_t MaxPathSegCount = MaxPathLength / 8;

  static constexpr char_type DirectorySeparator = '/';
  static constexpr char_type NullChar = '\0';
  static constexpr const char_type *EmptyString = &NullChar;

  /*
  * === === Utility Aliases === ===
  */

  typedef ArrayList<char_type, MaxPathLength> TextArray;
  typedef ArrayList<separator_index, MaxPathSegCount> SeparatorArray;

  /*
  * === === Enums === ===
  */
  enum class RelationSegmentType : uint8_t
  {
    None = 0,

    // just a ordinary segment path
    Normal,
    // should be treated like normal, no effect except being deleted
    Current,

    Parent, // go one directory up if possible

    DriveLetter, // WIN32: a drive letter 'X:/', anchor the path to said drive
    Home, // UNIX: home '~'
  };

public:
  /*
  * === === GLOBALS === ===
  */

  static const FilePath &get_working_directory();
  static const FilePath &get_parent_directory();
  static const FilePath &get_executable_path();
  static FilePath FindExecutableInPATHEnv(std::string name);

  /*
  * === === Lifetime (Ctors/Dtors/Copy Ctors) === ===
  */

  inline FilePath() = default;

  FilePath(const string_blob &str);
  FilePath(const string_blob &str, const string_blob &base);

  inline FilePath(const string_type &str) : FilePath(string_blob(str.data(), str.length())) {}
  inline FilePath(const char_type *cstr)
    : FilePath(string_blob(cstr, string_tools::length(cstr, MaxPathLength - 1))) {}

  inline FilePath(const string_type &str, const string_type &base)
    : FilePath(string_blob(str.c_str(), str.length()), string_blob(base.c_str(), base.length())) {}

  inline FilePath(const char_type *cstr, const char_type *base)
    : FilePath(
      string_blob(cstr, string_tools::length(cstr, MaxPathLength - 1)),
      string_blob(base, string_tools::length(base, MaxPathLength - 1))
    ) {}

  // narrowing will have gonky behavior
  inline FilePath(const iterator_entry &entry)
    : FilePath(entry.path().c_str()) {}

  FilePath(const FilePath &copy) = default;
  FilePath(FilePath &&move) noexcept = default;
  FilePath &operator=(const FilePath &assign) = default;
  FilePath &operator=(FilePath &&move) noexcept = default;

  ~FilePath();

  /*
  * === === Operators === ===
  */

  bool operator<(const FilePath &other) const;
  bool operator==(const FilePath &other) const;

  FilePath &operator+=(const FilePath &right);
  FilePath operator+(const FilePath &right) const;

  operator string_type() const;

  /*
  * === === Functionality === ===
  */

  // then parent (returns a copy if there is no path, like in roots or drive letters)
  FilePath parent() const;

  // the filepath filename (name + extension)
  string_type filename() const;

  // the filepath name: `"path/file.ext"` returns `"file"`, "path/.dotfile" returns `".dotfile"`
  string_type name() const;
  // the filepath extension: `"path/file.ext"` returns `"ext"`,
  // `"path/.dotfile"` returns `""` (empty)
  string_type extension() const;

  string_blob get_text() const;
  inline const string_char *c_str() const { return m_text.data(); }

  Blob<const separator_index> get_separators() const;

  iterator create_iterator() const;

  FilePath join_path(const string_blob &path) const;

  inline FilePath join_path(const FilePath &path) const;
  inline FilePath join_path(const string_type &path) const;
  inline FilePath join_path(const char_type *path) const {
    return join_path(string_blob(path, string_type::traits_type::length(path)));
  }

  /// @brief adds a segment to the path (e.g. "/some/file".add_path("path") -> "/some/file/path")
  /// @returns *this
  /// @note modifies the path inplace, if you want a joined copy, use join_path()
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

  /// @brief pops a segment of the path (e.g. "/some/file/path" -> "/some/file")
  /// @returns *this
  /// @note modifies the path inplace
  FilePath &pop_path();

  std::ifstream stream_read(bool binary = true) const;
  std::ofstream stream_write(bool binary = true) const;

  string read_string(streamsize max_size = streamsize_max) const;
  void write(const StrBlob &value) const;

  errno_t create_file() const;
  errno_t create_directory() const;

  errno_t remove() const;
  errno_t remove_recursive() const;

  std::filesystem::path to_std_path() const;

  /// @returns is the path empty?
  bool empty() const;

  /// @returns does anything exist at this filepath
  bool exists() const;

  /// @returns is the filepath pointing to a regular file? + returns false if it does not exist
  bool is_file() const;
  /// @returns is the filepath pointing to a directory? + returns false if it does not exist
  bool is_directory() const;

  /// @returns is the path constructed with text and separators data
  bool is_valid() const;

  /// @returns is this an absolute path? not a relative one
  bool is_absolute() const;

  // resolves the path to be absolute
  FilePath &resolve(const FilePath &base);

  // resolves the path to be absolute with the working directory as a base
  inline FilePath &resolve() { return resolve(get_working_directory()); }

  // resolves a copy to be absolute then returns it
  inline FilePath resolved_copy(const FilePath &base) const {
    FilePath copy{*this};
    copy.resolve(base);
    return copy;
  }

  // resolves a copy to be absolute then returns it, base is the working directory as a base
  inline FilePath resolved_copy() const { return resolved_copy(get_working_directory()); }

  hash_t hash() const;

  /*
  * === === Exposed Utility === ===
  */

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

  /*
  * === === Inner Utility === ===
  */

  static void _calculate_separators(const string_blob &text, SeparatorArray &out);
  static string_type _resolve_path(const string_blob &text, const string_blob &base);
  static void _add_resolve_segment(RelationSegmentType rel_type, FilePath::string_type &result_str, const FilePath::string_blob &segment_source);
  static bool _preprocess(Blob<TextArray::value_type> &text);

  static inline bool _preprocess(TextArray::value_type *text, size_t &length) {
    Blob<TextArray::value_type> blob{text, length};
    bool result = _preprocess(blob);
    length = blob.length();
    return result;
  }

private:
  TextArray m_text{};
  SeparatorArray m_separators{};
};

/*
* === === Header Definitions === ===
*/

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
  return is_valid_filename_char(character) || string_tools::is_directory_separator(character);
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
    if (string_tools::is_directory_separator(source[i]))
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
