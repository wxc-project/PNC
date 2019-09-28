#pragma once

#include "resource.h"
#include "XhListCtrl.h"
// CFilterLayerDlg 对话框

class CFilterLayerDlg : public CDialog
{
	DECLARE_DYNAMIC(CFilterLayerDlg)

public:
	CFilterLayerDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFilterLayerDlg();
	void ContextMenu(CWnd* pWnd, CPoint point);
	void RefreshListCtrl();
// 对话框数据
	enum { IDD = IDD_FILTER_LAYER_DLG };
	CXhListCtrl m_xListCtrl;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
	afx_msg void OnClose();
	afx_msg void OnMarkFilter();
	afx_msg void OnCancelFilter();
	afx_msg void OnNMRClickListCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListCtrl(NMHDR *pNMHDR, LRESULT *pResult);
};
