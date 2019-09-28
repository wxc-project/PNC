#if !defined(AFX_PLANKVERTEXDLG_H__48C4BEE7_DDA5_487E_A985_5A371BC68E0E__INCLUDED_)
#define AFX_PLANKVERTEXDLG_H__48C4BEE7_DDA5_487E_A985_5A371BC68E0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlankVertexDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPlankVertexDlg dialog
#include "Resource.h"

class CPlankVertexDlg : public CDialog
{
// Construction
public:
	CPlankVertexDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPlankVertexDlg)
	enum { IDD = IDD_PLATE_VERTEX_DLG };
	int		m_iVertexType;
	double	m_fPosX;
	double	m_fPosY;
	BOOL	m_bPolarCS;
	BOOL	m_bCartesianCS;
	double	m_fAlfa;
	double	m_fR;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlankVertexDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPlankVertexDlg)
	afx_msg void OnChkTransByPolar();
	afx_msg void OnChkTransByCartesian();
	afx_msg void OnChangeEPolarCoord();
	afx_msg void OnChangeECartesianCoord();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLANKVERTEXDLG_H__48C4BEE7_DDA5_487E_A985_5A371BC68E0E__INCLUDED_)
