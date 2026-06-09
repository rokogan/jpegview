#include "StdAfx.h"
#include "resource.h"
#include "SettingsDlg.h"
#include "SettingsProvider.h"
#include "HelpersGUI.h"
#include "FileExtensionsDlg.h"

static int ComboSel(HWND hCombo) {
	return (int)::SendMessage(hCombo, CB_GETCURSEL, 0, 0);
}

static void ComboAdd(HWND hCombo, LPCTSTR sText) {
	::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)sText);
}

LRESULT CSettingsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow();
	HelpersGUI::ApplyModernWindowChrome(m_hWnd); // dark/light title bar to match the rest of the app
	HelpersGUI::ApplyDarkThemeToDialog(m_hWnd);  // dark body + controls when the theme is dark
	CSettingsProvider& sp = CSettingsProvider::This();

	// --- Appearance ---
	HWND hTheme = GetDlgItem(IDC_SET_THEME);
	ComboAdd(hTheme, _T("System default"));
	ComboAdd(hTheme, _T("Dark"));
	ComboAdd(hTheme, _T("Light"));
	CString sTheme = sp.AppTheme();
	int nThemeIdx = (sTheme.CompareNoCase(_T("Dark")) == 0) ? 1 : (sTheme.CompareNoCase(_T("Light")) == 0) ? 2 : 0;
	::SendMessage(hTheme, CB_SETCURSEL, nThemeIdx, 0);

	CheckDlgButton(IDC_SET_SHOWFILENAME, sp.ShowFileName() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SET_SHOWFILEINFO, sp.ShowFileInfo() ? BST_CHECKED : BST_UNCHECKED);

	// --- Startup & view ---
	CheckDlgButton(IDC_SET_FULLSCREEN, sp.ShowFullScreen() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SET_HQ, sp.HighQualityResampling() ? BST_CHECKED : BST_UNCHECKED);

	HWND hZoom = GetDlgItem(IDC_SET_ZOOM);
	ComboAdd(hZoom, _T("Fit to screen"));       // index 0 -> Fit
	ComboAdd(hZoom, _T("Fill screen"));         // index 1 -> Fill
	ComboAdd(hZoom, _T("Fit, don't enlarge"));  // index 2 -> FitNoZoom
	ComboAdd(hZoom, _T("Fill, don't enlarge")); // index 3 -> FillNoZoom
	int nZoomIdx;
	switch (sp.AutoZoomMode()) {
		case Helpers::ZM_FitToScreen: nZoomIdx = 0; break;
		case Helpers::ZM_FillScreen: nZoomIdx = 1; break;
		case Helpers::ZM_FillScreenNoZoom: nZoomIdx = 3; break;
		default: nZoomIdx = 2; break; // ZM_FitToScreenNoZoom
	}
	::SendMessage(hZoom, CB_SETCURSEL, nZoomIdx, 0);

	// --- Navigation & slideshow ---
	HWND hSort = GetDlgItem(IDC_SET_SORT);
	ComboAdd(hSort, _T("File name"));     // index 0 -> FileName
	ComboAdd(hSort, _T("Date modified")); // index 1 -> LastModDate
	ComboAdd(hSort, _T("Date created"));  // index 2 -> CreationDate
	ComboAdd(hSort, _T("File size"));     // index 3 -> FileSize
	ComboAdd(hSort, _T("Random"));        // index 4 -> Random
	int nSortIdx;
	switch (sp.Sorting()) {
		case Helpers::FS_LastModTime: nSortIdx = 1; break;
		case Helpers::FS_CreationTime: nSortIdx = 2; break;
		case Helpers::FS_FileSize: nSortIdx = 3; break;
		case Helpers::FS_Random: nSortIdx = 4; break;
		default: nSortIdx = 0; break; // FS_FileName
	}
	::SendMessage(hSort, CB_SETCURSEL, nSortIdx, 0);

	// "Ascending order" has no public accessor; read it straight from the user INI
	TCHAR buf[16];
	::GetPrivateProfileString(_T("JPEGView"), _T("FileSortAscending"), _T("true"), buf, 16, sp.GetUserINIFileName());
	CheckDlgButton(IDC_SET_SORTASC, (_tcsicmp(buf, _T("true")) == 0) ? BST_CHECKED : BST_UNCHECKED);

	HWND hNav = GetDlgItem(IDC_SET_NAV);
	ComboAdd(hNav, _T("Loop this folder"));   // index 0 -> LoopFolder
	ComboAdd(hNav, _T("Include subfolders")); // index 1 -> LoopSubFolders
	ComboAdd(hNav, _T("Same folder level"));  // index 2 -> LoopSameFolderLevel
	int nNavIdx;
	switch (sp.Navigation()) {
		case Helpers::NM_LoopSubDirectories: nNavIdx = 1; break;
		case Helpers::NM_LoopSameDirectoryLevel: nNavIdx = 2; break;
		default: nNavIdx = 0; break; // NM_LoopDirectory
	}
	::SendMessage(hNav, CB_SETCURSEL, nNavIdx, 0);

	HWND hTrans = GetDlgItem(IDC_SET_TRANSITION);
	ComboAdd(hTrans, _T("None"));  // index 0
	ComboAdd(hTrans, _T("Blend")); // index 1
	ComboAdd(hTrans, _T("Slide")); // index 2
	ComboAdd(hTrans, _T("Roll"));  // index 3
	int nTransIdx;
	switch (sp.SlideShowTransitionEffect()) {
		case Helpers::TE_Blend: nTransIdx = 1; break;
		case Helpers::TE_SlideRL: case Helpers::TE_SlideLR:
		case Helpers::TE_SlideTB: case Helpers::TE_SlideBT: nTransIdx = 2; break;
		case Helpers::TE_RollRL: case Helpers::TE_RollLR:
		case Helpers::TE_RollTB: case Helpers::TE_RollBT: nTransIdx = 3; break;
		default: nTransIdx = 0; break; // None
	}
	::SendMessage(hTrans, CB_SETCURSEL, nTransIdx, 0);

	// --- File handling ---
	CheckDlgButton(IDC_SET_SINGLEINST, sp.SingleInstance() ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;
}

