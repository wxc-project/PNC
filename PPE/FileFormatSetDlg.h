#pragma once

#include <vector>
using std::vector;
// CFileFormatSetDlg 对话框

class CFileFormatSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileFormatSetDlg)

public:
	CFileFormatSetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFileFormatSetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILE_FORMAT_DLG };
#endif
	vector<CXhChar16> m_sSplitters;
	vector<CXhChar16> m_sKeyArr;
	BOOL m_bTaType;
	BOOL m_bPartNo;
	BOOL m_bPartMat;
	BOOL m_bPartThick;
	BOOL m_bSingleNum;
	BOOL m_bProcessNum;
	//
	void UpdateEditLine();
	void InitCheckBoxByKeyArr();
	void UpdateCheckBoxEnableState();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnChkTaType();
	afx_msg void OnChkPartNo();
	afx_msg void OnChkPartMat();
	afx_msg void OnChkPartThick();
	afx_msg void OnChkSingleNum();
	afx_msg void OnChkProcessNum();
	afx_msg void OnBtnReset();
	afx_msg void OnEnChangeEFormatText();
};
