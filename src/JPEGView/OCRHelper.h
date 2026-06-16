// Optical character recognition via the Windows OCR engine (Windows.Media.Ocr)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// Thin wrapper around Windows.Media.Ocr. The implementation (OCRHelper.cpp) isolates all of the C++/WinRT
// dependency in a single translation unit and is compiled out for the legacy WINXP build.
namespace OCRHelper {

	// Runs Windows OCR over a tightly packed, top-down 32bpp BGRA image (the kind CJPEGImage::GetDIB returns)
	// and returns the recognized text in outText. Returns true on success (outText may still be empty if the
	// image has no text), false if OCR is unavailable (no installed language pack, pre-Windows-10) or fails.
	bool RecognizeToString(const void* pBGRADIB, int nWidth, int nHeight, std::wstring& outText);
}
