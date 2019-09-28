#if !defined(AFX_2DPTDLG_H__FF1A7A36_2CD2_4F3D_8F6B_55BF02467445__INCLUDED_)
#define AFX_2DPTDLG_H__FF1A7A36_2CD2_4F3D_8F6B_55BF02467445__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// 2DPtDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// C2DPtDlg dialog
#include "Resource.h"

class C2DPtDlg : public CDialog
{
// Construction
public:
	BOOL m_bCanModify;
	C2DPtDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(C2DPtDlg)
	enum { IDD = IDD_2D_VERTEX_DLG };
	double	m_fPtX;
	double	m_fPtY;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C2DPtDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(C2DPtDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_2DPTDLG_H__FF1A7A36_2CD2_4F3D_8F6B_55BF02467445__INCLUDED_)
