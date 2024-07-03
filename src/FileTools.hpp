#pragma once
#include "FilePath.hpp"
#include "misc/Buffer.hpp"
#include <fstream>

namespace FileTools
{
	// 128 MEGABYTES
	static constexpr std::streamsize MaxReadSize = 0x8000000;
	typedef std::ios::openmode openmode;

	enum class FileKind : uint8_t
	{
		Binary,
		Text,
	};

	static inline openmode _get_open_mode(const FileKind kind) {
		if (kind == FileKind::Binary)
		{
			return openmode::_S_bin;
		}

		return openmode(0);
	}
	static inline std::streamsize _get_unit_size(const FileKind kind) {
		if (kind == FileKind::Binary)
		{
			return 1;
		}

		return sizeof(string_char);
	}

	static inline size_t read_to(const FilePath &filepath, const MutableStrBlob &output,
															 FileKind kind = FileKind::Text) {

		std::ifstream input{filepath.c_str(), openmode::_S_in | _get_open_mode(kind)};

		if (!input.good())
		{
			return npos;
		}

		// -1 length for text files
		input.read(output.data, std::streamsize(output.size) - (kind == FileKind::Binary ? 0 : 1));

		// TODO: get read size
		return 1;
	}

	static inline Buffer read_all(const FilePath &filepath, FileKind kind = FileKind::Text) {
		std::ifstream input{filepath.c_str(), openmode::_S_in | _get_open_mode(kind)};

		if (!input.good())
		{
			return nullptr;
		}

		const auto begin = input.tellg();
		input.seekg(0, std::ios::end);
		const auto end = input.tellg();
		input.seekg(0, std::ios::beg);

		const std::streampos kind_offset = (kind == FileKind::Binary) ? 0 : 1;
		const std::streamsize size = std::min(MaxReadSize, end - begin);

		Buffer buffer{(size + kind_offset) * _get_unit_size(kind)};
		memset(buffer, 0, buffer.size());

		input.read(
			reinterpret_cast<decltype(input)::char_type *>(buffer.get()),
			buffer.size() - kind_offset // to keep null termination
		);
		return buffer;
	}

	static inline string read_str(const FilePath &filepath) {
		std::ifstream file{filepath};
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

}
