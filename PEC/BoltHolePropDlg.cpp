// LsFeatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BoltHolePropDlg.h"
#include "Query.h"
#include "CfgPartNoDlg.h"
#include "LmaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#if !defined(__TSA_)&&!defined(__TSA_FILE_)

/////////////////////////////////////////////////////////////////////////////
// CBoltPropDlg dialog
BOOL ReadClipPoint(f3dPoint &point);
void WritePointToClip(f3dPoint point);

CBoltHolePropDlg::CBoltHolePropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBoltHolePropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBoltPropDlg)
	m_sLsGuiGe = _T("6.8M20X50");
	m_fHoleD = 21.5;
	m_fPosX = 0.0;
	m_fPosY = 0.0;
	m_fPosZ = 0.0;
	m_hLs = _T("");
	m_fNormX = 0.0;
	m_fNormY = 0.0;
	m_fNormZ = 1.0;
	m_sRayNo = _T("");
	m_bVirtualBolt=TRUE;
	m_nWaistLen = 0;
	//}}AFX_DATA_INIT
	m_bCanModifyPos = TRUE;
	m_pLs=NULL;
	m_pWorkPart=NULL;
	m_dwRayNo=0;
	waist_vec.Set(1,0,0);
}


void CBoltHolePropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBoltPropDlg)
	DDX_CBString(pDX, IDC_CMB_LS_GUIGE, m_sLsGuiGe);
	DDX_Text(pDX, IDC_E_HOLE_D, m_fHoleD);
	DDX_Text(pDX, IDC_E_X, m_fPosX);
	DDX_Text(pDX, IDC_E_Y, m_fPosY);
	DDX_Text(pDX, IDC_E_Z, m_fPosZ);
	DDX_Text(pDX, IDC_S_LS_HANDLE, m_hLs);
	DDX_Text(pDX, IDC_E_NORM_X, m_fNormX);
	DDX_Text(pDX, IDC_E_NORM_Y, m_fNormY);
	DDX_Text(pDX, IDC_E_NORM_Z, m_fNormZ);
	DDX_Text(pDX, IDC_E_RAY_NO, m_sRayNo);
	DDX_Check(pDX, IDC_CHK_VIRTUAL_BOLT, m_bVirtualBolt);
	DDX_Text(pDX, IDC_E_WAIST_LEN, m_nWaistLen);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBoltHolePropDlg, CDialog)
	//{{AFX_MSG_MAP(CBoltPropDlg)
	ON_CBN_EDITCHANGE(IDC_CMB_LS_GUIGE, OnChangeCmbLsGuige)
	ON_EN_CHANGE(IDC_E_Y, OnChangeEY)
	ON_BN_CLICKED(IDC_BN_COPY_POS, OnBnCopyPos)
	ON_BN_CLICKED(IDC_BN_COPY_NORM, OnBnCopyNorm)
	ON_BN_CLICKED(IDC_BN_PASTE_NORM, OnBnPasteNorm)
	ON_BN_CLICKED(IDC_BN_PASTE_POS, OnBnPastePos)
	ON_BN_CLICKED(IDC_BN_LS_RAYNO, OnBnLsRayNo)
	ON_EN_KILLFOCUS(IDC_E_X, OnKillfocusEX)
	ON_CBN_SELCHANGE(IDC_CMB_LS_GUIGE, OnChangeCmbLsGuige)
	ON_BN_CLICKED(IDC_BTN_MODIFY_WAIST_VEC, OnModifyWaistVec)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBoltPropDlg message handlers

BOOL CBoltHolePropDlg::OnInitDialog() 
{
	CComboBox *pCMB = (CComboBox*)GetDlgItem(IDC_CMB_LS_GUIGE);
	UpdateCfgPartNoString();
	if(m_pLs)
	{
		AddLsRecord(pCMB,NULL);
		m_dwRayNo=m_pLs->dwRayNo;
	}
	else	//新生成螺栓
	{
		AddLsRecord(pCMB,0);
		waist_vec.Set(1,0,0);	//初始化腰圆孔方向
	}
	int iCurSel = pCMB->FindString(0,m_sLsGuiGe);
	if(iCurSel>=0)
		pCMB->SetCurSel(iCurSel);
	else
	{
		pCMB->AddString(m_sLsGuiGe);
		pCMB->SetCurSel(pCMB->GetCount()-1);
	}
	if(fabs(m_fPosX)<eps)
		m_fPosX =0;
	if(fabs(m_fPosY)<eps)
		m_fPosY =0;
	if(fabs(m_fPosZ)<eps)
		m_fPosZ =0;
	if(fabs(m_fNormX)<eps)
		m_fNormX =0;
	if(fabs(m_fNormY)<eps)
		m_fNormY =0;
	if(fabs(m_fNormZ)<eps)
		m_fNormZ =0;
	((CEdit*)GetDlgItem(IDC_E_X))->SetReadOnly(!m_bCanModifyPos);
	((CEdit*)GetDlgItem(IDC_E_Y))->SetReadOnly(!m_bCanModifyPos);
	((CEdit*)GetDlgItem(IDC_E_Z))->SetReadOnly(!m_bCanModifyPos);
	GetDlgItem(IDC_BN_PASTE_POS)->EnableWindow(m_bCanModifyPos);
	if(m_pLs)
	{
		//仅当螺栓法线设计类型为指定法线时才可以修改螺栓法线位置，否则修改后一到位将恢复到原来的值
		GetDlgItem(IDC_BN_PASTE_NORM)->EnableWindow(FALSE);
		((CEdit*)GetDlgItem(IDC_E_NORM_X))->SetReadOnly(TRUE);
		((CEdit*)GetDlgItem(IDC_E_NORM_Y))->SetReadOnly(TRUE);
		((CEdit*)GetDlgItem(IDC_E_NORM_Z))->SetReadOnly(TRUE);
	}
	CDialog::OnInitDialog();
	OnChangeEY();
	return TRUE;
}

