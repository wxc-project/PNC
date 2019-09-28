#pragma once

#include "Resource.h"
// CInputMKRectDlg 对话框

class CInputMKRectDlg : public CDialog
{
	DECLARE_DYNAMIC(CInputMKRectDlg)

public:
	CInputMKRectDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInputMKRectDlg();

// 对话框数据
	enum { IDD = IDD_INPUT_MK_RECT_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	int m_nRectL;
	int m_nRectW;
	double m_fTextH;
};
