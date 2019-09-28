// SideBaffleHighDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PPE.h"
#include "SideBaffleHighDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSideBaffleHighDlg dialog


CSideBaffleHighDlg::CSideBaffleHighDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSideBaffleHighDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSideBaffleHighDlg)
	m_fSideBaffleHigh = 200.0;
	//}}AFX_DATA_INIT
}


void CSideBaffleHighDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSideBaffleHighDlg)
	DDX_Text(pDX, IDC_E_SIDE_BAFFLE_HIGH, m_fSideBaffleHigh);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSideBaffleHighDlg, CDialog)
	//{{AFX_MSG_MAP(CSideBaffleHighDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSideBaffleHighDlg message handlers
