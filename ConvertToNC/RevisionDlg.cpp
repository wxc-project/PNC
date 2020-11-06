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
#include "PNCSysSettingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef __UBOM_ONLY_
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
		CProjectTowerType *pPrjTowerType = pBom?(CProjectTowerType*)pBom->BelongModel():NULL;
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
	else if (pItemInfo->itemType == ANGLE_DWG_ITEM ||
		pItemInfo->itemType == PLATE_DWG_ITEM ||
		pItemInfo->itemType == PART_DWG_ITEM)
	{
		CAngleProcessInfo* pJgInfo = (pItem->m_idPropType == BOMPART::ANGLE) ? (CAngleProcessInfo*)pItem->m_idProp : NULL;
		CPlateProcessInfo* pPlateInfo = (pItem->m_idPropType == BOMPART::PLATE) ? (CPlateProcessInfo*)pItem->m_idProp : NULL;
		if (pJgInfo)
		{
			scope = pJgInfo->GetCADEntScope();
			bRetCode = TRUE;
		}
		else if (pPlateInfo)
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
	ON_COMMAND(ID_IMPORT_DWG_FILE, OnImportDwgFile)
	ON_COMMAND(ID_COMPARE_DATA,OnCompareData)
	ON_COMMAND(ID_EXPORT_COMPARE_RESULT,OnExportCompResult)
	ON_COMMAND(ID_REFRESH_PART_NUM,OnRefreshPartNum)
	ON_COMMAND(ID_REFRESH_SINGLE_NUM, OnRefreshSingleNum)
	ON_COMMAND(ID_REFRESH_GUIGE, OnRefreshSpec)
	ON_COMMAND(ID_REFRESH_MAT, OnRefreshMat)
	ON_COMMAND(ID_MODIFY_ERP_FILE,OnModifyErpFile)
	ON_COMMAND(ID_BATCH_PRINT_PART, OnBatchPrintPart)
	ON_COMMAND(ID_EMPTY_PLATE_RETRIEVED_RESLUT, OnEmptyRetrievedPlates)
	ON_COMMAND(ID_EMPTY_ANGLE_RETRIEVED_RESLUT, OnEmptyRetrievedAngles)
	ON_COMMAND(ID_BATCH_RETRIEVED_PLATES, OnBatchRetrievedPlates)
	ON_COMMAND(ID_BATCH_RETRIEVED_ANGLES, OnBatchRetrievedAngles)
	ON_COMMAND(ID_DELETE_ITEM,OnDeleteItem)
	ON_BN_CLICKED(IDC_BTN_SEARCH, OnSearchPart)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDC_CHK_GRADE, OnBnClickedChkGrade)
	ON_EN_CHANGE(IDC_E_LEN_ERR, OnEnChangeELenErr)
	ON_BN_CLICKED(IDC_CHK_MAT_H, &CRevisionDlg::OnBnClickedChkMatH)
	ON_BN_CLICKED(IDC_BTN_SETTINGS, &CRevisionDlg::OnBnClickedSettings)
	ON_CBN_SELCHANGE(IDC_CMB_CLIENT, &CRevisionDlg::OnCbnSelchangeCmbClient)
	ON_CBN_SELCHANGE(IDC_CMB_CFG, &CRevisionDlg::OnCbnSelchangeCmbCfg)
END_MESSAGE_MAP()

