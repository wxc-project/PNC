// PlankVertexDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PlankVertexDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlankVertexDlg dialog


CPlankVertexDlg::CPlankVertexDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlankVertexDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlankVertexDlg)
	m_iVertexType = 0;
	m_fPosX = 0.0;
	m_fPosY = 0.0;
	m_bPolarCS = FALSE;
	m_bCartesianCS = TRUE;
	m_fAlfa = 0.0;
	m_fR = 0.0;
	//}}AFX_DATA_INIT
}


void CPlankVertexDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlankVertexDlg)
	DDX_CBIndex(pDX, IDC_CMB_VERTEX_TYPE, m_iVertexType);
	DDX_Text(pDX, IDC_E_PT_X, m_fPosX);
	DDX_Text(pDX, IDC_E_PT_Y, m_fPosY);
	DDX_Check(pDX, IDC_CHK_POLAR_CS, m_bPolarCS);
	DDX_Check(pDX, IDC_CHK_CARTESIAN_CS, m_bCartesianCS);
	DDX_Text(pDX, IDC_E_PT_ALFA, m_fAlfa);
	DDX_Text(pDX, IDC_E_PT_R, m_fR);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlankVertexDlg, CDialog)
	//{{AFX_MSG_MAP(CPlankVertexDlg)
	ON_BN_CLICKED(IDC_CHK_CARTESIAN_CS, OnChkTransByCartesian)
	ON_EN_CHANGE(IDC_E_PT_ALFA, OnChangeEPolarCoord)
	ON_EN_CHANGE(IDC_E_PT_X, OnChangeECartesianCoord)
	ON_BN_CLICKED(IDC_CHK_POLAR_CS, OnChkTransByPolar)
	ON_EN_CHANGE(IDC_E_PT_R, OnChangeEPolarCoord)
	ON_EN_CHANGE(IDC_E_PT_Y, OnChangeECartesianCoord)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlankVertexDlg message handlers

BOOL CPlankVertexDlg::OnInitDialog() 
{
	m_bPolarCS=!m_bCartesianCS;
	CDialog::OnInitDialog();
	
	((CEdit*)GetDlgItem(IDC_E_PT_ALFA))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_R))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_X))->SetReadOnly(!m_bCartesianCS);
	((CEdit*)GetDlgItem(IDC_E_PT_Y))->SetReadOnly(!m_bCartesianCS);
	if(m_bPolarCS)
		OnChangeEPolarCoord();
	else
		OnChangeECartesianCoord();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPlankVertexDlg::OnChkTransByPolar() 
{
	UpdateData();
	m_bCartesianCS=!m_bPolarCS;
	((CEdit*)GetDlgItem(IDC_E_PT_ALFA))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_R))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_X))->SetReadOnly(!m_bCartesianCS);
	((CEdit*)GetDlgItem(IDC_E_PT_Y))->SetReadOnly(!m_bCartesianCS);
	UpdateData(FALSE);
}

void CPlankVertexDlg::OnChkTransByCartesian() 
{
	UpdateData();
	m_bPolarCS=!m_bCartesianCS;
	((CEdit*)GetDlgItem(IDC_E_PT_ALFA))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_R))->SetReadOnly(!m_bPolarCS);
	((CEdit*)GetDlgItem(IDC_E_PT_X))->SetReadOnly(!m_bCartesianCS);
	((CEdit*)GetDlgItem(IDC_E_PT_Y))->SetReadOnly(!m_bCartesianCS);
	UpdateData(FALSE);
}

void CPlankVertexDlg::OnChangeEPolarCoord() 
{
	UpdateData();
	double alfa = m_fAlfa*RADTODEG_COEF;
	m_fPosX=m_fR*cos(alfa);
	m_fPosY=m_fR*sin(alfa);
	UpdateData(FALSE);
}

void CPlankVertexDlg::OnChangeECartesianCoord() 
{
	CString rString;
	GetDlgItem(IDC_E_PT_X)->GetWindowText(rString);
	if(rString.CompareNoCase("-")||rString.IsEmpty())
		return;	//暂时不合法的坐标输入值，可能由于刚清空或未输入全
	GetDlgItem(IDC_E_PT_Y)->GetWindowText(rString);
	if(rString.CompareNoCase("-")||rString.IsEmpty())
		return;	//暂时不合法的坐标输入值，可能由于刚清空或未输入全
	UpdateData();
	m_fR=sqrt(m_fPosX*m_fPosX+m_fPosY*m_fPosY);
	double alfa=Cal2dLineAng(0,0,m_fPosX,m_fPosY);
	m_fAlfa=alfa*DEGTORAD_COEF;
	UpdateData(FALSE);
}
