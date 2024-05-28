#include "Glob.hpp"
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
		size_t count = 0;
		std::array<string::value_type, MaxSegmentCharacters> chars;
	};

	inline Segment() = default;

	inline Segment(SegmentType _type, const IndexRange &_range, const DataCharacters &_data_chars = {})
		: type{_type}, range{_range}, data_chars{_data_chars} {

	}

	SegmentType type;
	IndexRange range;

	DataCharacters data_chars;
};

static Glob::Segment parse_char_selector(const StrBlob &blob) {
	size_t closure_counter = 0;
	size_t end = blob.size;

	for (size_t i = 0; i < blob.size; i++)
	{
		if (blob[i] == '[')
		{
			closure_counter++;
			continue;
		}

		if (blob[i] == ']')
		{
			if (closure_counter == 0) {
				end = i;
				break;
			}

			closure_counter--;
			continue;
		}
	}

	if (end == blob.size)
	{
		//? should this print out an error?
	}

	size_t start = 1;

	const bool inverted = blob[1] == '!';

	if (inverted)
	{
		start++;
	}

	// after '[' and before ']'
	const IndexRange chars_range{start, end};

	typedef Glob::Segment::DataCharacters DataChars;

	DataChars data_chars{};

	for (size_t i = chars_range.begin; i < chars_range.end; i++)
	{
		// reached max characters count
		if (data_chars.count >= MaxSegmentCharacters)
		{
			break;
		}

		const string::value_type character = blob[i];

		bool found = false;

		// checking if the character is a duplicate
		for (size_t j = 0; j < data_chars.count; j++)
		{
			if (data_chars.chars[j] == character)
			{
				found = true;
				break;
			}
		}

		// duplicate character
		if (found)
		{
			continue;
		}

		data_chars.chars[data_chars.count++] = character;
	}

	return {
		inverted ? SegmentType::AvoidCharacters : SegmentType::SelectCharacters,
		{0, end},
		data_chars
	};
}

Glob::Glob(const string &str) : m_source{str}, m_segments{parse(str)} {
}

Glob::SegmentCollection Glob::parse(const StrBlob &blob) {
	vector<Segment> segments{};

	for (size_t i = 0; i < blob.size; i++)
	{
		if (FilePath::is_directory_separator(blob[i]))
		{
			size_t count = StringUtils::count(&blob[i], blob.size - i, FilePath::is_directory_separator);
			i += count - 1;
			segments.emplace_back(SegmentType::DirectorySeparator, IndexRange(i, i + count));
			continue;
		}

		if (blob[i] == '?')
		{
			size_t count = StringUtils::count(&blob[i], blob.size - i, inner::EqualTo('?'));
			i += count - 1;
			segments.emplace_back(SegmentType::AnyCharacter, IndexRange(i, i + count));
			continue;
		}

		if (blob[i] == '*')
		{
			size_t count = StringUtils::count(
				&blob[i],
				// check either 1 or two stars in a row
				std::min<size_t>(blob.size - i, 2),
				inner::EqualTo('*')
			);

			i += count - 1;

			segments.emplace_back(
				count == 2 ? SegmentType::AnyPath : SegmentType::AnyName,
				IndexRange(i, i + count)
			);
			continue;
		}

		if (blob[i] == '[')
		{
			segments.push_back(parse_char_selector(blob.slice(i)));
			IndexRange &range = segments.back().range;
			range = range.shifted(i);
			continue;
		}

		size_t text_length = StringUtils::count(&blob[i], blob.size - i, is_char_reserved);
		segments.emplace_back(SegmentType::Text, IndexRange(i, i + text_length));
		i += text_length - 1;
	}

	// TODO: find a better abroach
	Segment *seg = new Segment[segments.size()];
	for (size_t i = 0; i < segments.size(); i++)
	{
		seg[i] = segments[i];
	}

	return {seg, segments.size()};
}
