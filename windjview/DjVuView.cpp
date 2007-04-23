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

#include "stdafx.h"
#include "WinDjView.h"
#include "Global.h"

#include "DjVuDoc.h"
#include "DjVuView.h"

#include "MainFrm.h"
#include "PrintDlg.h"
#include "Drawing.h"
#include "ProgressDlg.h"
#include "ZoomDlg.h"
#include "GotoPageDlg.h"
#include "FindDlg.h"
#include "ChildFrm.h"
#include "BookmarksWnd.h"
#include "SearchResultsView.h"
#include "FullscreenWnd.h"
#include "ThumbnailsView.h"
#include "MagnifyWnd.h"
#include "AnnotationDlg.h"
#include "BookmarkDlg.h"
#include "MyFileDialog.h"
#include "NavPane.h"

#include "RenderThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDjVuView

HCURSOR CDjVuView::hCursorHand = NULL;
HCURSOR CDjVuView::hCursorDrag = NULL;
HCURSOR CDjVuView::hCursorLink = NULL;
HCURSOR CDjVuView::hCursorText = NULL;
HCURSOR CDjVuView::hCursorMagnify = NULL;
HCURSOR CDjVuView::hCursorCross = NULL;
HCURSOR CDjVuView::hCursorZoomRect = NULL;

const int c_nDefaultMargin = 4;
const int c_nDefaultShadowMargin = 3;
const int c_nDefaultPageGap = 4;
const int c_nDefaultFacingGap = 4;
const int c_nDefaultPageBorder = 1;
const int c_nDefaultPageShadow = 3;

const int c_nFullscreenMargin = 0;
const int c_nFullscreenShadowMargin = 0;
const int c_nFullscreenPageGap = 4;
const int c_nFullscreenFacingGap = 4;
const int c_nFullscreenPageBorder = 0;
const int c_nFullscreenPageShadow = 0;

const int c_nHourglassWidth = 15;
const int c_nHourglassHeight = 24;

const int s_nCursorHideDelay = 3500;


// IMPORTANT: There seems to be a problem with the way TranslateAccelerator
// interacts with MFC. When an accelerator corresponds to an item in a submenu
// (like "View"->"Go To"->"First Page", the OnInitMenuPopup function is called
// with the "View" menu as an argument, not with the "Go To" menu. As a result,
// the update handler for a menu item in the "Go To" submenu is not called,
// and if this item was disabled when the menu was last displayed, it will stay
// disabled, even if it is enabled on the toolbar. TranslateAccelerator doesn't
// send a WM_COMMAND message if a corresponding menu item is disabled, so the
// accelerator doesn't work as expected.
//
// To fix this, for every item on a submenu with an accelerator, a second
// command ID is introduced, and the accelerator table specifies this second ID
// (so that it is not attached to a menu). Note that there is no update handler
// for this second ID, so the command handler should take this into account.
//
// This applies to the following items: "First Page", "Previous Page",
// "Next Page", "Last Page" (because they might become disabled), and also
// "Previous View" and "Next View" in the CMainFrame.

IMPLEMENT_DYNCREATE(CDjVuView, CMyScrollView)

BEGIN_MESSAGE_MAP(CDjVuView, CMyScrollView)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_NEXTPAGE, OnViewNextpage)
	ON_COMMAND(ID_VIEW_PREVIOUSPAGE, OnViewPreviouspage)
	ON_COMMAND(ID_VIEW_NEXTPAGE_SHORTCUT, OnViewNextpage)
	ON_COMMAND(ID_VIEW_PREVIOUSPAGE_SHORTCUT, OnViewPreviouspage)
	ON_UPDATE_COMMAND_UI(ID_VIEW_NEXTPAGE, OnUpdateViewNextpage)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PREVIOUSPAGE, OnUpdateViewPreviouspage)
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_COMMAND_RANGE(ID_ZOOM_50, ID_ZOOM_CUSTOM, OnViewZoom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ZOOM_50, ID_ZOOM_CUSTOM, OnUpdateViewZoom)
	ON_COMMAND(ID_ROTATE_LEFT, OnRotateLeft)
	ON_COMMAND(ID_ROTATE_RIGHT, OnRotateRight)
	ON_COMMAND(ID_ROTATE_180, OnRotate180)
	ON_COMMAND(ID_VIEW_FIRSTPAGE, OnViewFirstpage)
	ON_COMMAND(ID_VIEW_LASTPAGE, OnViewLastpage)
	ON_COMMAND(ID_VIEW_FIRSTPAGE_SHORTCUT, OnViewFirstpage)
	ON_COMMAND(ID_VIEW_LASTPAGE_SHORTCUT, OnViewLastpage)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LASTPAGE, OnUpdateViewNextpage)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FIRSTPAGE, OnUpdateViewPreviouspage)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CANCELMODE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(ID_LAYOUT_SINGLEPAGE, ID_LAYOUT_CONTINUOUS_FACING, OnViewLayout)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LAYOUT_SINGLEPAGE, ID_LAYOUT_CONTINUOUS_FACING, OnUpdateViewLayout)
	ON_MESSAGE(WM_PAGE_RENDERED, OnPageRendered)
	ON_MESSAGE(WM_PAGE_DECODED, OnPageDecoded)
	ON_WM_DESTROY()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_FIND_STRING, OnFindString)
	ON_COMMAND(ID_FIND_ALL, OnFindAll)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNeedText)
	ON_COMMAND(ID_ZOOM_IN, OnViewZoomIn)
	ON_COMMAND(ID_ZOOM_OUT, OnViewZoomOut)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_IN, OnUpdateViewZoomIn)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_OUT, OnUpdateViewZoomOut)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_WM_MOUSEACTIVATE()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_PAGE_INFORMATION, OnPageInformation)
	ON_COMMAND_RANGE(ID_EXPORT_PAGE, ID_EXPORT_SELECTION, OnExportPage)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_SELECTION, OnUpdateExportSelection)
	ON_COMMAND_RANGE(ID_MODE_DRAG, ID_MODE_ZOOM_RECT, OnChangeMode)
	ON_UPDATE_COMMAND_UI_RANGE(ID_MODE_DRAG, ID_MODE_ZOOM_RECT, OnUpdateMode)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND_RANGE(ID_DISPLAY_COLOR, ID_DISPLAY_FOREGROUND, OnViewDisplay)
	ON_UPDATE_COMMAND_UI_RANGE(ID_DISPLAY_COLOR, ID_DISPLAY_FOREGROUND, OnUpdateViewDisplay)
	ON_COMMAND(ID_VIEW_GOTO_PAGE, OnViewGotoPage)
	ON_WM_TIMER()
	ON_COMMAND(ID_LAYOUT_FIRSTPAGE_ALONE, OnFirstPageAlone)
	ON_UPDATE_COMMAND_UI(ID_LAYOUT_FIRSTPAGE_ALONE, OnUpdateFirstPageAlone)
	ON_MESSAGE_VOID(WM_MOUSELEAVE, OnMouseLeave)
	ON_COMMAND_RANGE(ID_HIGHLIGHT_SELECTION, ID_HIGHLIGHT_TEXT, OnHighlight)
	ON_UPDATE_COMMAND_UI_RANGE(ID_HIGHLIGHT_SELECTION, ID_HIGHLIGHT_TEXT, OnUpdateHighlight)
	ON_COMMAND(ID_ANNOTATION_DELETE, OnDeleteAnnotation)
	ON_COMMAND(ID_ANNOTATION_EDIT, OnEditAnnotation)
	ON_COMMAND(ID_BOOKMARK_ADD, OnAddBookmark)
	ON_COMMAND(ID_ZOOM_TO_SELECTION, OnZoomToSelection)
	ON_COMMAND(ID_NEXT_PANE, OnSwitchFocus)
	ON_COMMAND(ID_PREV_PANE, OnSwitchFocus)
END_MESSAGE_MAP()

// CDjVuView construction/destruction

CDjVuView::CDjVuView()
	: m_nPage(-1), m_nPageCount(0), m_nZoomType(ZoomPercent), m_fZoom(100.0),
	  m_nLayout(SinglePage), m_nRotate(0), m_bDragging(false), m_pSource(NULL),
	  m_pRenderThread(NULL), m_bInsideUpdateLayout(false), m_bClick(false),
	  m_pageRendered(false, true), m_nPendingPage(-1), m_nClickedPage(-1),
	  m_nMode(Drag), m_bHasSelection(false), m_nDisplayMode(Color), m_bShiftDown(false),
	  m_bNeedUpdate(false), m_bCursorHidden(false), m_bDraggingPage(false),
	  m_bDraggingText(false), m_bFirstPageAlone(false), m_bInitialized(false),
	  m_bDraggingMagnify(false), m_pHScrollBar(NULL), m_pVScrollBar(NULL),
	  m_bControlDown(false), m_nType(Normal), m_bPanning(false), m_bHoverIsCustom(false),
	  m_bDraggingRect(false), m_nSelectionPage(-1), m_pHoverAnno(NULL),
	  m_bIgnoreMouseLeave(false), m_pClickedAnno(NULL), m_bDraggingLink(false),
	  m_bPopupMenu(false), m_bClickedCustom(false)
{
	m_nMargin = c_nDefaultMargin;
	m_nShadowMargin = c_nDefaultShadowMargin;
	m_nPageGap = c_nDefaultPageGap;
	m_nFacingGap = c_nDefaultFacingGap;
	m_nPageBorder = c_nDefaultPageBorder;
	m_nPageShadow = c_nDefaultPageShadow;
}

CDjVuView::~CDjVuView()
{
	delete m_pHScrollBar;
	delete m_pVScrollBar;

	DeleteBitmaps();

	m_dataLock.Lock();

	for (set<CDIB*>::iterator it = m_bitmaps.begin(); it != m_bitmaps.end(); ++it)
		delete *it;
	m_bitmaps.clear();

	m_dataLock.Unlock();

	if (m_pSource != NULL)
		m_pSource->Release();
}

BOOL CDjVuView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMyScrollView::PreCreateWindow(cs);
}

// CDjVuView drawing

void CDjVuView::OnDraw(CDC* pDC)
{
	if (pDC->IsPrinting())
	{
		// TODO: Print preview
		Page& page = m_pages[m_nPage];

		if (page.pBitmap != NULL)
		{
			int nOffsetX = pDC->GetDeviceCaps(PHYSICALOFFSETX);
			int nOffsetY = pDC->GetDeviceCaps(PHYSICALOFFSETY);

			pDC->SetViewportOrg(-nOffsetX, -nOffsetY);

			CSize szPage = page.GetSize(m_nRotate);
			page.pBitmap->Draw(pDC, CPoint(0, 0), szPage);
		}
		return;
	}

	// CScrollView sets viewport to minus scroll position, so that
	// all drawing can be done in natural coordinates. Unfortunately,
	// this does not work in Win98, because in this OS coordinates
	// cannot be larger than 32767. So we will subtract scroll position
	// explicitly.
	pDC->SetViewportOrg(CPoint(0, 0));

	CRect rcClient;
	GetClientRect(rcClient);

	CRect rcClip;
	pDC->GetClipBox(rcClip);
	rcClip.IntersectRect(CRect(rcClip), rcClient);

	m_offscreenDC.Create(pDC, rcClip.Size());
	m_offscreenDC.SetViewportOrg(-rcClip.TopLeft());
	m_offscreenDC.IntersectClipRect(rcClip);

	CPoint ptScrollPos = GetScrollPosition();
	rcClip.OffsetRect(ptScrollPos);

	CRect rcIntersect;

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		DrawPage(&m_offscreenDC, m_nPage);

		if (m_nLayout == Facing && HasFacingPage(m_nPage))
			DrawPage(&m_offscreenDC, m_nPage + 1);
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		for (int nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			if (rcIntersect.IntersectRect(m_pages[nPage].rcDisplay, rcClip))
				DrawPage(&m_offscreenDC, nPage);
		}
	}

	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		Page& page = m_pages[nPage];
		if (m_nLayout == SinglePage || m_nLayout == Facing)
		{
			if (nPage != m_nPage && !(m_nLayout == Facing && HasFacingPage(m_nPage) && nPage == m_nPage + 1))
				continue;
		}
		else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		{
			if (!rcIntersect.IntersectRect(m_pages[nPage].rcDisplay, rcClip))
				continue;
		}

		if (page.pBitmap == NULL || page.pBitmap->m_hObject == NULL)
			continue;

		// Draw annotations
		for (list<Annotation>::iterator lit = page.info.anno.begin(); lit != page.info.anno.end(); ++lit)
			DrawAnnotation(&m_offscreenDC, *lit, nPage, &(*lit) == m_pHoverAnno);

		// Draw custom annotations
		map<int, PageSettings>::iterator it = m_pSource->GetSettings()->pageSettings.find(nPage);
		if (it != m_pSource->GetSettings()->pageSettings.end())
		{
			PageSettings& pageSettings = (*it).second;
			for (list<Annotation>::iterator lit = pageSettings.anno.begin(); lit != pageSettings.anno.end(); ++lit)
				DrawAnnotation(&m_offscreenDC, *lit, nPage, &(*lit) == m_pHoverAnno);
		}

		// Draw selection
		for (GPosition pos = page.selection; pos; ++pos)
		{
			GRect rect = page.selection[pos]->rect;
			CRect rcText = TranslatePageRect(nPage, rect);
			m_offscreenDC.InvertRect(rcText - ptScrollPos);
		}
	}

	// Draw rectangle selection
	if (m_nSelectionPage != -1 && m_rcSelection.width() > 1 && m_rcSelection.height() > 1)
	{
		CRect rcSel = TranslatePageRect(m_nSelectionPage, m_rcSelection);
		rcSel.InflateRect(0, 0, 1, 1);
		InvertFrame(&m_offscreenDC, rcSel - ptScrollPos);
	}

	rcClip.OffsetRect(-ptScrollPos);
	pDC->BitBlt(rcClip.left, rcClip.top, rcClip.Width(), rcClip.Height(),
			&m_offscreenDC, rcClip.left, rcClip.top, SRCCOPY);

	m_offscreenDC.Release();
}

void CDjVuView::DrawAnnotation(CDC* pDC, const Annotation& anno, int nPage, bool bActive)
{
	CRect rcBounds = TranslatePageRect(nPage, anno.rectBounds);
	rcBounds.OffsetRect(-GetScrollPosition());

	bool bShowAll = m_bShiftDown && GetFocus() == this;

	// Border
	if (bActive || bShowAll || !anno.bHideInactiveBorder)
	{
		if (anno.nBorderType == Annotation::BorderXOR || bShowAll)
		{
			InvertFrame(pDC, rcBounds);
		}
		else if (anno.nBorderType == Annotation::BorderSolid)
		{
			FrameRect(pDC, rcBounds, anno.crBorder);
		}
		rcBounds.DeflateRect(1, 1);
	}

	for (size_t i = 0; i < anno.rects.size(); ++i)
	{
		CRect rect = TranslatePageRect(nPage, anno.rects[i]);
		rect.OffsetRect(-GetScrollPosition());
		if (!rect.IntersectRect(CRect(rect), rcBounds))
			continue;

		// Fill
		if (anno.nFillType == Annotation::FillSolid)
		{
			HighlightRect(pDC, rect, anno.crFill, anno.fTransparency);
		}
		else if (anno.nFillType == Annotation::FillXOR)
		{
			pDC->InvertRect(rect);
		}
	}
}

void CDjVuView::DrawPage(CDC* pDC, int nPage)
{
	Page& page = m_pages[nPage];

	COLORREF clrFrame = ::GetSysColor(COLOR_WINDOWFRAME);
	COLORREF clrBtnshadow = ::GetSysColor(COLOR_BTNSHADOW);
	COLORREF clrWindow = ::GetSysColor(COLOR_WINDOW);
	COLORREF clrBtnface = ::GetSysColor(COLOR_BTNFACE);

	COLORREF clrShadow = ChangeBrightness(clrBtnshadow, 0.75);
	COLORREF clrBackground = ChangeBrightness(clrBtnface, 0.85);
	if (m_nType == Fullscreen || m_nType == Magnify && GetMainFrame()->IsFullscreenMode())
		clrBackground = RGB(0, 0, 0);

	CPoint ptScrollPos = GetScrollPosition();

	CRect rcClip;
	pDC->GetClipBox(rcClip);
	rcClip.OffsetRect(ptScrollPos);
	rcClip.IntersectRect(rcClip, page.rcDisplay);

	if (rcClip.IsRectEmpty())
		return;

	if (page.szBitmap.cx <= 0 || page.szBitmap.cy <= 0)
	{
		pDC->FillSolidRect(rcClip - ptScrollPos, clrBackground);
		return;
	}

	if (page.pBitmap == NULL || page.pBitmap->m_hObject == NULL)
	{
		// Draw white rect
		CRect rcPage(page.ptOffset, page.szBitmap);
		pDC->FillSolidRect(rcPage - ptScrollPos, clrWindow);

		if (!page.bBitmapRendered &&
				page.szBitmap.cx >= c_nHourglassWidth && page.szBitmap.cy >= c_nHourglassHeight)
		{
			// Draw hourglass
			CPoint pt((page.szBitmap.cx - c_nHourglassWidth) / 2, (page.szBitmap.cy - c_nHourglassHeight) / 2);
			m_hourglass.Draw(pDC, 0, CPoint(pt + page.ptOffset - ptScrollPos), ILD_NORMAL);
		}
	}
	else if (page.pBitmap->GetSize() == page.szBitmap)
	{
		// Draw offscreen bitmap
		CPoint ptPartOffset(max(rcClip.left - page.ptOffset.x, 0),
			max(rcClip.top - page.ptOffset.y, 0));

		CSize szPageClip = page.szBitmap - ptPartOffset;
		CSize szPart(min(rcClip.Width(), szPageClip.cx), min(rcClip.Height(), szPageClip.cy));

		if (szPart.cx > 0 && szPart.cy > 0)
		{
			CRect rcPart(ptPartOffset, szPart);
			page.pBitmap->DrawDC(pDC,
				page.ptOffset + ptPartOffset - ptScrollPos, rcPart);
		}
	}
	else
	{
		// Offscreen bitmap has yet wrong size, stretch it
		page.pBitmap->Draw(pDC, page.ptOffset - ptScrollPos, page.szBitmap);
	}

	CRect rcBorder(page.ptOffset, page.szBitmap);
	int nBorder = m_nPageBorder;
	while (nBorder-- > 0)
	{
		// Draw border
		rcBorder.InflateRect(1, 1);
		FrameRect(pDC, rcBorder - ptScrollPos, clrFrame);
	}

	if (m_nPageShadow > 0)
	{
		// Draw shadow
		CRect rcWhiteBar(CPoint(rcBorder.left, rcBorder.bottom),
				CSize(m_nPageShadow + 1, m_nPageShadow));
		pDC->FillSolidRect(rcWhiteBar - ptScrollPos, clrBackground);
		rcWhiteBar = CRect(CPoint(rcBorder.right, rcBorder.top),
				CSize(m_nPageShadow, m_nPageShadow + 1));
		pDC->FillSolidRect(rcWhiteBar - ptScrollPos, clrBackground);

		CRect rcShadow(CPoint(rcBorder.left + m_nPageShadow + 1, rcBorder.bottom),
				CSize(rcBorder.Width() - 1, m_nPageShadow));
		pDC->FillSolidRect(rcShadow - ptScrollPos, clrShadow);
		rcShadow = CRect(CPoint(rcBorder.right, rcBorder.top + m_nPageShadow + 1),
				CSize(m_nPageShadow, rcBorder.Height() - 1));
		pDC->FillSolidRect(rcShadow - ptScrollPos, clrShadow);

		rcBorder.InflateRect(0, 0, m_nPageShadow, m_nPageShadow);
	}

	// Fill everything else with backgroundColor
	int nSaveDC = pDC->SaveDC();
	pDC->IntersectClipRect(page.rcDisplay - ptScrollPos);
	pDC->ExcludeClipRect(rcBorder - ptScrollPos);
	pDC->FillSolidRect(rcClip - ptScrollPos, clrBackground);
	pDC->RestoreDC(nSaveDC);
}


// CDjVuView printing

BOOL CDjVuView::OnPreparePrinting(CPrintInfo* pInfo)
{
	pInfo->SetMaxPage(-1);
	return DoPreparePrinting(pInfo);
}


// CDjVuView diagnostics

#ifdef _DEBUG
void CDjVuView::AssertValid() const
{
	CMyScrollView::AssertValid();
}

void CDjVuView::Dump(CDumpContext& dc) const
{
	CMyScrollView::Dump(dc);
}

CDjVuDoc* CDjVuView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDjVuDoc)));
	return (CDjVuDoc*)m_pDocument;
}
#endif


// CDjVuView message handlers

void CDjVuView::OnInitialUpdate()
{
	CMyScrollView::OnInitialUpdate();

	m_pSource = GetDocument()->GetSource();
	m_pSource->AddRef();

	CAppSettings* pAppSettings = theApp.GetAppSettings();
	DocSettings* pDocSettings = m_pSource->GetSettings();

	theApp.AddObserver(this);
	pDocSettings->AddObserver(this);

	CBitmap bitmap;
	bitmap.LoadBitmap(IDB_HOURGLASS);
	m_hourglass.Create(c_nHourglassWidth, c_nHourglassHeight, ILC_COLOR24 | ILC_MASK, 0, 1);
	m_hourglass.Add(&bitmap, RGB(192, 0, 32));

	if (m_toolTip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX) && m_toolTip.AddTool(this))
	{
		TOOLINFO info;
		info.cbSize = sizeof(TOOLINFO);
		info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		info.hwnd = m_hWnd;
		info.uId = (UINT)m_hWnd;
		info.lpszText = LPSTR_TEXTCALLBACK;
		m_toolTip.SetToolInfo(&info);

		m_toolTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
		m_toolTip.Activate(false);
	}

	m_nPageCount = m_pSource->GetPageCount();
	m_pages.resize(m_nPageCount);

	// Save m_nPage, because UpdateLayout, which is called before RenderPage, can change it
	int nStartupPage = (m_nPage >= 0 && m_nPage < GetPageCount() ? m_nPage : 0);
	m_nPage = nStartupPage;

	if (m_nType == Normal)
	{
		m_nMode = pAppSettings->nDefaultMode;
		m_nLayout = pAppSettings->nDefaultLayout;
		m_bFirstPageAlone = pAppSettings->bFirstPageAlone;
		m_nZoomType = pAppSettings->nDefaultZoomType;
		m_fZoom = pAppSettings->fDefaultZoom;
		if (m_nZoomType == ZoomPercent)
			m_fZoom = 100.0;

		Page& page = m_pages[0];
		page.Init(m_pSource, 0, false, true);
		if (page.info.pAnt != NULL)
		{
			ReadZoomSettings(page.info.pAnt);
			ReadDisplayMode(page.info.pAnt);
		}

		// Restore zoom
		if (pAppSettings->bRestoreView && pDocSettings->nZoomType >= ZoomStretch && pDocSettings->nZoomType <= ZoomPercent)
		{
			m_nZoomType = pDocSettings->nZoomType;
			m_fZoom = max(min(pDocSettings->fZoom, 800.0), 10.0);
		}

		// Restore layout
		if (pAppSettings->bRestoreView && pDocSettings->nLayout >= SinglePage && pDocSettings->nLayout <= ContinuousFacing)
		{
			m_nLayout = pDocSettings->nLayout;
			m_bFirstPageAlone = pDocSettings->bFirstPageAlone;
		}

		// Restore mode
		if (pAppSettings->bRestoreView && pDocSettings->nDisplayMode >= Color && pDocSettings->nDisplayMode <= Foreground)
			m_nDisplayMode = pDocSettings->nDisplayMode;

		// Restore rotate
		if (pAppSettings->bRestoreView)
			m_nRotate = pDocSettings->nRotate;
	}

	m_displaySettings = *theApp.GetDisplaySettings();

	m_pRenderThread = new CRenderThread(m_pSource, this);

	UpdateLayout(RECALC);
	RenderPage(nStartupPage);

	if (m_nType == Normal)
	{
		pDocSettings->nZoomType = m_nZoomType;
		pDocSettings->fZoom = m_fZoom;
		pDocSettings->nDisplayMode = m_nDisplayMode;
		pDocSettings->nLayout = m_nLayout;
		pDocSettings->bFirstPageAlone = m_bFirstPageAlone;
		pDocSettings->nRotate = m_nRotate;

		Bookmark bmView;
		CreateBookmarkFromView(bmView);
		pDocSettings->nPage = bmView.nPage;
		pDocSettings->ptOffset = bmView.ptOffset;
	}

	m_nTimerID = SetTimer(1, 100, NULL);
	ShowCursor();

	m_bInitialized = true;
}

