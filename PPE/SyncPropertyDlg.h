#pragma once


// CSyncPropertyDlg 对话框
#include "XhListCtrl.h"
#include "ProcessPart.h"
class CSyncPropertyDlg : public CDialog
{
	DECLARE_DYNAMIC(CSyncPropertyDlg)

public:
	CSyncPropertyDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSyncPropertyDlg();
	void RefeshSyncFlag();
// 对话框数据
	enum { IDD = IDD_SYNC_PROPERTY_DLG };
	CProcessPart* m_pProcessPart;
	CXhListCtrl m_xPropertyList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRClickListSyncProp(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSetSyncItem();
	afx_msg void OnRevokeSyncItem();
};
