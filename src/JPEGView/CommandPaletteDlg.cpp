#include "StdAfx.h"
#include "CommandPaletteDlg.h"
#include "NLS.h"
#include "HelpersGUI.h"

// Dark theme colors for the palette (the title bar is themed separately via ApplyModernWindowChrome).
static const COLORREF CP_BACK_COLOR = RGB(32, 32, 34);
static const COLORREF CP_TEXT_COLOR = RGB(230, 230, 232);

// The set of commands offered by the palette: {command id, English label (localized at display time)}.
// Ids are the same IDM_* values CMainDlg::ExecuteCommand() dispatches; the palette just re-dispatches the
// chosen one. Commands that no-op without a loaded image behave exactly as they do from the context menu.
struct PaletteCommand { int nCommandId; LPCTSTR sLabel; };

static const PaletteCommand kPaletteCommands[] = {
	{ IDM_OPEN,                    _T("Open image...") },
	{ IDM_SAVE,                    _T("Save processed image...") },
	{ IDM_SAVE_SCREEN,             _T("Save image as displayed...") },
	{ IDM_EXPORT,                  _T("Export (choose format and quality)...") },
	{ IDM_OCR,                     _T("Copy text from image (OCR)") },
	{ IDM_QUICK_ADJUST,            _T("Quick adjust (brightness/contrast/saturation)") },
	{ IDM_RELOAD,                  _T("Reload image") },
	{ IDM_EXPLORE,                 _T("Open containing folder in Explorer") },
	{ IDM_PRINT,                   _T("Print image...") },
	{ IDM_BATCH_COPY,              _T("Batch rename/copy...") },
	{ IDM_RENAME,                  _T("Rename file") },
	{ IDM_MOVE_TO_RECYCLE_BIN,     _T("Delete file (to recycle bin)") },
	{ IDM_TOUCH_IMAGE,             _T("Set modification date to current date") },
	{ IDM_TOUCH_IMAGE_EXIF,        _T("Set modification date to EXIF date") },
	{ IDM_SET_WALLPAPER_ORIG,      _T("Set as wallpaper: original image") },
	{ IDM_SET_WALLPAPER_DISPLAY,   _T("Set as wallpaper: processed image") },
	{ IDM_COPY,                    _T("Copy to clipboard") },
	{ IDM_COPY_FULL,              _T("Copy original size image") },
	{ IDM_COPY_PATH,               _T("Copy file path") },
	{ IDM_PASTE,                   _T("Paste from clipboard") },
	{ IDM_SHOW_FILEINFO,           _T("Show picture info (EXIF)") },
	{ IDM_SHOW_ON_MAP,             _T("Show location on map") },
	{ IDM_SHOW_FILENAME,           _T("Show filename") },
	{ IDM_SHOW_NAVPANEL,           _T("Show navigation panel") },
	{ IDM_COLOR_PICKER,            _T("Pick color (eyedropper)") },
	{ IDM_SHOW_ORIGINAL,           _T("Show original (toggle while held)") },
	{ IDM_NEXT,                    _T("Next image") },
	{ IDM_PREV,                    _T("Previous image") },
	{ IDM_FIRST,                   _T("First image") },
	{ IDM_LAST,                    _T("Last image") },
	{ IDM_LOOP_FOLDER,             _T("Navigation: loop folder") },
	{ IDM_LOOP_RECURSIVELY,        _T("Navigation: loop recursively") },
	{ IDM_LOOP_SIBLINGS,           _T("Navigation: loop sibling folders") },
	{ IDM_SORT_MOD_DATE,           _T("Sort by modification date") },
	{ IDM_SORT_CREATION_DATE,      _T("Sort by creation date") },
	{ IDM_SORT_NAME,               _T("Sort by file name") },
	{ IDM_SORT_SIZE,               _T("Sort by file size") },
	{ IDM_SORT_RANDOM,             _T("Sort randomly") },
	{ IDM_SLIDESHOW_RESUME,        _T("Resume slideshow") },
	{ IDM_ROTATE_90,               _T("Rotate +90 (lossy)") },
	{ IDM_ROTATE_270,              _T("Rotate -90 (lossy)") },
	{ IDM_ROTATE,                  _T("Rotate by angle...") },
	{ IDM_CHANGESIZE,              _T("Change size (resample)...") },
	{ IDM_PERSPECTIVE,             _T("Perspective correction...") },
	{ IDM_MIRROR_H,                _T("Mirror horizontally") },
	{ IDM_MIRROR_V,                _T("Mirror vertically") },
	{ IDM_ROTATE_90_LOSSLESS,      _T("Lossless: rotate +90") },
	{ IDM_ROTATE_270_LOSSLESS,     _T("Lossless: rotate -90") },
	{ IDM_ROTATE_180_LOSSLESS,     _T("Lossless: rotate 180") },
	{ IDM_MIRROR_H_LOSSLESS,       _T("Lossless: mirror horizontally") },
	{ IDM_MIRROR_V_LOSSLESS,       _T("Lossless: mirror vertically") },
	{ IDM_AUTO_CORRECTION,         _T("Auto correction") },
	{ IDM_LDC,                     _T("Local density correction") },
	{ IDM_LANDSCAPE_MODE,          _T("Landscape enhancement mode") },
	{ IDM_KEEP_PARAMETERS,         _T("Keep parameters") },
	{ IDM_SAVE_PARAMETERS,         _T("Set current parameters as default...") },
	{ IDM_SAVE_PARAM_DB,           _T("Save parameters to DB") },
	{ IDM_CLEAR_PARAM_DB,          _T("Clear parameters from DB") },
	{ IDM_TOGGLE_RESAMPLING_QUALITY, _T("Toggle resampling quality") },
	{ IDM_FIT_TO_SCREEN,           _T("Fit to screen") },
	{ IDM_FILL_WITH_CROP,          _T("Fill with crop") },
	{ IDM_FIT_WINDOW_TO_IMAGE,     _T("Fit window to image") },
	{ IDM_SPAN_SCREENS,            _T("Span all screens") },
	{ IDM_TOGGLE_MONITOR,          _T("Move to other monitor") },
	{ IDM_FULL_SCREEN_MODE,        _T("Full screen mode") },
	{ IDM_HIDE_TITLE_BAR,          _T("Hide window title bar") },
	{ IDM_ALWAYS_ON_TOP,           _T("Always on top") },
	{ IDM_ZOOM_400,                _T("Zoom 400 %") },
	{ IDM_ZOOM_200,                _T("Zoom 200 %") },
	{ IDM_ZOOM_100,                _T("Zoom 100 %") },
	{ IDM_ZOOM_50,                 _T("Zoom 50 %") },
	{ IDM_ZOOM_25,                 _T("Zoom 25 %") },
	{ IDM_AUTO_ZOOM_FIT_NO_ZOOM,   _T("Auto zoom: fit to screen (no enlarge)") },
	{ IDM_AUTO_ZOOM_FILL_NO_ZOOM,  _T("Auto zoom: fill with crop (no enlarge)") },
	{ IDM_AUTO_ZOOM_FIT,           _T("Auto zoom: fit to screen") },
	{ IDM_AUTO_ZOOM_FILL,          _T("Auto zoom: fill with crop") },
	{ IDM_SETTINGS,                _T("Preferences...") },
	{ IDM_EDIT_GLOBAL_CONFIG,      _T("Edit global settings...") },
	{ IDM_EDIT_USER_CONFIG,        _T("Edit user settings...") },
	{ IDM_UPDATE_USER_CONFIG,      _T("Update user settings...") },
	{ IDM_MANAGE_OPEN_WITH_MENU,   _T("Manage 'Open image with' menu...") },
	{ IDM_SET_AS_DEFAULT_VIEWER,   _T("Set as default viewer...") },
	{ IDM_BACKUP_PARAMDB,          _T("Backup parameter DB...") },
	{ IDM_RESTORE_PARAMDB,         _T("Restore parameter DB...") },
	{ IDM_ABOUT,                   _T("About JPEGView...") },
	{ IDM_EXIT,                    _T("Exit") },
};

