// InputAnValDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "InputAnValDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInputAnIntegerValDlg dialog


CInputAnIntegerValDlg::CInputAnIntegerValDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputAnIntegerValDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInputAnIntegerValDlg)
	m_fVal = 0;
	m_sValTitle = _T("");
	//}}AFX_DATA_INIT
}


void CInputAnIntegerValDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInputAnIntegerValDlg)
	DDX_Text(pDX, IDC_E_VAL, m_fVal);
	DDX_Text(pDX, IDC_S_TITLE, m_sValTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInputAnIntegerValDlg, CDialog)
	//{{AFX_MSG_MAP(CInputAnIntegerValDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInputAnIntegerValDlg message handlers

BOOL CInputAnIntegerValDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SetWindowText("输入"+m_sValTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CInputAnStringValDlg dialog


CInputAnStringValDlg::CInputAnStringValDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputAnStringValDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInputAnStringValDlg)
	m_sItemValue = _T("");
	//}}AFX_DATA_INIT
	m_sCaption = _T("");
	m_nStrMaxLen = 1000000000;	//默认一个足够大的值
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

	SetWindowText("输入"+m_sCaption);

	return TRUE;
}

void CInputAnStringValDlg::OnOK() 
{
	UpdateData();
	if(m_sItemValue.GetLength()>m_nStrMaxLen)
		AfxMessageBox("字符串过长，请重新输入");
	else
		CDialog::OnOK();
}