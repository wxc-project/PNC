#if !defined(AFX_INPUTANVALDLG_H__CF0843FF_AFB5_4E33_9BBB_D27A1E8E5C71__INCLUDED_)
#define AFX_INPUTANVALDLG_H__CF0843FF_AFB5_4E33_9BBB_D27A1E8E5C71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InputAnValDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInputAnIntegerValDlg dialog

class CInputAnIntegerValDlg : public CDialog
{
	// Construction
public:
	CInputAnIntegerValDlg(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CInputAnIntegerValDlg)
	enum { IDD = IDD_INPUT_AN_INTEGER_VAL_DLG };
	double		m_fVal;
	CString	m_sValTitle;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInputAnIntegerValDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInputAnIntegerValDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CInputAnStringValDlg dialog

class CInputAnStringValDlg : public CDialog
{
// Construction
public:
	CInputAnStringValDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInputAnStringValDlg)
	enum { IDD = IDD_INPUT_AN_STRING_VAL_DLG };
	CString	m_sItemValue;
	CString m_sCaption;
	//}}AFX_DATA
	int		m_nStrMaxLen;	//字符串最大长度

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInputAnStringValDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInputAnStringValDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPUTANVALDLG_H__CF0843FF_AFB5_4E33_9BBB_D27A1E8E5C71__INCLUDED_)
