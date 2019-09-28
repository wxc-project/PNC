#if !defined(AFX_MOUSESTATUSBAR_H__6359980E_5BA4_4CC4_97D3_A64C0A5A1833__INCLUDED_)
#define AFX_MOUSESTATUSBAR_H__6359980E_5BA4_4CC4_97D3_A64C0A5A1833__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MouseStatusBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMouseStatusBar window

class CMouseStatusBar : public CMFCStatusBar//Ctrl
{
// Construction
public:
	CMouseStatusBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMouseStatusBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF m_crTxtColor;
	virtual ~CMouseStatusBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMouseStatusBar)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOUSESTATUSBAR_H__6359980E_5BA4_4CC4_97D3_A64C0A5A1833__INCLUDED_)
