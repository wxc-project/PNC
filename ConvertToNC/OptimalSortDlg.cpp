// OptimalSortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OptimalSortDlg.h"
#include "CadToolFunc.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"
#include "ParseAdaptNo.h"
#include "BatchPrint.h"
#include "ExcelOper.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)

typedef CAngleProcessInfo* AngleInfoPtr;
typedef BOMPART* PART_PTR;
static CXhChar100 VariantToString(VARIANT value)
{
	CXhChar100 sValue;
	if (value.vt == VT_BSTR)
		return sValue.Copy(CString(value.bstrVal));
	else if (value.vt == VT_R8)
		return sValue.Copy(CXhChar100(value.dblVal));
	else if (value.vt == VT_R4)
		return sValue.Copy(CXhChar100(value.fltVal));
	else if (value.vt == VT_INT)
		return sValue.Copy(CXhChar100(value.intVal));
	else
		return sValue;
}
static int CompareBomPartPtrFunc(const PART_PTR& partPtr1, const PART_PTR& partPtr2, BOOL bAscending)
{
	int nRetCode = 0;
	//然后根据材质进行排序(从大到小)
	int iMatMark1 = CProcessPart::QuerySteelMatIndex(partPtr1->cMaterial);
	int iMatMark2 = CProcessPart::QuerySteelMatIndex(partPtr2->cMaterial);
	if (iMatMark1 > iMatMark2)
		nRetCode = 1;
	else if (iMatMark1 < iMatMark2)
		nRetCode = -1;
	if (nRetCode == 0)
	{	//规格从大到小
		if (partPtr1->wide > partPtr2->wide)
			nRetCode = 1;
		else if (partPtr1->wide < partPtr2->wide)
			nRetCode = -1;
		else if (partPtr1->thick > partPtr2->thick)
			nRetCode = 1;
		else if (partPtr1->thick < partPtr2->thick)
			nRetCode = -1;

		if(nRetCode==0)
		{	//件号从小到大
			CXhChar16 sPartNo1 = partPtr1->sPartNo;
			CXhChar16 sPartNo2 = partPtr2->sPartNo;
			nRetCode = ComparePartNoString(sPartNo1, sPartNo2, "SHhGPT");
		}
		else
		{
			if (!bAscending)	//规格支持从小到大排序或从大到小排序，但材质和件号始终是从小到大排序
				nRetCode *= -1;
		}
	}
	else
	{
		if (!bAscending)	//规格支持从小到大排序或从大到小排序，但材质和件号始终是从小到大排序
			nRetCode *= -1;
	}
	return nRetCode;
}
static int CompareBomPartPtrFunc2(const PART_PTR& partPtr1, const PART_PTR& partPtr2, BOOL bAscending)
{
	int nRetCode = 0;
	if (partPtr1->wide > partPtr2->wide)
		nRetCode = 1;
	else if (partPtr1->wide < partPtr2->wide)
		nRetCode = -1;
	else if (partPtr1->thick > partPtr2->thick)
		nRetCode = 1;
	else if (partPtr1->thick < partPtr2->thick)
		nRetCode = -1;
	if (nRetCode == 0)
	{	//然后根据材质进行排序(从小到大)
		int iMatMark1 = CProcessPart::QuerySteelMatIndex(partPtr1->cMaterial);
		int iMatMark2 = CProcessPart::QuerySteelMatIndex(partPtr2->cMaterial);
		if (iMatMark1 > iMatMark2)
			nRetCode = 1;
		else if (iMatMark1 < iMatMark2)
			nRetCode = -1;
		else
		{	//件号从小到大
			CXhChar16 sPartNo1 = partPtr1->sPartNo;
			CXhChar16 sPartNo2 = partPtr2->sPartNo;
			nRetCode = ComparePartNoString(sPartNo1, sPartNo2, "SHhGPT");
		}
	}
	else
	{
		if (!bAscending)	//规格支持从小到大排序或从大到小排序，但材质和件号始终是从小到大排序
			nRetCode *= -1;
	}
	return nRetCode;
}
static int CompareJgPtrFun(const AngleInfoPtr& jginfo1, const AngleInfoPtr& jginfo2)
{
	//首先根据材料名称进行排序
	if (jginfo1->m_ciType > jginfo2->m_ciType)
		return 1;
	else if (jginfo1->m_ciType < jginfo2->m_ciType)
		return -1;
	//根据规格排序(从大到小)
	return CompareBomPartPtrFunc(&jginfo1->m_xAngle, &jginfo2->m_xAngle,FALSE);
}

