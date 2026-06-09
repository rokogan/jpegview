#pragma once

#include "resource.h"

// Modal "Preferences" dialog that reads and writes common settings to the user INI file,
// so everyday options can be changed from a GUI instead of hand-editing the INI.
class CSettingsDlg : public CDialogImpl<CSettingsDlg>
{
public:
	enum { IDD = IDD_SETTINGS };

	BEGIN_MSG_MAP(CSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_SET_FILEASSOC, OnFileAssoc)
		COMMAND_ID_HANDLER(IDC_SET_EDITINI, OnEditIni)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFileAssoc(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEditIni(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};