LRESULT CSettingsDlg::OnCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!HelpersGUI::ResolveDarkMode()) {
		bHandled = FALSE; // light mode: default handling
		return 0;
	}
	HDC hDC = (HDC)wParam;
	::SetTextColor(hDC, RGB(240, 240, 240));
	if (uMsg == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORLISTBOX) {
		::SetBkColor(hDC, RGB(45, 45, 45));
		return (LRESULT)HelpersGUI::GetDarkEditBrush();
	}
	::SetBkColor(hDC, RGB(32, 32, 32));
	return (LRESULT)HelpersGUI::GetDarkDialogBrush();
}

LRESULT CSettingsDlg::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSettingsProvider& sp = CSettingsProvider::This();

	int nThemeIdx = ComboSel(GetDlgItem(IDC_SET_THEME));
	sp.WriteUserSetting(_T("AppTheme"), (nThemeIdx == 1) ? _T("Dark") : (nThemeIdx == 2) ? _T("Light") : _T("System"));

	sp.WriteUserSetting(_T("ShowFileName"), IsDlgButtonChecked(IDC_SET_SHOWFILENAME) ? _T("true") : _T("false"));
	sp.WriteUserSetting(_T("ShowFileInfo"), IsDlgButtonChecked(IDC_SET_SHOWFILEINFO) ? _T("true") : _T("false"));
	sp.WriteUserSetting(_T("ShowFullScreen"), IsDlgButtonChecked(IDC_SET_FULLSCREEN) ? _T("true") : _T("false"));
	sp.WriteUserSetting(_T("HighQualityResampling"), IsDlgButtonChecked(IDC_SET_HQ) ? _T("true") : _T("false"));
	sp.WriteUserSetting(_T("SingleInstance"), IsDlgButtonChecked(IDC_SET_SINGLEINST) ? _T("true") : _T("false"));
	sp.WriteUserSetting(_T("FileSortAscending"), IsDlgButtonChecked(IDC_SET_SORTASC) ? _T("true") : _T("false"));

	static const LPCTSTR sZoom[] = { _T("Fit"), _T("Fill"), _T("FitNoZoom"), _T("FillNoZoom") };
	int nZoomIdx = ComboSel(GetDlgItem(IDC_SET_ZOOM));
	if (nZoomIdx >= 0 && nZoomIdx < 4) sp.WriteUserSetting(_T("AutoZoomMode"), sZoom[nZoomIdx]);

	static const LPCTSTR sSort[] = { _T("FileName"), _T("LastModDate"), _T("CreationDate"), _T("FileSize"), _T("Random") };
	int nSortIdx = ComboSel(GetDlgItem(IDC_SET_SORT));
	if (nSortIdx >= 0 && nSortIdx < 5) sp.WriteUserSetting(_T("FileDisplayOrder"), sSort[nSortIdx]);

	static const LPCTSTR sNav[] = { _T("LoopFolder"), _T("LoopSubFolders"), _T("LoopSameFolderLevel") };
	int nNavIdx = ComboSel(GetDlgItem(IDC_SET_NAV));
	if (nNavIdx >= 0 && nNavIdx < 3) sp.WriteUserSetting(_T("FolderNavigation"), sNav[nNavIdx]);

	static const LPCTSTR sTrans[] = { _T("None"), _T("Blend"), _T("SlideRL"), _T("RollRL") };
	int nTransIdx = ComboSel(GetDlgItem(IDC_SET_TRANSITION));
	if (nTransIdx >= 0 && nTransIdx < 4) sp.WriteUserSetting(_T("SlideShowTransitionEffect"), sTrans[nTransIdx]);

	EndDialog(IDOK);
	return 0;
}

LRESULT CSettingsDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT CSettingsDlg::OnFileAssoc(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CFileExtensionsDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT CSettingsDlg::OnEditIni(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSettingsProvider& sp = CSettingsProvider::This();
	if (!sp.ExistsUserINI())
		sp.CopyUserINIFromTemplate();
	::ShellExecute(m_hWnd, _T("open"), sp.GetUserINIFileName(), NULL, NULL, SW_SHOWNORMAL);
	return 0;
}
