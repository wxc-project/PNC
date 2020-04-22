#include "stdafx.h"
#include "SelectJgCardDlg.h"
#include "XhCharString.h"
#include "InputDlg.h"
#include "CadToolFunc.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectJgCardDlg dialog
static void SelectCardPath(CString& sJgCardPath)
{
	CFileDialog dlg(TRUE,"dwg","工艺卡",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
#ifdef AFX_TARG_ENU_ENGLISH
		"process card(*.dwg)|*.dwg||");
#else
		"工艺卡(*.dwg)|*.dwg||");
#endif
	if(dlg.DoModal()!=IDOK)
		return;
	sJgCardPath=dlg.GetPathName();
}

CSelectJgCardDlg::CSelectJgCardDlg(CWnd* pParent /*=NULL*/)
	:	CDialog(CSelectJgCardDlg::IDD, pParent)
	,   m_sJgCardPath(_T(""))
{
	m_pWnd = pParent;
}

void CSelectJgCardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcBarDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CMB_CARD_PATH, m_xPathCmbBox);
	DDX_CBString(pDX, IDC_CMB_CARD_PATH, m_sJgCardPath);
}

BEGIN_MESSAGE_MAP(CSelectJgCardDlg, CDialog)
	//{{AFX_MSG_MAP(CProcBarDlg)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_CMB_CARD_PATH, &CSelectJgCardDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectJgCardDlg message handlers
BOOL CSelectJgCardDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_xPathCmbBox.InsertString(0,CString("<编辑...>"));
	m_xPathCmbBox.InsertString(1,CString("<浏览...>"));
	char path[MAX_PATH]="";
	if(GetJgCardPath(path))
	{
		m_sJgCardPath=path;
		m_xPathCmbBox.InsertString(2,path);
		m_xPathCmbBox.SetCurSel(2);
	}
	return TRUE;
}

void CSelectJgCardDlg::OnOK()
{
	SetJgCardPath(m_sJgCardPath);
	CDialog::OnOK();
}
void CSelectJgCardDlg::OnCancel() 
{
	CDialog::OnCancel();
}

void CSelectJgCardDlg::OnCbnSelchangeCombo1()
{
	int nIndex=m_xPathCmbBox.GetCurSel();
	CString path_str;
	if(nIndex==0)
	{
 		CInputDlg dlg;
 		if(dlg.DoModal()==IDOK)
 			path_str=dlg.m_sInputValue;
	}
	else if(nIndex==1)
		SelectCardPath(path_str);
	else
		m_xPathCmbBox.GetLBText(nIndex,path_str);
	nIndex=m_xPathCmbBox.FindString(0,path_str);
	if(nIndex<0 && path_str.GetLength()>0)
	{
		int nCount=m_xPathCmbBox.GetCount();
		m_xPathCmbBox.InsertString(nCount,path_str);
	}
	UpdateData();
	m_sJgCardPath=path_str;
	UpdateData(FALSE);
}
