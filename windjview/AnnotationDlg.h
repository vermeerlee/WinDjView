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

#include "MyColorPicker.h"


// CAnnotationDlg dialog

class CAnnotationDlg : public CDialog
{
	DECLARE_DYNAMIC(CAnnotationDlg)

public:
	CAnnotationDlg(UINT nTitle, CWnd* pParent = NULL);
	virtual ~CAnnotationDlg();

// Dialog Data
	enum { IDD = IDD_ANNOTATION };
	CMyColorPicker m_colorBorder;
	CMyColorPicker m_colorFill;
	CSliderCtrl m_sliderTransparency;
	CComboBox m_cboBorderType;
	CComboBox m_cboFillType;
	BOOL m_bHideInactive;
	int m_nBorderType;
	int m_nFillType;
	COLORREF m_crBorder;
	COLORREF m_crFill;
	int m_nTransparency;
	CString m_strComment;
	bool m_bAddBookmark;
	bool m_bEnableBookmark;
	CString m_strBookmark;

protected:
	UINT m_nTitle;
	void UpdateControls();
	void ToggleDialog(bool bExpand, bool bCenterWindow = false);

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnChangeCombo();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnAddBookmark();
	DECLARE_MESSAGE_MAP()
};