#pragma once

// For usage in stdext::hash_map, hash and compare of type LPCTSTR
class CHashCompareLPCTSTR
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;
	CHashCompareLPCTSTR() {}
	size_t operator( )(const LPCTSTR& Key) const;
	bool operator( )(const LPCTSTR& _Key1, const LPCTSTR& _Key2) const;
};

// Equality predicate for usage as KeyEqual in std::unordered_map
struct CEqualLPCTSTR
{
	bool operator()(const LPCTSTR& a, const LPCTSTR& b) const { return _tcscmp(a, b) == 0; }
};