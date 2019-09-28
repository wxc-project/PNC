// NewPartFileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PPE.h"
#include "NewPartFileDlg.h"
#include "Query.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewPartFileDlg dialog


CNewPartFileDlg::CNewPartFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewPartFileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewPartFileDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_sPartNo = _T("");
	m_sLen = _T("1500");
	m_sSpec = _T("40X3");
	m_sMaterial = _T("Q235");
	m_bImprotNcFile=FALSE;
}


void CNewPartFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewPartFileDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_E_PARTNO, m_sPartNo);
	DDX_Text(pDX, IDC_E_LEN, m_sLen);
	DDX_CBString(pDX, IDC_CMB_SPEC, m_sSpec);
	DDX_CBString(pDX, IDC_CMB_MATERIAL, m_sMaterial);
}


BEGIN_MESSAGE_MAP(CNewPartFileDlg, CDialog)
	//{{AFX_MSG_MAP(CNewPartFileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewPartFileDlg message handlers


BOOL CNewPartFileDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pCMB = (CComboBox*)GetDlgItem(IDC_CMB_SPEC);
	AddJgRecord(pCMB);
	int iSel = pCMB->FindString(0,m_sSpec);
	if(iSel>=0)
		pCMB->SetCurSel(iSel);
	pCMB = (CComboBox*)GetDlgItem(IDC_CMB_MATERIAL);
	AddSteelMatRecord(pCMB);
	iSel = pCMB->FindString(0,m_sMaterial);
	if(iSel>=0)
		pCMB->SetCurSel(iSel);
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CNewPartFileDlg::OnOK()
{
	UpdateData();
	if(m_sPartNo.CompareNoCase("CNC")==0||m_sPartNo.CompareNoCase("TXT")==0)
		m_bImprotNcFile=TRUE;
	else
		m_bImprotNcFile=FALSE;
	CDialog::OnOK();
}
