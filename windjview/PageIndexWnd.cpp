//	WinDjView
//	Copyright (C) 2004-2006 Andrew Zhezherun
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

#include "stdafx.h"
#include "WinDjView.h"
#include "PageIndexWnd.h"
#include "DjVuDoc.h"
#include "ChildFrm.h"
#include "DjVuView.h"
#include "MainFrm.h"


static int s_nTextHeight = 11;

static int s_nRightGap = 4;
static int s_nVertGap = 6;
static int s_nMinTextWidth = 10;
static int s_nMinListHeight = 10;

// CPageIndexWnd

IMPLEMENT_DYNAMIC(CPageIndexWnd, CWnd)

BEGIN_MESSAGE_MAP(CPageIndexWnd, CWnd)
	ON_NOTIFY(TVN_SELCHANGED, CPageIndexWnd::ID_LIST, OnSelChanged)
	ON_NOTIFY(TVN_ITEMCLICKED, CPageIndexWnd::ID_LIST, OnItemClicked)
	ON_NOTIFY(TVN_KEYDOWN, CPageIndexWnd::ID_LIST, OnKeyDownList)
	ON_EN_CHANGE(CPageIndexWnd::ID_TEXT, OnChangeText)
	ON_NOTIFY(NM_KEYDOWN, CPageIndexWnd::ID_TEXT, OnKeyDownText)
	ON_WM_CREATE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

CPageIndexWnd::CPageIndexWnd()
	: m_pDoc(NULL)
{
	m_edtText.SetParent(this);
}

CPageIndexWnd::~CPageIndexWnd()
{
}


// CPageIndexWnd message handlers

BOOL CPageIndexWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return false;

	static CString strWndClass = AfxRegisterWndClass(CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW));
	cs.lpszClass = strWndClass;

	return true;
}

int CPageIndexWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateSystemDialogFont(m_font);

	m_edtText.Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD,
		CRect(), this, ID_TEXT);
	m_edtText.SetFont(&m_font);

	m_list.Create(NULL, NULL, WS_VISIBLE | WS_TABSTOP | WS_CHILD
		| TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_TRACKSELECT,
		CRect(), this, ID_LIST);

	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 1);
	CBitmap bitmap;
	bitmap.LoadBitmap(IDB_BOOKMARKS);
	m_imageList.Add(&bitmap, RGB(192, 64, 32));
	m_list.SetImageList(&m_imageList, TVSIL_NORMAL);

	m_list.SetItemHeight(20);
	m_list.SetWrapLabels(false);

	UpdateControls();
	Invalidate();

	return 0;
}

void TrimQuotes(wstring& s)
{
	if (s.length() > 0 && s[0] == '\"')
		s.erase(0, 1);
	if (s.length() > 0 && s[s.length() - 1] == '\"')
		s.erase(s.length() - 1, 1);
}

bool CPageIndexWnd::InitPageIndex(CDjVuDoc* pDoc)
{
	m_pDoc = pDoc;

	wstring strPageIndex;
	if (!MakeWString(pDoc->GetPageIndex(), strPageIndex))
		return false;

	size_t nNextPos;
	size_t nLength = static_cast<int>(strPageIndex.length());

	for (size_t nPos = 0; nPos < nLength; nPos = nNextPos)
	{
		nNextPos = strPageIndex.find_first_of(L"\n\r", nPos);
		if (nNextPos == wstring::npos)
			nNextPos = nLength;

		wstring strLine = strPageIndex.substr(nPos, nNextPos - nPos);
		while (nNextPos < nLength &&
				(strPageIndex[nNextPos] == '\n' || strPageIndex[nNextPos] == '\r'))
			++nNextPos;

		int nComma1 = strLine.find(',');
		if (nComma1 == wstring::npos)
			continue;
		int nComma2 = strLine.find(',', nComma1 + 1);
		if (nComma2 == wstring::npos)
			continue;

		m_entries.push_back(IndexEntry());
		IndexEntry& entry = m_entries.back();

		entry.strFirst = strLine.substr(0, nComma1);
		entry.strLast = strLine.substr(nComma1 + 1, nComma2 - nComma1 - 1);
		entry.strLink = strLine.substr(nComma2 + 1, nLength - nComma2 - 1);

		TrimQuotes(entry.strFirst);
		TrimQuotes(entry.strLast);
		TrimQuotes(entry.strLink);

		CString strTitle = MakeCString(entry.strFirst) + _T(" - ") + MakeCString(entry.strLast);
		HTREEITEM hItem = m_list.InsertItem(strTitle, 0, 1, TVI_ROOT);

		m_list.SetItemData(hItem, m_entries.size() - 1);
		entry.hItem = hItem;
	}

	m_sorted.resize(m_entries.size());
	for (size_t i = 0; i < m_sorted.size(); ++i)
		m_sorted[i] = &m_entries[i];

	sort(m_sorted.begin(), m_sorted.end(), CompareEntries);

	return !m_entries.empty();
}

void CPageIndexWnd::GoToItem(HTREEITEM hItem)
{
	size_t nEntry = m_list.GetItemData(hItem);
	IndexEntry& entry = m_entries[nEntry];

	if (entry.strLink.length() > 0)
	{
		CDjVuView* pView = ((CChildFrame*)GetParentFrame())->GetDjVuView();
		pView->GoToURL(MakeUTF8String(entry.strLink));
	}
}

