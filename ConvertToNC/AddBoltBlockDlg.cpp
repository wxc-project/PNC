// AddBoltBlock.cpp : 实现文件
//

#include "stdafx.h"
#include "AddBoltBlockDlg.h"


// CAddBoltBlock 对话框

IMPLEMENT_DYNAMIC(CAddBoltBlockDlg, CDialog)

CAddBoltBlockDlg::CAddBoltBlockDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddBoltBlockDlg::IDD, pParent)
{

}

CAddBoltBlockDlg::~CAddBoltBlockDlg()
{
}

void CAddBoltBlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_BLOCK_NAME, m_sBlockName);
	DDX_Text(pDX, IDC_E_BOLT_D, m_sBoltD);
	DDX_Text(pDX, IDC_E_HOLE_D, m_sHoleD);
}


BEGIN_MESSAGE_MAP(CAddBoltBlockDlg, CDialog)
END_MESSAGE_MAP()


// CAddBoltBlock 消息处理程序
