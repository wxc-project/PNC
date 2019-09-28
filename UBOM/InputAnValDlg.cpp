// InputAnValDlg.cpp : implementation file
//<LOCALE_TRANSLATE BY wbt />

#include "stdafx.h"
#include "InputAnValDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInputAnStringValDlg dialog


CInputAnStringValDlg::CInputAnStringValDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputAnStringValDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInputAnStringValDlg)
	m_sItemValue = _T("");
	//}}AFX_DATA_INIT
	m_nStrMaxLen = 1000000000;	//默认一个足够大的值
	m_sTip = _T("");
}


void CInputAnStringValDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInputAnStringValDlg)
	DDX_Text(pDX, IDC_E_VAL, m_sItemValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInputAnStringValDlg, CDialog)
	//{{AFX_MSG_MAP(CInputAnStringValDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInputAnStringValDlg message handlers

BOOL CInputAnStringValDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if(m_sTip.GetLength()>0)
	{
		m_toolTipCtrl.Create(this,TTS_ALWAYSTIP);
		m_toolTipCtrl.AddTool(this,m_sTip);
		m_toolTipCtrl.AddTool(GetDlgItem(IDC_E_VAL),m_sTip);
		m_toolTipCtrl.SetDelayTime(100);
		m_toolTipCtrl.SetDelayTime(TTDT_AUTOPOP,30000);
		m_toolTipCtrl.SetMaxTipWidth(200);
		m_toolTipCtrl.SetTipTextColor(RGB(0,0,0)); 
		m_toolTipCtrl.SetTipBkColor( RGB(255,255,0)); 
		m_toolTipCtrl.Activate(TRUE);
	}
	return TRUE;
}
void CInputAnStringValDlg::OnOK() 
{
	UpdateData();
	if(m_sItemValue.GetLength()>m_nStrMaxLen)
#ifdef AFX_TARG_ENU_ENGLISH
		AfxMessageBox("The string is too long, please input again");
#else 
		AfxMessageBox("字符串过长，请重新输入");
#endif
	else
		CDialog::OnOK();
}

BOOL CInputAnStringValDlg::PreTranslateMessage(MSG* pMsg)
{
	if(m_toolTipCtrl.GetSafeHwnd()!=NULL)
	{
		switch(pMsg->message)
		{ 
		case WM_LBUTTONDOWN: 
		case WM_LBUTTONUP: 
		case WM_MOUSEMOVE: 
			m_toolTipCtrl.RelayEvent(pMsg); 
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}
