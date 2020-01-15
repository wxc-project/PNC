#ifndef __PROC_BAR_DLG_H_
#define __PROC_BAR_DLG_H_
/////////////////////////////////////////////////////////////////////////////
// CProcBarDlg dialog
#include "resource.h"
class CProcBarDlg : public CDialog
{
// Construction
public:
	void Refresh(int proc);
	CWnd *m_pWnd;
	CProcBarDlg(CWnd* pParent = NULL);   // standard constructor
	BOOL Create();
	void SetTitle(CString ss);

// Dialog Data
	//{{AFX_DATA(CProcBarDlg)
	enum { IDD = IDD_PROC_BAR_DLG };
	CProgressCtrl	m_ctrlProcBar;
	CString	m_sProgress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProcBarDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProcBarDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif