#include "stdafx.h"
#include "OCRHelper.h"

#ifndef WINXP

#include <thread>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>

// WindowsApp.lib is the umbrella import library that resolves the WinRT activation factories.
#pragma comment(lib, "windowsapp.lib")

// Classic-COM interface for getting the raw byte pointer behind an IMemoryBufferReference.
// (stdafx.h pulls in <unknwn.h> via ATL, so C++/WinRT is in COM-interop mode and as<>() finds this.)
struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) IMemoryBufferByteAccess : ::IUnknown {
	virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media::Ocr;

// Box-average downscale of a tightly packed, top-down 32bpp BGRA buffer into a freshly allocated one
// (caller deletes[]). Used to fit the image under OcrEngine::MaxImageDimension; alpha is set opaque.
static uint8_t* DownscaleBGRA(const uint8_t* pSrc, int srcW, int srcH, int dstW, int dstH) {
	uint8_t* pDst = new (std::nothrow) uint8_t[(size_t)dstW * dstH * 4];
	if (pDst == NULL)
		return NULL;
	for (int dy = 0; dy < dstH; dy++) {
		int sy0 = (int)((__int64)dy * srcH / dstH);
		int sy1 = (int)((__int64)(dy + 1) * srcH / dstH);
		if (sy1 <= sy0) sy1 = sy0 + 1;
		for (int dx = 0; dx < dstW; dx++) {
			int sx0 = (int)((__int64)dx * srcW / dstW);
			int sx1 = (int)((__int64)(dx + 1) * srcW / dstW);
			if (sx1 <= sx0) sx1 = sx0 + 1;
			unsigned int b = 0, g = 0, r = 0, n = 0;
			for (int sy = sy0; sy < sy1; sy++) {
				const uint8_t* pRow = pSrc + ((size_t)sy * srcW + sx0) * 4;
				for (int sx = sx0; sx < sx1; sx++) {
					b += pRow[0]; g += pRow[1]; r += pRow[2];
					pRow += 4; n++;
				}
			}
			uint8_t* pPix = pDst + ((size_t)dy * dstW + dx) * 4;
			pPix[0] = (uint8_t)(b / n); pPix[1] = (uint8_t)(g / n); pPix[2] = (uint8_t)(r / n); pPix[3] = 255;
		}
	}
	return pDst;
}

// Runs on a worker thread inside a multi-threaded apartment so the blocking .get() below can't deadlock.
static bool RecognizeOnThisThread(const void* pBGRADIB, int nWidth, int nHeight, std::wstring& outText) {
	OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
	if (engine == nullptr)
		return false; // no installed OCR language pack

	// The OCR engine rejects images larger than MaxImageDimension; downscale to fit if necessary.
	const uint8_t* pPixels = (const uint8_t*)pBGRADIB;
	int w = nWidth, h = nHeight;
	uint8_t* pScaled = NULL;
	uint32_t nMaxDim = engine.MaxImageDimension();
	if (nMaxDim > 0 && ((uint32_t)w > nMaxDim || (uint32_t)h > nMaxDim)) {
		double dScale = (double)nMaxDim / max(w, h);
		int dw = max(1, (int)(w * dScale));
		int dh = max(1, (int)(h * dScale));
		pScaled = DownscaleBGRA(pPixels, w, h, dw, dh);
		if (pScaled == NULL)
			return false;
		pPixels = pScaled;
		w = dw; h = dh;
	}

	bool bResult = true;
	try {
		SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, w, h, BitmapAlphaMode::Premultiplied);
		{
			BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
			IMemoryBufferReference reference = buffer.CreateReference();
			auto byteAccess = reference.as<IMemoryBufferByteAccess>();
			uint8_t* pDest = NULL;
			uint32_t nCapacity = 0;
			bool bGotBuffer = SUCCEEDED(byteAccess->GetBuffer(&pDest, &nCapacity)) && pDest != NULL;
			if (bGotBuffer) {
				BitmapPlaneDescription desc = buffer.GetPlaneDescription(0);
				size_t nSrcStride = (size_t)w * 4; // tightly packed top-down BGRA
				for (int y = 0; y < h; y++) {
					memcpy(pDest + desc.StartIndex + (size_t)y * desc.Stride,
						pPixels + (size_t)y * nSrcStride, nSrcStride);
				}
			}
			reference.Close();
			buffer.Close();
			if (!bGotBuffer)
				bResult = false;
		}

		if (bResult) {
			OcrResult result = engine.RecognizeAsync(bitmap).get();
			if (result != nullptr)
				outText = result.Text().c_str();
		}
	} catch (...) {
		bResult = false;
	}

	delete[] pScaled;
	return bResult; // engine ran; outText may be empty if no text was found
}

namespace OCRHelper {

	bool RecognizeToString(const void* pBGRADIB, int nWidth, int nHeight, std::wstring& outText) {
		if (pBGRADIB == NULL || nWidth <= 0 || nHeight <= 0)
			return false;

		bool bResult = false;
		std::thread worker([&]() {
			winrt::init_apartment(winrt::apartment_type::multi_threaded);
			try {
				bResult = RecognizeOnThisThread(pBGRADIB, nWidth, nHeight, outText);
			} catch (...) {
				bResult = false;
			}
			winrt::uninit_apartment();
		});
		worker.join();
		return bResult;
	}
}

#else

namespace OCRHelper {
	bool RecognizeToString(const void* /* pBGRADIB */, int /* nWidth */, int /* nHeight */, std::wstring& /* outText */) {
		return false;
	}
}

#endif
