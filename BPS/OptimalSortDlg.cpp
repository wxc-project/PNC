// OptimalSortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OptimalSortDlg.h"
#include "CadToolFunc.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"
#include "ParseAdaptNo.h"
#include "SelectJgCardDlg.h"

//解析宽度,厚度字符串
DWORD GetJgInfoHashTblByStr(const char* sValueStr,CHashList<int> &infoHashTbl)
{
	char str[200]="";
	_snprintf(str,199,"%s",sValueStr);
	if(str[0]=='*'||str[0]=='\0')
		infoHashTbl.Empty();
	else
	{
		for(long nNum=FindAdaptNo(str,".,，","-");!IsAdaptNoEnd();nNum=FindAdaptNo(NULL,".,，","-"))
		{
			DWORD dwSegKey=(nNum==0)?-1:nNum;
			infoHashTbl.SetValue(dwSegKey,nNum);
		}
	}
	return infoHashTbl.GetNodeNum();
}
//回调函数处理
static BOOL FireItemChanged(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,NM_LISTVIEW* pNMListView)
{	//选中项发生变化后更新属性栏
	if(pItem->m_idProp==NULL)
		return FALSE;
	CAngleProcessInfo* pJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
	SCOPE_STRU scope=pJgInfo->GetCADEntScope();
	ZoomAcadView(scope,20);
	return TRUE;
}
static int FireCompareItem(const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem1,const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem2,DWORD lPara)
{
	COMPARE_FUNC_EXPARA* pExPara=(COMPARE_FUNC_EXPARA*)lPara;
	int iSubItem=0;
	BOOL bAscending=true;
	if(pExPara)
	{
		iSubItem=pExPara->iSubItem;
		bAscending=pExPara->bAscending;
	}
	CString sText1=pItem1->m_lpNodeInfo->GetSubItemText(iSubItem);
	CString sText2=pItem2->m_lpNodeInfo->GetSubItemText(iSubItem);
	int result=0;
	if(iSubItem==3)
	{
		result=ComparePartNoString(sText1,sText2);
		if(!bAscending)
			result*=-1;
	}
	return result;
}
// COptimalSortDlg 对话框
IMPLEMENT_DYNAMIC(COptimalSortDlg, CDialog)
COptimalSortDlg::COptimalSortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptimalSortDlg::IDD, pParent)
	, m_nRecord(0)
	, m_sCardPath(_T(""))
{
	m_bSelJg=TRUE;
	m_bSelYg=TRUE;
	m_bSelTube=TRUE;
	m_bSelFlat=TRUE;
	m_bSelJig=TRUE;
	m_bSelGgs=TRUE;
	m_bSelQ235=TRUE;
	m_bSelQ345=TRUE;
	m_bSelQ390=TRUE;
	m_bSelQ420=TRUE;
	m_bSelQ460=TRUE;
	m_sWidth="*";
	m_sThick="*";
}

COptimalSortDlg::~COptimalSortDlg()
{
}

void COptimalSortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_JG_LIST, m_xListCtrl);
	DDX_Check(pDX, IDC_CHE_JG, m_bSelJg);
	DDX_Check(pDX, IDC_CHE_YG, m_bSelYg);
	DDX_Check(pDX, IDC_CHE_TUBE, m_bSelTube);
	DDX_Check(pDX, IDC_CHE_FLAT, m_bSelFlat);
	DDX_Check(pDX, IDC_CHE_JIG, m_bSelJig);
	DDX_Check(pDX, IDC_CHE_GGS, m_bSelGgs);
	DDX_Check(pDX, IDC_CHE_Q235, m_bSelQ235);
	DDX_Check(pDX, IDC_CHE_Q345, m_bSelQ345);
	DDX_Check(pDX, IDC_CHE_Q390, m_bSelQ390);
	DDX_Check(pDX, IDC_CHE_Q420, m_bSelQ420);
	DDX_Check(pDX, IDC_CHE_Q460, m_bSelQ460);
	DDX_Text(pDX, IDC_E_WIDTH, m_sWidth);
	DDX_Text(pDX, IDC_E_THICK, m_sThick);
	DDX_Text(pDX, IDC_E_NUM, m_nRecord);
	DDX_Text(pDX, IDC_E_CARD_PATH, m_sCardPath);
}


