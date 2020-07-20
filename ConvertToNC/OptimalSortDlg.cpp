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

static int CompareBomPartPtrFunc3(const PART_PTR& partPtr1, const PART_PTR& partPtr2)
{
	int ciPartType1 = partPtr1->cPartType;
	int ciPartType2 = partPtr2->cPartType;
	if (ciPartType1 > ciPartType2)
		return 1;
	else if (ciPartType1 < ciPartType2)
		return -1;
	else if(ciPartType1==BOMPART::ANGLE)
	{
		AngleInfoPtr pAngleInfo1 = (CAngleProcessInfo*)partPtr1->feature2;
		AngleInfoPtr pAngleInfo2 = (CAngleProcessInfo*)partPtr2->feature2;
		if (pAngleInfo1&&pAngleInfo2)
		{
			if (pAngleInfo1->m_ciType > pAngleInfo2->m_ciType)
				return 1;
			else if (pAngleInfo1->m_ciType < pAngleInfo2->m_ciType)
				return -1;
		}
	}
	return CompareBomPartPtrFunc(partPtr1, partPtr2, FALSE);
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
	BOMPART* pBomPart = (BOMPART*)pItem->m_idProp;
	if (pBomPart&&pBomPart->cPartType == BOMPART::PLATE&&pBomPart->feature2 != NULL)
	{
		CPlateProcessInfo* pPlateInfo = (CPlateProcessInfo*)pBomPart->feature2;
		SCOPE_STRU scope = pPlateInfo->GetCADEntScope();
		ZoomAcadView(scope, 20);
	}
	else if (pBomPart&&pBomPart->feature2 != NULL)//pBomPart->cPartType == BOMPART::ANGLE&&
	{
		CAngleProcessInfo* pJgInfo = (CAngleProcessInfo*)pBomPart->feature2;
		SCOPE_STRU scope = pJgInfo->GetCADEntScope();
		ZoomAcadView(scope, 20);
	}
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
	, m_bOtherNotes(FALSE)
	, m_iCmbPrintGroup(3)
	, m_iCmbPrintMode(0)
	, m_iCmbPrintType(0)
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
	m_bOtherNotes = TRUE;
	m_bCommonAngle = TRUE;
	m_nQ235Count = m_nQ345Count = m_nQ355Count = m_nQ390Count = m_nQ420Count = m_nQ460Count = 0;
	m_nJgCount = m_nPlateCount = m_nYGCount = m_nTubeCount = m_nJiaCount = m_nFlatCount = m_nGgsCount = 0;
	m_nCutAngle = m_nKaiHe = m_nPushFlat = m_nCutRoot = m_nCutBer = m_nBend = m_nCommonAngle = m_nOtherNotes = 0;
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
	DDX_Check(pDX, IDC_CHE_OTHER_NOTES, m_bOtherNotes);
	DDX_CBIndex(pDX, IDC_CMB_PRINT_GROUP, m_iCmbPrintGroup);
	DDX_CBIndex(pDX, IDC_CMB_PRINT_MODE, m_iCmbPrintMode);
	DDX_CBIndex(pDX, IDC_CMB_PRINT_TYPE, m_iCmbPrintType);
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
	ON_BN_CLICKED(IDC_CHE_OTHER_NOTES, OnUpdateJgInfo)
	ON_BN_CLICKED(IDC_CHE_COMMON_ANGLE, OnUpdateJgInfo)
	ON_EN_CHANGE(IDC_E_WIDTH, OnEnChangeEWidth)
	ON_EN_CHANGE(IDC_E_THICK, OnEnChangeEThick)
	ON_BN_CLICKED(IDOK, &COptimalSortDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_PRINT_SET, &COptimalSortDlg::OnBnClickedBtnPrintSet)
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
	m_xListCtrl.AddColumnHeader("备注", 100);
	m_xListCtrl.InitListCtrl(NULL,FALSE);
	m_xListCtrl.EnableSortItems(!IsValidPrintBom());
	m_xListCtrl.SetItemChangedFunc(FireItemChanged);
	m_xListCtrl.SetCompareItemFunc(FireCompareItem);
	//初始化控件状态
	InitCtrlState();
	//初始化列表框
	UpdatePartList();
	RefeshListCtrl();
	//从注册表中加载打印配置
	GetPrintParaFromReg("Settings", "PrintType");
	GetPrintParaFromReg("Settings", "PrintMode");
	GetPrintParaFromReg("Settings", "PrintGroup");
	return TRUE;
}
void COptimalSortDlg::RefeshListCtrl()
{
	m_xListCtrl.DeleteAllItems();
	CSuperGridCtrl::CTreeItem *pItem=NULL;
	BOOL bValidPrintBom = IsValidPrintBom();
	for(BOMPART **ppPart=m_xDisplayPartList.GetFirst(); ppPart;ppPart=m_xDisplayPartList.GetNext())
	{
		BOMPART *pPart = *ppPart;
		if (pPart == NULL)
			continue;
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		lpInfo->SetSubItemText(0,pPart->GetPartTypeName(FALSE),TRUE);
		CXhChar16 sMat= CBomModel::QueryMatMarkIncQuality(pPart);
		lpInfo->SetSubItemText(1,sMat,TRUE);//材质	
		lpInfo->SetSubItemText(2,pPart->GetSpec(),TRUE);		//规格
		lpInfo->SetSubItemText(3,pPart->GetPartNo(),TRUE);		//件号
		lpInfo->SetSubItemText(4,CXhChar50("%d",pPart->feature1),TRUE);	//件数
		lpInfo->SetSubItemText(5, pPart->sNotes, TRUE);			//备注
		BOOL bHasCard = pPart->feature2 != NULL;
		if (!bHasCard)
		{
			for (int i = 0; i < 6; i++)
				lpInfo->SetSubItemColor(i, RGB(255,0,0));
		}
		pItem=m_xListCtrl.InsertRootItem(lpInfo, FALSE);
		pItem->m_idProp=(long)pPart;
	}
	m_xListCtrl.Redraw();
	UpdateHelpStr();
	UpdateData(FALSE);
}

