// SelPlateNcModeDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "SelPlateNcModeDlg.h"
#include "afxdialogex.h"
#include "PPEModel.h"
#include "SysPara.h"

// CSelPlateNcModeDlg 对话框

IMPLEMENT_DYNAMIC(CSelPlateNcModeDlg, CDialogEx)

CSelPlateNcModeDlg::CSelPlateNcModeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SEL_PLATE_NC_DLG, pParent)
	, m_bSelFlame(FALSE)
	, m_bSelPlasma(FALSE)
	, m_bSelPunch(FALSE)
	, m_bSelDrill(FALSE)
	, m_bSelLaser(FALSE)
{

}

CSelPlateNcModeDlg::~CSelPlateNcModeDlg()
{
}

void CSelPlateNcModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_FLAME, m_bSelFlame);
	DDX_Check(pDX, IDC_CHK_PLASMA, m_bSelPlasma);
	DDX_Check(pDX, IDC_CHK_PUNCH, m_bSelPunch);
	DDX_Check(pDX, IDC_CHK_DRILL, m_bSelDrill);
	DDX_Check(pDX, IDC_CHK_LASER, m_bSelLaser);
}


BEGIN_MESSAGE_MAP(CSelPlateNcModeDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHK_FLAME, &CSelPlateNcModeDlg::OnBnClickedChkFlame)
	ON_BN_CLICKED(IDC_CHK_PLASMA, &CSelPlateNcModeDlg::OnBnClickedChkPlasma)
	ON_BN_CLICKED(IDC_CHK_PUNCH, &CSelPlateNcModeDlg::OnBnClickedChkPunch)
	ON_BN_CLICKED(IDC_CHK_DRILL, &CSelPlateNcModeDlg::OnBnClickedChkDrill)
	ON_BN_CLICKED(IDC_CHK_LASER, &CSelPlateNcModeDlg::OnBnClickedChkLaser)
END_MESSAGE_MAP()


// CSelPlateNcModeDlg 消息处理程序
BOOL CSelPlateNcModeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_bSelFlame = g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE);
	m_bSelPlasma = g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE);
	m_bSelPunch = g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE);
	m_bSelDrill = g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE);
	m_bSelLaser = g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE);
	UpdateData(FALSE);
	return TRUE;
}

void CSelPlateNcModeDlg::OnBnClickedChkFlame()
{
	UpdateData(TRUE);
	if (m_bSelFlame)
		g_sysPara.AddNcFlag(CNCPart::FLAME_MODE);
	else
		g_sysPara.DelNcFlag(CNCPart::FLAME_MODE);
	UpdateData(FALSE);
}


void CSelPlateNcModeDlg::OnBnClickedChkPlasma()
{
	UpdateData(TRUE);
	if (m_bSelPlasma)
		g_sysPara.AddNcFlag(CNCPart::PLASMA_MODE);
	else
		g_sysPara.DelNcFlag(CNCPart::PLASMA_MODE);
	UpdateData(FALSE);
}


void CSelPlateNcModeDlg::OnBnClickedChkPunch()
{
	UpdateData(TRUE);
	if (m_bSelPunch)
		g_sysPara.AddNcFlag(CNCPart::PUNCH_MODE);
	else
		g_sysPara.DelNcFlag(CNCPart::PUNCH_MODE);
	UpdateData(FALSE);
}


void CSelPlateNcModeDlg::OnBnClickedChkDrill()
{
	UpdateData(TRUE);
	if (m_bSelDrill)
		g_sysPara.AddNcFlag(CNCPart::DRILL_MODE);
	else
		g_sysPara.DelNcFlag(CNCPart::DRILL_MODE);
	UpdateData(FALSE);
}


void CSelPlateNcModeDlg::OnBnClickedChkLaser()
{
	UpdateData(TRUE);
	if (m_bSelLaser)
		g_sysPara.AddNcFlag(CNCPart::LASER_MODE);
	else
		g_sysPara.DelNcFlag(CNCPart::LASER_MODE);
	UpdateData(FALSE);
}
