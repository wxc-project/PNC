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
	void SelectPart(int iCurSel);
	CDamBoardManager m_xDamBoardManager;
	CBitmap m_bmpLeftRotate,m_bmpRightRotate,m_bmpMirror,m_bmpMove,m_bmpMoveMk;
public:
	CPartListDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPartListDlg();
	CXhListCtrl m_partList;
// 对话框数据
	enum { IDD = IDD_PART_LIST_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
public:
	BOOL UpdatePartList();
	void ProcessKeyDown(WORD wVKey);
#if defined (_ARX_2007) && defined (__SUPPORT_DOCK_UI_) && !defined _UNICODE
	virtual BOOL    FindContextHelpFullPath(LPCWSTR fileName, CString& fullPath) { return FALSE; }
	virtual LPCTSTR AppRootKey() { return ""; }
#endif
	BOOL CreateDlg();
	virtual BOOL OnInitDialog();
	afx_msg void OnNMClickPartList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeydownListPart(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnSendToPpe();
	afx_msg void OnBnClickedBtnExportDxf();
	afx_msg void OnBnClickedBtnAnticlockwiseRotation();
	afx_msg void OnBnClickedBtnClockwiseRotation();
	afx_msg void OnBnClickedBtnMirror();
	afx_msg void OnBnClickedBtnMove();
	afx_msg void OnBnClickedBtnMoveMkRect();
	afx_msg void OnBnClickedChkEditMk();
	virtual void PreSubclassWindow();
	BOOL m_bEditMK;
};
#endif