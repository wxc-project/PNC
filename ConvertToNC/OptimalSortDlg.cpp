// OptimalSortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OptimalSortDlg.h"
#include "CadToolFunc.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"
#include "ParseAdaptNo.h"
//#include "SelectJgCardDlg.h"

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
	m_bSelPlate=TRUE;
	m_bSelYg=TRUE;
	m_bSelTube=TRUE;
	m_bSelFlat=TRUE;
	m_bSelJig=TRUE;
	m_bSelGgs=TRUE;
	m_bSelQ235=TRUE;
	m_bSelQ345=TRUE;
	m_bSelQ355=TRUE;
	m_bSelQ390=TRUE;
	m_bSelQ420=TRUE;
	m_bSelQ460=TRUE;
	m_sWidth="*";
	m_sThick="*";
	m_pDwgFile = NULL;
}

COptimalSortDlg::~COptimalSortDlg()
{
}

void COptimalSortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_JG_LIST, m_xListCtrl);
	DDX_Check(pDX, IDC_CHE_JG, m_bSelJg);
	DDX_Check(pDX, IDC_CHE_PLATE, m_bSelPlate);
	DDX_Check(pDX, IDC_CHE_YG, m_bSelYg);
	DDX_Check(pDX, IDC_CHE_TUBE, m_bSelTube);
	DDX_Check(pDX, IDC_CHE_FLAT, m_bSelFlat);
	DDX_Check(pDX, IDC_CHE_JIG, m_bSelJig);
	DDX_Check(pDX, IDC_CHE_GGS, m_bSelGgs);
	DDX_Check(pDX, IDC_CHE_Q235, m_bSelQ235);
	DDX_Check(pDX, IDC_CHE_Q345, m_bSelQ345);
	DDX_Check(pDX, IDC_CHE_Q355, m_bSelQ355);
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
	for(CAngleProcessInfo** ppJgInfo=m_xJgList.GetFirst();ppJgInfo;ppJgInfo=m_xJgList.GetNext())
	{
		CAngleProcessInfo *pJgInfo = *ppJgInfo;
		if (pJgInfo == NULL)
			continue;
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
		CXhChar16 sMat= pJgInfo->m_xAngle.sMaterial;
		lpInfo->SetSubItemText(1,sMat,TRUE);//材质	
		lpInfo->SetSubItemText(2,pJgInfo->m_xAngle.GetSpec(),TRUE);			//规格
		lpInfo->SetSubItemText(3,pJgInfo->m_xAngle.GetPartNo(),TRUE);		//件号
		lpInfo->SetSubItemText(4,CXhChar50("%d",pJgInfo->m_xAngle.GetPartNum()),TRUE);	//件数
		pItem=m_xListCtrl.InsertRootItem(lpInfo);
		pItem->m_idProp=(long)pJgInfo;
	}
	for (CPlateProcessInfo** ppPlateInfo = m_xPlateList.GetFirst(); ppPlateInfo; ppPlateInfo = m_xPlateList.GetNext())
	{
		CPlateProcessInfo *pPlateInfo = *ppPlateInfo;
		if (pPlateInfo == NULL)
			continue;
		CListCtrlItemInfo *lpInfo = new CListCtrlItemInfo();
		lpInfo->SetSubItemText(0, "钢板", TRUE);
		CXhChar16 sMat = pPlateInfo->xBomPlate.sMaterial;
		lpInfo->SetSubItemText(1, sMat, TRUE);//材质	
		lpInfo->SetSubItemText(2, pPlateInfo->xBomPlate.GetSpec(), TRUE);			//规格
		lpInfo->SetSubItemText(3, pPlateInfo->xBomPlate.GetPartNo(), TRUE);		//件号
		lpInfo->SetSubItemText(4, CXhChar50("%d", pPlateInfo->xBomPlate.GetPartNum()), TRUE);	//件数
		pItem = m_xListCtrl.InsertRootItem(lpInfo);
		pItem->m_idProp = (long)pPlateInfo;
	}
	UpdateData(FALSE);
}
typedef CAngleProcessInfo* AngleInfoPtr;
static int CompareJgPtrFun(const AngleInfoPtr& jginfo1,const AngleInfoPtr& jginfo2)
{
	//首先根据材料名称进行排序
	if(jginfo1->m_ciType>jginfo2->m_ciType)
		return 1;
	else if(jginfo1->m_ciType<jginfo2->m_ciType)
		return -1;
	//然后根据材质进行排序
	int iMatMark1=CProcessPart::QuerySteelMatIndex(jginfo1->m_xAngle.cMaterial);
	int iMatMark2= CProcessPart::QuerySteelMatIndex(jginfo2->m_xAngle.cMaterial);
	if(iMatMark1>iMatMark2)
		return 1;
	else if(iMatMark1<iMatMark2)
		return -1;
	//最后根据材质
	if(jginfo1->m_ciType==1)
	{
		if(jginfo1->m_xAngle.wide> jginfo2->m_xAngle.wide)
			return 1;
		else if(jginfo1->m_xAngle.wide < jginfo2->m_xAngle.wide)
			return -1;
		if (jginfo1->m_xAngle.thick > jginfo2->m_xAngle.thick)
			return 1;
		else if(jginfo1->m_xAngle.thick < jginfo2->m_xAngle.thick)
			return -1;
	}
	CXhChar16 sPartNo1 = jginfo1->m_xAngle.sPartNo;
	CXhChar16 sPartNo2 = jginfo2->m_xAngle.sPartNo;
	return ComparePartNoString(sPartNo1,sPartNo2);
}