// CRevisionDlg 消息处理程序
BOOL CRevisionDlg::OnInitDialog()
{
#ifdef __SUPPORT_DOCK_UI_
	CAcUiDialog::OnInitDialog();
#else
	CDialog::OnInitDialog();
#endif
	m_bQuality = g_xUbomModel.m_bCmpQualityLevel;
	m_bMaterialH = g_xUbomModel.m_bEqualH_h;
	m_sLegErr.Format("%.1f", g_xUbomModel.m_fMaxLenErr);
	//初始化列表框
	m_xListReport.EnableSortItems(true,true);
	m_xListReport.SetGridLineColor(RGB(220, 220, 220));
	m_xListReport.SetEvenRowBackColor(RGB(224, 237, 236));
	m_xListReport.SetCompareItemFunc(FireCompareItem);
	m_xListReport.SetItemChangedFunc(FireItemChanged);
	m_xListReport.EmptyColumnHeader();
	RefreshListCtrl(NULL);
	//初始化树列表
	m_treeCtrl.ModifyStyle(0, TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_EDITLABELS);
	RefreshTreeCtrl();
	//初始化客户下拉框
#ifdef __ALFA_TEST_
	//记录所有用户，用于测试
	CComboBox* pClientCmb = (CComboBox*)GetDlgItem(IDC_CMB_CLIENT);
	std::map<CString, int>::iterator iterS = g_xUbomModel.m_xMapClientInfo.begin();
	std::map<CString, int>::iterator iterE = g_xUbomModel.m_xMapClientInfo.end();
	for (; iterS != iterE; ++iterS)
		pClientCmb->AddString(iterS->first);
	int nSel = pClientCmb->FindString(0, g_xUbomModel.m_sCustomizeName);
	if (nSel >= 0)
		pClientCmb->SetCurSel(nSel);
	//初始化配置下拉框
	CComboBox* pCfgCmb = (CComboBox*)GetDlgItem(IDC_CMB_CFG);
	multimap<int, CString>::iterator iterF = g_xUbomModel.m_xMapClientCfgFile.lower_bound(g_xUbomModel.m_uiCustomizeSerial);
	multimap<int, CString>::iterator iterL = g_xUbomModel.m_xMapClientCfgFile.upper_bound(g_xUbomModel.m_uiCustomizeSerial);
	for (; iterF != iterL; iterF++)
		pCfgCmb->AddString(iterF->second);
	pCfgCmb->SetCurSel(0);
#else
	CRect tree_rect, cmb_rect;
	GetDlgItem(IDC_CMB_CLIENT)->GetWindowRect(cmb_rect);
	m_treeCtrl.GetWindowRect(&tree_rect);
	ScreenToClient(&tree_rect);
	tree_rect.bottom += cmb_rect.Height() + 5;
	//
	GetDlgItem(IDC_S_CLIENT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CMB_CLIENT)->ShowWindow(SW_HIDE);
	if (g_xUbomModel.m_xMapClientCfgFile.count(g_xUbomModel.m_uiCustomizeSerial) <= 1)
	{	//只有一个配置不进行显示
		GetDlgItem(IDC_S_CFG)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CMB_CFG)->ShowWindow(SW_HIDE);
		tree_rect.bottom += cmb_rect.Height() + 5;
	}
	else
	{
		CComboBox* pCfgCmb = (CComboBox*)GetDlgItem(IDC_CMB_CFG);
		multimap<int, CString>::iterator iterF = g_xUbomModel.m_xMapClientCfgFile.lower_bound(g_xUbomModel.m_uiCustomizeSerial);
		multimap<int, CString>::iterator iterL = g_xUbomModel.m_xMapClientCfgFile.upper_bound(g_xUbomModel.m_uiCustomizeSerial);
		for (; iterF != iterL; iterF++)
			pCfgCmb->AddString(iterF->second);
		pCfgCmb->SetCurSel(0);
	}
	m_treeCtrl.MoveWindow(tree_rect);
