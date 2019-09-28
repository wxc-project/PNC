// KaiHeLineAngleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "KaiHeLineAngleDlg.h"
#include "afxdialogex.h"


// CKaiHeLineAngleDlg 对话框

IMPLEMENT_DYNAMIC(CKaiHeLineAngleDlg, CDialog)

CKaiHeLineAngleDlg::CKaiHeLineAngleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKaiHeLineAngleDlg::IDD, pParent)
	, m_fKaiHeAngle(0)
	, m_iDimPos(0)
	, m_iLeftLen(0)
	, m_iRightLen(0)
	, m_sKaiHeWing(_T(""))
{
	m_iKaiHeWing=0;
	m_bHasKaiHeWing=FALSE;
}

CKaiHeLineAngleDlg::~CKaiHeLineAngleDlg()
{
}

void CKaiHeLineAngleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_KAIHE_ANGLE, m_fKaiHeAngle);
	DDX_Text(pDX, IDC_E_DIM_POS, m_iDimPos);
	DDX_Text(pDX, IDC_E_LEFT_LEN, m_iLeftLen);
	DDX_Text(pDX, IDC_E_RIGHT_LEN, m_iRightLen);
	DDX_CBIndex(pDX, IDC_CMB_KAIHE_WING, m_iKaiHeWing);
	DDX_Text(pDX, IDC_E_KAIHE_WING, m_sKaiHeWing);
}


BEGIN_MESSAGE_MAP(CKaiHeLineAngleDlg, CDialog)
END_MESSAGE_MAP()


// CKaiHeLineAngleDlg 消息处理程序
BOOL CKaiHeLineAngleDlg::OnInitDialog() 
{
	if(m_bHasKaiHeWing)
	{
		GetDlgItem(IDC_CMB_KAIHE_WING)->ShowWindow(SW_HIDE);
		CEdit* pEdit=(CEdit*)GetDlgItem(IDC_E_KAIHE_WING);
		pEdit->ShowWindow(SW_SHOW);
		pEdit->SetReadOnly(TRUE);
		if(m_iKaiHeWing==0)
			m_sKaiHeWing="X肢";
		else
			m_sKaiHeWing="Y肢";
		
	}
	else
	{
		GetDlgItem(IDC_E_KAIHE_WING)->ShowWindow(SW_HIDE);
		CComboBox *pCMB = (CComboBox*)GetDlgItem(IDC_CMB_KAIHE_WING);
		pCMB->ShowWindow(SW_SHOW);
		pCMB->SetCurSel(m_iKaiHeWing);
	}
	CDialog::OnInitDialog();
	return TRUE;
}
void CKaiHeLineAngleDlg::OnOK()
{
	CDialog::OnOK();
}
