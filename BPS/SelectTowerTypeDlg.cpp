// SelectTowerTypeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SelectTowerTypeDlg.h"
#include "afxdialogex.h"
#include "TMS.h"
#include "ServerTowerType.h"
#include "BPSModel.h"

// CSelectTowerTypeDlg 对话框

IMPLEMENT_DYNAMIC(CSelectTowerTypeDlg, CDialogEx)

CSelectTowerTypeDlg::CSelectTowerTypeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSelectTowerTypeDlg::IDD, pParent)
{
	m_pTowerType=NULL;
}

CSelectTowerTypeDlg::~CSelectTowerTypeDlg()
{
}

void CSelectTowerTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_PROJECT_NAME, m_cmbProject);
	DDX_Control(pDX, IDC_CMB_TOWERTYPE_NAME, m_cmbTowerType);
}


BEGIN_MESSAGE_MAP(CSelectTowerTypeDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_CMB_PROJECT_NAME, &CSelectTowerTypeDlg::OnCbnSelchangeCmbProjectName)
END_MESSAGE_MAP()


// CSelectTowerTypeDlg 消息处理程序

void CSelectTowerTypeDlg::InitProjectTowerTypes()
{	
	//if(AgentServer.hashTowerTypes.GetNodeNum()<=0)
	{	//加载在放样塔型列表
		CBuffer buffer;
		if(!TMS.QueryStateTowerTypes(&buffer,0,0,0,CServerTowerType::SERIALIZE_TMA,false))
		{
			AfxMessageBox("数据加载失败！");
			return;
		}
		buffer.SeekToBegin();
		AgentServer.ParseAndUpdateTowerTypesFromBuffer(buffer,1,CServerTowerType::SERIALIZE_TMA);
	}
	CXhChar100 sTowerType;
	for(CAngleProcessInfo *pAngleInfo=BPSModel.EnumFirstJg();pAngleInfo;pAngleInfo=BPSModel.EnumNextJg())
	{
		if(pAngleInfo->m_sTowerType.GetLength()>0)
		{
			sTowerType.Copy(pAngleInfo->m_sTowerType);
			break;
		}
	}
	//
	hashProjectById.Empty();
	CServerTowerType *pInitTowerType=NULL;
	for(CServerTowerType *pTowerType=AgentServer.hashTowerTypes.GetFirst();pTowerType;pTowerType=AgentServer.hashTowerTypes.GetNext())
	{
		int projectKey=pTowerType->m_idBelongProject<=0?-1:pTowerType->m_idBelongProject;
		CServerProject *pProject=hashProjectById.GetValue(projectKey);
		if(pProject==NULL)
		{
			pProject=hashProjectById.Add(projectKey);
			pProject->id=pTowerType->m_idBelongProject;
			pProject->m_sProjectName.Copy(pTowerType->m_sProjName);
		}
		pProject->towerTypeList.append(pTowerType);
		if(sTowerType.GetLength()>0&&pTowerType->m_sName.Copy(sTowerType))
			pInitTowerType=pTowerType;
		if(BPSModel.m_idTowerType==pTowerType->Id)
			m_pTowerType=pTowerType;
	}
	if(m_pTowerType==NULL)
		m_pTowerType=pInitTowerType;
}

BOOL CSelectTowerTypeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	InitProjectTowerTypes();
	//
	while(m_cmbProject.GetCount()>0)
		m_cmbProject.DeleteString(0);
	int iCurSel=0;
	for(CServerProject *pProject=hashProjectById.GetFirst();pProject;pProject=hashProjectById.GetNext())
	{
		int iItem=m_cmbProject.AddString(CXhChar500("%s(id:%d)",(char*)pProject->m_sProjectName,pProject->id));
		m_cmbProject.SetItemData(iItem,(DWORD)pProject);
		if(m_pTowerType&&m_pTowerType->m_idBelongProject==pProject->id)
			iCurSel=iItem;
	}
	if(m_cmbProject.GetCount()>0)
		m_cmbProject.SetCurSel(iCurSel);
	OnCbnSelchangeCmbProjectName();
	return TRUE;
}


void CSelectTowerTypeDlg::OnCbnSelchangeCmbProjectName()
{
	int iCurPrjSel=m_cmbProject.GetCurSel();
	if(iCurPrjSel<0)
		return;
	CServerProject *pProject=(CServerProject*)m_cmbProject.GetItemData(iCurPrjSel);
	if(pProject==NULL)
		return;
	while(m_cmbTowerType.GetCount()>0)
		m_cmbTowerType.DeleteString(0);
	int iCurSel=0;
	for(CServerTowerType **ppTowerType=pProject->towerTypeList.GetFirst();ppTowerType;ppTowerType=pProject->towerTypeList.GetNext())
	{
		CServerTowerType *pTowerType=*ppTowerType;
		int iItem=m_cmbTowerType.AddString(CXhChar500("%s(id:%d)",(char*)pTowerType->m_sName,pTowerType->Id));
		m_cmbTowerType.SetItemData(iItem,(DWORD)pTowerType);
		if(pTowerType==m_pTowerType)
			iCurSel=iItem;
	}
	if(m_cmbTowerType.GetCount()>0)
		m_cmbTowerType.SetCurSel(iCurSel);
}

void CSelectTowerTypeDlg::OnOK()
{
	int iCurSel=m_cmbTowerType.GetCurSel();
	if(iCurSel>=0)
	{
		m_pTowerType=(CServerTowerType*)m_cmbTowerType.GetItemData(iCurSel);
		BPSModel.m_idProject=m_pTowerType->m_idBelongProject;
		BPSModel.m_idTowerType=m_pTowerType->Id;
	}
	else
		m_pTowerType=NULL;
	CDialogEx::OnOK();
}
