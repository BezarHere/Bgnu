#include "FileStats.hpp"

#ifdef __linux__
#include <sys/stat.h>
typedef timespec FILETIME;
#else
#include <Windows.h>
typedef DWORLD mode_t;
#endif

typedef std::chrono::microseconds microseconds;

static inline microseconds ToMs(const FILETIME &file_time);
static inline FileFlags OsAttrs2FileFlags(mode_t flags);

FileStats::FileStats(const FilePath &path) {
#ifdef __linux__
  struct stat file_stats = {0};
  const bool success = !stat(path.c_str(), &file_stats);
#else
  WIN32_FILE_ATTRIBUTE_DATA attributes = {};
  const bool success = GetFileAttributesExA(path.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &attributes);
#endif

	if (!success)
	{
		Logger::error("failed to get the file attributes for the path '%s'", path.c_str());
		return;
	}

#ifdef __linux__

  this->size = file_stats.st_size;

  this->last_access_time = ToMs(file_stats.st_atim);
  this->last_write_time = ToMs(file_stats.st_mtim);
  this->creation_time = ToMs(file_stats.st_ctim);

  

  this->flags = OsAttrs2FileFlags(file_stats.st_mode);

#else
	this->size =
		attributes.nFileSizeLow | (size_t(attributes.nFileSizeHigh) << (sizeof(attributes.nFileSizeHigh) * 8));

	this->last_access_time = ToMs(attributes.ftLastAccessTime);
	this->last_write_time = ToMs(attributes.ftLastWriteTime);
	this->creation_time = ToMs(attributes.ftCreationTime);

	this->flags = OsAttrs2FileFlags(attributes.dwFileAttributes);
#endif
}

inline microseconds ToMs(const FILETIME &file_time) {
#ifdef __linux__
  return microseconds(file_time.tv_sec + (file_time.tv_nsec / 1000000));
#else
	constexpr auto HighDateBits = sizeof(file_time.dwHighDateTime) * 8;
	return microseconds(file_time.dwLowDateTime | (int64_t(file_time.dwHighDateTime) << HighDateBits));
#endif
}

inline FileFlags OsAttrs2FileFlags(mode_t flags) {
  FileFlags result = 0;
#ifdef __linux__
  if (S_ISDIR(flags))
  {
    result |= eFileFlag_Directory;
  }
  if (S_ISCHR(flags) || S_ISBLK(flags))
  {
    result |= eFileFlag_Device;
  }

  if (S_ISFIFO(flags) | S_ISSOCK(flags))
  {
    result |= eFileFlag_Virtual;
  }
#else
	constexpr std::pair<FileFlags, mode_t> flags_conv[] = {
		{eFileFlag_Device, FILE_ATTRIBUTE_DEVICE},
		{eFileFlag_Directory, FILE_ATTRIBUTE_DIRECTORY},
		{eFileFlag_Hidden, FILE_ATTRIBUTE_HIDDEN},
		{eFileFlag_ReadOnly, FILE_ATTRIBUTE_READONLY},
		{eFileFlag_Virtual, FILE_ATTRIBUTE_VIRTUAL},
	};

  for (uint8_t i = 0; i < std::size(flags_conv); i++)
	{
		if (flags_conv[i].second && flags & flags_conv[i].second)
		{
			result |= flags_conv[i].first;
		}
	}
#endif


	

	return result;
}