void CDjVuView::ReadZoomSettings(GP<DjVuANT> pAnt)
{
	switch (pAnt->zoom)
	{
	case DjVuANT::ZOOM_STRETCH:
		m_nZoomType = ZoomStretch;
		break;

	case DjVuANT::ZOOM_ONE2ONE:
		m_nZoomType = ZoomActualSize;
		break;

	case DjVuANT::ZOOM_WIDTH:
		m_nZoomType = ZoomFitWidth;
		break;

	case DjVuANT::ZOOM_PAGE:
		m_nZoomType = ZoomFitPage;
		break;
	}

	if (pAnt->zoom > 0)
	{
		m_nZoomType = ZoomPercent;
		m_fZoom = max(min(pAnt->zoom, 800.0), 10.0);
	}
}

void CDjVuView::ReadDisplayMode(GP<DjVuANT> pAnt)
{
	switch (pAnt->mode)
	{
	case DjVuANT::MODE_FORE:
		m_nDisplayMode = Foreground;
		break;

	case DjVuANT::MODE_BACK:
		m_nDisplayMode = Background;
		break;

	case DjVuANT::MODE_BW:
		m_nDisplayMode = BlackAndWhite;
		break;
	}
}

CMainFrame* CDjVuView::GetMainFrame() const
{
	if (m_nType == Normal)
		return (CMainFrame*) GetTopLevelFrame();
	else if (m_nType == Fullscreen)
		return ((CFullscreenWnd*) GetTopLevelParent())->GetOwner()->GetMainFrame();
	else if (m_nType == Magnify)
		return ((CMagnifyWnd*) GetTopLevelParent())->GetOwner()->GetMainFrame();

	ASSERT(false);
	return GetMainWnd();
}

BOOL CDjVuView::OnEraseBkgnd(CDC* pDC)
{
	return true;
}

void CDjVuView::RenderPage(int nPage, int nTimeout, bool bUpdateWindow)
{
	ASSERT(nPage >= 0 && nPage < m_nPageCount);

	if (m_nLayout == Facing || m_nLayout == ContinuousFacing)
		nPage = FixPageNumber(nPage);
	Page& page = m_pages[nPage];

	m_nPendingPage = nPage;
	m_pageRendered.ResetEvent();

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		m_nPage = nPage;

		if (!page.info.bDecoded)
			page.Init(m_pSource, nPage);

		if (m_nLayout == Facing && nPage < m_nPageCount - 1)
		{
			Page& nextPage = m_pages[nPage + 1];
			if (!nextPage.info.bDecoded)
				nextPage.Init(m_pSource, nPage + 1);
		}

		UpdateLayout(RECALC);
		ScrollToPositionNoRepaint(CPoint(GetScrollPos(SB_HORZ), 0));
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdatePageSizes(page.ptOffset.y);

		int nUpdatedPos = page.ptOffset.y - m_nPageBorder - (nPage == 0 ? m_nMargin : min(m_nMargin, m_nPageGap));
		ScrollToPositionNoRepaint(CPoint(GetScrollPos(SB_HORZ), nUpdatedPos));
	}

	UpdateVisiblePages();

	if (nTimeout != -1 && !m_pages[nPage].bBitmapRendered)
		::WaitForSingleObject(m_pageRendered, nTimeout);
	m_nPendingPage = -1;

	UpdatePageNumber();

	Invalidate();

	if (bUpdateWindow)
		UpdateWindow();
}

void CDjVuView::ScrollToPage(int nPage, const CPoint& ptOffset, bool bMargin)
{
	RenderPage(nPage, -1, false);

	GRect rect(ptOffset.x, ptOffset.y);
	CPoint ptPos = TranslatePageRect(nPage, rect, true, false).TopLeft() - m_pages[nPage].ptOffset;

	CPoint ptPageOffset = m_pages[nPage].ptOffset;

	if (bMargin)
	{
		// Offset top-left to 1/10 of the client area
		CRect rcClient;
		GetClientRect(rcClient);

		ptPos.x -= static_cast<int>(0.1 * rcClient.Width() + 0.5);
		ptPos.y -= static_cast<int>(0.1 * rcClient.Height() + 0.5);
	}
	else
	{
		if (ptPos.x == 0)
			ptPos.x = -m_nMargin;
		if (ptPos.y == 0)
			ptPos.y = -m_nPageBorder - (nPage == 0 ? m_nMargin : min(m_nMargin, m_nPageGap));
	}

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdatePageSizes(ptPageOffset.y, ptPos.y);
		ptPageOffset = m_pages[nPage].ptOffset;
	}

	ScrollToPositionNoRepaint(ptPageOffset + ptPos);

	Invalidate();
	UpdateWindow();

	UpdatePageNumber();
}

void CDjVuView::DeleteBitmaps()
{
	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		m_pages[nPage].DeleteBitmap();
	}
}

void CDjVuView::UpdateLayout(UpdateType updateType)
{
	if (m_bInsideUpdateLayout)
		return;

	m_bInsideUpdateLayout = true;

	if (m_nLayout == SinglePage)
	{
		Page& page = m_pages[m_nPage];

		// Update current page size 3 times to allow for scrollbars
		for (int i = 0; i < 3; ++i)
		{
			// First update all page sizes so that cached bitmaps
			// will be of right size
			for (int nPage = 0; nPage < m_nPageCount; ++nPage)
			{
				Page& page = m_pages[nPage];
				page.szBitmap = CalcPageSize(page.GetSize(m_nRotate), page.info.nDPI);
			}

			m_szDisplay = UpdatePageRect(m_nPage);

			CRect rcClient;
			GetClientRect(rcClient);
			CSize szDevPage(rcClient.Width()*3/4, rcClient.Height()*3/4);
			CSize szDevLine(15, 15);

			SetScrollSizes(m_szDisplay, szDevPage, szDevLine);
		}

		if (updateType != RECALC)
		{
			UpdateVisiblePages();
		}
	}
	else if (m_nLayout == Facing)
	{
		// Update current page size 3 times to allow for scrollbars
		for (int i = 0; i < 3; ++i)
		{
			// First update all page sizes so that cached bitmaps
			// will be of right size
			for (int nPage = 0; nPage < m_nPageCount; nPage = GetNextPage(nPage))
			{
				Page& page = m_pages[nPage];
				if (HasFacingPage(nPage))
				{
					Page& nextPage = m_pages[nPage + 1];
					CalcPageSizeFacing(page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap,
							nextPage.GetSize(m_nRotate), nextPage.info.nDPI, nextPage.szBitmap);
				}
				else
				{
					CalcPageSizeFacing(page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap,
							page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap);
				}
			}

			m_szDisplay = UpdatePageRectFacing(m_nPage);

			CRect rcClient;
			GetClientRect(rcClient);
			CSize szDevPage(rcClient.Width()*3/4, rcClient.Height()*3/4);
			CSize szDevLine(15, 15);

			SetScrollSizes(m_szDisplay, szDevPage, szDevLine);
		}

		if (updateType != RECALC)
		{
			UpdateVisiblePages();
		}
	}
	else if (m_nLayout == Continuous)
	{
		CRect rcClient;
		GetClientRect(rcClient);

		// Save page and offset to restore after changes
		int nAnchorPage = -1;
		CPoint ptAnchorOffset;
		CPoint ptDjVuOffset;
		CSize szPrevBitmap;
		CPoint ptTop = GetScrollPosition();

		if (updateType == TOP)
		{
			nAnchorPage = CalcTopPage();
			ptAnchorOffset = ptTop - m_pages[nAnchorPage].ptOffset;
		}
		else if (updateType == BOTTOM)
		{
			CPoint ptBottom = ptTop + rcClient.Size();

			int nPage = CalcTopPage();
			while (nPage < m_nPageCount - 1 &&
					ptBottom.y > m_pages[nPage].rcDisplay.bottom)
				++nPage;

			nAnchorPage = nPage;
			ptAnchorOffset = ptBottom - m_pages[nAnchorPage].ptOffset;
		}

		if (nAnchorPage != -1)
		{
			ptDjVuOffset = ScreenToDjVu(nAnchorPage, ptAnchorOffset, false);
			szPrevBitmap = m_pages[nAnchorPage].szBitmap;
		}

		for (int i = 0; i < 3; ++i)
		{
			m_szDisplay = CSize(0, 0);
			int nTop = m_nMargin;

			int nPage;
			for (nPage = 0; nPage < m_nPageCount; ++nPage)
			{
				PreparePageRect(nPage);

				Page& page = m_pages[nPage];
				page.ptOffset.Offset(m_nMargin, nTop);
				page.rcDisplay.OffsetRect(0, nTop);
				page.rcDisplay.InflateRect(0, nPage == 0 ? m_nMargin : m_nPageGap,
						m_nMargin + m_nShadowMargin, nPage == m_nPageCount - 1 ? m_nShadowMargin : 0);

				nTop = page.rcDisplay.bottom + m_nPageGap;

				if (page.rcDisplay.Width() > m_szDisplay.cx)
					m_szDisplay.cx = page.rcDisplay.Width();

				if (page.info.bDecoded)
					page.bHasSize = true;
			}

			m_szDisplay.cy = m_pages.back().rcDisplay.bottom;

			GetClientRect(rcClient);
			if (m_szDisplay.cx < rcClient.Width())
				m_szDisplay.cx = rcClient.Width();

			// Center pages horizontally
			for (nPage = 0; nPage < m_nPageCount; ++nPage)
			{
				Page& page = m_pages[nPage];

				if (page.rcDisplay.Width() < m_szDisplay.cx)
				{
					page.ptOffset.x += (m_szDisplay.cx - page.rcDisplay.Width()) / 2;
					page.rcDisplay.right = m_szDisplay.cx;
				}
			}

			// Center pages vertically
			if (m_szDisplay.cy < rcClient.Height())
			{
				int nOffset = (rcClient.Height() - m_szDisplay.cy) / 2;
				for (nPage = 0; nPage < m_nPageCount; ++nPage)
				{
					Page& page = m_pages[nPage];
					page.rcDisplay.OffsetRect(0, nOffset);
					page.ptOffset.Offset(0, nOffset);
				}

				m_szDisplay.cy = rcClient.Height();
			}

			m_pages[0].rcDisplay.top = 0;
			m_pages.back().rcDisplay.bottom = m_szDisplay.cy;

			CSize szDevPage(rcClient.Width()*3/4, rcClient.Height()*3/4);
			CSize szDevLine(15, 15);
			SetScrollSizesNoRepaint(m_szDisplay, szDevPage, szDevLine);
		}

		if (nAnchorPage != -1)
		{
			GRect rect(ptDjVuOffset.x, ptDjVuOffset.y);
			CPoint ptNewOffset = TranslatePageRect(nAnchorPage, rect, true, false).TopLeft()
					- m_pages[nAnchorPage].ptOffset;
			if (m_pages[nAnchorPage].szBitmap != szPrevBitmap)
			{
				if (ptAnchorOffset.x >= 0)
					ptAnchorOffset.x = ptNewOffset.x;
				if (ptAnchorOffset.y >= 0)
					ptAnchorOffset.y = ptNewOffset.y;
			}
		}

		if (updateType == TOP)
		{
			ScrollToPositionNoRepaint(m_pages[nAnchorPage].ptOffset + ptAnchorOffset);
		}
		else if (updateType == BOTTOM)
		{
			ScrollToPositionNoRepaint(m_pages[nAnchorPage].ptOffset + ptAnchorOffset - rcClient.Size());
		}

		if (updateType != RECALC)
		{
			UpdateVisiblePages();
		}

		UpdatePageNumber();
	}
	else if (m_nLayout == ContinuousFacing)
	{
		CRect rcClient;
		GetClientRect(rcClient);

		// Save page and offset to restore after changes
		int nAnchorPage = -1;
		CPoint ptAnchorOffset;
		CPoint ptDjVuOffset;
		CSize szPrevBitmap;
		CPoint ptTop = GetScrollPosition();

		if (updateType == TOP)
		{
			nAnchorPage = CalcTopPage();
			ptAnchorOffset = ptTop - m_pages[nAnchorPage].ptOffset;
		}
		else if (updateType == BOTTOM)
		{
			CPoint ptBottom = ptTop + rcClient.Size();

			int nPage = CalcTopPage();
			while (nPage < m_nPageCount - 1 &&
					ptBottom.y > m_pages[nPage].rcDisplay.bottom)
				++nPage;

			nAnchorPage = nPage;
			ptAnchorOffset = ptBottom - m_pages[nAnchorPage].ptOffset;
		}

		if (nAnchorPage != -1)
		{
			ptDjVuOffset = ScreenToDjVu(nAnchorPage, ptAnchorOffset, false);
			szPrevBitmap = m_pages[nAnchorPage].szBitmap;
		}

		for (int i = 0; i < 3; ++i)
		{
			GetClientRect(rcClient);

			m_szDisplay = CSize(0, 0);
			int nTop = m_nMargin;

			int nPage;
			for (nPage = 0; nPage < m_nPageCount; nPage = GetNextPage(nPage))
			{
				PreparePageRectFacing(nPage);

				Page& page = m_pages[nPage];
				Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);

				page.ptOffset.Offset(m_nMargin, nTop);
				page.rcDisplay.OffsetRect(0, nTop);
				page.rcDisplay.InflateRect(0, nPage == 0 ? m_nMargin : m_nPageGap,
					m_nMargin, GetNextPage(nPage) >= m_nPageCount ? m_nShadowMargin : 0);

				if (pNextPage != NULL)
				{
					pNextPage->ptOffset.Offset(m_nMargin, nTop);
					pNextPage->rcDisplay.OffsetRect(m_nMargin, nTop);
					pNextPage->rcDisplay.InflateRect(0, nPage == 0 ? m_nMargin : m_nPageGap,
						m_nShadowMargin, GetNextPage(nPage) >= m_nPageCount ? m_nShadowMargin : 0);

					// Align pages horizontally
					if (page.szBitmap.cx < pNextPage->szBitmap.cx)
					{
						page.ptOffset.x += pNextPage->szBitmap.cx - page.szBitmap.cx;
						page.rcDisplay.right += pNextPage->szBitmap.cx - page.szBitmap.cx;
						pNextPage->ptOffset.x += pNextPage->szBitmap.cx - page.szBitmap.cx;
						pNextPage->rcDisplay.OffsetRect(pNextPage->szBitmap.cx - page.szBitmap.cx, 0);
					}
					else if (page.szBitmap.cx > pNextPage->szBitmap.cx)
					{
						pNextPage->rcDisplay.right += page.szBitmap.cx - pNextPage->szBitmap.cx;
					}

					if (pNextPage->rcDisplay.right > m_szDisplay.cx)
						m_szDisplay.cx = pNextPage->rcDisplay.right;
				}
				else
				{
					page.rcDisplay.right += m_nShadowMargin;

					if (page.rcDisplay.Width() > m_szDisplay.cx)
						m_szDisplay.cx = page.rcDisplay.Width();
				}

				nTop = page.rcDisplay.bottom + m_nPageGap;

				if (page.info.bDecoded)
					page.bHasSize = true;
				if (pNextPage != NULL && pNextPage->info.bDecoded)
					pNextPage->bHasSize = true;
			}

			m_szDisplay.cy = m_pages.back().rcDisplay.bottom;

			GetClientRect(rcClient);
			if (m_szDisplay.cx < rcClient.Width())
				m_szDisplay.cx = rcClient.Width();

			// Center pages horizontally
			for (nPage = 0; nPage < m_nPageCount; nPage = GetNextPage(nPage))
			{
				Page& page = m_pages[nPage];
				Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);

				if (pNextPage != NULL)
				{
					int nOffset = m_szDisplay.cx - pNextPage->rcDisplay.right;
					if (nOffset > 0)
					{
						page.ptOffset.x += nOffset / 2;
						page.rcDisplay.right += nOffset / 2;
						pNextPage->ptOffset.x += nOffset / 2;
						pNextPage->rcDisplay.OffsetRect(nOffset / 2, 0);
						pNextPage->rcDisplay.right = m_szDisplay.cx;
					}
				}
				else
				{
					if (page.rcDisplay.Width() < m_szDisplay.cx)
					{
						page.ptOffset.x += (m_szDisplay.cx - page.rcDisplay.Width()) / 2;
						page.rcDisplay.right = m_szDisplay.cx;
					}
				}
			}

			// Center pages vertically
			if (m_szDisplay.cy < rcClient.Height())
			{
				int nOffset = (rcClient.Height() - m_szDisplay.cy) / 2;
				for (nPage = 0; nPage < m_nPageCount; ++nPage)
				{
					Page& page = m_pages[nPage];
					page.rcDisplay.OffsetRect(0, nOffset);
					page.ptOffset.Offset(0, nOffset);
				}

				m_szDisplay.cy = rcClient.Height();
			}

			m_pages[0].rcDisplay.top = 0;
			if (HasFacingPage(0))
				m_pages[1].rcDisplay.top = 0;

			m_pages[m_nPageCount - 1].rcDisplay.bottom = m_szDisplay.cy;
			if (m_nPageCount >= 2 && IsValidPage(m_nPageCount - 2) && HasFacingPage(m_nPageCount - 2))
				m_pages[m_nPageCount - 2].rcDisplay.bottom = m_szDisplay.cy;

			CSize szDevPage(rcClient.Width()*3/4, rcClient.Height()*3/4);
			CSize szDevLine(15, 15);
			SetScrollSizesNoRepaint(m_szDisplay, szDevPage, szDevLine);
		}

		if (nAnchorPage != -1)
		{
			GRect rect(ptDjVuOffset.x, ptDjVuOffset.y);
			CPoint ptNewOffset = TranslatePageRect(nAnchorPage, rect, true, false).TopLeft()
					- m_pages[nAnchorPage].ptOffset;
			if (m_pages[nAnchorPage].szBitmap != szPrevBitmap)
			{
				if (ptAnchorOffset.x >= 0)
					ptAnchorOffset.x = ptNewOffset.x;
				if (ptAnchorOffset.y >= 0)
					ptAnchorOffset.y = ptNewOffset.y;
			}
		}

		if (updateType == TOP)
		{
			ScrollToPositionNoRepaint(m_pages[nAnchorPage].ptOffset + ptAnchorOffset);
		}
		else if (updateType == BOTTOM)
		{
			ScrollToPositionNoRepaint(m_pages[nAnchorPage].ptOffset + ptAnchorOffset - rcClient.Size());
		}

		if (updateType != RECALC)
		{
			UpdateVisiblePages();
		}

		UpdatePageNumber();
	}

	m_bInsideUpdateLayout = false;
}

void CDjVuView::UpdatePagesCacheSingle(bool bUpdateImages)
{
	ASSERT(m_nLayout == SinglePage);

	for (int nDiff = m_nPageCount; nDiff >= 0; --nDiff)
	{
		if (m_nPage - nDiff >= 0)
			UpdatePageCacheSingle(m_nPage - nDiff, bUpdateImages);
		if (m_nPage + nDiff < m_nPageCount && nDiff != 0)
			UpdatePageCacheSingle(m_nPage + nDiff, bUpdateImages);
	}
}

void CDjVuView::UpdatePagesCacheFacing(bool bUpdateImages)
{
	ASSERT(m_nLayout == Facing);

	for (int nDiff = m_nPageCount; nDiff >= 0; --nDiff)
	{
		if (m_nPage - nDiff >= 0)
			UpdatePageCacheFacing(m_nPage - nDiff, bUpdateImages);
		if (m_nPage + nDiff < m_nPageCount && nDiff != 0)
			UpdatePageCacheFacing(m_nPage + nDiff, bUpdateImages);
	}
}

void CDjVuView::UpdatePageCache(int nPage, const CRect& rcClient, bool bUpdateImages)
{
	// Pages visible on screen are put to the front of the rendering queue.
	// Pages which are within 2 screens from the view are put to the back
	// of the rendering queue.
	// Pages which are within 10 screens from the view are put to the back
	// of the decoding queue.

	int nTop = GetScrollPos(SB_VERT);
	Page& page = m_pages[nPage];

	if (!page.info.bDecoded)
	{
		if (m_nType == Magnify)
			return;

		m_pRenderThread->AddReadInfoJob(nPage);
	}
	else if (page.rcDisplay.top < nTop + 3*rcClient.Height() &&
			 page.rcDisplay.bottom > nTop - 2*rcClient.Height())
	{
		if (page.pBitmap == NULL || page.szBitmap != page.pBitmap->GetSize() && bUpdateImages)
		{
			if (m_nType == Magnify)
				CopyBitmapFrom(((CMagnifyWnd*) GetTopLevelParent())->GetOwner(), nPage);

			m_pRenderThread->AddJob(nPage, m_nRotate, page.szBitmap, m_displaySettings, m_nDisplayMode);
			InvalidatePage(nPage);
		}
	}
	else
	{
		page.DeleteBitmap();
		if (m_nType == Magnify)
			return;

		if (page.rcDisplay.top < nTop + 11*rcClient.Height() &&
			page.rcDisplay.bottom > nTop - 10*rcClient.Height() || nPage == 0 || nPage == m_nPageCount - 1)
		{
			m_pRenderThread->AddDecodeJob(nPage);
		}
		else if (m_pSource->IsPageCached(nPage, this))
		{
			m_pRenderThread->AddCleanupJob(nPage);
		}
	}
}

void CDjVuView::UpdatePageCacheSingle(int nPage, bool bUpdateImages)
{
	// Current page and adjacent are rendered, next +- 9 pages are decoded.
	Page& page = m_pages[nPage];
	long nPageSize = page.szBitmap.cx * page.szBitmap.cy;

	if (!page.info.bDecoded)
	{
		if (m_nType == Magnify)
			return;

		m_pRenderThread->AddReadInfoJob(nPage);
	}
	else if (nPageSize < 3000000 && abs(nPage - m_nPage) <= 2 ||
			 abs(nPage - m_nPage) <= 1)
	{
		if (page.pBitmap == NULL || page.szBitmap != page.pBitmap->GetSize() && bUpdateImages)
		{
			if (m_nType == Magnify)
				CopyBitmapFrom(((CMagnifyWnd*) GetTopLevelParent())->GetOwner(), nPage);

			m_pRenderThread->AddJob(nPage, m_nRotate, page.szBitmap, m_displaySettings, m_nDisplayMode);
			InvalidatePage(nPage);
		}
	}
	else
	{
		page.DeleteBitmap();
		if (m_nType == Magnify)
			return;

		if (abs(nPage - m_nPage) <= 10 || nPage == 0 || nPage == m_nPageCount - 1)
		{
			m_pRenderThread->AddDecodeJob(nPage);
		}
		else if (m_pSource->IsPageCached(nPage, this))
		{
			m_pRenderThread->AddCleanupJob(nPage);
		}
	}
}

void CDjVuView::UpdatePageCacheFacing(int nPage, bool bUpdateImages)
{
	// Current page and adjacent are rendered, next +- 9 pages are decoded.
	Page& page = m_pages[nPage];
	long nPageSize = page.szBitmap.cx * page.szBitmap.cy;

	if (!page.info.bDecoded)
	{
		if (m_nType == Magnify)
			return;

		m_pRenderThread->AddReadInfoJob(nPage);
	}
	else if (nPageSize < 1500000 && nPage >= m_nPage - 4 && nPage <= m_nPage + 5 ||
			 nPage >= m_nPage - 2 && nPage <= m_nPage + 3)
	{
		if (page.pBitmap == NULL || page.szBitmap != page.pBitmap->GetSize() && bUpdateImages)
		{
			if (m_nType == Magnify)
				CopyBitmapFrom(((CMagnifyWnd*) GetTopLevelParent())->GetOwner(), nPage);

			m_pRenderThread->AddJob(nPage, m_nRotate, page.szBitmap, m_displaySettings, m_nDisplayMode);
			InvalidatePage(nPage);
		}
	}
	else
	{
		page.DeleteBitmap();
		if (m_nType == Magnify)
			return;

		if (abs(nPage - m_nPage) <= 10 || nPage == 0 || nPage == m_nPageCount - 1)
		{
			m_pRenderThread->AddDecodeJob(nPage);
		}
		else if (m_pSource->IsPageCached(nPage, this))
		{
			m_pRenderThread->AddCleanupJob(nPage);
		}
	}
}

