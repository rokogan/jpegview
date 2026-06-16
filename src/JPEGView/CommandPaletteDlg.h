// Command palette dialog
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// A "command palette": a modal dialog with a search box and a result list. Typing filters the list of
// JPEGView commands (substring match, case-insensitive); Up/Down move the selection, Enter or double-click
// confirms. When DoModal() returns IDOK, GetChosenCommandId() holds the selected IDM_* command id, which the
// caller re-dispatches through CMainDlg::ExecuteCommand().
class CCommandPaletteDlg : public CDialogImpl<CCommandPaletteDlg>
{
public:
	enum { IDD = IDD_COMMANDPALETTE };

	CCommandPaletteDlg();
	~CCommandPaletteDlg();

	BEGIN_MSG_MAP(CCommandPaletteDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorChild)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorChild)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorChild)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_CP_FILTER, EN_CHANGE, OnFilterChanged)
		COMMAND_HANDLER(IDC_CP_LIST, LBN_DBLCLK, OnListDblClk)
	ALT_MSG_MAP(1) // messages of the subclassed search edit box
		MESSAGE_HANDLER(WM_KEYDOWN, OnFilterKeyDown)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnCtlColorDlg(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnCtlColorChild(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnOK(WORD, WORD, HWND, BOOL&);
	LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
	LRESULT OnFilterChanged(WORD, WORD, HWND, BOOL&);
	LRESULT OnListDblClk(WORD, WORD, HWND, BOOL&);
	LRESULT OnFilterKeyDown(UINT, WPARAM, LPARAM, BOOL&);

	// Valid only after DoModal() returned IDOK.
	int GetChosenCommandId() const { return m_nChosenCmd; }

private:
	CContainedWindowT<CEdit> m_edtFilter;
	CListBox m_lstResults;
	HBRUSH m_hbrBack;
	int m_nChosenCmd;

	void RefilterList();
	void AcceptSelection();
};
