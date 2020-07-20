// TowerInstanceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RevisionDlg.h"
#include "ProcessPart.h"
#include "CadToolFunc.h"
#include "SortFunc.h"
#include "image.h"
#include "folder_dialog.h"
#include "MsgBox.h"
#include "ComparePartNoString.h"
#include "OptimalSortDlg.h"
#include "PNCSysPara.h"
#include "BomExport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CRevisionDlg *g_pRevisionDlg;
//
#ifdef __SUPPORT_DOCK_UI_
IMPLEMENT_DYNCREATE(CRevisionDlg, CAcUiDialog)
#else
IMPLEMENT_DYNCREATE(CRevisionDlg, CDialog)
#endif

//
CXhChar500 CRevisionDlg::g_sPrintDwgFileName;
static BOOL FireItemChanged(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,NM_LISTVIEW* pNMListView)
{	//选中项发生变化后更新属性栏
	if(pItem->m_idProp==NULL)
		return FALSE;
	CRevisionDlg *pRevisionDlg=(CRevisionDlg*)pListCtrl->GetParent();
	CXhTreeCtrl* pTreeCtrl=pRevisionDlg->GetTreeCtrl();
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	if(hSelectedItem==NULL)
		return FALSE;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	SCOPE_STRU scope;
	BOOL bRetCode = FALSE;
	if (pItemInfo->itemType == BOM_ITEM)
	{
		BOMPART *pPart = (BOMPART*)pItem->m_idProp;
		CBomFile* pBom = (CBomFile*)pItemInfo->dwRefData;
		CProjectTowerType *pPrjTowerType = pBom?pBom->BelongModel():NULL;
		if (pPart&&pBom&&pPrjTowerType)
		{
			if(pPart->cPartType==BOMPART::PLATE)
			{
				CPlateProcessInfo *pPlateInfo=pPrjTowerType->FindPlateInfoByPartNo(pPart->sPartNo);
				if (pPlateInfo)
				{
					scope = pPlateInfo->GetCADEntScope();
					bRetCode = TRUE;
				}
			}
			else if(pPart->cPartType==BOMPART::ANGLE)
			{
				CAngleProcessInfo *pAngleInfo = pPrjTowerType->FindAngleInfoByPartNo(pPart->sPartNo);
				if (pAngleInfo)
				{
					scope = pAngleInfo->GetCADEntScope();
					bRetCode = TRUE;
				}
			}
		}
	}
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM)
	{
		CAngleProcessInfo* pJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
		if (pJgInfo)
		{
			scope = pJgInfo->GetCADEntScope();
			bRetCode = TRUE;
		}
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CPlateProcessInfo* pPlateInfo=(CPlateProcessInfo*)pItem->m_idProp;
		if (pPlateInfo)
		{
			scope = pPlateInfo->GetCADEntScope();
			bRetCode = TRUE;
		}
	}
	if (bRetCode)	//缩放至命令对应的实体
		ZoomAcadView(scope, 20);
	return bRetCode;
}
static CXhChar16 RemovePartNoMatBriefMark(CXhChar16 sPartNo,char cMaterial)
{
	if( cMaterial!='S'&&
		cMaterial!='H'&&
		cMaterial!='G'&&
		cMaterial!='P'&&
		cMaterial!='T')
		return sPartNo;
	CXhChar16 sNewPartNo(sPartNo);
	if(sPartNo.At(0)==cMaterial)
		sNewPartNo.Copy((char*)sPartNo+1);
	else if(sPartNo.At(sPartNo.GetLength()-1)==cMaterial)
		sNewPartNo.NCopy(sPartNo,sPartNo.GetLength()-1);
	sNewPartNo=sNewPartNo.Replace(' ',0);
	return sNewPartNo;
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
	int result = 0;
	if (iSubItem == 0)	//件号
		result = ComparePartNoString(sText1, sText2, "SHGPT");
	else if (iSubItem == 1)
		result = CompareMultiSectionString(sText1, sText2);
	else
		result = sText1.Compare(sText2);
	if (!bAscending)
		result *= -1;
	return result;
}
//////////////////////////////////////////////////////////////////////////
// CRevisionDlg 对话框
#ifdef __SUPPORT_DOCK_UI_
CRevisionDlg::CRevisionDlg(CWnd* pParent /*=NULL*/)
	: CAcUiDialog(CRevisionDlg::IDD, pParent)
#else
CRevisionDlg::CRevisionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRevisionDlg::IDD, pParent)
#endif
{
	m_sCurFile = _T("");
	m_sRecordNum= _T("");
	m_sSearchText = _T("");
	m_sLegErr = _T("");
	m_bQuality = TRUE;
	m_bMaterialH = FALSE;
	//
	m_nRightMargin=0;
	m_nBtmMargin=0;
	m_iCompareMode=0;
}

CRevisionDlg::~CRevisionDlg()
{
}

void CRevisionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REPORT, m_xListReport);
	DDX_Control(pDX, IDC_TREE_CONTRL, m_treeCtrl);
	DDX_Text(pDX, IDC_E_FILE_NAME, m_sCurFile);
	DDX_Text(pDX, IDC_E_NUM, m_sRecordNum);
	DDX_Text(pDX, IDC_E_SEARCH_TEXT, m_sSearchText);
	DDX_Text(pDX, IDC_E_LEN_ERR, m_sLegErr);
	DDX_Check(pDX, IDC_CHK_GRADE, m_bQuality);
	DDX_Check(pDX, IDC_CHK_MAT_H, m_bMaterialH);
}


BEGIN_MESSAGE_MAP(CRevisionDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CONTRL, &OnNMRClickTreeCtrl)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTRL, &OnTvnSelchangedTreeContrl)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE_CONTRL, &OnTvnBeginlabeleditTreeContrl)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_CONTRL, &OnTvnEndlabeleditTreeContrl)
	ON_NOTIFY(NM_CLICK, IDC_TREE_CONTRL, &OnNMClickTreeContrl)
	ON_COMMAND(ID_NEW_ITEM,OnNewItem)
	ON_COMMAND(ID_LOAD_PROJECT,OnLoadProjFile)
	ON_COMMAND(ID_EXPORT_PROJECT,OnExportProjFile)
	ON_COMMAND(ID_EXPORT_XLS_FILE, OnExportXLSFile)
	ON_COMMAND(ID_IMPORT_BOM_FILE,OnImportBomFile)
	ON_COMMAND(ID_IMPORT_ANGLE_DWG,OnImportAngleDwg)
	ON_COMMAND(ID_IMPORT_PLATE_DWG,OnImportPlateDwg)
	ON_COMMAND(ID_COMPARE_DATA,OnCompareData)
	ON_COMMAND(ID_EXPORT_COMPARE_RESULT,OnExportCompResult)
	ON_COMMAND(ID_REFRESH_PART_NUM,OnRefreshPartNum)
	ON_COMMAND(ID_REFRESH_SINGLE_NUM, OnRefreshSingleNum)
	ON_COMMAND(ID_MODIFY_ERP_FILE,OnModifyErpFile)
	ON_COMMAND(ID_RETRIEVED_ANGLES, OnRetrievedAngles)
	ON_COMMAND(ID_RETRIEVED_PLATES, OnRetrievedPlates)
	ON_COMMAND(ID_REVISE_TEH_PLATE, OnRetrievedPlate)
	ON_COMMAND(ID_BATCH_PRINT_PART, OnBatchPrintPart)
	ON_COMMAND(ID_DELETE_ITEM,OnDeleteItem)
	ON_BN_CLICKED(IDC_BTN_SEARCH, OnSearchPart)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDC_CHK_GRADE, OnBnClickedChkGrade)
	ON_EN_CHANGE(IDC_E_LEN_ERR, OnEnChangeELenErr)
	ON_BN_CLICKED(IDC_CHK_MAT_H, &CRevisionDlg::OnBnClickedChkMatH)
END_MESSAGE_MAP()

// CRevisionDlg 消息处理程序
BOOL CRevisionDlg::OnInitDialog()
{
#ifdef __SUPPORT_DOCK_UI_
	CAcUiDialog::OnInitDialog();
#else
	CDialog::OnInitDialog();
#endif
	m_bQuality = g_pncSysPara.m_bCmpQualityLevel;
	m_bMaterialH = g_pncSysPara.m_bEqualH_h;
	m_sLegErr.Format("%.1f", g_pncSysPara.m_fMaxLenErr);
	//初始化列表框
	m_xListReport.EnableSortItems(true,true);
	m_xListReport.SetGridLineColor(RGB(220, 220, 220));
	m_xListReport.SetEvenRowBackColor(RGB(224, 237, 236));
	m_xListReport.SetCompareItemFunc(FireCompareItem);
	m_xListReport.SetItemChangedFunc(FireItemChanged);
	m_xListReport.EmptyColumnHeader();
	for (size_t i = 0; i < g_xUbomModel.m_xBomTitleArr.size(); i++)
		m_xListReport.AddColumnHeader(g_xUbomModel.m_xBomTitleArr[i].m_sTitle, g_xUbomModel.m_xBomTitleArr[i].m_nWidth);
	m_xListReport.InitListCtrl();
	RefreshListCtrl(NULL);
	//初始化树列表
	m_treeCtrl.ModifyStyle(0, TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_EDITLABELS);
	RefreshTreeCtrl();
	//移动窗口到合适位置
	RECT rect,clientRect;
	GetClientRect(&clientRect);
	GetDlgItem(IDC_LIST_REPORT)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_nRightMargin=clientRect.right-rect.right;
	m_nBtmMargin=clientRect.bottom-rect.bottom;
	UpdateData(FALSE);
	return TRUE;
}
void CRevisionDlg::OnOK()
{
}
void CRevisionDlg::OnCancel()
{	
}
void CRevisionDlg::OnClose()
{
	DestroyWindow();
}
BOOL CRevisionDlg::CreateDlg()
{
	return CDialog::Create(CRevisionDlg::IDD);
}
void CRevisionDlg::InitRevisionDlg()
{
	if(GetSafeHwnd()==0)
		CreateDlg();
	else
		OnInitDialog();
	UpdateData(FALSE);
}