typedef CPlateProcessInfo* PlateInfoPtr;
static int ComparePlatePtrFun(const PlateInfoPtr& platePtr1, const PlateInfoPtr& platePtr2)
{
	//然后根据材质进行排序
	int iMatMark1 = CProcessPart::QuerySteelMatIndex(platePtr1->xBomPlate.cMaterial);
	int iMatMark2 = CProcessPart::QuerySteelMatIndex(platePtr2->xBomPlate.cMaterial);
	if (iMatMark1 > iMatMark2)
		return 1;
	else if (iMatMark1 < iMatMark2)
		return -1;
	//最后根据材质
	if (platePtr1->xBomPlate.wide > platePtr2->xBomPlate.wide)
		return 1;
	else if (platePtr1->xBomPlate.wide < platePtr2->xBomPlate.wide)
		return -1;
	if (platePtr1->xBomPlate.thick > platePtr2->xBomPlate.thick)
		return 1;
	else if (platePtr1->xBomPlate.thick < platePtr2->xBomPlate.thick)
		return -1;
	CXhChar16 sPartNo1 = platePtr1->xBomPlate.sPartNo;
	CXhChar16 sPartNo2 = platePtr2->xBomPlate.sPartNo;
	return ComparePartNoString(sPartNo1, sPartNo2);
}