typedef CPlateProcessInfo* PlateInfoPtr;
static int ComparePlatePtrFun(const PlateInfoPtr& platePtr1, const PlateInfoPtr& platePtr2)
{	//根据规格排序(从大到小)
	return CompareBomPartPtrFunc(&platePtr1->xBomPlate, &platePtr2->xBomPlate,FALSE);
}

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
	if (iSubItem == 1 || iSubItem == 2)
	{	//点击规格、材质列，按规格、材质、件号
		CString sPartType1 = pItem1->m_lpNodeInfo->GetSubItemText(0);
		CString sPartType2 = pItem2->m_lpNodeInfo->GetSubItemText(0);
		BOMPART *pBomPart1 = NULL, *pBomPart2 = NULL;
		if (sPartType1.CompareNoCase("钢板") == 0)
		{
			CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pItem1->m_idProp;
			pBomPart1 = &pSelPlateInfo->xBomPlate;
		}
		else
		{
			CAngleProcessInfo* pSelJgInfo = (CAngleProcessInfo*)pItem1->m_idProp;
			pBomPart1 = &pSelJgInfo->m_xAngle;
		}
		if (sPartType2.CompareNoCase("钢板") == 0)
		{
			CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pItem2->m_idProp;
			pBomPart2 = &pSelPlateInfo->xBomPlate;
		}
		else
		{
			CAngleProcessInfo* pSelJgInfo = (CAngleProcessInfo*)pItem2->m_idProp;
			pBomPart2 = &pSelJgInfo->m_xAngle;
		}
		if (pBomPart1&&pBomPart2)
		{
			result = CompareBomPartPtrFunc(pBomPart1, pBomPart2, bAscending);
		}
	}
	else if(iSubItem==3)
	{	//按角钢排序
		result=ComparePartNoString(sText1,sText2,"SHhPGT");
		if(!bAscending)
			result*=-1;
	}
	return result;
}
static int FireColmunClick(CSuperGridCtrl* pListCtrl, int iSubItem, bool bAscending)
{
	return 0;
}
// COptimalSortDlg 对话框
IMPLEMENT_DYNAMIC(COptimalSortDlg, CDialog)
COptimalSortDlg::COptimalSortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptimalSortDlg::IDD, pParent)
	, m_nRecord(0)
	, m_bCutAngle(FALSE)
	, m_bKaiHe(FALSE)
	, m_bPushFlat(FALSE)
	, m_bCutRoot(FALSE)
	, m_bCutBer(FALSE)
	, m_bBend(FALSE)
	, m_bCommonAngle(FALSE)
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
	m_bCutAngle = TRUE;
	m_bKaiHe = TRUE;
	m_bPushFlat = TRUE;
	m_bCutRoot = TRUE;
	m_bCutBer = TRUE;
	m_bBend = TRUE;
	m_bCommonAngle = TRUE;
	m_nQ235Count = m_nQ345Count = m_nQ355Count = m_nQ390Count = m_nQ420Count = m_nQ460Count = 0;
	m_nJgCount = m_nPlateCount = m_nYGCount = m_nTubeCount = m_nJiaCount = m_nFlatCount = m_nGgsCount = 0;
	m_nCutAngle = m_nKaiHe = m_nPushFlat = m_nCutRoot = m_nCutBer = m_nBend = m_nCommonAngle = 0;
	m_iPrintType = 0;
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
	DDX_Check(pDX, IDC_CHE_CUT_ANGLE, m_bCutAngle);
	DDX_Check(pDX, IDC_CHE_KAIHE, m_bKaiHe);
	DDX_Check(pDX, IDC_CHE_PUSH_FLAT, m_bPushFlat);
	DDX_Check(pDX, IDC_CHE_CUT_ROOT, m_bCutRoot);
	DDX_Check(pDX, IDC_CHE_CUT_BER, m_bCutBer);
	DDX_Check(pDX, IDC_CHE_BEND, m_bBend);
	DDX_Check(pDX, IDC_CHE_COMMON_ANGLE, m_bCommonAngle);
}


