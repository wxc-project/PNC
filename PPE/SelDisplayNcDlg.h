#pragma once


// CSelDisplayNcDlg 对话框

class CSelDisplayNcDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSelDisplayNcDlg)

public:
	CSelDisplayNcDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSelDisplayNcDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISPLAY_NC_DLG };
#endif
	int m_iCutType;
	int m_iProcessType;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
};
