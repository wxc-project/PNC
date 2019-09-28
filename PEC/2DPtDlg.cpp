// 2DPtDlg.cpp : implementation file
//

#include "stdafx.h"
#include "2DPtDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C2DPtDlg dialog


C2DPtDlg::C2DPtDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C2DPtDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(C2DPtDlg)
	m_fPtX = 0.0;
	m_fPtY = 0.0;
	//}}AFX_DATA_INIT
	m_bCanModify = TRUE;
}


void C2DPtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C2DPtDlg)
	DDX_Text(pDX, IDC_E_PT_X, m_fPtX);
	DDX_Text(pDX, IDC_E_PT_Y, m_fPtY);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(C2DPtDlg, CDialog)
	//{{AFX_MSG_MAP(C2DPtDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C2DPtDlg message handlers

BOOL C2DPtDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_E_PT_X)->EnableWindow(m_bCanModify);
	GetDlgItem(IDC_E_PT_Y)->EnableWindow(m_bCanModify);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
