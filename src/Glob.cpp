#include "Glob.hpp"
#include "StringTools.hpp"
#include <iostream>

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

	inline Segment(SegmentType _type, const IndexRange &_range)
		: type{_type}, range{_range} {

	}

	inline Segment(SegmentType _type, const IndexRange &_range, const DataCharacters &_data_chars)
		: type{_type}, range{_range}, data_chars{_data_chars} {

	}

	inline static constexpr bool is_type_greed(SegmentType type) {
		return type == SegmentType::AnyName || type == SegmentType::AnyPath;
	}

	// will this segment read infinite character if it's allowed to
	inline bool is_greedy() const { return is_type_greed(type); }

	// can this segment match 0 character as a valid match
	inline bool can_be_empty() const { return is_greedy(); }

	// skips just make the next segment skip acceptable position
	// only for greedy segments, for they are the only segment the can fill the skip gap
	inline bool can_use_skip() const { return is_greedy(); }

	SegmentType type;
	IndexRange range;

	DataCharacters data_chars{};
};

static Glob::SegmentCollection copy_segments(const Glob::SegmentCollection &collection);
static Glob::Segment parse_char_selector(const StrBlob &blob);

Glob::Glob(const string &str)
	: m_source{str}, m_segments{parse({str.data(), str.length()})} {
}

Glob::~Glob() noexcept {
	_clear();
}

Glob::Glob(const Glob &copy)
	: m_source{copy.m_source},
	m_segments{copy_segments(copy.m_segments)} {
}

Glob &Glob::operator=(const Glob &copy) {

	m_source = copy.m_source;

	SegmentCollection segments_copy = copy_segments(copy.m_segments);

	_clear();
	m_segments = segments_copy;

	return *this;
}

bool Glob::test(const FilePath &path) const {
	return test(path.get_text());
}

bool Glob::test(const StrBlob &path) const {
	if (!is_valid())
	{
		return false;
	}

	const size_t len = run_match_all(0, path);

	if (len == 0)
	{
		return false;
	}

	// std::cout << "glob: " << path << " equals " << len << '\n';

	return len == path.length();
}


struct MatchFrame
{
	// can be removed & subsituted by the frame's index, but this is less troublesome
	size_t segment_index;
	size_t source_pos;
	size_t skip = 0;
};

size_t Glob::run_match_all(size_t start_index, const StrBlob &source) const {
	// genres limit, hopefully never reached
	constexpr size_t RunMatchAllLimit = 1024;

	vector<MatchFrame> frames{};
	frames.reserve(m_segments.size);

	// start segment, start of source
	frames.emplace_back(start_index, 0);

	size_t _limit = 0;

	// two conditions:
	// * one, to break processing on failure (no frames)
	// * two to break on success (all segments matched thus creating frames)
	while (!frames.empty() && frames.back().segment_index < m_segments.size)
	{

		// limit checks
		if (++_limit > RunMatchAllLimit)
		{
			throw std::length_error("run_match_all limit reached");

			// frames.clear();
			// break;
		}


		const auto &frame = frames.back();

		size_t next_match_len = 0;

		size_t match_len = run_match(
			frame.segment_index,
			source.slice(frame.source_pos),
			frame.skip,
			&next_match_len
		);

		// if this segment can match zero chars and the next segment (if calculated) matched something
		// *ex: '**/file.txt' matches "/file.txt" even tho '**' matched nothing, '/' matched so it's ok
		const bool valid_empty_match = \
			m_segments[frame.segment_index].can_be_empty() && next_match_len > 0;

		// segment matched or not matched but it's ok (for ex: greedy segments can match zero chars)
		if (match_len > 0 || valid_empty_match)
		{
			// we continue to the next frame
			frames.emplace_back(frame.segment_index + 1, frame.source_pos + match_len);
			continue;
		}

		// go back to the last skip-able segment and inc it's `skip`

		// ! local `frame` is now dangling ! 
		frames.pop_back();

		//* if no frame can be skiped then `source` can't be matched,
		//* and we will opt-out on the outer loop
		while (!frames.empty())
		{
			if (m_segments[frames.back().segment_index].can_use_skip())
			{
				frames.back().skip++;
				break;
			}

			// segment can't use 'skip', pop it
			frames.pop_back();
		}
	}

	// empty frames mean that the no way segments could match the source
	if (frames.empty())
	{
		return 0;
	}

	// returns the length matched
	return frames.back().source_pos;
}

size_t Glob::run_match(size_t index, const StrBlob &source, size_t skip, size_t *next_match) const {
	// segment at `index`
	const auto &seg = m_segments[index];

	// a greedy segments that is not the last one
	if (seg.is_greedy() && index < m_segments.size - 1)
	{
		// where and how much did the next non-greedy segment get?
		Match end_range = match_segment(index + 1, source, skip);

		if (next_match)
		{
			*next_match = end_range.length;
		}

		// greedy segment test length
		size_t greedy_len = test_segment(index, source.slice(0, end_range.position));

		// greedy segment read to the start of the next segment, making *no* gap
		if (end_range.position == greedy_len && (seg.can_be_empty() || greedy_len > 0)) {
			return greedy_len;
		}

		// there is a gap between the two segments, failing the segments
		// Unknown example, i dont know how to trigger this in a test
		return 0;
	}

	return test_segment(index, source);
}

