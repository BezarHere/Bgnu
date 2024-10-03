#include "FileStats.hpp"
#include <Windows.h>

typedef std::chrono::microseconds microseconds;

static inline microseconds ToMs(const FILETIME &file_time);
static inline FileFlags WinAttrs2FileFlags(DWORD flags);

FileStats::FileStats(const FilePath &path) {
	WIN32_FILE_ATTRIBUTE_DATA attributes = {};
	const bool success = GetFileAttributesExA(path.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &attributes);

	if (!success)
	{
		Logger::error("failed to get the file attributes for the path '%s'", path.c_str());
		return;
	}

	this->size =
		attributes.nFileSizeLow | (size_t(attributes.nFileSizeHigh) << (sizeof(attributes.nFileSizeHigh) * 8));

	this->last_access_time = ToMs(attributes.ftLastAccessTime);
	this->last_write_time = ToMs(attributes.ftLastWriteTime);
	this->creation_time = ToMs(attributes.ftCreationTime);

	this->flags = WinAttrs2FileFlags(attributes.dwFileAttributes);
}

inline microseconds ToMs(const FILETIME &file_time) {
	constexpr auto HighDateBits = sizeof(file_time.dwHighDateTime) * 8;
	return microseconds(file_time.dwLowDateTime | (int64_t(file_time.dwHighDateTime) << HighDateBits));
}

inline FileFlags WinAttrs2FileFlags(DWORD flags) {
	constexpr std::pair<FileFlags, DWORD> flags_conv[] = {
		{eFileFlag_Device, FILE_ATTRIBUTE_DEVICE},
		{eFileFlag_SystemFile, FILE_ATTRIBUTE_SYSTEM},
		{eFileFlag_Directory, FILE_ATTRIBUTE_DIRECTORY},
		{eFileFlag_Encrypted, FILE_ATTRIBUTE_ENCRYPTED},
		{eFileFlag_Hidden, FILE_ATTRIBUTE_HIDDEN},
		{eFileFlag_Offline, FILE_ATTRIBUTE_OFFLINE},
		{eFileFlag_ReadOnly, FILE_ATTRIBUTE_READONLY},
		{eFileFlag_Temporay, FILE_ATTRIBUTE_TEMPORARY},
		{eFileFlag_Virtual, FILE_ATTRIBUTE_VIRTUAL},
	};
	FileFlags result = 0;

	for (uint8_t i = 0; i < std::size(flags_conv); i++)
	{
		if (flags ^ flags_conv[i].second)
		{
			result |= flags_conv[i].first;
		}
	}

	return result;
}