#endif
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
		pItemInfo->itemType==PLATE_GROUP || pItemInfo->itemType == PART_GROUP)
	{
		HTREEITEM hParentItem=pTreeCtrl->GetParentItem(hItem);
		pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hParentItem);
		if(pItemInfo&&pItemInfo->itemType==PROJECT_ITEM)
			pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	}
	else if(pItemInfo->itemType==BOM_ITEM)
	{
		CBomFile* pBom=(CBomFile*)pItemInfo->dwRefData;
		pProject=(CProjectTowerType*)pBom->BelongModel();
	}
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM ||
		pItemInfo->itemType == PLATE_DWG_ITEM ||
		pItemInfo->itemType == PART_DWG_ITEM)
	{
		CDwgFileInfo* pDwg=(CDwgFileInfo*)pItemInfo->dwRefData;
		pProject=pDwg->BelongModel();
	}
	return pProject;
}
static CSuperGridCtrl::CTreeItem *InsertPartToList(CSuperGridCtrl &list,CSuperGridCtrl::CTreeItem *pParentItem,
	BOMPART *pPart,CHashStrList<BOOL> *pHashBoolByPropName=NULL,BOOL bUpdate=FALSE,BYTE modifyFlag=0)
{
	COLORREF clr=RGB(230,100,230);
	PART_ANGLE* pBomJg = (pPart->cPartType == BOMPART::ANGLE) ? (PART_ANGLE*)pPart : NULL;
	CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
	for (size_t i = 0; i < g_xBomCfg.GetBomTitleCount(); i++)
	{
		if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_PN))
		{	//件号
			lpInfo->SetSubItemText(i, pPart->sPartNo, TRUE);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_SPEC))
		{	//规格
			lpInfo->SetSubItemText(i, pPart->sSpec, TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_SPEC))
				lpInfo->SetSubItemColor(i, clr);
			if ((modifyFlag & CAngleProcessInfo::MODIFY_DES_GUIGE) > 0)
				lpInfo->SetSubItemColor(i, RGB(255, 90, 90));
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_MAT))
		{	//材质
			lpInfo->SetSubItemText(i, CBomModel::QueryMatMarkIncQuality(pPart), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_MAT))
				lpInfo->SetSubItemColor(i, clr);
			if ((modifyFlag & CAngleProcessInfo::MODIFY_DES_MAT) > 0)
				lpInfo->SetSubItemColor(i, RGB(255, 90, 90));
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_LEN))
		{	//长度
			lpInfo->SetSubItemText(i, CXhChar50("%.0f", pPart->length), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_LEN))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_SING_N))
		{	//单基数
			lpInfo->SetSubItemText(i, CXhChar50("%d", pPart->GetPartNum()), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_SING_N))
				lpInfo->SetSubItemColor(i, clr);
			if ((modifyFlag & CAngleProcessInfo::MODIFY_SINGLE_NUM) > 0)
				lpInfo->SetSubItemColor(i, RGB(255, 90, 90));
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_MANU_N))
		{	//加工数
			lpInfo->SetSubItemText(i, CXhChar50("%d", pPart->feature1), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_MANU_N))
				lpInfo->SetSubItemColor(i, clr);
			if ((modifyFlag & CAngleProcessInfo::MODIFY_MANU_NUM) > 0)
				lpInfo->SetSubItemColor(i, RGB(255, 90, 90));
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_SING_W))
		{	//单重
			lpInfo->SetSubItemText(i, CXhChar50("%g", pPart->fPieceWeight), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_SING_W))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_MANU_W))
		{	//加工重量
			lpInfo->SetSubItemText(i, CXhChar50("%g", pPart->fSumWeight), TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_MANU_W))
				lpInfo->SetSubItemColor(i, clr);
			if ((modifyFlag & CAngleProcessInfo::MODIFY_SUM_WEIGHT) > 0)
				lpInfo->SetSubItemColor(i, RGB(255, 90, 90));
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_NOTES))
		{	//备注
			lpInfo->SetSubItemText(i, pPart->sNotes, TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_NOTES))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_WELD))
		{	//焊接
			lpInfo->SetSubItemText(i, pPart->bWeldPart ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_WELD))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_ZHI_WAN))
		{	//制弯
			lpInfo->SetSubItemText(i, (pPart->siZhiWan > 0) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_ZHI_WAN))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_CUT_ANGLE))
		{	//切角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutAngle) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_CUT_ANGLE))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_PUSH_FLAT))
		{	//打扁
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->nPushFlat > 0) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_PUSH_FLAT))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_CUT_BER))
		{	//铲背
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutBer) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_CUT_BER))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_CUT_ROOT))
		{	//刨角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bCutRoot) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_CUT_ROOT))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_KAI_JIAO))
		{	//开角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bKaiJiao) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_KAI_JIAO))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_HE_JIAO))
		{	//合角
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bHeJiao) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_HE_JIAO))
				lpInfo->SetSubItemColor(i, clr);
		}
		else if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_FOO_NAIL))
		{	//带脚钉
			lpInfo->SetSubItemText(i, (pBomJg && pBomJg->bHasFootNail) ? "*" : "", TRUE);
			if (pHashBoolByPropName&&pHashBoolByPropName->GetValue(CBomConfig::KEY_FOO_NAIL))
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
	//更新标题列
	if (hItem == NULL)
	{
		m_xListReport.EmptyColumnHeader();
		for (size_t i = 0; i < g_xBomCfg.GetBomTitleCount(); i++)
			m_xListReport.AddColumnHeader(g_xBomCfg.GetTitleName(i), g_xBomCfg.GetTitleWidth(i));
		m_xListReport.InitListCtrl();
		m_xListReport.DeleteAllItems();
		m_xListReport.Redraw();
		return;
	}
	//更新内容
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
			pInfo->itemType != PLATE_DWG_ITEM &&
			pInfo->itemType != PART_DWG_ITEM)
			return;
		m_xListReport.DeleteAllItems();
		if(pInfo->itemType==BOM_ITEM)
		{
			DisplayCadProgress(0, "显示料单信息......");
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
				DisplayCadProgress(int(100 * index / nNum));
				pItem=InsertPartToList(m_xListReport,NULL,pPart,NULL);
				pItem->m_idProp=(long)pPart;
			}
			m_sCurFile.Format("%s",(char*)pBom->m_sBomName);
			m_sRecordNum=pBom->GetPartNumStr();
			DisplayCadProgress(100);
		}
		else
		{
			CDwgFileInfo* pDwg = (CDwgFileInfo*)pInfo->dwRefData;
			int nJgNum = pDwg->GetAngleNum();
			int nPlateNum = pDwg->GetPlateNum();
			if (nJgNum>0)
			{
				DisplayCadProgress(0, "显示角钢图纸信息......");
				CAngleProcessInfo *pAngleInfo = NULL;
				ARRAY_LIST<CAngleProcessInfo*> anglePartPtrArr;
				for (pAngleInfo = pDwg->EnumFirstJg(); pAngleInfo; pAngleInfo = pDwg->EnumNextJg())
					anglePartPtrArr.append(pAngleInfo);
				CHeapSort<CAngleProcessInfo*>::HeapSort(anglePartPtrArr.m_pData, anglePartPtrArr.GetSize(), CompareAnglePtrByPartNo);
				for (CAngleProcessInfo **ppAngle = anglePartPtrArr.GetFirst(); ppAngle; ppAngle = anglePartPtrArr.GetNext(), index++)
				{
					pAngleInfo = *ppAngle;
					if (pAngleInfo == NULL)
						continue;
					DisplayCadProgress(int(100 * index / nJgNum));
					pItem = InsertPartToList(m_xListReport, NULL, &pAngleInfo->m_xAngle, NULL, FALSE, pAngleInfo->m_ciModifyState);
					pItem->m_idProp = (long)pAngleInfo;
					pItem->m_idPropType = BOMPART::ANGLE;
				}
				DisplayCadProgress(100);
			}
			if (nPlateNum>0)
			{
				DisplayCadProgress(0, "显示钢板图纸信息......");
				CDwgFileInfo* pDwg = (CDwgFileInfo*)pInfo->dwRefData;
				CPlateProcessInfo *pPlateInfo = NULL;
				ARRAY_LIST<CPlateProcessInfo*> platePtrArr;
				for (pPlateInfo = pDwg->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwg->EnumNextPlate())
					platePtrArr.append(pPlateInfo);
				CHeapSort<CPlateProcessInfo*>::HeapSort(platePtrArr.m_pData, platePtrArr.GetSize(), ComparePlatePtrByPartNo);
				for (CPlateProcessInfo **ppPlateInfo = platePtrArr.GetFirst(); ppPlateInfo; ppPlateInfo = platePtrArr.GetNext(), index++)
				{
					pPlateInfo = *ppPlateInfo;
					if (pPlateInfo == NULL)
						continue;
					DisplayCadProgress(int(100 * index / nPlateNum));
					pItem = InsertPartToList(m_xListReport, NULL, &pPlateInfo->xBomPlate, NULL, FALSE, pPlateInfo->m_ciModifyState);
					pItem->m_idProp = (long)pPlateInfo;
					pItem->m_idPropType = BOMPART::PLATE;
				}
				DisplayCadProgress(100);
			}
			m_sCurFile.Format("%s", (char*)pDwg->m_sDwgName);
			CString sNum;
			if (nPlateNum > 0 && nJgNum > 0)
				sNum.Format("%d=%d+%d", nJgNum+nPlateNum, nJgNum, nPlateNum);
			else if (nPlateNum > 0)
				sNum.Format("%d=%d", nPlateNum, nPlateNum);
			else if (nJgNum > 0)
				sNum.Format("%d=%d", nJgNum, nJgNum);
			else
				sNum.Format("0");
			m_sRecordNum = sNum;
		}
		m_xListReport.Redraw();
	}
	else
	{
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
			DisplayCadProgress(0, "显示校审结果......");
			nNum = pProject->GetResultCount();
			for (index = 0; index < keyStrArr.GetSize();index++)
			{	
				DisplayCadProgress(int(100 * index / nNum));
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
			DisplayCadProgress(100);
		}
		else if (m_iCompareMode == CProjectTowerType::COMPARE_DWG_FILE)
		{
			CDwgFileInfo* pDwgFile = (CDwgFileInfo*)pInfo->dwRefData;
			nNum = pProject->GetResultCount();
			if (pDwgFile->GetAngleNum()>0)
			{
				DisplayCadProgress(0, "显示校审结果......");
				for (index = 0; index < keyStrArr.GetSize(); index++)
				{
					DisplayCadProgress(int(100 * index / nNum));
					pResult = pProject->GetResult(keyStrArr[index]);
					if (pResult->pOrgPart && pResult->pOrgPart->cPartType == BOMPART::ANGLE)
					{
						CAngleProcessInfo *pAngleInfo = pDwgFile->FindAngleByPartNo(pResult->pOrgPart->sPartNo);
						pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
						if (pAngleInfo)
						{
							pItem->m_idProp = (long)pAngleInfo;
							pItem->m_idPropType = BOMPART::ANGLE;
						}
						if (pResult->pLoftPart == NULL)
						{	//DWG中没有此构件
							pItem->m_bStrikeout = TRUE;
							pItem->SetBkColor(RGB(140, 140, 255));
						}
						else
						{
							pItem = InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
							if (pAngleInfo)
							{
								pItem->m_idProp = (long)pAngleInfo;
								pItem->m_idPropType = BOMPART::ANGLE;
							}
						}
					}
					else if (pResult->pOrgPart == NULL && pResult->pLoftPart && pResult->pLoftPart->cPartType == BOMPART::ANGLE)
					{
						pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
						pItem->SetBkColor(RGB(128, 128, 255));
					}
				}
				DisplayCadProgress(100);
			}
			if (pDwgFile->GetPlateNum()>0)
			{
				DisplayCadProgress(0, "显示校审结果......");
				for (index = 0; index < keyStrArr.GetSize(); index++)
				{
					DisplayCadProgress(int(100 * index / nNum));
					pResult = pProject->GetResult(keyStrArr[index]);
					if (pResult->pOrgPart && pResult->pOrgPart->cPartType == BOMPART::PLATE)
					{
						CPlateProcessInfo *pPlateInfo = pDwgFile->FindPlateByPartNo(pResult->pOrgPart->sPartNo);
						pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
						if (pPlateInfo)
						{
							pItem->m_idProp = (long)pPlateInfo;
							pItem->m_idPropType = BOMPART::PLATE;
						}
						if (pResult->pLoftPart == NULL)
						{	//放样表中没有此构件
							pItem->m_bStrikeout = TRUE;
							pItem->SetBkColor(RGB(140, 140, 255));
						}
						else
						{
							pItem = InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
							if (pPlateInfo)
							{
								pItem->m_idProp = (long)pPlateInfo;
								pItem->m_idPropType = BOMPART::PLATE;
							}
						}
					}
					else if (pResult->pOrgPart==NULL && pResult->pLoftPart &&pResult->pLoftPart->cPartType == BOMPART::PLATE)
					{
						pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
						pItem->SetBkColor(RGB(128, 128, 255));
					}
				}
				DisplayCadProgress(100);
			}
			m_sCurFile.Format("%s和%s校审",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pDwgFile->m_sDwgName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ALL_DWGS)
		{
			m_sCurFile="已打开DWG文件漏号检测";
			DisplayCadProgress(0, "显示校审结果......");
			nNum = pProject->GetResultCount();
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult(), index++)
			{	//添加新行并设置背景色
				DisplayCadProgress(int(100 * index / nNum));
				if(pResult->pOrgPart)
					continue;
				CSuperGridCtrl::CTreeItem* pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
				pItem->SetBkColor(RGB(150,220,150));
			}
			DisplayCadProgress(100);
		}
		m_xListReport.Redraw();
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
	HTREEITEM hProjItem, hBomGroup, hJgGroup, hPlateGroup, hPartGroup, hSonItem;
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
	hPartGroup = pTreeCtrl->InsertItem("混合dwg组", PRJ_IMG_FILEGROUP, PRJ_IMG_FILEGROUP, hProjItem);
	pItemInfo = itemInfoList.append(TREEITEM_INFO(PART_GROUP, NULL));
	pTreeCtrl->SetItemData(hPartGroup, (DWORD)pItemInfo);
	HTREEITEM hSelectItem = NULL;
	for(CDwgFileInfo* pDwgInfo=pProject->dwgFileList.GetFirst();pDwgInfo;pDwgInfo=pProject->dwgFileList.GetNext())
	{
		if(pDwgInfo->GetAngleNum()>0 && pDwgInfo->GetPlateNum()<=0)
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hJgGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
			if (g_sPrintDwgFileName.EqualNoCase(pDwgInfo->m_sDwgName))
				hSelectItem = hSonItem;
		}
		if(pDwgInfo->GetPlateNum()>0 && pDwgInfo->GetAngleNum()<=0)
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hPlateGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
			if (g_sPrintDwgFileName.EqualNoCase(pDwgInfo->m_sDwgName))
				hSelectItem = hSonItem;
		}
		if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() > 0)
		{
			hSonItem = pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName, PRJ_IMG_FILE, PRJ_IMG_FILE, hPartGroup);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(PART_DWG_ITEM, (DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem, (DWORD)pItemInfo);
			if (g_sPrintDwgFileName.EqualNoCase(pDwgInfo->m_sDwgName))
				hSelectItem = hSonItem;
		}
	}
	//
	pTreeCtrl->Expand(hProjItem,TVE_EXPAND);
	pTreeCtrl->Expand(hBomGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hJgGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hPlateGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hPartGroup, TVE_EXPAND);
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
		if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE)||
			g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE))
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
	else if (pItemInfo->itemType == ANGLE_GROUP || 
			 pItemInfo->itemType == PLATE_GROUP ||
			 pItemInfo->itemType == PART_GROUP)
	{
		pMenu->AppendMenu(MF_STRING, ID_IMPORT_DWG_FILE, "加载DWG文件");
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
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "进行数据校核");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "导出校审结果");
		}
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SUM_NUM))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_PART_NUM, "更新加工数");
		if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SING_N))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_SINGLE_NUM, "更新单基数");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_WEIGHT))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_WEIGHT, "更新总重");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SPEC))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_GUIGE, "更新规格");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_MAT))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_MAT, "更新材质");
		if(pMenu->GetMenuItemCount()>0)
			pMenu->AppendMenu(MF_SEPARATOR);
		if (pItemInfo->itemType == ANGLE_DWG_ITEM)
			pMenu->AppendMenu(MF_STRING, ID_BATCH_RETRIEVED_ANGLES, "分批提取角钢");
		else
			pMenu->AppendMenu(MF_STRING, ID_BATCH_RETRIEVED_PLATES, "分批提取钢板");
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "删除文件");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_BATCH_PRINT))
		{
			pMenu->AppendMenu(MF_SEPARATOR);
			pMenu->AppendMenu(MF_STRING, ID_BATCH_PRINT_PART, "批量打印");
		}
	}
	else if (pItemInfo->itemType == PART_DWG_ITEM)
	{
		pMenu->AppendMenu(MF_STRING, ID_BATCH_RETRIEVED_ANGLES, "分批提取角钢");
		pMenu->AppendMenu(MF_STRING, ID_EMPTY_ANGLE_RETRIEVED_RESLUT, "清空角钢数据");
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_BATCH_RETRIEVED_PLATES, "分批提取钢板");
		pMenu->AppendMenu(MF_STRING, ID_EMPTY_PLATE_RETRIEVED_RESLUT, "清空钢板数据");
		pMenu->AppendMenu(MF_SEPARATOR);
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE))
		{
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "进行数据校核");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "导出校审结果");
		}
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SUM_NUM))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_PART_NUM, "更新加工数");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SING_N))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_SINGLE_NUM, "更新单基数");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_WEIGHT))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_WEIGHT, "更新总重");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_SPEC))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_GUIGE, "更新规格");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_AMEND_MAT))
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_MAT, "更新材质");
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "删除文件");
		if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_BATCH_PRINT))
		{
			pMenu->AppendMenu(MF_SEPARATOR);
			pMenu->AppendMenu(MF_STRING, ID_BATCH_PRINT_PART, "批量打印");
		}
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
		"BOM文件|*.xls;*.xlsx;*.xlsm|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|Excel(*.xlsm)|*.xlsm|所有文件(*.*)|*.*||");
	if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE))
		dlg.m_ofn.lpstrTitle = "选择待校审的料表组";
	else
		dlg.m_ofn.lpstrTitle = "选择单个物料清单";
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_BOM_COMPARE))
	{	//导入料表组
		POSITION pos = dlg.GetStartPosition();
		while (pos)
		{
			CString sFilePath = dlg.GetNextPathName(pos);
			if(g_xBomCfg.IsTmaBomFile(sFilePath))
				pProject->InitBomInfo(sFilePath, TRUE);
			else if (g_xBomCfg.IsErpBomFile(sFilePath))
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
			logerr.Log("加载%s原始物料清单失败!",(char*)g_xBomCfg.m_xTmaTblCfg.m_sFileTypeKeyStr);
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
			logerr.Log("加载%s对比物料清单失败!",(char*)g_xBomCfg.m_xErpTblCfg.m_sFileTypeKeyStr);	
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
//导入DWG文件
void CRevisionDlg::OnImportDwgFile()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if (pItemInfo == NULL)
		return;
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	CFileDialog dlg(TRUE,"dwg","DWG文件",
		OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,//|OFN_ALLOWMULTISELECT,
		"DWG文件(*.dwg)|*.dwg|所有文件(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	CDwgFileInfo* pDwgFile =pProject->AppendDwgBomInfo(dlg.GetPathName());
	if(pDwgFile ==NULL)
		return;
	if(pItemInfo->itemType == ANGLE_GROUP && g_xUbomModel.m_bExtractAnglesWhenOpenFile)
		pDwgFile->RetrieveAngles();
	if (pItemInfo->itemType == PLATE_GROUP && g_xUbomModel.m_bExtractPltesWhenOpenFile)
		pDwgFile->RetrievePlates();
	//添加角钢DWG文件数节点
	HTREEITEM hItem = FindTreeItem(hSelectedItem, pDwgFile->m_sDwgName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(pDwgFile->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		if(pItemInfo->itemType==ANGLE_GROUP)
			pItemInfo = itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM, (DWORD)pDwgFile));
		else if(pItemInfo->itemType==PLATE_GROUP)
			pItemInfo = itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM, (DWORD)pDwgFile));
		else
			pItemInfo = itemInfoList.append(TREEITEM_INFO(PART_DWG_ITEM, (DWORD)pDwgFile));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	}
	m_treeCtrl.SelectItem(hItem);
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	RefreshListCtrl(hItem);
}
//
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
	if (pItemInfo->itemType == BOM_GROUP)
	{	//进行放样和ERP的料单对比
		m_iCompareMode=CProjectTowerType::COMPARE_BOM_FILE;
		if(pProject->CompareOrgAndLoftParts()!=1)
			return;
	}
	if (pItemInfo->itemType == ANGLE_GROUP ||
		pItemInfo->itemType == PLATE_GROUP ||
		pItemInfo->itemType == PART_GROUP)
	{
		m_iCompareMode = CProjectTowerType::COMPARE_ALL_DWGS;
		int iRet = 1;
		if (pItemInfo->itemType == ANGLE_GROUP)
			iRet = pProject->CompareLoftAndPartDwgs(0);
		else if (pItemInfo->itemType == PLATE_GROUP)
			iRet = pProject->CompareLoftAndPartDwgs(1);
		else
			iRet = pProject->CompareLoftAndPartDwgs(2);
		if (iRet != 1)
			return;
	}
	if (pItemInfo->itemType == ANGLE_DWG_ITEM ||
		pItemInfo->itemType == PLATE_DWG_ITEM ||
		pItemInfo->itemType == PART_DWG_ITEM)
	{
		CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
		m_iCompareMode = CProjectTowerType::COMPARE_DWG_FILE;
		if (pProject->CompareLoftAndPartDwg(pDwgInfo->m_sFileName) != 1)
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
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_DWG_FILE);
	else if(pItemInfo->itemType==PLATE_DWG_ITEM && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_DWG_FILE);
	else if (pItemInfo->itemType==PART_DWG_ITEM && pProject->GetResultCount() > 0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_DWG_FILE);
	else if(pItemInfo->itemType==ANGLE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ALL_DWGS);
	else if(pItemInfo->itemType==PLATE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ALL_DWGS);
	else if (pItemInfo->itemType==PART_GROUP && pProject->GetResultCount() > 0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ALL_DWGS);
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
	if(pDwgInfo->GetAngleNum()>0)
		pDwgInfo->ModifyAngleDwgPartNum();
	if (pDwgInfo->GetPlateNum()>0)
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
	if (pDwgInfo->GetAngleNum()>0)
		pDwgInfo->ModifyAngleDwgSingleNum();
	if (pDwgInfo->GetPlateNum()>0)
		pDwgInfo->ModifyPlateDwgPartNum();
