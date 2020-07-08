// SelDisplayNcDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "SelDisplayNcDlg.h"
#include "afxdialogex.h"
#include "SysPara.h"
#include "PPEModel.h"

// CSelDisplayNcDlg 对话框

IMPLEMENT_DYNAMIC(CSelDisplayNcDlg, CDialogEx)

CSelDisplayNcDlg::CSelDisplayNcDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DISPLAY_NC_DLG, pParent)
{
	m_iCutType = 0;
	m_iProcessType = 0;
}

CSelDisplayNcDlg::~CSelDisplayNcDlg()
{
}

void CSelDisplayNcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RAD_FLAME, m_iCutType);
	DDX_Radio(pDX, IDC_RAD_PUNCH, m_iProcessType);
}


BEGIN_MESSAGE_MAP(CSelDisplayNcDlg, CDialogEx)
END_MESSAGE_MAP()


// CSelDisplayNcDlg 消息处理程序
BOOL CSelDisplayNcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	if (g_sysPara.IsValidDisplayFlag(CNCPart::FLAME_MODE))
		m_iCutType = 0;
	else if (g_sysPara.IsValidDisplayFlag(CNCPart::PLASMA_MODE))
		m_iCutType = 1;
	//
	if (g_sysPara.IsValidDisplayFlag(CNCPart::PUNCH_MODE))
		m_iProcessType = 0;
	else if (g_sysPara.IsValidDisplayFlag(CNCPart::DRILL_MODE))
		m_iProcessType = 1;
	UpdateData(FALSE);
	return TRUE;
}

void CSelDisplayNcDlg::OnOK()
{
	UpdateData(TRUE);
	g_sysPara.nc.m_ciDisplayType = 0;
	if (m_iCutType == 0)
		g_sysPara.nc.m_ciDisplayType |= CNCPart::FLAME_MODE;
	else
		g_sysPara.nc.m_ciDisplayType |= CNCPart::PLASMA_MODE;
	//
	if (m_iProcessType == 0)
		g_sysPara.nc.m_ciDisplayType |= CNCPart::PUNCH_MODE;
	else
		g_sysPara.nc.m_ciDisplayType |= CNCPart::DRILL_MODE;
	//复合模式下，更新螺栓孔信息
	for (CProcessPart*  pSrcPlate = model.EnumPartFirst(); pSrcPlate; pSrcPlate = model.EnumPartNext())
	{
		if(!pSrcPlate->IsPlate())
			continue;
		CProcessPlate* pDestPlate = NULL, *pCompPlate = NULL;
		if (g_sysPara.IsValidDisplayFlag(CNCPart::PUNCH_MODE))
		{
			pDestPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), CNCPart::PUNCH_MODE);
			pCompPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), g_sysPara.nc.m_ciDisplayType);
			pCompPlate->m_xBoltInfoList.Empty();
			for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
				pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
		}
		else if(g_sysPara.IsValidDisplayFlag(CNCPart::DRILL_MODE))
		{
			pDestPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), CNCPart::DRILL_MODE);
			pCompPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), g_sysPara.nc.m_ciDisplayType);
			pCompPlate->m_xBoltInfoList.Empty();
			for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
				pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
		}	
	}
	return CDialogEx::OnOK();
}

