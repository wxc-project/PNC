#pragma once

#include "supergridctrl.h"
#include "SysPara.h"
// CFilterMkDlg 对话框

class CFilterMkDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFilterMkDlg)
public:
	CFilterMkDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFilterMkDlg();
	//
	void RefreshListCtrl();
	void UpdateSysParaItemToUI();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILTER_MK_DLG };
#endif
	CSuperGridCtrl m_listCtrl;
	//
	ATOM_LIST<FILTER_MK_PARA> m_arrParaItem;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnAddItem();
};
