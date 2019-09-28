// InputMKRectDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InputMKRectDlg.h"


// CInputMKRectDlg 对话框

IMPLEMENT_DYNAMIC(CInputMKRectDlg, CDialog)

CInputMKRectDlg::CInputMKRectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputMKRectDlg::IDD, pParent)
	, m_nRectL(0)
	, m_nRectW(0)
	, m_fTextH(0)
{

}

CInputMKRectDlg::~CInputMKRectDlg()
{
}

void CInputMKRectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_RECT_L, m_nRectL);
	DDX_Text(pDX, IDC_E_RECT_W, m_nRectW);
	DDX_Text(pDX, IDC_E_TEXT_H, m_fTextH);
}


BEGIN_MESSAGE_MAP(CInputMKRectDlg, CDialog)
END_MESSAGE_MAP()


// CInputMKRectDlg 消息处理程序
