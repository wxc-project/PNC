#pragma once
#include "resource.h"

// CAddBoltBlock 对话框

class CAddBoltBlockDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddBoltBlockDlg)

public:
	CAddBoltBlockDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAddBoltBlockDlg();

// 对话框数据
	enum { IDD = IDD_ADD_BOLT_BLOCK_DLG };
	CString m_sBlockName;
	CString m_sBoltD;
	CString m_sHoleD;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
