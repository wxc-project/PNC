#pragma once

#include "SysPara.h"
// CHoleIncrementSetDlg �Ի���

class CHoleIncrementSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CHoleIncrementSetDlg)
private:
	NC_INFO_PARA* pNcPare;
public:
	CHoleIncrementSetDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CHoleIncrementSetDlg();

	BYTE m_ciCurNcMode;
// �Ի�������
	enum { IDD = IDD_HOLE_INCREMENT_DLG };
	BOOL	m_arrIsCanUse[7];
	double  m_fM12Increment;
	double  m_fM16Increment;
	double  m_fM20Increment;
	double  m_fM24Increment;
	double  m_fCutIncrement;
	double  m_fProIncrement;
	double  m_fWaistIncrement;
protected:
	void InitCtrlValue();
	void RefreshCtrlState();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnChkM12();
	afx_msg void OnChkM16();
	afx_msg void OnChkM20();
	afx_msg void OnChkM24();
	afx_msg void OnChkCutSH();
	afx_msg void OnChkProSH();
	afx_msg void OnChkWaist();
};
