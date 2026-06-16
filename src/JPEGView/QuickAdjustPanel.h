#pragma once

#include "Panel.h"

class CImageProcessingParams;
class CHistogram;

// Minimalist "Quick adjust" card (Direction-B): a floating rounded panel shown top-right with a small
// live histogram and Brightness / Contrast / Saturation sliders bound to the live processing params.
// Toggled on demand from the navigation pill; reuses the same sliders as the full image-processing panel.
class CQuickAdjustPanel : public CPanel {
public:
	enum {
		ID_slBrightness,
		ID_slContrast,
		ID_slSaturation,
		ID_btnReset
	};

	CQuickAdjustPanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture, CImageProcessingParams* pParams);

	CButtonCtrl* GetBtnReset() { return GetControl<CButtonCtrl*>(ID_btnReset); }

	// The histogram to draw (set each frame by the controller); NULL hides the histogram area.
	void SetHistogram(const CHistogram* pHistogram) { m_pHistogram = pHistogram; }

	virtual CRect PanelRect();
	virtual void OnPaint(CDC & dc, const CPoint& offset);
	virtual void RequestRepositioning() { m_clientRect = CRect(0, 0, 0, 0); }

protected:
	virtual void RepositionAll();

private:
	static void PaintResetBtn(void* pContext, const CRect& rect, CDC& dc);
	void PaintHistogram(CDC & dc, const CRect& rect);

	CImageProcessingParams* m_pParams;
	const CHistogram* m_pHistogram;
	CRect m_clientRect;
	int m_nWidth, m_nHeight;
	int m_nPad;
	int m_nHistogramHeight;
	int m_nRowHeight;
};
