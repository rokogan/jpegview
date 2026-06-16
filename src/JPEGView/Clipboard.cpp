#include "StdAfx.h"
#include "Clipboard.h"
#include "JPEGImage.h"
#include "Helpers.h"
#include "BasicProcessing.h"
#include "MaxImageDef.h"
#include <gdiplus.h>

static void CopyOriginalFileNameToClipboard(LPCWSTR filename) {
	int fileNameLength = (int)wcslen(filename);
	DWORD fileNameLengthBytes = sizeof(wchar_t) * (fileNameLength + 1);
	DROPFILES df = {sizeof(DROPFILES), {0, 0}, 0, TRUE};
	HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(DROPFILES) + fileNameLengthBytes + sizeof(wchar_t)); // for double NULL char
	if (hMem == NULL) return;
	int offset = sizeof(DROPFILES) / sizeof(wchar_t);
	WCHAR *pGlobal = (WCHAR *) ::GlobalLock(hMem);
	if (pGlobal == NULL) { ::GlobalFree(hMem); return; }
	::CopyMemory(pGlobal, &df, sizeof(DROPFILES));
	::CopyMemory(pGlobal + offset, filename, fileNameLengthBytes); // that's pGlobal + 20 bytes (the size of DROPFILES);
	pGlobal[offset + fileNameLength + 1] = 0; // add additional NULL character
	::GlobalUnlock(hMem);
	if (::SetClipboardData(CF_HDROP, hMem) == NULL) ::GlobalFree(hMem);
}

static void CopyFileNameTextToClipboard(LPCWSTR filename) {
	int fileNameLength = (int)wcslen(filename);
	DWORD fileNameLengthBytes = sizeof(wchar_t) * (fileNameLength + 1);
	HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, fileNameLengthBytes + sizeof(wchar_t)); // for double NULL char
	if (hMem == NULL) return;
	WCHAR* pGlobal = (WCHAR*) ::GlobalLock(hMem);
	if (pGlobal == NULL) { ::GlobalFree(hMem); return; }
	::CopyMemory(pGlobal, filename, fileNameLengthBytes);
	pGlobal[fileNameLength + 1] = 0; // add additional NULL character
	::GlobalUnlock(hMem);
	if (::SetClipboardData(CF_UNICODETEXT, hMem) == NULL) ::GlobalFree(hMem);
}

void CClipboard::CopyImageToClipboard(HWND hWnd, CJPEGImage * pImage, LPCTSTR fileName) {
	if (pImage == NULL || pImage->DIBPixelsLastProcessed(true) == NULL) {
		return;
	}

	pImage->EnableDimming(false);
	DoCopy(hWnd, pImage->DIBWidth(), pImage->DIBHeight(), pImage->DIBPixelsLastProcessed(true), pImage->IsClipboardImage() ? NULL : fileName);
	pImage->EnableDimming(true);
}

void CClipboard::CopyFullImageToClipboard(HWND hWnd, CJPEGImage * pImage, const CImageProcessingParams& procParams,
										  EProcessingFlags eFlags, LPCTSTR fileName) {
	CopyFullImageToClipboard(hWnd, pImage, procParams, eFlags, CRect(0, 0, pImage->OrigWidth(), pImage->OrigHeight()), fileName);
}

void CClipboard::CopyFullImageToClipboard(HWND hWnd, CJPEGImage * pImage, const CImageProcessingParams& procParams,
										  EProcessingFlags eFlags, CRect clipRect, LPCTSTR fileName) {
	if (pImage == NULL) {
		return;
	}

	clipRect.left = max(0, clipRect.left);
	clipRect.top = max(0, clipRect.top);
	clipRect.right = min(pImage->OrigWidth(), clipRect.right);
	clipRect.bottom = min(pImage->OrigHeight(), clipRect.bottom);

	pImage->EnableDimming(false);
	void* pDIB = pImage->GetDIB(pImage->OrigSize(), clipRect.Size(), clipRect.TopLeft(), procParams, eFlags);
	DoCopy(hWnd, clipRect.Width(), clipRect.Height(), pDIB, pImage->IsClipboardImage() ? NULL : fileName);
	pImage->EnableDimming(true);
}

void CClipboard::CopyPathToClipboard(HWND hWnd, CJPEGImage* pImage, LPCTSTR fileName) {
	if (pImage == NULL || pImage->DIBPixelsLastProcessed(true) == NULL) {
		return;
	}
	pImage->EnableDimming(false);
	DoCopyFileNameText(hWnd, fileName);
	pImage->EnableDimming(true);
}

