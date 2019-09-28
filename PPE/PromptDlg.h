#if !defined(AFX_PROMPTDLG_H__8EC78781_1547_11D5_8CF3_0000B498B2A9__INCLUDED_)
#define AFX_PROMPTDLG_H__8EC78781_1547_11D5_8CF3_0000B498B2A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PromptDlg.h : header file
//
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg dialog

class CPromptDlg : public CDialog
{
	CWnd* m_pWnd;
	int  m_iType;	//0、显示文字   1、显示图片
	CBitmap m_bBitMap;
	CBrush m_bBrush;
// Dialog Data
	//{{AFX_DATA(CPromptDlg)
	enum { IDD = IDD_PROMPT_DLG };
	CString	m_sPromptMsg;
	//}}AFX_DATA
public:
	BOOL m_bPickingObjects;
	BOOL m_bForceExitCommand;	//强行退出当前执行的命令
// Construction
public:
	BOOL Destroy();
	void SetMsg(const char* msg);
	CPromptDlg(CWnd* pWnd);   // standard constructor
	BOOL Create();
	void SetMsg(long bitMapId);
	BOOL StartPickObjects(BOOL bCaptureLButtonUpMsg=FALSE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPromptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPromptDlg)
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
};
extern CPromptDlg *g_pPromptMsg;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROMPTDLG_H__8EC78781_1547_11D5_8CF3_0000B498B2A9__INCLUDED_)
