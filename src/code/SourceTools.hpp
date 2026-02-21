#pragma once
#include "FilePath.hpp"
#include "base.hpp"

enum class SourceFileType {
  None = 0,

  C,
  CPP,
};

class SourceTools
{
public:
  static constexpr pair<const string_char *, SourceFileType> ExtensionToSourceTable[] = {
    { "c", SourceFileType::C },     { "h", SourceFileType::C },

    { "cpp", SourceFileType::CPP }, { "cc", SourceFileType::CPP },  { "cxx", SourceFileType::CPP },
    { "c++", SourceFileType::CPP }, { "hpp", SourceFileType::CPP }, { "hh", SourceFileType::CPP },
    { "hxx", SourceFileType::CPP }, { "h++", SourceFileType::CPP },

  };

  // pair are interpreted as { <TYPE> (first), <OTHER COMPATABLE TYPE> (second) }
  // types can be converted from `first` to `second` (not otherwise unless stated)
  static constexpr pair<SourceFileType, SourceFileType> CompatableSourceTypeTable[] = {
    { SourceFileType::C, SourceFileType::CPP },

  };

  static void get_dependencies(const StrBlob &file, SourceFileType type, vector<string> &out);
  static FilePath get_intermediate_filepath(const FilePath &filepath, const FilePath &dst_dir);

  static inline bool is_compatable_types(SourceFileType type, SourceFileType partner);
  static inline SourceFileType get_extension_file_type(const StrBlob &extension);
  static inline SourceFileType get_default_file_type(const FilePath &filepath) {
    const string extension = filepath.extension();
    return get_extension_file_type({ extension.c_str(), extension.length() });
  }
};

inline bool SourceTools::is_compatable_types(SourceFileType type, SourceFileType partner) {
  for (size_t i = 0; i < std::size(CompatableSourceTypeTable); i++)
  {
    if (type == CompatableSourceTypeTable[i].first &&
        partner == CompatableSourceTypeTable[i].second)
    {
      return true;
    }
  }
  return false;
}

inline SourceFileType SourceTools::get_extension_file_type(const StrBlob &extension) {
  for (size_t i = 0; i < std::size(ExtensionToSourceTable); i++)
  {
    if (string_tools::equal(extension.begin(), ExtensionToSourceTable[i].first))
    {
      return ExtensionToSourceTable[i].second;
    }
  }
  return SourceFileType::None;
}
