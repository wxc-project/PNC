#if !defined(AFX_SIDEBAFFLEHIGHDLG_H__CE0348CB_07E1_47AF_9A55_9BF889BF97A9__INCLUDED_)
#define AFX_SIDEBAFFLEHIGHDLG_H__CE0348CB_07E1_47AF_9A55_9BF889BF97A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SideBaffleHighDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSideBaffleHighDlg dialog

class CSideBaffleHighDlg : public CDialog
{
// Construction
public:
	CSideBaffleHighDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSideBaffleHighDlg)
	enum { IDD = IDD_SIDE_BAFFLE_HIGH_DLG };
	double	m_fSideBaffleHigh;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSideBaffleHighDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSideBaffleHighDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIDEBAFFLEHIGHDLG_H__CE0348CB_07E1_47AF_9A55_9BF889BF97A9__INCLUDED_)
