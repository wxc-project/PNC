// SystemSettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PNCSysSettingDlg.h"
#include "PropertyListOper.h"
#include "PNCSysPara.h"
#include "IdentifyInfoDlg.h"
#include "AddBoltBlockDlg.h"
#include "LogFile.h"
#include "XhLicAgent.h"
#include "FilterLayerDlg.h"
#include "CadToolFunc.h"
#include "DrawDamBoard.h"
#include "resource.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//

static const BYTE PROPGROUP_BY_RULE = 0;
static const BYTE PROPGROUP_BY_TEXT = 1;
static const BYTE PROPGROUP_BY_BOLT = 2;
static const BYTE SCHEMA_BY_Enable = 0;
static const BYTE SCHEMA_BY_SchemaName = 1;
static const BYTE SCHEMA_BY_DimStyle = 2;
static const BYTE SCHEMA_BY_PnKey = 3;
static const BYTE SCHEMA_BY_ThickKey = 4;
static const BYTE SCHEMA_BY_MatKey = 5;
static const BYTE SCHEMA_BY_PnNumKey = 6;
static const BYTE SCHEMA_BY_FrontBendKey = 7;
static const BYTE SCHEMA_BY_ReverseBendKey = 8;
void UpdateTxtDimProperty(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	if(pParentItem==NULL)
		return ;
	CPropertyListOper<CPNCSysPara> oper(pPropList,&g_pncSysPara);
	pPropList->DeleteAllSonItems(pParentItem);
	if(g_pncSysPara.m_sPnKey.Length>0)
		oper.InsertCmbEditPropItem(pParentItem,"m_sPnKey","","","",-1,TRUE);			//件号标识符
	if(g_pncSysPara.m_sThickKey.Length>0)
		oper.InsertCmbEditPropItem(pParentItem,"m_sThickKey", "", "", "", -1, TRUE);	//厚度标识符
	if(g_pncSysPara.m_sMatKey.Length>0)
		oper.InsertCmbEditPropItem(pParentItem,"m_sMatKey", "", "", "", -1, TRUE);		//材质标识符
	if(g_pncSysPara.m_sPnNumKey.Length>0)
		oper.InsertCmbEditPropItem(pParentItem,"m_sPnNumKey", "", "", "", -1, TRUE);	//件数标识符
}
void UpdateFilterLayerProperty(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	if(pParentItem==NULL)
		return;
	CPropertyListOper<CPNCSysPara> oper(pPropList,&g_pncSysPara);
	pPropList->DeleteAllSonItems(pParentItem);
	CPNCSysPara::LAYER_ITEM* pItem=NULL;
	for(pItem=g_pncSysPara.EnumFirst();pItem;pItem=g_pncSysPara.EnumNext())
	{
		if(!pItem->m_bMark)
			continue;
		CItemInfo* lpInfo = new CItemInfo();
		lpInfo->m_controlType=PIT_EDIT;
		lpInfo->m_strPropName="图层名称";
		lpInfo->m_strPropHelp="默认过滤的图层名称";
		CPropTreeItem* pPropItem=pPropList->InsertItem(pParentItem,lpInfo, -1,TRUE);
		pPropItem->m_idProp = (long)pItem;
		pPropItem->m_dwPropGroup=pParentItem->m_dwPropGroup;
		pPropItem->m_lpNodeInfo->m_strPropValue.Format("%s",(char*)pItem->m_sLayer);
		pPropItem->SetReadOnly();
	}
}

