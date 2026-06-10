// libFuzzer target for the TGA decoder (CReaderTGA).
//
// This file is NOT part of the MSVC solution. It is built separately with
// clang (see README.md).
//
// Real entry point fuzzed (src/JPEGView/ReaderTGA.{h,cpp}):
//   static CJPEGImage* CReaderTGA::ReadTgaImage(LPCTSTR strFileName,
//       COLORREF backgroundColor, bool& bOutOfMemory);
//
// NOTE: unlike the PNG/EXIF decoders, this reader takes a *file path*, not a
// memory buffer (it uses _tfopen/fread internally). So the harness writes the
// fuzz bytes to a temporary file and passes its path. On a successful decode
// it returns a heap CJPEGImage that must be `delete`d.
//
// Build caveat: this pulls in CJPEGImage and its transitive dependencies
// (Helpers, BasicProcessing, GDI+/WTL types). See README.md.

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "StdAfx.h"
#include "ReaderTGA.h"
#include "JPEGImage.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// Write the input to a temp file the decoder can open.
	TCHAR tmpDir[MAX_PATH];
	TCHAR tmpPath[MAX_PATH];
	if (GetTempPath(MAX_PATH, tmpDir) == 0)
		return 0;
	if (GetTempFileName(tmpDir, _T("fzt"), 0, tmpPath) == 0)
		return 0;

	FILE* f = _tfopen(tmpPath, _T("wb"));
	if (f == nullptr) {
		DeleteFile(tmpPath);
		return 0;
	}
	if (size > 0)
		fwrite(data, 1, size, f);
	fclose(f);

	bool bOutOfMemory = false;
	try {
		CJPEGImage* pImage = CReaderTGA::ReadTgaImage(tmpPath, 0 /*black bg*/, bOutOfMemory);
		delete pImage;
	} catch (...) {
		// The reader does not document throwing, but the CJPEGImage ctor and
		// downstream processing may; swallow to keep fuzzing.
	}

	DeleteFile(tmpPath);
	return 0;
}
