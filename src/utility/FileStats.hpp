#pragma once
#include <chrono>

#include "FilePath.hpp"

enum eFileFlags : uint16_t {
  eFileFlag_Hidden = 0x01,
  eFileFlag_ReadOnly = 0x02,
  eFileFlag_Device = 0x04,
  eFileFlag_Virtual = 0x08,

  eFileFlag_Directory = 0x100,
};

typedef std::underlying_type<eFileFlags>::type FileFlags;

struct FileStats
{
  FileStats(const FilePath &path);

  size_t size;  // bytes
  std::chrono::microseconds last_access_time;
  std::chrono::microseconds last_write_time;
  std::chrono::microseconds creation_time;

  FileFlags flags;
};
