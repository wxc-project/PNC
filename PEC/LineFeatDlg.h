#if !defined(AFX_LINEFEATDLG_H__25B40534_B26E_4636_9BCF_3B8BA9DC46DA__INCLUDED_)
#define AFX_LINEFEATDLG_H__25B40534_B26E_4636_9BCF_3B8BA9DC46DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LineFeatDlg.h : header file
//
#include "Resource.h"
/////////////////////////////////////////////////////////////////////////////
// CLineFeatDlg dialog

class CLineFeatDlg : public CDialog
{
// Construction
public:
	int line_type;
	CLineFeatDlg(CWnd* pParent = NULL);   // standard constructor
	double m_fHuoQuAngle,m_fThick;
	f3dArcLine *m_pArcLine;
// Dialog Data
	//{{AFX_DATA(CLineFeatDlg)
	enum { IDD = IDD_LINE_FEATURE_DLG };
	double	m_fEndX;
	double	m_fEndY;
	double	m_fEndZ;
	double	m_fLength;
	double	m_fStartX;
	double	m_fStartY;
	double	m_fStartZ;
	CString	m_sLineType;
	int		m_iEdgeType;
	double	m_fDeformCoef;
	double	m_fHuoQuR;
	int		m_iSectorAngleStyle;
	int		m_iRotateStyle;
	double	m_fArcAngleOrR;
	double	m_fCenterX;
	double	m_fCenterY;
	double	m_fCenterZ;
	double	m_fColAxisX;
	double	m_fColAxisY;
	double	m_fColAxisZ;
	BOOL	m_bWeldEdge;
	BOOL	m_bRollEdge;
	double	m_fLocalPointY;
	int		m_iLocalPointVec;
	short	m_nManuSpace;
	//}}AFX_DATA
	short	m_nRollEdgeOffsetDist;	//æÌ±ﬂÕ‚“∆∂Ø¡ø

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLineFeatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLineFeatDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEDeformPara();
	afx_msg void OnSelchangeCmbEdgeType();
	afx_msg void OnRdoSectorAngleStyle();
	afx_msg void OnBnPasteCenter();
	afx_msg void OnBnPasteColAxis();
	afx_msg void OnChkWeldEdge();
	afx_msg void OnChkRollEdge();
	afx_msg void OnSelchangeLocalPointVec();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINEFEATDLG_H__25B40534_B26E_4636_9BCF_3B8BA9DC46DA__INCLUDED_)
