#pragma once

#include "PanelController.h"

class CQuickAdjustPanel;
class CImageProcessingParams;

// Controller for the minimalist "Quick adjust" card. The card is shown on demand (toggled from the
// navigation pill) and stays fully opaque while active - it uses the dim paint path with no dimming so
// its rounded corners blend to the image, without depending on the navigation pill's fade factor.
class CQuickAdjustPanelCtl : public CPanelController
{
public:
	CQuickAdjustPanelCtl(CMainDlg* pMainDlg, CImageProcessingParams* pParams);
	virtual ~CQuickAdjustPanelCtl();

	virtual float DimFactor() { return 0.0f; } // no dimming -> the tile keeps the image, card drawn opaque
	virtual bool IsVisible();
	virtual bool IsActive() { return IsVisible(); }

	virtual void SetVisible(bool bVisible);
	virtual void SetActive(bool bActive) {}

	virtual void OnPrePaintMainDlg(HDC hPaintDC); // feeds the live histogram to the panel before painting

	bool IsShown() const { return m_bVisible; }

private:
	bool m_bVisible;
	CQuickAdjustPanel* m_pQuickAdjustPanel;
	CImageProcessingParams* m_pParams;

	static void OnReset(void* pContext, int nParameter, CButtonCtrl & sender);
};
