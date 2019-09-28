// MouseStatusBar.cpp : implementation file
//

#include "stdafx.h"
#include "PPE.h"
#include "MouseStatusBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMouseStatusBar

CMouseStatusBar::CMouseStatusBar()
{
	m_crTxtColor=0;
}

CMouseStatusBar::~CMouseStatusBar()
{
}


BEGIN_MESSAGE_MAP(CMouseStatusBar, CMFCStatusBar)
	//{{AFX_MSG_MAP(CMouseStatusBar)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMouseStatusBar message handlers

void CMouseStatusBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CRect rect;
	GetItemRect(2,&rect);
	//if(rect.PtInRect(point))//处理未处理信息包（如阅读消息或更新数据等）
	//	theApp.ProcessInfoPackage();
	
	CMFCStatusBar::OnLButtonDblClk(nFlags, point);
}

void CMouseStatusBar::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	dc.SetTextColor(m_crTxtColor);
	CMFCStatusBar::OnDrawItem(nIDCtl, lpDrawItemStruct);
	dc.Detach();
}
