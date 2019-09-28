#if !defined(AFX_NEWPARTFILEDLG_H__858B67EC_015C_451F_9387_35E7573B9178__INCLUDED_)
#define AFX_NEWPARTFILEDLG_H__858B67EC_015C_451F_9387_35E7573B9178__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewPartFileDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewPartFileDlg dialog

class CNewPartFileDlg : public CDialog
{
// Construction
public:
	CNewPartFileDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewPartFileDlg)
	enum { IDD = IDD_NEW_PART_FILE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewPartFileDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewPartFileDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_sPartNo;
	CString m_sLen;
	CString m_sSpec;
	CString m_sMaterial;
	BOOL m_bImprotNcFile;
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWPARTFILEDLG_H__858B67EC_015C_451F_9387_35E7573B9178__INCLUDED_)