static bool IsCommonAngle(CAngleProcessInfo *pJgInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = (pJgInfo != NULL) ? &pJgInfo->m_xAngle : pPart;
	if (pCurPart == NULL || pCurPart->cPartType != BOMPART::ANGLE)
		return false;
	PART_ANGLE *pAngle = (PART_ANGLE*)pCurPart;
	if (pAngle->bCutAngle || pAngle->bKaiJiao || pAngle->bHeJiao ||
		pAngle->nPushFlat > 0 || pAngle->bCutRoot || pAngle->bCutBer ||
		pAngle->GetHuoquLineCount() > 0 || strlen(pAngle->sNotes)>0 ||
		//pPart为导入打印清单构件时备注信息取工艺卡和打印清单之和 wht 20-07-17
		(pJgInfo && pPart && pPart!=&pJgInfo->m_xAngle&&strlen(pPart->sNotes)>0))
		return false;
	else
		return true;
}
static bool IsOtherNotesAngle(CAngleProcessInfo *pJgInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = (pJgInfo != NULL) ? &pJgInfo->m_xAngle : pPart;
	if (pCurPart==NULL || pCurPart->cPartType!=BOMPART::ANGLE)
		return false;
	PART_ANGLE *pAngle = (PART_ANGLE*)pCurPart;
	if (!pAngle->bCutAngle && !pAngle->bKaiJiao && !pAngle->bHeJiao &&
		!(pAngle->nPushFlat > 0) && !pAngle->bCutRoot && !pAngle->bCutBer &&
		!(pAngle->GetHuoquLineCount() > 0) && 
		//pPart为导入打印清单构件时备注信息取工艺卡和打印清单之和 wht 20-07-17
		(strlen(pAngle->sNotes) > 0 || (pJgInfo && pPart && pPart !=&pJgInfo->m_xAngle&&strlen(pPart->sNotes)>0)))
		return true;
	else
		return false;
}
bool COptimalSortDlg::IsFillTheFilter(CAngleProcessInfo* pJgInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = pPart;
	if (pJgInfo != NULL)
	{
		pCurPart = &pJgInfo->m_xAngle;
		if (!m_bSelJg && pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JG)
			return false;
		if (!m_bSelPlate && pJgInfo->m_ciType == CAngleProcessInfo::TYPE_PLATE)
			return false;
		if (!m_bSelYg && pJgInfo->m_ciType == CAngleProcessInfo::TYPE_YG)
			return false;
		if (!m_bSelTube && pJgInfo->m_ciType == CAngleProcessInfo::TYPE_TUBE)
			return false;
		if (!m_bSelFlat&&pJgInfo->m_ciType == CAngleProcessInfo::TYPE_FLAT)
			return false;
		if (!m_bSelJig&&pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JIG)
			return false;
		if (!m_bSelGgs&&pJgInfo->m_ciType == CAngleProcessInfo::TYPE_GGS)
			return false;
	}
	else
	{
		if (!m_bSelJg && pCurPart->cPartType == BOMPART::ANGLE)
			return false;
		if (!m_bSelPlate && pCurPart->cPartType == BOMPART::PLATE)
			return false;
		if (!m_bSelYg && pCurPart->cPartType == BOMPART::ROUND)
			return false;
		if (!m_bSelTube && pCurPart->cPartType == BOMPART::TUBE)
			return false;
		if (!m_bSelFlat && pCurPart->cPartType == BOMPART::FLAT)
			return false;
		if ((!m_bSelJig && !m_bSelGgs) && pCurPart->cPartType == BOMPART::ACCESSORY)
			return false;
	}
	if (pCurPart == NULL)
		return false;
	if (!m_bSelQ235 && pCurPart->cMaterial == 'S')
		return false;
	if (!m_bSelQ345 && pCurPart->cMaterial == 'H')
		return false;
	if (!m_bSelQ355 && pCurPart->cMaterial == 'h')
		return false;
	if (!m_bSelQ390 && pCurPart->cMaterial == 'G')
		return false;
	if (!m_bSelQ420 && pCurPart->cMaterial == 'P')
		return false;
	if (!m_bSelQ460 && pCurPart->cMaterial == 'T')
		return false;
	if (m_thickHashTbl.GetNodeNum() > 0 && m_thickHashTbl.GetValue((int)pCurPart->thick) == NULL)
		return false;
	if (m_widthHashTbl.GetNodeNum() > 0 && m_widthHashTbl.GetValue((int)pCurPart->wide) == NULL)
		return false;
	if (pCurPart&&pCurPart->cPartType == BOMPART::ANGLE)
	{
		PART_ANGLE *pAngle = (PART_ANGLE*)pCurPart;
		if ((m_bCutAngle && pAngle->bCutAngle) ||
			(m_bCutBer && pAngle->bCutBer) ||
			(m_bCutRoot && pAngle->bCutRoot) ||
			(m_bKaiHe && (pAngle->bKaiJiao || pAngle->bHeJiao)) ||
			(m_bPushFlat && pAngle->nPushFlat > 0) ||
			(m_bBend && pAngle->GetHuoquLineCount() > 0) ||
			(m_bOtherNotes && IsOtherNotesAngle(pJgInfo, pPart)) ||
			(m_bCommonAngle && IsCommonAngle(pJgInfo, pPart)))
			return true;
		else
			return false;
	}
	return true;
}
bool COptimalSortDlg::IsFillTheFilter(CPlateProcessInfo* pPlateInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = pPart;
	if (pPlateInfo != NULL)
		pCurPart = &pPlateInfo->xBomPlate;

	if (pCurPart->cPartType!=BOMPART::PLATE || !m_bSelPlate)
		return false;
	if (!m_bSelQ235 && pCurPart->cMaterial == 'S')
		return false;
	if (!m_bSelQ345 && pCurPart->cMaterial == 'H')
		return false;
	if (!m_bSelQ355 && pCurPart->cMaterial == 'h')
		return false;
	if (!m_bSelQ390 && pCurPart->cMaterial == 'G')
		return false;
	if (!m_bSelQ420 && pCurPart->cMaterial == 'P')
		return false;
	if (!m_bSelQ460 && pCurPart->cMaterial == 'T')
		return false;
	if (!m_bBend && pCurPart->GetHuoquLineCount() > 0)
		return false;
	if (m_thickHashTbl.GetNodeNum() > 0 && m_thickHashTbl.GetValue((int)pCurPart->thick) == NULL)
		return false;
	return true;
}
void COptimalSortDlg::UpdatePartList()
{
	CLogErrorLife logErrLife;
	m_xDisplayPartList.Empty();
	m_nRecord=0;
	if (m_pDwgFile == NULL)
		return;
	if (IsValidPrintBom())
	{	//用户提供了打印清单，不需要进行排序
		for(BOMPART* pBomPart=m_xPrintBomFile.EnumFirstPart();pBomPart;pBomPart=m_xPrintBomFile.EnumNextPart())
		{
			pBomPart->feature2 = NULL;
			if (pBomPart->cPartType == BOMPART::ANGLE)
			{
				CAngleProcessInfo* pJgInfo = m_pDwgFile->FindAngleByPartNo(pBomPart->sPartNo);
				pBomPart->feature2 = (long)pJgInfo;
				if (IsFillTheFilter(pJgInfo, pBomPart))
				{
					m_nRecord++;
					m_xDisplayPartList.append(pBomPart);
				}
			}
			else if (pBomPart->cPartType == BOMPART::PLATE)
			{
				CPlateProcessInfo* pPlateInfo = m_pDwgFile->FindPlateByPartNo(pBomPart->sPartNo);
				pBomPart->feature2 = (long)pPlateInfo;
				if (IsFillTheFilter(pPlateInfo,pBomPart))
				{
					m_nRecord++;
					m_xDisplayPartList.append(pBomPart);
				}
			}
			else
			{
				CAngleProcessInfo* pJgInfo = m_pDwgFile->FindAngleByPartNo(pBomPart->sPartNo);
				CPlateProcessInfo* pPlateInfo = m_pDwgFile->FindPlateByPartNo(pBomPart->sPartNo);
				if (pPlateInfo)
				{
					pBomPart->feature2 = (long)pPlateInfo;
					if (IsFillTheFilter(pPlateInfo, pBomPart))
					{
						m_nRecord++;
						m_xDisplayPartList.append(pBomPart);
					}
				}
				else
				{
					pBomPart->feature2 = (long)pJgInfo;
					if (IsFillTheFilter(pJgInfo, pBomPart))
					{
						m_nRecord++;
						m_xDisplayPartList.append(pBomPart);
					}
				}
			}
		}
	}
	else
	{
		for (CAngleProcessInfo* pJgInfo = m_pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = m_pDwgFile->EnumNextJg())
		{
			pJgInfo->m_xAngle.feature2 = (long)pJgInfo;
			if (IsFillTheFilter(pJgInfo,&pJgInfo->m_xAngle))
			{
				m_nRecord++;
				m_xDisplayPartList.append(&pJgInfo->m_xAngle);
			}
		}
		//
		for (CPlateProcessInfo* pPlateInfo = m_pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = m_pDwgFile->EnumNextPlate())
		{
			pPlateInfo->xBomPlate.feature2 = (long)pPlateInfo;
			if (IsFillTheFilter(pPlateInfo,&pPlateInfo->xBomPlate))
			{
				m_nRecord++;
				m_xDisplayPartList.append(&pPlateInfo->xBomPlate);
			}
		}
		//按照材料名称-材质-规格的顺序进行排序
		if (m_xDisplayPartList.GetSize() > 0)
			CQuickSort<BOMPART*>::QuickSort(m_xDisplayPartList.m_pData, m_xDisplayPartList.GetSize(), CompareBomPartPtrFunc3);
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
	CLogErrorLife logLife;
	m_xPrintScopyList.Empty();
	POSITION pos = m_xListCtrl.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int i=m_xListCtrl.GetNextSelectedItem(pos);
		CSuperGridCtrl::CTreeItem* pItem=m_xListCtrl.GetTreeItem(i);
		BOMPART *pBomPart = (BOMPART*)pItem->m_idProp;
		if (pBomPart&&pBomPart->cPartType == BOMPART::PLATE)
		{
			CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pBomPart->feature2;
			if (pSelPlateInfo)
			{
				PRINT_SCOPE *pScope = m_xPrintScopyList.append();
				pScope->Init(pSelPlateInfo);
			}
			else
				logerr.Log("%s#未找到工艺卡", (char*)pBomPart->GetPartNo());
		}
		else //if (pBomPart&&pBomPart->cPartType == BOMPART::ANGLE)
		{
			CAngleProcessInfo* pSelJgInfo = pBomPart ? (CAngleProcessInfo*)pBomPart->feature2 : NULL;
			if (pSelJgInfo)
			{
				PRINT_SCOPE *pScope = m_xPrintScopyList.append();
				pScope->Init(pSelJgInfo);
			}
			else
				logerr.Log("%s#未找到工艺卡", (char*)pBomPart->GetPartNo());
		}
	}
	if (m_xPrintScopyList.GetNodeNum() == 1)
		m_xPrintScopyList.Empty();	//选中多行时支持打印选中行，选中一行打印所有
	if(m_xPrintScopyList.GetNodeNum()<=0)
	{
		logerr.ClearContents();
		for(BOMPART **ppBomPart=m_xDisplayPartList.GetFirst(); ppBomPart; ppBomPart =m_xDisplayPartList.GetNext())
		{
			BOMPART *pBomPart = *ppBomPart;
			if (pBomPart == NULL )
				continue;
			if (pBomPart->cPartType == BOMPART::PLATE)
			{
				CPlateProcessInfo* pSelPlateInfo = (CPlateProcessInfo*)pBomPart->feature2;
				if (pSelPlateInfo)
				{
					PRINT_SCOPE *pScope = m_xPrintScopyList.append();
					pScope->Init(pSelPlateInfo);
				}
				else
					logerr.Log("%s#未找到工艺卡", (char*)pBomPart->GetPartNo());
			}
			else //if (pBomPart->cPartType == BOMPART::ANGLE)
			{
				CAngleProcessInfo* pSelJgInfo = (CAngleProcessInfo*)pBomPart->feature2;
				if (pSelJgInfo)
				{
					PRINT_SCOPE *pScope = m_xPrintScopyList.append();
					pScope->Init(pSelJgInfo);
				}
				else
					logerr.Log("%s#未找到工艺卡", (char*)pBomPart->GetPartNo());
			}
		}
	}
	return CDialog::OnOK();
}


