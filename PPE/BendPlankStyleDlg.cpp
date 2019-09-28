// BendPlankStyleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "BendPlankStyleDlg.h"
#include "CopyPasteOper.h "


// CBendPlankStyleDlg 对话框

IMPLEMENT_DYNAMIC(CBendPlankStyleDlg, CDialog)

CBendPlankStyleDlg::CBendPlankStyleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBendPlankStyleDlg::IDD, pParent)
{
	m_fBendAngle = 0.0;
	m_fNormX = 0.0;
	m_fNormY = 0.0;
	m_fNormZ = 0.0;
	m_iBendFaceStyle = 0;
}

CBendPlankStyleDlg::~CBendPlankStyleDlg()
{
}

void CBendPlankStyleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_BEND_ANGLE, m_fBendAngle);
	DDX_Text(pDX, IDC_E_NORM_X, m_fNormX);
	DDX_Text(pDX, IDC_E_NORM_Y, m_fNormY);
	DDX_Text(pDX, IDC_E_NORM_Z, m_fNormZ);
	DDX_Radio(pDX, IDC_RDO_BEND_FACE_STYLE, m_iBendFaceStyle);
}


BEGIN_MESSAGE_MAP(CBendPlankStyleDlg, CDialog)
	ON_BN_CLICKED(IDC_BN_PASTE_NORM, OnBnPasteNorm)
	ON_BN_CLICKED(IDC_RDO_BEND_FACE_STYLE, OnRdoBendFaceStyle)
	ON_BN_CLICKED(IDC_RADIO13, OnRdoBendFaceStyle)
	ON_BN_CLICKED(IDC_RADIO14, OnRdoBendFaceStyle)
END_MESSAGE_MAP()


// CBendPlankStyleDlg 消息处理程序

void CBendPlankStyleDlg::OnBnPasteNorm() 
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

BOOL CBendPlankStyleDlg::OnInitDialog() 
{
	if(m_iBendFaceStyle==0)	//火曲角度确定
	{
		GetDlgItem(IDC_E_BEND_ANGLE)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_X)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_Y)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_Z)->EnableWindow(FALSE);
	}
	else	//法线确定
	{
		GetDlgItem(IDC_E_BEND_ANGLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_X)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_Y)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_Z)->EnableWindow(TRUE);
	}
	GetDlgItem(IDC_RADIO13)->EnableWindow(FALSE);
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CBendPlankStyleDlg::OnRdoBendFaceStyle() 
{
	UpdateData();
	if(m_iBendFaceStyle==0)	//火曲角度确定
	{
		GetDlgItem(IDC_E_BEND_ANGLE)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_X)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_Y)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_Z)->EnableWindow(FALSE);
	}
	else	//法线确定
	{
		GetDlgItem(IDC_E_BEND_ANGLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_NORM_X)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_Y)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_NORM_Z)->EnableWindow(TRUE);
	}

}