void CBoltHolePropDlg::OnChangeCmbLsGuige() 
{
	CComboBox *pCMB = (CComboBox*)GetDlgItem(IDC_CMB_LS_GUIGE);
	int iCurSel = pCMB->GetCurSel();
	if(iCurSel>=0)
		pCMB->GetLBText(iCurSel,m_sLsGuiGe);
	else
		return;

	BOLT_INFO boltInfo;
	if(!restore_Ls_guige(m_sLsGuiGe,boltInfo))
	{
		AfxMessageBox("不合理的螺栓规格!");
		UpdateData();
		UpdateData(FALSE);
	}

	if(boltInfo.bolt_d==12)
		m_fHoleD = 13.5;
	else if(boltInfo.bolt_d==16)
		m_fHoleD = 17.5;
	else if(boltInfo.bolt_d==20)
		m_fHoleD = 21.5;
	else if(boltInfo.bolt_d==24)
		m_fHoleD = 25.5;
	else if(boltInfo.bolt_d==27)
		m_fHoleD = 35;
	else if(boltInfo.bolt_d==30)
		m_fHoleD = 40;
	else if(boltInfo.bolt_d==36)
		m_fHoleD = 45;
	else if(boltInfo.bolt_d==42)
		m_fHoleD = 55;
	else if(boltInfo.bolt_d==45)
		m_fHoleD = 60;
	else if(boltInfo.bolt_d==48)
		m_fHoleD = 60;
	else if(boltInfo.bolt_d==52)
		m_fHoleD = 65;
	else if(boltInfo.bolt_d==56)
		m_fHoleD = 70;
	else if(boltInfo.bolt_d==60)
		m_fHoleD = 75;
	else if(boltInfo.bolt_d==64)
		m_fHoleD = 80;
	CString ss;
	ss.Format("%.1f",m_fHoleD);
	GetDlgItem(IDC_E_HOLE_D)->SetWindowText(ss);
}

void CBoltHolePropDlg::OnOK() 
{
	CDialog::OnOK();
	OnKillfocusEX();
	if(m_pLs)
		m_pLs->dwRayNo=m_dwRayNo;
}

void CBoltHolePropDlg::OnChangeEY() 
{
	CString ss;
	GetDlgItem(IDC_E_Y)->GetWindowText(ss);
	int nStrLen=ss.GetLength();
	if((nStrLen==1&&ss[0]=='-')||(nStrLen>1&&ss[nStrLen-1]=='.'))
		return;
	UpdateData();
	m_fNormX=0;
	m_fNormY=0;
	m_fNormZ=1;
	UpdateData(FALSE);
}

void CBoltHolePropDlg::OnBnCopyPos() 
{
	UpdateData(TRUE);
	f3dPoint point(m_fPosX,m_fPosY,m_fPosZ);
	WritePointToClip(point);
}

void CBoltHolePropDlg::OnBnCopyNorm() 
{
	UpdateData(TRUE);
	f3dPoint point(m_fNormX,m_fNormY,m_fNormZ);
	WritePointToClip(point);
}

void CBoltHolePropDlg::OnBnPasteNorm() 
{
	f3dPoint point;
	if(ReadClipPoint(point))
	{
		m_fNormX = point.x;
		m_fNormY = point.y;
		m_fNormZ = point.z;
		UpdateData(FALSE);
	}
}

void CBoltHolePropDlg::OnBnPastePos() 
{
	f3dPoint point;
	if(ReadClipPoint(point))
	{
		m_fPosX = point.x;
		m_fPosY = point.y;
		m_fPosZ = point.z;
		UpdateData(FALSE);
	}
}

void CBoltHolePropDlg::OnBnLsRayNo() 
{
	CCfgPartNoDlg dlg;
	dlg.cfg_style=CFG_LSRAY_NO;
	dlg.cfgword.flag.word[0]=m_dwRayNo;
	if(dlg.DoModal()==IDOK)
	{
		m_dwRayNo=dlg.cfgword.flag.word[0];
		UpdateCfgPartNoString();
	}
}
void CBoltHolePropDlg::UpdateCfgPartNoString()
{
	m_sRayNo.Empty();
	for(int i=0;i<32;i++)
	{
		if(m_dwRayNo&GetSingleWord(i+1))
		{
			CString sText;
			sText.Format("%d",i+1);
			if(m_sRayNo.GetLength()<=0)
				m_sRayNo=sText;
			else
				m_sRayNo+=","+sText;
		}
	}
	UpdateData(FALSE);
}

void CBoltHolePropDlg::OnKillfocusEX() 
{
	UpdateData();
	
	UpdateData(FALSE);
}

//修改腰圆孔方向即螺栓X轴方向
void CBoltHolePropDlg::OnModifyWaistVec() 
{
	UpdateData();
	CReportVectorDlg dlg;
	dlg.m_sCaption="修改腰圆孔方向";
	dlg.m_fVectorX = waist_vec.x;
	dlg.m_fVectorY = waist_vec.y;
	dlg.m_fVectorZ = waist_vec.z;

	if(dlg.DoModal()==IDOK)
	{
		waist_vec.x = dlg.m_fVectorX;
		waist_vec.y = dlg.m_fVectorY;
		waist_vec.z = dlg.m_fVectorZ;
	}
	UpdateData(FALSE);
}
#endif