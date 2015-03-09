// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitProgressDlg.h"
#include "AppUtils.h"
#include "SmartHandle.h"


IMPLEMENT_DYNAMIC(CGitProgressDlg, CResizableStandAloneDialog)
CGitProgressDlg::CGitProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CGitProgressDlg::IDD, pParent)
	, m_dwCloseOnEnd((DWORD)-1)
{
}

CGitProgressDlg::~CGitProgressDlg()
{
}

void CGitProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
	DDX_Control(pDX, IDC_TITLE_ANIMATE, m_Animate);
	DDX_Control(pDX, IDC_PROGRESSBAR, m_ProgCtrl);
	DDX_Control(pDX, IDC_INFOTEXT, m_InfoCtrl);
	DDX_Control(pDX, IDC_PROGRESSLABEL, m_ProgLableCtrl);
	DDX_Control(pDX, IDC_LOGBUTTON, m_cMenuButton);
}

BEGIN_MESSAGE_MAP(CGitProgressDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_WM_CLOSE()
	ON_WM_SETCURSOR()
	ON_EN_SETFOCUS(IDC_INFOTEXT, &CGitProgressDlg::OnEnSetfocusInfotext)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_PROG_CMD_FINISH, OnCmdEnd)
	ON_MESSAGE(WM_PROG_CMD_START, OnCmdStart)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()



BOOL CGitProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = AtlLoadSystemLibraryUsingFullPath(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
	}
	m_ProgList.m_pTaskbarList.Release();
	if (FAILED(m_ProgList.m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
		m_ProgList.m_pTaskbarList = nullptr;

	UpdateData(FALSE);

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TITLE_ANIMATE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	//SetPromptParentWindow(this->m_hWnd);

	m_Animate.Open(IDR_DOWNLOAD);
	m_ProgList.m_pAnimate = &m_Animate;
	m_ProgList.m_pProgControl = &m_ProgCtrl;
	m_ProgList.m_pProgressLabelCtrl = &m_ProgLableCtrl;
	m_ProgList.m_pInfoCtrl = &m_InfoCtrl;
	m_ProgList.m_pPostWnd = this;
	m_ProgList.m_bSetTitle = true;

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("GITProgressDlg"));

	m_background_brush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	m_ProgList.Init();

	return TRUE;
}

LRESULT CGitProgressDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_ProgList.m_pTaskbarList.Release();
	m_ProgList.m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	SetUUIDOverlayIcon(m_hWnd);
	return 0;
}

void CGitProgressDlg::OnBnClickedLogbutton()
{
	ShowWindow(SW_HIDE);
	m_PostCmdList.at(m_cMenuButton.GetCurrentEntry()).action();
	EndDialog(IDOK);
}


void CGitProgressDlg::OnClose()
{
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CGitProgressDlg::OnOK()
{
	if ((m_ProgList.IsCancelled())&&(!m_ProgList.IsRunning()))
	{
		// I have made this wait a sensible amount of time (10 seconds) for the thread to finish
		// You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you
		// don't do any more operations on the window which might require message passing
		// If you try to send windows messages once we're waiting here, then the thread can't finished
		// because the Window's message loop is blocked at this wait
		WaitForSingleObject(m_ProgList.m_pThread->m_hThread, 10000);
		__super::OnOK();
	}
	m_ProgList.Cancel();
}

void CGitProgressDlg::OnCancel()
{
	if ((m_ProgList.IsCancelled())&&(!m_ProgList.IsRunning()))
		__super::OnCancel();
	m_ProgList.Cancel();
}


BOOL CGitProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_SVNPROGRESS)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CGitProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!m_ProgList.IsRunning())&&(!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}



void CGitProgressDlg::OnEnSetfocusInfotext()
{
	CString sTemp;
	GetDlgItemText(IDC_INFOTEXT, sTemp);
	if (sTemp.IsEmpty())
		GetDlgItem(IDC_INFOTEXT)->HideCaret();
}


LRESULT CGitProgressDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
	HDC hDC = (HDC)wParam;
	HWND hwndCtl = (HWND)lParam;

	if (::GetDlgCtrlID(hwndCtl) == IDC_TITLE_ANIMATE)
	{
		CDC *pDC = CDC::FromHandle(hDC);
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
		return (LRESULT)(HBRUSH)m_background_brush.GetSafeHandle();
	}
	return FALSE;
}

HBRUSH CGitProgressDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr;
	if (pWnd->GetDlgCtrlID() == IDC_TITLE_ANIMATE)
	{
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW)); // add this
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)m_background_brush.GetSafeHandle();
	}
	else
		hbr = CResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}

LRESULT	CGitProgressDlg::OnCmdEnd(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	RefreshCursor();

	DialogEnableWindow(IDCANCEL, FALSE);
	DialogEnableWindow(IDOK, TRUE);

	m_PostCmdList.clear();
	if (m_ProgList.m_Command->m_PostCmdCallback)
		m_ProgList.m_Command->m_PostCmdCallback(m_ProgList.DidErrorsOccur() ? 1 : 0, m_PostCmdList);

	if (!m_PostCmdList.empty())
	{
		for (auto it = m_PostCmdList.cbegin(); it != m_PostCmdList.cend(); ++it)
			m_cMenuButton.AddEntry((*it).icon, (*it).label);
		m_cMenuButton.ShowWindow(SW_SHOW);
	}

	CWnd * pWndOk = GetDlgItem(IDOK);
	if (pWndOk && ::IsWindow(pWndOk->GetSafeHwnd()))
	{
		SendMessage(DM_SETDEFID, IDOK);
		GetDlgItem(IDOK)->SetFocus();
	}

	return 0;
}
LRESULT	CGitProgressDlg::OnCmdStart(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, TRUE);

	return 0;
}
