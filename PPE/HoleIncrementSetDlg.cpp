// HoleIncrementSetDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "HoleIncrementSetDlg.h"
#include "afxdialogex.h"
#include "SysPara.h"


// CHoleIncrementSetDlg 对话框

IMPLEMENT_DYNAMIC(CHoleIncrementSetDlg, CDialog)

CHoleIncrementSetDlg::CHoleIncrementSetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHoleIncrementSetDlg::IDD, pParent)
{
	m_arrIsCanUse[0]=FALSE;
	m_arrIsCanUse[1]=FALSE;
	m_arrIsCanUse[2]=FALSE;
	m_arrIsCanUse[3]=FALSE;
	m_arrIsCanUse[4]=FALSE;
	//
	m_ciCurNcMode = 0;
	pNcPare = NULL;
}

CHoleIncrementSetDlg::~CHoleIncrementSetDlg()
{
}

void CHoleIncrementSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_M12, m_arrIsCanUse[0]);
	DDX_Check(pDX, IDC_CHK_M16, m_arrIsCanUse[1]);
	DDX_Check(pDX, IDC_CHK_M20, m_arrIsCanUse[2]);
	DDX_Check(pDX, IDC_CHK_M24, m_arrIsCanUse[3]);
	DDX_Check(pDX, IDC_CHK_CUT_SH, m_arrIsCanUse[4]);
	DDX_Check(pDX, IDC_CHK_PRO_SH, m_arrIsCanUse[5]);
	DDX_Text(pDX, IDC_E_M12, m_fM12Increment);
	DDX_Text(pDX, IDC_E_M16, m_fM16Increment);
	DDX_Text(pDX, IDC_E_M20, m_fM20Increment);
	DDX_Text(pDX, IDC_E_M24, m_fM24Increment);
	DDX_Text(pDX, IDC_E_CUT_SH, m_fCutIncrement);
	DDX_Text(pDX, IDC_E_PRO_SH, m_fProIncrement);
}


BEGIN_MESSAGE_MAP(CHoleIncrementSetDlg, CDialog)
	ON_BN_CLICKED(IDC_CHK_M12, OnChkM12)
	ON_BN_CLICKED(IDC_CHK_M16, OnChkM16)
	ON_BN_CLICKED(IDC_CHK_M20, OnChkM20)
	ON_BN_CLICKED(IDC_CHK_M24, OnChkM24)
	ON_BN_CLICKED(IDC_CHK_CUT_SH, OnChkCutSH)
	ON_BN_CLICKED(IDC_CHK_PRO_SH, OnChkProSH)
END_MESSAGE_MAP()


