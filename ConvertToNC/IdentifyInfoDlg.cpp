// IdentifyInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IdentifyInfoDlg.h"
#include "PNCSysPara.h"

// CIdentifyInfoDlg 对话框

IMPLEMENT_DYNAMIC(CIdentifyInfoDlg, CDialog)

CIdentifyInfoDlg::CIdentifyInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIdentifyInfoDlg::IDD, pParent)
	, m_sPnKey(_T(""))
{
	m_arrIsCanUse[0]=FALSE;
	m_arrIsCanUse[1]=FALSE;
	m_arrIsCanUse[2]=FALSE;
	m_arrIsCanUse[3]=FALSE;
}

CIdentifyInfoDlg::~CIdentifyInfoDlg()
{
}

void CIdentifyInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_PARTNO, m_arrIsCanUse[0]);
	DDX_Check(pDX, IDC_CHK_THICK, m_arrIsCanUse[1]);
	DDX_Check(pDX, IDC_CHK_MATERIAL, m_arrIsCanUse[2]);
	DDX_Check(pDX, IDC_CHK_NUM, m_arrIsCanUse[3]);
	DDX_Text(pDX, IDC_E_PARTNO, m_sPnKey);
	DDX_Text(pDX, IDC_E_THICK, m_sThickKey);
	DDX_Text(pDX, IDC_E_MATERIAL, m_sMatKey);
	DDX_Text(pDX, IDC_E_NUM, m_sNumKey);
}


BEGIN_MESSAGE_MAP(CIdentifyInfoDlg, CDialog)
	ON_BN_CLICKED(IDC_CHK_PARTNO, OnChkPartNo)
	ON_BN_CLICKED(IDC_CHK_THICK, OnChkThick)
	ON_BN_CLICKED(IDC_CHK_MATERIAL, OnChkMaterial)
	ON_BN_CLICKED(IDC_CHK_NUM, OnChkNum)
END_MESSAGE_MAP()


// CIdentifyInfoDlg 消息处理程序
BOOL CIdentifyInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//件号识别
	if(g_pncSysPara.m_sPnKey.Length>0)
	{
		m_arrIsCanUse[0]=TRUE;
		m_sPnKey=g_pncSysPara.m_sPnKey;
	}
	((CEdit*)GetDlgItem(IDC_E_PARTNO))->SetReadOnly(!m_arrIsCanUse[0]);
	//厚度识别
	if(g_pncSysPara.m_sThickKey.Length>0)
	{
		m_arrIsCanUse[1]=TRUE;
		m_sThickKey=g_pncSysPara.m_sThickKey;
	}
	((CEdit*)GetDlgItem(IDC_E_THICK))->SetReadOnly(!m_arrIsCanUse[1]);
	//材质识别
	if(g_pncSysPara.m_sMatKey.Length>0)
	{
		m_arrIsCanUse[2]=TRUE;
		m_sMatKey=g_pncSysPara.m_sMatKey;
	}
	((CEdit*)GetDlgItem(IDC_E_MATERIAL))->SetReadOnly(!m_arrIsCanUse[2]);
	//厚度识别
	if(g_pncSysPara.m_sPnNumKey.Length>0)
	{
		m_arrIsCanUse[3]=TRUE;
		m_sNumKey=g_pncSysPara.m_sPnNumKey;
	}
	((CEdit*)GetDlgItem(IDC_E_NUM))->SetReadOnly(!m_arrIsCanUse[3]);
	
	UpdateData(FALSE);
	return TRUE;
}
void CIdentifyInfoDlg::OnChkPartNo()
{
	m_arrIsCanUse[0]=!m_arrIsCanUse[0];
	((CEdit*)GetDlgItem(IDC_E_PARTNO))->SetReadOnly(!m_arrIsCanUse[0]);
}
void CIdentifyInfoDlg::OnChkThick()
{
	m_arrIsCanUse[1]=!m_arrIsCanUse[1];
	((CEdit*)GetDlgItem(IDC_E_THICK))->SetReadOnly(!m_arrIsCanUse[1]);
}
void CIdentifyInfoDlg::OnChkMaterial()
{
	m_arrIsCanUse[2]=!m_arrIsCanUse[2];
	((CEdit*)GetDlgItem(IDC_E_MATERIAL))->SetReadOnly(!m_arrIsCanUse[2]);
}
void CIdentifyInfoDlg::OnChkNum()
{
	m_arrIsCanUse[3]=!m_arrIsCanUse[3];
	((CEdit*)GetDlgItem(IDC_E_NUM))->SetReadOnly(!m_arrIsCanUse[3]);
}