void COptimalSortDlg::OnBnClickedOk()
{
	UpdateData();
	m_iPrintType = m_iCmbPrintType;//CBatchPrint::PRINT_TYPE_PAPER;
	WritePrintParaToReg("Settings", "PrintType");
	WritePrintParaToReg("Settings", "PrintMode");
	WritePrintParaToReg("Settings", "PrintGroup");
	OnOK();
	//CDialog::OnOK();
}

void COptimalSortDlg::InitByProcessAngle(CAngleProcessInfo* pJgInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = (pJgInfo != NULL) ? &pJgInfo->m_xAngle : pPart;
	if (pCurPart == NULL)
		return;
	if (pJgInfo)
	{
		if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JG)
		{
			m_nJgCount++;
			if (IsOtherNotesAngle(pJgInfo, pPart))
				m_nOtherNotes++;
			else if (!IsCommonAngle(pJgInfo, pPart))
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
		else
		{
			if (IsOtherNotesAngle(pJgInfo, pPart))
				m_nOtherNotes++;
			else if (IsCommonAngle(pJgInfo, pPart))
				m_nCommonAngle++;
			if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_YG)
				m_nYGCount++;
			else if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_TUBE)
				m_nTubeCount++;
			else if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_JIG)
				m_nJiaCount++;
			else if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_FLAT)
				m_nFlatCount++;
			else if (pJgInfo->m_ciType == CAngleProcessInfo::TYPE_GGS)
				m_nGgsCount++;
		}
	}
	else
	{
		if (pCurPart->cPartType == BOMPART::ANGLE)
		{
			m_nJgCount++;
			PART_ANGLE *pAngle = (PART_ANGLE*)pCurPart;
			if (IsOtherNotesAngle(pJgInfo, pAngle))
				m_nOtherNotes++;
			else if (!IsCommonAngle(pJgInfo, pAngle))
			{
				if (pAngle->bCutAngle)
					m_nCutAngle++;
				if (pAngle->bKaiJiao || pAngle->bHeJiao)
					m_nKaiHe++;
				if (pAngle->nPushFlat > 0)
					m_nPushFlat++;
				if (pAngle->bCutRoot)
					m_nCutRoot++;
				if (pAngle->bCutBer)
					m_nCutBer++;
				if (pAngle->GetHuoquLineCount() > 0)
					m_nBend++;
			}
			else
				m_nCommonAngle++;
		}
		else
		{
			if (IsOtherNotesAngle(pJgInfo, pPart))
				m_nOtherNotes++;
			else if (IsCommonAngle(pJgInfo, pPart))
				m_nCommonAngle++;

			if (pCurPart->cPartType == BOMPART::ROUND)
				m_nYGCount++;
			else if (pCurPart->cPartType == BOMPART::TUBE)
				m_nTubeCount++;
			else if (pCurPart->cPartType == BOMPART::PLATE)
				m_nPlateCount++;
			else if (pCurPart->cPartType == BOMPART::FLAT)
				m_nFlatCount++;
			else if (pCurPart->cPartType == BOMPART::ACCESSORY)
				m_nJiaCount++;	//附件归属为夹具
		}
	}
	CXhChar16 sMat = CBomModel::QueryMatMarkIncQuality(pCurPart);
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
void COptimalSortDlg::InitByProcessPlate(CPlateProcessInfo* pPlateInfo, BOMPART *pPart)
{
	BOMPART *pCurPart = (pPlateInfo != NULL) ? &pPlateInfo->xBomPlate : pPart;
	if (pCurPart == NULL)
		return;
	if (pPlateInfo == NULL)
	{
		if (pCurPart->cPartType == BOMPART::ANGLE)
		{
			m_nJgCount++;
			PART_ANGLE *pAngle = (PART_ANGLE*)pCurPart;
			if (IsOtherNotesAngle((CAngleProcessInfo*)NULL, pAngle))
				m_nOtherNotes++;
			else if (!IsCommonAngle((CAngleProcessInfo*)NULL, pAngle))
			{
				if (pAngle->bCutAngle)
					m_nCutAngle++;
				if (pAngle->bKaiJiao || pAngle->bHeJiao)
					m_nKaiHe++;
				if (pAngle->nPushFlat > 0)
					m_nPushFlat++;
				if (pAngle->bCutRoot)
					m_nCutRoot++;
				if (pAngle->bCutBer)
					m_nCutBer++;
				if (pAngle->GetHuoquLineCount() > 0)
					m_nBend++;
			}
			else
				m_nCommonAngle++;
		}
		else
		{
			if (IsOtherNotesAngle((CAngleProcessInfo*)NULL, pCurPart))
				m_nOtherNotes++;
			if (pCurPart->cPartType == BOMPART::ROUND)
				m_nYGCount++;
			else if (pCurPart->cPartType == BOMPART::TUBE)
				m_nTubeCount++;
			else if (pCurPart->cPartType == BOMPART::PLATE)
				m_nPlateCount++;
			else if (pCurPart->cPartType == BOMPART::FLAT)
				m_nFlatCount++;
			else if (pCurPart->cPartType == BOMPART::ACCESSORY)
				m_nJiaCount++;	//附件归属为夹具
		}
	}
	else
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
void COptimalSortDlg::InitCtrlState()
{
	m_nQ235Count = m_nQ345Count = m_nQ355Count = m_nQ390Count = m_nQ420Count = m_nQ460Count = 0;
	m_nJgCount = m_nPlateCount = m_nYGCount = m_nTubeCount = m_nJiaCount = m_nFlatCount = m_nGgsCount = 0;
	m_nCutAngle = m_nKaiHe = m_nPushFlat = m_nCutRoot = m_nCutBer = m_nBend = m_nCommonAngle = m_nOtherNotes = 0;
	//如果有打印清单，根据打印清单更新控件状态，否则根据图纸信息进行初始化
	if (m_xPrintBomFile.GetPartNum() > 0)
	{
		for (BOMPART* pBomPart = m_xPrintBomFile.EnumFirstPart(); pBomPart; pBomPart = m_xPrintBomFile.EnumNextPart())
		{
			if (pBomPart->cPartType == BOMPART::ANGLE)
			{
				CAngleProcessInfo* pJgInfo = m_pDwgFile->FindAngleByPartNo(pBomPart->sPartNo);
				InitByProcessAngle(pJgInfo,pBomPart);
			}
			else if (pBomPart->cPartType == BOMPART::PLATE)
			{
				CPlateProcessInfo* pPlateInfo = m_pDwgFile->FindPlateByPartNo(pBomPart->sPartNo);
				InitByProcessPlate(pPlateInfo,pBomPart);
			}
			else
			{
				CAngleProcessInfo* pJgInfo = m_pDwgFile->FindAngleByPartNo(pBomPart->sPartNo);
				CPlateProcessInfo* pPlateInfo = m_pDwgFile->FindPlateByPartNo(pBomPart->sPartNo);
				if(pPlateInfo)
					InitByProcessPlate(pPlateInfo, pBomPart);
				else
					InitByProcessAngle(pJgInfo, pBomPart);
			}
		}
	}
	else
	{
		for (CAngleProcessInfo* pJgInfo = m_pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = m_pDwgFile->EnumNextJg())
			InitByProcessAngle(pJgInfo,&pJgInfo->m_xAngle);
		for (CPlateProcessInfo* pPlateInfo = m_pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = m_pDwgFile->EnumNextPlate())
			InitByProcessPlate(pPlateInfo,&pPlateInfo->xBomPlate);
	}
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
	GetDlgItem(IDC_CHE_COMMON_ANGLE)->EnableWindow(m_nCommonAngle > 0);
	GetDlgItem(IDC_CHE_OTHER_NOTES)->EnableWindow(m_nOtherNotes > 0);
	//
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
	m_bOtherNotes = m_nOtherNotes > 0;
}
bool COptimalSortDlg::InitPrintBom(const char* sFileName)
{
	m_xPrintScopyList.Empty();
	if (sFileName == NULL)
		return false;
	else
	{
		m_xPrintBomFile.m_sFileName.Copy(sFileName);
		bool bRetCode = (m_xPrintBomFile.ImportPrintExcelFile()==TRUE);
		return bRetCode;
	}
}
bool COptimalSortDlg::IsValidPrintBom()
{
	return m_xPrintBomFile.GetPartNum()>0;
}
void COptimalSortDlg::UpdateHelpStr()
{
	if (m_xListCtrl.GetSafeHwnd() == NULL)
		return;
	CXhChar500 sHelpStr;
	int nSumCount = m_xPrintBomFile.GetPartNum();
	int nAngleCount = 0, nPlateCount = 0, nOtherCount = 0;
	if (IsValidPrintBom())
	{
		for (BOMPART *pBomPart = m_xPrintBomFile.EnumFirstPart(); pBomPart; pBomPart = m_xPrintBomFile.EnumNextPart())
		{
			if (pBomPart->cPartType == BOMPART::ANGLE)
				nAngleCount++;
			else if (pBomPart->cPartType == BOMPART::PLATE)
				nPlateCount++;
			else
				nOtherCount++;
		}
		sHelpStr.Printf("打印清单：%d=%d[L]+%d[-]+%d。", nSumCount, nAngleCount, nPlateCount, nOtherCount);
	}
	nSumCount = m_xDisplayPartList.Count;
	nAngleCount = nPlateCount = nOtherCount = 0;
	for (BOMPART **ppBomPart = m_xDisplayPartList.GetFirst(); ppBomPart; ppBomPart = m_xDisplayPartList.GetNext())
	{
		BOMPART *pBomPart = *ppBomPart;
		if (pBomPart && pBomPart->cPartType == BOMPART::ANGLE)
			nAngleCount++;
		else if (pBomPart && pBomPart->cPartType == BOMPART::PLATE)
			nPlateCount++;
		else
			nOtherCount++;
	}
	sHelpStr.Append(CXhChar500("当前显示：%d=%d[L]+%d[-]+%d。", nSumCount, nAngleCount, nPlateCount, nOtherCount));
	GetDlgItem(IDC_S_HELP_STR)->SetWindowText(sHelpStr);
}

void COptimalSortDlg::OnBnClickedBtnPrintSet()
{
#ifdef _ARX_2007
	SendCommandToCad(L"PLOT ");
#else
	SendCommandToCad("PLOT ");
#endif
}

BOOL COptimalSortDlg::GetPrintParaFromReg(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH] = "";
	char sSubKey[MAX_PATH] = "";
	DWORD dwDataType, dwLength = MAX_PATH;
	sprintf(sSubKey, "Software\\Xerofox\\UBOM\\%s", lpszSection);
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS && hKey)
	{
		if (stricmp(lpszEntry, "PrintType") == 0 ||
			stricmp(lpszEntry, "PrintMode") == 0 ||
			stricmp(lpszEntry, "PrintGroup") == 0 )
		{
			DWORD dwValue = 0;
			dwLength = sizeof(DWORD);
			if (RegQueryValueEx(hKey, lpszEntry, NULL, &dwDataType, (BYTE*)&dwValue, &dwLength) == ERROR_SUCCESS)
			{
				if (stricmp(lpszEntry, "PrintType") == 0)
					m_iCmbPrintType = atoi(sValue);
				else if (stricmp(lpszEntry, "PrintMode") == 0)
					m_iCmbPrintMode = atoi(sValue);
				else //if(stricmp(lpszEntry, "PrintGroup") == 0)
					m_iCmbPrintGroup = (UINT)dwValue;
			}
			else
				return FALSE;
		}
		RegCloseKey(hKey);
	}
	else
		return FALSE;
	return TRUE;
}