void CDjVuView::UpdatePagesCacheContinuous(bool bUpdateImages)
{
	ASSERT(m_nLayout == Continuous || m_nLayout == ContinuousFacing);

	CRect rcClient;
	GetClientRect(rcClient);
	int nTop = GetScrollPos(SB_VERT);

	int nTopPage = CalcTopPage();

	int nBottomPage = GetNextPage(nTopPage);
	while (nBottomPage < m_nPageCount &&
		m_pages[nBottomPage].rcDisplay.top < nTop + rcClient.Height())
	{
		nBottomPage = GetNextPage(nBottomPage);
	}

	--nBottomPage;
	if (nBottomPage >= m_nPageCount)
		nBottomPage = m_nPageCount - 1;

	for (int nDiff = m_nPageCount; nDiff >= 1; --nDiff)
	{
		if (nTopPage - nDiff >= 0)
			UpdatePageCache(nTopPage - nDiff, rcClient, bUpdateImages);
		if (nBottomPage + nDiff < m_nPageCount)
			UpdatePageCache(nBottomPage + nDiff, rcClient, bUpdateImages);
	}

	int nLastPage = m_nPage;
	int nMaxSize = -1;
	for (int nPage = nBottomPage; nPage >= nTopPage; --nPage)
	{
		UpdatePageCache(nPage, rcClient, bUpdateImages);
		CRect rcIntersect;
		if (rcIntersect.IntersectRect(rcClient,
				CRect(m_pages[nPage].ptOffset + (-GetScrollPosition()), m_pages[nPage].szBitmap))
			&& rcIntersect.Width() * rcIntersect.Height() >= nMaxSize)
		{
			nLastPage = nPage;
			nMaxSize = rcIntersect.Width() * rcIntersect.Height();
		}
	}

	// Push to the front of the queue a page with the largest visible area
	UpdatePageCache(nLastPage, rcClient, bUpdateImages);
}

void CDjVuView::UpdateVisiblePages()
{
	if (!m_bInitialized || m_pRenderThread->IsPaused())
		return;

	bool bUpdateImages = true;
	if (m_nType == Normal)
	{
		CMDIChildWnd* pActive = GetMainFrame()->MDIGetActive();
		if (pActive == NULL)
			return;

		bUpdateImages = (pActive->GetActiveView() == this);
	}

	m_pRenderThread->PauseJobs();
	m_pRenderThread->RemoveAllJobs();

	if (m_nLayout == SinglePage)
		UpdatePagesCacheSingle(bUpdateImages);
	else if (m_nLayout == Facing)
		UpdatePagesCacheFacing(bUpdateImages);
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePagesCacheContinuous(bUpdateImages);

	m_pRenderThread->ResumeJobs();
}

CSize CDjVuView::CalcPageSize(const CSize& szPage, int nDPI) const
{
	return CalcPageSize(szPage, nDPI, m_nZoomType);
}

CSize CDjVuView::CalcPageSize(const CSize& szPage, int nDPI, int nZoomType) const
{
	CSize szDisplay(1, 1);
	if (szPage.cx <= 0 || szPage.cy <= 0)
		return szDisplay;

	if (nZoomType <= ZoomFitWidth && nZoomType >= ZoomStretch)
	{
		int nFrame = m_nMargin + m_nShadowMargin + 2*m_nPageBorder + m_nPageShadow;
		CSize szFrame(nFrame, nFrame);

		CRect rcClient;
		GetClientRect(rcClient);

		return CalcZoomedPageSize(szPage, rcClient.Size() - szFrame, nZoomType);
	}
	else
	{
		CScreenDC dcScreen;
		int nLogPixelsX = dcScreen.GetDeviceCaps(LOGPIXELSX);
		int nLogPixelsY = dcScreen.GetDeviceCaps(LOGPIXELSX);

		CSize szDisplay;
		szDisplay.cx = static_cast<int>(szPage.cx*nLogPixelsX*m_fZoom*0.01/nDPI);
		szDisplay.cy = static_cast<int>(szPage.cy*nLogPixelsY*m_fZoom*0.01/nDPI);
		return szDisplay;
	}
}

CSize CDjVuView::CalcZoomedPageSize(const CSize& szPage, CSize szBounds, int nZoomType) const
{
	szBounds.cx = max(szBounds.cx, 10);
	szBounds.cy = max(szBounds.cy, 10);

	CSize szDisplay(szBounds);

	switch (nZoomType)
	{
	case ZoomFitWidth:
		szDisplay.cy = szDisplay.cx * szPage.cy / szPage.cx;
		break;

	case ZoomFitHeight:
		szDisplay.cx = szDisplay.cy * szPage.cx / szPage.cy;
		break;

	case ZoomFitPage:
		szDisplay.cx = szDisplay.cy * szPage.cx / szPage.cy;

		if (szDisplay.cx > szBounds.cx)
		{
			szDisplay.cx = szBounds.cx;
			szDisplay.cy = szDisplay.cx * szPage.cy / szPage.cx;
		}
		break;

	case ZoomActualSize:
		szDisplay = szPage;
		break;

	case ZoomStretch:
		break;

	case ZoomPercent:
	default:
		ASSERT(false);
	}

	return szDisplay;
}

void CDjVuView::CalcPageSizeFacing(const CSize& szPage1, int nDPI1, CSize& szDisplay1,
		const CSize& szPage2, int nDPI2, CSize& szDisplay2) const
{
	CalcPageSizeFacing(szPage1, nDPI1, szDisplay1, szPage2, nDPI2, szDisplay2, m_nZoomType);
}

void CDjVuView::CalcPageSizeFacing(const CSize& szPage1, int nDPI1, CSize& szDisplay1,
		const CSize& szPage2, int nDPI2, CSize& szDisplay2, int nZoomType) const
{
	bool bFirstPageOk = (szPage1.cx > 0 && szPage1.cy > 0);
	bool bSecondPageOk = (szPage2.cx > 0 && szPage2.cy > 0);

	if (!bFirstPageOk)
		szDisplay1 = CSize(1, 1);
	if (!bSecondPageOk)
		szDisplay2 = CSize(1, 1);
	if (!bFirstPageOk && !bSecondPageOk)
		return;

	if (nZoomType <= ZoomFitWidth && nZoomType >= ZoomStretch)
	{
		int nFrame = m_nMargin + 2*m_nPageBorder + m_nPageShadow + m_nShadowMargin;
		CSize szFrame(nFrame + m_nFacingGap + 2*m_nPageBorder + m_nPageShadow, nFrame);

		CRect rcClient;
		GetClientRect(rcClient);

		CSize szBounds = rcClient.Size() - szFrame;
		CSize szHalf(szBounds.cx / 2, szBounds.cy);

		if (bFirstPageOk)
			szDisplay1 = CalcZoomedPageSize(szPage1, szHalf, nZoomType);
		if (bSecondPageOk)
			szDisplay2 = CalcZoomedPageSize(szPage2, szHalf, nZoomType);
	}
	else
	{
		CScreenDC dcScreen;
		int nLogPixelsX = dcScreen.GetDeviceCaps(LOGPIXELSX);
		int nLogPixelsY = dcScreen.GetDeviceCaps(LOGPIXELSX);

		if (bFirstPageOk)
		{
			szDisplay1.cx = static_cast<int>(szPage1.cx*nLogPixelsX*m_fZoom*0.01/nDPI1);
			szDisplay1.cy = static_cast<int>(szPage1.cy*nLogPixelsY*m_fZoom*0.01/nDPI1);
		}

		if (bSecondPageOk)
		{
			szDisplay2.cx = static_cast<int>(szPage2.cx*nLogPixelsX*m_fZoom*0.01/nDPI2);
			szDisplay2.cy = static_cast<int>(szPage2.cy*nLogPixelsY*m_fZoom*0.01/nDPI2);
		}
	}
}

CSize CDjVuView::UpdatePageRect(int nPage)
{
	PreparePageRect(nPage);

	Page& page = m_pages[nPage];
	page.ptOffset.Offset(m_nMargin, m_nMargin);
	page.rcDisplay.InflateRect(0, 0, m_nMargin + m_nShadowMargin, m_nMargin + m_nShadowMargin);

	CRect rcClient;
	GetClientRect(rcClient);

	if (page.rcDisplay.Width() < rcClient.Width())
	{
		page.ptOffset.x += (rcClient.Width() - page.rcDisplay.Width())/2;
		page.rcDisplay.right = rcClient.Width();
	}

	if (page.rcDisplay.Height() < rcClient.Height())
	{
		page.ptOffset.y += (rcClient.Height() - page.rcDisplay.Height())/2;
		page.rcDisplay.bottom = rcClient.Height();
	}

	if (page.info.bDecoded)
		page.bHasSize = true;

	return page.rcDisplay.Size();
}

void CDjVuView::PreparePageRect(int nPage)
{
	// Calculates page rectangle, but neither adds borders,
	// nor centers the page in the view, and puts it at (0, 0)

	Page& page = m_pages[nPage];
	CSize szPage = page.GetSize(m_nRotate);

	page.ptOffset = CPoint(m_nPageBorder, m_nPageBorder);
	page.szBitmap = CalcPageSize(szPage, page.info.nDPI);
	page.rcDisplay = CRect(CPoint(0, 0),
		page.szBitmap + CSize(2*m_nPageBorder + m_nPageShadow, 2*m_nPageBorder + m_nPageShadow));
}

CSize CDjVuView::UpdatePageRectFacing(int nPage)
{
	PreparePageRectFacing(nPage);

	Page& page = m_pages[nPage];
	Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);

	page.ptOffset.Offset(m_nMargin, m_nMargin);
	page.rcDisplay.InflateRect(0, 0, m_nMargin, m_nMargin + m_nShadowMargin);

	CSize szDisplay;

	if (pNextPage != NULL)
	{
		pNextPage->ptOffset.Offset(m_nMargin, m_nMargin);
		pNextPage->rcDisplay.OffsetRect(m_nMargin, 0);
		pNextPage->rcDisplay.InflateRect(0, 0, m_nShadowMargin, m_nMargin + m_nShadowMargin);

		szDisplay = pNextPage->rcDisplay.BottomRight();
	}
	else
	{
		page.rcDisplay.right += m_nShadowMargin;

		szDisplay = page.rcDisplay.BottomRight();
	}

	CRect rcClient;
	GetClientRect(rcClient);

	// Center horizontally
	if (szDisplay.cx < rcClient.Width())
	{
		page.ptOffset.x += (rcClient.Width() - szDisplay.cx)/2;
		page.rcDisplay.InflateRect(0, 0, (rcClient.Width() - szDisplay.cx)/2, 0);
		if (pNextPage != NULL)
		{
			pNextPage->ptOffset.x += (rcClient.Width() - szDisplay.cx)/2;
			pNextPage->rcDisplay.OffsetRect((rcClient.Width() - szDisplay.cx)/2, 0);
			pNextPage->rcDisplay.right = rcClient.Width();
		}
		else
		{
			page.rcDisplay.right = rcClient.Width();
		}

		szDisplay.cx = rcClient.Width();
	}

	// Center vertically
	if (szDisplay.cy < rcClient.Height())
	{
		page.ptOffset.y += (rcClient.Height() - szDisplay.cy)/2;
		if (pNextPage != NULL)
			pNextPage->ptOffset.y += (rcClient.Height() - szDisplay.cy)/2;

		szDisplay.cy = rcClient.Height();
	}

	page.rcDisplay.bottom = szDisplay.cy;
	if (pNextPage != NULL)
		pNextPage->rcDisplay.bottom = szDisplay.cy;

	if (page.info.bDecoded)
		page.bHasSize = true;
	if (pNextPage != NULL && pNextPage->info.bDecoded)
		pNextPage->bHasSize = true;

	return szDisplay;
}

void CDjVuView::PreparePageRectFacing(int nPage)
{
	// Calculates page rectangles, but neither adds borders,
	// nor centers pages in the view, and puts them at (0, 0)

	Page& page = m_pages[nPage];
	Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);
	CSize szPage = page.GetSize(m_nRotate);

	if (pNextPage != NULL)
	{
		CSize szNextPage = pNextPage->GetSize(m_nRotate);
		CalcPageSizeFacing(szPage, page.info.nDPI, page.szBitmap,
				szNextPage, pNextPage->info.nDPI, pNextPage->szBitmap);
	}
	else
	{
		CalcPageSizeFacing(szPage, page.info.nDPI, page.szBitmap,
				szPage, page.info.nDPI, page.szBitmap);
	}

	page.ptOffset = CPoint(m_nPageBorder, m_nPageBorder);
	page.rcDisplay = CRect(CPoint(0, 0),
		page.szBitmap + CSize(2*m_nPageBorder + m_nPageShadow, 2*m_nPageBorder + m_nPageShadow));

	if (pNextPage != NULL)
	{
		pNextPage->ptOffset = CPoint(page.rcDisplay.right + m_nFacingGap + m_nPageBorder, m_nPageBorder);
		pNextPage->rcDisplay = CRect(CPoint(page.rcDisplay.right, 0),
			pNextPage->szBitmap + CSize(m_nFacingGap + 2*m_nPageBorder + m_nPageShadow, 2*m_nPageBorder + m_nPageShadow));

		// Align pages vertically
		if (page.rcDisplay.bottom < pNextPage->rcDisplay.bottom)
		{
			page.ptOffset.y += (pNextPage->rcDisplay.bottom - page.rcDisplay.bottom) / 2;
			page.rcDisplay.bottom = pNextPage->rcDisplay.bottom;
		}
		else if (page.rcDisplay.bottom > pNextPage->rcDisplay.bottom)
		{
			pNextPage->ptOffset.y += (page.rcDisplay.bottom - pNextPage->rcDisplay.bottom) / 2;
			pNextPage->rcDisplay.bottom = page.rcDisplay.bottom;
		}
	}
	else if (nPage == 0 && m_bFirstPageAlone)
	{
		// Special case
		page.ptOffset.x += page.szBitmap.cx + m_nFacingGap + 2*m_nPageBorder + m_nPageShadow;

		page.rcDisplay.right = 2*page.rcDisplay.Width() + m_nFacingGap;
	}
	else
	{
		page.rcDisplay.right = 2*page.rcDisplay.Width() + m_nFacingGap;
	}
}

void CDjVuView::OnViewNextpage()
{
	int nPage = GetNextPage(m_nPage);

	if (nPage < m_nPageCount)
		RenderPage(nPage);
	else
		OnViewLastpage();
}

void CDjVuView::OnUpdateViewNextpage(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsViewNextpageEnabled());
}

bool CDjVuView::IsViewNextpageEnabled()
{
	if (m_nLayout == SinglePage || m_nLayout == Facing)
		return GetNextPage(m_nPage) < m_nPageCount;
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		return GetScrollPos(SB_VERT) < GetScrollLimit(SB_VERT);
	else
		return false;
}

void CDjVuView::OnViewPreviouspage()
{
	int nPage = m_nPage - 1;
	if (m_nLayout == Facing || m_nLayout == ContinuousFacing)
		--nPage;

	nPage = max(0, nPage);
	RenderPage(nPage);
}

void CDjVuView::OnUpdateViewPreviouspage(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsViewPreviouspageEnabled());
}

bool CDjVuView::IsViewPreviouspageEnabled() const
{
	if (m_nLayout == SinglePage || m_nLayout == Facing)
		return m_nPage > 0;
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		return GetScrollPos(SB_VERT) > 0;
	else
		return false;
}

void CDjVuView::OnSize(UINT nType, int cx, int cy)
{
	if (nType != SIZE_MINIMIZED && cx > 0 && cy > 0
			&& m_nPage != -1 && !m_bInsideUpdateLayout)
	{
		if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
			UpdatePageSizes(GetScrollPos(SB_VERT));

		UpdateLayout();
		Invalidate();
	}

	CMyScrollView::OnSize(nType, cx, cy);
}

void CDjVuView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CSize szScroll(0, 0);
	bool bScroll = true;
	bool bNextPage = false;
	bool bPrevPage = false;

	BOOL bHasHorzBar, bHasVertBar;
	CheckScrollBars(bHasHorzBar, bHasVertBar);

	switch (nChar)
	{
	case 'G':
	case 'g':
		GetTopLevelParent()->SendMessage(WM_COMMAND, ID_VIEW_GOTO_PAGE);
		return;

	case VK_HOME:
		OnViewFirstpage();
		return;

	case VK_END:
		OnViewLastpage();
		return;

	case VK_ADD:
		OnViewZoomIn();
		return;

	case VK_SUBTRACT:
		OnViewZoomOut();
		return;

	case VK_MULTIPLY:
		OnViewZoom(ID_ZOOM_FITPAGE);
		return;

	case VK_DOWN:
		szScroll.cy = 3*m_lineDev.cy;
		bNextPage = !bHasVertBar;
		break;

	case VK_UP:
		szScroll.cy = -3*m_lineDev.cy;
		bPrevPage = !bHasVertBar;
		break;

	case VK_RIGHT:
		szScroll.cx = 3*m_lineDev.cx;
		bNextPage = !bHasHorzBar;
		break;

	case VK_LEFT:
		szScroll.cx = -3*m_lineDev.cx;
		bPrevPage = !bHasHorzBar;
		break;

	case VK_NEXT:
	case VK_SPACE:
		szScroll.cy = m_pageDev.cy;
		bNextPage = true;
		break;

	case VK_PRIOR:
	case VK_BACK:
		szScroll.cy = -m_pageDev.cy;
		bPrevPage = true;
		break;

	default:
		bScroll = false;
		break;
	}

	if (!bScroll)
	{
		CMyScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if ((m_nLayout == Continuous || m_nLayout == ContinuousFacing) && szScroll.cy != 0)
		UpdatePageSizes(GetScrollPos(SB_VERT), szScroll.cy);

	if (!OnScrollBy(szScroll))
	{
		if (bNextPage && IsViewNextpageEnabled())
		{
			OnViewNextpage();
		}
		else if (bPrevPage && IsViewPreviouspageEnabled())
		{
			CPoint ptScroll = GetScrollPosition();
			OnViewPreviouspage();

			if (m_nLayout == SinglePage || m_nLayout == Facing)
				OnScrollBy(CPoint(ptScroll.x, m_szDisplay.cy) - GetScrollPosition());
		}
	}

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdateVisiblePages();
		UpdatePageNumber();
	}

	UpdateDragAction();

	CMyScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDjVuView::OnViewZoom(UINT nID)
{
	switch (nID)
	{
	case ID_ZOOM_50:
		ZoomTo(ZoomPercent, 50.0);
		break;

	case ID_ZOOM_75:
		ZoomTo(ZoomPercent, 75.0);
		break;

	case ID_ZOOM_100:
		ZoomTo(ZoomPercent, 100.0);
		break;

	case ID_ZOOM_200:
		ZoomTo(ZoomPercent, 200.0);
		break;

	case ID_ZOOM_400:
		ZoomTo(ZoomPercent, 400.0);
		break;

	case ID_ZOOM_FITWIDTH:
		ZoomTo(ZoomFitWidth);
		break;

	case ID_ZOOM_FITHEIGHT:
		ZoomTo(ZoomFitHeight);
		break;

	case ID_ZOOM_FITPAGE:
		ZoomTo(ZoomFitPage);
		break;

	case ID_ZOOM_ACTUALSIZE:
		ZoomTo(ZoomActualSize);
		break;

	case ID_ZOOM_STRETCH:
		ZoomTo(ZoomStretch);
		break;

	case ID_ZOOM_CUSTOM:
		{
			// Leave two digits after decimal point
			double fZoom = static_cast<int>(GetZoom()*100.0)*0.01;

			CCustomZoomDlg dlg(fZoom);
			if (dlg.DoModal() == IDOK)
				ZoomTo(ZoomPercent, dlg.m_fZoom);
		}
		break;
	}
}

void CDjVuView::OnUpdateViewZoom(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_ZOOM_50:
		pCmdUI->SetCheck(m_nZoomType == ZoomPercent && m_fZoom == 50.0);
		break;

	case ID_ZOOM_75:
		pCmdUI->SetCheck(m_nZoomType == ZoomPercent && m_fZoom == 75.0);
		break;

	case ID_ZOOM_100:
		pCmdUI->SetCheck(m_nZoomType == ZoomPercent && m_fZoom == 100.0);
		break;

	case ID_ZOOM_200:
		pCmdUI->SetCheck(m_nZoomType == ZoomPercent && m_fZoom == 200.0);
		break;

	case ID_ZOOM_400:
		pCmdUI->SetCheck(m_nZoomType == ZoomPercent && m_fZoom == 400.0);
		break;

	case ID_ZOOM_FITWIDTH:
		pCmdUI->SetCheck(m_nZoomType == ZoomFitWidth);
		break;

	case ID_ZOOM_FITHEIGHT:
		pCmdUI->SetCheck(m_nZoomType == ZoomFitHeight);
		break;

	case ID_ZOOM_FITPAGE:
		pCmdUI->SetCheck(m_nZoomType == ZoomFitPage);
		break;

	case ID_ZOOM_ACTUALSIZE:
		pCmdUI->SetCheck(m_nZoomType == ZoomActualSize);
		break;

	case ID_ZOOM_STRETCH:
		pCmdUI->SetCheck(m_nZoomType == ZoomStretch);
		break;

	case ID_ZOOM_CUSTOM:
		pCmdUI->SetCheck(!IsStandardZoom(m_nZoomType, m_fZoom));
		break;
	}
}

double CDjVuView::GetZoom() const
{
	if (m_nZoomType == ZoomPercent)
		return m_fZoom;

	const Page& page = m_pages[m_nPage];
	CSize szPage = page.GetSize(m_nRotate);

	if (szPage.cx <= 0 || szPage.cy <= 0)
		return 100.0;

	// Calculate zoom from display area size
	CScreenDC dcScreen;
	int nLogPixels = dcScreen.GetDeviceCaps(LOGPIXELSX);

	return 100.0*page.szBitmap.cx*page.info.nDPI/(szPage.cx*nLogPixels);
}

double CDjVuView::GetZoom(ZoomType nZoomType) const
{
	int nPage = m_nPage;
	const Page& page = m_pages[nPage];
	CSize szPage = page.GetSize(m_nRotate);

	// Calculate zoom from display area size
	CSize szDisplay;

	CScreenDC dcScreen;
	int nLogPixels = dcScreen.GetDeviceCaps(LOGPIXELSX);

	if (m_nLayout == SinglePage || m_nLayout == Continuous)
	{
		szDisplay = CalcPageSize(szPage, page.info.nDPI, nZoomType);
	}
	else if (m_nLayout == Facing || m_nLayout == ContinuousFacing)
	{
		const Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);
		CSize szPage = page.GetSize(m_nRotate);

		if (pNextPage != NULL)
		{
			CSize szPage2 = pNextPage->GetSize(m_nRotate);
			CSize szDisplay2;
			CalcPageSizeFacing(szPage, page.info.nDPI, szDisplay,
					szPage2, pNextPage->info.nDPI, szDisplay2, nZoomType);

			if ((szPage.cx <= 0 || szPage.cy <= 0) && szPage2.cx > 0 && szPage2.cy > 0)
				return 100.0*szDisplay2.cx*pNextPage->info.nDPI/(szPage2.cx*nLogPixels);
		}
		else
		{
			CalcPageSizeFacing(szPage, page.info.nDPI, szDisplay,
					szPage, page.info.nDPI, szDisplay, nZoomType);
		}
	}

	if (szPage.cx <= 0 || szPage.cy <= 0)
		return 100.0;

	return 100.0*szDisplay.cx*page.info.nDPI/(szPage.cx*nLogPixels);
}

void CDjVuView::OnViewLayout(UINT nID)
{
	int nPrevLayout = m_nLayout;

	int nAnchorPage = m_nPage;
	int nOffset = GetScrollPos(SB_VERT) - m_pages[nAnchorPage].ptOffset.y;

	switch (nID)
	{
	case ID_LAYOUT_SINGLEPAGE:
		m_nLayout = SinglePage;
		break;

	case ID_LAYOUT_CONTINUOUS:
		m_nLayout = Continuous;
		break;

	case ID_LAYOUT_FACING:
		m_nLayout = Facing;
		break;

	case ID_LAYOUT_CONTINUOUS_FACING:
		m_nLayout = ContinuousFacing;
		break;
	}

	if (m_nType == Normal)
	{
		theApp.GetAppSettings()->nDefaultLayout = m_nLayout;
		m_pSource->GetSettings()->nLayout = m_nLayout;
	}

	if (m_nLayout != nPrevLayout)
		SetLayout(m_nLayout, nAnchorPage, nOffset);
}

void CDjVuView::OnUpdateViewLayout(CCmdUI* pCmdUI)
{
	if (m_nType != Normal)
	{
		pCmdUI->Enable(false);
		return;
	}

	switch (pCmdUI->m_nID)
	{
	case ID_LAYOUT_SINGLEPAGE:
		pCmdUI->SetCheck(m_nLayout == SinglePage);
		break;

	case ID_LAYOUT_CONTINUOUS:
		pCmdUI->SetCheck(m_nLayout == Continuous);
		break;

	case ID_LAYOUT_FACING:
		pCmdUI->SetCheck(m_nLayout == Facing);
		break;

	case ID_LAYOUT_CONTINUOUS_FACING:
		pCmdUI->SetCheck(m_nLayout == ContinuousFacing);
		break;
	}
}

