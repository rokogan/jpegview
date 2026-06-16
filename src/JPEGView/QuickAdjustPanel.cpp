#include "StdAfx.h"
#include "QuickAdjustPanel.h"
#include "ProcessParams.h"
#include "HistogramCorr.h"
#include "Helpers.h"
#include "HelpersGUI.h"
#include "SettingsProvider.h"
#include "NLS.h"
#include <math.h>

#define QA_PAD 14
#define QA_WIDTH 300
#define QA_HISTOGRAM_HEIGHT 46
#define QA_ROW_HEIGHT 34
#define QA_RESET_SIZE 16

CQuickAdjustPanel::CQuickAdjustPanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture, CImageProcessingParams* pParams)
	: CPanel(hWnd, pNotifyMouseCapture) {
	m_pParams = pParams;
	m_pHistogram = NULL;
	m_clientRect = CRect(0, 0, 0, 0);
	m_nPad = (int)(QA_PAD * m_fDPIScale);
	m_nWidth = (int)(QA_WIDTH * m_fDPIScale);
	m_nHistogramHeight = (int)(QA_HISTOGRAM_HEIGHT * m_fDPIScale);
	m_nRowHeight = (int)(QA_ROW_HEIGHT * m_fDPIScale);
	m_nHeight = m_nPad + m_nHistogramHeight + m_nPad + 3 * m_nRowHeight + m_nPad;

	int nSliderLen = (int)(108 * m_fDPIScale);
	// Same bindings/ranges as the full image-processing panel, so both stay in sync.
	AddSlider(ID_slBrightness, CNLS::GetString(_T("Brightness")), &(pParams->Gamma), NULL, 0.5, 2.0, 1.0, true, true, true, nSliderLen);
	AddSlider(ID_slContrast, CNLS::GetString(_T("Contrast")), &(pParams->Contrast), NULL, -0.5, 0.5, 0.0, true, false, false, nSliderLen);
	AddSlider(ID_slSaturation, CNLS::GetString(_T("Saturation")), &(pParams->Saturation), NULL, 0.0, 2.0, 1.0, true, false, false, nSliderLen);
	AddUserPaintButton(ID_btnReset, CNLS::GetString(_T("Reset")), &PaintResetBtn);
}

CRect CQuickAdjustPanel::PanelRect() {
	CRect clientRect;
	::GetClientRect(m_hWnd, &clientRect);
	// Float on the right edge, vertically centred (clear of the top-left EXIF box and the bottom pill).
	int nLeft = clientRect.right - m_nWidth - m_nPad;
	int nTop = (clientRect.Height() - m_nHeight) / 2;
	if (nTop < m_nPad) nTop = m_nPad;
	m_clientRect = CRect(CPoint(nLeft, nTop), CSize(m_nWidth, m_nHeight));
	return m_clientRect;
}

void CQuickAdjustPanel::RepositionAll() {
	CRect r = PanelRect();
	int nContentLeft = r.left + m_nPad;
	int nContentRight = r.right - m_nPad;
	int nSliderLen = (int)(108 * m_fDPIScale);

	int nY = r.top + m_nPad + m_nHistogramHeight + m_nPad;
	ControlsIterator iter;
	for (iter = m_controls.begin(); iter != m_controls.end(); iter++) {
		CSliderDouble* pSlider = dynamic_cast<CSliderDouble*>(iter->second);
		if (pSlider != NULL) {
			pSlider->SetSliderLen(nSliderLen);
			pSlider->SetPosition(CRect(CPoint(nContentLeft, nY), CSize(nContentRight - nContentLeft, m_nRowHeight)));
			nY += m_nRowHeight;
		}
	}
	int nReset = (int)(QA_RESET_SIZE * m_fDPIScale);
	CButtonCtrl* pReset = GetBtnReset();
	if (pReset != NULL) {
		pReset->SetPosition(CRect(CPoint(nContentRight - nReset, r.top + m_nPad), CSize(nReset, nReset)));
	}
}

void CQuickAdjustPanel::PaintHistogram(CDC & dc, const CRect& rect) {
	if (m_pHistogram == NULL)
		return;
	const int* pGrey = m_pHistogram->GetChannelGrey();
	int nMax = 0;
	for (int i = 0; i < 256; i++) nMax = max(pGrey[i], nMax);
	if (nMax == 0)
		return;

	const int nBars = 32;
	double dScale = (rect.Height() - 2) / sqrt((double)nMax);
	int nBarGap = max(1, (int)(1 * m_fDPIScale));
	int nBarW = max(1, (rect.Width() - (nBars - 1) * nBarGap) / nBars);
	HBRUSH hBrush = ::CreateSolidBrush(RGB(120, 140, 180));
	int nX = rect.left;
	for (int b = 0; b < nBars; b++) {
		int nSum = 0, nCount = 0;
		int nStart = b * 256 / nBars, nEnd = (b + 1) * 256 / nBars;
		for (int i = nStart; i < nEnd; i++) { nSum += pGrey[i]; nCount++; }
		int nAvg = nCount ? nSum / nCount : 0;
		int nH = (int)(sqrt((double)nAvg) * dScale + 0.5);
		if (nH > 0) {
			CRect bar(nX, rect.bottom - nH, nX + nBarW, rect.bottom);
			::FillRect(dc, &bar, hBrush);
		}
		nX += nBarW + nBarGap;
	}
	::DeleteObject(hBrush);
}

void CQuickAdjustPanel::OnPaint(CDC & dc, const CPoint& offset) {
	CRect r = PanelRect();
	r.OffsetRect(offset);
	// Rounded "card" body. The dim-path tile already holds the image (DimFactor 0), so the corners
	// outside the RoundRect stay equal to the image - clean rounded card, no clip region.
	int nRadius = (int)(16 * m_fDPIScale);
	HBRUSH hBrush = ::CreateSolidBrush(RGB(28, 28, 30));
	HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(70, 70, 74));
	HBRUSH hOldBrush = dc.SelectBrush(hBrush);
	HPEN hOldPen = dc.SelectPen(hPen);
	dc.RoundRect(r.left, r.top, r.right, r.bottom, nRadius, nRadius);
	dc.SelectBrush(hOldBrush);
	dc.SelectPen(hOldPen);
	::DeleteObject(hBrush);
	::DeleteObject(hPen);

	CRect histRect(r.left + m_nPad, r.top + m_nPad, r.right - m_nPad, r.top + m_nPad + m_nHistogramHeight);
	PaintHistogram(dc, histRect);

	CPanel::OnPaint(dc, offset);
}

void CQuickAdjustPanel::PaintResetBtn(void* pContext, const CRect& rect, CDC& dc) {
	// Circular reset arrow.
	CRect r = Helpers::InflateRect(rect, 0.2f);
	dc.SelectStockBrush(HOLLOW_BRUSH);
	dc.Arc(r.left, r.top, r.right, r.bottom, r.right, r.top + r.Height() / 3, r.left, r.top + r.Height() / 3);
	int nAx = r.right - 1;
	int nAy = r.top + r.Height() / 3;
	int nA = max(2, (int)(3 * HelpersGUI::ScreenScaling));
	dc.MoveTo(nAx, nAy);
	dc.LineTo(nAx - nA, nAy - nA);
	dc.MoveTo(nAx, nAy);
	dc.LineTo(nAx + nA, nAy - nA);
}
