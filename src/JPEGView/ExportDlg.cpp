#include "StdAfx.h"
#include "ExportDlg.h"
#include "NLS.h"
#include "HelpersGUI.h"

// Output formats offered by the export dialog. usesQuality => the JPEG/WebP-lossy quality value applies.
struct ExportFormat { LPCTSTR sLabel; LPCTSTR sExt; bool bLosslessWebP; bool bUsesQuality; };

static const ExportFormat kExportFormats[] = {
	{ _T("JPEG (*.jpg)"),            _T("jpg"),  false, true  },
	{ _T("PNG (*.png)"),             _T("png"),  false, false },
	{ _T("WebP (*.webp)"),           _T("webp"), false, true  },
	{ _T("WebP lossless (*.webp)"),  _T("webp"), true,  false },
	{ _T("BMP (*.bmp)"),             _T("bmp"),  false, false },
	{ _T("TIFF (*.tiff)"),           _T("tiff"), false, false },
	{ _T("QOI (*.qoi)"),             _T("qoi"),  false, false },
};

int CExportDlg::sm_nSelectedFormat = 0;   // JPEG
int CExportDlg::sm_nQuality = 90;
bool CExportDlg::sm_bFullSize = true;

CExportDlg::CExportDlg() {
	m_bLosslessWebP = false;
	m_bFullSize = true;
	m_nQuality = -1;
}

LRESULT CExportDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	SetWindowText(CNLS::GetString(_T("Export image")));
	m_cbFormat.Attach(GetDlgItem(IDC_EXP_FORMAT));
	m_edtQuality.Attach(GetDlgItem(IDC_EXP_QUALITY));
	m_chkFullSize.Attach(GetDlgItem(IDC_EXP_FULLSIZE));

	GetDlgItem(IDC_EXP_LBL_FORMAT).SetWindowText(CNLS::GetString(_T("Format:")));
	GetDlgItem(IDC_EXP_LBL_QUALITY).SetWindowText(CNLS::GetString(_T("Quality:")));
	m_chkFullSize.SetWindowText(CNLS::GetString(_T("Export at full resolution")));
	GetDlgItem(IDOK).SetWindowText(CNLS::GetString(_T("Export")));
	GetDlgItem(IDCANCEL).SetWindowText(CNLS::GetString(_T("Cancel")));

	for (int i = 0; i < sizeof(kExportFormats) / sizeof(kExportFormats[0]); i++) {
		m_cbFormat.AddString(kExportFormats[i].sLabel);
	}
	m_cbFormat.SetCurSel(sm_nSelectedFormat);
	m_edtQuality.LimitText(3);
	TCHAR buff[8];
	_stprintf_s(buff, 8, _T("%d"), sm_nQuality);
	m_edtQuality.SetWindowText(buff);
	m_chkFullSize.SetCheck(sm_bFullSize ? BST_CHECKED : BST_UNCHECKED);
	UpdateQualityEnabled();

	HelpersGUI::ApplyModernWindowChrome(m_hWnd);
	return TRUE;
}

void CExportDlg::UpdateQualityEnabled() {
	int nSel = m_cbFormat.GetCurSel();
	bool bUsesQuality = (nSel >= 0) && kExportFormats[nSel].bUsesQuality;
	m_edtQuality.EnableWindow(bUsesQuality);
	GetDlgItem(IDC_EXP_LBL_QUALITY).EnableWindow(bUsesQuality);
}

LRESULT CExportDlg::OnFormatChanged(WORD, WORD, HWND, BOOL&) {
	UpdateQualityEnabled();
	return 0;
}

LRESULT CExportDlg::OnOK(WORD, WORD, HWND, BOOL&) {
	int nSel = m_cbFormat.GetCurSel();
	if (nSel == CB_ERR) nSel = 0;
	m_sExtension = kExportFormats[nSel].sExt;
	m_bLosslessWebP = kExportFormats[nSel].bLosslessWebP;
	m_bFullSize = m_chkFullSize.GetCheck() == BST_CHECKED;

	if (kExportFormats[nSel].bUsesQuality) {
		TCHAR buff[8];
		m_edtQuality.GetWindowText(buff, 8);
		m_nQuality = _ttoi(buff);
		m_nQuality = max(1, min(100, m_nQuality));
		sm_nQuality = m_nQuality;
	} else {
		m_nQuality = -1;
	}
	sm_nSelectedFormat = nSel;
	sm_bFullSize = m_bFullSize;

	EndDialog(IDOK);
	return 0;
}

LRESULT CExportDlg::OnCancel(WORD, WORD, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}