void COptimalSortDlg::UpdateJgInfoList()
{
	m_xJgList.Empty();
	m_nRecord=0;
	for(CAngleProcessInfo* pJgInfo= m_pDwgFile->EnumFirstJg();pJgInfo;pJgInfo= m_pDwgFile->EnumNextJg())
	{
		if(!m_bSelJg && pJgInfo->m_ciType==CAngleProcessInfo::TYPE_JG)
			continue;
		if(!m_bSelPlate && pJgInfo->m_ciType==CAngleProcessInfo::TYPE_PLATE)
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
		if(!m_bSelQ235 && pJgInfo->m_xAngle.cMaterial=='S')
			continue;
		if(!m_bSelQ345 && pJgInfo->m_xAngle.cMaterial=='H')
			continue;
		if(!m_bSelQ355 && pJgInfo->m_xAngle.cMaterial=='h')
			continue;
		if(!m_bSelQ390 && pJgInfo->m_xAngle.cMaterial=='G')
			continue;
		if(!m_bSelQ420 && pJgInfo->m_xAngle.cMaterial=='P')
			continue;
		if(!m_bSelQ460 && pJgInfo->m_xAngle.cMaterial=='T')
			continue;
		if(m_thickHashTbl.GetNodeNum()>0&&m_thickHashTbl.GetValue((int)pJgInfo->m_xAngle.thick)==NULL)
			continue;
		if(m_widthHashTbl.GetNodeNum()>0&&m_widthHashTbl.GetValue((int)pJgInfo->m_xAngle.wide)==NULL)
			continue;
		m_nRecord++;
		m_xJgList.append(pJgInfo);
	}
	//按照材料名称-材质-规格的顺序进行排序
	if(m_xJgList.GetSize()>0)
		CQuickSort<CAngleProcessInfo*>::QuickSort(m_xJgList.m_pData,m_xJgList.GetSize(),CompareJgPtrFun);
	//
	if (m_bSelPlate)
	{
		for (CPlateProcessInfo* pPlateInfo = m_pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = m_pDwgFile->EnumNextPlate())
		{
			if (!m_bSelQ235 && pPlateInfo->xBomPlate.cMaterial == 'S')
				continue;
			if (!m_bSelQ345 && pPlateInfo->xBomPlate.cMaterial == 'H')
				continue;
			if (!m_bSelQ355 && pPlateInfo->xBomPlate.cMaterial == 'h')
				continue;
			if (!m_bSelQ390 && pPlateInfo->xBomPlate.cMaterial == 'G')
				continue;
			if (!m_bSelQ420 && pPlateInfo->xBomPlate.cMaterial == 'P')
				continue;
			if (!m_bSelQ460 && pPlateInfo->xBomPlate.cMaterial == 'T')
				continue;
			if (m_thickHashTbl.GetNodeNum() > 0 && m_thickHashTbl.GetValue((int)pPlateInfo->xBomPlate.thick) == NULL)
				continue;
			m_nRecord++;
			m_xPlateList.append(pPlateInfo);
		}
		//按照材料名称-材质-规格的顺序进行排序
		if (m_xPlateList.GetSize() > 0)
			CQuickSort<CPlateProcessInfo*>::QuickSort(m_xPlateList.m_pData, m_xPlateList.GetSize(), ComparePlatePtrFun);
	}
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
	/*CSelectJgCardDlg dlg;
	if(dlg.DoModal()==IDOK)
		CIdentifyManager::InitJgCardInfo(dlg.m_sJgCardPath);
	UpdateData(FALSE);*/
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
			CString sTypeName = pItem->m_lpNodeInfo->GetSubItemText(0);
			if (sTypeName.CompareNoCase("钢板") == 0)
			{
				CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pItem->m_idProp;
				m_xPrintScopyList.append(pSelPlateInfo->GetCADEntScope());
			}
			else
			{
				CAngleProcessInfo* pSelJgInfo = (CAngleProcessInfo*)pItem->m_idProp;
				m_xPrintScopyList.append(pSelJgInfo->GetCADEntScope());
			}
		}
	}
	else
	{
		for (CAngleProcessInfo** ppAngle = m_xJgList.GetFirst(); ppAngle; ppAngle = m_xJgList.GetNext())
		{
			if (ppAngle == NULL || *ppAngle == NULL)
				continue;
			m_xPrintScopyList.append((*ppAngle)->GetCADEntScope());
		}
		for (CPlateProcessInfo** ppPlate = m_xPlateList.GetFirst(); ppPlate; ppPlate = m_xPlateList.GetNext())
		{
			if (ppPlate == NULL || *ppPlate == NULL)
				continue;
			m_xPrintScopyList.append((*ppPlate)->GetCADEntScope());
		}
	}
	return CDialog::OnOK();
}


void COptimalSortDlg::OnBnClickedOk()
{
	CDialog::OnOK();
}
