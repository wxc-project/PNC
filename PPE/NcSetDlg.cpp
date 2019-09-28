// NcSetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PPE.h"
#include "NcSetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNcSetDlg dialog


CNcSetDlg::CNcSetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNcSetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNcSetDlg)
	m_fHoleDIncrement = 0.0;
	m_fMKHoleD = 10.0;
	//}}AFX_DATA_INIT
}


void CNcSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNcSetDlg)
	DDX_Text(pDX, IDC_E_HOLE_D_INCREMENT, m_fHoleDIncrement);
	DDX_Text(pDX, IDC_E_MK_HOLE_D, m_fMKHoleD);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNcSetDlg, CDialog)
	//{{AFX_MSG_MAP(CNcSetDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNcSetDlg message handlers