CJPEGImage* CClipboard::PasteImageFromClipboard(HWND hWnd, const CImageProcessingParams& procParams, 
												EProcessingFlags eFlags) {
	if (!::OpenClipboard(hWnd)) {
		return NULL;
	}

	CJPEGImage* pImage = NULL;
	HANDLE handle = ::GetClipboardData(CF_DIB);
	if (handle != NULL) {
		BITMAPINFO* pbmInfo = (BITMAPINFO*)::GlobalLock(handle);
		if (pbmInfo != NULL) {
			SIZE_T nBlockSize = ::GlobalSize(handle);
			int nNumColors = pbmInfo->bmiHeader.biClrUsed;
			if (nNumColors == 0 && pbmInfo->bmiHeader.biBitCount <= 8) {
				nNumColors = 1 << pbmInfo->bmiHeader.biBitCount;
			}
			if (pbmInfo->bmiHeader.biCompression == BI_BITFIELDS) {
				nNumColors = 3;
			}
			// The CF_DIB block is untrusted (any process can set the clipboard). Validate every header
			// field that locates the pixel bits against the actual block size before dereferencing, so a
			// crafted biSize/biClrUsed cannot push pDIBBits (and GDI+'s reads) out of bounds.
			DWORD biSize = pbmInfo->bmiHeader.biSize;
			int biWidth = pbmInfo->bmiHeader.biWidth;
			int biHeight = abs(pbmInfo->bmiHeader.biHeight);
			int biBitCount = pbmInfo->bmiHeader.biBitCount;
			size_t nHeaderBytes = (size_t)biSize + (size_t)nNumColors * sizeof(RGBQUAD);
			size_t nRowBytes = (biBitCount > 0) ? (((size_t)biWidth * biBitCount + 31) / 32) * 4 : 0;
			size_t nPixelBytes = nRowBytes * (size_t)biHeight;
			char* pDIBBits = (char*)pbmInfo + nHeaderBytes;

			if (nNumColors >= 0 && biBitCount > 0 &&
				biSize >= sizeof(BITMAPINFOHEADER) && biSize <= nBlockSize &&
				biWidth >= 0 && biWidth <= MAX_IMAGE_DIMENSION && biHeight <= MAX_IMAGE_DIMENSION &&
				nHeaderBytes <= nBlockSize && nPixelBytes <= nBlockSize - nHeaderBytes) {
				Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(pbmInfo, pDIBBits);
				if (pBitmap->GetLastStatus() == Gdiplus::Ok) {
					Gdiplus::Rect bmRect(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
					Gdiplus::BitmapData bmData;
					if (pBitmap->LockBits(&bmRect, Gdiplus::ImageLockModeRead, PixelFormat32bppRGB, &bmData) == Gdiplus::Ok) {
						assert(bmData.PixelFormat == PixelFormat32bppRGB);
						void* pDIB = CBasicProcessing::ConvertGdiplus32bppRGB(bmRect.Width, bmRect.Height, bmData.Stride, bmData.Scan0);
						pImage = (pDIB == NULL) ? NULL : new CJPEGImage(bmRect.Width, bmRect.Height, pDIB, NULL, 4, 0, IF_CLIPBOARD, false, 0, 1, 0);
						pBitmap->UnlockBits(&bmData);
					}
				}
				delete pBitmap;
			}

			::GlobalUnlock(handle);
		}
	}

	::CloseClipboard();

	return pImage;
}


void CClipboard::DoCopy(HWND hWnd, int nWidth, int nHeight, const void* pSourceImageDIB32, LPCTSTR fileName) {
	if (!::OpenClipboard(hWnd)) {
		return;
	}
	::EmptyClipboard();

	// get needed size of memory block (64-bit math so the byte size cannot wrap for large images)
	size_t nPixelBytes;
	if (!Helpers::SafeImageByteSize(nWidth, nHeight, 3, true, nPixelBytes)) {
		::CloseClipboard();
		return;
	}
	size_t nSizeBytes = sizeof(BITMAPINFO) + nPixelBytes;

	// Allocate memory
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, nSizeBytes);
	if (hMem == NULL) {
		::CloseClipboard();
		return;
	}
	void* pMemory = ::GlobalLock(hMem);
	if (pMemory == NULL) { ::GlobalFree(hMem); ::CloseClipboard(); return; }

	BITMAPINFO* pBMInfo = (BITMAPINFO*) pMemory;
	memset(pBMInfo, 0, sizeof(BITMAPINFO));

	pBMInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBMInfo->bmiHeader.biWidth = nWidth;
	pBMInfo->bmiHeader.biHeight = nHeight;
	pBMInfo->bmiHeader.biPlanes = 1;
	pBMInfo->bmiHeader.biBitCount = 24;
	pBMInfo->bmiHeader.biCompression = BI_RGB;
	pBMInfo->bmiHeader.biXPelsPerMeter = 10000;
	pBMInfo->bmiHeader.biYPelsPerMeter = 10000;
	pBMInfo->bmiHeader.biClrUsed = 0;

	uint8* pDIBPixelsTarget = (uint8*)pMemory + sizeof(BITMAPINFO) - sizeof(RGBQUAD);
	CBasicProcessing::Convert32bppTo24bppDIB(nWidth, nHeight, pDIBPixelsTarget, pSourceImageDIB32, true);

	::GlobalUnlock(hMem);
	if (::SetClipboardData(CF_DIB, hMem) == NULL) ::GlobalFree(hMem);

	if (fileName != NULL) {
		CopyOriginalFileNameToClipboard(fileName);
	}

	::CloseClipboard();
}

void CClipboard::DoCopyFileNameText(HWND hWnd, LPCTSTR fileName) {
	if (!::OpenClipboard(hWnd)) {
		return;
	}
	::EmptyClipboard();

	CopyFileNameTextToClipboard(fileName);

	::CloseClipboard();
}