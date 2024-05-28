#include "Glob.hpp"
#include "FilePath.hpp"
#include "StringUtils.hpp"

constexpr size_t MaxSegmentCharacters = 64;

enum class SegmentType : uint8_t
{
	Invalid = 0,
	Text,
	DirectorySeparator,

	SelectCharacters, // select chars '[CcBb]all.json'
	AvoidCharacters, // select any other chars 'non-vowel_[!aeuioy].doc'

	AnyCharacter, // '?'
	AnyName, // star, selecting anything except directory separators
	AnyPath, // double star, selects any amount of path segments ('[seg.1]/[seg.2]/[seg.3]')
};

struct Glob::Segment
{
	struct DataCharacters
	{
		size_t count;
		std::array<string::value_type, MaxSegmentCharacters> chars;
	};

	inline Segment(SegmentType _type, const IndexRange &_range, const DataCharacters &_data_chars)
		: type{_type}, range{_range}, data_chars{_data_chars} {

	}

	SegmentType type;
	IndexRange range;

	DataCharacters data_chars;
};


Glob::Glob(const string &str) : m_source{str}, m_segments{parse(str)} {
}

Glob::SegmentCollection Glob::parse(const StrBlob &blob) {
	string ss = "3123";
	for (size_t i = 0; i < blob.size; i++)
	{
		if (FilePath::is_directory_separator(blob[i]))
		{
			size_t count = StringUtils::count(&blob[i], blob.size - i, FilePath::is_directory_separator);
		}
	}

	return nullptr;
}
