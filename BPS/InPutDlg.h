#pragma once
#ifndef __IN_PUT_DLG_
#define __IN_PUT_DLG_

// CInputDlg 对话框
#include "resource.h"

class CInputDlg : public CDialog
{
	DECLARE_DYNAMIC(CInputDlg)

public:
	CInputDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInputDlg();
	// 对话框数据
	enum { IDD = IDD_INPUT_DLG };
	CString m_sInputValue;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEInput();
	afx_msg void OnBtnClickedOk();
};

#endif