BEGIN_MESSAGE_MAP(COptimalSortDlg, CDialog)
	ON_BN_CLICKED(IDC_CHE_JG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_PLATE,OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_YG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_TUBE,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_FLAT,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_JIG,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_GGS,	OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q235, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q345, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q355, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q390, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q420, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_Q460, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_CUT_ANGLE, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_KAIHE, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_CUT_ROOT, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_CUT_BER, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_PUSH_FLAT, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_BEND, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_COMMON_ANGLE, OnUpdateJgInfo)
	ON_EN_CHANGE(IDC_E_WIDTH, OnEnChangeEWidth)
	ON_EN_CHANGE(IDC_E_THICK, OnEnChangeEThick)
	ON_BN_CLICKED(IDOK, &COptimalSortDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_PDF, &COptimalSortDlg::OnBnClickedBtnPdf)
	ON_BN_CLICKED(IDC_BTN_PNG, &COptimalSortDlg::OnBnClickedBtnPng)
END_MESSAGE_MAP()


// COptimalSortDlg 消息处理程序
BOOL COptimalSortDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_xListCtrl.EmptyColumnHeader();
	m_xListCtrl.AddColumnHeader("材料名称",75);
	m_xListCtrl.AddColumnHeader("材质",55);
	m_xListCtrl.AddColumnHeader("规格",70);
	m_xListCtrl.AddColumnHeader("件号",65);
	m_xListCtrl.AddColumnHeader("加工数",60);
	m_xListCtrl.InitListCtrl(NULL,FALSE);
	m_xListCtrl.EnableSortItems(true);
	m_xListCtrl.SetItemChangedFunc(FireItemChanged);
	m_xListCtrl.SetCompareItemFunc(FireCompareItem);
	//
	GetDlgItem(IDC_CHE_JG)->EnableWindow(m_nJgCount > 0);
	GetDlgItem(IDC_CHE_PLATE)->EnableWindow(m_nPlateCount > 0);
	GetDlgItem(IDC_CHE_TUBE)->EnableWindow(m_nTubeCount > 0);
	GetDlgItem(IDC_CHE_YG)->EnableWindow(m_nYGCount > 0);
	GetDlgItem(IDC_CHE_JIG)->EnableWindow(m_nJiaCount > 0);
	GetDlgItem(IDC_CHE_FLAT)->EnableWindow(m_nFlatCount > 0);
	GetDlgItem(IDC_CHE_GGS)->EnableWindow(m_nGgsCount > 0);
	//
	GetDlgItem(IDC_CHE_Q235)->EnableWindow(m_nQ235Count > 0);
	GetDlgItem(IDC_CHE_Q345)->EnableWindow(m_nQ345Count > 0);
	GetDlgItem(IDC_CHE_Q355)->EnableWindow(m_nQ355Count > 0);
	GetDlgItem(IDC_CHE_Q390)->EnableWindow(m_nQ390Count > 0);
	GetDlgItem(IDC_CHE_Q420)->EnableWindow(m_nQ420Count > 0);
	GetDlgItem(IDC_CHE_Q460)->EnableWindow(m_nQ460Count > 0);

	GetDlgItem(IDC_CHE_CUT_ANGLE)->EnableWindow(m_nCutAngle > 0);
	GetDlgItem(IDC_CHE_KAIHE)->EnableWindow(m_nKaiHe > 0);
	GetDlgItem(IDC_CHE_PUSH_FLAT)->EnableWindow(m_nPushFlat > 0);
	GetDlgItem(IDC_CHE_CUT_ROOT)->EnableWindow(m_nCutRoot > 0);
	GetDlgItem(IDC_CHE_CUT_BER)->EnableWindow(m_nCutBer > 0);
	GetDlgItem(IDC_CHE_BEND)->EnableWindow(m_nBend > 0);
	GetDlgItem(IDC_CHE_COMMON_ANGLE)->EnableWindow(m_bCommonAngle > 0);


	m_bSelJg = m_nJgCount > 0;
	m_bSelPlate = m_nPlateCount > 0;
	m_bSelYg = m_nYGCount > 0;
	m_bSelTube = m_nTubeCount > 0;
	m_bSelFlat = m_nFlatCount > 0;
	m_bSelJig = m_nJiaCount > 0;
	m_bSelGgs = m_nGgsCount > 0;
	m_bSelQ235 = m_nQ235Count > 0;
	m_bSelQ345 = m_nQ345Count > 0;
	m_bSelQ355 = m_nQ355Count > 0;
	m_bSelQ390 = m_nQ390Count > 0;
	m_bSelQ420 = m_nQ420Count > 0;
	m_bSelQ460 = m_nQ460Count > 0;
	m_bCutAngle = m_nCutAngle > 0;
	m_bKaiHe = m_nKaiHe > 0;
	m_bPushFlat = m_nPushFlat > 0;
	m_bCutRoot = m_nCutRoot > 0;
	m_bCutBer = m_nCutBer > 0;
	m_bBend = m_nBend > 0;
	m_bCommonAngle = m_nCommonAngle > 0;
	//初始化列表框
	UpdatePartList();
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
		if(pJgInfo->m_ciType==CAngleProcessInfo::TYPE_JG)
			lpInfo->SetSubItemText(0,"角钢",TRUE);
		else if(pJgInfo->m_ciType== CAngleProcessInfo::TYPE_YG)
			lpInfo->SetSubItemText(0,"圆钢",TRUE);
		else if(pJgInfo->m_ciType== CAngleProcessInfo::TYPE_TUBE)
			lpInfo->SetSubItemText(0,"钢管",TRUE);
		else if(pJgInfo->m_ciType== CAngleProcessInfo::TYPE_FLAT)
			lpInfo->SetSubItemText(0,"扁铁",TRUE);
		else if(pJgInfo->m_ciType==CAngleProcessInfo::TYPE_JIG)
			lpInfo->SetSubItemText(0,"夹具",TRUE);
		else if(pJgInfo->m_ciType==CAngleProcessInfo::TYPE_GGS)
			lpInfo->SetSubItemText(0,"钢格栅",TRUE);
		CXhChar16 sMat= CBomModel::QueryMatMarkIncQuality(&pJgInfo->m_xAngle);
		lpInfo->SetSubItemText(1,sMat,TRUE);//材质	
		lpInfo->SetSubItemText(2,pJgInfo->m_xAngle.GetSpec(),TRUE);			//规格
		lpInfo->SetSubItemText(3,pJgInfo->m_xAngle.GetPartNo(),TRUE);		//件号
		lpInfo->SetSubItemText(4,CXhChar50("%d",pJgInfo->m_xAngle.feature1),TRUE);	//件数
		pItem=m_xListCtrl.InsertRootItem(lpInfo, FALSE);
		pItem->m_idProp=(long)pJgInfo;
	}
	for (CPlateProcessInfo** ppPlateInfo = m_xPlateList.GetFirst(); ppPlateInfo; ppPlateInfo = m_xPlateList.GetNext())
	{
		CPlateProcessInfo *pPlateInfo = *ppPlateInfo;
		if (pPlateInfo == NULL)
			continue;
		CListCtrlItemInfo *lpInfo = new CListCtrlItemInfo();
		lpInfo->SetSubItemText(0, "钢板", TRUE);
		CXhChar16 sMat = CBomModel::QueryMatMarkIncQuality(&pPlateInfo->xBomPlate);
		lpInfo->SetSubItemText(1, sMat, TRUE);	//材质	
		lpInfo->SetSubItemText(2, pPlateInfo->xBomPlate.GetSpec(), TRUE);		//规格
		lpInfo->SetSubItemText(3, pPlateInfo->xBomPlate.GetPartNo(), TRUE);		//件号
		lpInfo->SetSubItemText(4, CXhChar50("%d", pPlateInfo->xBomPlate.feature1), TRUE);	//件数
		pItem = m_xListCtrl.InsertRootItem(lpInfo,FALSE);
		pItem->m_idProp = (long)pPlateInfo;
	}
	m_xListCtrl.Redraw();
	UpdateData(FALSE);
}

