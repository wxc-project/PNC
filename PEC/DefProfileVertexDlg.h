#if !defined(AFX_DEFPROFILEVERTEXDLG_H__6DC4E44C_CA25_408B_81AE_87863E3DC408__INCLUDED_)
#define AFX_DEFPROFILEVERTEXDLG_H__6DC4E44C_CA25_408B_81AE_87863E3DC408__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DefProfileVertexDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDefProfileVertexDlg dialog
#include "Resource.h"

class CDefProfileVertexDlg : public CDialog
{
// Construction
public:
	f3dPoint pickPoint;
	f3dArcLine datum_line;
	CDefProfileVertexDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDefProfileVertexDlg)
	enum { IDD = IDD_DEF_VERTEX_DLG };
	int		m_iDefPointType;
	double	m_fPosX;
	double	m_fPosY;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDefProfileVertexDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDefProfileVertexDlg)
	afx_msg void OnSelchangeCmbDefType();
	afx_msg void OnChangeEPtX();
	afx_msg void OnChangeEPtY();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEFPROFILEVERTEXDLG_H__6DC4E44C_CA25_408B_81AE_87863E3DC408__INCLUDED_)
