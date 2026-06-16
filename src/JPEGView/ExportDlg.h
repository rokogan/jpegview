// Export image dialog (format + quality + size)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// Small dialog that collects export options - output format, JPEG/WebP quality, and whether to export at
// full resolution or the size shown on screen. The caller then runs the normal Save-As flow with these
// options. Resampling to a different size stays the job of the Change Size dialog; this only chooses format
// and quality for a saved copy without altering the working image.
class CExportDlg : public CDialogImpl<CExportDlg>
{
public:
	enum { IDD = IDD_EXPORT };

	CExportDlg();

	BEGIN_MSG_MAP(CExportDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_EXP_FORMAT, CBN_SELCHANGE, OnFormatChanged)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnOK(WORD, WORD, HWND, BOOL&);
	LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
	LRESULT OnFormatChanged(WORD, WORD, HWND, BOOL&);

	// All valid only after DoModal() returned IDOK.
	LPCTSTR GetExtension() const { return m_sExtension; } // e.g. "jpg" - used as the Save-As default extension
	bool IsLosslessWebP() const { return m_bLosslessWebP; }
	bool IsFullSize() const { return m_bFullSize; }
	int GetQuality() const { return m_nQuality; }         // 0..100, or -1 when the format ignores quality

private:
	CComboBox m_cbFormat;
	CEdit m_edtQuality;
	CButton m_chkFullSize;

	CString m_sExtension;
	bool m_bLosslessWebP;
	bool m_bFullSize;
	int m_nQuality;

	void UpdateQualityEnabled();

	// Remember the last choice within the session.
	static int sm_nSelectedFormat;
	static int sm_nQuality;
	static bool sm_bFullSize;
};
