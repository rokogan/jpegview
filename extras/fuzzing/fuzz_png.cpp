// libFuzzer target for the libpng-backed PNG decoder (PngReader).
//
// This file is NOT part of the MSVC solution. It is built separately with
// clang (see README.md) and exists purely to fuzz the untrusted-input
// decoder entry point with ASan + libFuzzer.
//
// Real entry point fuzzed (src/JPEGView/PNGWrapper.{h,cpp}):
//   static void* PngReader::ReadImage(int& width, int& height, int& bpp,
//       bool& has_animation, int& frame_count, int& frame_time,
//       void*& exif_chunk, bool& outOfMemory, void* buffer, size_t sizebytes);
//   static void* PngReader::GetEXIFBlock(void* buffer, size_t sizebytes);
//
// Ownership notes (matching ImageLoadThread.cpp usage):
//   - ReadImage returns a malloc'd pixel buffer  -> free() it.
//   - exif_chunk is malloc'd                      -> free() it.
//   - GetEXIFBlock returns a malloc'd block       -> free() it.

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <stdexcept>

#include "PNGWrapper.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// PngReader::ReadImage trims the first 8 bytes (PNG signature) internally
	// and reads from the supplied buffer, so feed the raw bytes directly.
	// It can modify the buffer / requires a non-const, writable copy.
	std::vector<uint8_t> buf(data, data + size);

	int width = 0, height = 0, bpp = 0;
	bool has_animation = false;
	int frame_count = 0;
	int frame_time = 0;
	void* exif_chunk = nullptr;
	bool outOfMemory = false;

	try {
		void* pixels = PngReader::ReadImage(width, height, bpp, has_animation,
			frame_count, frame_time, exif_chunk, outOfMemory,
			buf.data(), buf.size());
		// ReadImage returns a malloc'd buffer; the EXIF chunk is malloc'd too.
		free(pixels);
		free(exif_chunk);
		// Animated PNGs keep an internal libpng cache alive across frames;
		// drop it so each input starts clean and we don't leak between runs.
		PngReader::DeleteCache();
	} catch (...) {
		// The decoder throws std::runtime_error on malformed input (setjmp path).
		PngReader::DeleteCache();
	}

	// Also exercise the standalone EXIF-chunk extractor path.
	try {
		void* exif = PngReader::GetEXIFBlock(buf.data(), buf.size());
		free(exif);
	} catch (...) {
	}

	return 0;
}
