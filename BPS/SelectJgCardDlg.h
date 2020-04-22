#ifndef __SELECT_JG_CARD_DLG_
#define __SELECT_JG_CARD_DLG_
/////////////////////////////////////////////////////////////////////////////
// CSelectJgCardDlg dialog
#include "resource.h"
#include "XhCharString.h"

class CSelectJgCardDlg : public CDialog
{
	// Construction
public:
	CWnd *m_pWnd;
	CSelectJgCardDlg(CWnd* pParent = NULL);   // standard constructor
	// Dialog Data
	//{{AFX_DATA(CProcBarDlg)
	enum { IDD = IDD_SELECT_JG_CARD_DLG };
	CString	m_sJgCardPath;
protected:
	CComboBox	m_xPathCmbBox;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnCbnSelchangeCombo1();
};

#endif