// CHoleIncrementSetDlg 消息处理程序
BOOL CHoleIncrementSetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_ciCurNcMode == CNCPart::FLAME_MODE)
		pNcPare = &g_sysPara.nc.m_xFlamePara;
	else if (m_ciCurNcMode == CNCPart::PLASMA_MODE)
		pNcPare = &g_sysPara.nc.m_xPlasmaPara;
	else if (m_ciCurNcMode == CNCPart::PUNCH_MODE)
		pNcPare = &g_sysPara.nc.m_xPunchPara;
	else if (m_ciCurNcMode == CNCPart::DRILL_MODE)
		pNcPare = &g_sysPara.nc.m_xDrillPara;
	else if (m_ciCurNcMode == CNCPart::LASER_MODE)
		pNcPare = &g_sysPara.nc.m_xLaserPara;
	else
		pNcPare = NULL;
	InitCtrlValue();
	RefreshCtrlState();
	UpdateData(FALSE);
	return TRUE;
}
void CHoleIncrementSetDlg::InitCtrlValue()
{
	if(pNcPare)
	{	//切割
		m_fM12Increment = pNcPare->m_xHoleIncrement.m_fM12;
		m_fM16Increment = pNcPare->m_xHoleIncrement.m_fM16;
		m_fM20Increment = pNcPare->m_xHoleIncrement.m_fM20;
		m_fM24Increment = pNcPare->m_xHoleIncrement.m_fM24;
		m_fProIncrement = pNcPare->m_xHoleIncrement.m_fProSH;
		m_fCutIncrement = pNcPare->m_xHoleIncrement.m_fCutSH;
	}
	else
	{
		double fDatum = g_sysPara.holeIncrement.m_fDatum;
		//M12
		m_fM12Increment = g_sysPara.holeIncrement.m_fM12;
		if (fabs(m_fM12Increment - fDatum) > EPS)
			m_arrIsCanUse[0] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_M12))->SetReadOnly(!m_arrIsCanUse[0]);
		//M16
		m_fM16Increment = g_sysPara.holeIncrement.m_fM16;
		if (fabs(m_fM16Increment - fDatum) > EPS)
			m_arrIsCanUse[1] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_M16))->SetReadOnly(!m_arrIsCanUse[1]);
		//M20
		m_fM20Increment = g_sysPara.holeIncrement.m_fM20;
		if (fabs(m_fM20Increment - fDatum) > EPS)
			m_arrIsCanUse[2] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_M20))->SetReadOnly(!m_arrIsCanUse[2]);
		//M24
		m_fM24Increment = g_sysPara.holeIncrement.m_fM24;
		if (fabs(m_fM24Increment - fDatum) > EPS)
			m_arrIsCanUse[3] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_M24))->SetReadOnly(!m_arrIsCanUse[3]);
		//SH
		m_fCutIncrement = g_sysPara.holeIncrement.m_fCutSH;
		if (fabs(m_fCutIncrement - fDatum) > EPS)
			m_arrIsCanUse[4] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_CUT_SH))->SetReadOnly(!m_arrIsCanUse[4]);
		//SH
		m_fProIncrement = g_sysPara.holeIncrement.m_fProSH;
		if (fabs(m_fProIncrement - fDatum) > EPS)
			m_arrIsCanUse[5] = TRUE;
		((CEdit*)GetDlgItem(IDC_E_PRO_SH))->SetReadOnly(!m_arrIsCanUse[5]);
	}
}
void CHoleIncrementSetDlg::RefreshCtrlState()
{
	if (m_ciCurNcMode == CNCPart::FLAME_MODE || m_ciCurNcMode==CNCPart::PLASMA_MODE)
	{	//切割
		m_arrIsCanUse[0] = FALSE;
		GetDlgItem(IDC_CHK_M12)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_M12)->EnableWindow(FALSE);
		m_arrIsCanUse[1] = FALSE;
		GetDlgItem(IDC_CHK_M16)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_M16)->EnableWindow(FALSE);
		m_arrIsCanUse[2] = FALSE;
		GetDlgItem(IDC_CHK_M20)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_M20)->EnableWindow(FALSE);
		m_arrIsCanUse[3] = FALSE;
		GetDlgItem(IDC_CHK_M24)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_M24)->EnableWindow(FALSE);
		m_arrIsCanUse[4] = TRUE;
		GetDlgItem(IDC_CHK_CUT_SH)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_CUT_SH)->EnableWindow(TRUE);
		m_arrIsCanUse[5] = FALSE;
		GetDlgItem(IDC_CHK_PRO_SH)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_PRO_SH)->EnableWindow(FALSE);
	}
	else if (m_ciCurNcMode == CNCPart::PUNCH_MODE || m_ciCurNcMode == CNCPart::DRILL_MODE)
	{	//板床
		GetDlgItem(IDC_CHK_M12)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M12)->EnableWindow(TRUE);
		m_arrIsCanUse[0] = TRUE;
		GetDlgItem(IDC_CHK_M16)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M16)->EnableWindow(TRUE);
		m_arrIsCanUse[1] = TRUE;
		GetDlgItem(IDC_CHK_M20)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M20)->EnableWindow(TRUE);
		m_arrIsCanUse[2] = TRUE;
		GetDlgItem(IDC_CHK_M24)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M24)->EnableWindow(TRUE);
		m_arrIsCanUse[3] = TRUE;
		//保留切割大号孔
		BOOL bEnable = FALSE;
		if (m_ciCurNcMode == CNCPart::PUNCH_MODE&&g_sysPara.nc.m_xPunchPara.m_bReserveBigSH)
			bEnable = TRUE;
		if (m_ciCurNcMode == CNCPart::DRILL_MODE&&g_sysPara.nc.m_xDrillPara.m_bReserveBigSH)
			bEnable = TRUE;
		m_arrIsCanUse[4] = bEnable;
		GetDlgItem(IDC_CHK_CUT_SH)->EnableWindow(bEnable);
		GetDlgItem(IDC_E_CUT_SH)->EnableWindow(bEnable);
		//降级处理小号孔
		bEnable = TRUE;
		if (m_ciCurNcMode == CNCPart::PUNCH_MODE&&g_sysPara.nc.m_xPunchPara.m_bReduceSmallSH)
			bEnable = FALSE;
		if (m_ciCurNcMode == CNCPart::DRILL_MODE&&g_sysPara.nc.m_xDrillPara.m_bReduceSmallSH)
			bEnable = FALSE;
		m_arrIsCanUse[5] = bEnable;
		GetDlgItem(IDC_CHK_PRO_SH)->EnableWindow(bEnable);
		GetDlgItem(IDC_E_PRO_SH)->EnableWindow(bEnable);
	}
	else
	{
		if (m_ciCurNcMode == CNCPart::LASER_MODE)
		{
			for (int i = 0; i < 6; i++)
				m_arrIsCanUse[i] = TRUE;
		}
		GetDlgItem(IDC_CHK_M12)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M12)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHK_M16)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M16)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHK_M20)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M20)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHK_M24)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_M24)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHK_PRO_SH)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_PRO_SH)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHK_CUT_SH)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_CUT_SH)->EnableWindow(TRUE);
	}
}
void CHoleIncrementSetDlg::OnChkM12()
{
	m_arrIsCanUse[0]=!m_arrIsCanUse[0];
	((CEdit*)GetDlgItem(IDC_E_M12))->SetReadOnly(!m_arrIsCanUse[0]);
}
void CHoleIncrementSetDlg::OnChkM16()
{
	m_arrIsCanUse[1]=!m_arrIsCanUse[1];
	((CEdit*)GetDlgItem(IDC_E_M16))->SetReadOnly(!m_arrIsCanUse[1]);
}
void CHoleIncrementSetDlg::OnChkM20()
{
	m_arrIsCanUse[2]=!m_arrIsCanUse[2];
	((CEdit*)GetDlgItem(IDC_E_M20))->SetReadOnly(!m_arrIsCanUse[2]);
}
void CHoleIncrementSetDlg::OnChkM24()
{
	m_arrIsCanUse[3]=!m_arrIsCanUse[3];
	((CEdit*)GetDlgItem(IDC_E_M24))->SetReadOnly(!m_arrIsCanUse[3]);
}
void CHoleIncrementSetDlg::OnChkCutSH()
{
	m_arrIsCanUse[4]=!m_arrIsCanUse[4];
	((CEdit*)GetDlgItem(IDC_E_CUT_SH))->SetReadOnly(!m_arrIsCanUse[4]);
}
void CHoleIncrementSetDlg::OnChkProSH()
{
	m_arrIsCanUse[5] = !m_arrIsCanUse[5];
	((CEdit*)GetDlgItem(IDC_E_PRO_SH))->SetReadOnly(!m_arrIsCanUse[5]);
}
void CHoleIncrementSetDlg::OnOK()
{
	UpdateData();
	if (pNcPare)
	{
		pNcPare->m_xHoleIncrement.m_fM12 = m_fM12Increment;
		pNcPare->m_xHoleIncrement.m_fM16 = m_fM16Increment;
		pNcPare->m_xHoleIncrement.m_fM20 = m_fM20Increment;
		pNcPare->m_xHoleIncrement.m_fM24 = m_fM24Increment;
		pNcPare->m_xHoleIncrement.m_fProSH = m_fProIncrement;
		pNcPare->m_xHoleIncrement.m_fCutSH = m_fCutIncrement;
	}
	else
	{
		double fDatum = g_sysPara.holeIncrement.m_fDatum;
		if (m_arrIsCanUse[0])
			g_sysPara.holeIncrement.m_fM12 = m_fM12Increment;
		else
			g_sysPara.holeIncrement.m_fM12 = fDatum;
		if (m_arrIsCanUse[1])
			g_sysPara.holeIncrement.m_fM16 = m_fM16Increment;
		else
			g_sysPara.holeIncrement.m_fM16 = fDatum;
		if (m_arrIsCanUse[2])
			g_sysPara.holeIncrement.m_fM20 = m_fM20Increment;
		else
			g_sysPara.holeIncrement.m_fM20 = fDatum;
		if (m_arrIsCanUse[3])
			g_sysPara.holeIncrement.m_fM24 = m_fM24Increment;
		else
			g_sysPara.holeIncrement.m_fM24 = fDatum;
		if (m_arrIsCanUse[4])
			g_sysPara.holeIncrement.m_fCutSH = m_fCutIncrement;
		else
			g_sysPara.holeIncrement.m_fCutSH = fDatum;
		if (m_arrIsCanUse[5])
			g_sysPara.holeIncrement.m_fProSH = m_fCutIncrement;
		else
			g_sysPara.holeIncrement.m_fProSH = fDatum;
	}
	return CDialog::OnOK();
}