void CDjVuView::OnRotateLeft()
{
	m_nRotate = (m_nRotate + 1) % 4;
	if (m_nType == Normal)
		m_pSource->GetSettings()->nRotate = m_nRotate;

	DeleteBitmaps();
	m_pRenderThread->RejectCurrentJob();

	UpdateLayout();

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePageSizes(GetScrollPos(SB_VERT));

	Invalidate();
	UpdateHoverAnnotation();
	UpdateCursor();

	UpdateObservers(RotateChanged(m_nRotate));
}

void CDjVuView::OnRotateRight()
{
	m_nRotate = (m_nRotate + 3) % 4;
	if (m_nType == Normal)
		m_pSource->GetSettings()->nRotate = m_nRotate;

	DeleteBitmaps();
	m_pRenderThread->RejectCurrentJob();

	UpdateLayout();

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePageSizes(GetScrollPos(SB_VERT));

	Invalidate();
	UpdateHoverAnnotation();
	UpdateCursor();

	UpdateObservers(RotateChanged(m_nRotate));
}

void CDjVuView::OnRotate180()
{
	m_nRotate = (m_nRotate + 2) % 4;
	if (m_nType == Normal)
		m_pSource->GetSettings()->nRotate = m_nRotate;

	DeleteBitmaps();
	m_pRenderThread->RejectCurrentJob();

	UpdateLayout();
	Invalidate();

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePageSizes(GetScrollPos(SB_VERT));

	UpdateObservers(RotateChanged(m_nRotate));
}

void CDjVuView::OnViewFirstpage()
{
	GetMainFrame()->AddToHistory(this);

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		RenderPage(0);
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		UpdatePagesFromTop(0, rcClient.Height());

		OnScrollBy(CPoint(GetScrollPos(SB_HORZ), 0) - GetScrollPosition());
		UpdateVisiblePages();
		UpdatePageNumber();
	}

	GetMainFrame()->AddToHistory(this);
}

void CDjVuView::OnViewLastpage()
{
	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		RenderPage(m_nPageCount - 1);
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		UpdatePagesFromBottom(m_szDisplay.cy - rcClient.Height(), m_szDisplay.cy);

		OnScrollBy(CSize(0, m_szDisplay.cy));
		UpdateVisiblePages();
		UpdatePageNumber();
	}
}

void CDjVuView::OnSetFocus(CWnd* pOldWnd)
{
	CMyScrollView::OnSetFocus(pOldWnd);

	if (m_nType == Normal)
	{
		UpdateObservers(VIEW_ACTIVATED);
	}

	m_nClickCount = 0;
}

void CDjVuView::OnKillFocus(CWnd* pNewWnd)
{
	CMyScrollView::OnKillFocus(pNewWnd);

	StopDragging();
}

void CDjVuView::OnCancelMode()
{
	CMyScrollView::OnCancelMode();

	StopDragging();
}

void CDjVuView::ZoomTo(int nZoomType, double fZoom)
{
	m_nZoomType = nZoomType;
	m_fZoom = fZoom;

	if (m_nZoomType == ZoomPercent)
	{
		// Leave two digits after decimal point
		m_fZoom = static_cast<int>(m_fZoom*100.0)*0.01;
		m_fZoom = max(min(m_fZoom, 800.0), 10.0);
	}

	if (m_nType == Normal)
	{
		theApp.GetAppSettings()->nDefaultZoomType = nZoomType;
		theApp.GetAppSettings()->fDefaultZoom = fZoom;
		m_pSource->GetSettings()->nZoomType = nZoomType;
		m_pSource->GetSettings()->fZoom = fZoom;
	}

	UpdateLayout();

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePageSizes(GetScrollPos(SB_VERT));

	Invalidate();
	UpdateHoverAnnotation();
	UpdateCursor();

	UpdateObservers(ZOOM_CHANGED);

	UpdateDragAction();
}

BOOL CDjVuView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		if (hCursorHand == NULL)
			hCursorHand = theApp.LoadCursor(IDC_CURSOR_HAND);
		if (hCursorDrag == NULL)
			hCursorDrag = theApp.LoadCursor(IDC_CURSOR_DRAG);
		if (hCursorLink == NULL)
			hCursorLink = ::LoadCursor(0, IDC_HAND);
		if (hCursorLink == NULL)
			hCursorLink = theApp.LoadCursor(IDC_CURSOR_LINK);
		if (hCursorText == NULL)
			hCursorText = ::LoadCursor(0, IDC_IBEAM);
		if (hCursorMagnify == NULL)
			hCursorMagnify = theApp.LoadCursor(IDC_CURSOR_MAGNIFY);
		if (hCursorCross == NULL)
			hCursorCross = ::LoadCursor(0, IDC_CROSS);
		if (hCursorZoomRect == NULL)
			hCursorZoomRect = theApp.LoadCursor(IDC_CURSOR_ZOOM_RECT);

		CRect rcClient;
		GetClientRect(rcClient);

		CPoint ptCursor;
		::GetCursorPos(&ptCursor);
		ScreenToClient(&ptCursor);

		int nPage = GetPageFromPoint(ptCursor);

		HCURSOR hCursor = NULL;
		if (hCursor == NULL && (m_bDraggingMagnify || m_nType == Magnify))
			hCursor = hCursorMagnify;
		if (hCursor == NULL && m_bDraggingPage)
			hCursor = hCursorDrag;
		if (hCursor == NULL && m_bDraggingText)
			hCursor = hCursorText;
		if (hCursor == NULL && m_bDraggingRect)
			hCursor = (m_nMode == ZoomRect ? hCursorZoomRect : hCursorCross);
		if (hCursor == NULL && m_bDraggingLink)
			hCursor = hCursorLink;

		if (hCursor == NULL && (m_bControlDown || (m_nMode == MagnifyingGlass && !m_bShiftDown)))
			hCursor = hCursorMagnify;
		if (hCursor == NULL && m_pHoverAnno != NULL && m_pHoverAnno->strURL.length() != 0)
			hCursor = hCursorLink;
		if (hCursor == NULL && (m_bShiftDown || m_nMode == Drag)
				&& (m_totalDev.cx > rcClient.Width() || m_totalDev.cy > rcClient.Height()))
			hCursor = m_bDragging ? hCursorDrag : hCursorHand;
		if (hCursor == NULL && m_nMode == SelectRect)
			hCursor = hCursorCross;
		if (hCursor == NULL && m_nMode == ZoomRect)
			hCursor = hCursorZoomRect;
		if (hCursor == NULL && m_nMode == Select && nPage != -1)
		{
			const Page& page = m_pages[nPage];
			CRect rcBitmap(CPoint(page.ptOffset - GetScrollPosition()), page.szBitmap);

			if (rcBitmap.PtInRect(ptCursor) && m_pages[nPage].info.bHasText)
				hCursor = hCursorText;
		}

		if (hCursor != NULL)
		{
			SetCursor(hCursor);
			return true;
		}
	}

	return CMyScrollView::OnSetCursor(pWnd, nHitTest, message);
}

void CDjVuView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		StopDragging();
		m_nCursorTime = ::GetTickCount();
	}

	CRect rcClient;
	GetClientRect(rcClient);

	m_bClick = true;
	++m_nClickCount;
	::GetCursorPos(&m_ptClick);

	if (m_bControlDown || (m_nMode == MagnifyingGlass && !m_bShiftDown))
	{
		UpdateHoverAnnotation(CPoint(-1, -1));

		m_bDragging = true;
		m_bDraggingMagnify = true;

		m_ptPrevCursor = CPoint(-1, -1);

		StartMagnify();
		UpdateMagnifyWnd();

		SetCapture();
		ShowCursor();
		UpdateCursor();
	}
	else if (m_pHoverAnno != NULL && m_pHoverAnno->strURL.length() != 0)
	{
		m_bDragging = true;
		m_bDraggingLink = true;

		SetCapture();
		ShowCursor();
		UpdateCursor();
	}
	else if ((m_bShiftDown || m_nMode == Drag)
			&& (m_totalDev.cx > rcClient.Width() || m_totalDev.cy > rcClient.Height()))
	{
		UpdateHoverAnnotation(CPoint(-1, -1));

		m_nStartPage = GetPageNearPoint(point);
		if (m_nStartPage == -1)
			return;

		m_bDragging = true;
		m_bDraggingPage = true;

		m_ptStartPos = GetScrollPosition() - m_pages[m_nStartPage].ptOffset;
		::GetCursorPos(&m_ptStart);

		SetCapture();
		ShowCursor();
		UpdateCursor();
	}
	else if (m_nMode == Select)
	{
		UpdateHoverAnnotation(CPoint(-1, -1));
		ClearSelection();

		m_nStartPage = GetPageNearPoint(point);
		if (m_nStartPage == -1)
			return;

		m_bDragging = true;
		m_bDraggingText = true;

		m_nPrevPage = m_nStartPage;
		m_nSelStartPos = GetTextPosFromPoint(m_nStartPage, point);

		SetCapture();
		ShowCursor();
		UpdateCursor();
	}
	else if (m_nMode == SelectRect || m_nMode == ZoomRect)
	{
		UpdateHoverAnnotation(CPoint(-1, -1));
		ClearSelection();

		m_nStartPage = GetPageNearPoint(point);
		if (m_nStartPage == -1)
			return;

		Page& page = m_pages[m_nStartPage];
		CPoint pt = point + GetScrollPosition() - page.ptOffset;
		m_ptStart = ScreenToDjVu(m_nStartPage, pt);

		m_nSelectionPage = m_nStartPage;
		m_rcSelection.xmin = m_ptStart.x;
		m_rcSelection.xmax = m_ptStart.x + 1;
		m_rcSelection.ymin = m_ptStart.y - 1;
		m_rcSelection.ymax = m_ptStart.y;

		m_bDragging = true;
		m_bDraggingRect = true;

		SetCapture();
		ShowCursor();
		UpdateCursor();
	}
	else if (m_nMode == NextPrev && !m_bShiftDown)
	{
		UpdateHoverAnnotation(CPoint(-1, -1));

		m_bDragging = true;
		SetCapture();

		OnViewNextpage();
	}

	CMyScrollView::OnLButtonDown(nFlags, point);
}

CPoint CDjVuView::ScreenToDjVu(int nPage, const CPoint& point, bool bClip) const
{
	const Page& page = m_pages[nPage];
	CSize szPage = page.GetSize(m_nRotate);

	double fRatioX = szPage.cx / (1.0*page.szBitmap.cx);
	double fRatioY = szPage.cy / (1.0*page.szBitmap.cy);

	CPoint ptResult(static_cast<int>(point.x * fRatioX + 0.5),
			static_cast<int>(szPage.cy - point.y * fRatioY + 0.5));

	GRect output(0, 0, page.info.szPage.cx, page.info.szPage.cy);

	int nRotate = (m_nRotate + page.info.nInitialRotate) % 4;
	if (nRotate != 0)
	{
		GRect rect(ptResult.x, ptResult.y);

		GRect input(0, 0, szPage.cx, szPage.cy);

		if ((page.info.nInitialRotate & 1) != 0)
			swap(output.xmax, output.ymax);

		GRectMapper mapper;
		mapper.clear();
		mapper.set_input(input);
		mapper.set_output(output);               
		mapper.rotate(4 - nRotate);
		mapper.map(rect);

		ptResult = CPoint(rect.xmin, rect.ymin);
	}

	if (bClip)
	{
		ptResult.x = max(min(ptResult.x, output.xmax - 1), 0);
		ptResult.y = max(min(ptResult.y, output.ymax - 1), 0);
	}

	return ptResult;
}

void CDjVuView::GetTextPosFromTop(DjVuTXT::Zone& zone, const CPoint& pt, int& nPos) const
{
	if (zone.rect.xmin > pt.x || zone.rect.ymax < pt.y)
		return;

	if (zone.rect.xmax < pt.x || zone.rect.ymin > pt.y)
	{
		int nEnd = zone.text_start + zone.text_length;
		if (nPos == -1 || nPos < nEnd)
			nPos = nEnd;
		return;
	}

	if (zone.children.isempty()) 
	{
		nPos = zone.text_start;
	}
	else
	{
		for (GPosition pos = zone.children; pos; ++pos)
			GetTextPosFromTop(zone.children[pos], pt, nPos);
	}
}

void CDjVuView::GetTextPosFromBottom(DjVuTXT::Zone& zone,  const CPoint& pt, int& nPos) const
{
	if (zone.rect.xmax < pt.x || zone.rect.ymin > pt.y)
		return;

	if (zone.rect.xmin > pt.x || zone.rect.ymax < pt.y)
	{
		if (nPos == -1 || nPos > zone.text_start)
			nPos = zone.text_start;
		return;
	}

	if (zone.children.isempty()) 
	{
		nPos = zone.text_start;
	}
	else
	{
		for (GPosition pos = zone.children; pos; ++pos)
			GetTextPosFromBottom(zone.children[pos], pt, nPos);
	}
}

int CDjVuView::GetTextPosFromPoint(int nPage, const CPoint& point)
{
	Page& page = m_pages[nPage];
	if (page.info.bHasText && !page.info.bTextDecoded)
		page.Init(m_pSource, nPage, true);

	if (page.info.pText == NULL)
		return 0;

	CPoint ptPage = point + GetScrollPosition() - page.ptOffset;
	CPoint ptDjVu = ScreenToDjVu(nPage, ptPage);

	int nPos = -1;
	GetTextPosFromTop(page.info.pText->page_zone, ptDjVu, nPos);

	if (nPos == -1)
		GetTextPosFromBottom(page.info.pText->page_zone, ptDjVu, nPos);

	if (nPos == -1)
		nPos = page.info.pText->textUTF8.length();

	return nPos;
}

void CDjVuView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDraggingLink && m_pHoverAnno != NULL)
	{
		Annotation* pClickedAnno = m_pHoverAnno;

		UpdateHoverAnnotation(point);

		if (pClickedAnno == m_pHoverAnno && pClickedAnno->strURL.length() != 0)
			GoToURL(pClickedAnno->strURL);
	}

	if (m_bDragging)
	{
		StopDragging();
		m_nCursorTime = ::GetTickCount();
	}

	UpdateHoverAnnotation(point);

	if (m_bClick)
	{
		m_bClick = false;
		if (m_nClickCount > 1)
			ClearSelection();
	}

	CMyScrollView::OnLButtonUp(nFlags, point);
}

void CDjVuView::StopDragging()
{
	if (m_bDragging)
	{
		m_bDragging = false;
		m_bDraggingPage = false;
		m_bDraggingText = false;
		m_bDraggingLink = false;

		if (m_bDraggingMagnify)
		{
			GetMainFrame()->GetMagnifyWnd()->Hide();
			m_bDraggingMagnify = false;
		}

		if (m_bDraggingRect)
		{
			if (m_rcSelection.width() <= 1 || m_rcSelection.height() <= 1)
				m_nSelectionPage = -1;
			m_bDraggingRect = false;

			if (m_nMode == ZoomRect && m_nSelectionPage != -1)
			{
				OnZoomToSelection();

				CRect rcDisplay = TranslatePageRect(m_nSelectionPage, m_rcSelection);
				rcDisplay.OffsetRect(-GetScrollPosition());
				rcDisplay.InflateRect(0, 0, 1, 1);
				InvalidateRect(rcDisplay);

				m_nSelectionPage = -1;
			}
		}

		ReleaseCapture();
	}
}

void CDjVuView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (point != m_ptMouse)
	{
		m_ptMouse = point;
		ShowCursor();
	}

	if (m_bClick)
	{
		CPoint ptCursor;
		::GetCursorPos(&ptCursor);
		if (m_ptClick != ptCursor)
			m_bClick = false;
	}

	if (!m_bDragging)
	{
		UpdateHoverAnnotation(point);
		return;
	}

	if (m_bDraggingPage)
	{
		CPoint ptCursor;
		::GetCursorPos(&ptCursor);

		CPoint ptStartPos = m_pages[m_nStartPage].ptOffset + m_ptStartPos;
		CPoint ptScroll = ptStartPos + m_ptStart - ptCursor - GetScrollPosition();

		if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		{
			int y = GetScrollPos(SB_VERT);
			UpdatePageSizes(y, ptScroll.y);

			ptStartPos = m_pages[m_nStartPage].ptOffset + m_ptStartPos;
			ptScroll = ptStartPos + m_ptStart - ptCursor - GetScrollPosition();
		}

		if (ptScroll != CPoint(0, 0))
			m_bClick = false;

		OnScrollBy(ptScroll);

		if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		{
			UpdateVisiblePages();
			UpdatePageNumber();
		}

		UpdateWindow();

		return;
	}
	else if (m_bDraggingText)
	{
		int nCurPage = GetPageNearPoint(point);
		if (nCurPage == -1)
			return;

		int nSelPos = GetTextPosFromPoint(nCurPage, point);
		bool bInfoLoaded = false;
		CWaitCursor* pWaitCursor = NULL;

		int nPage;

		if (nCurPage < m_nStartPage)
		{
			if (m_nPrevPage >= m_nStartPage)
			{
				for (nPage = m_nStartPage; nPage <= m_nPrevPage; ++nPage)
					ClearSelection(nPage);

				SelectTextRange(nCurPage, nSelPos, -1, bInfoLoaded, pWaitCursor);
				for (nPage = nCurPage + 1; nPage < m_nStartPage; ++nPage)
					SelectTextRange(nPage, 0, -1, bInfoLoaded, pWaitCursor);
				SelectTextRange(m_nStartPage, 0, m_nSelStartPos, bInfoLoaded, pWaitCursor);
			}
			else if (m_nPrevPage <= nCurPage)
			{
				for (nPage = m_nPrevPage; nPage <= nCurPage; ++nPage)
					ClearSelection(nPage);

				SelectTextRange(nCurPage, nSelPos, -1, bInfoLoaded, pWaitCursor);
			}
			else
			{
				ClearSelection(m_nPrevPage);

				SelectTextRange(nCurPage, nSelPos, -1, bInfoLoaded, pWaitCursor);
				for (nPage = nCurPage + 1; nPage <= m_nPrevPage; ++nPage)
					SelectTextRange(nPage, 0, -1, bInfoLoaded, pWaitCursor);
			}
		}
		else if (nCurPage > m_nStartPage)
		{
			if (m_nPrevPage <= m_nStartPage)
			{
				for (nPage = m_nPrevPage; nPage <= m_nStartPage; ++nPage)
					ClearSelection(nPage);

				SelectTextRange(m_nStartPage, m_nSelStartPos, -1, bInfoLoaded, pWaitCursor);
				for (nPage = m_nStartPage + 1; nPage < nCurPage; ++nPage)
					SelectTextRange(nPage, 0, -1, bInfoLoaded, pWaitCursor);
				SelectTextRange(nCurPage, 0, nSelPos, bInfoLoaded, pWaitCursor);
			}
			else if (m_nPrevPage >= nCurPage)
			{
				for (nPage = nCurPage; nPage <= m_nPrevPage; ++nPage)
					ClearSelection(nPage);

				SelectTextRange(nCurPage, 0, nSelPos, bInfoLoaded, pWaitCursor);
			}
			else
			{
				ClearSelection(m_nPrevPage);

				for (nPage = m_nPrevPage; nPage < nCurPage; ++nPage)
					SelectTextRange(nPage, 0, -1, bInfoLoaded, pWaitCursor);
				SelectTextRange(nCurPage, 0, nSelPos, bInfoLoaded, pWaitCursor);
			}
		}
		else
		{
			if (m_nPrevPage < m_nStartPage)
			{
				for (nPage = m_nPrevPage; nPage <= m_nStartPage; ++nPage)
					ClearSelection(nPage);
			}
			else
			{
				for (nPage = m_nStartPage; nPage <= m_nPrevPage; ++nPage)
					ClearSelection(nPage);
			}

			if (nSelPos < m_nSelStartPos)
			{
				SelectTextRange(nCurPage, nSelPos, m_nSelStartPos, bInfoLoaded, pWaitCursor);
			}
			else if (nSelPos > m_nSelStartPos)
			{
				SelectTextRange(nCurPage, m_nSelStartPos, nSelPos, bInfoLoaded, pWaitCursor);
			}
		}

		m_nPrevPage = nCurPage;
		UpdateSelectionStatus();

		if (bInfoLoaded)
		{
			UpdateLayout();
			Invalidate();
		}

		if (pWaitCursor != NULL)
			delete pWaitCursor;
	}
	else if (m_bDraggingRect)
	{
		Page& page = m_pages[m_nSelectionPage];
		CPoint pt = point + GetScrollPosition() - page.ptOffset;
		CPoint ptCurrent = ScreenToDjVu(m_nStartPage, pt, true);

		CRect rcDisplay = TranslatePageRect(m_nSelectionPage, m_rcSelection);
		rcDisplay.OffsetRect(-GetScrollPosition());
		rcDisplay.InflateRect(0, 0, 1, 1);
		InvalidateRect(rcDisplay);

		m_rcSelection.xmin = min(ptCurrent.x, m_ptStart.x);
		m_rcSelection.xmax = max(ptCurrent.x, m_ptStart.x) + 1;
		m_rcSelection.ymin = min(ptCurrent.y, m_ptStart.y) - 1;
		m_rcSelection.ymax = max(ptCurrent.y, m_ptStart.y);

		rcDisplay = TranslatePageRect(m_nSelectionPage, m_rcSelection);
		rcDisplay.OffsetRect(-GetScrollPosition());
		rcDisplay.InflateRect(0, 0, 1, 1);
		InvalidateRect(rcDisplay);

		UpdateWindow();
	}
	else if (m_bDraggingMagnify)
	{
		UpdateMagnifyWnd();
		return;
	}

	CMyScrollView::OnMouseMove(nFlags, point);
}

void CDjVuView::OnMouseLeave()
{
	if (!m_bIgnoreMouseLeave)
		UpdateHoverAnnotation(CPoint(-1, -1));
}

void CDjVuView::SelectTextRange(int nPage, int nStart, int nEnd,
	bool& bInfoLoaded, CWaitCursor*& pWaitCursor)
{
	Page& page = m_pages[nPage];
	if (!page.info.bDecoded)
	{
		if (pWaitCursor == NULL)
			pWaitCursor = new CWaitCursor();

		page.Init(m_pSource, nPage);
		bInfoLoaded = true;
	}

	if (page.info.bHasText && !page.info.bTextDecoded)
	{
		if (pWaitCursor == NULL)
			pWaitCursor = new CWaitCursor();

		page.Init(m_pSource, nPage, true);
	}

	if (page.info.pText == NULL)
		return;

	if (nEnd == -1)
		nEnd = page.info.pText->textUTF8.length();

	DjVuSelection selection;
	FindSelectionZones(selection, page.info.pText, nStart, nEnd);

	page.selection = selection;
	if (!selection.isempty())
	{
		nStart = -1;
		nEnd = -1;
		for (GPosition pos = selection; pos; ++pos)
		{
			if (nStart == -1 || nStart > selection[pos]->text_start)
				nStart = selection[pos]->text_start;

			int nZoneEnd = selection[pos]->text_start + selection[pos]->text_length;
			if (nEnd == -1 || nEnd < nZoneEnd)
				nEnd = nZoneEnd;
		}

		page.nSelStart = nStart;
		page.nSelEnd = nEnd;
		m_bHasSelection = true;
	}
	else
	{
		page.nSelStart = -1;
		page.nSelEnd = -1;
	}

	InvalidatePage(nPage);
}

void CDjVuView::OnFilePrint()
{
	CPrintDlg dlg(GetDocument(), m_nPage, m_nRotate, m_nDisplayMode);
	if (m_nSelectionPage != -1)
	{
		dlg.m_bHasSelection = true;
		dlg.m_rcSelection = m_rcSelection;
	}

	if (dlg.DoModal() == IDOK)
	{
		ASSERT(dlg.m_pPrinter != NULL && dlg.m_pPaper != NULL);

		CProgressDlg progress_dlg(PrintThreadProc);
		progress_dlg.SetUserData(reinterpret_cast<DWORD_PTR>(&dlg));

		if (progress_dlg.DoModal() == IDOK)
		{
			if (progress_dlg.GetResultCode() > 0)
				AfxMessageBox(IDS_PRINTING_FAILED);
		}
	}
}

