#pragma once
#include "base.hpp"
#include "Range.hpp"
#include "FilePath.hpp"

struct Glob
{
public:
	struct Segment;

	typedef Blob<Segment> SegmentCollection;

	Glob(const string &str);
	inline Glob(const string_char *cstr) : Glob{string{cstr}} {}

	~Glob() noexcept;

	Glob(const Glob &copy);
	Glob &operator=(const Glob &copy);

	inline bool is_valid() const {
		return m_segments.size > 0;
	}

	bool test(const FilePath &path) const;
	bool test(const StrBlob &path) const;

	inline bool test(const string &str) const { return test(StrBlob(str.data(), str.length())); }
	inline bool test(const string_char *cstr) const {
		return test(StrBlob(cstr, string::traits_type::length(cstr)));
	}

	inline const string &get_source() const {
		return m_source;
	}

	template <typename _T>
	inline bool operator()(_T &&value) const { return test(value); }

private:
	struct Match
	{
		size_t length;
		size_t position;
	};

	size_t run_match_all(size_t start_index, const StrBlob &source) const;
	size_t run_match(size_t index, const StrBlob &source,
									 size_t skip, size_t *next_match = nullptr) const;

	Match match_segment(size_t index, const StrBlob &source, size_t skip) const;
	size_t test_segment(size_t index, const StrBlob &source) const;

	size_t find_last_non_greedy() const;

	void _clear();

	static size_t test_any_name(const StrBlob &source);
	static size_t test_any_path(const StrBlob &source);


	static SegmentCollection parse(const StrBlob &blob);

	static inline constexpr bool is_char_reserved(string_char character) {
		return character == string_char('*') ||
			string_tools::is_directory_separator(character) ||
			character == string_char('[');
	}

private:
	string m_source;
	SegmentCollection m_segments;
};
