// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__252BAD9A_3A9D_48C3_8D7D_620923E1A68F__INCLUDED_)
#define AFX_MAINFRM_H__252BAD9A_3A9D_48C3_8D7D_620923E1A68F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PartPropertyDlg.h"
#include "PartTreeDlg.h"
#include "PPEView.h"
#include "PPEToolBar.h"
#include "DialogPanel.h"
#include "MouseStatusBar.h"

class CMainFrame : public CFrameWndEx
{
	DECLARE_DYNCREATE(CMainFrame)
	CSplitterWnd m_wndSplitter;
	BOOL CreateDockingWindows();
protected:  // control bar embedded members
	CMouseStatusBar  m_wndStatusBar;
	CPPEToolBar		m_wndToolBar;
	CPPEToolBar		m_wndToolBarPlate;
	CPPEToolBar		m_wndToolBarFile;
	CPPEToolBar		m_wndToolBarEdit;
	CPPEToolBar		m_wndToolBarPBJ;
	CPPEToolBar		m_wndToolBarCutPlate;
	CPPEToolBar		m_wndToolBarToolKit;
	CMFCMenuBar		m_wndMenuBar;
	CDialogPanel	m_partPropertyView;
	CDialogPanel	m_partTreeView;
	CMFCToolBarImages	m_userImages;
	//
	CMFCToolBarComboBoxButton  *m_comboBtn;
public:
	CPartPropertyDlg* GetPartPropertyPage(){return (CPartPropertyDlg*)m_partPropertyView.GetDlgPtr();}
	CPartTreeDlg* GetPartTreePage(){return (CPartTreeDlg*)m_partTreeView.GetDlgPtr();}
	void CloseProcess();	//¹Ø±Õ³ÌÐò
	void SetPaneText(int nIndex, LPCTSTR lpszNewText, BOOL bUpdate = TRUE);
	void ModifyDockpageStatus(CRuntimeClass *pRuntimeClass, BOOL bShow);
public:
	CMainFrame();
	virtual ~CMainFrame();
	//
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToolbarReset(WPARAM, LPARAM);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnSelDisplayNC();
	afx_msg void OnComboSelChangeClick();
	afx_msg void OnPrevPart();
	afx_msg void OnRotateAntiClockwise();
	afx_msg void OnNextPart();
	afx_msg void OnRotateClockwise();
	afx_msg void OnOverturnPlate();
	afx_msg void OnFirstPart();
	afx_msg void OnFinalPart();
	afx_msg void OnClose();
	afx_msg void OnDrawModeNc();
	afx_msg void OnDrawModeEdit();
	afx_msg void OnDrawModeProcesscard();
	afx_msg void OnUpdateDrawModeNc(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDrawModeEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDrawModeProcesscard(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRotateAntiClockwise(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRotateClockwise(CCmdUI *pCmdUI);
	afx_msg void OnUpdateOverturnPlate(CCmdUI *pCmdUI);
	afx_msg void OnInsertVertex();
	afx_msg void OnUpdateInsertVertex(CCmdUI *pCmdUI);
	afx_msg void OnNewLs();
	afx_msg void OnUpdateNewLs(CCmdUI *pCmdUI);
	afx_msg void OnFeatureProp();
	afx_msg void OnUpdateFeatureProp(CCmdUI *pCmdUI);
	afx_msg void OnSpecWcsOrgin();
	afx_msg void OnUpdateSpecWcsOrgin(CCmdUI *pCmdUI);
	afx_msg void OnSpecAxisXVertex();
	afx_msg void OnUpdateSpecAxisXVertex(CCmdUI *pCmdUI);
	afx_msg void OnAssembleCs();
	afx_msg void OnWorkCs();
	afx_msg void OnDelPartFeature();
	afx_msg void OnUpdateDelPartFeature(CCmdUI *pCmdUI);
	afx_msg void OnCalPlateProfile();
	afx_msg void OnUpdateCalPlateProfile(CCmdUI *pCmdUI);
	afx_msg void OnGlAllZoom();
	afx_msg void OnViewPropList();
	afx_msg void OnUpdateViewPropList(CCmdUI *pCmdUI);
	afx_msg void OnViewPartList();
	afx_msg void OnUpdateViewPartList(CCmdUI *pCmdUI);
	afx_msg void OnKaiHeJG();
	afx_msg void OnUpdateKaiHeJG(CCmdUI *pCmdUI);
	afx_msg void OnMeasureDist();
	afx_msg void OnAmendHoleIncrement();
	afx_msg void OnUpdateAmendHoleIncrement(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__252BAD9A_3A9D_48C3_8D7D_620923E1A68F__INCLUDED_)