HTREEITEM CRevisionDlg::FindTreeItem(HTREEITEM hParentItem,CXhChar100 sName)
{
	if(hParentItem==NULL)
		return NULL;
	CXhTreeCtrl* pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem;
	hItem=pTreeCtrl->GetChildItem(hParentItem);
	while(hItem)
	{
		CString sItemName=pTreeCtrl->GetItemText(hItem);
		if(stricmp(sItemName,sName)==0)
			break;
		hItem=pTreeCtrl->GetNextItem(hItem,TVGN_NEXT);
	}
	return hItem;
}
CProjectTowerType* CRevisionDlg::GetProject(HTREEITEM hItem)
{
	if(hItem==NULL)
		return NULL;
	CProjectTowerType* pProject=NULL;
	CXhTreeCtrl* pTreeCtrl=GetTreeCtrl();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItem);
	if(pItemInfo->itemType==PROJECT_ITEM)
		pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	else if(pItemInfo->itemType==BOM_GROUP || pItemInfo->itemType==ANGLE_GROUP ||
		pItemInfo->itemType==PLATE_GROUP)
	{
		HTREEITEM hParentItem=pTreeCtrl->GetParentItem(hItem);
		pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hParentItem);
		if(pItemInfo&&pItemInfo->itemType==PROJECT_ITEM)
			pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	}
	else if(pItemInfo->itemType==BOM_ITEM)
	{
		CBomFile* pBom=(CBomFile*)pItemInfo->dwRefData;
		pProject=pBom->BelongModel();
	}
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM || pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CDwgFileInfo* pDwg=(CDwgFileInfo*)pItemInfo->dwRefData;
		pProject=pDwg->BelongModel();
	}
	return pProject;
}
static CSuperGridCtrl::CTreeItem *InsertPartToList(CSuperGridCtrl &list,CSuperGridCtrl::CTreeItem *pParentItem,
	BOMPART *pPart,CHashStrList<BOOL> *pHashBoolByPropName=NULL,BOOL bUpdate=FALSE)
{
	COLORREF clr=RGB(230,100,230);
	PART_ANGLE* pBomJg = (pPart->cPartType == BOMPART::ANGLE) ? (PART_ANGLE*)pPart : NULL;
	CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
	for (size_t i = 0; i < g_xUbomModel.m_xBomTitleArr.size(); i++)
	{
		if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_PN))
		{	//件号
			lpInfo->SetSubItemText(i, pPart->sPartNo, TRUE);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_SPEC))
		{	//规格
			lpInfo->SetSubItemText(i, pPart->sSpec, TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_SPEC))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_MAT))
		{	//材质
			lpInfo->SetSubItemText(i, CBomModel::QueryMatMarkIncQuality(pPart), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_MAT))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_LEN))
		{	//长度
			lpInfo->SetSubItemText(i, CXhChar50("%.0f", pPart->length), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_LEN))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_SING_N))
		{	//单基数
			lpInfo->SetSubItemText(i, CXhChar50("%d", pPart->GetPartNum()), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_SING_N))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_MANU_N))
		{	//加工数
			lpInfo->SetSubItemText(i, CXhChar50("%d", pPart->feature1), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_MANU_N))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_MANU_W))
		{	//加工重量
			lpInfo->SetSubItemText(i, CXhChar50("%.f", pPart->fSumWeight), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_MANU_W))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_NOTES))
		{	//备注
			lpInfo->SetSubItemText(i, pPart->sNotes, TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_NOTES))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_WELD))
		{	//焊接
			lpInfo->SetSubItemText(i, pPart->bWeldPart ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_WELD))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_ZHI_WAN))
		{	//制弯
			lpInfo->SetSubItemText(i, (pPart->siZhiWan > 0) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_ZHI_WAN))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ANGLE))
		{	//切角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutAngle) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_CUT_ANGLE))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_PUSH_FLAT))
		{	//打扁
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->nPushFlat > 0) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_PUSH_FLAT))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_BER))
		{	//铲背
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutBer) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_CUT_BER))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ROOT))
		{	//刨角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutRoot) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_CUT_ROOT))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_KAI_JIAO))
		{	//开角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bKaiJiao) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_KAI_JIAO))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_HE_JIAO))
		{	//合角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bHeJiao) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_HE_JIAO))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_FOO_NAIL))
		{	//带脚钉
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bHasFootNail) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomModel::KEY_FOO_NAIL))
				lpInfo->SetSubItemColor(i, clr);
		}
	}
	if(pParentItem)
		return list.InsertItem(pParentItem,lpInfo,-1,bUpdate==TRUE);
	else
		return list.InsertRootItem(lpInfo,bUpdate);
}
typedef CPlateProcessInfo* CPlateInfoPtr;
typedef CAngleProcessInfo* CAngleInfoPtr;
typedef BOMPART* BomPartPtr;
int ComparePlatePtrByPartNo(const CPlateInfoPtr &plate1, const CPlateInfoPtr &plate2);
int CompareByPartNo(const CXhChar50& item1, const CXhChar50& item2)
{
	return ComparePartNoString(item1, item2, "SHGPT");
}
int CompareAnglePtrByPartNo(const CAngleInfoPtr &angle1, const CAngleInfoPtr &angle2)
{
	CXhChar50 sPartNo1 = angle1->m_xAngle.GetPartNo();
	CXhChar50 sPartNo2 = angle2->m_xAngle.GetPartNo();
	return ComparePartNoString(sPartNo1, sPartNo2, "SHGPT");
}
int CompareBomPartPtrByPartNo(const BomPartPtr &part1, const BomPartPtr &part2)
{
	CXhChar50 sPartNo1 = part1->GetPartNo();
	CXhChar50 sPartNo2 = part2->GetPartNo();
	return ComparePartNoString(sPartNo1, sPartNo2, "SHGPT");
}
void CRevisionDlg::RefreshListCtrl(HTREEITEM hItem,BOOL bCompared/*=FALSE*/)
{
	m_sRecordNum = "";
	m_sCurFile = "";
	TREEITEM_INFO *pInfo = hItem != NULL ? (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem) : NULL;
	if(pInfo==NULL || hItem==NULL)
		return;
	CSuperGridCtrl::CTreeItem *pItem = NULL;
	int index = 0, nNum = 0;
	if(!bCompared)
	{
		if (pInfo->itemType != BOM_ITEM &&
			pInfo->itemType != ANGLE_DWG_ITEM &&
			pInfo->itemType != PLATE_DWG_ITEM)
			return;
		m_xListReport.DeleteAllItems();
		if(pInfo->itemType==BOM_ITEM)
		{
			DisplayProgress(0, "显示料单信息......");
			CBomFile* pBom=(CBomFile*)pInfo->dwRefData;
			nNum = pBom->GetPartNum();
			BOMPART *pPart = NULL;
			ARRAY_LIST<BOMPART*> partPtrArr;
			for (pPart = pBom->EnumFirstPart(); pPart; pPart = pBom->EnumNextPart())
				partPtrArr.Append(pPart);
			CHeapSort<BOMPART*>::HeapSort(partPtrArr.m_pData, partPtrArr.GetSize(), CompareBomPartPtrByPartNo);
			for(BOMPART **ppPart=partPtrArr.GetFirst();ppPart;ppPart=partPtrArr.GetNext(), index++)
			{
				pPart = *ppPart;
				if(pPart==NULL)
					continue;
				DisplayProgress(int(100 * index / nNum));
				pItem=InsertPartToList(m_xListReport,NULL,pPart,NULL);
				pItem->m_idProp=(long)pPart;
			}
			m_sCurFile.Format("%s",(char*)pBom->m_sBomName);
			m_sRecordNum=pBom->GetPartNumStr();
		}
		else if(pInfo->itemType==ANGLE_DWG_ITEM)
		{
			DisplayProgress(0, "显示角钢图纸信息......");
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pDwg->GetJgNum();
			CAngleProcessInfo *pAngleInfo = NULL;
			ARRAY_LIST<CAngleProcessInfo*> anglePartPtrArr;
			for (pAngleInfo = pDwg->EnumFirstJg(); pAngleInfo; pAngleInfo = pDwg->EnumNextJg())
				anglePartPtrArr.append(pAngleInfo);
			CHeapSort<CAngleProcessInfo*>::HeapSort(anglePartPtrArr.m_pData, anglePartPtrArr.GetSize(), CompareAnglePtrByPartNo);
			for(CAngleProcessInfo **ppAngle=anglePartPtrArr.GetFirst();ppAngle;ppAngle=anglePartPtrArr.GetNext(), index++)
			{
				pAngleInfo = *ppAngle;
				if (pAngleInfo == NULL)
					continue;
				DisplayProgress(int(100 * index / nNum));
				pItem=InsertPartToList(m_xListReport,NULL,&pAngleInfo->m_xAngle,NULL);
				pItem->m_idProp=(long)pAngleInfo;
			}
			m_sCurFile.Format("%s",(char*)pDwg->m_sDwgName);
			m_sRecordNum.Format("%d",pDwg->GetJgNum());
		}
		else if(pInfo->itemType==PLATE_DWG_ITEM)
		{
			DisplayProgress(0, "显示钢板图纸信息......");
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pDwg->GetPlateNum();
			CPlateProcessInfo *pPlateInfo = NULL;
			ARRAY_LIST<CPlateProcessInfo*> platePtrArr;
			for (pPlateInfo = pDwg->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwg->EnumNextPlate())
				platePtrArr.append(pPlateInfo);
			CHeapSort<CPlateProcessInfo*>::HeapSort(platePtrArr.m_pData, platePtrArr.GetSize(), ComparePlatePtrByPartNo);
			for(CPlateProcessInfo **ppPlateInfo=platePtrArr.GetFirst();ppPlateInfo;ppPlateInfo=platePtrArr.GetNext(), index++)
			{
				pPlateInfo = *ppPlateInfo;
				if (pPlateInfo == NULL)
					continue;
				DisplayProgress(int(100 * index / nNum));
				pItem=InsertPartToList(m_xListReport,NULL,&pPlateInfo->xBomPlate,NULL);
				pItem->m_idProp=(long)pPlateInfo;
			}
			m_sCurFile.Format("%s",(char*)pDwg->m_sDwgName);
			m_sRecordNum.Format("%d", pDwg->GetPlateNum());
		}
		m_xListReport.Redraw();
		DisplayProgress(100);
	}
	else
	{
		DisplayProgress(0, "显示校审结果......");
		m_xListReport.DeleteAllItems();
		CProjectTowerType::COMPARE_PART_RESULT *pResult=NULL;
		CProjectTowerType* pProject=GetProject(hItem);
		m_sRecordNum.Format("%d", pProject->GetResultCount());
		//对校审结果进行排序
		ARRAY_LIST<CXhChar50> keyStrArr;
		for (pResult = pProject->EnumFirstResult(); pResult; pResult = pProject->EnumNextResult())
		{
			if (pResult->pLoftPart)
				keyStrArr.append(CXhChar50(pResult->pLoftPart->sPartNo));
			else
				keyStrArr.append(CXhChar50(pResult->pOrgPart->sPartNo));
		}
		CQuickSort<CXhChar50>::QuickSort(keyStrArr.m_pData, keyStrArr.GetSize(), CompareByPartNo);
		//
		if(m_iCompareMode==CProjectTowerType::COMPARE_BOM_FILE)
		{
			nNum = pProject->GetResultCount();
			for (index = 0; index < keyStrArr.GetSize();index++)
			{	
				DisplayProgress(int(100 * index / nNum));
				pResult = pProject->GetResult(keyStrArr[index]);
				//添加新行并设置背景色
				if (pResult->pOrgPart)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);	
					if (pResult->pLoftPart == NULL)
					{	//生技有，放样没有
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else //数据不一致
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
				}
				else
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s和%s校审",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pProject->m_xOrigBom.m_sBomName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWG)
		{
			CDwgFileInfo* pJgDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pProject->GetResultCount();
			for (index = 0; index < keyStrArr.GetSize(); index++)
			{
				DisplayProgress(int(100 * index / nNum));
				pResult = pProject->GetResult(keyStrArr[index]);
				if (pResult->pOrgPart && pResult->pOrgPart->cPartType == BOMPART::ANGLE)
				{
					CAngleProcessInfo *pAngleInfo=pJgDwg->FindAngleByPartNo(pResult->pOrgPart->sPartNo);
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
					if(pAngleInfo)
						pItem->m_idProp = (long)pAngleInfo;
					if (pResult->pLoftPart == NULL)
					{	//DWG中没有此构件
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
				}
				else if (pResult->pOrgPart == NULL && pResult->pLoftPart && pResult->pLoftPart->cPartType == BOMPART::ANGLE)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s和%s校审",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pJgDwg->m_sDwgName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWG)
		{
			CDwgFileInfo* pPlateDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pProject->GetResultCount();
			for (index = 0; index < keyStrArr.GetSize(); index++)
			{
				DisplayProgress(int(100 * index / nNum));
				pResult = pProject->GetResult(keyStrArr[index]);
				if (pResult->pOrgPart && pResult->pOrgPart->cPartType==BOMPART::PLATE)
				{
					CPlateProcessInfo *pPlateInfo = pPlateDwg->FindPlateByPartNo(pResult->pOrgPart->sPartNo);
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
					if(pPlateInfo)
						pItem->m_idProp = (long)pPlateInfo;
					if (pResult->pLoftPart == NULL)
					{	//放样表中没有此构件
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
				}
				else if (pResult->pOrgPart==NULL && pResult->pLoftPart &&pResult->pLoftPart->cPartType == BOMPART::PLATE)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s和%s校审",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pPlateDwg->m_sDwgName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS || 
			m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWGS)
		{
			if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS)
				m_sCurFile="已打开角钢DWG文件漏号检测";
			else
				m_sCurFile="已打开钢板DWG文件漏号检测";
			nNum = pProject->GetResultCount();
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult(), index++)
			{	//添加新行并设置背景色
				DisplayProgress(int(100 * index / nNum));
				if(pResult->pOrgPart)
					continue;
				CSuperGridCtrl::CTreeItem* pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
				pItem->SetBkColor(RGB(150,220,150));
			}
		}
		m_xListReport.Redraw();
		DisplayProgress(100);
	}
	UpdateData(FALSE);
}
void CRevisionDlg::RefreshTreeCtrl()
{
	CString ss;
	itemInfoList.Empty();
	m_treeCtrl.DeleteAllItems();
	HTREEITEM hItem=NULL;
	HTREEITEM hRootItem=m_treeCtrl.InsertItem("工程塔型", PRJ_IMG_ROOT, PRJ_IMG_ROOT,TVI_ROOT);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(PROJECT_GROUP,0));
	m_treeCtrl.SetItemData(hRootItem,(DWORD)pItemInfo);
	for(CProjectTowerType *pPrjTowerType=g_xUbomModel.m_xPrjTowerTypeList.GetFirst();pPrjTowerType;pPrjTowerType=g_xUbomModel.m_xPrjTowerTypeList.GetNext())
		RefreshProjectItem(hRootItem, pPrjTowerType);
}
void CRevisionDlg::RefreshProjectItem(HTREEITEM hParenItem,CProjectTowerType* pProject)
{
	if(pProject==NULL)
		return;
	CTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hProjItem,hBomGroup,hJgGroup,hPlateGroup,hSonItem;
	hProjItem=pTreeCtrl->InsertItem(pProject->m_sProjName,PRJ_IMG_MODULECASE,PRJ_IMG_MODULECASE,hParenItem);
	TREEITEM_INFO* pItemInfo=itemInfoList.append(TREEITEM_INFO(PROJECT_ITEM,(DWORD)pProject));
	pTreeCtrl->SetItemData(hProjItem,(DWORD)pItemInfo);
	//料单节点
	hBomGroup=pTreeCtrl->InsertItem("料单xls组",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_GROUP,NULL));
	pTreeCtrl->SetItemData(hBomGroup,(DWORD)pItemInfo);
	if(pProject->m_xLoftBom.GetPartNum()>0)
	{
		hSonItem=pTreeCtrl->InsertItem(pProject->m_xLoftBom.m_sBomName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xLoftBom)));
		pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
	}
	if(pProject->m_xOrigBom.GetPartNum()>0)
	{
		hSonItem=pTreeCtrl->InsertItem(pProject->m_xOrigBom.m_sBomName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xOrigBom)));
		pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
	}
	//dwg文件节点
	hJgGroup=pTreeCtrl->InsertItem("角钢dwg组",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_GROUP,NULL));
	pTreeCtrl->SetItemData(hJgGroup,(DWORD)pItemInfo);
	hPlateGroup=pTreeCtrl->InsertItem("钢板dwg组",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_GROUP,NULL));
	pTreeCtrl->SetItemData(hPlateGroup,(DWORD)pItemInfo);
	HTREEITEM hSelectItem = NULL;
	for(CDwgFileInfo* pDwgInfo=pProject->dwgFileList.GetFirst();pDwgInfo;pDwgInfo=pProject->dwgFileList.GetNext())
	{
		if(pDwgInfo->IsJgDwgInfo())
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hJgGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
			if (g_sPrintDwgFileName.EqualNoCase(pDwgInfo->m_sDwgName))
				hSelectItem = hSonItem;
		}
		else
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hPlateGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
			if (g_sPrintDwgFileName.EqualNoCase(pDwgInfo->m_sDwgName))
				hSelectItem = hSonItem;
		}
	}
	//
	pTreeCtrl->Expand(hProjItem,TVE_EXPAND);
	pTreeCtrl->Expand(hBomGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hJgGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hPlateGroup,TVE_EXPAND);
	if(hSelectItem)
		pTreeCtrl->SelectItem(hSelectItem);
	else
		pTreeCtrl->SelectItem(hProjItem);
}
void CRevisionDlg::ContextMenu(CWnd *pWnd, CPoint point)
{
	CTreeCtrl *pTreeCtrl = GetTreeCtrl();
	if (pTreeCtrl == NULL)
		return;
	//初始化菜单
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu = popMenu.GetSubMenu(0);
	while (pMenu->GetMenuItemCount() > 0)
		pMenu->DeleteMenu(0, MF_BYPOSITION);
	//添加菜单项
	CPoint scr_point = point;
	pTreeCtrl->ClientToScreen(&scr_point);
	HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo = NULL;
	if (hItem)
		pItemInfo = (TREEITEM_INFO*)pTreeCtrl->GetItemData(hItem);
	if (pItemInfo == NULL)
		return;
	if (pItemInfo->itemType == PROJECT_GROUP)	//工程塔形组
	{
		pMenu->AppendMenu(MF_STRING, ID_NEW_ITEM, "新建工程");
		pMenu->AppendMenu(MF_STRING, ID_LOAD_PROJECT, "加载工程文件");
	}
	else if (pItemInfo->itemType == PROJECT_ITEM)
	{
		pMenu->AppendMenu(MF_STRING, ID_EXPORT_PROJECT, "生成工程文件");
		if (g_xBomExport.IsVaild())
		{
			pMenu->AppendMenu(MF_SEPARATOR);
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_XLS_FILE, "导出ERP接口数据");
		}
	}
	else if (pItemInfo->itemType == BOM_GROUP)
	{
		pMenu->AppendMenu(MF_STRING, ID_IMPORT_BOM_FILE, "加载料单文件");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE))
		{	//料单校审功能
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "料单数据校审");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "导出校审结果");
		}
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_AMEND))
			pMenu->AppendMenu(MF_STRING, ID_MODIFY_ERP_FILE, "修正BOM数据");
	}
	else if (pItemInfo->itemType == BOM_ITEM)
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "删除料表");
	else if (pItemInfo->itemType == ANGLE_GROUP || pItemInfo->itemType == PLATE_GROUP)
	{
		if (pItemInfo->itemType == ANGLE_GROUP)
			pMenu->AppendMenu(MF_STRING, ID_IMPORT_ANGLE_DWG, "加载角钢DWG文件");
		else
			pMenu->AppendMenu(MF_STRING, ID_IMPORT_PLATE_DWG, "加载钢板DWG文件");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE))
		{
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "DWG漏号检测");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "导出检测结果");
		}
	}
	else if (pItemInfo->itemType == ANGLE_DWG_ITEM || pItemInfo->itemType == PLATE_DWG_ITEM)
	{
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE))
		{
			CXhChar16 sName = (pItemInfo->itemType == ANGLE_DWG_ITEM) ? "角钢" : "钢板";
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, (char*)CXhChar50("%s数据校核", (char*)sName));
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "导出校审结果");
		}
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SUM_NUM))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_PART_NUM, "更新加工数");
		if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SING_N))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_SINGLE_NUM, "更新单基数");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_WEIGHT))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_WEIGHT, "更新总重");
		if(pMenu->GetMenuItemCount()>0)
			pMenu->AppendMenu(MF_SEPARATOR);
		if (pItemInfo->itemType == ANGLE_DWG_ITEM)
			pMenu->AppendMenu(MF_STRING, ID_RETRIEVED_ANGLES, "重新提取角钢");
		else
		{
			pMenu->AppendMenu(MF_STRING, ID_RETRIEVED_PLATES, "重新提取钢板");
			pMenu->AppendMenu(MF_STRING, ID_REVISE_TEH_PLATE, "修正特定钢板");
		}
		if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_BATCH_PRINT))
			pMenu->AppendMenu(MF_STRING, ID_BATCH_PRINT_PART, "批量打印");
		if (pMenu->GetMenuItemCount() > 0)
			pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "删除文件");
	}
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, scr_point.x, scr_point.y, this);
}
//
void CRevisionDlg::OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	CXhTreeCtrl *pTreeCtrl = GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	pTreeCtrl->ScreenToClient(&HitTestInfo.pt);
	pTreeCtrl->HitTest(&HitTestInfo);
	pTreeCtrl->Select(HitTestInfo.hItem,TVGN_CARET);
	ContextMenu(pTreeCtrl,HitTestInfo.pt);
	*pResult = 0;
}
void CRevisionDlg::OnTvnSelchangedTreeContrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	if(hItem==NULL)
		return;
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if(pItemInfo->itemType==ANGLE_DWG_ITEM || pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		AcApDocument *pDoc=NULL;
		AcApDocumentIterator *pIter=acDocManager->newAcApDocumentIterator();
		for(;!pIter->done();pIter->step())
		{
			pDoc=pIter->document();
			CXhChar500 file_path;
#ifdef _ARX_2007
			file_path.Copy(_bstr_t(pDoc->fileName()));
#else
			file_path.Copy(pDoc->fileName());
#endif
			if(strstr(file_path,pDwgInfo->m_sFileName))
				break;
		}
		if(pDoc)
			acDocManager->activateDocument(pDoc);	//激活指定文件
	}
	RefreshListCtrl(hItem);
	*pResult = 0;
}
void CRevisionDlg::OnTvnBeginlabeleditTreeContrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	TV_ITEM item= pTVDispInfo->item;
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(item.hItem);
	if (pItemInfo == NULL || pItemInfo->itemType != PROJECT_ITEM)
		*pResult = 1;
	else
		*pResult = 0;
}
void CRevisionDlg::OnTvnEndlabeleditTreeContrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	*pResult = 0;

	TV_ITEM item;
	item = pTVDispInfo->item;
	CString strName = item.pszText;
	if (strName == _T(""))
		return;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo && pItemInfo->itemType == PROJECT_ITEM)
	{
		CProjectTowerType* pProject = (CProjectTowerType*)pItemInfo->dwRefData;
		pProject->m_sProjName.Copy(strName);
		m_treeCtrl.SetItemText(hSelItem, strName);
	}
}
void CRevisionDlg::OnNMClickTreeContrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	if (hSelItem == NULL)
		return;
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo && pItemInfo->itemType == PROJECT_ITEM)
	{	//修改工程名称
		m_treeCtrl.SelectItem(hSelItem);
		m_treeCtrl.EditLabel(hSelItem);
		return;
	}
	CEdit* pEdit = m_treeCtrl.GetEditControl();
	if (::IsWindow(pEdit->GetSafeHwnd()))
		pEdit->SendMessage(WM_CLOSE);
	*pResult = 0;
}
//新建项目
void CRevisionDlg::OnNewItem()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL)
		return;
	if(pItemInfo->itemType==PROJECT_GROUP)
	{	//新建塔形工程
		CProjectTowerType* pProject=g_xUbomModel.m_xPrjTowerTypeList.Add(0);
		pProject->m_sProjName.Copy("新建工程");
		RefreshProjectItem(hSelectedItem,pProject);
	}
}
//加载工程文件
void CRevisionDlg::OnLoadProjFile()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PROJECT_GROUP)
		return;
	CFileDialog dlg(TRUE,"ubm","工程文件",OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,"工程文件(*.ubm)|*.ubm||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=g_xUbomModel.m_xPrjTowerTypeList.Add(0);
	pProject->ReadProjectFile(dlg.GetPathName());
	//刷新数列表
	RefreshProjectItem(hSelectedItem,pProject);
}
//生成工程文件
void CRevisionDlg::OnExportProjFile()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PROJECT_ITEM)
		return;
	CProjectTowerType* pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	CFileDialog dlg(FALSE,"ubm",pProject->m_sProjName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"料单工程文件(*.ubm)|*.ubm||");
	if(dlg.DoModal()==IDOK)
		pProject->WriteProjectFile(dlg.GetPathName());	
}

