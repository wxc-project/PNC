#pragma once
#include "resource.h"
#include "XhTreeCtrl.h"
#include "f_ent_list.h"
#include "XhCharString.h"
#include "PNCModel.h"

#ifndef __UBOM_ONLY_

struct TREEITEM_INFO {
	static const BYTE	PROJECT_ITEM = 1;
	static const BYTE 	DXF_FOLDER = 2;
	static const BYTE 	DXF_FILE_ITEM = 3;
	int itemType;
	DWORD dwRefData;
	//
	TREEITEM_INFO() { ; }
	TREEITEM_INFO(int type, DWORD dw) { itemType = type; dwRefData = dw; }
};

// CExplodeTxtDlg 对话框
#ifdef __SUPPORT_DOCK_UI_
class CExplodeTxtDlg : public CAcUiDialog
#else
class CExplodeTxtDlg : public CDialog
#endif
{
	DECLARE_DYNCREATE(CExplodeTxtDlg)
private:
	HTREEITEM m_hProjectItem;
public:
	CExplodeTxtDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CExplodeTxtDlg();
	//
	BOOL CreateDlg();
	void ContextMenu(CWnd *pWnd, CPoint point);
	void RefreshFolderItem(CDxfFolder* pDxfFolder);
// 对话框数据
	enum { IDD = IDD_EXPLODE_TXT_DLG };
	CXhTreeCtrl m_treeCtrl;
	//
	ATOM_LIST<TREEITEM_INFO> itemInfoList;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PreSubclassWindow();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRclickTreeCtrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
	afx_msg void OnImportItem();
	afx_msg void OnExplodeItem();
	afx_msg void OnSaveItem();
};
#endif
