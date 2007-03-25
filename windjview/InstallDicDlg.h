//	WinDjView
//	Copyright (C) 2004-2007 Andrew Zhezherun
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License version 2
//	as published by the Free Software Foundation.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//	http://www.gnu.org/copyleft/gpl.html

// $Id$

#pragma once


// CInstallDicDlg dialog

class CInstallDicDlg : public CDialog
{
	DECLARE_DYNAMIC(CInstallDicDlg)

public:
	CInstallDicDlg(UINT nID = CInstallDicDlg::IDD, CWnd* pParent = NULL);
	virtual ~CInstallDicDlg();

// Dialog Data
	enum { IDD = IDD_INSTALL_DIC };
	int m_nChoice;
	BOOL m_bKeepOriginal;
	CString m_strDictLocation;

protected:
	UINT m_nTemplateID;
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBrowseLocation();
	DECLARE_MESSAGE_MAP()
};