int CDjVuView::GetPageFromPoint(CPoint point) const
{
	point += GetScrollPosition();

	CRect rcFrame(m_nPageBorder, m_nPageBorder, m_nPageBorder + m_nPageShadow, m_nPageBorder + m_nPageShadow);

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		CRect rcPage(m_pages[m_nPage].ptOffset, m_pages[m_nPage].szBitmap);
		rcPage.InflateRect(rcFrame);

		if (rcPage.PtInRect(point))
			return m_nPage;

		if (m_nLayout == Facing && HasFacingPage(m_nPage))
		{
			CRect rcPage2(m_pages[m_nPage + 1].ptOffset, m_pages[m_nPage + 1].szBitmap);
			rcPage2.InflateRect(rcFrame);
			return (rcPage2.PtInRect(point) ? m_nPage + 1 : -1);
		}
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		for (int nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			CRect rcPage(m_pages[nPage].ptOffset, m_pages[nPage].szBitmap);
			rcPage.InflateRect(rcFrame);

			if (rcPage.PtInRect(point))
				return nPage;
		}
	}

	return -1;
}

int CDjVuView::GetPageNearPoint(CPoint point) const
{
	CRect rcClient;
	GetClientRect(rcClient);

	point.x = max(0, min(rcClient.Width() - 1, point.x));
	point.y = max(0, min(rcClient.Height() - 1, point.y));
	point += GetScrollPosition();

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		if (m_pages[m_nPage].rcDisplay.PtInRect(point))
			return m_nPage;

		if (m_nLayout == Facing && HasFacingPage(m_nPage))
			return (m_pages[m_nPage + 1].rcDisplay.PtInRect(point) ? m_nPage + 1 : -1);
	}
	else if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		for (int nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			if (m_pages[nPage].rcDisplay.PtInRect(point))
				return nPage;
		}
	}

	return -1;
}

void CDjVuView::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nMode == NextPrev && !m_bDragging && !m_bShiftDown)
	{
		OnViewPreviouspage();
		return;
	}

	CMyScrollView::OnRButtonDown(nFlags, point);
}

void CDjVuView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (m_nMode == NextPrev || m_bDraggingMagnify)
		return;

	if (point.x < 0 || point.y < 0)
		point = CPoint(0, 0);

	CRect rcClient;
	GetClientRect(rcClient);
	ClientToScreen(rcClient);

	if (!rcClient.PtInRect(point))
	{
		CMyScrollView::OnContextMenu(pWnd, point);
		return;
	}

	ScreenToClient(&point);
	m_nClickedPage = GetPageFromPoint(point);
	if (m_nClickedPage == -1)
		return;

	m_pClickedAnno = m_pHoverAnno;
	m_bClickedCustom = m_bHoverIsCustom;

	CMenu menu;
	menu.LoadMenu(IDR_POPUP);

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	if (m_bHasSelection || m_nSelectionPage != -1)
	{
		pPopup->DeleteMenu(ID_ANNOTATION_DELETE, MF_BYCOMMAND);
		pPopup->DeleteMenu(ID_ANNOTATION_EDIT, MF_BYCOMMAND);

		if (!m_bHasSelection)
		{
			pPopup->ModifyMenu(ID_EDIT_COPY, MF_BYCOMMAND | MF_STRING,
					ID_EDIT_COPY, LoadString(IDS_COPY_SELECTION));
			pPopup->DeleteMenu(ID_HIGHLIGHT_TEXT, MF_BYCOMMAND);
		}
		else
		{
			pPopup->DeleteMenu(ID_HIGHLIGHT_SELECTION, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_EXPORT_SELECTION, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_ZOOM_TO_SELECTION, MF_BYCOMMAND);
		}
	}
	else
	{
		pPopup->DeleteMenu(ID_EDIT_COPY, MF_BYCOMMAND);
		pPopup->DeleteMenu(ID_HIGHLIGHT_TEXT, MF_BYCOMMAND);
		pPopup->DeleteMenu(ID_HIGHLIGHT_SELECTION, MF_BYCOMMAND);
		pPopup->DeleteMenu(ID_EXPORT_SELECTION, MF_BYCOMMAND);
		pPopup->DeleteMenu(ID_ZOOM_TO_SELECTION, MF_BYCOMMAND);

		if (m_pClickedAnno == NULL || !m_bClickedCustom)
		{
			pPopup->DeleteMenu(ID_ANNOTATION_DELETE, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_ANNOTATION_EDIT, MF_BYCOMMAND);
			pPopup->DeleteMenu(0, MF_BYPOSITION);
		}
	}

	m_bIgnoreMouseLeave = true;
	m_bPopupMenu = true;
	ShowCursor();

	ClientToScreen(&point);
	UINT nID = pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
			point.x, point.y, GetTopLevelParent());

	m_bPopupMenu = false;

	if (nID != 0)
		GetTopLevelParent()->SendMessage(WM_COMMAND, nID);

	m_pClickedAnno = NULL;
	m_nClickedPage = -1;

	UpdateHoverAnnotation();
	UpdateCursor();
}

void CDjVuView::OnPageInformation()
{
	if (m_nClickedPage == -1)
		return;

	GP<DjVuImage> pImage = m_pSource->GetPage(m_nClickedPage, NULL);
	if (pImage == NULL)
	{
		AfxMessageBox(IDS_ERROR_DECODING_PAGE, MB_ICONERROR | MB_OK);
		return;
	}

	CString strDescr = MakeCString(pImage->get_long_description());
	CString strInfo, strLine;

	int nLine = 0;
	while (AfxExtractSubString(strLine, strDescr, nLine++, '\n'))
	{
		if (!strLine.IsEmpty() && strLine[0] == '\003')
			strLine = strLine.Mid(1);

		TCHAR szName[1000], szFile[1000];
		int nWidth, nHeight, nDPI, nVersion, nPart, nColors, nCCS, nShapes;
		double fSize, fRatio;
		CString strFormatted;

		if (strLine.Find(_T("DjVuFile.djvu_header")) == 0)
		{
			strLine = strLine.Mid(20);
			_stscanf(strLine, _T("%d%d%d%d"), &nWidth, &nHeight, &nDPI, &nVersion);
			strFormatted.Format(_T("DJVU Image (%dx%d, %d dpi) version %d:\n"),
				nWidth, nHeight, nDPI, nVersion);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_header")) == 0)
		{
			strLine = strLine.Mid(20);
			_stscanf(strLine, _T("%d%d%d"), &nWidth, &nHeight, &nDPI);
			strFormatted.Format(_T("IW44 Image (%dx%d, %d dpi):\n"),
				nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.page_info")) == 0)
		{
			strLine = strLine.Mid(18);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tPage information."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.indir_chunk1")) == 0)
		{
			strLine = strLine.Mid(21);
			_stscanf(strLine, _T("%s%lf%s"), szFile, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIndirection chunk (%s)."),
				fSize, szName, szFile);
		}
		else if (strLine.Find(_T("DjVuFile.indir_chunk2")) == 0)
		{
			strLine = strLine.Mid(21);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIndirection chunk."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JB2_fg")) == 0)
		{
			strLine = strLine.Mid(15);
			_stscanf(strLine, _T("%d%d%lf%s"), &nColors, &nCCS, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJB2 foreground colors (%d color%s, %d ccs)."),
				fSize, szName, nColors, (nColors == 1 ? _T("") : _T("s")), nCCS);
		}
		else if (strLine.Find(_T("DjVuFile.shape_dict")) == 0)
		{
			strLine = strLine.Mid(19);
			_stscanf(strLine, _T("%d%lf%s"), &nShapes, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJB2 shapes dictionary (%d shape%s)."),
				fSize, szName, nShapes, (nShapes == 1 ? _T("") : _T("s")));
		}
		else if (strLine.Find(_T("DjVuFile.fg_mask")) == 0)
		{
			strLine = strLine.Mid(16);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJB2 foreground mask (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.G4_mask")) == 0)
		{
			strLine = strLine.Mid(16);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tG4 foreground mask (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_bg1")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIW44 background (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_bg2")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%d%d%lf%s"), &nPart, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIW44 background (part %d, %d dpi)."),
				fSize, szName, nPart, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_data1")) == 0)
		{
			strLine = strLine.Mid(19);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIW44 data (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_data2")) == 0)
		{
			strLine = strLine.Mid(19);
			_stscanf(strLine, _T("%d%d%lf%s"), &nPart, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIW44 data (part %d, %d dpi)."),
				fSize, szName, nPart, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.IW44_fg")) == 0)
		{
			strLine = strLine.Mid(16);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tIW44 foreground colors (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_bg1_w")) == 0)
		{
			strLine = strLine.Mid(19);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG background."), fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_bg1")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG background (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_bg2")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG background (unimplemented)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_fg1_w")) == 0)
		{
			strLine = strLine.Mid(19);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG foreground colors."), fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_fg1")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG foreground colors (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG_fg2")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG foreground colors (unimplemented)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG2K_bg")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG-2000 background (unimplemented)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.JPEG2K_fg")) == 0)
		{
			strLine = strLine.Mid(17);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tJPEG-2000 foreground colors (unimplemented)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.ratio")) == 0)
		{
			strLine = strLine.Mid(14);
			_stscanf(strLine, _T("%lf%lf"), &fRatio, &fSize);
			strFormatted.Format(_T("\nCompression ratio: %.1f (%.1f Kb)"),
				fRatio, fSize);
		}
		else if (strLine.Find(_T("DjVuFile.text")) == 0)
		{
			strLine = strLine.Mid(13);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tText (text, etc.)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.unrecog_chunk")) == 0)
		{
			strLine = strLine.Mid(22);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tUnrecognized chunk."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.nav_dir")) == 0)
		{
			strLine = strLine.Mid(16);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tNavigation directory (obsolete)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.color_import1")) == 0)
		{
			strLine = strLine.Mid(22);
			_stscanf(strLine, _T("%d%d%d%lf%s"), &nWidth, &nHeight, &nDPI, &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tLINK Color Import (%dx%d, %d dpi)."),
				fSize, szName, nWidth, nHeight, nDPI);
		}
		else if (strLine.Find(_T("DjVuFile.color_import2")) == 0)
		{
			strLine = strLine.Mid(22);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tLINK Color Import (unimplemented)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.anno1")) == 0)
		{
			strLine = strLine.Mid(14);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tAnnotations (bundled)."),
				fSize, szName);
		}
		else if (strLine.Find(_T("DjVuFile.anno2")) == 0)
		{
			strLine = strLine.Mid(14);
			_stscanf(strLine, _T("%lf%s"), &fSize, szName);
			strFormatted.Format(_T(" %5.1f Kb\t'%s'\tAnnotations (hyperlinks, etc.)."),
				fSize, szName);
		}
		else
			strFormatted = strLine;

		strInfo += (!strInfo.IsEmpty() ? _T("\n") : _T("")) + strFormatted;
	}

	AfxMessageBox(strInfo, MB_ICONINFORMATION | MB_OK);
}

LRESULT CDjVuView::OnPageRendered(WPARAM wParam, LPARAM lParam)
{
	int nPage = (int)wParam;
	CDIB* pBitmap = reinterpret_cast<CDIB*>(lParam);

	m_dataLock.Lock();
	m_bitmaps.erase(pBitmap);
	m_dataLock.Unlock();

	OnPageDecoded(nPage, true);

	Page& page = m_pages[nPage];
	page.DeleteBitmap();
	page.pBitmap = pBitmap;
	page.bBitmapRendered = true;
	m_pageRendered.SetEvent();
	
	if (InvalidatePage(nPage))
		UpdateWindow();

	return 0;
}

void CDjVuView::PageDecoded(int nPage)
{
	PostMessage(WM_PAGE_DECODED, nPage);
}

void CDjVuView::PageRendered(int nPage, CDIB* pDIB)
{
	LPARAM lParam = reinterpret_cast<LPARAM>(pDIB);
	if (pDIB != NULL)
	{
		m_dataLock.Lock();
		m_bitmaps.insert(pDIB);
		m_dataLock.Unlock();
	}

	if (m_nPendingPage == nPage)
		SendMessage(WM_PAGE_RENDERED, nPage, lParam);
	else
		PostMessage(WM_PAGE_RENDERED, nPage, lParam);
}

void CDjVuView::OnDestroy()
{
	if (m_nType == Normal && m_pSource != NULL)
		GetMainFrame()->AddToHistory(this, true);

	if (m_pRenderThread != NULL)
	{
		m_pRenderThread->Stop();
		m_pRenderThread = NULL;
	}

	theApp.RemoveObserver(this);
	if (m_pSource != NULL)
		m_pSource->GetSettings()->RemoveObserver(this);

	KillTimer(m_nTimerID);
	ShowCursor();

	CMyScrollView::OnDestroy();
}

LRESULT CDjVuView::OnPageDecoded(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_nPageCount == m_pSource->GetPageCount());

	int nPage = (int)wParam;

	Page& page = m_pages[nPage];
	bool bHadInfo = page.info.bDecoded;

	page.Init(m_pSource, nPage);

	if (!bHadInfo)
	{
		if ((lParam || m_nTimerID == 0) && (m_nLayout == Continuous || m_nLayout == ContinuousFacing))
		{
			UpdateLayout();
			Invalidate();
			m_bNeedUpdate = false;
		}
		else
			m_bNeedUpdate = true;
	}

	return 0;
}

BOOL CDjVuView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		CRect rcClient;
		GetClientRect(rcClient);

		int nCode = HIBYTE(nScrollCode);
		if (nCode == SB_TOP)
		{
			UpdatePagesFromTop(0, rcClient.Height());
		}
		else if (nCode == SB_BOTTOM)
		{
			UpdatePagesFromBottom(m_szDisplay.cy - rcClient.Height(), m_szDisplay.cy);
		}
		else if (nCode == SB_THUMBTRACK)
		{
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);

			if (!GetScrollInfo(SB_VERT, &si, SIF_TRACKPOS))
				return true;

			UpdatePageSizes(si.nTrackPos);

			int yOrig = GetScrollPos(SB_VERT);

			BOOL bResult = OnScrollBy(CSize(0, si.nTrackPos - yOrig), bDoScroll);
			if (bResult && bDoScroll)
			{
				UpdateVisiblePages();
				UpdatePageNumber();

				UpdateWindow();
			}

			return bResult;
		}
		else
		{
			int y = GetScrollPos(SB_VERT);
			int nScroll = 0;

			// Calculate new x position
			switch (nCode)
			{
			case SB_LINEUP:
				nScroll = -m_lineDev.cy;
				break;
			case SB_LINEDOWN:
				nScroll = m_lineDev.cy;
				break;
			case SB_PAGEUP:
				nScroll = -m_pageDev.cy;
				break;
			case SB_PAGEDOWN:
				nScroll = m_pageDev.cy;
				break;
			}

			UpdatePageSizes(y, nScroll);
		}
	}

	BOOL bResult = CMyScrollView::OnScroll(nScrollCode, nPos, bDoScroll);

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdateVisiblePages();
		UpdatePageNumber();
	}

	return bResult;
}

void CDjVuView::UpdatePageSizes(int nTop, int nScroll)
{
	ASSERT(m_nLayout == Continuous || m_nLayout == ContinuousFacing);

	CRect rcClient;
	GetClientRect(rcClient);

	if (nScroll < 0)
	{
		UpdatePagesFromBottom(nTop + nScroll, nTop + rcClient.Height());
	}
	else
	{
		if (!UpdatePagesFromTop(nTop, nTop + rcClient.Height() + nScroll))
		{
			int nBottom = m_szDisplay.cy;
			UpdatePagesFromBottom(nBottom - rcClient.Height() - nScroll, nBottom);
		}
	}
}

void CDjVuView::UpdatePagesFromBottom(int nTop, int nBottom)
{
	// Check that the pages which become visible as a result
	// of this operation have their szPage filled in

	int nPage = 0;
	while (nPage < m_nPageCount - 1 &&
		   nBottom > m_pages[nPage].rcDisplay.bottom)
		++nPage;

	if (m_nLayout == Continuous)
	{
		UpdatePageSize(nPage);
		int nPageTop = m_pages[nPage].rcDisplay.top;

		while (nPage > 0 && nTop < nPageTop)
		{
			--nPage;
			UpdatePageSize(nPage);

			nPageTop -= m_pages[nPage].szBitmap.cy + 2 + m_nPageGap + m_nPageShadow;
		}
	}
	else
	{
		nPage = FixPageNumber(nPage);
		UpdatePageSizeFacing(nPage);
		int nPageTop = m_pages[nPage].rcDisplay.top;

		while (nPage > 0 && nTop < nPageTop)
		{
			nPage = max(0, nPage - 2);
			UpdatePageSizeFacing(nPage);

			int nHeight = m_pages[nPage].szBitmap.cy;
			if (HasFacingPage(nPage))
				nHeight = max(nHeight, m_pages[nPage + 1].szBitmap.cy);
			nPageTop -= nHeight + 2 + m_nPageGap + m_nPageShadow;
		}
	}

	if (m_bNeedUpdate)
	{
		UpdateLayout(BOTTOM);
		Invalidate();
		m_bNeedUpdate = false;
	}

	UpdatePageNumber();
}

bool CDjVuView::UpdatePagesFromTop(int nTop, int nBottom)
{
	int nPage = 0;
	while (nPage < m_nPageCount - 1 &&
		   nTop >= m_pages[nPage].rcDisplay.bottom)
		++nPage;

	int nPageBottom;

	if (m_nLayout == Continuous)
	{
		UpdatePageSize(nPage);
		nPageBottom = m_pages[nPage].rcDisplay.top + m_pages[nPage].szBitmap.cy + 1 + m_nPageShadow + m_nPageGap;

		while (++nPage < m_nPageCount && nBottom > nPageBottom)
		{
			UpdatePageSize(nPage);

			nPageBottom += m_pages[nPage].szBitmap.cy + 2 + m_nPageShadow + m_nPageGap;
		}
	}
	else
	{
		nPage = FixPageNumber(nPage);
		UpdatePageSizeFacing(nPage);

		int nHeight = m_pages[nPage].szBitmap.cy;
		if (HasFacingPage(nPage))
			nHeight = max(nHeight, m_pages[nPage + 1].szBitmap.cy);
		nPageBottom = m_pages[nPage].rcDisplay.top + nHeight + 1 + m_nPageShadow + m_nPageGap;

		while ((nPage = GetNextPage(nPage)) < m_nPageCount && nBottom > nPageBottom)
		{
			UpdatePageSizeFacing(nPage);

			int nHeight = m_pages[nPage].szBitmap.cy;
			if (HasFacingPage(nPage))
				nHeight = max(nHeight, m_pages[nPage + 1].szBitmap.cy);
			nPageBottom += nHeight + 1 + m_nPageShadow + m_nPageGap;
		}
	}

	bool bNeedScrollUp = false;
	if (nPage >= m_nPageCount)
		bNeedScrollUp = (nBottom > nPageBottom + m_nShadowMargin - m_nPageGap);

	if (m_bNeedUpdate)
	{
		UpdateLayout(TOP);
		Invalidate();
		m_bNeedUpdate = false;
	}

	UpdatePageNumber();

	return !bNeedScrollUp;
}

void CDjVuView::UpdatePageSize(int nPage)
{
	Page& page = m_pages[nPage];

	if (!page.info.bDecoded || !page.bHasSize)
	{
		if (!page.info.bDecoded)
			page.Init(m_pSource, nPage);

		page.szBitmap = CalcPageSize(page.GetSize(m_nRotate), page.info.nDPI);
		m_bNeedUpdate = true;
	}
}

void CDjVuView::UpdatePageSizeFacing(int nPage)
{
	Page& page = m_pages[nPage];
	Page* pNextPage = (HasFacingPage(nPage) ? &m_pages[nPage + 1] : NULL);

	bool bUpdated = false;
	if (!page.info.bDecoded || !page.bHasSize)
	{
		if (!page.info.bDecoded)
			page.Init(m_pSource, nPage);

		bUpdated = true;
		m_bNeedUpdate = true;
	}

	if (pNextPage != NULL && (!pNextPage->info.bDecoded || !pNextPage->bHasSize))
	{
		if (!pNextPage->info.bDecoded)
			pNextPage->Init(m_pSource, nPage + 1);

		bUpdated = true;
		m_bNeedUpdate = true;
	}

	if (bUpdated)
	{
		if (pNextPage != NULL)
		{
			CalcPageSizeFacing(page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap,
					pNextPage->GetSize(m_nRotate), pNextPage->info.nDPI, pNextPage->szBitmap);
		}
		else
		{
			CalcPageSizeFacing(page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap,
					page.GetSize(m_nRotate), page.info.nDPI, page.szBitmap);
		}
	}
}

int CDjVuView::CalcTopPage() const
{
	ASSERT(m_nLayout == Continuous || m_nLayout == ContinuousFacing);

	int nTop = GetScrollPos(SB_VERT);

	int nPage = 0;
	while (nPage < m_nPageCount - 1 && nTop >= m_pages[nPage].rcDisplay.bottom)
		++nPage;

	return FixPageNumber(nPage);
}

int CDjVuView::CalcCurrentPage() const
{
	if (m_nLayout == SinglePage || m_nLayout == Facing)
		return m_nPage;

	CRect rcClient;
	GetClientRect(rcClient);

	int nPage = CalcTopPage();
	const Page& page = m_pages[nPage];
	int nHeight = min(page.szBitmap.cy, rcClient.Height());

	if (page.rcDisplay.bottom - GetScrollPos(SB_VERT) < 0.3*nHeight &&
			GetNextPage(nPage) < m_nPageCount)
	{
		nPage = GetNextPage(nPage);
	}

	return nPage;
}

BOOL CDjVuView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	CWnd* pWnd = WindowFromPoint(point);
	if (pWnd != this && !IsChild(pWnd) && IsFromCurrentProcess(pWnd) &&
			pWnd->SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(nFlags, zDelta), MAKELPARAM(point.x, point.y)) != 0)
		return true;

	if ((nFlags & MK_CONTROL) != 0)
	{
		// Zoom in/out
		if (theApp.GetAppSettings()->bInvertWheelZoom == (zDelta < 0))
			OnViewZoomOut();
		else if (zDelta != 0)
			OnViewZoomIn();

		return true;
	}

	BOOL bHasHorzBar, bHasVertBar;
	CheckScrollBars(bHasHorzBar, bHasVertBar);

	if (!bHasVertBar && !bHasHorzBar && (m_nLayout == Continuous || m_nLayout == ContinuousFacing))
		return false;

	UINT uWheelScrollLines = GetMouseScrollLines();
	int nToScroll = ::MulDiv(-zDelta, uWheelScrollLines, WHEEL_DELTA);
	int nDisplacement;

	if (bHasVertBar && (!bHasHorzBar || (nFlags & MK_SHIFT) == 0))
	{
		if (uWheelScrollLines == WHEEL_PAGESCROLL)
		{
			nDisplacement = m_pageDev.cy;
			if (zDelta > 0)
				nDisplacement = -nDisplacement;
		}
		else
		{
			nDisplacement = nToScroll * m_lineDev.cy;
			nDisplacement = min(nDisplacement, m_pageDev.cy);
		}

		if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
			UpdatePageSizes(GetScrollPos(SB_VERT), nDisplacement);

		OnScrollBy(CSize(0, nDisplacement));

		if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		{
			UpdateVisiblePages();
			UpdatePageNumber();
		}
	}
	else if (bHasHorzBar)
	{
		if (uWheelScrollLines == WHEEL_PAGESCROLL)
		{
			nDisplacement = m_pageDev.cx;
			if (zDelta > 0)
				nDisplacement = -nDisplacement;
		}
		else
		{
			nDisplacement = nToScroll * m_lineDev.cx;
			nDisplacement = min(nDisplacement, m_pageDev.cx);
		}

		OnScrollBy(CSize(nDisplacement, 0));
	}
	else if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		if (zDelta < 0 && IsViewNextpageEnabled())
		{
			OnViewNextpage();
		}
		else if (zDelta > 0 && IsViewPreviouspageEnabled())
		{
			CPoint ptScroll = GetScrollPosition();
			OnViewPreviouspage();
			OnScrollBy(CPoint(ptScroll.x, m_szDisplay.cy) - GetScrollPosition());
			Invalidate();
		}
	}

	UpdateWindow();

	UpdateDragAction();

	if (!m_bDragging)
		UpdateHoverAnnotation();

	UpdateCursor();

	return true;
}

bool CDjVuView::InvalidatePage(int nPage)
{
	ASSERT(nPage >= 0 && nPage < m_nPageCount);
	if (m_nLayout == SinglePage && nPage != m_nPage ||
			(m_nLayout == Facing && nPage != m_nPage && (!HasFacingPage(m_nPage) || nPage != m_nPage + 1)))
		return false;

	CPoint ptOffset = GetScrollPosition();

	CRect rcClient;
	GetClientRect(rcClient);
	rcClient.OffsetRect(ptOffset);

	CRect rcIntersect;
	if (rcIntersect.IntersectRect(rcClient, m_pages[nPage].rcDisplay))
	{
		InvalidateRect(rcIntersect - ptOffset);
		return true;
	}

	return false;
}

