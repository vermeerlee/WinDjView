//	WinDjView
//	Copyright (C) 2004-2005 Andrew Zhezherun
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
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

#include "AppSettings.h"


// CSettingsDlg dialog

class CSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = NULL);
	virtual ~CSettingsDlg();

// Dialog Data
	enum { IDD = IDD_SETTINGS };
	BOOL m_bRestoreAssocs;
	BOOL m_bGenAllThumbnails;
	BOOL m_bFullscreenClicks;
	CStatic m_ctlAbout;
	BOOL m_bAdjustDisplay;
	CSliderCtrl m_sliderBrightness;
	CSliderCtrl m_sliderContrast;
	CSliderCtrl m_sliderGamma;
	CString m_strBrightnessValue;
	CString m_strContrastValue;
	CString m_strGammaValue;

protected:
	CFont m_font;
	CDisplaySettings m_displaySettings;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnAssociate();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnAdjustDisplay();
	DECLARE_MESSAGE_MAP()
};
