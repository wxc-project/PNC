#pragma once
#include "resource.h"
#include "supergridctrl.h"
#include "BPSModel.h"
#include "ArrayList.h"
// COptimalSortDlg 对话框

class COptimalSortDlg : public CDialog
{
	DECLARE_DYNAMIC(COptimalSortDlg)
	CHashList<int> m_widthHashTbl;
	CHashList<int> m_thickHashTbl;
	ARRAY_LIST<CAngleProcessInfo> m_xJgList;
public:
	COptimalSortDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~COptimalSortDlg();
	//
	void UpdateJgInfoList();
	void RefeshListCtrl();
// 对话框数据
	enum { IDD = IDD_OPTIMAL_SORT_DLG };
	CSuperGridCtrl m_xListCtrl;
	BOOL m_bSelJg;
	BOOL m_bSelYg;
	BOOL m_bSelTube;
	BOOL m_bSelFlat;
	BOOL m_bSelJig;
	BOOL m_bSelGgs;
	BOOL m_bSelQ235;
	BOOL m_bSelQ345;
	BOOL m_bSelQ390;
	BOOL m_bSelQ420;
	BOOL m_bSelQ460;
	CString m_sWidth;
	CString m_sThick;
	CString m_sCardPath;
	int m_nRecord;
	//
	ATOM_LIST<SCOPE_STRU> m_xPrintScopyList;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateJgInfo();
	afx_msg void OnEnChangeEWidth();
	afx_msg void OnEnChangeEThick();
	afx_msg void OnBnClickedBtnJgCard();
public:
	afx_msg void OnBnClickedOk();
};
