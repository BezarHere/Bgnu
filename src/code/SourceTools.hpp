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
  struct ExtensionInfo
  {
    const string_char *name;
    SourceFileType type;
    bool compilable = false;
  };

  static constexpr ExtensionInfo ExtensionToSourceTable[] = {
    { "c", SourceFileType::C, true },      { "h", SourceFileType::C, false },

    { "cpp", SourceFileType::CPP, true },  { "cc", SourceFileType::CPP, true },
    { "cxx", SourceFileType::CPP, true },  { "c++", SourceFileType::CPP, true },
    { "hpp", SourceFileType::CPP, false }, { "hh", SourceFileType::CPP, false },
    { "hxx", SourceFileType::CPP, false }, { "h++", SourceFileType::CPP, false },

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
  static inline bool is_extension_compilable(const StrBlob &extension);
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
    if (string_tools::equal(extension.begin(), ExtensionToSourceTable[i].name))
    {
      return ExtensionToSourceTable[i].type;
    }
  }
  return SourceFileType::None;
}

inline bool SourceTools::is_extension_compilable(const StrBlob &extension) {
  for (size_t i = 0; i < std::size(ExtensionToSourceTable); i++)
  {
    if (string_tools::equal(extension.begin(), ExtensionToSourceTable[i].name))
    {
      return ExtensionToSourceTable[i].compilable;
    }
  }
  return false;
}