bool CDjVuView::InvalidateAnno(Annotation* pAnno, int nPage)
{
	ASSERT(nPage >= 0 && nPage < m_nPageCount);
	if (m_nLayout == SinglePage && nPage != m_nPage ||
			(m_nLayout == Facing && nPage != m_nPage && (!HasFacingPage(m_nPage) || nPage != m_nPage + 1)))
		return false;

	CPoint ptOffset = GetScrollPosition();
	CRect rect = TranslatePageRect(nPage, pAnno->rectBounds);

	CRect rcClient;
	GetClientRect(rcClient);
	rcClient.OffsetRect(ptOffset);

	CRect rcIntersect;
	if (rcIntersect.IntersectRect(rcClient, rect))
	{
		InvalidateRect(rcIntersect - ptOffset);
		return true;
	}

	return false;
}

void CDjVuView::OnExportPage(UINT nID)
{
	if (m_nSelectionPage == -1 && nID == ID_EXPORT_SELECTION)
		return;

	int nPage = (nID == ID_EXPORT_SELECTION ? m_nSelectionPage : m_nClickedPage);

	CString strFileName = FormatString(_T("p%04d%s.bmp"), nPage + 1,
		nID == ID_EXPORT_SELECTION ? _T("-sel") : _T(""));

	CMyFileDialog dlg(false, _T("bmp"), strFileName, OFN_OVERWRITEPROMPT |
		OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST,
		LoadString(IDS_BMP_FILTER));

	CString strTitle;
	strTitle.LoadString(IDS_EXPORT_PAGE);
	dlg.m_ofn.lpstrTitle = strTitle.GetBuffer(0);

	UINT nResult = dlg.DoModal();
	SetFocus();
	if (nResult != IDOK)
		return;

	CWaitCursor wait;
	strFileName = dlg.GetPathName();

	Page& page = m_pages[nPage];
	if (!page.info.bDecoded)
		page.Init(m_pSource, nPage);

	GP<DjVuImage> pImage = m_pSource->GetPage(nPage, NULL);
	if (pImage == NULL)
	{
		AfxMessageBox(IDS_ERROR_DECODING_PAGE, MB_ICONERROR | MB_OK);
		return;
	}

	CSize size = page.GetSize(m_nRotate);
	CDIB* pBitmap = CRenderThread::Render(pImage, size, m_displaySettings, m_nDisplayMode, m_nRotate);

	if (nID == ID_EXPORT_SELECTION)
	{
		CRect rcCrop = TranslatePageRect(nPage, m_rcSelection, false);
		CDIB* pCropped = pBitmap->Crop(rcCrop);
		delete pBitmap;
		pBitmap = pCropped;
	}

	if (pBitmap != NULL && pBitmap->m_hObject != NULL)
	{
		CDIB* pNewBitmap = pBitmap->ReduceColors();
		if (pNewBitmap != NULL && pNewBitmap->m_hObject != NULL)
		{
			delete pBitmap;
			pBitmap = pNewBitmap;
		}
		else
		{
			delete pNewBitmap;
		}

		pBitmap->Save(strFileName);
	}
	else
	{
		AfxMessageBox(IDS_ERROR_RENDERING_PAGE, MB_ICONERROR | MB_OK);
	}

	delete pBitmap;
}

void CDjVuView::OnUpdateExportSelection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_nSelectionPage != -1);
}

void CDjVuView::OnFindString()
{
	CWaitCursor wait;

	CFindDlg* pDlg = GetMainFrame()->m_pFindDlg;
	ASSERT(pDlg != NULL);

	GUTF8String strFindUTF8 = MakeUTF8String(pDlg->m_strFind);
	GUTF8String strFindUp;
	if (!pDlg->m_bMatchCase)
		strFindUp = strFindUTF8.upcase();

	GUTF8String& strFind = (pDlg->m_bMatchCase ? strFindUTF8 : strFindUp);

	if (strFind.length() == 0)
	{
		::MessageBeep(MB_OK);
		return;
	}

	int nStartPage, nStartPos;
	bool bHasSelection = false;

	// Check if there is a selection
	for (int i = 0; i < m_nPageCount; ++i)
	{
		Page& page = m_pages[i];
		if (page.nSelEnd != -1)
		{
			bHasSelection = true;
			nStartPage = i;
			nStartPos = page.nSelEnd;
			break;
		}
	}

	if (!bHasSelection)
	{
		nStartPage = m_nPage;
		nStartPos = 0;
	}

	int nPage = nStartPage;
	while (nPage < m_nPageCount)
	{
		Page& page = m_pages[nPage];
		if (!page.info.bDecoded || page.info.bHasText && !page.info.bTextDecoded)
		{
			if (!page.info.bDecoded)
				m_bNeedUpdate = true;

			page.Init(m_pSource, nPage, true);
		}

		if (page.info.pText != NULL)
		{
			GP<DjVuTXT> pText = page.info.pText;

			int nPos = -1;
			if (pDlg->m_bMatchCase)
			{
				GUTF8String& text = pText->textUTF8;
				nPos = text.search(strFind, nStartPos);
			}
			else
			{
				GUTF8String textUp = pText->textUTF8.upcase();
				nPos = textUp.search(strFind, nStartPos);
			}

			if (nPos != -1)
			{
				if (m_bNeedUpdate)
				{
					UpdateLayout();
					Invalidate();
					m_bNeedUpdate = false;
				}

				int nSelEnd = nPos + strFind.length();
				DjVuSelection selection;
				FindSelectionZones(selection, pText, nPos, nSelEnd);

				if (!bHasSelection && nPage == nStartPage &&
						!IsSelectionBelowTop(nPage, selection))
				{
					nStartPos = nPos + strFind.length();
					continue;
				}

				ClearSelection();

				int nStart = -1;
				int nEnd = -1;
				for (GPosition pos = selection; pos; ++pos)
				{
					if (nStart == -1 || nStart > selection[pos]->text_start)
						nStart = selection[pos]->text_start;

					int nZoneEnd = selection[pos]->text_start + selection[pos]->text_length;
					if (nEnd == -1 || nEnd < nZoneEnd)
						nEnd = nZoneEnd;
				}

				page.nSelStart = nStart;
				page.nSelEnd = nEnd;
				page.selection = selection;
				m_bHasSelection = !selection.isempty();

				InvalidatePage(nPage);

				if (!IsSelectionVisible(nPage, page.selection))
				{
					EnsureSelectionVisible(nPage, page.selection);
				}

				GetMainFrame()->SetMessageText(AFX_IDS_IDLEMESSAGE);
				return;
			}
		}

		nStartPos = 0;
		++nPage;
	}

	if (m_bNeedUpdate)
	{
		UpdateLayout();
		Invalidate();
		m_bNeedUpdate = false;
	}

	GetMainFrame()->HilightStatusMessage(LoadString(IDS_STRING_NOT_FOUND));
	::MessageBeep(MB_OK);
}

void CDjVuView::OnFindAll()
{
	CWaitCursor wait;

	CFindDlg* pDlg = GetMainFrame()->m_pFindDlg;
	ASSERT(pDlg != NULL);

	GUTF8String strFindUTF8 = MakeUTF8String(pDlg->m_strFind);
	GUTF8String strFindUp;
	if (!pDlg->m_bMatchCase)
		strFindUp = strFindUTF8.upcase();

	GUTF8String& strFind = (pDlg->m_bMatchCase ? strFindUTF8 : strFindUp);

	if (strFind.length() == 0)
	{
		::MessageBeep(MB_OK);
		return;
	}

	CSearchResultsView* pResults = NULL;
	int nResultCount = 0;

	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		pDlg->SetStatusText(FormatString(IDS_SEARCHING_PAGE, nPage + 1, m_nPageCount));

		Page& page = m_pages[nPage];
		if (!page.info.bDecoded || page.info.bHasText && !page.info.bTextDecoded)
		{
			if (!page.info.bDecoded)
				m_bNeedUpdate = true;

			page.Init(m_pSource, nPage, true);
		}

		if (page.info.pText == NULL)
			continue;

		GP<DjVuTXT> pText = page.info.pText;
		GUTF8String textUp;
		if (!pDlg->m_bMatchCase)
			textUp = pText->textUTF8.upcase();

		GUTF8String& text = (pDlg->m_bMatchCase ? pText->textUTF8 : textUp);

		int nPos = 0;
		do
		{
			nPos = text.search(strFind, nPos);
			if (nPos == -1)
				break;

			int nSelEnd = nPos + strFind.length();

			if (pResults == NULL)
			{
				pResults = ((CChildFrame*)GetParentFrame())->GetSearchResults();
				pResults->Reset();
			}

			++nResultCount;
			CString strPreview = MakePreviewString(pText->textUTF8, nPos, nSelEnd);
			pResults->AddString(strPreview, nPage, nPos, nSelEnd);

			nPos = nSelEnd;
		} while (nPos != -1);
	}

	if (m_bNeedUpdate)
	{
		UpdateLayout();
		Invalidate();
		m_bNeedUpdate = false;
	}

	if (nResultCount > 0)
	{
		GetMainFrame()->SetMessageText(FormatString(IDS_NUM_OCCURRENCES, nResultCount));
		pDlg->ShowWindow(SW_HIDE);
	}
	else
	{
		GetMainFrame()->HilightStatusMessage(LoadString(IDS_STRING_NOT_FOUND));
		::MessageBeep(MB_OK);
	}
}

CString MakePreviewString(const GUTF8String& text, int nStart, int nEnd)
{
	// Convert text to CString and add -1/+10 words to the string
	CString strPreview = MakeCString(text.substr(nStart, nEnd - nStart));

	CString strBefore = MakeCString(text.substr(0, nStart));

	int nPos = strBefore.GetLength() - 1;
	for (int i = 0; i < 2 && nPos >= 0; ++i)
	{
		if (i > 0)
			strPreview = _T(" ") + strPreview;

		while (nPos >= 0 && static_cast<unsigned int>(strBefore[nPos]) > 0x20
			&& strBefore.GetLength() - nPos < 30)
		{
			strPreview = strBefore[nPos--] + strPreview;
		}

		while (nPos >= 0 && static_cast<unsigned int>(strBefore[nPos]) <= 0x20)
			--nPos;
	}

	CString strAfter = MakeCString(text.substr(nEnd, text.length() - nEnd));

	nPos = 0;
	for (int j = 0; j < 11 && nPos < strAfter.GetLength(); ++j)
	{
		if (j > 0)
			strPreview += _T(" ");

		while (nPos < strAfter.GetLength() && static_cast<unsigned int>(strAfter[nPos]) > 0x20
			&& nPos < 200)
		{
			strPreview += strAfter[nPos++];
		}

		while (nPos < strAfter.GetLength() && static_cast<unsigned int>(strAfter[nPos]) <= 0x20)
			++nPos;
	}

	return strPreview;
}

void CDjVuView::FindSelectionZones(DjVuSelection& sel, DjVuTXT* pText, int nStart, int nEnd) const
{
	for (GPosition pos = pText->page_zone.children; pos; ++pos)
		pText->page_zone.children[pos].find_zones(sel, nStart, nEnd);
}

CRect CDjVuView::GetSelectionRect(int nPage, const DjVuSelection& selection) const
{
	CRect rcSel;
	bool bFirst = true;

	for (GPosition pos = selection; pos; ++pos)
	{
		GRect rect = selection[pos]->rect;
		CRect rcText = TranslatePageRect(nPage, rect);

		if (bFirst)
		{
			rcSel = rcText;
			bFirst = false;
		}
		else
			rcSel.UnionRect(rcSel, rcText);
	}

	return rcSel;
}

bool CDjVuView::IsSelectionBelowTop(int nPage, const DjVuSelection& selection) const
{
	int nTop = GetScrollPos(SB_VERT);

	CRect rcSel = GetSelectionRect(nPage, selection);
	return (rcSel.top >= nTop);
}

bool CDjVuView::IsSelectionVisible(int nPage, const DjVuSelection& selection) const
{
	if (m_nLayout == SinglePage && nPage != m_nPage)
		return false;
	if (m_nLayout == Facing && nPage != m_nPage && (!HasFacingPage(m_nPage) || nPage != m_nPage + 1))
		return false;

	CPoint ptScroll = GetScrollPosition();

	CRect rcClient;
	GetClientRect(rcClient);
	rcClient.OffsetRect(ptScroll);

	CRect rcSel = GetSelectionRect(nPage, selection);
	rcClient.IntersectRect(rcClient, rcSel);

	if (rcClient != rcSel)
		return false;

	CFindDlg* pDlg = GetMainFrame()->m_pFindDlg;
	if (pDlg != NULL && pDlg->IsWindowVisible())
	{
		CRect rcFindDlg;
		pDlg->GetWindowRect(rcFindDlg);
		ScreenToClient(rcFindDlg);
		rcFindDlg.OffsetRect(ptScroll);

		if (rcFindDlg.IntersectRect(rcFindDlg, rcSel))
			return false;
	}

	return true;
}

void CDjVuView::EnsureSelectionVisible(int nPage, const DjVuSelection& selection)
{
	if ((m_nLayout == SinglePage || m_nLayout == Facing) && m_nPage != nPage)
	{
		RenderPage(nPage, 1000);
	}

	CRect rcClient;
	GetClientRect(rcClient);
	CPoint ptScroll = GetScrollPosition();
	rcClient.OffsetRect(ptScroll);

	const Page& page = m_pages[nPage];
	int nHeight = min(page.szBitmap.cy, rcClient.Height());
	int nWidth = min(page.szBitmap.cx, rcClient.Width());

	CRect rcSel = GetSelectionRect(nPage, selection);

	// Scroll horizontally
	int nLeftPos = rcClient.left;
	if (rcSel.left < rcClient.left)
	{
		nLeftPos = max(page.rcDisplay.left, static_cast<int>(rcSel.left - 0.2*nWidth));
		if (nLeftPos + rcClient.Width() < rcSel.right)
			nLeftPos = rcSel.right - rcClient.Width();
		nLeftPos = min(rcSel.left, nLeftPos);
	}
	else if (rcSel.right > rcClient.right)
	{
		int nRight = min(page.rcDisplay.right, static_cast<int>(rcSel.right + 0.2*nWidth));
		nLeftPos = min(rcSel.left, nRight - rcClient.Width());
	}

	// Scroll vertically
	int nTopPos = rcClient.top;
	if (rcSel.top < rcClient.top)
	{
		nTopPos = max(page.rcDisplay.top, static_cast<int>(rcSel.top - 0.3*nHeight));
		if (nTopPos + rcClient.Height() < rcSel.bottom)
			nTopPos = rcSel.bottom - rcClient.Height();
		nTopPos = min(rcSel.top, nTopPos);
	}
	else if (rcSel.bottom > rcClient.bottom)
	{
		int nBottom = min(m_szDisplay.cy, static_cast<int>(rcSel.bottom + 0.3*nHeight));
		nTopPos = min(rcSel.top, nBottom - rcClient.Height());
	}

	rcClient.OffsetRect(0, nTopPos - ptScroll.y);

	// Further scroll the document if selection is obscured by the Find dialog
	CFindDlg* pDlg = GetMainFrame()->m_pFindDlg;
	ASSERT(pDlg != NULL);
	CRect rcFindDlg;
	pDlg->GetWindowRect(rcFindDlg);
	ScreenToClient(rcFindDlg);
	rcFindDlg.OffsetRect(0, nTopPos);

	if (rcSel.bottom > rcFindDlg.top && rcSel.top < rcFindDlg.bottom)
	{
		int nTopSpace = rcFindDlg.top - rcClient.top;
		int nBottomSpace = rcClient.bottom - rcFindDlg.bottom;
		if (nTopSpace >= rcSel.Height() || nTopSpace >= nBottomSpace && nTopSpace > 0)
		{
			nTopPos = max(0, rcSel.top - (nTopSpace - rcSel.Height())/2);
			nTopPos = min(rcSel.top, nTopPos);
		}
		else if (nBottomSpace > 0)
		{
			int nBottom = rcSel.bottom + (nBottomSpace - rcSel.Height())/2;
			nTopPos = min(rcSel.top - (rcFindDlg.bottom - rcClient.top), nBottom - rcClient.Height());
		}
	}

	CPoint ptScrollBy(nLeftPos - ptScroll.x, nTopPos - GetScrollPos(SB_VERT));

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
		UpdatePageSizes(GetScrollPos(SB_VERT), ptScrollBy.y);

	OnScrollBy(ptScrollBy);

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdateVisiblePages();
		UpdatePageNumber();
	}
}

CRect CDjVuView::TranslatePageRect(int nPage, GRect rect, bool bToDisplay, bool bClip) const
{
	const Page& page = m_pages[nPage];
	CSize szPage = page.GetSize(m_nRotate);

	int nRotate = m_nRotate + page.info.nInitialRotate;
	if (nRotate != 0)
	{
		GRect input(0, 0, szPage.cx, szPage.cy);
		GRect output(0, 0, page.info.szPage.cx, page.info.szPage.cy);

		if ((page.info.nInitialRotate % 2) != 0)
			swap(output.xmax, output.ymax);

		GRectMapper mapper;
		mapper.clear();
		mapper.set_input(input);
		mapper.set_output(output);
		mapper.rotate(4 - nRotate);
		mapper.unmap(rect);
	}

	CRect rcResult;

	if (bToDisplay)
	{
		double fRatioX = (1.0*page.szBitmap.cx) / szPage.cx;
		double fRatioY = (1.0*page.szBitmap.cy) / szPage.cy;

		rcResult = CRect(static_cast<int>(rect.xmin * fRatioX + 0.5),
			static_cast<int>((szPage.cy - rect.ymax) * fRatioY + 0.5),
			static_cast<int>(rect.xmax * fRatioX + 0.5),
			static_cast<int>((szPage.cy - rect.ymin) * fRatioY + 0.5));

		if (bClip)
		{
			rcResult.left = max(rcResult.left, 0);
			rcResult.top = max(rcResult.top, 0);
			rcResult.right = min(rcResult.right, page.szBitmap.cx);
			rcResult.bottom = min(rcResult.bottom, page.szBitmap.cy);
		}

		rcResult.OffsetRect(page.ptOffset);
	}
	else if (bClip)
	{
		rcResult.left = max(rect.xmin, 0);
		rcResult.top = max(szPage.cy - rect.ymax, 0);
		rcResult.right = min(rect.xmax, szPage.cx);
		rcResult.bottom = min(szPage.cy - rect.ymin, szPage.cy);
	}

	return rcResult;
}

void CDjVuView::ClearSelection(int nPage)
{
	if (nPage != -1)
	{
		Page& page = m_pages[nPage];

		if (page.nSelStart != -1)
		{
			page.selection.empty();
			page.nSelStart = -1;
			page.nSelEnd = -1;
			InvalidatePage(nPage);
		}
	}
	else
	{
		for (nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			Page& page = m_pages[nPage];

			if (page.nSelStart != -1)
			{
				page.selection.empty();
				page.nSelStart = -1;
				page.nSelEnd = -1;
				InvalidatePage(nPage);
			}
		}

		if (m_nSelectionPage != -1)
		{
			InvalidatePage(m_nSelectionPage);
			m_nSelectionPage = -1;
		}

		m_bHasSelection = false;
	}
}

void CDjVuView::UpdateSelectionStatus()
{
	m_bHasSelection = false;

	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		Page& page = m_pages[nPage];

		if (page.nSelStart != -1)
		{
			m_bHasSelection = true;
			break;
		}
	}
}

void CDjVuView::UpdateHoverAnnotation()
{
	CPoint ptCursor;
	GetCursorPos(&ptCursor);
	ScreenToClient(&ptCursor);

	UpdateHoverAnnotation(ptCursor);
}

void CDjVuView::UpdateHoverAnnotation(const CPoint& point)
{
	int nPage;
	bool bCustom;
	Annotation* pAnno = GetAnnotationFromPoint(point, nPage, bCustom);

	if (pAnno != m_pHoverAnno)
	{
		if (m_pHoverAnno != NULL)
		{
			m_toolTip.Activate(false);
			InvalidateAnno(m_pHoverAnno, m_nHoverPage);
			GetMainFrame()->SetMessageText(AFX_IDS_IDLEMESSAGE);
		}

		m_pHoverAnno = pAnno;
		m_nHoverPage = nPage;
		m_bHoverIsCustom = bCustom;

		if (m_pHoverAnno != NULL)
		{
			m_toolTip.Activate(true);
			InvalidateAnno(m_pHoverAnno, m_nHoverPage);

			if (m_nType == Normal)
				GetMainFrame()->SetMessageText(MakeCString(m_pHoverAnno->strURL));

			TRACKMOUSEEVENT tme;

			ZeroMemory(&tme, sizeof(tme));
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = m_hWnd;
			tme.dwHoverTime = HOVER_DEFAULT;

			m_bIgnoreMouseLeave = false;
			TrackMouseEvent(&tme);
		}

		UpdateCursor();
	}
}