void CRevisionDlg::OnExportXLSFile()
{
	CXhTreeCtrl *pTreeCtrl = GetTreeCtrl();
	if (pTreeCtrl == NULL)
		return;
	HTREEITEM hSelectedItem = pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if (pItemInfo == NULL || pItemInfo->itemType != PROJECT_ITEM)
		return;
	CProjectTowerType* pPrjTowerType = (CProjectTowerType*)pItemInfo->dwRefData;
	g_xBomExport.ExportExcelFile(pPrjTowerType);
}
//导入料单文件(放样料单和生计料单)
void CRevisionDlg::OnImportBomFile()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=BOM_GROUP)
		return;
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	CFileDialog dlg(TRUE,"xls","物料清单.xls",
		OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,
		"BOM文件|*.xls;*.xlsx|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|所有文件(*.*)|*.*||");
	if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE))
		dlg.m_ofn.lpstrTitle = "选择待校审的料表组";
	else
		dlg.m_ofn.lpstrTitle = "选择单个物料清单";
	if(dlg.DoModal()!=IDOK)
		return;
#ifndef _ARX_2007
	CWaitCursor waitCursor;
#endif
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE))
	{	//导入料表组
		POSITION pos = dlg.GetStartPosition();
		while (pos)
		{
			CString sFilePath = dlg.GetNextPathName(pos);
			if(pProject->IsTmaBomFile(sFilePath))
				pProject->InitBomInfo(sFilePath, TRUE);
			else if (pProject->IsErpBomFile(sFilePath))
				pProject->InitBomInfo(sFilePath, FALSE);
		}
		//添加BOM树节点
		pTreeCtrl->Expand(hSelectedItem, TVE_EXPAND);
		if (pProject->m_xLoftBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xLoftBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xLoftBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xLoftBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				logerr.Log("加载放样物料清单失败!");
			else if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
				logerr.Log("加载放样出图构件清单失败!");
			else
				logerr.Log("加载物料清单失败!");
		}
		//
		if (pProject->m_xOrigBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xOrigBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xOrigBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xOrigBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				logerr.Log("加载ERP物料清单失败!");
			else if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
				logerr.Log("加载图纸构件清单!");
			else
				logerr.Log("加载物料清单失败!");
		}		
	}
	else
	{	//只需导入一种料单
		POSITION pos = dlg.GetStartPosition();
		CString sFilePath = dlg.GetNextPathName(pos);
		pProject->InitBomInfo(sFilePath, TRUE);
		pTreeCtrl->Expand(hSelectedItem, TVE_EXPAND);
		if (pProject->m_xLoftBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xLoftBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xLoftBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xLoftBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
			logerr.Log("加载物料清单失败!");
	}	
}
//导入角钢DWG文件
void CRevisionDlg::OnImportAngleDwg()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=ANGLE_GROUP)
		return;
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	CFileDialog dlg(TRUE,"dwg","角钢加工文件.dwg",
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,//|OFN_ALLOWMULTISELECT,
					"DWG文件(*.dwg)|*.dwg|所有文件(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	CDwgFileInfo* pJgDwgInfo=pProject->AppendDwgBomInfo(dlg.GetPathName(),TRUE);
	if(pJgDwgInfo==NULL)
		return;
	//添加角钢DWG文件数节点
	hItem=FindTreeItem(hSelectedItem, pJgDwgInfo->m_sDwgName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(pJgDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pJgDwgInfo));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	}
	m_treeCtrl.SelectItem(hItem);
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	RefreshListCtrl(hItem);
}
//导入钢板DWG文件
void CRevisionDlg::OnImportPlateDwg()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PLATE_GROUP)
		return;
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	CFileDialog dlg(TRUE,"dwg","钢板加工文件.dwg",
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,//|OFN_ALLOWMULTISELECT,
					"DWG文件(*.dwg)|*.dwg|所有文件(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	CDwgFileInfo* pPlateDwgInfo=pProject->AppendDwgBomInfo(dlg.GetPathName(),FALSE);
	if(pPlateDwgInfo==NULL)
		return;
	//添加钢板DWG文件数节点
	hItem=FindTreeItem(hSelectedItem, pPlateDwgInfo->m_sDwgName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(pPlateDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM,(DWORD)pPlateDwgInfo));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	}
	m_treeCtrl.SelectItem(hItem);
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	RefreshListCtrl(hItem);
}
void CRevisionDlg::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);
}
void CRevisionDlg::OnSize(UINT nType, int cx, int cy)
{
	RECT rect;
	CWnd* pWnd=CWnd::GetDlgItem(IDC_LIST_REPORT);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
#ifdef __SUPPORT_DOCK_UI_
		rect.bottom = cy;
		rect.right = cx;
#else
		rect.bottom = cy - m_nBtmMargin;
		rect.right = cx - m_nRightMargin;
#endif
		pWnd->MoveWindow(&rect);
	}
	/*pWnd = CWnd::GetDlgItem(IDC_TREE_CONTRL);
	if (pWnd->GetSafeHwnd() != NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
#ifdef __SUPPORT_DOCK_UI_
		rect.bottom = cy;
#else
		rect.bottom = cy - m_nBtmMargin;
#endif
		pWnd->MoveWindow(&rect);
	}*/
	CDialog::OnSize(nType, cx, cy);
}
//
void CRevisionDlg::OnCompareData()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hSelItem=pTreeCtrl->GetSelectedItem();
	if(hSelItem==NULL)
		return;
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelItem);
	CProjectTowerType* pProject=GetProject(hSelItem);
	if(pProject==NULL)
	{
		AfxMessageBox("没有找到工程塔形,无法执行数据比对！");
		return;
	}
	if(pItemInfo->itemType==ANGLE_DWG_ITEM)
	{	//进行TMA料单和角钢DWG对比
		m_iCompareMode=CProjectTowerType::COMPARE_ANGLE_DWG;
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		if(pProject->CompareLoftAndAngleDwg(pDwgInfo->m_sFileName)!=1)
			return;
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{	//进行TMA料单和钢板DWG对比
		m_iCompareMode=CProjectTowerType::COMPARE_PLATE_DWG;
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		if(pProject->CompareLoftAndPlateDwg(pDwgInfo->m_sFileName)!=1)
			return;
	}
	else if(pItemInfo->itemType==BOM_GROUP)
	{	//进行放样和ERP的料单对比
		m_iCompareMode=CProjectTowerType::COMPARE_BOM_FILE;
		if(pProject->CompareOrgAndLoftParts()!=1)
			return;
	}
	else if(pItemInfo->itemType==ANGLE_GROUP)
	{	//角钢DWG文件进行漏号检测
		m_iCompareMode=CProjectTowerType::COMPARE_ANGLE_DWGS;
		if(pProject->CompareLoftAndAngleDwgs()!=1)
			return;
	}
	else if(pItemInfo->itemType==PLATE_GROUP)
	{	//钢板DWG文件进行漏号检测
		m_iCompareMode=CProjectTowerType::COMPARE_PLATE_DWGS;
		if(pProject->CompareLoftAndPlateDwgs()!=1)
			return;
	}
	RefreshListCtrl(hSelItem,TRUE);
}
//
void CRevisionDlg::OnExportCompResult()
{
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	CProjectTowerType* pProject=GetProject(hSelItem);
	if(pProject==NULL)
		return;
	if(pItemInfo->itemType==BOM_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_BOM_FILE);
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ANGLE_DWG);
	else if(pItemInfo->itemType==PLATE_DWG_ITEM && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_PLATE_DWG);
	else if(pItemInfo->itemType==ANGLE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ANGLE_DWGS);
	else if(pItemInfo->itemType==PLATE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_PLATE_DWGS);
	else
		AfxMessageBox("比对结果相同!");
}
//更新加工数
void CRevisionDlg::OnRefreshPartNum()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if(pItemInfo==NULL)
		return;
	CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
	if(pDwgInfo->IsJgDwgInfo())
		pDwgInfo->ModifyAngleDwgPartNum();
	else
		pDwgInfo->ModifyPlateDwgPartNum();
