// libFuzzer target for the EXIF (TIFF/APP1) parser (CEXIFReader).
//
// This file is NOT part of the MSVC solution. It is built separately with
// clang (see README.md).
//
// Real entry point fuzzed (src/JPEGView/EXIFReader.{h,cpp}):
//   CEXIFReader::CEXIFReader(void* pApp1Block, EImageFormat eImageFormat);
//
// The constructor does all the parsing work. It expects pApp1Block to point
// at a JPEG APP1 marker (0xFF 0xE1) followed by the EXIF/TIFF payload, and it
// reads a size field from bytes [2..3]. It does NOT take ownership of the
// memory and allocates a couple of small GPSCoordinate objects that the
// destructor frees, so construct + destruct is the full cycle to fuzz.

#include <cstdint>
#include <cstddef>
#include <vector>

#include "StdAfx.h"
#include "ImageProcessingTypes.h"
#include "EXIFReader.h"

static void RunOne(uint8_t* p, size_t /*size*/)
{
	try {
		// The image format only affects whether the orientation tag is read;
		// IF_JPEG is the normal path for an APP1 EXIF block.
		CEXIFReader reader(p, IF_JPEG);
		(void)reader.GetImageOrientation();
	} catch (...) {
		// EXIFReader.cpp guards several spots with try/catch already, but be
		// defensive in case any malformed input escapes those.
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// The constructor dereferences at least the first 4 bytes unconditionally
	// (marker + size), so require a small minimum and a writable copy
	// (WriteImageOrientation paths can mutate the block).
	if (size < 4)
		return 0;

	// Pass 1: feed the raw bytes as-is. Reaches the marker check; if the
	// corpus already contains real APP1 blocks this drives the deep parser.
	{
		std::vector<uint8_t> buf(data, data + size);
		RunOne(buf.data(), buf.size());
	}

	// Pass 2: synthesize a valid APP1 marker + length header in front of the
	// fuzz bytes so the mutator can explore the TIFF/IFD parsing without first
	// having to guess the 0xFF 0xE1 prefix and the big-endian size word.
	{
		std::vector<uint8_t> buf;
		buf.reserve(size + 4);
		uint16_t app1Len = (uint16_t)((size + 2 > 0xFFFF) ? 0xFFFF : size + 2);
		buf.push_back(0xFF);
		buf.push_back(0xE1);
		buf.push_back((uint8_t)(app1Len >> 8));   // big-endian size, high byte
		buf.push_back((uint8_t)(app1Len & 0xFF)); // big-endian size, low byte
		buf.insert(buf.end(), data, data + size);
		RunOne(buf.data(), buf.size());
	}

	return 0;
}