void CPageIndexWnd::OnSelChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if (pNMTreeView->action == TVC_BYMOUSE)
	{
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		if (hItem != NULL)
			GoToItem(hItem);
	}

	*pResult = 0;
}

void CPageIndexWnd::OnItemClicked(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if (pNMTreeView->action == TVC_BYMOUSE)
	{
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		if (hItem != NULL)
			GoToItem(hItem);
	}

	*pResult = 0;
}

void CPageIndexWnd::OnKeyDownList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	if (pTVKeyDown->wVKey == VK_RETURN || pTVKeyDown->wVKey == VK_SPACE)
	{
		HTREEITEM hItem = m_list.GetSelectedItem();
		if (hItem != NULL)
			GoToItem(hItem);

		*pResult = 0;
	}
	else
		*pResult = 1;
}

void CPageIndexWnd::OnKeyDownText(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMKEY pKeyDown = reinterpret_cast<LPNMKEY>(pNMHDR);

	if (pKeyDown->nVKey == VK_RETURN)
	{
		HTREEITEM hItem = m_list.GetSelectedItem();
		if (hItem != NULL)
			GoToItem(hItem);

		*pResult = 0;
	}
	else
		*pResult = 1;
}

void CPageIndexWnd::OnChangeText()
{
	CString strText;
	GetDlgItem(ID_TEXT)->GetWindowText(strText);

	wstring text;
	MakeWString(strText, text);

	int left = 0, right = m_sorted.size();
	while (left < right - 1)
	{
		int mid = (left + right) / 2;
		if (m_sorted[mid]->strFirst.compare(text) <= 0)
			left = mid;
		else
			right = mid;
	}

	if (left > 0 && m_sorted[left - 1]->strLast.compare(text) == 0)
		--left;

	m_list.SetSelectedItem(m_sorted[left]->hItem);
}

void CPageIndexWnd::PostNcDestroy()
{
	// Should be created on heap
	delete this;
}

void CPageIndexWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos) 
{
	CWnd::OnWindowPosChanged(lpwndpos);

	UpdateControls();
}

inline CPoint ConvertDialogPoint(const CPoint& point, const CSize& szBaseUnits)
{
	CPoint result;

	result.x = MulDiv(point.x, szBaseUnits.cx, 4);
	result.y = MulDiv(point.y, szBaseUnits.cy, 8);

	return result;
}

void CPageIndexWnd::UpdateControls()
{
	CRect rcClient;
	GetClientRect(rcClient);

	CDC dcScreen;
	dcScreen.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	CFont* pOldFont = dcScreen.SelectObject(&m_font);

	TEXTMETRIC tm;
	dcScreen.GetTextMetrics(&tm);
	dcScreen.SelectObject(pOldFont);

	CSize szBaseUnit(tm.tmAveCharWidth, tm.tmHeight);

	CPoint ptText = ConvertDialogPoint(CPoint(0, s_nTextHeight), szBaseUnit);
	CSize szText(max(s_nMinTextWidth, rcClient.Width() - s_nRightGap - 2), ptText.y - 2);
	ptText = CPoint(1, 1);

	CPoint ptList(1, szText.cy + s_nVertGap + 3);
	CSize szList(max(s_nMinTextWidth, rcClient.Width() - 1/*s_nRightGap - 2*/),
		max(s_nMinListHeight, rcClient.Height() - ptList.y));

	m_edtText.SetWindowPos(NULL, ptText.x, ptText.y, szText.cx, szText.cy, SWP_NOACTIVATE);
	m_list.SetWindowPos(NULL, ptList.x, ptList.y, szList.cx, szList.cy, SWP_NOACTIVATE);

	m_rcText = CRect(ptText, szText);
	m_rcText.InflateRect(1, 1);
	m_rcList = CRect(ptList, szList);
	m_rcList.InflateRect(1, 1);

	m_rcGap = CRect(0, ptText.y, rcClient.Width(), ptList.y - 1);
}

BOOL CPageIndexWnd::OnEraseBkgnd(CDC* pDC)
{
	return true;
}

void CPageIndexWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect rcClient;
	GetClientRect(rcClient);

	COLORREF clrBtnface = ::GetSysColor(COLOR_BTNFACE);
	COLORREF clrShadow = ::GetSysColor(COLOR_BTNSHADOW);
	CBrush brushShadow(clrShadow);

	CRect rcRightGap(rcClient.Width() - s_nRightGap, 0, rcClient.Width(), m_rcGap.bottom);
	dc.FillSolidRect(rcRightGap, clrBtnface);
	dc.FillSolidRect(m_rcGap, clrBtnface);

	dc.FrameRect(m_rcText, &brushShadow);
	dc.FrameRect(m_rcList, &brushShadow);
}

BOOL CPageIndexWnd::CIndexEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			HTREEITEM hItem = m_pParent->m_list.GetSelectedItem();
			if (hItem != NULL)
				m_pParent->GoToItem(hItem);

			return true;
		}
		if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN ||
			pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT)
		{
			m_pParent->m_list.SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
			return true;
		}
	}
	else if (pMsg->message == WM_MOUSEWHEEL)
	{
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));

		CWnd* pWnd = WindowFromPoint(point);
		if (pWnd != this && !IsChild(pWnd) && GetMainFrame()->IsChild(pWnd))
			pWnd->SendMessage(WM_MOUSEWHEEL, pMsg->wParam, pMsg->lParam);

		return true;
	}

	return CEdit::PreTranslateMessage(pMsg);
}