#ifdef _ARX_2007
	SendCommandToCad(L"RE ");
#else
	SendCommandToCad("RE ");
#endif
	RefreshListCtrl(hSelItem);
}
//更新单基数
void CRevisionDlg::OnRefreshSingleNum()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo->IsJgDwgInfo())
		pDwgInfo->ModifyAngleDwgSingleNum();
	else
		pDwgInfo->ModifyPlateDwgPartNum();
#ifdef _ARX_2007
	SendCommandToCad(L"RE ");
#else
	SendCommandToCad("RE ");
#endif
	RefreshListCtrl(hSelItem);
}
//
void CRevisionDlg::OnModifyErpFile()
{
	CLogErrorLife logErrLife;
	CTreeCtrl* pTree=GetTreeCtrl();
	HTREEITEM hSelItem=pTree->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTree->GetItemData(hSelItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=BOM_GROUP)
		return;
	BYTE ciMatCharPosType=1;	//0.前面|1.后面
	strcpy(BTN_ID_YES_TEXT, "件号前");
	strcpy(BTN_ID_NO_TEXT, "件号后");
	strcpy(BTN_ID_CANCEL_TEXT, "关闭");
	int nRetCode = MsgBox(adsw_acadMainWnd(), "请选择件号中简化材质字符的位置！", "修正件号", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (nRetCode == IDYES)
		ciMatCharPosType = 0;
	else if (nRetCode == IDNO)
		ciMatCharPosType = 1;
	else
		return;
	CProjectTowerType* pProject=(CProjectTowerType*)GetProject(hSelItem);
	if (pProject == NULL)
		return;
	BOOL bErpBOM = pProject->ModifyErpBomPartNo(ciMatCharPosType);
	BOOL bTmaBOM = pProject->ModifyTmaBomPartNo(ciMatCharPosType);
	if (bErpBOM && bTmaBOM)
		AfxMessageBox("ERP料单、放样料单件号数据更新完毕!");
	else if (bErpBOM)
		AfxMessageBox("ERP料单件号数据更新完毕!");
	else if (bTmaBOM)
	{
		AfxMessageBox("放样料单件号数据更新完毕!");
	}
	else
		return;
	//刷新界面
	HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hSelItem);
	if (hChildItem != NULL)
		m_treeCtrl.SelectItem(hChildItem);
}