static bool IsCommonAngle(CAngleProcessInfo *pJgInfo)
{
	if (pJgInfo == NULL)
		return false;
	if (pJgInfo->m_xAngle.bCutAngle || pJgInfo->m_xAngle.bKaiJiao || pJgInfo->m_xAngle.bHeJiao ||
		pJgInfo->m_xAngle.nPushFlat > 0 || pJgInfo->m_xAngle.bCutRoot || pJgInfo->m_xAngle.bCutBer ||
		pJgInfo->m_xAngle.GetHuoquLineCount() > 0)
		return false;
	else
		return true;
}

void COptimalSortDlg::UpdatePartList()
{
	CLogErrorLife logErrLife;
	m_xJgList.Empty();
	m_xPlateList.Empty();
	m_nRecord=0;
	if (m_pDwgFile == NULL)
		return;
	int nToPrintBom = m_hashPrintBom.GetNodeNum();
	if (nToPrintBom > 0)
	{	//用户提供了打印清单，不需要进行排序
		for(BOMPART* pBom=m_hashPrintBom.GetFirst();pBom;pBom=m_hashPrintBom.GetNext())
		{
			if (pBom->cPartType == BOMPART::ANGLE)
			{
				CAngleProcessInfo* pJgInfo = m_pDwgFile->FindAngleByPartNo(pBom->sPartNo);
				if (pJgInfo)
				{
					m_nRecord++;
					m_xJgList.append(pJgInfo);
				}
				else
					logerr.Log("没有找到件号为{%s}的角钢", (char*)pBom->sPartNo);
			}
			else if (pBom->cPartType == BOMPART::PLATE)
			{
				CPlateProcessInfo* pPlateInfo = m_pDwgFile->FindPlateByPartNo(pBom->sPartNo);
				if (pPlateInfo)
				{
					m_nRecord++;
					m_xPlateList.append(pPlateInfo);
				}
				else
					logerr.Log("没有找到件号为{%s}的钢板", (char*)pBom->sPartNo);
			}
		}
		return;
	}
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
		if (!m_bCutAngle && pJgInfo->m_xAngle.bCutAngle)
			continue;
		if (!m_bCutBer && pJgInfo->m_xAngle.bCutBer)
			continue;
		if (!m_bCutRoot && pJgInfo->m_xAngle.bCutRoot)
			continue;
		if (!m_bKaiHe && (pJgInfo->m_xAngle.bKaiJiao||pJgInfo->m_xAngle.bHeJiao))
			continue;
		if (!m_bPushFlat && pJgInfo->m_xAngle.nPushFlat>0)
			continue;
		if (!m_bBend && pJgInfo->m_xAngle.GetHuoquLineCount()>0)
			continue;
		if (!m_bCommonAngle && IsCommonAngle(pJgInfo))
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
			if (!m_bBend && pPlateInfo->xBomPlate.GetHuoquLineCount() > 0)
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
	UpdatePartList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnEnChangeEThick()
{
	UpdateData();
	m_thickHashTbl.Empty();
	GetJgInfoHashTblByStr(m_sThick,m_thickHashTbl);
	UpdatePartList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnEnChangeEWidth()
{
	UpdateData();
	m_widthHashTbl.Empty();
	GetJgInfoHashTblByStr(m_sWidth,m_widthHashTbl);
	UpdatePartList();
	RefeshListCtrl();
}
void COptimalSortDlg::OnOK()
{
	m_xPrintScopyList.Empty();
	POSITION pos = m_xListCtrl.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int i=m_xListCtrl.GetNextSelectedItem(pos);
		CSuperGridCtrl::CTreeItem* pItem=m_xListCtrl.GetTreeItem(i);
		CString sTypeName = pItem->m_lpNodeInfo->GetSubItemText(0);
		if (sTypeName.CompareNoCase("钢板") == 0)
		{
			CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pItem->m_idProp;
			PRINT_SCOPE *pScope = m_xPrintScopyList.append();
			pScope->Init(pSelPlateInfo);
		}
		else
		{
			CAngleProcessInfo* pSelJgInfo = (CAngleProcessInfo*)pItem->m_idProp;
			PRINT_SCOPE *pScope = m_xPrintScopyList.append();
			pScope->Init(pSelJgInfo);
		}
	}
	if (m_xPrintScopyList.GetNodeNum() == 1)
		m_xPrintScopyList.Empty();	//选中多行时支持打印选中行，选中一行打印所有
	if(m_xPrintScopyList.GetNodeNum()<=0)
	{
		for (CAngleProcessInfo** ppAngle = m_xJgList.GetFirst(); ppAngle; ppAngle = m_xJgList.GetNext())
		{
			if (ppAngle == NULL || *ppAngle == NULL)
				continue;
			PRINT_SCOPE *pScope = m_xPrintScopyList.append();
			pScope->Init(*ppAngle);
		}
		for (CPlateProcessInfo** ppPlate = m_xPlateList.GetFirst(); ppPlate; ppPlate = m_xPlateList.GetNext())
		{
			if (ppPlate == NULL || *ppPlate == NULL)
				continue;
			PRINT_SCOPE *pScope=m_xPrintScopyList.append();
			pScope->Init(*ppPlate);
		}
	}
	return CDialog::OnOK();
}


void COptimalSortDlg::OnBnClickedOk()
{
	m_iPrintType = CBatchPrint::PRINT_TYPE_PAPER;
	OnOK();
	//CDialog::OnOK();
}

void COptimalSortDlg::OnBnClickedBtnPdf()
{
	m_iPrintType = CBatchPrint::PRINT_TYPE_PDF;
	OnOK();
}

void COptimalSortDlg::OnBnClickedBtnPng()
{
	m_iPrintType = CBatchPrint::PRINT_TYPE_PNG;
	OnOK();
}

void COptimalSortDlg::Init(CDwgFileInfo *pDwgFile)
{
	m_pDwgFile = pDwgFile;
	m_nQ235Count = m_nQ345Count = m_nQ355Count = m_nQ390Count = m_nQ420Count = m_nQ460Count = 0;
	m_nJgCount = m_nPlateCount = m_nYGCount = m_nTubeCount = m_nJiaCount = m_nFlatCount = m_nGgsCount = 0;
	m_nCutAngle = m_nKaiHe = m_nPushFlat = m_nCutRoot = m_nCutBer = m_nBend = 0;
	if (m_pDwgFile)
	{
		for (CAngleProcessInfo* pJgInfo = m_pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = m_pDwgFile->EnumNextJg())
		{
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JG)
			{
				m_nJgCount++;
				if (!IsCommonAngle(pJgInfo))
				{
					if (pJgInfo->m_xAngle.bCutAngle)
						m_nCutAngle++;
					if (pJgInfo->m_xAngle.bKaiJiao || pJgInfo->m_xAngle.bHeJiao)
						m_nKaiHe++;
					if (pJgInfo->m_xAngle.nPushFlat > 0)
						m_nPushFlat++;
					if (pJgInfo->m_xAngle.bCutRoot)
						m_nCutRoot++;
					if (pJgInfo->m_xAngle.bCutBer)
						m_nCutBer++;
					if (pJgInfo->m_xAngle.GetHuoquLineCount() > 0)
						m_nBend++;
				}
				else
					m_nCommonAngle++;
			}
			else if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_YG)
				m_nYGCount++;
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_TUBE)
				m_nTubeCount++;
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JIG)
				m_nJiaCount++;
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_FLAT)
				m_nFlatCount++;
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_GGS)
				m_nGgsCount++;
			CXhChar16 sMat = CBomModel::QueryMatMarkIncQuality(&pJgInfo->m_xAngle);
			if (strstr(sMat, "Q235") != NULL)
				m_nQ235Count++;
			else if (strstr(sMat, "Q345") != NULL)
				m_nQ345Count++;
			else if (strstr(sMat, "Q355") != NULL)
				m_nQ355Count++;
			else if (strstr(sMat, "Q390") != NULL)
				m_nQ390Count++;
			else if (strstr(sMat, "Q420") != NULL)
				m_nQ420Count++;
			else if (strstr(sMat, "Q460") != NULL)
				m_nQ460Count++;
		}
		for (CPlateProcessInfo* pPlateInfo = m_pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = m_pDwgFile->EnumNextPlate())
		{
			m_nPlateCount++;
			CXhChar16 sMat = CBomModel::QueryMatMarkIncQuality(&pPlateInfo->xBomPlate);
			if (strstr(sMat, "Q235") != NULL)
				m_nQ235Count++;
			else if (strstr(sMat, "Q345") != NULL)
				m_nQ345Count++;
			else if (strstr(sMat, "Q355") != NULL)
				m_nQ355Count++;
			else if (strstr(sMat, "Q390") != NULL)
				m_nQ390Count++;
			else if (strstr(sMat, "Q420") != NULL)
				m_nQ420Count++;
			else if (strstr(sMat, "Q460") != NULL)
				m_nQ460Count++;
		}
	}
}
void COptimalSortDlg::InitPrintBom(const char* sFileName)
{
	m_xPrintScopyList.Empty();
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(sFileName))
		return;
	//获取定制列信息
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xJgPrintCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xJgPrintCfg.m_nStartRow - 1;
	//解析数据
	int nSheetNum = excelobj.GetWorkSheetCount();
	int nValidSheetCount = 0, iValidSheet = (nSheetNum == 1) ? 1 : 0;
	BOOL bRetCode = FALSE;
	int iCfgSheetIndex = 0;
	if (g_xUbomModel.m_sJGPrintSheetName.GetLength() > 0)	
	{	//根据配置文件中指定的sheet加载表单
		iCfgSheetIndex = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sJGPrintSheetName);
		if (iCfgSheetIndex > 0)
			iValidSheet = iCfgSheetIndex;
	}
	if (iValidSheet > 0)
	{	//优先读取指定sheet
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iValidSheet);
		if (ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow))
			nValidSheetCount++;
	}
	if (nValidSheetCount == 0)
		logerr.Log("缺少关键列(件号或规格或材质或单基数)!");
}
//解析BOMSHEET内容
BOOL COptimalSortDlg::ParseSheetContent(CVariant2dArray &sheetContentMap, CHashStrList<DWORD>& hashColIndex, int iStartRow)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	CLogErrorLife logLife;
	CStringArray repeatPartLabelArr;
	int nRowCount = sheetContentMap.RowsCount();
	for (int i = iStartRow; i <= nRowCount; i++)
	{
		VARIANT value;
		//件号
		DWORD *pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PART_NO);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		if (value.vt == VT_EMPTY)
			continue;
		CXhChar100 sPartNo = VariantToString(value);
		if (sPartNo.GetLength() < 2 || strstr(sPartNo, "编号"))
			continue;
		//材质
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MATERIAL);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		CXhChar100 sMaterial = VariantToString(value);
		//规格
		int cls_id = 0;
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_SPEC);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		CXhChar100 sSpec = VariantToString(value);
		if (strstr(sSpec, "L") || strstr(sSpec, "∠"))
		{
			cls_id = BOMPART::ANGLE;
			if (strstr(sSpec, "∠"))
				sSpec.Replace("∠", "L");
			if (strstr(sSpec, "×"))
				sSpec.Replace("×", "*");
			if (strstr(sSpec, "x"))
				sSpec.Replace("x", "*");
			if (strstr(sSpec, "X"))
				sSpec.Replace("X", "*");
		}
		else if (strstr(sSpec, "-"))
		{
			cls_id = BOMPART::PLATE;
			if (strstr(sSpec, "x") || strstr(sSpec, "X"))
			{
				char *skey = strtok((char*)sSpec, "x,X");
				sSpec.Copy(skey);
			}
		}
		else //if(strstr(sSpec,"φ"))
			cls_id = BOMPART::TUBE;
		//长度
		float fLength = 0;
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_LEN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fLength = (float)atof(VariantToString(value));
		}
		if (sMaterial.GetLength() <= 0 && sSpec.GetLength() <= 0)
			continue;	//异常数据
		BOMPART* pBomPart = pBomPart = m_hashPrintBom.GetValue(sPartNo);
		if (pBomPart == NULL)
		{
			pBomPart = m_hashPrintBom.Add(sPartNo);
			pBomPart->cPartType = cls_id;
			pBomPart->sPartNo.Copy(sPartNo);
			pBomPart->sSpec.Copy(sSpec);
			pBomPart->sMaterial = sMaterial;
			pBomPart->cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			pBomPart->length = fLength;
		}
	}
	return TRUE;
}
#endif