void COptimalSortDlg::WritePrintParaToReg(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH] = "";
	char sSubKey[MAX_PATH] = "";
	DWORD dwLength = 0;
	sprintf(sSubKey, "Software\\Xerofox\\UBOM\\%s", lpszSection);
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS && hKey)
	{
		if (stricmp(lpszEntry, "PrintType") == 0 ||
			stricmp(lpszEntry, "PrintMode") == 0 ||
			stricmp(lpszEntry, "PrintGroup") == 0)
		{
			DWORD dwValue = 0;
			dwLength = sizeof(DWORD);
			if (stricmp(lpszEntry, "PrintType") == 0)
				dwValue = m_iCmbPrintType;
			else if (stricmp(lpszEntry, "PrintMode") == 0)
				dwValue = m_iCmbPrintMode;
			else //if(stricmp(lpszEntry, "PrintGroup") == 0)
				dwValue = m_iCmbPrintGroup;
			RegSetValueEx(hKey, lpszEntry, NULL, REG_DWORD, (BYTE*)&dwValue, dwLength);
		}
		RegCloseKey(hKey);
	}
}

BOOL COptimalSortDlg::DestroyWindow()
{
	WritePrintParaToReg("Settings", "PrintType");
	WritePrintParaToReg("Settings", "PrintMode");
	WritePrintParaToReg("Settings", "PrintGroup");
	return CDialog::DestroyWindow();
}
#endif

