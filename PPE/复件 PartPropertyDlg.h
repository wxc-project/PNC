#pragma once


// CPartPropertyDlg 对话框

class CPartPropertyDlg : public CDialog
{
	DECLARE_DYNCREATE(CPartPropertyDlg)
public:
	CPartPropertyDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPartPropertyDlg();

// 对话框数据
	enum { IDD = IDD_PART_PROPERTY_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
