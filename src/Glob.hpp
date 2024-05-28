#pragma once
#include "base.hpp"
#include "Range.hpp"

struct Glob
{
public:
	struct Segment;

	typedef Blob<Segment> SegmentCollection;

	Glob(const string &str);

private:
	static SegmentCollection parse(const StrBlob &blob);

private:
	string m_source;
	SegmentCollection m_segments;
};