#ifdef _ARX_2007
	SendCommandToCad(L"RE ");
#else
	SendCommandToCad("RE ");
#endif
	RefreshListCtrl(hSelItem);
}
//更新规格
void CRevisionDlg::OnRefreshSpec()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo->GetAngleNum() > 0)
		pDwgInfo->ModifyAngleDwgSpec();
	if (pDwgInfo->GetPlateNum() > 0)
		pDwgInfo->ModifyPlateDwgSpec();
#ifdef _ARX_2007
	SendCommandToCad(L"RE ");
#else
	SendCommandToCad("RE ");
#endif
	RefreshListCtrl(hSelItem);
}
//更新材质
void CRevisionDlg::OnRefreshMat()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo->GetAngleNum() > 0)
		pDwgInfo->ModifyAngleDwgMaterial();
	if (pDwgInfo->GetPlateNum() > 0)
		pDwgInfo->ModifyPlateDwgMaterial();
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
		pPrjTowerType->DeleteDwgBomInfo(pDwgInfo);
		m_treeCtrl.DeleteItem(hSelItem);	//删除选中项
	}
	else if (pItemInfo->itemType == BOM_ITEM)
	{
		CBomFile *pBomFile = (CBomFile*)pItemInfo->dwRefData;
		if (pBomFile == NULL)
			return;
		pPrjTowerType = (CProjectTowerType*)pBomFile->BelongModel();
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
	g_xUbomModel.m_bCmpQualityLevel = m_bQuality;
	UpdateData(FALSE);
}

