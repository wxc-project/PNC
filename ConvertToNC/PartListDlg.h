#pragma once

#include "resource.h"
#include "XhListCtrl.h"
#include "DrawDamBoard.h"

#ifndef __UBOM_ONLY_
// CPartListDlg 对话框
#ifdef __SUPPORT_DOCK_UI_
class CPartListDlg : public CAcUiDialog
#else
class CPartListDlg : public CDialog
#endif
{
	DECLARE_DYNCREATE(CPartListDlg)
	
	CDamBoardManager m_xDamBoardManager;
	CBitmap m_bmpLeftRotate,m_bmpRightRotate,m_bmpMirror,m_bmpMove,m_bmpMoveMk;
	static int m_nDlgWidth;
public:
	CPartListDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPartListDlg();
	
// 对话框数据
	enum { IDD = IDD_PART_LIST_DLG };
	BOOL m_bEditMK;
	CString m_sNote;
	CXhListCtrl m_partList;
public:
	BOOL IsValidDoc(CString& sFileName);
	BOOL CreateDlg();
	BOOL UpdatePartList();
	void ClearPartList();
	void RefreshCtrlState();
	void SelectPart(int iCurSel, BOOL bClone = TRUE);
	void RelayoutWnd();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
#if defined (_ARX_2007) && defined (__SUPPORT_DOCK_UI_) && !defined _UNICODE
	virtual BOOL    FindContextHelpFullPath(LPCWSTR fileName, CString& fullPath) { return FALSE; }
	virtual LPCTSTR AppRootKey() { return ""; }
#endif
	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnClose();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMClickPartList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkPartList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickPartList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeydownListPart(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnExtract();
	afx_msg void OnBnClickedSettings();
	afx_msg void OnBnClickedBtnSendToPpe();
	afx_msg void OnBnClickedBtnExportDxf();
	afx_msg void OnBnClickedBtnAnticlockwiseRotation();
	afx_msg void OnBnClickedBtnClockwiseRotation();
	afx_msg void OnBnClickedBtnMirror();
	afx_msg void OnBnClickedBtnMove();
	afx_msg void OnBnClickedBtnMoveMkRect();
	afx_msg void OnBnClickedChkEditMk();
	afx_msg void OnRetrievedPlate();
};
#endif