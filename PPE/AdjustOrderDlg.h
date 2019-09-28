#pragma once
#include "ProcessPart.h"

// CAdjustOrderDlg 对话框

class CAdjustOrderDlg : public CDialog
{
	DECLARE_DYNAMIC(CAdjustOrderDlg)
public:
	int m_nSubListCtrlBtmMargin;
	int m_nBtBtmMargin;
	CProcessPlate* pPlate;
	ATOM_LIST<DWORD> keyList;
public:
	CAdjustOrderDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAdjustOrderDlg();

// 对话框数据
	enum { IDD = IDD_ADJUST_ORDER_DLG };
	CString m_sPartNo;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg void OnBnAddLsIndex();
	afx_msg void OnBnDelLsIndex();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
};
