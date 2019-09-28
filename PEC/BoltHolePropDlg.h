#if !defined(AFX_BOLTPROPDLG_H__49BCA49D_FD93_4ED1_A64E_5363F2D38AF5__INCLUDED_)
#define AFX_BOLTPROPDLG_H__49BCA49D_FD93_4ED1_A64E_5363F2D38AF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BoltPropDlg.h : header file
//
#if !defined(__TSA_)&&!defined(__TSA_FILE_)
/////////////////////////////////////////////////////////////////////////////
// CBoltPropDlg dialog
#include "Resource.h"
#include "ProcessPart.h"

class CBoltHolePropDlg : public CDialog
{
// Construction
public:
	f3dPoint ToUcsPoint(f3dPoint from_point,f3dPoint ls_norm);
	BOOL m_bCanModifyPos;
	CBoltHolePropDlg(CWnd* pParent = NULL);   // standard constructor
	CProcessPart *m_pWorkPart;
	UCS_STRU work_ucs;
	BOLT_INFO *m_pLs;
	DWORD m_dwRayNo;
	f3dPoint waist_vec;
public:
	void UpdateCfgPartNoString();

// Dialog Data
	//{{AFX_DATA(CBoltPropDlg)
	enum { IDD = IDD_BOLT_PROP_DLG };
	CString	m_sLsGuiGe;
	double	m_fHoleD;
	double	m_fPosX;
	double	m_fPosY;
	double	m_fPosZ;
	CString	m_hLs;
	double	m_fNormX;
	double	m_fNormY;
	double	m_fNormZ;
	CString	m_sRayNo;
	BOOL	m_bVirtualBolt;
	short	m_nWaistLen;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBoltPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBoltPropDlg)
	afx_msg void OnChangeCmbLsGuige();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeEY();
	afx_msg void OnBnCopyPos();
	afx_msg void OnBnCopyNorm();
	afx_msg void OnBnPasteNorm();
	afx_msg void OnBnPastePos();
	afx_msg void OnBnLsRayNo();
	afx_msg void OnKillfocusEX();
	afx_msg void OnModifyWaistVec();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSFEATDLG_H__49BCA49D_FD93_4ED1_A64E_5363F2D38AF5__INCLUDED_)