Glob::Match Glob::match_segment(size_t index, const StrBlob &source, size_t skip) const {
	for (size_t i = 0; i < source.size; i++)
	{
		size_t len = test_segment(index, source.slice(i));
		if (len == 0)
		{
			continue;
		}

		if (skip)
		{
			skip--;
			continue;
		}

		return {len, i};
	}

	return {};
}

size_t Glob::test_segment(size_t index, const StrBlob &source) const {
	if (index >= m_segments.size)
	{
		//? should we throw an out of range?
		return false;
	}

	const auto &segment = m_segments[index];

	switch (segment.type)
	{
	case SegmentType::Text:
		{
			// can't possibly match, source too short
			if (source.size < segment.range.length())
			{
				return false;
			}

			for (size_t i = 0; i < segment.range.length(); i++)
			{
				if (m_source[segment.range.begin + i] != source[i])
				{
					// text mismatch at current char
					return 0;
				}
			}

			// text match
			return segment.range.length();
		}

	case SegmentType::DirectorySeparator:
		{
			size_t count = StringTools::count(source.data, source.size, StringTools::is_directory_separator);
			return count;
		}

	case SegmentType::SelectCharacters:
	case SegmentType::AvoidCharacters:
		{
			//* oh boy, this will be confusing after couple days 

			const auto &data_chars = segment.data_chars;
			const bool inverted = segment.type == SegmentType::AvoidCharacters;

			const auto proc = [&data_chars, inverted](const string_char character)
				{
					for (size_t i = 0; i < data_chars.count; i++)
					{
						// character match
						if (character == data_chars.chars[i])
						{

							return !inverted;
						}
					}

					// no character matched
					return inverted;
				};

			size_t selected_count = StringTools::count(source.data, source.size, proc);
			return selected_count;
		}

	case SegmentType::AnyCharacter:
		{
			// not enough chars to match all the '?'
			if (source.size < segment.range.length())
			{
				return 0;
			}

			size_t count = StringTools::count(
				source.data,
				std::min(source.size, segment.range.length()),
				// only continue if we found anything except directory separators ('?' can't match dir seps)
				inner::InvertOp(&StringTools::is_directory_separator)
			);

			if (count == segment.range.length())
			{
				return count;
			}

			return 0;
		}

	case SegmentType::AnyName:
		return Glob::test_any_name(source);
	case SegmentType::AnyPath:
		return Glob::test_any_path(source);

	default:
		return 0;
	}

	return false;
}

size_t Glob::find_last_non_greedy() const {
	size_t index = m_segments.size;

	// iterates from last to first
	while (index != 0)
	{
		index--;

		if (m_segments[index].is_greedy())
		{
			continue;
		}

		// segment at index is not greedy
		return index;
	}

	// all segments are greedy!
	return npos;
}

void Glob::_clear() {
	delete[] m_segments.data;
	m_segments.size = 0;
}


size_t Glob::test_any_name(const StrBlob &source) {
	for (size_t i = 0; i < source.size; i++)
	{
		if (StringTools::is_directory_separator(source[i]))
		{
			return i;
		}
	}

	return source.size;
}

size_t Glob::test_any_path(const StrBlob &source) {
	// match everything
	return source.size;
}

Glob::SegmentCollection Glob::parse(const StrBlob &blob) {
	vector<Segment> segments{};

	for (size_t i = 0; i < blob.size; i++)
	{
		if (StringTools::is_directory_separator(blob[i]))
		{
			size_t count = StringTools::count(&blob[i], blob.size - i, StringTools::is_directory_separator);
			i += count - 1;
			segments.emplace_back(SegmentType::DirectorySeparator, IndexRange(i, i + count));
			continue;
		}

		if (blob[i] == '?')
		{
			size_t count = StringTools::count(&blob[i], blob.size - i, inner::EqualTo('?'));
			i += count - 1;
			segments.emplace_back(SegmentType::AnyCharacter, IndexRange(i, i + count));
			continue;
		}

		if (blob[i] == '*')
		{
			size_t count = StringTools::count(
				&blob[i],
				// check either 1 or more stars in a row
				blob.size - i,
				inner::EqualTo('*')
			);

			i += count - 1;

			// 1 star -> any name
			// +2 stars -> any path (two greedy segments in a row might crash this impl of glob) 
			segments.emplace_back(
				count >= 2 ? SegmentType::AnyPath : SegmentType::AnyName,
				IndexRange(i, i + count)
			);
			continue;
		}

		if (blob[i] == '[')
		{
			segments.push_back(parse_char_selector(blob.slice(i)));
			IndexRange &range = segments.back().range;
			range = range.shifted(i);

			i += range.length() - 1;
			continue;
		}

		size_t text_length = StringTools::count(&blob[i], blob.size - i, inner::InvertOp(&is_char_reserved));
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

Glob::SegmentCollection copy_segments(const Glob::SegmentCollection &collection) {
	Glob::SegmentCollection segments;

	segments.size = collection.size;
	segments.data = new Glob::Segment[segments.size];

	for (size_t i = 0; i < segments.size; i++)
	{
		segments[i] = collection[i];
	}

	return segments;
}

Glob::Segment parse_char_selector(const StrBlob &blob) {
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

	// no closing ']' found
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
