#pragma once
#include "XhTreeCtrl.h"
// CPartTreeDlg 对话框
enum PART_TREE_IMG{
	IMG_ANGLE_SET,
	IMG_ANGLE,
	IMG_ANGLE_SEL,
	IMG_PLATE_SET,
	IMG_PLATE,
	IMG_PLATE_SEL,
};
struct TREEITEM_INFO{
	TREEITEM_INFO(){;}
	TREEITEM_INFO(int type,DWORD dw){itemType=type;dwRefData=dw;}
	DWORD dwRefData;
	int itemType;
	//
	static const int INFO_PLATESET	= 1;
	static const int INFO_PLATE		= 2;
	static const int INFO_ANGLESET	= 3;
	static const int INFO_ANGLE		= 4;
	static const int INFO_PLATE_GROUP = 5;
};
class CPartTreeDlg : public CDialogEx
{
	DECLARE_DYNCREATE(CPartTreeDlg)
	CImageList m_xModelImages;
	ATOM_LIST<TREEITEM_INFO>itemInfoList;
	HTREEITEM m_hAngleParentItem,m_hPlateParentItem;
	HTREEITEM InsertTreeItem(CTreeCtrl &treeCtrl,const char* sText,int nImage,int nSelectedImage,
							 HTREEITEM hParent,int nNodeType,void *pData=NULL);
public:
	CPartTreeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPartTreeDlg();
// 对话框数据
	enum { IDD = IDD_PART_TREE_DLG };
	CXhTreeCtrl m_treeCtrl;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRclickTreeCtrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteItem();
	afx_msg void OnSmartSortBolts1();
	afx_msg void OnSmartSortBolts2();
	afx_msg void OnSmartSortBolts3();
	afx_msg void OnAdjustHoleOrder();
	afx_msg void OnSortByPartNo();
	afx_msg void OnSortByThick();
	afx_msg void OnSortByMaterial();
	afx_msg void OnGroupByThickMaterial();
	afx_msg void OnTvnKeydownTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
public:
	CTreeCtrl *GetTreeCtrl() { return &m_treeCtrl; }
	void InitTreeView(const char* cur_part_no=NULL);
	void InsertTreeItem(char* sText,CProcessPart *pPart);
	void ContextMenu(CWnd *pWnd, CPoint point);
	void JumpSelectItem(BYTE head0_prev1_next2_tail3);
	void UpdateTreeItemText(HTREEITEM hItem=NULL);
	void UpdatePlateTreeItem(int sortByPN0_TK1_MAT2=0);
	void UpdateAngleTreeItem(int sortByPN0_SPEC1_MAT2=0);
	void CancelSelTreeItem();
};
