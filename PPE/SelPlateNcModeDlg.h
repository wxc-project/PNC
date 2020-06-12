#pragma once


// CSelPlateNcModeDlg 对话框

class CSelPlateNcModeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSelPlateNcModeDlg)

public:
	CSelPlateNcModeDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSelPlateNcModeDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SEL_PLATE_NC_DLG };
#endif
	BOOL m_bSelFlame;
	BOOL m_bSelPlasma;
	BOOL m_bSelPunch;
	BOOL m_bSelDrill;
	BOOL m_bSelLaser;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedChkFlame();
	afx_msg void OnBnClickedChkPlasma();
	afx_msg void OnBnClickedChkPunch();
	afx_msg void OnBnClickedChkDrill();
	afx_msg void OnBnClickedChkLaser();
};