CCommandPaletteDlg::CCommandPaletteDlg() : m_edtFilter(this, 1) {
	m_hbrBack = NULL;
	m_nChosenCmd = 0;
}

CCommandPaletteDlg::~CCommandPaletteDlg() {
	if (m_hbrBack != NULL) {
		::DeleteObject(m_hbrBack);
	}
}

LRESULT CCommandPaletteDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	SetWindowText(CNLS::GetString(_T("Command palette")));
	m_hbrBack = ::CreateSolidBrush(CP_BACK_COLOR);
	m_edtFilter.SubclassWindow(GetDlgItem(IDC_CP_FILTER));
	m_lstResults.Attach(GetDlgItem(IDC_CP_LIST));
	CenterWindow(GetParent());
	RefilterList();
	HelpersGUI::ApplyModernWindowChrome(m_hWnd); // dark title bar (client/controls themed via WM_CTLCOLOR* below)
	m_edtFilter.SetFocus();
	return FALSE; // focus set explicitly
}

LRESULT CCommandPaletteDlg::OnCtlColorDlg(UINT, WPARAM, LPARAM, BOOL&) {
	return (LRESULT)m_hbrBack;
}

LRESULT CCommandPaletteDlg::OnCtlColorChild(UINT, WPARAM wParam, LPARAM, BOOL&) {
	HDC hdc = (HDC)wParam;
	::SetBkColor(hdc, CP_BACK_COLOR);
	::SetTextColor(hdc, CP_TEXT_COLOR);
	return (LRESULT)m_hbrBack;
}