void CRevisionDlg::OnBnClickedChkMatH()
{
	UpdateData();
	g_xUbomModel.m_bEqualH_h = m_bMaterialH;
	UpdateData(FALSE);
}

void CRevisionDlg::OnEnChangeELenErr()
{
	UpdateData();
	g_xUbomModel.m_fMaxLenErr = atof(m_sLegErr);
	UpdateData(FALSE);
}
static COptimalSortDlg printDlg;
void CRevisionDlg::OnBatchPrintPart()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
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
	printDlg.SetDwgFile(pDwgInfo);
	if (g_xBomCfg.m_xPrintTblCfg.IsValid() &&
		pDwgInfo->PrintBomPartCount()<=0 &&
		AfxMessageBox("是否导入角钢打印清单？", MB_YESNO) == IDYES)
	{
		CFileDialog file_dlg(TRUE, "xls", "角钢打印清单.xls",
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT,
			"BOM文件|*.xls;*.xlsm;*.xlsx;|Excel(*.xls)|*.xls|Excel(*.xlsm)|*.xlsm|Excel(*.xlsx)|*.xlsx|所有文件(*.*)|*.*||");
		if (file_dlg.DoModal() == IDOK)
		{
			POSITION pos = file_dlg.GetStartPosition();
			CString sFilePath = file_dlg.GetNextPathName(pos);
			if (printDlg.InitPrintBom(sFilePath) == false)
			{
				AfxMessageBox("打印清单读取失败，请重新确认配置是否正确!");
				return;
			}
		}
	}
	if (printDlg.DoModal() == IDOK)
	{
		CBatchPrint batchPrint(&printDlg.m_xPrintScopyList,TRUE, printDlg.m_iPrintType);
		batchPrint.Print();
	}
}
void CRevisionDlg::OnBatchRetrievedPlates()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL)
		return;
	pDwgInfo->RetrievePlates(TRUE);
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnBatchRetrievedAngles()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL)
		return;
	pDwgInfo->RetrieveAngles(TRUE);
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnEmptyRetrievedPlates()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL)
		return;
	pDwgInfo->EmptyPlateList();
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnEmptyRetrievedAngles()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL)
		return;
	pDwgInfo->EmptyJgList();
	RefreshListCtrl(hSelItem);
}
void CRevisionDlg::OnBnClickedSettings()
{
#ifdef _ARX_2007
	CWndShowLife show(this);
#endif
	CAcModuleResourceOverride resOverride;
	CLogErrorLife logErrLife;
	CPNCSysSettingDlg dlg;
	dlg.DoModal();
}

