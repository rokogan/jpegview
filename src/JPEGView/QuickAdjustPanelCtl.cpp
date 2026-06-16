#include "StdAfx.h"
#include "resource.h"
#include "MainDlg.h"
#include "QuickAdjustPanelCtl.h"
#include "QuickAdjustPanel.h"
#include "JPEGImage.h"
#include "ProcessParams.h"

CQuickAdjustPanelCtl::CQuickAdjustPanelCtl(CMainDlg* pMainDlg, CImageProcessingParams* pParams) : CPanelController(pMainDlg, false) {
	m_bVisible = false;
	m_pParams = pParams;
	m_pPanel = m_pQuickAdjustPanel = new CQuickAdjustPanel(pMainDlg->GetHWND(), this, pParams);
	m_pQuickAdjustPanel->GetBtnReset()->SetButtonPressedHandler(&OnReset, this);
}

CQuickAdjustPanelCtl::~CQuickAdjustPanelCtl() {
	delete m_pQuickAdjustPanel;
	m_pQuickAdjustPanel = NULL;
}

bool CQuickAdjustPanelCtl::IsVisible() {
	return m_bVisible && CurrentImage() != NULL;
}

void CQuickAdjustPanelCtl::SetVisible(bool bVisible) {
	if (m_bVisible != bVisible) {
		m_bVisible = bVisible;
		InvalidateMainDlg();
	}
}

void CQuickAdjustPanelCtl::OnPrePaintMainDlg(HDC hPaintDC) {
	CJPEGImage* pImage = CurrentImage();
	m_pQuickAdjustPanel->SetHistogram((pImage != NULL) ? pImage->GetProcessedHistogram() : NULL);
}

void CQuickAdjustPanelCtl::OnReset(void* pContext, int nParameter, CButtonCtrl & sender) {
	CQuickAdjustPanelCtl* pThis = (CQuickAdjustPanelCtl*)pContext;
	pThis->m_pParams->Gamma = 1.0;
	pThis->m_pParams->Contrast = 0.0;
	pThis->m_pParams->Saturation = 1.0;
	pThis->InvalidateMainDlg();
}
