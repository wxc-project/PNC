// SelColorDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "SelColorDlg.h"
#include "Resource.h"
#include "PNCSysPara.h"

// CSelColorDlg 对话框

IMPLEMENT_DYNAMIC(CSelColorDlg, CDialog)

CSelColorDlg::CSelColorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SEL_COLOR_DLG, pParent)
{

}

CSelColorDlg::~CSelColorDlg()
{
}

void CSelColorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_EDGE_CLR, m_cmbEdgeClr);
	DDX_Control(pDX, IDC_CMB_M12_CLR, m_cmbM12Clr);
	DDX_Control(pDX, IDC_CMB_M16_CLR, m_cmbM16Clr);
	DDX_Control(pDX, IDC_CMB_M20_CLR, m_cmbM20Clr);
	DDX_Control(pDX, IDC_CMB_M24_CLR, m_cmbM24Clr);
	DDX_Control(pDX, IDC_CMB_OTHER_CLR, m_cmbOtherClr);
}


BEGIN_MESSAGE_MAP(CSelColorDlg, CDialog)
END_MESSAGE_MAP()


// CSelColorDlg 消息处理程序
BOOL CSelColorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	m_cmbEdgeClr.InitBox(g_pncSysPara.crMode.crEdge);
	m_cmbM12Clr.InitBox(g_pncSysPara.crMode.crLS12);
	m_cmbM16Clr.InitBox(g_pncSysPara.crMode.crLS16);
	m_cmbM20Clr.InitBox(g_pncSysPara.crMode.crLS20);
	m_cmbM24Clr.InitBox(g_pncSysPara.crMode.crLS24);
	m_cmbOtherClr.InitBox(g_pncSysPara.crMode.crOtherLS);
	//
	UpdateData(FALSE);
	return TRUE;
}

void CSelColorDlg::OnOK()
{
	UpdateData();
	int iCurSel = -1;
	iCurSel = m_cmbEdgeClr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crEdge = (DWORD)m_cmbEdgeClr.GetItemData(iCurSel);
	//
	iCurSel = m_cmbM12Clr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crLS12 = (DWORD)m_cmbM12Clr.GetItemData(iCurSel);
	//
	iCurSel = m_cmbM16Clr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crLS16 = (DWORD)m_cmbM16Clr.GetItemData(iCurSel);
	//
	iCurSel = m_cmbM20Clr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crLS20 = (DWORD)m_cmbM20Clr.GetItemData(iCurSel);
	//
	iCurSel = m_cmbM24Clr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crLS24 = (DWORD)m_cmbM24Clr.GetItemData(iCurSel);
	//
	iCurSel = m_cmbOtherClr.GetCurSel();
	if (iCurSel > -1)
		g_pncSysPara.crMode.crOtherLS = (DWORD)m_cmbOtherClr.GetItemData(iCurSel);
	//
	CDialog::OnOK();
}

