
// PNCDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "CADExecutePath.h"

// CPNCDlg 对话框
class CPNCDlg : public CDialogEx
{
// 构造
public:
	CPNCDlg(CWnd* pParent = NULL);	// 标准构造函数	

// 对话框数据
	enum { IDD = IDD_PNC_DIALOG };
	int m_nRightMargin;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	virtual BOOL OnInitDialog();
	BOOL GetCadPath(char* cad_path);
	// 实现
public:
	HICON m_hIcon;
	CComboBox m_xPathCmbBox;
	CString m_sPath;
	ARRAY_LIST<CAD_PATH> m_xCadPathArr;
	CString m_sRxFile;
	// 生成的消息映射函数
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBtnRunPPE();
	afx_msg void OnBnClickedBtnAbout();
};