void CRevisionDlg::OnRetrievedAngles()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != ANGLE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo==NULL || !pDwgInfo->IsJgDwgInfo())
		return;
#ifndef _ARX_2007
	CWaitCursor waitCursor;
#endif
	pDwgInfo->ExtractDwgInfo(pDwgInfo->m_sFileName, TRUE);
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnRetrievedPlates()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != PLATE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL || pDwgInfo->IsJgDwgInfo())
		return;
#ifndef _ARX_2007
	CWaitCursor waitCursor;
#endif
	pDwgInfo->ExtractDwgInfo(pDwgInfo->m_sFileName, FALSE);
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnRetrievedPlate()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != PLATE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL || pDwgInfo->IsJgDwgInfo())
		return;
	pDwgInfo->ExtractThePlate();
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnDeleteItem()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo=NULL;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CProjectTowerType *pPrjTowerType = NULL;
	if (pItemInfo->itemType == PLATE_DWG_ITEM || pItemInfo->itemType == ANGLE_DWG_ITEM)
	{
		CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
		if (pDwgInfo == NULL)
			return;
		pPrjTowerType = pDwgInfo->BelongModel();
		if (pPrjTowerType == NULL)
			return;
		for (CDwgFileInfo *pTempDwgInfo = pPrjTowerType->dwgFileList.GetFirst(); pTempDwgInfo; pTempDwgInfo = pPrjTowerType->dwgFileList.GetNext())
		{
			if (pTempDwgInfo == pDwgInfo)
			{
				pPrjTowerType->dwgFileList.DeleteCursor(TRUE);
				m_treeCtrl.DeleteItem(hSelItem);	//删除选中项
				break;
			}
		}
	}
	else if (pItemInfo->itemType == BOM_ITEM)
	{
		CBomFile *pBomFile = (CBomFile*)pItemInfo->dwRefData;
		if (pBomFile == NULL)
			return;
		pPrjTowerType = pBomFile->BelongModel();
		if (pPrjTowerType == NULL)
			return;
		if (pBomFile == &pPrjTowerType->m_xLoftBom || pBomFile == &pPrjTowerType->m_xOrigBom)
		{
			pBomFile->Empty();
			m_treeCtrl.DeleteItem(hSelItem);
		}
	}
	else if (pItemInfo->itemType == PROJECT_ITEM)
	{
		CProjectTowerType *pPrjTowerType = (CProjectTowerType*)pItemInfo->dwRefData;
		for (CProjectTowerType *pTemp = g_xUbomModel.m_xPrjTowerTypeList.GetFirst(); pTemp; pTemp = g_xUbomModel.m_xPrjTowerTypeList.GetNext())
		{
			if (pPrjTowerType == pTemp)
			{
				g_xUbomModel.m_xPrjTowerTypeList.DeleteCursor(TRUE);
				m_treeCtrl.DeleteItem(hSelItem);
			}
		}
	}
}
void CRevisionDlg::OnSearchPart()
{
	UpdateData();
	if (m_sSearchText.GetLength() <= 0)
		return;
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if (hItem == NULL || pInfo == NULL)
		return;
	long prop_id = 0;
	if (pInfo->itemType == BOM_ITEM)
	{
		CBomFile* pBomFile = (CBomFile*)pInfo->dwRefData;
		BOMPART *pPart = pBomFile->FindPart(m_sSearchText);
		prop_id = (long)pPart;
	}
	else if (pInfo->itemType == ANGLE_DWG_ITEM)
	{
		CDwgFileInfo* pDwg = (CDwgFileInfo*)pInfo->dwRefData;
		CAngleProcessInfo *pAngleInfo = pDwg->FindAngleByPartNo(m_sSearchText);
		prop_id = (long)pAngleInfo;
	}
	else if (pInfo->itemType == PLATE_DWG_ITEM)
	{
		CDwgFileInfo* pDwg = (CDwgFileInfo*)pInfo->dwRefData;
		CPlateProcessInfo *pPlateInfo = pDwg->FindPlateByPartNo(m_sSearchText);
		prop_id = (long)pPlateInfo;
	}
	CSuperGridCtrl::CTreeItem* pFindItem = m_xListReport.FindItemByPropId(prop_id, NULL);
	if (pFindItem)
		m_xListReport.SelectItem(pFindItem);
	UpdateData(FALSE);
}
void CRevisionDlg::PreSubclassWindow()
{
#ifdef __SUPPORT_DOCK_UI_
	ModifyStyle(DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION, DS_SETFONT | WS_CHILD);
	CAcUiDialog::PreSubclassWindow();
#else
	//ModifyStyle(WS_CHILD, DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU);
	CDialog::PreSubclassWindow();
#endif
}
LRESULT CRevisionDlg::OnAcadKeepFocus(WPARAM, LPARAM)
{
	return TRUE;
}

