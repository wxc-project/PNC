// ReConnServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ReConnServerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReConnServerDlg dialog


CReConnServerDlg::CReConnServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReConnServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReConnServerDlg)
	m_sServerName = _T("");
	m_sUserName = _T("");
	m_sPassword = _T("");
	//}}AFX_DATA_INIT
	m_bInputPassword=FALSE;
}


void CReConnServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReConnServerDlg)
	DDX_Text(pDX, IDC_E_SERVER_NAME, m_sServerName);
	DDX_Text(pDX, IDC_E_USER_NAME, m_sUserName);
	DDX_Text(pDX, IDC_E_USER_PASSWORD, m_sPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReConnServerDlg, CDialog)
	//{{AFX_MSG_MAP(CReConnServerDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReConnServerDlg message handlers
BOOL CReConnServerDlg::OnInitDialog() 
{
	if(!m_bInputPassword)
	{	//1.隐藏密码输入控件
		CWnd *pKeyWnd=GetDlgItem(IDC_S_KEY);
		pKeyWnd->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_S_PASSWORD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_USER_PASSWORD)->ShowWindow(SW_HIDE);
		//2.调整窗口尺寸及按钮位置
		CRect rc;
		pKeyWnd->GetWindowRect(rc);
		int nHeight=rc.Height();
		GetWindowRect(rc);
		rc.bottom-=nHeight;
		MoveWindow(rc);
		for(int i=0;i<2;i++)
		{
			CRect rcSon;
			CWnd *pSonWnd=GetDlgItem(IDCANCEL);
			if(i==1)
				pSonWnd=GetDlgItem(IDOK);
			pSonWnd->GetWindowRect(rcSon);
			ScreenToClient(rcSon);
			rcSon.top-=nHeight;
			rcSon.bottom-=nHeight;
			pSonWnd->MoveWindow(rcSon);
		}
	}
	return CDialog::OnInitDialog();
}