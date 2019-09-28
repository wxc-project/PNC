// PartPropertyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "PartPropertyDlg.h"


// CPartPropertyDlg 对话框

IMPLEMENT_DYNCREATE(CPartPropertyDlg, CDialog)

CPartPropertyDlg::CPartPropertyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPartPropertyDlg::IDD, pParent)
{

}

CPartPropertyDlg::~CPartPropertyDlg()
{
}

void CPartPropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPartPropertyDlg, CDialog)
END_MESSAGE_MAP()


// CPartPropertyDlg 消息处理程序