void CDjVuView::UpdateCursor()
{
	CPoint ptCursor;
	GetCursorPos(&ptCursor);

	if (WindowFromPoint(ptCursor) != this)
		return;

	ScreenToClient(&ptCursor);

	CRect rcClient;
	GetClientRect(rcClient);

	if (rcClient.PtInRect(ptCursor))
		SendMessage(WM_SETCURSOR, (WPARAM) m_hWnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
}

Annotation* CDjVuView::GetAnnotationFromPoint(const CPoint& point, int& nPage, bool& bCustom)
{
	bCustom = false;
	nPage = -1;

	CRect rcClient;
	GetClientRect(rcClient);
	if (!rcClient.PtInRect(point))
		return NULL;

	nPage = GetPageFromPoint(point);
	if (nPage == -1)
		return NULL;

	Page& page = m_pages[nPage];

	map<int, PageSettings>::iterator it = m_pSource->GetSettings()->pageSettings.find(nPage);
	if (it != m_pSource->GetSettings()->pageSettings.end())
	{
		PageSettings& pageSettings = (*it).second;
		list<Annotation>::reverse_iterator rit;
		for (rit = pageSettings.anno.rbegin(); rit != pageSettings.anno.rend(); ++rit)
		{
			Annotation& anno = *rit;
			if (PtInAnnotation(anno, nPage, point))
			{
				bCustom = true;
				return &anno;
			}
		}
	}

	list<Annotation>::reverse_iterator rit;
	for (rit = page.info.anno.rbegin(); rit != page.info.anno.rend(); ++rit)
	{
		Annotation& anno = *rit;
		if (PtInAnnotation(anno, nPage, point))
			return &anno;
	}

	nPage = -1;
	return NULL;
}

bool CDjVuView::PtInAnnotation(const Annotation& anno, int nPage, const CPoint& point) const
{
	CRect rectBounds = TranslatePageRect(nPage, anno.rectBounds);
	rectBounds.OffsetRect(-GetScrollPosition());

	if (!rectBounds.PtInRect(point))
		return false;

	for (size_t i = 0; i < anno.rects.size(); ++i)
	{
		CRect rect = TranslatePageRect(nPage, anno.rects[i]);
		rect.OffsetRect(-GetScrollPosition());

		if (rect.PtInRect(point))
			return true;
	}

	return false;
}

BOOL CDjVuView::OnToolTipNeedText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	CPoint ptCursor;
	::GetCursorPos(&ptCursor);
	ScreenToClient(&ptCursor);

	CRect rcClient;
	GetClientRect(rcClient);

	// Ensure that the cursor is in the client rect, because main frame
	// also wants these messages to provide tooltips for the toolbar
	if (!rcClient.PtInRect(ptCursor))
		return false;

	if (m_pHoverAnno != NULL)
	{
		m_strToolTip = MakeCString(m_pHoverAnno->strComment);
		if (m_strToolTip.IsEmpty())
			m_strToolTip = MakeCString(m_pHoverAnno->strURL);
	}
	else
	{
		m_strToolTip.Empty();
	}

	pTTT->lpszText = m_strToolTip.GetBuffer(0);
	if (!m_strToolTip.IsEmpty())
	{
		m_toolTip.SetWindowPos(&wndTopMost, 0, 0, 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
	}

	return true;
}

BOOL CDjVuView::PreTranslateMessage(MSG* pMsg)
{
	if (::IsWindow(m_toolTip.m_hWnd))
		m_toolTip.RelayEvent(pMsg);

	return CMyScrollView::PreTranslateMessage(pMsg);
}

void CDjVuView::GoToBookmark(const Bookmark& bookmark, int nAddToHistory)
{
	if (bookmark.nLinkType == Bookmark::URL)
	{
		GoToURL(bookmark.strURL, nAddToHistory);
	}
	else if (bookmark.nLinkType == Bookmark::Page)
	{
		GoToPage(bookmark.nPage, nAddToHistory);
	}
	else if (bookmark.nLinkType == Bookmark::View)
	{
		int nPage = max(min(bookmark.nPage, m_nPageCount - 1), 0);

		if ((nAddToHistory & AddSource) != 0)
			GetMainFrame()->AddToHistory(this);

		if (bookmark.nPage == nPage)
			ScrollToPage(nPage, bookmark.ptOffset, bookmark.bMargin);
		else
			RenderPage(nPage);

		if ((nAddToHistory & AddTarget) != 0)
			GetMainFrame()->AddToHistory(this, bookmark);
	}
}

void CDjVuView::GoToURL(const GUTF8String& url, int nAddToHistory)
{
	if (url.length() == 0)
		return;

	if (url[0] == '#')
	{
		int nPage = -1;

		try
		{
			char base = '\0';
			const char *s = (const char*)url + 1;
			const char *q = s;
			if (*q == '+' || *q == '-')
				base = *q++;

			GUTF8String str = q;
			if (str.is_int())
			{
				nPage = str.toInt();
				if (base == '+')
					nPage = m_nPage + nPage;
				else if (base=='-')
					nPage = m_nPage - nPage;
				else
					--nPage;
			}
			else
			{
				nPage = m_pSource->GetPageFromId(s);
				if (nPage == -1)
					return;
			}
		}
		catch (GException&)
		{
		}
		catch (...)
		{
			theApp.ReportFatalError();
		}

		GoToPage(nPage, nAddToHistory);
		return;
	}

	// Try to open a .djvu file given by a path (relative or absolute)

	int nPos = url.search('#');
	GUTF8String strPage, strURL = url;
	if (nPos != -1)
	{
		strPage = url.substr(nPos, url.length() - nPos);
		strURL = strURL.substr(0, nPos);
	}

	CString strPathName = MakeCString(strURL);
	LPTSTR pszExt = PathFindExtension(strPathName);
	if (!PathIsURL(strPathName) && pszExt != NULL
			&& (_tcsicmp(pszExt, _T(".djvu")) == 0 || _tcsicmp(pszExt, _T(".djv")) == 0))
	{
		// Check if the link leads to a local DjVu file

		if (_tcsnicmp(strPathName, _T("file:///"), 8) == 0)
			strPathName = strPathName.Mid(8);
		else if (_tcsnicmp(strPathName, _T("file://"), 7) == 0)
			strPathName = strPathName.Mid(7);

		// Try as absolute path
		if (PathIsRelative(strPathName))
		{
			CString strPath = m_pSource->GetFileName();
			PathRemoveFileSpec(strPath.GetBuffer(MAX_PATH));
			if (PathAppend(strPath.GetBuffer(MAX_PATH), strPathName))
			{
				strPath.ReleaseBuffer();
				strPathName = strPath;
			}
		}

		if (PathFileExists(strPathName))
		{
			if (AfxComparePath(m_pSource->GetFileName(), strPathName))
			{
				GoToURL(strPage, nAddToHistory);
			}
			else
			{
				if ((nAddToHistory & AddSource) != 0)
				{
					if (m_nType == Fullscreen)
					{
						CFullscreenWnd* pFullscreenWnd = (CFullscreenWnd*) GetTopLevelParent();
						CDjVuView* pOwner = pFullscreenWnd->GetOwner();

						pFullscreenWnd->Hide();

						GetMainFrame()->AddToHistory(pOwner);
					}
					else
						GetMainFrame()->AddToHistory(this);
				}

				theApp.OpenDocument(strPathName, strPage);
			}
			return;
		}
	}

	// Open a web link
	DWORD dwResult = (DWORD)::ShellExecute(NULL, _T("open"), MakeCString(url), NULL, NULL, SW_SHOWNORMAL);
	if (dwResult <= 32) // Failure
	{
		AfxMessageBox(LoadString(IDS_FAILED_TO_OPEN) + MakeCString(url));
	}
}

void CDjVuView::GoToPage(int nPage, int nAddToHistory)
{
	nPage = max(min(nPage, m_nPageCount - 1), 0);

	if ((nAddToHistory & AddSource) != 0)
		GetMainFrame()->AddToHistory(this);

	RenderPage(nPage);

	if ((nAddToHistory & AddTarget) != 0)
		GetMainFrame()->AddToHistory(this, nPage);
}

void CDjVuView::OnViewZoomIn()
{
	double fCurrentZoom = GetZoom();
	static const double levels[] = { 10.0, 12.5, 25, 33.33, 50, 66.66, 75,
		100, 125, 150, 200, 300, 400, 600, 800 };
	double fFitWidth, fFitPage, fActualSize;

	vector<double> zoomLevels(levels, levels + sizeof(levels)/sizeof(double));
	zoomLevels.push_back(fFitWidth = GetZoom(ZoomFitWidth));
	zoomLevels.push_back(fFitPage = GetZoom(ZoomFitPage));
	zoomLevels.push_back(fActualSize = GetZoom(ZoomActualSize));

	sort(zoomLevels.begin(), zoomLevels.end());

	int nZoom;
	for (nZoom = 0; nZoom < static_cast<int>(zoomLevels.size() - 1); ++nZoom)
	{
		if (zoomLevels[nZoom] >= fCurrentZoom + 1e-2)
			break;
	}

	if (zoomLevels[nZoom] == fFitWidth)
		ZoomTo(ZoomFitWidth);
	else if (zoomLevels[nZoom] == fFitPage)
		ZoomTo(ZoomFitPage);
	else if (zoomLevels[nZoom] == fActualSize)
		ZoomTo(ZoomActualSize);
	else
		ZoomTo(ZoomPercent, zoomLevels[nZoom]);
}

void CDjVuView::OnViewZoomOut()
{
	double fCurrentZoom = GetZoom();
	static const double levels[] = { 10.0, 12.5, 25, 33.33, 50, 66.66, 75,
		100, 125, 150, 200, 300, 400, 600, 800 };
	double fFitWidth, fFitPage, fActualSize;

	vector<double> zoomLevels(levels, levels + sizeof(levels)/sizeof(double));
	zoomLevels.push_back(fFitWidth = GetZoom(ZoomFitWidth));
	zoomLevels.push_back(fFitPage = GetZoom(ZoomFitPage));
	zoomLevels.push_back(fActualSize = GetZoom(ZoomActualSize));

	sort(zoomLevels.begin(), zoomLevels.end());

	int nZoom;
	for (nZoom = zoomLevels.size() - 1; nZoom > 0; --nZoom)
	{
		if (zoomLevels[nZoom] <= fCurrentZoom - 1e-2)
			break;
	}

	if (zoomLevels[nZoom] == fFitWidth)
		ZoomTo(ZoomFitWidth);
	else if (zoomLevels[nZoom] == fFitPage)
		ZoomTo(ZoomFitPage);
	else if (zoomLevels[nZoom] == fActualSize)
		ZoomTo(ZoomActualSize);
	else
		ZoomTo(ZoomPercent, zoomLevels[nZoom]);
}

void CDjVuView::OnUpdateViewZoomIn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetZoom() < 800);
}

void CDjVuView::OnUpdateViewZoomOut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetZoom() > 10);
}

void CDjVuView::OnChangeMode(UINT nID)
{
	switch (nID)
	{
	case ID_MODE_SELECT:
		m_nMode = Select;
		break;

	case ID_MODE_MAGNIFY:
		m_nMode = MagnifyingGlass;
		break;

	case ID_MODE_SELECT_RECT:
		m_nMode = SelectRect;
		break;

	case ID_MODE_ZOOM_RECT:
		m_nMode = ZoomRect;
		break;

	case ID_MODE_DRAG:
	default:
		m_nMode = Drag;
		break;
	}

	theApp.GetAppSettings()->nDefaultMode = m_nMode;
}

void CDjVuView::OnUpdateMode(CCmdUI* pCmdUI)
{
	int nID = 0;
	switch (m_nMode)
	{
	case Drag:
		nID = ID_MODE_DRAG;
		break;

	case Select:
		nID = ID_MODE_SELECT;
		break;

	case MagnifyingGlass:
		nID = ID_MODE_MAGNIFY;
		break;

	case SelectRect:
		nID = ID_MODE_SELECT_RECT;
		break;

	case ZoomRect:
		nID = ID_MODE_ZOOM_RECT;
		break;
	}

	pCmdUI->SetCheck(nID == pCmdUI->m_nID);
}

void CDjVuView::OnEditCopy()
{
	if (m_nSelectionPage == -1 && !m_bHasSelection)
		return;

	CWaitCursor wait;

	if (!OpenClipboard())
		return;
	EmptyClipboard();

	if (m_bHasSelection)
	{
		wstring text;
		GetNormalizedText(text, true);
		int nLength = text.length();

		HGLOBAL hText = NULL, hUnicodeText = NULL;

		// Prepare Unicode text
		hUnicodeText = ::GlobalAlloc(GMEM_MOVEABLE, (nLength + 1)*sizeof(WCHAR));
		if (hUnicodeText != NULL)
		{
			// Lock the handle and copy the text to the buffer.
			LPWSTR pszUnicodeText = (LPWSTR)::GlobalLock(hUnicodeText);
			memmove(pszUnicodeText, text.c_str(), (nLength + 1)*sizeof(WCHAR));

			// Prepare ANSI text
			int nSize = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DISCARDNS,
					pszUnicodeText, -1, NULL, 0, NULL, NULL);
			if (nSize > 0)
			{
				hText = ::GlobalAlloc(GMEM_MOVEABLE, nSize*sizeof(CHAR));
				if (hText != NULL)
				{
					LPSTR pszText = (LPSTR)::GlobalLock(hText);
					WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DISCARDNS,
						pszUnicodeText, -1, pszText, nSize, NULL, NULL);
					::GlobalUnlock(pszText);
				}
			}

			::GlobalUnlock(pszUnicodeText);
		}

		if (hUnicodeText != NULL)
			SetClipboardData(CF_UNICODETEXT, hUnicodeText);
		if (hText != NULL)
			SetClipboardData(CF_TEXT, hText);
	}
	else
	{
		GP<DjVuImage> pImage = m_pSource->GetPage(m_nSelectionPage, NULL);
		if (pImage == NULL)
		{
			AfxMessageBox(IDS_ERROR_DECODING_PAGE, MB_ICONERROR | MB_OK);
		}
		else
		{
			CSize size = m_pages[m_nSelectionPage].GetSize(m_nRotate);
			CDIB* pBitmap = CRenderThread::Render(pImage, size, m_displaySettings, m_nDisplayMode, m_nRotate);

			CRect rcCrop = TranslatePageRect(m_nSelectionPage, m_rcSelection, false);
			CDIB* pCropped = pBitmap->Crop(rcCrop);
			delete pBitmap;
			pBitmap = pCropped;

			if (pBitmap == NULL || pBitmap->m_hObject == NULL)
			{
				AfxMessageBox(IDS_ERROR_RENDERING_PAGE, MB_ICONERROR | MB_OK);
			}
			else
			{
				CDIB* pNewBitmap = pBitmap->ReduceColors();
				if (pNewBitmap != NULL && pNewBitmap->m_hObject != NULL)
				{
					delete pBitmap;
					pBitmap = pNewBitmap;
				}
				else
				{
					delete pNewBitmap;
				}

				HGLOBAL hData = pBitmap->SaveToMemory();
				if (hData != NULL)
					SetClipboardData(CF_DIB, hData);
			}

			delete pBitmap;
		}
	}

	CloseClipboard();
}

void CDjVuView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bHasSelection || m_nSelectionPage != -1);
}

void CDjVuView::GetNormalizedText(wstring& text, bool bSelected, int nMaxLength)
{
	text.resize(0);
	int nLastHyphen = -1;
	bool bAgain = false;

	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		Page& page = m_pages[nPage];

		wstring chunk;
		if (bSelected)
		{
			if (page.nSelStart != -1)
			{
				ASSERT(page.info.pText != NULL);
				MakeWString(page.info.pText->textUTF8.substr(page.nSelStart, page.nSelEnd - page.nSelStart), chunk);
			}
		}
		else
		{
			if (!page.info.bDecoded || page.info.bHasText && !page.info.bTextDecoded)
			{
				page.Init(m_pSource, nPage, true);
				m_bNeedUpdate = true;
			}

			if (page.info.pText != NULL)
				MakeWString(page.info.pText->textUTF8, chunk);
		}

		if (text.length() + chunk.length() > text.capacity())
			text.reserve(max(2*text.length(), text.length() + chunk.length()));

		// Replace \n with \r\n
		size_t nLastPos = 0, nPos = 0;
		while ((nPos = chunk.find(L'\n', nLastPos)) != wstring::npos)
		{
			text.insert(text.length(), chunk, nLastPos, nPos - nLastPos);
			text += L"\r\n";
			nLastPos = nPos + 1;
		}
		text.insert(text.length(), chunk, nLastPos, chunk.length() - nLastPos);

		// If a word is hyphenated, both initial part and the full word
		// are usually stored in the text layer, and the latter is associated
		// with the part of the word after the hyphen. This is done to ensure
		// that hyphenated words can be located with the "Find" command.
		// The following code cuts the initial parts of hyphenated words,
		// to keep the extracted text clean.

		bAgain = false;
		int nHyphen = text.find(L'-', nLastHyphen + 1);
		while (nHyphen != string::npos)
		{
			int nPos = nHyphen;
			for (nPos = nHyphen - 1; nPos >= 0 && text[nPos] > 0x20 && (nHyphen - nPos < 50); --nPos)
				;

			wstring strPrev = text.substr(nPos + 1, nHyphen - nPos - 1);

			bool bFoundEOL = false;
			int nWordPos;
			for (nWordPos = nHyphen + 1;
				 nWordPos < static_cast<int>(text.length()) && text[nWordPos] <= 0x20;
				 ++nWordPos)
			{
				if (wcschr(L"\n\r\013\035\038", text[nWordPos]) != NULL)
					bFoundEOL = true;
			}

			if (bFoundEOL)
			{
				if (text.length() - nWordPos < strPrev.length())
				{
					nLastHyphen = nHyphen - 1;
					bAgain = true;
					break;
				}

				if (text.substr(nWordPos, strPrev.length()) == strPrev)
					text.erase(nWordPos, strPrev.length());
			}

			nHyphen = text.find(L'-', nHyphen + 1);
		}

		if (!bAgain)
			nLastHyphen = text.length() - 1;

		if (nMaxLength >= 0 && static_cast<int>(text.length()) >= nMaxLength)
			break;
	}

	if (m_bNeedUpdate)
	{
		UpdateLayout();
		Invalidate();
		m_bNeedUpdate = false;
	}
}

void CDjVuView::SettingsChanged()
{
	if (m_displaySettings != *theApp.GetDisplaySettings())
	{
		m_displaySettings = *theApp.GetDisplaySettings();

		DeleteBitmaps();
		m_pRenderThread->RejectCurrentJob();

		UpdateVisiblePages();
	}
}

void CDjVuView::OnViewDisplay(UINT nID)
{
	int nDisplayMode = Color;

	switch (nID)
	{
	case ID_DISPLAY_COLOR:
		nDisplayMode = Color;
		break;

	case ID_DISPLAY_BW:
		nDisplayMode = BlackAndWhite;
		break;

	case ID_DISPLAY_BACKGROUND:
		nDisplayMode = Background;
		break;

	case ID_DISPLAY_FOREGROUND:
		nDisplayMode = Foreground;
		break;
	}

	if (m_nType == Normal)
	{
		m_pSource->GetSettings()->nDisplayMode = nDisplayMode;
	}

	if (m_nDisplayMode != nDisplayMode)
	{
		m_nDisplayMode = nDisplayMode;

		DeleteBitmaps();
		m_pRenderThread->RejectCurrentJob();

		UpdateVisiblePages();
	}
}

void CDjVuView::OnUpdateViewDisplay(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_DISPLAY_COLOR:
		pCmdUI->SetCheck(m_nDisplayMode == Color);
		break;

	case ID_DISPLAY_BW:
		pCmdUI->SetCheck(m_nDisplayMode == BlackAndWhite);
		break;

	case ID_DISPLAY_BACKGROUND:
		pCmdUI->SetCheck(m_nDisplayMode == Background);
		break;

	case ID_DISPLAY_FOREGROUND:
		pCmdUI->SetCheck(m_nDisplayMode == Foreground);
		break;
	}
}

void CDjVuView::GoToSelection(int nPage, int nStartPos, int nEndPos, int nAddToHistory)
{
	if ((nAddToHistory & AddSource) != 0)
		GetMainFrame()->AddToHistory(this);

	Page& page = m_pages[nPage];
	bool bInfoLoaded;
	CWaitCursor* pWaitCursor = NULL;

	ClearSelection();
	SelectTextRange(nPage, nStartPos, nEndPos, bInfoLoaded, pWaitCursor);

	if (bInfoLoaded)
	{
		UpdateLayout();
		Invalidate();
	}

	if (!IsSelectionVisible(nPage, page.selection))
	{
		if (pWaitCursor == NULL)
			pWaitCursor = new CWaitCursor();

		EnsureSelectionVisible(nPage, page.selection);
	}

	if (pWaitCursor)
		delete pWaitCursor;

	if ((nAddToHistory & AddTarget) != 0)
		GetMainFrame()->AddToHistory(this);
}

void CDjVuView::OnViewFullscreen()
{
	if (m_nType == Fullscreen)
	{
		CFullscreenWnd* pFullscreenWnd = GetMainFrame()->GetFullscreenWnd();
		pFullscreenWnd->Hide();
		return;
	}

	m_pRenderThread->PauseJobs();
	CThumbnailsView* pThumbnails = ((CChildFrame*)GetParentFrame())->GetThumbnailsView();
	if (pThumbnails != NULL)
		pThumbnails->PauseDecoding();

	CFullscreenWnd* pWnd = GetMainFrame()->GetFullscreenWnd();

	CDjVuView* pView = (CDjVuView*)RUNTIME_CLASS(CDjVuView)->CreateObject();
	pView->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), pWnd, 2);
	pView->m_pDocument = m_pDocument;

	pWnd->Show(this, pView);

	pView->m_nMargin = c_nFullscreenMargin;
	pView->m_nShadowMargin = c_nFullscreenShadowMargin;
	pView->m_nPageGap = c_nFullscreenPageGap;
	pView->m_nFacingGap = c_nFullscreenFacingGap;
	pView->m_nPageBorder = c_nFullscreenPageBorder;
	pView->m_nPageShadow = c_nFullscreenPageShadow;

	pView->m_nType = Fullscreen;
	pView->m_nMode = (theApp.GetAppSettings()->bFullscreenClicks ? NextPrev : Drag);
	pView->m_nLayout = (m_nLayout == Facing || m_nLayout == ContinuousFacing ? Facing : SinglePage);
	pView->m_bFirstPageAlone = m_bFirstPageAlone;
	pView->m_nZoomType = ZoomFitPage;
	pView->m_nDisplayMode = m_nDisplayMode;
	pView->m_nRotate = m_nRotate;
	pView->m_nPage = m_nPage;

	if (theApp.GetAppSettings()->bFullscreenHideScroll)
	{
		pView->ShowScrollBar(SB_BOTH, FALSE);
		pView->CreateScrollbars();
	}

	pView->OnInitialUpdate();

	pView->UpdatePageInfoFrom(this);
	pView->CopyBitmapsFrom(this);

	pView->ShowWindow(SW_SHOW);
	pView->SetFocus();

	pView->UpdateLayout();
	pView->UpdateWindow();
}

void CDjVuView::ResumeDecoding()
{
	m_pRenderThread->ResumeJobs();

	UpdateLayout();
	UpdateVisiblePages();
	Invalidate();
}

int CDjVuView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	if (m_nType != Normal)
	{
		// From MFC: CView::OnMouseActivate
		// Don't call CFrameWnd::SetActiveView

		int nResult = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
		if (nResult == MA_NOACTIVATE || nResult == MA_NOACTIVATEANDEAT)
			return nResult;

		// set focus to this view, but don't notify the parent frame
		OnActivateView(TRUE, this, this);
		return nResult;
	}

	return CScrollView::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

bool CDjVuView::UpdatePageInfoFrom(CDjVuView* pFrom)
{
	ASSERT(m_nPageCount == pFrom->m_nPageCount);

	bool bUpdated = false;
	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		Page& page = m_pages[nPage];
		Page& srcPage = pFrom->m_pages[nPage];

		if (!page.info.bDecoded && srcPage.info.bDecoded)
			bUpdated = true;

		page.info.Update(srcPage.info);
	}

	return bUpdated;
}

void CDjVuView::CopyBitmapsFrom(CDjVuView* pFrom, bool bMove)
{
	ASSERT(m_nPageCount == pFrom->m_nPageCount);

	for (int nPage = 0; nPage < m_nPageCount; ++nPage)
	{
		Page& page = m_pages[nPage];
		Page& srcPage = pFrom->m_pages[nPage];

		if (page.pBitmap == NULL && srcPage.pBitmap != NULL)
		{
			if (bMove)
			{
				page.pBitmap = srcPage.pBitmap;
				page.bBitmapRendered = true;
				srcPage.pBitmap = NULL;
				srcPage.bBitmapRendered = false;
			}
			else
			{
				page.pBitmap = CDIB::CreateDIB(srcPage.pBitmap);
				page.bBitmapRendered = true;
			}
		}
	}
}

void CDjVuView::CopyBitmapFrom(CDjVuView* pFrom, int nPage)
{
	ASSERT(nPage >= 0 && nPage < pFrom->m_nPageCount);

	Page& page = m_pages[nPage];
	Page& srcPage = pFrom->m_pages[nPage];

	if (page.pBitmap == NULL && srcPage.pBitmap != NULL)
	{
		page.pBitmap = CDIB::CreateDIB(srcPage.pBitmap);
		page.bBitmapRendered = true;
	}
}

void CDjVuView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nMode == Select)
	{
		ClearSelection();

		int nPage = GetPageFromPoint(point);
		if (nPage == -1)
			return;

		int nPos = GetTextPosFromPoint(nPage, point);
		bool bInfoLoaded = false;
		CWaitCursor* pWaitCursor = NULL;
		SelectTextRange(nPage, nPos, nPos + 1, bInfoLoaded, pWaitCursor);

		if (bInfoLoaded)
		{
			UpdateLayout();
			Invalidate();
		}

		if (pWaitCursor != NULL)
			delete pWaitCursor;
	}

	CMyScrollView::OnLButtonDblClk(nFlags, point);
}

void CDjVuView::UpdateKeyboard(UINT nKey, bool bDown)
{
	bool bUpdate = false;
	bool bUpdateWindow = false;
	if (nKey == VK_SHIFT)
	{
		bUpdateWindow = (m_bShiftDown != bDown);
		bUpdate = bUpdateWindow;
		m_bShiftDown = bDown;
	}
	else if (nKey == VK_CONTROL)
	{
		bUpdate = (m_bControlDown != bDown);
		m_bControlDown = bDown;
	}

	if (bUpdateWindow)
	{
		Invalidate();
		UpdateWindow();
	}

	if (bUpdate)
		UpdateCursor();
}

void CDjVuView::OnViewGotoPage()
{
	CGotoPageDlg dlg(m_nPage, m_nPageCount);
	if (dlg.DoModal() == IDOK)
		GoToPage(dlg.m_nPage - 1);
}

void CDjVuView::OnTimer(UINT nIDEvent)
{
	if (nIDEvent != 1)
	{
		CMyScrollView::OnTimer(nIDEvent);
		return;
	}

	if (m_bNeedUpdate)
	{
		UpdateLayout();
		Invalidate();
		m_bNeedUpdate = false;
	}

	if (m_nType == Fullscreen && !m_bCursorHidden)
	{
		if (GetFocus() == this && !m_bDragging && !m_bPanning && !m_bPopupMenu)
		{
			int nTickCount = ::GetTickCount();
			if (nTickCount - m_nCursorTime > s_nCursorHideDelay)
			{
				m_bCursorHidden = true;
				::ShowCursor(false);
			}
		}
		else
		{
			m_nCursorTime = ::GetTickCount();
		}
	}

	if (m_bDraggingText || m_bDraggingRect)
	{
		// Autoscroll
		CPoint ptCursor;
		::GetCursorPos(&ptCursor);

		CRect rcClient;
		GetClientRect(rcClient);
		ClientToScreen(rcClient);

		CSize szOffset(0, 0);
		if (ptCursor.x < rcClient.left)
			szOffset.cx = -50;
		if (ptCursor.x >= rcClient.right)
			szOffset.cx = 50;

		if (ptCursor.y < rcClient.top)
			szOffset.cy = -50;
		if (ptCursor.y >= rcClient.bottom)
			szOffset.cy = 50;

		if (szOffset.cx != 0 || szOffset.cy != 0)
		{
			if (OnScrollBy(szOffset))
			{
				UpdateVisiblePages();
				UpdateDragAction();
			}
		}
	}
}