BEGIN_MESSAGE_MAP(COptimalSortDlg, CDialog)
	ON_BN_CLICKED(IDC_CHE_JG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_YG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_TUBE,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_FLAT,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_JIG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_GGS,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q235, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q345, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q390, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q420, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q460, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_BTN_JG_CARD, OnBnClickedBtnJgCard)
	ON_EN_CHANGE(IDC_E_WIDTH, OnEnChangeEWidth)
	ON_EN_CHANGE(IDC_E_THICK, OnEnChangeEThick)
	ON_BN_CLICKED(IDOK, &COptimalSortDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// COptimalSortDlg 消息处理程序
BOOL COptimalSortDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_xListCtrl.EmptyColumnHeader();
	m_xListCtrl.AddColumnHeader("材料名称",85);
	m_xListCtrl.AddColumnHeader("材质",70);
	m_xListCtrl.AddColumnHeader("规格",65);
	m_xListCtrl.AddColumnHeader("件号",70);
	m_xListCtrl.AddColumnHeader("加工数",75);
	m_xListCtrl.InitListCtrl(NULL,FALSE);
	m_xListCtrl.EnableSortItems(false);
	m_xListCtrl.SetItemChangedFunc(FireItemChanged);
	//
	CXhChar100 APP_PATH,sJgCardPath;
	GetAppPath(APP_PATH);
	if(GetJgCardPath(sJgCardPath)==FALSE)
		sJgCardPath.Printf("%s安微宏源角钢工艺卡.dwg",(char*)APP_PATH);
	m_sCardPath.Format("%s",(char*)sJgCardPath);
	//初始化列表框
	UpdateJgInfoList();
	RefeshListCtrl();
	return TRUE;
}
void COptimalSortDlg::RefeshListCtrl()
{
	m_xListCtrl.DeleteAllItems();
	CSuperGridCtrl::CTreeItem *pItem=NULL;
	for(CAngleProcessInfo* pJgInfo=m_xJgList.GetFirst();pJgInfo;pJgInfo=m_xJgList.GetNext())
	{
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		if(pJgInfo->m_ciType==1)
			lpInfo->SetSubItemText(0,"角钢",TRUE);
		else if(pJgInfo->m_ciType==2)
			lpInfo->SetSubItemText(0,"圆钢",TRUE);
		else if(pJgInfo->m_ciType==3)
			lpInfo->SetSubItemText(0,"钢管",TRUE);
		else if(pJgInfo->m_ciType==4)
			lpInfo->SetSubItemText(0,"扁铁",TRUE);
		else if(pJgInfo->m_ciType==5)
			lpInfo->SetSubItemText(0,"夹具",TRUE);
		else
			lpInfo->SetSubItemText(0,"钢格栅",TRUE);
		CXhChar16 sMat=CBPSModel::QuerySteelMatMark(pJgInfo->m_cMaterial);
		lpInfo->SetSubItemText(1,sMat,TRUE);//材质	
		lpInfo->SetSubItemText(2,pJgInfo->m_sSpec,TRUE);		//规格
		lpInfo->SetSubItemText(3,pJgInfo->m_sPartNo,TRUE);		//件号
		lpInfo->SetSubItemText(4,CXhChar50("%d",pJgInfo->m_nSumNum),TRUE);	//件数
		pItem=m_xListCtrl.InsertRootItem(lpInfo);
		pItem->m_idProp=(long)pJgInfo;
	}
	UpdateData(FALSE);
}
static int CompareFun1(const CAngleProcessInfo& jginfo1,const CAngleProcessInfo& jginfo2)
{
	//首先根据材料名称进行排序
	if(jginfo1.m_ciType>jginfo2.m_ciType)
		return 1;
	else if(jginfo1.m_ciType<jginfo2.m_ciType)
		return -1;
	//然后根据材质进行排序
	int iMatMark1=CBPSModel::QuerySteelMatIndex(jginfo1.m_cMaterial);
	int iMatMark2=CBPSModel::QuerySteelMatIndex(jginfo2.m_cMaterial);
	if(iMatMark1>iMatMark2)
		return 1;
	else if(iMatMark1<iMatMark2)
		return -1;
	//最后根据材质
	if(jginfo1.m_ciType==1)
	{
		if(jginfo1.m_nWidth>jginfo2.m_nWidth)
			return 1;
		else if(jginfo1.m_nWidth<jginfo2.m_nWidth)
			return -1;
		if(jginfo1.m_nThick>jginfo2.m_nThick)
			return 1;
		else if(jginfo1.m_nThick<jginfo2.m_nThick)
			return -1;
	}
	return ComparePartNoString(jginfo1.m_sPartNo,jginfo2.m_sPartNo);
}
void COptimalSortDlg::UpdateJgInfoList()
{
	m_xJgList.Empty();
	m_nRecord=0;
	for(CAngleProcessInfo* pJgInfo=BPSModel.EnumFirstJg();pJgInfo;pJgInfo=BPSModel.EnumNextJg())
	{
		if(!m_bSelJg && pJgInfo->m_ciType==CAngleProcessInfo::TYPE_JG)
			continue;
		if(!m_bSelYg && pJgInfo->m_ciType==CAngleProcessInfo::TYPE_YG)
			continue;
		if(!m_bSelTube && pJgInfo->m_ciType==CAngleProcessInfo::TYPE_TUBE)
			continue;
		if(!m_bSelFlat&&pJgInfo->m_ciType==CAngleProcessInfo::TYPE_FLAT)
			continue;
		if(!m_bSelJig&&pJgInfo->m_ciType==CAngleProcessInfo::TYPE_JIG)
			continue;
		if(!m_bSelGgs&&pJgInfo->m_ciType==CAngleProcessInfo::TYPE_GGS)
			continue;
		if(!m_bSelQ235 && pJgInfo->m_cMaterial=='S')
			continue;
		if(!m_bSelQ345 && pJgInfo->m_cMaterial=='H')
			continue;
		if(!m_bSelQ390 && pJgInfo->m_cMaterial=='G')
			continue;
		if(!m_bSelQ420 && pJgInfo->m_cMaterial=='P')
			continue;
		if(!m_bSelQ460 && pJgInfo->m_cMaterial=='T')
			continue;
		if(m_thickHashTbl.GetNodeNum()>0&&m_thickHashTbl.GetValue(pJgInfo->m_nThick)==NULL)
			continue;
		if(m_widthHashTbl.GetNodeNum()>0&&m_widthHashTbl.GetValue(pJgInfo->m_nWidth)==NULL)
			continue;
		m_nRecord++;
		CAngleProcessInfo* pSelJg=m_xJgList.append();
		pSelJg->CopyProperty(pJgInfo);
	}
	//按照材料名称-材质-规格的顺序进行排序
	if(m_xJgList.GetSize()>0)
		CQuickSort<CAngleProcessInfo>::QuickSort(m_xJgList.m_pData,m_xJgList.GetSize(),CompareFun1);
}
void COptimalSortDlg::OnUpdateJgInfo()
{
	UpdateData();
	UpdateJgInfoList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnEnChangeEThick()
{
	UpdateData();
	m_thickHashTbl.Empty();
	GetJgInfoHashTblByStr(m_sThick,m_thickHashTbl);
	UpdateJgInfoList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnEnChangeEWidth()
{
	UpdateData();
	m_widthHashTbl.Empty();
	GetJgInfoHashTblByStr(m_sWidth,m_widthHashTbl);
	UpdateJgInfoList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnBnClickedBtnJgCard()
{
	CSelectJgCardDlg dlg;
	if(dlg.DoModal()==IDOK)
		CIdentifyManager::InitJgCardInfo(dlg.m_sJgCardPath);
	UpdateData(FALSE);
}
void COptimalSortDlg::OnOK()
{
	m_xPrintScopyList.Empty();
	if(m_xListCtrl.GetSelectedCount()>1)
	{
		POSITION pos = m_xListCtrl.GetFirstSelectedItemPosition();
		while(pos!=NULL)
		{
			int i=m_xListCtrl.GetNextSelectedItem(pos);
			CSuperGridCtrl::CTreeItem* pItem=m_xListCtrl.GetTreeItem(i);		
			CAngleProcessInfo* pSelJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
			m_xPrintScopyList.append(pSelJgInfo->GetCADEntScope());
		}
	}
	else
	{
		for(CAngleProcessInfo* pAngle=m_xJgList.GetFirst();pAngle;pAngle=m_xJgList.GetNext())
			m_xPrintScopyList.append(pAngle->GetCADEntScope());
	}
	return CDialog::OnOK();
}


void COptimalSortDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialog::OnOK();
}