void CRevisionDlg::OnCbnSelchangeCmbClient()
{
	UpdateData();
	CComboBox *pCMB = (CComboBox*)GetDlgItem(IDC_CMB_CLIENT);
	if (pCMB->GetCount()<=1)
		return;
	CString sKey;
	int iCurSel = pCMB->GetCurSel();
	pCMB->GetLBText(iCurSel, sKey);
	map<CString, int>::iterator iter = g_xUbomModel.m_xMapClientInfo.find(sKey);
	if (iter == g_xUbomModel.m_xMapClientInfo.end())
		return;
	g_xUbomModel.m_uiCustomizeSerial = iter->second;
	//更新配置文件
	CComboBox* pCfgCmb = (CComboBox*)GetDlgItem(IDC_CMB_CFG);
	pCfgCmb->ResetContent();
	multimap<int, CString>::iterator iterS = g_xUbomModel.m_xMapClientCfgFile.lower_bound(g_xUbomModel.m_uiCustomizeSerial);
	multimap<int, CString>::iterator iterE = g_xUbomModel.m_xMapClientCfgFile.upper_bound(g_xUbomModel.m_uiCustomizeSerial);
	for (; iterS != iterE; iterS++)
		pCfgCmb->AddString(iterS->second);
	pCfgCmb->SetCurSel(0);
	if (pCfgCmb->GetCount() > 0)
		OnCbnSelchangeCmbCfg();
}

void CRevisionDlg::OnCbnSelchangeCmbCfg()
{
	UpdateData();
	CString sCfgName;
	CComboBox* pCfgCmb = (CComboBox*)GetDlgItem(IDC_CMB_CFG);
	int iCurSel = pCfgCmb->GetCurSel();
	pCfgCmb->GetLBText(iCurSel, sCfgName);
	//读取配置文件
	char APP_PATH[MAX_PATH] = "";
	GetAppPath(APP_PATH);
	CString cfg_file(APP_PATH);
	cfg_file += "配置\\";
	cfg_file += sCfgName;
	ImportUbomConfigFile(cfg_file);
#ifdef __ALFA_TEST_
	int nWidth = g_xBomCfg.InitBomTitle();
	RefreshTreeCtrl();
	RefreshListCtrl(NULL);
	//更新CAD显示标题
	CXhChar100 sWndText("UBOM");
	sWndText.Append(g_xUbomModel.m_sCustomizeName, '-');
	::SetWindowText(adsw_acadMainWnd(), sWndText);
#else
	//同一个客户显示的数据列相同，仅刷新界面即可
	RefreshTreeCtrl();
	RefreshListCtrl(NULL);
#endif
}
#endif
