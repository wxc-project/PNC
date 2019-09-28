#if !defined(AFX_NCSETDLG_H__CA9709AB_BCD0_4C3C_9083_94E66F755479__INCLUDED_)
#define AFX_NCSETDLG_H__CA9709AB_BCD0_4C3C_9083_94E66F755479__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NcSetDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNcSetDlg dialog

class CNcSetDlg : public CDialog
{
// Construction
public:
	CNcSetDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNcSetDlg)
	enum { IDD = IDD_NC_SET_DLG };
	double	m_fHoleDIncrement;
	double	m_fMKHoleD;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNcSetDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNcSetDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCSETDLG_H__CA9709AB_BCD0_4C3C_9083_94E66F755479__INCLUDED_)
