// SystemSettingDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PNCSysSettingDlg.h"
#include "PropertyListOper.h"
#include "PNCSysPara.h"
#include "LogFile.h"
#include "XhLicAgent.h"
#include "FilterLayerDlg.h"
#include "CadToolFunc.h"
#include "DrawDamBoard.h"
#include "resource.h"
#include "SelColorDlg.h"
#include "InputAnValDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
static const BYTE PROPGROUP_RULE = 0;
static const BYTE PROPGROUP_TEXT = 1;
static const BYTE PROPGROUP_BOLT = 2;

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
		lpInfo->m_strPropName="ͼ������";
		lpInfo->m_strPropHelp="Ĭ�Ϲ��˵�ͼ������";
		CPropTreeItem* pPropItem=pPropList->InsertItem(pParentItem,lpInfo, -1,TRUE);
		pPropItem->m_idProp = (long)pItem;
		pPropItem->m_dwPropGroup=pParentItem->m_dwPropGroup;
		pPropItem->m_lpNodeInfo->m_strPropValue.Format("%s",(char*)pItem->m_sLayer);
		pPropItem->SetReadOnly();
	}
}
static BOOL ModifySystemSettingValue(CPropertyList	*pPropList, CPropTreeItem *pItem, CString &valueStr)
{
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pPropList->GetParent();
	if (pSysSettingDlg == NULL)
		return FALSE;
	CPropertyListOper<CPNCSysPara> oper(pPropList, &g_pncSysPara);
	CLogErrorLife logErrLife;
	if (pItem->m_idProp == CPNCSysPara::GetPropID("m_fMapScale"))
		g_pncSysPara.m_fMapScale = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bIncDeformed"))
	{
		if (valueStr.Compare("��") == 0)
			g_pncSysPara.m_bIncDeformed = true;
		else
			g_pncSysPara.m_bIncDeformed = false;
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sJgCadName"))
	{
		g_pncSysPara.m_sJgCadName.Copy(valueStr);
		g_pncSysPara.InitJgCardInfo(valueStr);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sPartLabelTitle"))
		g_pncSysPara.m_sPartLabelTitle = valueStr;
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_sJgCardBlockName"))
		g_pncSysPara.m_sJgCardBlockName = valueStr;
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_fMaxLenErr"))
		g_pncSysPara.m_fMaxLenErr = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_ciMKPos"))
	{
		g_pncSysPara.m_ciMKPos = valueStr[0] - '0';
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_ciMKPos == 2)
			oper.InsertEditPropItem(pItem, "m_fMKHoleD", "", "", -1, TRUE);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_fMKHoleD"))
		g_pncSysPara.m_fMKHoleD = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMaxHoleD"))
		g_pncSysPara.m_nMaxHoleD = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_bUseMaxEdge"))
	{
		if (valueStr.CompareNoCase("��") == 0)
			g_pncSysPara.m_bUseMaxEdge = TRUE;
		else
			g_pncSysPara.m_bUseMaxEdge = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_bUseMaxEdge)
			oper.InsertEditPropItem(pItem, "m_nMaxEdgeLen", "", "", -1, TRUE);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMaxEdgeLen"))
		g_pncSysPara.m_nMaxEdgeLen = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iPPiMode"))
		g_pncSysPara.m_iPPiMode = valueStr[0] - '0';
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("AxisXCalType"))
		g_pncSysPara.m_iAxisXCalType = valueStr[0] - '0';
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_ciLayoutMode"))
	{
		if (valueStr.CompareNoCase("�ְ�Ա�") == 0)
			g_pncSysPara.m_ciLayoutMode = CPNCSysPara::LAYOUT_COMPARE;
		else if (valueStr.CompareNoCase("�Զ��Ű�") == 0)
			g_pncSysPara.m_ciLayoutMode = CPNCSysPara::LAYOUT_PRINT;
		else if (valueStr.CompareNoCase("����Ԥ��") == 0)
			g_pncSysPara.m_ciLayoutMode = CPNCSysPara::LAYOUT_PROCESS;
		else if (valueStr.CompareNoCase("ͼԪɸѡ") == 0)
			g_pncSysPara.m_ciLayoutMode = CPNCSysPara::LAYOUT_FILTRATE;
		else
			g_pncSysPara.m_ciLayoutMode = CPNCSysPara::LAYOUT_CLONE;
		pSysSettingDlg->UpdateLayoutProperty(pItem);
#ifndef __UBOM_ONLY_
		CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
		if (pPartListDlg != NULL && pPartListDlg->GetSafeHwnd() != NULL)
		{
			pPartListDlg->RefreshCtrlState();
			pPartListDlg->RelayoutWnd();
		}
#endif
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_ciArrangeType"))
		g_pncSysPara.m_ciArrangeType = valueStr[0] - '0';
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_ciGroupType"))
		g_pncSysPara.m_ciGroupType = valueStr[0] - '0';
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMapWidth"))
		g_pncSysPara.m_nMapWidth=atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMapLength"))
		g_pncSysPara.m_nMapLength=atoi(valueStr);
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_nMinDistance"))
		g_pncSysPara.m_nMinDistance=atoi(valueStr);
#ifndef __UBOM_ONLY_
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("CDrawDamBoard::BOARD_HEIGHT"))
	{
		CDrawDamBoard::BOARD_HEIGHT = atoi(valueStr);
		CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
		if (pPartListDlg&&g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
			pPartListDlg->m_xDamBoardManager.DrawAllDamBoard(&model);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("CDrawDamBoard::m_bDrawAllBamBoard"))
	{
		CDrawDamBoard::m_bDrawAllBamBoard = valueStr[0] - '0';
		CLockDocumentLife lockDocument;
		CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
		if (pPartListDlg&&g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
			pPartListDlg->m_xDamBoardManager.DrawAllDamBoard(&model);
	}
#endif
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMkRectLen"))
		g_pncSysPara.m_nMkRectLen = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_nMkRectWidth"))
		g_pncSysPara.m_nMkRectWidth = atoi(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("layer_mode"))
	{
		g_pncSysPara.m_iLayerMode = valueStr[0] - '0';
		UpdateFilterLayerProperty(pPropList, pItem);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("FilterPartNoCir"))
	{
		if(valueStr.CompareNoCase("����")==0)
			g_pncSysPara.m_ciBoltRecogMode |= 0X01;
		else
			g_pncSysPara.m_ciBoltRecogMode &= 0X02;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.IsFilterPartNoCir())
			oper.InsertEditPropItem(pItem, "m_fPartNoCirD", "", "", -1, TRUE);
	}
	else if(pItem->m_idProp==CPNCSysPara::GetPropID("m_fPartNoCirD"))
		g_pncSysPara.m_fPartNoCirD = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("RecogHoleDimText"))
	{
		if (valueStr.CompareNoCase("����ע����") == 0)
			g_pncSysPara.m_ciBoltRecogMode |= 0X02;
		else
			g_pncSysPara.m_ciBoltRecogMode &= 0X01;
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("RecogLsCircle"))
	{
		if (valueStr.CompareNoCase("���ݱ�׼�׽���ɸѡ") == 0)
			g_pncSysPara.m_ciBoltRecogMode |= 0X04;
		else
			g_pncSysPara.m_ciBoltRecogMode &= 0X03;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.IsRecogCirByBoltD())
		{
			oper.InsertEditPropItem(pItem, "standardM12", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "standardM16", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "standardM20", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "standardM24", "", "", -1, TRUE);
		}
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iRecogMode"))
	{
		g_pncSysPara.m_ciRecogMode = valueStr[0] - '0';
		pPropList->DeleteAllSonItems(pItem);
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
		{	//����ɫʶ��
			oper.InsertCmbColorPropItem(pItem, "m_iProfileColorIndex", "", "", "", -1, TRUE);
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//��ͼ��ʶ��
			CPropTreeItem *pPropItem = oper.InsertPopMenuItem(pItem, "layer_mode", "", "", "", -1, TRUE);
			UpdateFilterLayerProperty(pPropList, pPropItem);
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//������ʶ��
			oper.InsertCmbListPropItem(pItem, "m_iProfileLineTypeName", "", "", "", -1, TRUE);
		}
#ifdef __ALFA_TEST_
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		{	//���ش������
			oper.InsertEditPropItem(pItem, "m_fPixelScale", "", "", -1, TRUE);
		}
#endif
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_fPixelScale"))
		g_pncSysPara.m_fPixelScale = atof(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileLineTypeName"))
		g_pncSysPara.m_sProfileLineType.Copy(valueStr);
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileColorIndex") ||
		pItem->m_idProp == CPNCSysPara::GetPropID("m_iBendLineColorIndex"))
	{
		COLORREF curClr = 0;
		char tem_str[100] = "";
		sprintf(tem_str, "%s", valueStr);
		memmove(tem_str, tem_str + 3, 97);//����RGB
		sscanf(tem_str, "%X", &curClr);
		if (pItem->m_idProp == CPNCSysPara::GetPropID("m_iProfileColorIndex"))
			g_pncSysPara.m_ciProfileColorIndex = GetNearestACI(curClr);
		else //if(pItem->m_idProp==CPNCSysPara::GetPropID("m_iBendLineColorIndex"))
			g_pncSysPara.m_ciBendLineColorIndex = GetNearestACI(curClr);
	}
	else if (pItem->m_idProp >= CPNCSysPara::GetPropID("crMode.crEdge") &&
			pItem->m_idProp <= CPNCSysPara::GetPropID("crMode.crOtherLS"))
	{
		COLORREF curClr = 0;
		char tem_str[100] = "";
		sprintf(tem_str, "%s", valueStr);
		memmove(tem_str, tem_str + 3, 97);//����RGB
		sscanf(tem_str, "%X", &curClr);
		int clrIndex = GetNearestACI(curClr);
		if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crEdge"))
			g_pncSysPara.crMode.crEdge = curClr;
		else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crLS12"))
			g_pncSysPara.crMode.crLS12 = curClr;
		else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crLS16"))
			g_pncSysPara.crMode.crLS16 = curClr;
		else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crLS20"))
			g_pncSysPara.crMode.crLS20 = curClr;
		else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crLS24"))
			g_pncSysPara.crMode.crLS24 = curClr;
		else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode.crOtherLS"))
			g_pncSysPara.crMode.crOtherLS = curClr;
	}
	return TRUE;
}
static BOOL ButtonClickSystemSetting(CPropertyList *pPropList, CPropTreeItem* pItem)
{
	CPNCSysSettingDlg* pDlg = (CPNCSysSettingDlg*)pPropList->GetParent();
	if (pDlg == NULL)
		return FALSE;
	CPropertyListOper<CPNCSysPara> oper(pPropList, &g_pncSysPara);
	if (pItem->m_idProp == CPNCSysPara::GetPropID("layer_mode"))
	{
		if (g_pncSysPara.m_iLayerMode == 0)
		{
			CFilterLayerDlg dlg;
			if(dlg.DoModal()!=IDOK)
				return FALSE;
		}
		UpdateFilterLayerProperty(pPropList, pItem);
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("RecogLsBlock"))
	{
		pDlg->m_iSelTabGroup = 2;
		pDlg->m_ctrlPropGroup.SetCurSel(2);
		pDlg->RefreshCtrlState();
		pDlg->RefreshListItem();
	}
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("crMode"))
	{
		CSelColorDlg dlg;
		if (dlg.DoModal() == IDOK)
		{
			oper.UpdatePropItemValue("crMode.crEdge");
			oper.UpdatePropItemValue("crMode.crLS12");
			oper.UpdatePropItemValue("crMode.crLS16");
			oper.UpdatePropItemValue("crMode.crLS20");
			oper.UpdatePropItemValue("crMode.crLS24");
			oper.UpdatePropItemValue("crMode.crOtherLS");
		}
	}
	return TRUE;
}
BOOL FireSystemSettingPopMenuClick(CPropertyList* pPropList, CPropTreeItem* pItem, CString sMenuName, int iMenu)
{
	CPNCSysSettingDlg* pDlg = (CPNCSysSettingDlg*)pPropList->GetParent();
	if (pDlg == NULL)
		return FALSE;
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
	else if (pItem->m_idProp == CPNCSysPara::GetPropID("RecogLsBlock"))
	{
		pDlg->m_iSelTabGroup = 2;
		pDlg->m_ctrlPropGroup.SetCurSel(2);
		pDlg->RefreshCtrlState();
		pDlg->RefreshListItem();
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
#ifdef __PNC_
		pParaDlg->m_idEventProp = pItem->m_idProp;	//��¼�����¼�������ID
		pParaDlg->m_arrCmdPickPrompt.RemoveAll();
#ifdef AFX_TARG_ENU_ENGLISH
		pParaDlg->m_arrCmdPickPrompt.Add("\nplease select an line <Enter confirm>:\n");
#else
		pParaDlg->m_arrCmdPickPrompt.Add("\n��ѡ��һ��ֱ��ʰȡ��ɫ<Enterȷ��>:\n");
#endif
		pParaDlg->SelectEntObj();
#endif
	}
	else if (pItem->m_idProp >= CPNCSysPara::GetPropID("crMode.crEdge") &&
		pItem->m_idProp <= CPNCSysPara::GetPropID("crMode.crOtherLS"))
	{
#ifdef __PNC_
		pParaDlg->m_idEventProp = pItem->m_idProp;	//��¼�����¼�������ID
		pParaDlg->m_arrCmdPickPrompt.RemoveAll();
#ifdef AFX_TARG_ENU_ENGLISH
		pParaDlg->m_arrCmdPickPrompt.Add("\nplease select an line <Enter confirm>:\n");
#else
		pParaDlg->m_arrCmdPickPrompt.Add("\n��ѡ��һ��ֱ��ʰȡ��ɫ<Enterȷ��>:\n");
#endif
		pParaDlg->SelectEntObj();
#endif
	}
	return FALSE;
}
//
static BOOL FireContextMenu(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pSelItem, CPoint point)
{
	CPNCSysSettingDlg *pCfgDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	if (pCfgDlg == NULL || pSelItem==NULL)
		return FALSE;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu = popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0, MF_BYPOSITION);
	int iSelTab = pCfgDlg->m_ctrlPropGroup.GetCurSel();
	if (iSelTab == PROPGROUP_TEXT)
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ��");
	else if(iSelTab == PROPGROUP_BOLT)
	{
		BOLT_BLOCK* pBoltBlock = (BOLT_BLOCK*)pSelItem->m_idProp;
		if (pBoltBlock)
			pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ����˨��");
		else
		{
			//pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ������");
			pMenu->AppendMenu(MF_STRING, ID_ADD_ITEM, "�����˨��");
		}
	}
	//
	CPoint menu_pos = point;
	pListCtrl->ClientToScreen(&menu_pos);
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, menu_pos.x, menu_pos.y, pCfgDlg);
	return TRUE;
}
static BOOL FireLButtonDblclk(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem, int iSubItem)
{
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	if (pSysSettingDlg == NULL)
		return FALSE;
	int iSelTab = pSysSettingDlg->m_ctrlPropGroup.GetCurSel();
	if (iSelTab != PROPGROUP_TEXT)
		return FALSE;
	RECOG_SCHEMA *pSelSchema = pItem ? (RECOG_SCHEMA*)pItem->m_idProp : NULL;
	if (pSelSchema == NULL)
		return FALSE;
	if (iSubItem == 0)//˫����һ��
	{	//���е�m_bEnable��Ϊfalse�����ѡ����
		for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
			pSchema->m_bEnable = false;
		pSelSchema->m_bEnable = true;
		//
		pSysSettingDlg->RefreshListItem();
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
		lpInfo->SetSubItemText(0, _T(pSchema->m_bEnable ? "��" : ""), true);
	else
		lpInfo->SetSubItemText(0, "", true);
	lpInfo->SetSubItemText(1, _T(pSchema->m_sSchemaName), bEditable);
	lpInfo->AddSubItemText(pSchema->m_iDimStyle == 0 ? "����" : pSchema->m_iDimStyle == 1 ? "����" : "", bEditable);
	lpInfo->SetControlType(2, GCT_CMB_LIST);
	lpInfo->SetListItemsStr(2, "����|����|");
	lpInfo->AddSubItemText(pSchema->m_sPnKey, bEditable);
	lpInfo->SetControlType(3, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(3, "#|����:|���ţ�|");
	lpInfo->AddSubItemText(pSchema->m_sThickKey, bEditable);
	lpInfo->SetControlType(4, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(4, "-|���:|���|���:|��ȣ�|");
	lpInfo->AddSubItemText(pSchema->m_sMatKey, bEditable);
	lpInfo->SetControlType(5, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(5, "Q|����:|���ʣ�|");
	lpInfo->AddSubItemText(pSchema->m_sPnNumKey, bEditable);
	lpInfo->SetControlType(6, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(6, "����:|������|����:|������|");
	lpInfo->AddSubItemText(pSchema->m_sFrontBendKey, bEditable);
	lpInfo->SetControlType(7, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(7, "����|����");
	lpInfo->AddSubItemText(pSchema->m_sReverseBendKey, bEditable);
	lpInfo->SetControlType(8, GCT_CMB_EDIT);
	lpInfo->SetListItemsStr(8, "����|����");
	CSuperGridCtrl::CTreeItem* pGroupItem = pListCtrl->InsertRootItem(lpInfo);
	if (pSchema != &schema)
		pGroupItem->m_idProp = (long)pSchema;
	else
		pGroupItem->m_idProp = 0;
}
static void InsertBoltBolckItem(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pGroupItem, BOLT_BLOCK *pSchema)
{
	if (pSchema == NULL)
		return;
	CListCtrlItemInfo* lpInfoItem = new CListCtrlItemInfo();
	lpInfoItem->SetSubItemText(1, _T(pSchema->sBlockName));
	lpInfoItem->SetSubItemText(2, _T(CXhChar16("%d", pSchema->diameter)));
	lpInfoItem->SetSubItemText(3, _T(CXhChar16("%g", pSchema->hole_d)));
	lpInfoItem->SetCheck(TRUE);
	CSuperGridCtrl::CTreeItem* pItem = pListCtrl->InsertItem(pGroupItem, lpInfoItem);
	pItem->m_idProp = (long)pSchema;
}
static BOOL FireValueModify(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pSelItem,
	int iSubItem, CString& sTextValue)
{
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	if (pSysSettingDlg == NULL)
		return FALSE;
	CString sOldValue = _T("");
	if (pSelItem&&pSelItem->m_lpNodeInfo)
		sOldValue = pSelItem->m_lpNodeInfo->GetSubItemText(iSubItem);
	if (sOldValue.CompareNoCase(sTextValue) == 0)
		return FALSE;
	int nCurSel = pSysSettingDlg->m_ctrlPropGroup.GetCurSel();
	if (nCurSel == PROPGROUP_TEXT)
	{
		pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
		int iRows = pListCtrl->GetSelectedItem();
		CString sDimStyle = pListCtrl->GetItemText(iRows, 2);
		CString sPartNoKey = pListCtrl->GetItemText(iRows, 3);
		CString sThickKey = pListCtrl->GetItemText(iRows, 4);
		CString sMatKey = pListCtrl->GetItemText(iRows, 5);
		RECOG_SCHEMA* pSchema = (RECOG_SCHEMA*)pSelItem->m_idProp;
		if (pSchema == NULL && !sDimStyle.IsEmpty() && !sPartNoKey.IsEmpty() && 
			!sThickKey.IsEmpty() && !sMatKey.IsEmpty())
		{	//���һ�м��뵽ʶ���б���
			pSchema = g_pncSysPara.m_recogSchemaList.append();
			pSchema->m_bEnable = FALSE;
			pSchema->m_bEditable = FALSE;
			pSchema->m_sSchemaName = pListCtrl->GetItemText(iRows, 1);
			pSchema->m_iDimStyle = (sDimStyle.CompareNoCase("����") == 0) ? 0 : 1;
			pSchema->m_sPnKey = sPartNoKey;
			pSchema->m_sThickKey = sThickKey;
			pSchema->m_sMatKey = sMatKey;
			pSchema->m_sPnNumKey = pListCtrl->GetItemText(iRows, 6);
			pSelItem->m_idProp = (DWORD)pSchema;
			//�����¿���
			InsertRecogSchemaItem(pListCtrl, NULL);
		}
		else if(pSchema)
		{
			if (iSubItem == 1)
				pSchema->m_sSchemaName = sTextValue;
			else if (iSubItem == 2)
				pSchema->m_iDimStyle = (sTextValue.CompareNoCase("����") == 0) ? 0 : 1;
			else if (iSubItem == 3)
				pSchema->m_sPnKey = sTextValue;
			else if (iSubItem == 4)
				pSchema->m_sThickKey = sTextValue;
			else if (iSubItem == 5)
				pSchema->m_sMatKey = sTextValue;
			else if (iSubItem == 6)
				pSchema->m_sPnNumKey = sTextValue;
			else if (iSubItem == 7)
				pSchema->m_sFrontBendKey = sTextValue;
			else if (iSubItem == 8)
				pSchema->m_sReverseBendKey = sTextValue;
		}
	}
	else if (nCurSel == PROPGROUP_BOLT)
	{	
		BOLT_BLOCK* pBoltBlock = (BOLT_BLOCK*)pSelItem->m_idProp;
		if (pBoltBlock == NULL)
		{	//������ڵ�
			pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
			for (pBoltBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBoltBlock; pBoltBlock = g_pncSysPara.hashBoltDList.GetNext())
			{
				if (pBoltBlock->sGroupName.Equal(sOldValue))
					pBoltBlock->sGroupName = sTextValue;
			}
		}
		else
		{	//����ӽڵ�
			if (iSubItem == 1)
			{
				BOLT_BLOCK* pFindBolt = g_pncSysPara.hashBoltDList.GetValue(sTextValue);
				if (pFindBolt)
				{
					AfxMessageBox(CXhChar50("{%s}�Ѵ��ڴ���˨��",sTextValue));
					return FALSE;
				}
				pBoltBlock->sBlockName = sTextValue;
				g_pncSysPara.hashBoltDList.ModifyKeyStr(sOldValue, sTextValue);
			}
			else if (iSubItem == 2)
				pBoltBlock->diameter = atoi(sTextValue);
			else if (iSubItem == 3)
				pBoltBlock->hole_d = atof(sTextValue);
			pListCtrl->SetSubItemText(pSelItem, iSubItem, sTextValue);
		}
	}
	return TRUE;
}
static BOOL _LocalKeyDownItemFunc(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem, LV_KEYDOWN* pLVKeyDow)
{
	CPNCSysSettingDlg *pSysSettingDlg = (CPNCSysSettingDlg*)pListCtrl->GetParent();
	if (pSysSettingDlg == NULL || pItem == NULL)
		return FALSE;
	if (pLVKeyDow->wVKey == VK_DELETE)
	{
		pSysSettingDlg->OnPNCSysDel();
	}
	else if (pLVKeyDow->wVKey == VK_RETURN)
	{
		int nCols = pListCtrl->GetHeaderCtrl()->GetItemCount();//����
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
//////////////////////////////////////////////////////////////////////////
// CPNCSysSettingDlg �Ի���
IMPLEMENT_DYNAMIC(CPNCSysSettingDlg, CDialog)

CPNCSysSettingDlg::CPNCSysSettingDlg(CWnd* pParent /*=NULL*/)
#ifdef __PNC_
	: CCADCallBackDlg(CPNCSysSettingDlg::IDD, pParent)
#else
	: CDialog(CPNCSysSettingDlg::IDD, pParent)
#endif
{
	m_iSelTabGroup = 0;
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

BEGIN_MESSAGE_MAP(CPNCSysSettingDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_GROUP, OnSelchangeTabGroup)
	ON_BN_CLICKED(ID_BTN_DEFAULT, &CPNCSysSettingDlg::OnBnClickedBtnDefault)
	ON_COMMAND(ID_DELETE_ITEM, &CPNCSysSettingDlg::OnPNCSysDel)
	ON_COMMAND(ID_ADD_ITEM, &CPNCSysSettingDlg::OnPNCSysAdd)
END_MESSAGE_MAP()

// CPNCSysSettingDlg ��Ϣ�������
BOOL CPNCSysSettingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	m_ctrlPropGroup.DeleteAllItems();
	m_ctrlPropGroup.InsertItem(0, "��������");
	m_ctrlPropGroup.InsertItem(1, "����ʶ��");
#ifndef __UBOM_ONLY_
	m_ctrlPropGroup.InsertItem(2, "��˨ͼ��");
#endif
	//
	long col_wide_arr[7] = { 80,80,80,80,80,80,80 };
	m_listCtrlSysSetting.InitListCtrl(col_wide_arr);
	m_listCtrlSysSetting.SetDblclkDisplayCellCtrl(true);
	m_listCtrlSysSetting.SetModifyValueFunc(FireValueModify);
	m_listCtrlSysSetting.SetKeyDownItemFunc(_LocalKeyDownItemFunc);
	m_listCtrlSysSetting.SetLButtonDblclkFunc(FireLButtonDblclk);
	m_listCtrlSysSetting.SetContextMenuFunc(FireContextMenu);//�����Ҽ��˵��ص�����
	CWnd *pBtmWnd = GetDlgItem(IDC_E_PROP_HELP_STR);
	m_propList.m_hPromptWnd = pBtmWnd->GetSafeHwnd();
	m_propList.SetDividerScale(0.45);
	//
	g_pncSysPara.InitPropHashtable();
#ifdef __PNC_
	if (m_bInernalStart)	//�ڲ�����
		FinishSelectObjOper();		//���ѡ�����ĺ�������
	else       //���ڲ�����ʱ�������ò���������ѡ�����ɫ������� wht 19-10-30
		PNCSysSetImportDefault();
#endif
	m_ctrlPropGroup.SetCurSel(m_iSelTabGroup);
	RefreshCtrlState();
	RefreshListItem();
	DisplaySystemSetting();
	return TRUE;
}
void CPNCSysSettingDlg::RefreshCtrlState()
{
	m_listCtrlSysSetting.ShowWindow(m_iSelTabGroup == PROPGROUP_RULE ? SW_HIDE : SW_SHOW);
	m_propList.ShowWindow(m_iSelTabGroup == PROPGROUP_RULE ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_E_PROP_HELP_STR)->ShowWindow(m_iSelTabGroup == PROPGROUP_RULE ? SW_SHOW : SW_HIDE);
	if (m_iSelTabGroup == PROPGROUP_RULE)
		m_propList.m_iPropGroup = m_iSelTabGroup;
	else if (m_iSelTabGroup == PROPGROUP_TEXT)
	{
		while (m_listCtrlSysSetting.GetHeaderCtrl()->GetItemCount() > 0)
			m_listCtrlSysSetting.DeleteColumn(0);
		m_listCtrlSysSetting.InsertColumn(0, _T("����"), LVCFMT_LEFT, 40);
		m_listCtrlSysSetting.InsertColumn(1, _T("����"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(2, _T("����/����"), LVCFMT_LEFT, 65);
		m_listCtrlSysSetting.InsertColumn(3, _T("����"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(4, _T("���"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(5, _T("����"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(6, _T("�ӹ���"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(7, _T("����"), LVCFMT_LEFT, 55);
		m_listCtrlSysSetting.InsertColumn(8, _T("����"), LVCFMT_LEFT, 55);
	}
	else if (m_iSelTabGroup == PROPGROUP_BOLT)
	{
		while (m_listCtrlSysSetting.GetHeaderCtrl()->GetItemCount() > 0)
			m_listCtrlSysSetting.DeleteColumn(0);
		m_listCtrlSysSetting.InsertColumn(0, _T("����"), LVCFMT_LEFT, 130);
		m_listCtrlSysSetting.InsertColumn(1, _T("��˨ͼ��"), LVCFMT_LEFT, 100);
		m_listCtrlSysSetting.InsertColumn(2, _T("��˨ֱ��"), LVCFMT_LEFT, 100);
		m_listCtrlSysSetting.InsertColumn(3, _T("��˨�׾�"), LVCFMT_LEFT, 100);
	}
}
void CPNCSysSettingDlg::RefreshListItem()
{
	if (m_iSelTabGroup == PROPGROUP_RULE)
		return;
	m_listCtrlSysSetting.DeleteAllItems();
	if (m_iSelTabGroup == PROPGROUP_TEXT)
	{
		if (g_pncSysPara.m_recogSchemaList.GetNodeNum() == 0)
		{
			RECOG_SCHEMA *pNewSchema = g_pncSysPara.InsertRecogSchema("",
				g_pncSysPara.m_iDimStyle, g_pncSysPara.m_sPnKey,
				g_pncSysPara.m_sMatKey, g_pncSysPara.m_sThickKey, g_pncSysPara.m_sPnNumKey,
				g_pncSysPara.m_sFrontBendKey, g_pncSysPara.m_sReverseBendKey, TRUE);
			InsertRecogSchemaItem(&m_listCtrlSysSetting, pNewSchema);
		}
		else
		{
			for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
				InsertRecogSchemaItem(&m_listCtrlSysSetting, pSchema);
			InsertRecogSchemaItem(&m_listCtrlSysSetting, NULL);
		}
	}
	else if (m_iSelTabGroup == PROPGROUP_BOLT)
	{
		hashGroupByItemName.Empty();
		for (BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.GetFirst(); pBoltD; pBoltD = g_pncSysPara.hashBoltDList.GetNext())
		{
			CSuperGridCtrl::CTreeItem** ppGroupItem = hashGroupByItemName.GetValue(pBoltD->sGroupName);
			if (ppGroupItem == NULL)
			{
				ppGroupItem = hashGroupByItemName.Add(pBoltD->sGroupName);
				//
				CListCtrlItemInfo* lpInfo = new CListCtrlItemInfo();
				lpInfo->SetSubItemText(0, _T(pBoltD->sGroupName));
				*ppGroupItem = m_listCtrlSysSetting.InsertRootItem(lpInfo, TRUE);
				(*ppGroupItem)->m_bHideChildren = FALSE;
				(*ppGroupItem)->m_idProp = 0;
			}
			InsertBoltBolckItem(&m_listCtrlSysSetting, *ppGroupItem, pBoltD);
		}
	}
	m_listCtrlSysSetting.Redraw();
}

void CPNCSysSettingDlg::OnPNCSysDel()
{
	int iSelTab = m_ctrlPropGroup.GetCurSel();
	if (iSelTab == PROPGROUP_TEXT)
	{	//����ʶ��
		int nCols = m_listCtrlSysSetting.GetItemCount();
		int iSelRow = m_listCtrlSysSetting.GetSelectedItem();
		if (nCols == iSelRow + 1)
		{
			MessageBox("���һ�в���ɾ����", "ɾ����Ϣ��ʾ");
			return;
		}
		if (g_pncSysPara.m_recogSchemaList[iSelRow].m_bEditable)
		{
			MessageBox("Ĭ���в���ɾ����", "ɾ����Ϣ��ʾ");
			return;
		}
		else
		{
			if (IDOK == MessageBox("ȷ��Ҫɾ����", "��ʾ", IDOK))
			{
				m_listCtrlSysSetting.DeleteItem(iSelRow);
				g_pncSysPara.m_recogSchemaList.DeleteAt(iSelRow);
				g_pncSysPara.m_recogSchemaList.Clean();
			}
		}
	}
	else if (iSelTab == PROPGROUP_BOLT)
	{
		int iSelRow = m_listCtrlSysSetting.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pSelItem = m_listCtrlSysSetting.GetTreeItem(iSelRow);
		if (pSelItem->m_idProp==NULL)
		{	//ɾ����
			if (IDOK == MessageBox("ע�⣺ȷ��Ҫɾ������������", "��ʾ", IDOK))
			{
				CString sGroupName = m_listCtrlSysSetting.GetItemText(iSelRow, 0);
				for (BOLT_BLOCK* pBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBlock; pBlock = g_pncSysPara.hashBoltDList.GetNext())
				{
					if (pBlock->sGroupName.EqualNoCase(sGroupName))
						g_pncSysPara.hashBoltDList.DeleteCursor();
				}
				g_pncSysPara.hashBoltDList.Clean();
				//
				m_listCtrlSysSetting.DeleteAllSonItems(pSelItem);
				m_listCtrlSysSetting.DeleteItem(iSelRow);
			}
		}
		else
		{
			BOLT_BLOCK* pSelBlock = (BOLT_BLOCK*)pSelItem->m_idProp;
			if (pSelBlock && IDOK == MessageBox("ȷ��Ҫɾ����", "��ʾ", IDOK))
			{
				for (BOLT_BLOCK* pBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBlock; pBlock = g_pncSysPara.hashBoltDList.GetNext())
				{
					if (pBlock == pSelBlock)
					{
						g_pncSysPara.hashBoltDList.DeleteCursor();
						break;
					}
				}
				g_pncSysPara.hashBoltDList.Clean();
				//
				m_listCtrlSysSetting.DeleteItem(iSelRow);
			}
		}
	}
}
void CPNCSysSettingDlg::OnPNCSysAdd()
{
	int iCurSel = m_ctrlPropGroup.GetCurSel();
	if (iCurSel == PROPGROUP_BOLT)
	{
		int iRow = m_listCtrlSysSetting.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pSelItem = m_listCtrlSysSetting.GetTreeItem(iRow);
		CSuperGridCtrl::CTreeItem *pParentItem = m_listCtrlSysSetting.GetParentItem(pSelItem);
		if (pParentItem == NULL && pSelItem)
			pParentItem = pSelItem;
		if (pParentItem == NULL)
		{
			POSITION pos = m_listCtrlSysSetting.GetRootTailPosition();
			pParentItem = m_listCtrlSysSetting.GetPrevRoot(pos);
		}
		if (pParentItem == NULL)
			return;
		CString sGroupName = m_listCtrlSysSetting.GetItemText(iRow, 0);
		CInputAnStringValDlg dlg;
		dlg.m_sCaption = "��˨����";
		if (dlg.DoModal() == IDOK)
		{
			BOLT_BLOCK* pBlock = g_pncSysPara.hashBoltDList.GetValue(dlg.m_sItemValue);
			if (pBlock)
			{
				AfxMessageBox(CXhChar50("{%s}�Ѵ��ڸ���˨��!",(char*)pBlock->sBlockName));
				return;
			}
			pBlock = g_pncSysPara.hashBoltDList.Add(dlg.m_sItemValue);
			pBlock->sBlockName = dlg.m_sItemValue;
			pBlock->sGroupName = sGroupName;
			//
			RefreshListItem();
		}
	}
}

void CPNCSysSettingDlg::OnClose()
{
	PNCSysSetExportDefault();
}
void CPNCSysSettingDlg::OnOK()
{
	RECOG_SCHEMA *pSchema = NULL;
	for (pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		if (pSchema->m_bEnable)
			break;
	}
	if (pSchema) 
	{
		g_pncSysPara.ActiveRecogSchema(pSchema);
		CPNCSysSettingDlg::OnClose();
		CDialog::OnOK();
	}
	else
		MessageBox("δ����Ĭ�������", "����ʶ���ʶ");
	
}
void CPNCSysSettingDlg::OnCancel()
{
	PNCSysSetImportDefault();
	CDialog::OnCancel();
}
void CPNCSysSettingDlg::OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_iSelTabGroup = m_ctrlPropGroup.GetCurSel();
	RefreshCtrlState();
	if (m_iSelTabGroup == PROPGROUP_RULE)
		DisplaySystemSetting();
	else
		RefreshListItem();
}
void CPNCSysSettingDlg::DisplaySystemSetting()
{
	m_propList.CleanTree();
	m_propList.SetModifyValueFunc(ModifySystemSettingValue);
	m_propList.SetButtonClickFunc(ButtonClickSystemSetting);
	m_propList.SetPopMenuClickFunc(FireSystemSettingPopMenuClick);
	m_propList.SetPickColorFunc(FirePickColor);
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	UpdateUbomSettingProp();
#else
	UpdatePncSettingProp();
#endif
	//
	m_propList.Redraw();
}
void CPNCSysSettingDlg::UpdateUbomSettingProp()
{
	CPropertyListOper<CPNCSysPara> oper(&m_propList, &g_pncSysPara);
	CPropTreeItem* pRootItem = m_propList.GetRootItem();
	CPropTreeItem *pPropItem = NULL, *pGroupItem = NULL, *pItem = NULL;
	//��������
	pGroupItem = oper.InsertPropItem(pRootItem, "general_set");
	oper.InsertEditPropItem(pGroupItem, "m_sJgCadName");
	oper.InsertEditPropItem(pGroupItem, "m_fMaxLenErr");
	//ʶ��ģʽ
	pGroupItem = oper.InsertPropItem(pRootItem, "RecogMode");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_iRecogMode");
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
	{	//����ɫʶ��
		oper.InsertCmbColorPropItem(pPropItem, "m_iProfileColorIndex");
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
	{	//��ͼ��ʶ��
		pItem = oper.InsertPopMenuItem(pPropItem, "layer_mode");
		UpdateFilterLayerProperty(&m_propList, pItem);
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
	{	//������ʶ��
		oper.InsertCmbListPropItem(pPropItem, "m_iProfileLineTypeName");
	}
}
void CPNCSysSettingDlg::UpdatePncSettingProp()
{
	CPropertyListOper<CPNCSysPara> oper(&m_propList, &g_pncSysPara);
	CPropTreeItem* pRootItem = m_propList.GetRootItem();
	CPropTreeItem *pPropItem = NULL, *pGroupItem = NULL, *pItem = NULL;
	//��������
	pGroupItem = oper.InsertPropItem(pRootItem, "general_set");
	//�������Բ��ã����Կ���ȥ��
	//oper.InsertCmbListPropItem(pGroupItem, "m_bIncDeformed");		
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_bUseMaxEdge");
	if(g_pncSysPara.m_bUseMaxEdge)
		oper.InsertCmbListPropItem(pPropItem, "m_nMaxEdgeLen");
	oper.InsertEditPropItem(pGroupItem, "m_nMaxHoleD");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_ciMKPos");
	if (g_pncSysPara.m_ciMKPos == 2)
		oper.InsertEditPropItem(pPropItem, "m_fMKHoleD");
	oper.InsertCmbListPropItem(pGroupItem, "AxisXCalType");
	oper.InsertCmbListPropItem(pGroupItem, "m_iPPiMode");
	//ʶ��ģʽ����
	pGroupItem = oper.InsertPropItem(pRootItem, "RecogMode");
	//������ʶ��
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_iRecogMode");
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
	{	//����ɫʶ��
		oper.InsertCmbColorPropItem(pPropItem, "m_iProfileColorIndex");	
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
	{	//��ͼ��ʶ��
		pItem = oper.InsertPopMenuItem(pPropItem, "layer_mode");
		UpdateFilterLayerProperty(&m_propList, pItem);
	}
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
	{	//������ʶ��
		oper.InsertCmbListPropItem(pPropItem, "m_iProfileLineTypeName");
	}
#ifdef __ALFA_TEST_
	else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
	{	//������ʶ��
		oper.InsertEditPropItem(pPropItem, "m_fPixelScale");
	}
#endif
	//������ʶ����ɫ
	oper.InsertCmbColorPropItem(pGroupItem, "m_iBendLineColorIndex");
	//��˨ʶ��
	oper.InsertPopMenuItem(pGroupItem, "RecogLsBlock");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "RecogLsCircle");
	if (g_pncSysPara.IsRecogCirByBoltD())
	{
		oper.InsertEditPropItem(pPropItem, "standardM12");
		oper.InsertEditPropItem(pPropItem, "standardM16");
		oper.InsertEditPropItem(pPropItem, "standardM20");
		oper.InsertEditPropItem(pPropItem, "standardM24");
	}
	oper.InsertCmbListPropItem(pGroupItem, "RecogHoleDimText");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "FilterPartNoCir");
	if (g_pncSysPara.IsFilterPartNoCir())
		oper.InsertEditPropItem(pPropItem, "m_fPartNoCirD");
	//��ȡ�����ʾģʽ
	pGroupItem = oper.InsertPropItem(pRootItem, "DisplayMode");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_ciLayoutMode");
	UpdateLayoutProperty(pPropItem);
}
void CPNCSysSettingDlg::UpdateLayoutProperty(CPropTreeItem* pParentItem)
{
	if (pParentItem == NULL)
		return;
	CPropertyListOper<CPNCSysPara> oper(&m_propList, &g_pncSysPara);
	m_propList.DeleteAllSonItems(pParentItem);
	if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PRINT)
	{	//�Զ��Ű�
		oper.InsertEditPropItem(pParentItem, "m_nMapWidth", "", "", -1, TRUE);
		oper.InsertEditPropItem(pParentItem, "m_nMapLength", "", "", -1, TRUE);
		oper.InsertEditPropItem(pParentItem, "m_nMinDistance", "", "", -1, TRUE);
	}
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
	{	//����Ԥ��
		oper.InsertEditPropItem(pParentItem, "CDrawDamBoard::BOARD_HEIGHT", "", "", -1, TRUE);
		oper.InsertCmbListPropItem(pParentItem, "CDrawDamBoard::m_bDrawAllBamBoard", "", "", "", -1, TRUE);
		oper.InsertEditPropItem(pParentItem, "m_nMkRectLen", "", "", -1, TRUE);
		oper.InsertEditPropItem(pParentItem, "m_nMkRectWidth", "", "", -1, TRUE);
	}
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_COMPARE)
	{
		oper.InsertCmbListPropItem(pParentItem, "m_ciArrangeType", "", "", "", -1, TRUE);
		oper.InsertCmbListPropItem(pParentItem, "m_ciGroupType", "1.�κ�|2.����&���", "", "", -1, TRUE);
		CPropTreeItem* pItem = oper.InsertButtonPropItem(pParentItem, "crMode", "", "", -1, TRUE);
		pItem->m_bHideChildren = TRUE;
		oper.InsertCmbColorPropItem(pItem, "crMode.crEdge","","","",-1,TRUE);
		oper.InsertCmbColorPropItem(pItem, "crMode.crLS12", "", "", "", -1, TRUE);
		oper.InsertCmbColorPropItem(pItem, "crMode.crLS16", "", "", "", -1, TRUE);
		oper.InsertCmbColorPropItem(pItem, "crMode.crLS20", "", "", "", -1, TRUE);
		oper.InsertCmbColorPropItem(pItem, "crMode.crLS24", "", "", "", -1, TRUE);
	}
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_FILTRATE)
	{
		CPropTreeItem* pItem = oper.InsertCmbListPropItem(pParentItem, "m_ciGroupType", "", "", "", -1, TRUE);
		pItem->m_lpNodeInfo->m_strPropName = "ɸѡ����";
	}	
}
void CPNCSysSettingDlg::OnBnClickedBtnDefault()
{
	g_pncSysPara.Init();
	DisplaySystemSetting();
	OnSelchangeTabGroup(NULL, NULL);
}

#ifdef __PNC_
//ѡ�����ڵ����
void CPNCSysSettingDlg::SelectEntObj(int nResultEnt/*=1*/)
{
	m_bPauseBreakExit = TRUE;
	m_bInernalStart = TRUE;
	m_iBreakType = 1;	//ʰȡ��
	m_nResultEnt = nResultEnt;
	CDialog::OnOK();
}
void CPNCSysSettingDlg::FinishSelectObjOper()
{
	if (!m_bInernalStart || m_iBreakType != 1)
		return;
	//����ѡ��ʵ���ڲ�����
	CAD_SCREEN_ENT *pCADEnt = resultList.GetFirst();
	if (pCADEnt == NULL)
		return;
	CPropTreeItem *pItem = m_propList.FindItemByPropId(m_idEventProp, NULL);
	if (CPNCSysPara::GetPropID("m_iProfileColorIndex") == pItem->m_idProp ||
			CPNCSysPara::GetPropID("m_iBendLineColorIndex") == pItem->m_idProp)
	{
		m_propList.SetFocus();
		m_propList.SetCurSel(pItem->m_iIndex);	//ѡ��ָ������
		CAcDbObjLife acdbObjLife(pCADEnt->m_idEnt);
		AcDbEntity *pEnt = acdbObjLife.GetEnt();
		if (pEnt == NULL)
			return;
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
	else if (pItem->m_idProp >= CPNCSysPara::GetPropID("crMode.crEdge") &&
		pItem->m_idProp <= CPNCSysPara::GetPropID("crMode.crOtherLS"))
	{
		m_propList.SetFocus();
		m_propList.SetCurSel(pItem->m_iIndex);	//ѡ��ָ������
		CAcDbObjLife acdbObjLife(pCADEnt->m_idEnt);
		AcDbEntity *pEnt = acdbObjLife.GetEnt();
		if (pEnt == NULL)
			return;
		int iColorIndex = GetEntColorIndex(pEnt);
		COLORREF clr = GetColorFromIndex(iColorIndex);
		if (CPNCSysPara::GetPropID("crMode.crLS12") == pItem->m_idProp)
			g_pncSysPara.crMode.crLS12 = clr;
		else if (CPNCSysPara::GetPropID("crMode.crLS16") == pItem->m_idProp)
			g_pncSysPara.crMode.crLS16 = clr;
		else if (CPNCSysPara::GetPropID("crMode.crLS20") == pItem->m_idProp)
			g_pncSysPara.crMode.crLS20 = clr;
		else if (CPNCSysPara::GetPropID("crMode.crLS24") == pItem->m_idProp)
			g_pncSysPara.crMode.crLS24 = clr;
		else if (CPNCSysPara::GetPropID("crMode.crOtherLS") == pItem->m_idProp)
			g_pncSysPara.crMode.crOtherLS = clr;
		else if (CPNCSysPara::GetPropID("crMode.crEdge") == pItem->m_idProp)
			g_pncSysPara.crMode.crEdge = clr;
		else
			return;
		char tem_str[100] = "";
		if (g_pncSysPara.GetPropValueStr(pItem->m_idProp, tem_str) > 0)
			m_propList.SetItemPropValue(pItem->m_idProp, CString(tem_str));
	}
}
#endif