void UpdateBoltDimProperty(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	if(pParentItem==NULL)
		return ;
	CPropertyListOper<CPNCSysPara> oper(pPropList,&g_pncSysPara);
	pPropList->DeleteAllSonItems(pParentItem);
	CPropTreeItem* pPropItem=NULL,*pSonItem=NULL;
	for(BOLT_BLOCK *pBoltD=g_pncSysPara.hashBoltDList.GetFirst();pBoltD;pBoltD=g_pncSysPara.hashBoltDList.GetNext())
	{
		CXhChar50 valueStr;
		CItemInfo* lpInfo = new CItemInfo();
		lpInfo->m_controlType = PIT_BUTTON;
		lpInfo->m_buttonType = BDT_COMMON;
		lpInfo->m_sButtonName = "删除标识";
		lpInfo->m_strPropName = "螺栓标识";
		lpInfo->m_strPropHelp = "螺栓标识";
		pPropItem = pPropList->InsertItem(pParentItem,lpInfo, -1,TRUE);
		pPropItem->m_idProp = (long)pBoltD;
		pPropItem->m_dwPropGroup=pParentItem->m_dwPropGroup;
		//
		lpInfo = new CItemInfo();
		lpInfo->m_controlType = PIT_EDIT;
		lpInfo->m_strPropName = "图块名称";
		lpInfo->m_strPropHelp = "图块名称";
		pSonItem = pPropList->InsertItem(pPropItem, lpInfo, -1, TRUE);
		pSonItem->m_idProp = g_pncSysPara.GetPropID("block_name");
		pSonItem->m_dwPropGroup = pParentItem->m_dwPropGroup;
		if (g_pncSysPara.GetPropValueStr(pSonItem->m_idProp, valueStr, 50, pSonItem))
			pSonItem->m_lpNodeInfo->m_strPropValue = valueStr;
		//
		lpInfo = new CItemInfo();
		lpInfo->m_controlType = PIT_EDIT;
		lpInfo->m_strPropName = "螺栓直径";
		lpInfo->m_strPropHelp = "螺栓直径";
		pSonItem = pPropList->InsertItem(pPropItem, lpInfo, -1, TRUE);
		pSonItem->m_idProp = g_pncSysPara.GetPropID("bolt_d");
		pSonItem->m_dwPropGroup = pParentItem->m_dwPropGroup;
		if (g_pncSysPara.GetPropValueStr(pSonItem->m_idProp, valueStr, 50, pSonItem))
			pSonItem->m_lpNodeInfo->m_strPropValue = valueStr;
		//
		lpInfo = new CItemInfo();
		lpInfo->m_controlType = PIT_EDIT;
		lpInfo->m_strPropName = "孔径";
		lpInfo->m_strPropHelp = "孔径";
		pSonItem = pPropList->InsertItem(pPropItem, lpInfo, -1, TRUE);
		pSonItem->m_idProp = g_pncSysPara.GetPropID("hole_d");
		pSonItem->m_dwPropGroup = pParentItem->m_dwPropGroup;
		if (g_pncSysPara.GetPropValueStr(pSonItem->m_idProp, valueStr, 50, pSonItem))
			pSonItem->m_lpNodeInfo->m_strPropValue = valueStr;
	}
}
static BOOL ModifySystemSettingValue(CPropertyList	*pPropList, CPropTreeItem *pItem, CString &valueStr)
{
	CPropertyListOper<CPNCSysPara> oper(pPropList, &g_pncSysPara);
	CLogErrorLife logErrLife;
	if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iDimStyle"))
		g_pncSysPara.m_iDimStyle = valueStr[0] - '0';
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sPnKey"))
		g_pncSysPara.m_sPnKey.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sThickKey"))
		g_pncSysPara.m_sThickKey.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sMatKey"))
		g_pncSysPara.m_sMatKey.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sPnNumKey"))
		g_pncSysPara.m_sPnNumKey.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("block_name"))
	{
		if (pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.GetValue(valueStr);
			if (pBoltD)
			{
				logerr.Log("已存在此图块名称");
				return FALSE;
			}
			else
			{
				pBoltD = (BOLT_BLOCK*)pItem->m_pParent->m_idProp;
				g_pncSysPara.hashBoltDList.ModifyKeyStr(pBoltD->sBlockName, valueStr);
				pBoltD->sBlockName.Copy(valueStr);
			}
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("bolt_d"))
	{
		if (pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD = (BOLT_BLOCK*)pItem->m_pParent->m_idProp;
			if (pBoltD)
				pBoltD->diameter = atoi(valueStr);
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("hole_d"))
	{
		if (pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD = (BOLT_BLOCK*)pItem->m_pParent->m_idProp;
			if (pBoltD)
				pBoltD->hole_d = atof(valueStr);
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_fMapScale"))
		g_pncSysPara.m_fMapScale = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bIncDeformed"))
	{
		if (valueStr.Compare("是") == 0)
			g_pncSysPara.m_bIncDeformed = true;
		else
			g_pncSysPara.m_bIncDeformed = false;
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bMKPos"))
	{
		if (valueStr.Compare("是") == 0)
			g_pncSysPara.m_bMKPos = true;
		else
			g_pncSysPara.m_bMKPos = false;
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bReplaceSH"))
	{
		if (valueStr.Compare("是") == 0)
			g_pncSysPara.m_bReplaceSH = true;
		else
			g_pncSysPara.m_bReplaceSH = false;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_bReplaceSH)
			oper.InsertCmbListPropItem(pItem, "m_nReplaceHD", "", "", "", -1, TRUE);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nReplaceHD"))
		g_pncSysPara.m_nReplaceHD = atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_iPPiMode"))
		g_pncSysPara.m_iPPiMode=valueStr[0]-'0';
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("AxisXCalType"))
		g_pncSysPara.m_iAxisXCalType=valueStr[0]-'0';
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_bAutoLayout1"))
	{
		if (valueStr.Compare("是") == 0)
			g_pncSysPara.m_bAutoLayout = CPNCSysPara::LAYOUT_PRINT;
		else
			g_pncSysPara.m_bAutoLayout = CPNCSysPara::LAYOUT_NONE;
		pPropList->DeleteAllSonItems(pItem);
		if(g_pncSysPara.m_bAutoLayout==CPNCSysPara::LAYOUT_PRINT)
		{
			oper.InsertEditPropItem(pItem,"m_nMapWidth","","",-1,TRUE);
			oper.InsertEditPropItem(pItem,"m_nMapLength","","",-1,TRUE);
			oper.InsertEditPropItem(pItem,"m_nMinDistance","","",-1,TRUE);
		}
		//
		CXhChar200 sText;
		long idProp = CPNCSysPara::GetPropID("m_bAutoLayout2");
		if (g_pncSysPara.GetPropValueStr(idProp, sText) > 0)
			pPropList->SetItemPropValue(idProp, sText);
		//
		CPropTreeItem *pOtherItem = pPropList->FindItemByPropId(idProp, NULL);
		if (pOtherItem)
		{
			pPropList->DeleteAllSonItems(pOtherItem);
			if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			{
				oper.InsertEditPropItem(pOtherItem, "CDrawDamBoard::BOARD_HEIGHT", "", "", -1, TRUE);
				oper.InsertCmbListPropItem(pOtherItem, "CDrawDamBoard::m_bDrawAllBamBoard", "", "", "", -1, TRUE);
				oper.InsertEditPropItem(pOtherItem, "m_nMkRectLen", "", "", -1, TRUE);
				oper.InsertEditPropItem(pOtherItem, "m_nMkRectWidth", "", "", -1, TRUE);
				DisplayPartListDockBar();
			}
			else
				HidePartListDockBar();
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bAutoLayout2"))
	{
		if (valueStr.Compare("是") == 0)
			g_pncSysPara.m_bAutoLayout = CPNCSysPara::LAYOUT_SEG;
		else
			g_pncSysPara.m_bAutoLayout = CPNCSysPara::LAYOUT_NONE;
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
		{
			oper.InsertEditPropItem(pItem, "CDrawDamBoard::BOARD_HEIGHT", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "CDrawDamBoard::m_bDrawAllBamBoard", "", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "m_nMkRectLen", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "m_nMkRectWidth", "", "", -1, TRUE);
			DisplayPartListDockBar();
		}
		else
			HidePartListDockBar();
		CXhChar200 sText;
		long idProp = CPNCSysPara::GetPropID("m_bAutoLayout1");
		if (g_pncSysPara.GetPropValueStr(idProp, sText) > 0)
			pPropList->SetItemPropValue(idProp, sText);
		CPropTreeItem *pOtherItem = pPropList->FindItemByPropId(idProp, NULL);
		if (pOtherItem)
		{
			pPropList->DeleteAllSonItems(pOtherItem);
			if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_PRINT)
			{
				oper.InsertEditPropItem(pOtherItem, "m_nMapWidth", "", "", -1, TRUE);
				oper.InsertEditPropItem(pOtherItem, "m_nMapLength", "", "", -1, TRUE);
				oper.InsertEditPropItem(pOtherItem, "m_nMinDistance", "", "", -1, TRUE);
			}
		}
	}
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMapWidth"))
		g_pncSysPara.m_nMapWidth=atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMapLength"))
		g_pncSysPara.m_nMapLength=atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMinDistance"))
		g_pncSysPara.m_nMinDistance=atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("CDrawDamBoard::BOARD_HEIGHT"))
	{
		CDrawDamBoard::BOARD_HEIGHT = atoi(valueStr);
		CPartListDlg *pPartListDlg = GetPartListDlgPtr();
		if (pPartListDlg&&g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			pPartListDlg->m_xDamBoardManager.DrawAllDamBoard(&model);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("CDrawDamBoard::m_bDrawAllBamBoard"))
	{
		CDrawDamBoard::m_bDrawAllBamBoard = valueStr[0] - '0';
		CLockDocumentLife lockDocument;
		CPartListDlg *pPartListDlg = GetPartListDlgPtr();
		if (pPartListDlg&&g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			pPartListDlg->m_xDamBoardManager.DrawAllDamBoard(&model);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMkRectLen"))
		g_pncSysPara.m_nMkRectLen = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMkRectWidth"))
		g_pncSysPara.m_nMkRectWidth = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("layer_mode"))
	{
		g_pncSysPara.m_iLayerMode = valueStr[0] - '0';
		UpdateFilterLayerProperty(pPropList, pItem);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_ciBoltRecogMode"))
	{
		g_pncSysPara.m_ciBoltRecogMode = valueStr[0] - '0';
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iRecogMode"))
	{
		g_pncSysPara.m_ciRecogMode = valueStr[0] - '0';
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
		{	//按颜色识别
			oper.InsertCmbColorPropItem(pItem, "m_iProfileColorIndex", "", "", "", -1, TRUE);
			oper.InsertCmbColorPropItem(pItem, "m_iBendLineColorIndex", "", "", "", -1, TRUE);
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//按图层识别
			CPropTreeItem *pPropItem = oper.InsertPopMenuItem(pItem, "layer_mode", "", "", "", -1, TRUE);
			UpdateFilterLayerProperty(pPropList, pPropItem);
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//按线型识别
			oper.InsertCmbListPropItem(pItem, "m_iProfileLineTypeName", "", "", "", -1, TRUE);
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileLineTypeName"))
		g_pncSysPara.m_sProfileLineType.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileColorIndex") ||
		pItem->m_idProp == CPNCSysPara::GetPropID("m_iBendLineColorIndex"))
	{
		COLORREF curClr = 0;
		char tem_str[100] = "";
		sprintf(tem_str, "%s", valueStr);
		memmove(tem_str, tem_str + 3, 97);//跳过RGB
		sscanf(tem_str, "%X", &curClr);
		if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileColorIndex"))
			g_pncSysPara.m_ciProfileColorIndex = GetNearestACI(curClr);
		else //if(pItem->m_idProp==CPNCSysPara::GetPropID("m_iBendLineColorIndex"))
			g_pncSysPara.m_ciBendLineColorIndex = GetNearestACI(curClr);
	}
	return TRUE;
}
static BOOL ButtonClickSystemSetting(CPropertyList *pPropList, CPropTreeItem* pItem)
{
	CPropertyListOper<CPNCSysPara> oper(pPropList, &g_pncSysPara);
	if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMember"))
	{
		CIdentifyInfoDlg dlg;
		if (dlg.DoModal() != IDOK)
			return FALSE;
		//
		if (dlg.m_arrIsCanUse[0] && dlg.m_sPnKey.GetLength() > 0)
			g_pncSysPara.m_sPnKey.Copy(dlg.m_sPnKey);
		else
			g_pncSysPara.m_sPnKey.Empty();
		//
		if (dlg.m_arrIsCanUse[1] && dlg.m_sThickKey.GetLength() > 0)
			g_pncSysPara.m_sThickKey.Copy(dlg.m_sThickKey);
		else
			g_pncSysPara.m_sThickKey.Empty();
		//
		if (dlg.m_arrIsCanUse[2] && dlg.m_sMatKey.GetLength() > 0)
			g_pncSysPara.m_sMatKey.Copy(dlg.m_sMatKey);
		else
			g_pncSysPara.m_sMatKey.Empty();
		//
		if (dlg.m_arrIsCanUse[3] && dlg.m_sNumKey.GetLength() > 0)
			g_pncSysPara.m_sPnNumKey.Copy(dlg.m_sNumKey);
		else
			g_pncSysPara.m_sPnNumKey.Empty();
		//
		pPropList->SetItemPropValue(pItem->m_idProp, CXhChar16("%d", g_pncSysPara.GetKeyMemberNum()));
		UpdateTxtDimProperty(pPropList, pItem);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("bolt_dim_set"))
	{
		CAddBoltBlockDlg dlg;
		if (dlg.DoModal() != IDOK)
			return FALSE;
		if (strlen(dlg.m_sBlockName) <= 0 || (strlen(dlg.m_sBoltD) <= 0 && strlen(dlg.m_sHoleD) <= 0))
		{
			logerr.Log("螺栓图块名称或螺栓直径为空");
			return FALSE;
		}
		BOLT_BLOCK* pBoltD = g_pncSysPara.hashBoltDList.GetValue(dlg.m_sBlockName);
		if (pBoltD)
		{
			logerr.Log("已存在此螺栓图块");
			return FALSE;
		}
		pBoltD = g_pncSysPara.hashBoltDList.Add(dlg.m_sBlockName);
		pBoltD->sBlockName.Copy(dlg.m_sBlockName);
		if (dlg.m_sBoltD.GetLength() > 0)
			pBoltD->diameter = atoi(dlg.m_sBoltD);
		if (dlg.m_sHoleD.GetLength() > 0)
			pBoltD->hole_d = atof(dlg.m_sHoleD);
		//
		UpdateBoltDimProperty(pPropList, pItem);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("layer_mode"))
	{
		if (g_pncSysPara.m_iLayerMode == 0)
		{
			CFilterLayerDlg dlg;
			if(dlg.DoModal()!=IDOK)
				return FALSE;
		}
		UpdateFilterLayerProperty(pPropList, pItem);
	}
	else
	{
		for (BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.GetFirst(); pBoltD; pBoltD = g_pncSysPara.hashBoltDList.GetNext())
		{
			if ((long)pBoltD != pItem->m_idProp)
				continue;
			if (IDOK == MessageBox(NULL, "确定要删除选中螺栓标识吗？", "提示", IDOK))
			{
				g_pncSysPara.hashBoltDList.DeleteNode(pBoltD->sBlockName);
				pPropList->DeleteItemByPropId(pItem->m_idProp);
				return FALSE;
			}
		}
	}
	return TRUE;
}
BOOL FireSystemSettingPopMenuClick(CPropertyList* pPropList, CPropTreeItem* pItem, CString sMenuName, int iMenu)
{
	if (pItem->m_idProp == CPNCSysPara::GetPropID("layer_mode"))
	{
		g_pncSysPara.m_iLayerMode = sMenuName[0] - '0';
		if (g_pncSysPara.m_iLayerMode == 0)
		{
			CFilterLayerDlg dlg;
			if (dlg.DoModal() != IDOK)
				return FALSE;
		}
		pPropList->SetItemPropValue(pItem->m_idProp, sMenuName);
		UpdateFilterLayerProperty(pPropList, pItem);
		return TRUE;
	}
	return FALSE;
}
BOOL FirePickColor(CPropertyList* pPropList, CPropTreeItem* pItem, COLORREF &clr)
{
	CPNCSysSettingDlg *pParaDlg = (CPNCSysSettingDlg*)pPropList->GetParent();
	if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iBendLineColorIndex") ||
		pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileColorIndex"))
	{
		pParaDlg->m_idEventProp = pItem->m_idProp;	//记录触发事件的属性ID
		pParaDlg->m_arrCmdPickPrompt.RemoveAll();
#ifdef AFX_TARG_ENU_ENGLISH
		pParaDlg->m_arrCmdPickPrompt.Add("\nplease select an line <Enter confirm>:\n");
#else
		pParaDlg->m_arrCmdPickPrompt.Add("\n请选择一根直线拾取颜色<Enter确认>:\n");
#endif
		pParaDlg->SelectEntObj();
	}
	return FALSE;
}
// CPNCSysSettingDlg 对话框

IMPLEMENT_DYNAMIC(CPNCSysSettingDlg, CDialog)

CPNCSysSettingDlg::CPNCSysSettingDlg(CWnd* pParent /*=NULL*/)
#ifdef __PNC_
	: CCADCallBackDlg(CPNCSysSettingDlg::IDD, pParent)
#else
	: CDialog(CPNCSysSettingDlg::IDD, pParent)
#endif
{
	if (m_listCtrlSysSetting.GetSafeHwnd())
		m_listCtrlSysSetting.ShowWindow(SW_HIDE);
}

CPNCSysSettingDlg::~CPNCSysSettingDlg()
{
}

void CPNCSysSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BOX, m_propList);
	DDX_Control(pDX, IDC_TAB_GROUP, m_ctrlPropGroup);
	DDX_Control(pDX, IDC_LIST_SYSTEM_SETTING_DLG, m_listCtrlSysSetting);
}

void CPNCSysSettingDlg::OnPNCSysDel()
{
	int iCurSel = m_ctrlPropGroup.GetCurSel();
	int nCols = m_listCtrlSysSetting.GetItemCount();//总数
	int iRows = m_listCtrlSysSetting.GetSelectionMark();//获得选 中行的行标
	if (nCols != iRows + 1 && iCurSel == PROPGROUP_BY_TEXT)
	{
		if (g_pncSysPara.m_recogSchemaList[iRows].m_bEditable)
			MessageBox("默认行不能删除！", "删除信息提示");
		else
		{
			if (IDOK == MessageBox("确定要删除吗？", "提示", IDOK))
			{
				m_listCtrlSysSetting.DeleteItem(iRows);
				g_pncSysPara.m_recogSchemaList.DeleteAt(iRows);
			}
		}
	}
	else if (iCurSel == PROPGROUP_BY_BOLT)
	{
		CSuperGridCtrl::CTreeItem *pCurSelItem = m_listCtrlSysSetting.GetTreeItem(iRows);
		CSuperGridCtrl::CTreeItem *pParentItem = m_listCtrlSysSetting.GetParentItem(pCurSelItem);
		CString strTemp = m_listCtrlSysSetting.GetItemText(iRows, 0);
		if (pParentItem == NULL && pCurSelItem)//点击父节点
		{
			if (IDOK == MessageBox("注意：确定要删除整个分组吗？", "提示", IDOK))
			{
				pCurSelItem->DeleteSubItemText(&m_listCtrlSysSetting, iRows);
				for (int j = iRows + 1; j < nCols; j++)
				{
					CString boltKey = m_listCtrlSysSetting.GetItemText(j, 0);
					if (!boltKey.IsEmpty() || boltKey.GetLength() > 0)
						g_pncSysPara.hashBoltDList.DeleteNode(boltKey);
					else
						break;
				}
				m_listCtrlSysSetting.DeleteAllSonItems(pCurSelItem);
				m_listCtrlSysSetting.DeleteItem(iRows);
			}
		}
		else if (pParentItem != NULL && pCurSelItem)//点击子节点
		{
			if (IDOK == MessageBox("确定要删除吗？", "提示", IDOK))
			{
				CString boltKey = m_listCtrlSysSetting.GetItemText(iRows, 1);
				m_listCtrlSysSetting.DeleteItem(iRows);
				g_pncSysPara.hashBoltDList.DeleteNode(boltKey);
			}
		}
	}
	else
		MessageBox("最后一行不能删除！", "删除信息提示");
}
void CPNCSysSettingDlg::OnPNCSysAdd()
{
	int iCurSel = m_ctrlPropGroup.GetCurSel();
	if (iCurSel == PROPGROUP_BY_BOLT)
	{
		int iRows = m_listCtrlSysSetting.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pCurSelItem = m_listCtrlSysSetting.GetTreeItem(iRows);
		CSuperGridCtrl::CTreeItem *pParentItem = m_listCtrlSysSetting.GetParentItem(pCurSelItem);
		if (pParentItem == NULL && pCurSelItem)
			pParentItem = pCurSelItem;
		if (pParentItem == NULL)
		{
			POSITION pos = m_listCtrlSysSetting.GetRootTailPosition();
			pParentItem = m_listCtrlSysSetting.GetPrevRoot(pos);
		}
		if (pParentItem == NULL)
			return;

		CListCtrlItemInfo* lpInfoItem = new CListCtrlItemInfo();
		lpInfoItem->SetSubItemText(1, _T(""));
		lpInfoItem->SetSubItemText(2, _T(""));
		lpInfoItem->SetSubItemText(3, _T(""));
		lpInfoItem->SetCheck(TRUE);
		CSuperGridCtrl::CTreeItem* pItem = m_listCtrlSysSetting.InsertItem(pParentItem, lpInfoItem, -1, true);
		m_listCtrlSysSetting.SelectItem(pItem, 1, true, true);
	}
}
void CPNCSysSettingDlg::OnPNCSysGroupDel()
{
	OnPNCSysDel();
}
void CPNCSysSettingDlg::OnPNCSysGroupAdd()
{
	CListCtrlItemInfo* lpInfo = new CListCtrlItemInfo();
	lpInfo->SetSubItemText(0, _T(""));
	CSuperGridCtrl::CTreeItem* pParentItem = m_listCtrlSysSetting.InsertRootItem(lpInfo, TRUE);
	hashGroupByItemName.SetValue(" ", pParentItem);
	pParentItem->m_bHideChildren = FALSE;
	CListCtrlItemInfo* lpInfoItem = new CListCtrlItemInfo();
	lpInfoItem->SetSubItemText(1, _T(""));
	lpInfoItem->SetSubItemText(2, _T(""));
	lpInfoItem->SetSubItemText(3, _T(""));
	lpInfoItem->SetCheck(TRUE);
	CSuperGridCtrl::CTreeItem* pItem = m_listCtrlSysSetting.InsertItem(pParentItem, lpInfoItem, -1, true);
}
static BOOL FireLButtonDblclk(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem, int iSubItem)
{

	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	int nCurSel = pSysSettingDlg->m_ctrlPropGroup.GetCurSel();
	int iRows = pListCtrl->GetSelectedItem();//获取选中行
	int nCols = pListCtrl->GetHeaderCtrl()->GetItemCount();//列数
	if (nCurSel == PROPGROUP_BY_TEXT && iRows < g_pncSysPara.m_recogSchemaList.GetNodeNum())
	{
		if (iSubItem == 0)//双击第一列
		{
			//所有的m_bEnable设为false，清空选中项
			for (RECOG_SCHEMA *pBoltD = g_pncSysPara.m_recogSchemaList.GetFirst(); pBoltD; pBoltD = g_pncSysPara.m_recogSchemaList.GetNext())
			{
				pBoltD->m_bEnable = false;
			}
			RECOG_SCHEMA *Schema = g_pncSysPara.m_recogSchemaList.GetByIndex(iRows);
			Schema->m_bEnable = true;
			pSysSettingDlg->OnSelchangeTabGroup(NULL, NULL);
		}

	}
	else if (iSubItem = 1 || nCurSel == PROPGROUP_BY_BOLT)
	{
		CSuperGridCtrl::CTreeItem* pGroupItem = pListCtrl->GetTreeItem(iRows);
		pGroupItem->m_bHideChildren = FALSE;
	}
	return TRUE;
}
static void InsertRecogSchemaItem(CSuperGridCtrl* pListCtrl, RECOG_SCHEMA *pSchema)
{
	RECOG_SCHEMA schema;
	BOOL bEditable = TRUE;
	bool bEnable = TRUE;
	if (pSchema == NULL)
	{
		bEnable = FALSE;
		pSchema = &schema;
		bEditable = FALSE;
	}
	else
		bEditable = pSchema->m_bEditable;
	CListCtrlItemInfo* lpInfo = new CListCtrlItemInfo();
	if (bEnable)
		lpInfo->SetSubItemText(0, _T(pSchema->m_bEnable ? "√" : ""), true);
	else
		lpInfo->SetSubItemText(0, "", true);
	lpInfo->SetSubItemText(1, _T(pSchema->m_sSchemaName), bEditable);
	lpInfo->AddSubItemText(pSchema->m_iDimStyle == 0 ? "单行" : pSchema->m_iDimStyle == 1 ? "多行" : "", bEditable);
	lpInfo->SetControlType(2, GCT_CMB_LIST);
	lpInfo->SetListItemsStr(2, "单行|多行|");
	lpInfo->AddSubItemText(pSchema->m_sPnKey, bEditable);
	lpInfo->SetControlType(3, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(3, "#|件号:|件号：|");
	lpInfo->AddSubItemText(pSchema->m_sThickKey, bEditable);
	lpInfo->SetControlType(4, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(4, "-|板厚:|板厚：|厚度:|厚度：|");
	lpInfo->AddSubItemText(pSchema->m_sMatKey, bEditable);
	lpInfo->SetControlType(5, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(5, "Q|材质:|材质：|");
	lpInfo->AddSubItemText(pSchema->m_sPnNumKey, bEditable);
	lpInfo->SetControlType(6, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(6, "件数:|件数：|数量:|数量：|");
	lpInfo->AddSubItemText(pSchema->m_sFrontBendKey, bEditable);
	lpInfo->SetControlType(7, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(7, "正曲|外曲");
	lpInfo->AddSubItemText(pSchema->m_sReverseBendKey, bEditable);
	lpInfo->SetControlType(8, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(8, "反曲|内曲");
	CSuperGridCtrl::CTreeItem* pGroupItem = pListCtrl->InsertRootItem(lpInfo);
	if (pSchema != &schema)
		pGroupItem->m_idProp = (long)pSchema;
	else
		pGroupItem->m_idProp = 0;
}
static void InsertBoltBolckItem(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pGroupItem, BOLT_BLOCK *pSchema)
{
	BOLT_BLOCK schema;
	if (pSchema == NULL)
		pSchema = &schema;
	CListCtrlItemInfo* lpInfoItem = new CListCtrlItemInfo();
	lpInfoItem->SetSubItemText(1, _T(pSchema->sBlockName));
	CXhChar16 hole_d;
	sprintf(hole_d, "%d", pSchema->diameter);
	lpInfoItem->SetSubItemText(2, _T(hole_d));
	sprintf(hole_d, "%.3lf", pSchema->hole_d);
	lpInfoItem->SetSubItemText(3, _T(hole_d));
	lpInfoItem->SetCheck(TRUE);
	CSuperGridCtrl::CTreeItem* pItem = pListCtrl->InsertItem(pGroupItem, lpInfoItem);
	if (pSchema != &schema)
		pItem->m_idProp = (long)pSchema;
	else
		pItem->m_idProp = 0;
}

static BOOL FireValueModify(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pSelItem,
	int iSubItem, CString& sTextValue)
{
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();

	int nCurSel = pSysSettingDlg->m_ctrlPropGroup.GetCurSel();
	int iRows = pListCtrl->GetSelectedItem();
	int nCols = pListCtrl->GetItemCount();//总数
	//在文字
	if (nCurSel == PROPGROUP_BY_TEXT)
	{
		pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
		CString m_sSchemaName = pListCtrl->GetItemText(iRows, SCHEMA_BY_SchemaName);
		CString m_sPnKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_PnKey);
		CString m_sThickKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_ThickKey);
		CString m_sMatKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_MatKey);
		CString m_ModifyText = pListCtrl->GetItemText(iRows, iSubItem);
		if (iRows < g_pncSysPara.m_recogSchemaList.GetNodeNum())
		{
			//CSuperGridCtrl::CTreeItem* pGroupItem= pListCtrl->GetTreeItem(iRows);
			RECOG_SCHEMA* pSchema = (RECOG_SCHEMA*)pSelItem->m_idProp;
			if (iSubItem == SCHEMA_BY_SchemaName)
				pSchema->m_sSchemaName = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_DimStyle)
				pSchema->m_iDimStyle = atoi(!strcmp(m_ModifyText, "单行") ? "0" : "1");
			else if (iSubItem == SCHEMA_BY_PnKey)
				pSchema->m_sPnKey = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_ThickKey)
				pSchema->m_sThickKey = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_MatKey)
				pSchema->m_sMatKey = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_PnNumKey)
				pSchema->m_sPnNumKey = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_FrontBendKey)
				pSchema->m_sFrontBendKey = m_ModifyText;
			else if (iSubItem == SCHEMA_BY_ReverseBendKey)
				pSchema->m_sReverseBendKey = m_ModifyText;
		}
		else if (iRows >= g_pncSysPara.m_recogSchemaList.GetNodeNum())
		{
			if (!m_sSchemaName.IsEmpty() && !m_sPnKey.IsEmpty() && !m_sThickKey.IsEmpty() && !m_sMatKey.IsEmpty())
			{
				RECOG_SCHEMA* pSchema = g_pncSysPara.m_recogSchemaList.append();
				pSchema->m_bEnable = FALSE;
				pSchema->m_bEditable = FALSE;
				pSchema->m_sSchemaName = m_sSchemaName;
				CString m_iDimStyle = pListCtrl->GetItemText(iRows, SCHEMA_BY_DimStyle);
				pSchema->m_iDimStyle = atoi(!strcmp(m_iDimStyle, "单行") ? "0" : "1");
				pSchema->m_sPnKey = m_sPnKey;
				pSchema->m_sThickKey = m_sThickKey;
				pSchema->m_sMatKey = m_sMatKey;
				pSchema->m_sPnNumKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_PnNumKey);
				pSchema->m_sFrontBendKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_FrontBendKey);
				pSchema->m_sReverseBendKey = pListCtrl->GetItemText(iRows, SCHEMA_BY_ReverseBendKey);
				pSelItem->m_idProp = (long)pSchema;
				InsertRecogSchemaItem(pListCtrl, NULL);
			}
		}
	}
	else if (nCurSel == PROPGROUP_BY_BOLT)
	{
		CString strTempByGroupName = pListCtrl->GetItemText(iRows, 0);
		CString strTempByBlockName = pListCtrl->GetItemText(iRows, 1);

		CSuperGridCtrl::CTreeItem *pCurSelItem = pListCtrl->GetTreeItem(iRows);
		CSuperGridCtrl::CTreeItem *pParentItem = pListCtrl->GetParentItem(pCurSelItem);

		pCurSelItem->m_bHideChildren = FALSE;
		if (pParentItem == NULL && pCurSelItem)//点击父节点
		{
			int count = pCurSelItem->GetChildCount();

			for (int j = iRows + 1; j <= iRows + count; j++)
			{
				CString boltKey = pListCtrl->GetItemText(j, 1);
				pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
				if (!boltKey.IsEmpty() || boltKey.GetLength() > 0)
				{
					strTempByGroupName = pListCtrl->GetItemText(iRows, 0);
					BOLT_BLOCK* pBoltD = (BOLT_BLOCK*)pSelItem->m_idProp;
					if (pBoltD)
						pBoltD->sGroupName = strTempByGroupName;
				}
				else
					break;
			}
		}
		else if (pParentItem != NULL && pCurSelItem)//点击子节点
		{
			pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
			BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.GetValue(strTempByBlockName);
			if (pBoltD != NULL)
			{
				CString m_ModifyText = pListCtrl->GetItemText(iRows, iSubItem);
				switch (iSubItem)
				{
				case 1:
					pBoltD->sBlockName = m_ModifyText;
					break;
				case 2:
					pBoltD->diameter = atoi(m_ModifyText);
					break;
				case 3:
					pBoltD->hole_d = atof(m_ModifyText);
					break;
				}
			}
			else
			{
				if (!pListCtrl->GetItemText(iRows, 1).IsEmpty())
				{
					BOLT_BLOCK pBoltD1;
					CListCtrlItemInfo* m_lpNodeInfo = pParentItem->m_lpNodeInfo;
					pBoltD1.sGroupName = m_lpNodeInfo->GetSubItemText(0);
					pBoltD1.sBlockName = pListCtrl->GetItemText(iRows, 1);
					pBoltD1.diameter = atoi(pListCtrl->GetItemText(iRows, 2));
					pBoltD1.hole_d = atof(pListCtrl->GetItemText(iRows, 3));
					g_pncSysPara.hashBoltDList.SetValue(pListCtrl->GetItemText(iRows, 1), pBoltD1);
				}
			}
		}

	}
	return TRUE;
}
static BOOL _LocalKeyDownItemFunc(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem, LV_KEYDOWN* pLVKeyDow)
{
	if (pItem == NULL)
		return FALSE;
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();

	if (pLVKeyDow->wVKey == VK_DELETE)
	{
		pSysSettingDlg->OnPNCSysDel();
	}
	else if (pLVKeyDow->wVKey == VK_RETURN)
	{
		int nCols = pListCtrl->GetHeaderCtrl()->GetItemCount();//列数
		LV_ITEM EditItem = pListCtrl->m_curEditItem;
		int count1 = pListCtrl->GetSelectedItem();
		if (EditItem.iSubItem < nCols - 1)
		{
			pListCtrl->DisplayCellCtrl(count1, EditItem.iSubItem + 1);
			pListCtrl->m_curEditItem.iSubItem = EditItem.iSubItem + 1;
		}
		else
		{
			CSuperGridCtrl::CTreeItem* pNextItem = pListCtrl->GetTreeItem(pItem->GetIndex() + 1);
			if (pNextItem)
			{
				pListCtrl->SelectItem(pNextItem, 1, FALSE, TRUE);
				pListCtrl->DisplayCellCtrl(pItem->GetIndex() + 1, 1);
				pListCtrl->m_curEditItem.iSubItem = 1;
			}
		}
	}
	return TRUE;
}
BEGIN_MESSAGE_MAP(CPNCSysSettingDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_GROUP, OnSelchangeTabGroup)
	ON_BN_CLICKED(ID_BTN_DEFAULT, &CPNCSysSettingDlg::OnBnClickedBtnDefault)
	ON_COMMAND(ID_MENU_PNCSYS_DEL, &CPNCSysSettingDlg::OnPNCSysDel)
	ON_COMMAND(ID_MENU_PNCSYS_ADD, &CPNCSysSettingDlg::OnPNCSysAdd)
	ON_COMMAND(ID_MENU_BOLT_DEL, &CPNCSysSettingDlg::OnPNCSysGroupDel)
	ON_COMMAND(ID_MENU_BOLT_ADD, &CPNCSysSettingDlg::OnPNCSysGroupAdd)
END_MESSAGE_MAP()
static BOOL FireContextMenu(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pSelItem, CPoint point)
{
	CPNCSysSettingDlg *pCfgDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	if (pCfgDlg == NULL)
		return FALSE;
	int iCurSel = pCfgDlg->m_ctrlPropGroup.GetCurSel();
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MENU_ADDORDEL);
	CMenu *pMenu = popMenu.GetSubMenu(0);
	//pMenu->DeleteMenu(0, MF_BYPOSITION);
	BOOL bInSelItemRect = FALSE;
	CHeaderCtrl *pHeadCtrl = pListCtrl->GetHeaderCtrl();
	HDHITTESTINFO info = { 0 };
	info.pt = point;

	if (iCurSel == PROPGROUP_BY_BOLT)
	{
		int i = pListCtrl->GetSelectionMark();
		CString boltKey = pListCtrl->GetItemText(i, 0);
		if (!boltKey.IsEmpty() || boltKey.GetLength() > 0)
		{
			pMenu->AppendMenu(MF_STRING, ID_MENU_BOLT_ADD, "添加分组");
		}
	}
	CPoint menu_pos = point;
	pListCtrl->ClientToScreen(&menu_pos);
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, menu_pos.x, menu_pos.y, pCfgDlg);
	return TRUE;
}


// CPNCSysSettingDlg 消息处理程序
BOOL CPNCSysSettingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	m_ctrlPropGroup.DeleteAllItems();
	m_ctrlPropGroup.InsertItem(0, "常规设置");
	m_ctrlPropGroup.InsertItem(1, "文字识别");
	m_ctrlPropGroup.InsertItem(2, "螺栓识别");
	//
	PNCSysSetImportDefault();

	long col_wide_arr[7] = { 80,80,80,80,80,80,80 };
	m_listCtrlSysSetting.InitListCtrl(col_wide_arr);
	m_listCtrlSysSetting.SetDblclkDisplayCellCtrl(true);
	m_listCtrlSysSetting.SetModifyValueFunc(FireValueModify);
	m_listCtrlSysSetting.SetKeyDownItemFunc(_LocalKeyDownItemFunc);
	m_listCtrlSysSetting.SetLButtonDblclkFunc(FireLButtonDblclk);
	CWnd *pBtmWnd = GetDlgItem(IDC_E_PROP_HELP_STR);
	m_propList.m_hPromptWnd = pBtmWnd->GetSafeHwnd();
	m_propList.SetDividerScale(0.45);
	//
	g_pncSysPara.InitPropHashtable();
#ifdef __PNC_
	if (m_bInernalStart)	//内部启动
		FinishSelectObjOper();		//完成选择对象的后续操作
#endif
	DisplaySystemSetting();
	//设置右键菜单回调函数
	m_listCtrlSysSetting.SetContextMenuFunc(FireContextMenu);
	return TRUE;
}
void CPNCSysSettingDlg::OnClose()
{
	PNCSysSetExportDefault();
}
void CPNCSysSettingDlg::OnOK()
{
	bool bEnable = FALSE;
	for (RECOG_SCHEMA *pBoltD = g_pncSysPara.m_recogSchemaList.GetFirst(); pBoltD; pBoltD = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		if (pBoltD->m_bEnable)
			bEnable = TRUE;
	}
	if (bEnable) 
	{
		CPNCSysSettingDlg::OnClose();
		if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			DisplayPartListDockBar();
		CDialog::OnOK();
	}
	else
		MessageBox("未设置默认启用项！", "文字识别标识");
	
}
void CPNCSysSettingDlg::OnCancel()
{
	PNCSysSetImportDefault();
	CDialog::OnCancel();
}
void CPNCSysSettingDlg::OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult)
{
	int iCurSel = m_ctrlPropGroup.GetCurSel();
	m_listCtrlSysSetting.ShowWindow(iCurSel == PROPGROUP_BY_RULE ? SW_HIDE : SW_SHOW);
	m_propList.ShowWindow(iCurSel == PROPGROUP_BY_RULE ? SW_SHOW : SW_HIDE);
	if (iCurSel == PROPGROUP_BY_RULE)
	{
		m_propList.m_iPropGroup = iCurSel;
		m_propList.Redraw();
	}
	else
	{
		while (m_listCtrlSysSetting.GetHeaderCtrl()->GetItemCount() > 0)
			m_listCtrlSysSetting.DeleteColumn(0);
		m_listCtrlSysSetting.DeleteAllItems();
		if (iCurSel == PROPGROUP_BY_TEXT)
		{
			m_listCtrlSysSetting.InsertColumn(0, _T("启用"), LVCFMT_LEFT, 40);
			m_listCtrlSysSetting.InsertColumn(1, _T("名称"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(2, _T("单行/多行"), LVCFMT_LEFT, 65);
			m_listCtrlSysSetting.InsertColumn(3, _T("件号"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(4, _T("规格"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(5, _T("材质"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(6, _T("加工数"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(7, _T("正曲"), LVCFMT_LEFT, 55);
			m_listCtrlSysSetting.InsertColumn(8, _T("反曲"), LVCFMT_LEFT, 55);
			if (g_pncSysPara.m_recogSchemaList.GetNodeNum() == 0)
			{
				RECOG_SCHEMA *pSchema1 = new RECOG_SCHEMA();
				pSchema1->m_bEnable = TRUE;
				pSchema1->m_bEditable = FALSE;
				pSchema1->m_iDimStyle = g_pncSysPara.m_iDimStyle;
				pSchema1->m_sPnKey = g_pncSysPara.m_sPnKey;
				pSchema1->m_sThickKey = g_pncSysPara.m_sThickKey;
				pSchema1->m_sMatKey = g_pncSysPara.m_sMatKey;
				pSchema1->m_sPnNumKey = g_pncSysPara.m_sPnNumKey;
				InsertRecogSchemaItem(&m_listCtrlSysSetting, pSchema1);
			}
			else
			{
				for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
					InsertRecogSchemaItem(&m_listCtrlSysSetting, pSchema);
				InsertRecogSchemaItem(&m_listCtrlSysSetting, NULL);
			}
		}
		else if (iCurSel == PROPGROUP_BY_BOLT)
		{
			hashGroupByItemName.Empty();
			m_listCtrlSysSetting.InsertColumn(0, _T("分组"), LVCFMT_LEFT, 130);
			m_listCtrlSysSetting.InsertColumn(1, _T("螺栓图符"), LVCFMT_LEFT, 80);
			m_listCtrlSysSetting.InsertColumn(2, _T("螺栓直径"), LVCFMT_LEFT, 80);
			m_listCtrlSysSetting.InsertColumn(3, _T("孔径"), LVCFMT_LEFT, 80);
			for (BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.GetFirst(); pBoltD; pBoltD = g_pncSysPara.hashBoltDList.GetNext())
			{
				if (!hashGroupByItemName.GetValue(pBoltD->sGroupName))
				{
					CListCtrlItemInfo* lpInfo = new CListCtrlItemInfo();
					lpInfo->SetSubItemText(0, _T(pBoltD->sGroupName));
					CSuperGridCtrl::CTreeItem* pGroupItem = m_listCtrlSysSetting.InsertRootItem(lpInfo, TRUE);
					hashGroupByItemName.SetValue(pBoltD->sGroupName, pGroupItem);
					pGroupItem->m_bHideChildren = FALSE;
					InsertBoltBolckItem(&m_listCtrlSysSetting, pGroupItem, pBoltD);
				}
				else
				{
					CSuperGridCtrl::CTreeItem** ppGroupItem = hashGroupByItemName.GetValue(pBoltD->sGroupName);
					CSuperGridCtrl::CTreeItem* pGroupItem = ppGroupItem ? *ppGroupItem : NULL;
					InsertBoltBolckItem(&m_listCtrlSysSetting, pGroupItem, pBoltD);

				}
			}
			m_listCtrlSysSetting.Redraw();
		}
	}
}
void CPNCSysSettingDlg::DisplaySystemSetting()
{
	m_propList.CleanTree();
	m_propList.SetModifyValueFunc(ModifySystemSettingValue);
	m_propList.SetButtonClickFunc(ButtonClickSystemSetting);
	m_propList.SetPopMenuClickFunc(FireSystemSettingPopMenuClick);
	m_propList.SetPickColorFunc(FirePickColor);
	CPropertyListOper<CPNCSysPara> oper(&m_propList, &g_pncSysPara);
	CPropTreeItem* pRootItem = m_propList.GetRootItem();
	CPropTreeItem *pPropItem = NULL, *pGroupItem = NULL, *pItem = NULL;
	const int GROUP_GENERAL = 1, GROUP_DIM = 2;
	//常规设置
	pGroupItem = oper.InsertPropItem(pRootItem, "general_set");
	pGroupItem->m_dwPropGroup = GetSingleWord(GROUP_GENERAL);
	oper.InsertCmbListPropItem(pGroupItem, "m_bIncDeformed");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_bReplaceSH");
	if (g_pncSysPara.m_bReplaceSH)
		oper.InsertCmbListPropItem(pPropItem, "m_nReplaceHD");
	oper.InsertCmbListPropItem(pGroupItem, "m_iPPiMode");
#ifdef __LAYOUT_
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_bAutoLayout1");
	if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_PRINT)
	{
		oper.InsertEditPropItem(pPropItem, "m_nMapWidth");
		oper.InsertEditPropItem(pPropItem, "m_nMapLength");
		oper.InsertEditPropItem(pPropItem, "m_nMinDistance");
	}
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_bAutoLayout2");
	if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
	{
		oper.InsertEditPropItem(pPropItem, "CDrawDamBoard::BOARD_HEIGHT");
		oper.InsertCmbListPropItem(pPropItem, "CDrawDamBoard::m_bDrawAllBamBoard");
		oper.InsertEditPropItem(pPropItem, "m_nMkRectLen");
		oper.InsertEditPropItem(pPropItem, "m_nMkRectWidth");
	}
#endif
	oper.InsertCmbListPropItem(pGroupItem, "m_bMKPos");
	oper.InsertCmbListPropItem(pGroupItem, "AxisXCalType");
	oper.InsertCmbListPropItem(pGroupItem, "m_ciBoltRecogMode");
	//图层设置
	pGroupItem = oper.InsertPropItem(pRootItem, "layer_set");
	pGroupItem->m_dwPropGroup = GetSingleWord(GROUP_GENERAL);
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_iRecogMode");
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
	{	//按颜色识别
		oper.InsertCmbColorPropItem(pPropItem, "m_iProfileColorIndex");
		oper.InsertCmbColorPropItem(pPropItem, "m_iBendLineColorIndex");
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
	{	//按图层识别
		pItem = oper.InsertPopMenuItem(pPropItem, "layer_mode");
		UpdateFilterLayerProperty(&m_propList, pItem);
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
	{	//按线型识别
		oper.InsertCmbListPropItem(pPropItem, "m_iProfileLineTypeName");
	}
	//标注识别设置
	pGroupItem = oper.InsertPropItem(pRootItem, "text_dim_set");
	pGroupItem->m_dwPropGroup = GetSingleWord(GROUP_DIM);
	//文字识别
	oper.InsertCmbListPropItem(pGroupItem, "m_iDimStyle");	//标注类型
	pPropItem = oper.InsertButtonPropItem(pGroupItem, "m_nMember");//标注成员
	UpdateTxtDimProperty(&m_propList, pPropItem);
	//螺栓识别
	pGroupItem = oper.InsertGrayButtonPropItem(pRootItem, "bolt_dim_set");
	pGroupItem->m_lpNodeInfo->m_sButtonName = "添加标识";
	pGroupItem->m_dwPropGroup = GetSingleWord(GROUP_DIM);
	UpdateBoltDimProperty(&m_propList, pGroupItem);
	//绘图比例
	pGroupItem = oper.InsertPropItem(pRootItem, "map_scale_set");
	pGroupItem->m_dwPropGroup = GetSingleWord(GROUP_DIM);
	oper.InsertEditPropItem(pGroupItem, "m_fMapScale");	//图纸绘图比例
	//
	m_propList.Redraw();
}

void CPNCSysSettingDlg::OnBnClickedBtnDefault()
{
	g_pncSysPara.Init();
	DisplaySystemSetting();
}

//选择对象节点或线
void CPNCSysSettingDlg::SelectEntObj(int nResultEnt/*=1*/)
{
	m_bPauseBreakExit = TRUE;
	m_bInernalStart = TRUE;
	m_iBreakType = 1;	//拾取点
	m_nResultEnt = nResultEnt;
	CDialog::OnOK();
}

void CPNCSysSettingDlg::FinishSelectObjOper()
{
	if (!m_bInernalStart || m_iBreakType != 1)
		return;
#ifdef __PNC_
	//由于选择实体内部重启
	CAD_SCREEN_ENT *pCADEnt = resultList.GetFirst();
	CPropTreeItem *pItem = m_propList.FindItemByPropId(m_idEventProp, NULL);
	if (pCADEnt&&pItem &&
		(CPNCSysPara::GetPropID("m_iProfileColorIndex") == pItem->m_idProp ||
			CPNCSysPara::GetPropID("m_iBendLineColorIndex") == pItem->m_idProp))
	{
		m_propList.SetFocus();
		m_propList.SetCurSel(pItem->m_iIndex);	//选中指定属性
		AcDbEntity *pEnt = NULL;
		acdbOpenObject(pEnt, pCADEnt->m_idEnt, AcDb::kForRead);
		if (pEnt == NULL)
			return;
		CAcDbObjLife life(pEnt);
		int iColorIndex = GetEntColorIndex(pEnt);
		COLORREF clr = GetColorFromIndex(iColorIndex);
		char tem_str[100] = "";
		if (CPNCSysPara::GetPropID("m_iProfileColorIndex") == pItem->m_idProp)
			g_pncSysPara.m_ciProfileColorIndex = iColorIndex;
		else if (CPNCSysPara::GetPropID("m_iBendLineColorIndex") == pItem->m_idProp)
			g_pncSysPara.m_ciBendLineColorIndex = iColorIndex;
		else
			return;
		if (g_pncSysPara.GetPropValueStr(pItem->m_idProp, tem_str) > 0)
			m_propList.SetItemPropValue(pItem->m_idProp, CString(tem_str));
	}
#endif
}
