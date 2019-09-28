// DefProfileVertexDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DefProfileVertexDlg.h"
#include "f_ent.h"
#include "f_alg_fun.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefProfileVertexDlg dialog

CDefProfileVertexDlg::CDefProfileVertexDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDefProfileVertexDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDefProfileVertexDlg)
	m_iDefPointType = 0;
	m_fPosX = 0.0;
	m_fPosY = 0.0;
	//}}AFX_DATA_INIT
}


void CDefProfileVertexDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefProfileVertexDlg)
	DDX_CBIndex(pDX, IDC_CMB_DEF_TYPE, m_iDefPointType);
	DDX_Text(pDX, IDC_E_PT_X, m_fPosX);
	DDX_Text(pDX, IDC_E_PT_Y, m_fPosY);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefProfileVertexDlg, CDialog)
	//{{AFX_MSG_MAP(CDefProfileVertexDlg)
	ON_CBN_SELCHANGE(IDC_CMB_DEF_TYPE, OnSelchangeCmbDefType)
	ON_EN_CHANGE(IDC_E_PT_X, OnChangeEPtX)
	ON_EN_CHANGE(IDC_E_PT_Y, OnChangeEPtY)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefProfileVertexDlg message handlers

BOOL CDefProfileVertexDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	OnSelchangeCmbDefType();
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDefProfileVertexDlg::OnSelchangeCmbDefType() 
{
	UpdateData();
	f2dLine line;
	if(m_iDefPointType==1)
	{	//指定X值
		OnChangeEPtX();
	}
	else if(m_iDefPointType==2)
	{	//指定Y值
		OnChangeEPtY();
	}
	//else	//指定坐标
	UpdateData(FALSE);
}

void CDefProfileVertexDlg::OnChangeEPtX() 
{
	f2dLine line;
	if(m_iDefPointType==1)
	{	//指定X值
		CString sValue;
		GetDlgItem(IDC_E_PT_X)->GetWindowText(sValue);
		if(sValue.CompareNoCase("-")==0)
			return;
		UpdateData();
		line.startPt.Set(m_fPosX,0);
		line.endPt.Set(m_fPosX,100);
		if(datum_line.SectorAngle()>0)
		{
			f2dArc arc;
			arc.angle=datum_line.SectorAngle();
			if(datum_line.WorkNorm().z>0)
				arc.startPt = datum_line.Start();
			else
				arc.startPt = datum_line.End();
			arc.centre = datum_line.Center();
			f3dPoint pos1,pos2;
			line.startPt.y = arc.centre.y-2*arc.Radius();
			line.endPt.y = arc.centre.y+2*arc.Radius();
			if(Int2dla(line,arc,pos1.x,pos1.y,pos2.x,pos2.y)==2)
			{
				if(DISTANCE(pickPoint,pos1)<DISTANCE(pickPoint,pos2))
				{
					m_fPosX = pos1.x;
					m_fPosY = pos1.y;
				}
				else
				{
					m_fPosX = pos2.x;
					m_fPosY = pos2.y;
				}
			}
			else
			{
				m_fPosX = pos1.x;
				m_fPosY = pos1.y;
			}
		}
		else
		{
			f2dLine tmLine(f3dPoint(datum_line.Start()),f3dPoint(datum_line.End()));
			int nRetCode=Int2dll(line,tmLine,m_fPosX,m_fPosY);
			if(nRetCode==0)
				m_fPosY=datum_line.Start().y;
			else if(nRetCode<0)
				m_fPosY=0;
		}
		CEdit *pEdit=(CEdit*)GetDlgItem(IDC_E_PT_Y);
		CString ss;
		ss.Format("%f",m_fPosY);
		pEdit->SetWindowText(ss);
	}
}

void CDefProfileVertexDlg::OnChangeEPtY() 
{
	f2dLine line;
	if(m_iDefPointType==2)
	{	//指定Y值
		CString sValue;
		GetDlgItem(IDC_E_PT_Y)->GetWindowText(sValue);
		if(sValue.CompareNoCase("-")==0)
			return;
		UpdateData();
		line.startPt.Set(0,m_fPosY);
		line.endPt.Set(100,m_fPosY);
		if(datum_line.SectorAngle()>0)
		{
			f2dArc arc;
			arc.angle=datum_line.SectorAngle();
			if(datum_line.WorkNorm().z>0)
				arc.startPt = datum_line.Start();
			else
				arc.startPt = datum_line.End();
			arc.centre = datum_line.Center();
			f3dPoint pos1,pos2;
			line.startPt.x = arc.centre.x-2*arc.Radius();
			line.endPt.x = arc.centre.x+2*arc.Radius();
			if(Int2dla(line,arc,pos1.x,pos1.y,pos2.x,pos2.y)==2)
			{
				if(DISTANCE(pickPoint,pos1)<DISTANCE(pickPoint,pos2))
				{
					m_fPosX = pos1.x;
					m_fPosY = pos1.y;
				}
				else
				{
					m_fPosX = pos2.x;
					m_fPosY = pos2.y;
				}
			}
			else
			{
				m_fPosX = pos1.x;
				m_fPosY = pos1.y;
			}
		}
		else
		{
			f2dLine tmLine(f3dPoint(datum_line.Start()),f3dPoint(datum_line.End()));
			int nRetCode=Int2dll(line,tmLine,m_fPosX,m_fPosY);
			if(nRetCode==0)
				m_fPosX=datum_line.Start().x;
			else if(nRetCode<0)
				m_fPosX=0;
		}
		CEdit *pEdit=(CEdit*)GetDlgItem(IDC_E_PT_X);
		CString ss;
		ss.Format("%f",m_fPosX);
		pEdit->SetWindowText(ss);
	}
}