void CRevisionDlg::OnBnClickedChkGrade()
{
	UpdateData();
	g_pncSysPara.m_bCmpQualityLevel = m_bQuality;
	UpdateData(FALSE);
}

void CRevisionDlg::OnBnClickedChkMatH()
{
	UpdateData();
	g_pncSysPara.m_bEqualH_h = m_bMaterialH;
	UpdateData(FALSE);
}

void CRevisionDlg::OnEnChangeELenErr()
{
	UpdateData();
	g_pncSysPara.m_fMaxLenErr = atof(m_sLegErr);
	UpdateData(FALSE);
}
void CRevisionDlg::OnBatchPrintPart()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || (pItemInfo->itemType != ANGLE_DWG_ITEM && pItemInfo->itemType!=PLATE_DWG_ITEM))
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL)
		return;
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	g_sPrintDwgFileName.Copy(pDwgInfo->m_sFileName);
	CAcModuleResourceOverride resOverride;
	COptimalSortDlg dlg;
	dlg.SetDwgFile(pDwgInfo);
	if (g_xUbomModel.m_sJGPrintSheetName.GetLength() > 0 &&
		g_xUbomModel.m_xJgPrintCfg.m_sColIndexArr.GetLength() > 0&&
		AfxMessageBox("是否导入角钢打印清单？", MB_YESNO) == IDYES)
	{
		CFileDialog file_dlg(TRUE, "xls", "角钢打印清单.xls",
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT,
			"BOM文件|*.xls;*.xlsx|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|所有文件(*.*)|*.*||");
		if (file_dlg.DoModal() == IDOK)
		{
			POSITION pos = file_dlg.GetStartPosition();
			CString sFilePath = file_dlg.GetNextPathName(pos);
			if (dlg.InitPrintBom(sFilePath) == false)
			{
				AfxMessageBox("打印清单读取失败，请重新确认配置是否正确!");
				return;
			}
		}
	}
	if (dlg.DoModal() == IDOK)
	{
		CBatchPrint batchPrint(&dlg.m_xPrintScopyList,TRUE,dlg.m_iPrintType);
		batchPrint.Print();
	}
}
#endif
