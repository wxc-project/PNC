#pragma once

#include "SysPara.h"
// CHoleIncrementSetDlg 对话框

class CHoleIncrementSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CHoleIncrementSetDlg)
private:
	NC_INFO_PARA* pNcPare;
public:
	CHoleIncrementSetDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CHoleIncrementSetDlg();

	BYTE m_ciCurNcMode;
// 对话框数据
	enum { IDD = IDD_HOLE_INCREMENT_DLG };
	BOOL	m_arrIsCanUse[6];
	double  m_fM12Increment;
	double  m_fM16Increment;
	double  m_fM20Increment;
	double  m_fM24Increment;
	double  m_fCutIncrement;
	double  m_fProIncrement;
protected:
	void InitCtrlValue();
	void RefreshCtrlState();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnChkM12();
	afx_msg void OnChkM16();
	afx_msg void OnChkM20();
	afx_msg void OnChkM24();
	afx_msg void OnChkCutSH();
	afx_msg void OnChkProSH();
};
