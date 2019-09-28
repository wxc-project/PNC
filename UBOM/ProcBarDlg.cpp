#include "stdafx.h"
#include "ProcBarDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CProcBarDlg dialog


CProcBarDlg::CProcBarDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcBarDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProcBarDlg)
	m_sProgress = _T("0%");
	//}}AFX_DATA_INIT
	m_pWnd = pParent;
}
BOOL CProcBarDlg::Create()
{
	return CDialog::Create(CProcBarDlg::IDD);
}


void CProcBarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcBarDlg)
	DDX_Control(pDX, IDC_PROGRESS_BAR, m_ctrlProcBar);
	DDX_Text(pDX, IDC_NUMERIC_PROC, m_sProgress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProcBarDlg, CDialog)
	//{{AFX_MSG_MAP(CProcBarDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcBarDlg message handlers
BOOL CProcBarDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ctrlProcBar.SetRange(0,100);
	return TRUE;
}

void CProcBarDlg::SetTitle(CString ss)
{
	SetWindowText(ss);
}

void CProcBarDlg::Refresh(int proc)
{
	m_ctrlProcBar.SetPos(proc);
	m_sProgress.Format("%d%%",proc);
	UpdateData(FALSE);
}