void CCommandPaletteDlg::RefilterList() {
	CString sFilter;
	int nLen = m_edtFilter.GetWindowTextLength();
	m_edtFilter.GetWindowText(sFilter.GetBuffer(nLen + 1), nLen + 1);
	sFilter.ReleaseBuffer();
	sFilter.MakeLower();

	m_lstResults.SetRedraw(FALSE);
	m_lstResults.ResetContent();
	for (int i = 0; i < sizeof(kPaletteCommands) / sizeof(kPaletteCommands[0]); i++) {
		CString sLabel = CNLS::GetString(kPaletteCommands[i].sLabel);
		CString sLower = sLabel;
		sLower.MakeLower();
		if (sFilter.IsEmpty() || sLower.Find(sFilter) >= 0) {
			int nIndex = m_lstResults.AddString(sLabel);
			if (nIndex >= 0) {
				m_lstResults.SetItemData(nIndex, (DWORD_PTR)kPaletteCommands[i].nCommandId);
			}
		}
	}
	if (m_lstResults.GetCount() > 0) {
		m_lstResults.SetCurSel(0);
	}
	m_lstResults.SetRedraw(TRUE);
	m_lstResults.Invalidate();
}

void CCommandPaletteDlg::AcceptSelection() {
	int nSel = m_lstResults.GetCurSel();
	if (nSel == LB_ERR) {
		return; // nothing to run; keep the dialog open
	}
	m_nChosenCmd = (int)m_lstResults.GetItemData(nSel);
	EndDialog(IDOK);
}

LRESULT CCommandPaletteDlg::OnFilterChanged(WORD, WORD, HWND, BOOL&) {
	RefilterList();
	return 0;
}

LRESULT CCommandPaletteDlg::OnListDblClk(WORD, WORD, HWND, BOOL&) {
	AcceptSelection();
	return 0;
}

LRESULT CCommandPaletteDlg::OnOK(WORD, WORD, HWND, BOOL&) {
	AcceptSelection();
	return 0;
}

LRESULT CCommandPaletteDlg::OnCancel(WORD, WORD, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

// Up/Down move the result selection and Enter confirms - all while typing in the search box.
LRESULT CCommandPaletteDlg::OnFilterKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
	int nCount = m_lstResults.GetCount();
	if (wParam == VK_DOWN) {
		if (nCount > 0) {
			int nSel = m_lstResults.GetCurSel();
			m_lstResults.SetCurSel(min(nCount - 1, (nSel < 0) ? 0 : nSel + 1));
		}
		return 0;
	} else if (wParam == VK_UP) {
		if (nCount > 0) {
			int nSel = m_lstResults.GetCurSel();
			m_lstResults.SetCurSel(max(0, (nSel < 0) ? 0 : nSel - 1));
		}
		return 0;
	} else if (wParam == VK_RETURN) {
		AcceptSelection();
		return 0;
	}
	bHandled = FALSE; // normal typing passes through to the edit control
	return 0;
}
