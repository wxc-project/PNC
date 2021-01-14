#pragma once
#include "resource.h"
#include "supergridctrl.h"
#include "XhTreeCtrl.h"
#include "UbomModel.h"

#ifdef __UBOM_ONLY_
class CWndShowLife
{
	CWnd *m_pWnd;
public:
	CWndShowLife(CWnd *pWnd) {
		m_pWnd = pWnd;
		if (m_pWnd)
			m_pWnd->ShowWindow(SW_HIDE);
	}
	~CWndShowLife() {
		if (m_pWnd)
			m_pWnd->ShowWindow(SW_SHOW);
	}
};

// CRevisionDlg 对话框
enum TREEITEM_TYPE{
	PROJECT_GROUP,		//工程塔形组
	PROJECT_ITEM,		
	BOM_GROUP,			//料单组
	BOM_ITEM,
	ANGLE_GROUP,		
	ANGLE_DWG_ITEM,
	PLATE_GROUP,
	PLATE_DWG_ITEM,
	PART_GROUP,
	PART_DWG_ITEM,
};
struct TREEITEM_INFO{
	TREEITEM_INFO(){;}
	TREEITEM_INFO(long type,DWORD dw){itemType=type;dwRefData=dw;}
	long itemType;
	DWORD dwRefData;
};
#ifdef __SUPPORT_DOCK_UI_
class CRevisionDlg : public CAcUiDialog
#else
class CRevisionDlg : public CDialog
#endif
{
	DECLARE_DYNCREATE(CRevisionDlg)
	static int m_nWidth;
	int m_nRightMargin;
	int m_nBtmMargin;
	int m_iCompareMode;
public:
	ATOM_LIST<TREEITEM_INFO>itemInfoList;
	static CXhChar500 g_sPrintDwgFileName;
public:
	CRevisionDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CRevisionDlg();
	// 对话框数据
	enum { IDD = IDD_REVISION_DLG };
	//控件变量
	CSuperGridCtrl m_xListReport;
	CXhTreeCtrl m_treeCtrl;
	CString m_sCurFile;		// 当前显示文件
	CString m_sRecordNum;	// 显示记录数
	CString m_sSearchText;
	CString m_sLegErr;		// 长度误差
	BOOL m_bQuality;		// 校审质量等级
	BOOL m_bMaterialH;		// 校审材质H
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PreSubclassWindow();
	//
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedTreeContrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBeginlabeleditTreeContrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeleditTreeContrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickTreeContrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNewItem();
	afx_msg void OnLoadProjFile();
	afx_msg void OnExportProjFile();
	afx_msg void OnExportXLSFile();
	afx_msg void OnImportBomFile();
	afx_msg void OnImportDwgFile();
	afx_msg void OnCompareData();
	afx_msg void OnExportCompResult();
	afx_msg void OnRefreshPartNum();
	afx_msg void OnRefreshSingleNum();
	afx_msg void OnRefreshSpec();
	afx_msg void OnRefreshMat();
	afx_msg void OnFillDwgData();
	afx_msg void OnModifyErpFile();
	afx_msg void OnDeleteItem();
	afx_msg void OnSearchPart();
	afx_msg void OnBatchPrintPart();
	afx_msg void OnBatchRetrievedPlates();
	afx_msg void OnBatchRetrievedAngles();
	afx_msg void OnEmptyRetrievedPlates();
	afx_msg void OnEmptyRetrievedAngles();
	afx_msg void OnBnClickedChkGrade();
	afx_msg void OnEnChangeELenErr();
	afx_msg void OnBnClickedChkMatH();
	afx_msg void OnBnClickedSettings();
	afx_msg void OnCbnSelchangeCmbClient();
	afx_msg void OnCbnSelchangeCmbCfg();
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
public:
	CXhTreeCtrl *GetTreeCtrl(){return &m_treeCtrl;}
	void ContextMenu(CWnd *pWnd, CPoint point);
	void RefreshTreeCtrl();
	void RefreshProjectItem(HTREEITEM hItem,CProjectTowerType* pProject);
	void RefreshListCtrl(HTREEITEM hItem,BOOL bCompared=FALSE);
	CProjectTowerType* GetProject(HTREEITEM hItem);
	HTREEITEM FindTreeItem(HTREEITEM hItem,CXhChar100 sName);
	//
	BOOL CreateDlg();
	void InitRevisionDlg();
};
#endif