void CDjVuView::ShowCursor()
{
	if (m_bCursorHidden)
	{
		::ShowCursor(true);
		m_bCursorHidden = false;
	}

	m_nCursorTime = ::GetTickCount();
}

void CDjVuView::OnFirstPageAlone()
{
	int nAnchorPage = m_nPage;
	int nOffset = GetScrollPos(SB_VERT) - m_pages[nAnchorPage].ptOffset.y;

	m_bFirstPageAlone = !m_bFirstPageAlone;

	if (m_nType == Normal)
	{
		theApp.GetAppSettings()->bFirstPageAlone = m_bFirstPageAlone;
		m_pSource->GetSettings()->bFirstPageAlone = m_bFirstPageAlone;
	}

	if (m_nLayout == Facing || m_nLayout == ContinuousFacing)
		SetLayout(m_nLayout, nAnchorPage, nOffset);
}

void CDjVuView::OnUpdateFirstPageAlone(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bFirstPageAlone);
}

bool CDjVuView::IsValidPage(int nPage) const
{
	ASSERT(m_nLayout == Facing || m_nLayout == ContinuousFacing);

	if (m_bFirstPageAlone)
		return (nPage == 0 || nPage % 2 == 1);
	else
		return (nPage % 2 == 0);
}

bool CDjVuView::HasFacingPage(int nPage) const
{
	ASSERT(IsValidPage(nPage));

	if (m_bFirstPageAlone)
		return (nPage > 0 && nPage < m_nPageCount - 1);
	else
		return (nPage < m_nPageCount - 1);
}

int CDjVuView::FixPageNumber(int nPage) const
{
	if (m_nLayout == SinglePage || m_nLayout == Continuous)
		return nPage;

	if (m_bFirstPageAlone)
		return (nPage == 0 ? 0 : nPage - ((nPage - 1) % 2));
	else
		return (nPage - nPage % 2);
}

int CDjVuView::GetNextPage(int nPage) const
{
	if (m_nLayout == SinglePage || m_nLayout == Continuous)
		return nPage + 1;

	ASSERT(IsValidPage(nPage));
	if (m_bFirstPageAlone)
		return (nPage == 0 ? 1 : nPage + 2);
	else
		return (nPage + 2);
}

void CDjVuView::SetLayout(int nLayout, int nPage, int nOffset)
{
	m_nLayout = nLayout;

	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		RenderPage(nPage);
	}
	else
	{
		UpdateLayout(RECALC);
		UpdatePageSizes(m_pages[nPage].ptOffset.y);

		if (nOffset < 0)
		{
			CRect rcClient;
			GetClientRect(rcClient);

			int nTop = m_pages[nPage].ptOffset.y + nOffset;
			int nBottom = min(m_szDisplay.cy, nTop + rcClient.Height());

			UpdatePagesFromBottom(nBottom - rcClient.Height() + nOffset, nBottom);
		}
	}

	int nTop = m_pages[nPage].ptOffset.y + nOffset;
	OnScrollBy(CPoint(GetScrollPos(SB_HORZ), nTop) - GetScrollPosition());

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdateVisiblePages();
		UpdatePageNumber();
	}

	Invalidate();
	UpdateHoverAnnotation();
	UpdateCursor();
}

void CDjVuView::UpdateDragAction()
{
	if (m_bDragging)
	{
		m_ptPrevCursor = CPoint(-1, -1);

		CPoint ptCursor;
		::GetCursorPos(&ptCursor);
		ScreenToClient(&ptCursor);

		OnMouseMove(MK_LBUTTON, ptCursor);
	}
}

CScrollBar* CDjVuView::GetScrollBarCtrl(int nBar) const
{
	if (m_nType == Magnify || m_nType == Fullscreen && theApp.GetAppSettings()->bFullscreenHideScroll)
		return (nBar == SB_HORZ ? m_pHScrollBar : m_pVScrollBar);

	return NULL;
}

void CDjVuView::CreateScrollbars()
{
	m_pHScrollBar = new CScrollBar();
	m_pHScrollBar->Create(SBS_HORZ | WS_CHILD, CRect(0, 0, 0, 0), this, 0);

	m_pVScrollBar = new CScrollBar();
	m_pVScrollBar->Create(SBS_VERT |WS_CHILD, CRect(0, 0, 0, 0), this, 1);
}

void CDjVuView::StartMagnify()
{
	CPoint ptCursor;
	::GetCursorPos(&ptCursor);

	CMagnifyWnd* pMagnifyWnd = GetMainFrame()->GetMagnifyWnd();

	CDjVuView* pView = (CDjVuView*)RUNTIME_CLASS(CDjVuView)->CreateObject();
	pView->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), pMagnifyWnd, 2);
	pView->m_pDocument = m_pDocument;

	pMagnifyWnd->Show(this, pView, ptCursor);

	ScreenToClient(&ptCursor);
	int nPage = GetPageNearPoint(ptCursor);
	if (nPage == -1)
		nPage = m_nPage;

	int nExtraMargin = max(pMagnifyWnd->GetViewWidth(), pMagnifyWnd->GetViewHeight());

	pView->m_nMargin = m_nMargin + nExtraMargin;
	pView->m_nShadowMargin = m_nShadowMargin + nExtraMargin;
	pView->m_nPageGap = m_nPageGap;
	pView->m_nFacingGap = m_nFacingGap;
	pView->m_nPageBorder = m_nPageBorder;
	pView->m_nPageShadow = m_nPageShadow;

	pView->m_nType = Magnify;
	pView->m_nMode = Drag;
	pView->m_nLayout = m_nLayout;
	pView->m_bFirstPageAlone = m_bFirstPageAlone;
	pView->m_nDisplayMode = m_nDisplayMode;
	pView->m_nRotate = m_nRotate;
	pView->m_nPage = FixPageNumber(nPage);

	double fZoom = GetZoom();
	double fZoomActualSize = GetZoom(ZoomActualSize);
	if (m_nZoomType == ZoomActualSize || fZoom < fZoomActualSize)
	{
		pView->m_nZoomType = ZoomActualSize;
	}
	else
	{
		pView->m_nZoomType = ZoomPercent;
		pView->m_fZoom = fZoom;
	}

	pView->ShowScrollBar(SB_BOTH, FALSE);
	pView->CreateScrollbars();
	pView->OnInitialUpdate();

	pView->UpdatePageInfoFrom(this);

	pView->ShowWindow(SW_SHOW);
	pView->UpdateLayout();
}

void CDjVuView::UpdateMagnifyWnd()
{
	CMagnifyWnd* pMagnifyWnd = GetMainFrame()->GetMagnifyWnd();
	if (!pMagnifyWnd->IsWindowVisible())
	{
		StopDragging();
		return;
	}

	ASSERT(pMagnifyWnd->GetOwner() == this);

	CPoint ptCursor;
	::GetCursorPos(&ptCursor);

	if (ptCursor == m_ptPrevCursor)
		return;
	m_ptPrevCursor = ptCursor;

	CDjVuView* pView = pMagnifyWnd->GetView();
	pView->SetRedraw(false);

	CPoint ptCenter = ptCursor;

	ScreenToClient(&ptCursor);
	int nPage = GetPageNearPoint(ptCursor);
	if (nPage == -1)
		nPage = m_nPage;

	if (pView->UpdatePageInfoFrom(this))
	{
		pView->UpdateLayout();
		pView->UpdateWindow();
	}

	if (pView->m_nLayout == SinglePage || pView->m_nLayout == Facing)
	{
		int nNewPage = FixPageNumber(nPage);
		if (nNewPage != pView->m_nPage)
			pView->RenderPage(nNewPage);
	}

	const Page& page = m_pages[nPage];
	CSize szPage = page.GetSize(m_nRotate);
	double fRatioX = pView->m_pages[nPage].szBitmap.cx / (1.0*page.szBitmap.cx);
	double fRatioY = pView->m_pages[nPage].szBitmap.cy / (1.0*page.szBitmap.cy);

	ptCursor += GetScrollPosition() - page.ptOffset;
	CPoint ptResult(static_cast<int>(ptCursor.x * fRatioX), static_cast<int>(ptCursor.y * fRatioY));

	CPoint ptOffset = pView->m_pages[nPage].ptOffset + ptResult -
			CPoint(pMagnifyWnd->GetViewWidth() / 2, pMagnifyWnd->GetViewHeight() / 2);

	pView->OnScrollBy(ptOffset - pView->GetScrollPosition());
	if (pView->m_nLayout == Continuous || pView->m_nLayout == ContinuousFacing)
	{
		pView->UpdateVisiblePages();
	}

	pView->SetRedraw(true);

	pMagnifyWnd->Invalidate();
	pMagnifyWnd->CenterOnPoint(ptCenter);

	GetTopLevelParent()->UpdateWindow();
	pMagnifyWnd->UpdateWindow();
}

void CDjVuView::OnStartPan()
{
	StopDragging();
	ShowCursor();
	m_bPanning = true;
}

void CDjVuView::OnPan(CSize szScroll)
{
	if ((m_nLayout == Continuous || m_nLayout == ContinuousFacing) && szScroll.cy != 0)
		UpdatePageSizes(GetScrollPos(SB_VERT), szScroll.cy);

	OnScrollBy(szScroll, true);

	if (m_nLayout == Continuous || m_nLayout == ContinuousFacing)
	{
		UpdateVisiblePages();
		UpdatePageNumber();
	}
}

void CDjVuView::OnEndPan()
{
	ShowCursor();
	m_bPanning = false;
}

void CDjVuView::OnUpdate(const Observable* source, const Message* message)
{
	if (message->code == PAGE_RENDERED)
	{
		const BitmapMsg* msg = (const BitmapMsg*) message;
		PageRendered(msg->nPage, msg->pDIB);
	}
	else if (message->code == PAGE_DECODED)
	{
		const PageMsg* msg = (const PageMsg*) message;
		PageDecoded(msg->nPage);
	}
	else if (message->code == THUMBNAIL_CLICKED)
	{
		const PageMsg* msg = (const PageMsg*) message;
		GoToPage(msg->nPage);
	}
	else if (message->code == LINK_CLICKED)
	{
		const LinkClicked* msg = (const LinkClicked*) message;
		GoToURL(msg->url);
	}
	else if (message->code == BOOKMARK_CLICKED)
	{
		const BookmarkMsg* msg = (const BookmarkMsg*) message;
		GoToBookmark(*msg->pBookmark);
	}
	else if (message->code == SEARCH_RESULT_CLICKED)
	{
		const SearchResultClicked* msg = (const SearchResultClicked*) message;
		GoToSelection(msg->nPage, msg->nSelStart, msg->nSelEnd);
	}
	else if (message->code == APP_SETTINGS_CHANGED)
	{
		SettingsChanged();
	}
	else if (message->code == ANNOTATION_DELETED)
	{
		const AnnotationMsg* msg = (const AnnotationMsg*) message;

		if (m_pClickedAnno == msg->pAnno)
			m_pClickedAnno = NULL;

		UpdateHoverAnnotation();
	}
	else if (message->code == ANNOTATION_ADDED)
	{
		const AnnotationMsg* msg = (const AnnotationMsg*) message;
		InvalidateAnno(msg->pAnno, msg->nPage);

		UpdateHoverAnnotation();
	}
	else if (message->code == KEY_STATE_CHANGED)
	{
		const KeyStateChanged* msg = (const KeyStateChanged*) message;
		UpdateKeyboard(msg->nKey, msg->bPressed);
	}
}

void CDjVuView::UpdatePageNumber()
{
	if (m_nLayout == SinglePage || m_nLayout == Facing)
	{
		UpdateObservers(PageMsg(CURRENT_PAGE_CHANGED, m_nPage));
	}
	else
	{
		int nCurrentPage = CalcCurrentPage();
		if (nCurrentPage != m_nPage)
		{
			m_nPage = nCurrentPage;
			UpdateObservers(PageMsg(CURRENT_PAGE_CHANGED, m_nPage));
		}
	}

	if (m_nType == Normal)
	{
		Bookmark bmView;
		CreateBookmarkFromView(bmView);

		m_pSource->GetSettings()->nPage = bmView.nPage;
		m_pSource->GetSettings()->ptOffset = bmView.ptOffset;
	}
}

void CDjVuView::OnHighlight(UINT nID)
{
	if (m_nSelectionPage == -1 && !m_bHasSelection)
		return;

	CAnnotationDlg dlg(IDS_CREATE_ANNOTATION);

	dlg.m_nBorderType = theApp.GetAnnoTemplate()->nBorderType;
	dlg.m_crBorder = theApp.GetAnnoTemplate()->crBorder;
	dlg.m_bHideInactive = theApp.GetAnnoTemplate()->bHideInactiveBorder;
	dlg.m_nFillType = theApp.GetAnnoTemplate()->nFillType;
	dlg.m_crFill = theApp.GetAnnoTemplate()->crFill;
	dlg.m_nTransparency = static_cast<int>(theApp.GetAnnoTemplate()->fTransparency * 100.0 + 0.5);

	if (dlg.DoModal() != IDOK)
		return;

	theApp.GetAnnoTemplate()->nBorderType = dlg.m_nBorderType;
	theApp.GetAnnoTemplate()->crBorder = dlg.m_crBorder;
	theApp.GetAnnoTemplate()->bHideInactiveBorder = !!dlg.m_bHideInactive;
	theApp.GetAnnoTemplate()->nFillType = dlg.m_nFillType;
	theApp.GetAnnoTemplate()->crFill = dlg.m_crFill;
	theApp.GetAnnoTemplate()->fTransparency = dlg.m_nTransparency / 100.0;

	Annotation annoTemplate = *theApp.GetAnnoTemplate();
	annoTemplate.strComment = MakeUTF8String(dlg.m_strComment);

	Annotation* pNewAnno = NULL;
	int nAnnoPage = -1;

	if (m_nSelectionPage != -1)
	{
		Annotation anno = annoTemplate;
		anno.rects.push_back(m_rcSelection);
		anno.UpdateBounds();

		nAnnoPage = m_nSelectionPage;
		pNewAnno = m_pSource->GetSettings()->AddAnnotation(anno, m_nSelectionPage);
	}
	else if (m_bHasSelection)
	{
		for (int nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			Page& page = m_pages[nPage];

			if (!page.selection.isempty())
			{
				Annotation anno = annoTemplate;
				for (GPosition pos = page.selection; pos; ++pos)
					anno.rects.push_back(page.selection[pos]->rect);
				anno.UpdateBounds();

				Annotation* pAnno = m_pSource->GetSettings()->AddAnnotation(anno, nPage);
				if (pNewAnno == NULL)
				{
					nAnnoPage = nPage;
					pNewAnno = pAnno;
				}
			}
		}
	}

	ClearSelection();

	if (dlg.m_bAddBookmark && pNewAnno != NULL)
	{
		Bookmark bookmark;
		CreateBookmarkFromAnnotation(bookmark, pNewAnno, nAnnoPage);
		bookmark.strTitle = MakeUTF8String(dlg.m_strBookmark);

		m_pSource->GetSettings()->bookmarks.push_back(bookmark);
		Bookmark& bmNew = m_pSource->GetSettings()->bookmarks.back();

		CFrameWnd* pFrame;
		if (m_nType == Fullscreen)
		{
			pFrame = ((CFullscreenWnd*) GetTopLevelParent())->GetOwner()->GetParentFrame();
		}
		else
		{
			ASSERT(m_nType == Normal);
			pFrame = GetParentFrame();
		}

		CBookmarksWnd* pBookmarkWnd = ((CChildFrame*) pFrame)->GetCustomBookmarks();
		pBookmarkWnd->AddBookmark(bmNew);
	}
}

void CDjVuView::OnUpdateHighlight(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bHasSelection || m_nSelectionPage != -1);
}

void CDjVuView::OnDeleteAnnotation()
{
	if (m_pClickedAnno == NULL || !m_bClickedCustom || m_nClickedPage == -1)
		return;

	if (AfxMessageBox(IDS_PROMPT_ANNOTATION_DELETE, MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
	{
		m_pSource->GetSettings()->DeleteAnnotation(m_pClickedAnno, m_nClickedPage);
	}
}

void CDjVuView::OnEditAnnotation()
{
	if (m_pClickedAnno == NULL || !m_bClickedCustom || m_nClickedPage == -1)
		return;

	CAnnotationDlg dlg(IDS_EDIT_ANNOTATION);

	dlg.m_bEnableBookmark = false;
	dlg.m_nBorderType = m_pClickedAnno->nBorderType;
	dlg.m_crBorder = m_pClickedAnno->crBorder;
	dlg.m_bHideInactive = m_pClickedAnno->bHideInactiveBorder;
	dlg.m_nFillType = m_pClickedAnno->nFillType;
	dlg.m_crFill = m_pClickedAnno->crFill;
	dlg.m_nTransparency = static_cast<int>(m_pClickedAnno->fTransparency * 100.0 + 0.5);
	dlg.m_strComment = MakeCString(m_pClickedAnno->strComment);

	if (dlg.DoModal() == IDOK)
	{
		theApp.GetAnnoTemplate()->nBorderType = dlg.m_nBorderType;
		theApp.GetAnnoTemplate()->crBorder = dlg.m_crBorder;
		theApp.GetAnnoTemplate()->bHideInactiveBorder = !!dlg.m_bHideInactive;
		theApp.GetAnnoTemplate()->nFillType = dlg.m_nFillType;
		theApp.GetAnnoTemplate()->crFill = dlg.m_crFill;
		theApp.GetAnnoTemplate()->fTransparency = dlg.m_nTransparency / 100.0;

		m_pClickedAnno->nBorderType = dlg.m_nBorderType;
		m_pClickedAnno->crBorder = dlg.m_crBorder;
		m_pClickedAnno->bHideInactiveBorder = !!dlg.m_bHideInactive;
		m_pClickedAnno->nFillType = dlg.m_nFillType;
		m_pClickedAnno->crFill = dlg.m_crFill;
		m_pClickedAnno->fTransparency = dlg.m_nTransparency / 100.0;
		m_pClickedAnno->strComment = MakeUTF8String(dlg.m_strComment);

		InvalidateAnno(m_pClickedAnno, m_nClickedPage);
	}
}

void CDjVuView::OnAddBookmark()
{
	Bookmark bookmark;
	UINT nDlgTitle;

	if (m_nSelectionPage != -1 || m_bHasSelection)
	{
		CreateBookmarkFromSelection(bookmark);
		nDlgTitle = IDS_BOOKMARK_SELECTION;
	}
	else if (m_pClickedAnno != NULL)
	{
		CreateBookmarkFromAnnotation(bookmark, m_pClickedAnno, m_nClickedPage);
		nDlgTitle = IDS_BOOKMARK_ANNOTATION;
	}
	else
	{
		CreateBookmarkFromView(bookmark);
		nDlgTitle = IDS_BOOKMARK_VIEW;
	}

	CBookmarkDlg dlg(nDlgTitle);
	dlg.m_strTitle = MakeCString(bookmark.strTitle);

	if (dlg.DoModal() == IDOK)
	{
		bookmark.strTitle = MakeUTF8String(dlg.m_strTitle);

		m_pSource->GetSettings()->bookmarks.push_back(bookmark);
		Bookmark& bmNew = m_pSource->GetSettings()->bookmarks.back();

		CFrameWnd* pFrame;
		if (m_nType == Fullscreen)
		{
			pFrame = ((CFullscreenWnd*) GetTopLevelParent())->GetOwner()->GetParentFrame();
		}
		else
		{
			ASSERT(m_nType == Normal);
			pFrame = GetParentFrame();
		}

		CBookmarksWnd* pBookmarkWnd = ((CChildFrame*) pFrame)->GetCustomBookmarks();
		pBookmarkWnd->AddBookmark(bmNew);
	}
}

void MakeBookmarkTitle(const wstring& strText, wstring& result)
{
	result.resize(0);
	result.reserve(strText.length());

	bool bSkipSpaces = true;
	for (size_t i = 0; i < strText.length(); ++i)
	{
		if (strText[i] <= 0x20)
		{
			if (!bSkipSpaces)
				result += L' ';
			bSkipSpaces = true;
		}
		else
		{
			result += strText[i];
			bSkipSpaces = false;
		}
	}

	if (result.length() > 100)
	{
		result.erase(100);
		result += L"...";
	}
}

bool CDjVuView::CreateBookmarkFromSelection(Bookmark& bookmark)
{
	if (m_nSelectionPage != -1)
	{
		bookmark.nLinkType = Bookmark::View;
		bookmark.nPage = m_nSelectionPage;
		bookmark.ptOffset = CPoint(m_rcSelection.xmin, m_rcSelection.ymax);
		bookmark.bMargin = true;
		return true;
	}
	else if (m_bHasSelection)
	{
		for (int nPage = 0; nPage < m_nPageCount; ++nPage)
		{
			const Page& page = m_pages[nPage];

			if (!page.selection.isempty())
			{
				GPosition pos = page.selection;
				GRect rect = page.selection[pos]->rect;

				bookmark.nLinkType = Bookmark::View;
				bookmark.nPage = nPage;
				bookmark.ptOffset = CPoint(rect.xmin, rect.ymax);
				bookmark.bMargin = true;

				wstring strText, strTitle;
				GetNormalizedText(strText, true, 200);
				MakeBookmarkTitle(strText, strTitle);
				bookmark.strTitle = MakeUTF8String(strTitle);

				return true;
			}
		}
	}

	return false;
}

void CDjVuView::CreateBookmarkFromView(Bookmark& bookmark)
{
	int nPage;
	if (m_nLayout == SinglePage || m_nLayout == Facing)
		nPage = m_nPage;
	else
		nPage = CalcTopPage();

	CPoint ptOffset = GetScrollPosition() - m_pages[nPage].ptOffset;
	CPoint ptDjVuOffset = ScreenToDjVu(nPage, ptOffset);

	bookmark.nLinkType = Bookmark::View;
	bookmark.nPage = nPage;
	bookmark.ptOffset = ptDjVuOffset;
	bookmark.bMargin = false;
}

void CDjVuView::CreateBookmarkFromAnnotation(Bookmark& bookmark, const Annotation* pAnno, int nPage)
{
	bookmark.nLinkType = Bookmark::View;
	bookmark.nPage = nPage;
	bookmark.ptOffset = CPoint(pAnno->rectBounds.xmin, pAnno->rectBounds.ymax);
	bookmark.bMargin = true;

	wstring strComment, strTitle;
	MakeWString(pAnno->strComment, strComment);
	MakeBookmarkTitle(strComment, strTitle);
	bookmark.strTitle = MakeUTF8String(strTitle);
}

void CDjVuView::CreateBookmarkFromPage(Bookmark& bookmark, int nPage)
{
	bookmark.nLinkType = Bookmark::Page;
	bookmark.nPage = nPage;
}

void CDjVuView::OnZoomToSelection()
{
	if (m_nSelectionPage == -1)
		return;

	CRect rcClient;
	GetClientRect(rcClient);
	if ((GetStyle() & WS_VSCROLL) == 0)
		rcClient.right -= ::GetSystemMetrics(SM_CXVSCROLL);
	if ((GetStyle() & WS_HSCROLL) == 0)
		rcClient.bottom -= ::GetSystemMetrics(SM_CYHSCROLL);

	const Page& page = m_pages[m_nSelectionPage];
	CSize szPage = page.GetSize(m_nRotate);

	CScreenDC dcScreen;
	int nLogPixels = dcScreen.GetDeviceCaps(LOGPIXELSX);

	double fScale = min(1.0*rcClient.Width()/m_rcSelection.width(),
			1.0*rcClient.Height()/m_rcSelection.height());
	double fZoom = 100.0*fScale*page.info.nDPI/nLogPixels;

	ZoomTo(ZoomPercent, fZoom);

	CRect rcSel = TranslatePageRect(m_nSelectionPage, m_rcSelection);
	CPoint ptOffset = rcSel.TopLeft();

	rcSel -= GetScrollPosition();
	if (rcSel.Width() < rcClient.Width())
		ptOffset.x -= (rcClient.Width() - rcSel.Width()) / 2;
	if (rcSel.Height() < rcClient.Height())
		ptOffset.y -= (rcClient.Height() - rcSel.Height()) / 2;

	RenderPage(m_nSelectionPage, -1, false);

	OnScrollBy(ptOffset - GetScrollPosition());

	UpdateVisiblePages();
	UpdatePageNumber();
	UpdateHoverAnnotation();
}

void CDjVuView::OnSwitchFocus()
{
	if (m_nType != Normal)
		return;

	if (GetFocus() != this)
	{
		SetFocus();
		return;
	}

	CChildFrame* pFrame = ((CChildFrame*) GetParentFrame());
	if (!pFrame->IsNavPaneHidden() && !pFrame->IsNavPaneCollapsed())
		pFrame->GetNavPane()->SetFocus();
}
