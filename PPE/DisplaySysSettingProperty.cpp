#include "StdAfx.h"
#include "PPE.h"
#include "PPEView.h"
#include "MainFrm.h"
#include "PartPropertyDlg.h"
#include "PartLib.h"
#include "SysPara.h"
#include "PropertyListOper.h"
#include "HoleIncrementSetDlg.h"
#include "LicFuncDef.h"
#include "NcPart.h"
#include "PlankConfigDlg.h"

void UpdateHoleIncrementProperty(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	if(pParentItem==NULL)
		return ;
	CPropertyListOper<CSysPara> oper(pPropList,&g_sysPara);
	pPropList->DeleteAllSonItems(pParentItem);
	CNCPart::m_fHoleIncrement = g_sysPara.holeIncrement.m_fDatum;
	double fValue=0,fDatum=g_sysPara.holeIncrement.m_fDatum;
	//M12
	fValue=g_sysPara.holeIncrement.m_fM12;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fM12","","",-1,TRUE);
	//M16
	fValue=g_sysPara.holeIncrement.m_fM16;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fM16", "", "", -1, TRUE);
	//M20
	fValue=g_sysPara.holeIncrement.m_fM20;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fM20", "", "", -1, TRUE);
	//M24
	fValue=g_sysPara.holeIncrement.m_fM24;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fM24", "", "", -1, TRUE);
	//MSH
	fValue=g_sysPara.holeIncrement.m_fMSH;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fMSH", "", "", -1, TRUE);
}
BOOL ModifySyssettingProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CPropertyListOper<CSysPara> oper(pPropList,&g_sysPara);
	BOOL bUpdateHoleInc=FALSE;
	if (CSysPara::GetPropID("model.m_sCompanyName") == pItem->m_idProp)
		model.m_sCompanyName.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sPrjCode") == pItem->m_idProp)
		model.m_sPrjCode.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sPrjName") == pItem->m_idProp)
		model.m_sPrjName.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sTaType") == pItem->m_idProp)
		model.m_sTaType.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sTaAlias") == pItem->m_idProp)
		model.m_sTaAlias.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sTaStampNo") == pItem->m_idProp)
		model.m_sTaStampNo.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sOperator") == pItem->m_idProp)
		model.m_sOperator.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sAuditor") == pItem->m_idProp)
		model.m_sAuditor.Copy(valueStr);
	else if (CSysPara::GetPropID("model.m_sCritic") == pItem->m_idProp)
		model.m_sCritic.Copy(valueStr);
	else if (CSysPara::GetPropID("font.fTextHeight") == pItem->m_idProp)
	{
		g_sysPara.font.fTextHeight = atof(valueStr);
		g_sysPara.WriteSysParaToReg("TextHeight");
	}
	else if (CSysPara::GetPropID("nc.m_bAutoSortHole") == pItem->m_idProp)
	{
		if (strcmp(valueStr, "是") == 0)
			g_sysPara.nc.m_bAutoSortHole = TRUE;
		else
			g_sysPara.nc.m_bAutoSortHole = FALSE;
	}
	else if (CSysPara::GetPropID("nc.m_bSortByHoleD") == pItem->m_idProp)
	{
		if (strcmp(valueStr, "是") == 0)
			g_sysPara.nc.m_bSortByHoleD = TRUE;
		else
			g_sysPara.nc.m_bSortByHoleD = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_sysPara.nc.m_bSortByHoleD)
			oper.InsertCmbListPropItem(pItem, "nc.m_iGroupSortType", "", "", "", -1, TRUE);
	}
	else if (CSysPara::GetPropID("nc.m_iGroupSortType") == pItem->m_idProp)
		g_sysPara.nc.m_ciGroupSortType = valueStr[0] - '0';
	else if(CSysPara::GetPropID("nc.m_bDispMkRect")==pItem->m_idProp)
	{
		if(strcmp(valueStr,"是")==0)
			g_sysPara.nc.m_bDispMkRect=TRUE;
		else
			g_sysPara.nc.m_bDispMkRect=FALSE;
		g_sysPara.WriteSysParaToReg("DispMkRect");
		//
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.nc.m_bDispMkRect)
		{
			oper.InsertEditPropItem(pItem,"nc.m_fMKRectL","","",-1,TRUE);
			oper.InsertEditPropItem(pItem,"nc.m_fMKRectW","","",-1,TRUE);
		}
	}
	else if(CSysPara::GetPropID("nc.m_fMKRectL")==pItem->m_idProp)
	{
		g_sysPara.nc.m_fMKRectL=atof(valueStr);
		g_sysPara.WriteSysParaToReg("MKRectL");
	}
	else if(CSysPara::GetPropID("nc.m_fMKRectW")==pItem->m_idProp)
	{
		g_sysPara.nc.m_fMKRectW=atof(valueStr);
		g_sysPara.WriteSysParaToReg("MKRectW");
	}
	else if(CSysPara::GetPropID("nc.m_fMKHoleD")==pItem->m_idProp)
	{	
		g_sysPara.nc.m_fMKHoleD=atof(valueStr);
		g_sysPara.WriteSysParaToReg("MKHoleD");
	}
	else if(CSysPara::GetPropID("nc.m_fLimitSH")==pItem->m_idProp)
	{
		g_sysPara.nc.m_fLimitSH=atof(valueStr);
		g_sysPara.WriteSysParaToReg("LimitSH");
	}
	else if(CSysPara::GetPropID("nc.m_bNeedSH")==pItem->m_idProp)
	{
		if(valueStr.Compare("是")==0)
			g_sysPara.nc.m_bNeedSH=TRUE;
		else
			g_sysPara.nc.m_bNeedSH=FALSE;
		g_sysPara.WriteSysParaToReg("NeedSH");
	}
	else if (CSysPara::GetPropID("nc.m_iNcMode") == pItem->m_idProp)
	{
		g_sysPara.nc.m_iNcMode = 0;
		if (strstr(valueStr, "切割"))
			g_sysPara.AddNcFlag(CNCPart::CUT_MODE);
		if (strstr(valueStr, "板床"))
			g_sysPara.AddNcFlag(CNCPart::PROCESS_MODE);
		if (strstr(valueStr, "激光"))
			g_sysPara.AddNcFlag(CNCPart::LASER_MODE);
		g_sysPara.WriteSysParaToReg("NCMode");
#ifdef __PNC_
		CPropTreeItem *pCutItem = pPropList->FindItemByPropId(CSysPara::GetPropID("CutInfo"), NULL);
		CPropTreeItem *pProItem = pPropList->FindItemByPropId(CSysPara::GetPropID("ProcessInfo"), NULL);
		CPropTreeItem *pLasItem = pPropList->FindItemByPropId(CSysPara::GetPropID("LaserCutInfo"), NULL);
		if (pCutItem&&pProItem&&pLasItem)
		{
			if (g_sysPara.IsValidNcFlag(CNCPart::CUT_MODE))
			{
				if(pCutItem->m_bHideChildren)
					pPropList->Expand(pCutItem, pCutItem->m_iIndex);
			}
			else
				pPropList->Collapse(pCutItem);
			if (g_sysPara.IsValidNcFlag(CNCPart::PROCESS_MODE))
			{
				if(pProItem->m_bHideChildren)
					pPropList->Expand(pProItem, pProItem->m_iIndex);
			}
			else
				pPropList->Collapse(pProItem);
			if(g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
			{
				if(pLasItem->m_bHideChildren)
					pPropList->Expand(pLasItem, pLasItem->m_iIndex);
			}
			else
				pPropList->Collapse(pLasItem);
		}
#else
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.IsValidNcFlag(CNCPart::CUT_MODE))
			oper.InsertEditPropItem(pItem, "nc.m_fShapeAddDist", "", "", -1, TRUE);
		if(g_sysPara.IsValidNcFlag(CNCPart::PROCESS_MODE))
		{
			oper.InsertEditPropItem(pItem, "nc.m_sThickToPBJ", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem, "nc.m_sThickToPMZ", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.m_bNeedSH", "", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.m_bNeedMKRect", "", "", "", -1, TRUE);
		}
#endif
	}
#ifdef __PNC_
	else if(CSysPara::GetPropID("nc.m_bNeedMKRect")==pItem->m_idProp)
	{
		if(valueStr.Compare("是")==0)
			g_sysPara.nc.m_bNeedMKRect=TRUE;
		else
			g_sysPara.nc.m_bNeedMKRect=FALSE;
		g_sysPara.WriteSysParaToReg("NeedMKRect");
	}
	else if(CSysPara::GetPropID("nc.m_sThickToPBJ")==pItem->m_idProp)
	{
		g_sysPara.nc.m_sThickToPBJ.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("ThickToPBJ");
	}
	else if(CSysPara::GetPropID("nc.m_sThickToPMZ")==pItem->m_idProp)
	{
		g_sysPara.nc.m_sThickToPMZ.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("ThickToPMZ");
	}
	else if(CSysPara::GetPropID("nc.m_fShapeAddDist")==pItem->m_idProp)
	{
		g_sysPara.nc.m_fShapeAddDist=atof(valueStr);
		g_sysPara.WriteSysParaToReg("ShapeAddDist");
	}
	else if(CSysPara::GetPropID("nc.m_iDxfMode")==pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_iDxfMode = 1;
		else
			g_sysPara.nc.m_iDxfMode = 0;
		g_sysPara.WriteSysParaToReg("DxfMode");
	}

	else if (CSysPara::GetPropID("nc.bFlameCut") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.nc.m_bFlameCut = TRUE;
		else
			g_sysPara.nc.m_bFlameCut = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_sysPara.nc.m_bFlameCut)
		{
			oper.InsertEditPropItem(pItem, "nc.FlamePara.m_sThick", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.FlamePara.m_dwFileFlag", "", "", "", -1, TRUE);
		}
	}
	else if (CSysPara::GetPropID("nc.FlamePara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xFlamePara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.FlamePara.m_dwFileFlag") == pItem->m_idProp)
	{
		CXhChar100 sValue(valueStr);
		g_sysPara.nc.m_xFlamePara.m_dwFileFlag = 0;
		if (strstr(sValue, "DXF"))
			g_sysPara.nc.m_xFlamePara.AddFileFlag(CNCPart::PLATE_DXF_FILE);
		if (strstr(sValue, "TXT"))
			g_sysPara.nc.m_xFlamePara.AddFileFlag(CNCPart::PLATE_TXT_FILE);
		if (strstr(sValue, "CNC"))
			g_sysPara.nc.m_xFlamePara.AddFileFlag(CNCPart::PLATE_CNC_FILE);
	}
	else if (CSysPara::GetPropID("nc.bPlasmaCut") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.nc.m_bPlasmaCut = TRUE;
		else
			g_sysPara.nc.m_bPlasmaCut = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_sysPara.nc.m_bPlasmaCut)
		{
			oper.InsertEditPropItem(pItem, "nc.PlasmaPara.m_sThick", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.PlasmaPara.m_dwFileFlag", "", "", "", -1, TRUE);
		}
	}
	else if (CSysPara::GetPropID("nc.PlasmaPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xPlasmaPara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.PlasmaPara.m_dwFileFlag") == pItem->m_idProp)
	{
		CXhChar100 sValue(valueStr);
		g_sysPara.nc.m_xPlasmaPara.m_dwFileFlag = 0;
		if (strstr(sValue, "DXF"))
			g_sysPara.nc.m_xPlasmaPara.AddFileFlag(CNCPart::PLATE_DXF_FILE);
		if (strstr(sValue, "NC"))
			g_sysPara.nc.m_xPlasmaPara.AddFileFlag(CNCPart::PLATE_NC_FILE);
	}
	else if (CSysPara::GetPropID("nc.bPunchPress") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.nc.m_bPunchPress = TRUE;
		else
			g_sysPara.nc.m_bPunchPress = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_sysPara.nc.m_bPunchPress)
		{
			oper.InsertEditPropItem(pItem, "nc.PunchPara.m_sThick", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.PunchPara.m_dwFileFlag", "", "", "", -1, TRUE);
		}
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xPunchPara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.PunchPara.m_dwFileFlag") == pItem->m_idProp)
	{
		CXhChar100 sValue(valueStr);
		g_sysPara.nc.m_xPunchPara.m_dwFileFlag = 0;
		if (strstr(sValue, "DXF"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_DXF_FILE);
		if (strstr(sValue, "PBJ"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_PBJ_FILE);
		if(strstr(sValue, "WKF"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_WKF_FILE);
	}
	else if (CSysPara::GetPropID("nc.bDrillPress") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.nc.m_bDrillPress = TRUE;
		else
			g_sysPara.nc.m_bDrillPress = FALSE;
		//
		pPropList->DeleteAllSonItems(pItem);
		if (g_sysPara.nc.m_bDrillPress)
		{
			oper.InsertEditPropItem(pItem, "nc.DrillPara.m_sThick", "", "", -1, TRUE);
			oper.InsertCmbListPropItem(pItem, "nc.DrillPara.m_dwFileFlag", "", "", "", -1, TRUE);
		}
	}
	else if (CSysPara::GetPropID("nc.DrillPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xDrillPara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.DrillPara.m_dwFileFlag") == pItem->m_idProp)
	{
		CXhChar100 sValue(valueStr);
		g_sysPara.nc.m_xDrillPara.m_dwFileFlag = 0;
		if (strstr(sValue, "DXF"))
			g_sysPara.nc.m_xDrillPara.AddFileFlag(CNCPart::PLATE_DXF_FILE);
		if (strstr(sValue, "PMZ"))
			g_sysPara.nc.m_xDrillPara.AddFileFlag(CNCPart::PLATE_PMZ_FILE);
	}
	else if (CSysPara::GetPropID("nc.LaserPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xLaserPara.m_sThick.Copy(valueStr);
	else if(CSysPara::GetPropID("nc.m_fBaffleHigh")==pItem->m_idProp)
	{	
		g_sysPara.nc.m_fBaffleHigh=atof(valueStr);
		g_sysPara.WriteSysParaToReg("SideBaffleHigh");
	}
	else if(CSysPara::GetPropID("nc.m_sNcDriverPath")==pItem->m_idProp)
		g_sysPara.nc.m_sNcDriverPath=valueStr;
	else if(CSysPara::GetPropID("pbj.m_iPbjMode")==pItem->m_idProp)
	{
		g_sysPara.pbj.m_iPbjMode=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("PbjMode");
	}
	else if(CSysPara::GetPropID("pbj.m_bIncVertex")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			g_sysPara.pbj.m_bIncVertex=TRUE;
		else
			g_sysPara.pbj.m_bIncVertex=FALSE;
		g_sysPara.WriteSysParaToReg("PbjIncVertex");
	}
	else if (CSysPara::GetPropID("pbj.m_bIncSH") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.pbj.m_bIncSH = TRUE;
		else
			g_sysPara.pbj.m_bIncSH = FALSE;
		g_sysPara.WriteSysParaToReg("PbjIncSH");
	}
	else if(CSysPara::GetPropID("pbj.m_bAutoSplitFile")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			g_sysPara.pbj.m_bAutoSplitFile=TRUE;
		else
			g_sysPara.pbj.m_bAutoSplitFile=FALSE;
		g_sysPara.WriteSysParaToReg("PbjAutoSplitFile");
	}
	else if (CSysPara::GetPropID("pbj.m_bMergeHole") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.pbj.m_bMergeHole = TRUE;
		else
			g_sysPara.pbj.m_bMergeHole = FALSE;
		g_sysPara.WriteSysParaToReg("PbjMergeHole");
	}
	else if (CSysPara::GetPropID("pmz.m_iPmzMode") == pItem->m_idProp)
	{
		g_sysPara.pmz.m_iPmzMode = valueStr[0] - '0';
		g_sysPara.WriteSysParaToReg("PmzMode");
	}
	else if (CSysPara::GetPropID("pmz.m_bIncVertex") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.pmz.m_bIncVertex = TRUE;
		else
			g_sysPara.pmz.m_bIncVertex = FALSE;
		g_sysPara.WriteSysParaToReg("PmzIncVertex");
	}
	else if (CSysPara::GetPropID("pmz.m_bPmzCheck") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.pmz.m_bPmzCheck = TRUE;
		else
			g_sysPara.pmz.m_bPmzCheck = FALSE;
		g_sysPara.WriteSysParaToReg("PmzCheck");
	}
	else if (CSysPara::GetPropID("m_cDisplayCutType")==pItem->m_idProp)
	{
		g_sysPara.m_cDisplayCutType=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("m_cDisplayCutType");
		CPropTreeItem *pPlasmaCutItem=pPropList->FindItemByPropId(CSysPara::GetPropID("plasmaCut"),NULL);
		CPropTreeItem *pFlameCutItem=pPropList->FindItemByPropId(CSysPara::GetPropID("flameCut"),NULL);
		if(pPlasmaCutItem&&pFlameCutItem)
		{
			if(g_sysPara.m_cDisplayCutType==0)
			{
				pPropList->Collapse(pPlasmaCutItem);
				if(pFlameCutItem->m_bHideChildren)
					pPropList->Expand(pFlameCutItem,pFlameCutItem->m_iIndex);
			}
			else
			{
				pPropList->Collapse(pFlameCutItem);
				if(pFlameCutItem->m_bHideChildren)
					pPropList->Expand(pPlasmaCutItem,pPlasmaCutItem->m_iIndex);

			}
		}
	}
	else if (CSysPara::GetPropID("plasmaCut.m_sOutLineLen")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_sOutLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_sOutLineLen");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_sIntoLineLen")==pItem->m_idProp)
	{	
		g_sysPara.plasmaCut.m_sIntoLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_sIntoLineLen");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bInitPosFarOrg")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_bInitPosFarOrg=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bInitPosFarOrg");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bCutPosInInitPos")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_bCutPosInInitPos=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bCutPosInInitPos");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_wEnlargedSpace")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_wEnlargedSpace=atoi(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_wEnlargedSpace");
	}
	else if (CSysPara::GetPropID("flameCut.m_sOutLineLen")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_sOutLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_sOutLineLen");
	}
	else if (CSysPara::GetPropID("flameCut.m_sIntoLineLen")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_sIntoLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_sIntoLineLen");
	}
	else if (CSysPara::GetPropID("flameCut.m_bInitPosFarOrg")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_bInitPosFarOrg=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("flameCut.m_bInitPosFarOrg");
	}
	else if (CSysPara::GetPropID("flameCut.m_bCutPosInInitPos")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_bCutPosInInitPos=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("flameCut.m_bCutPosInInitPos");
	}
	else if (CSysPara::GetPropID("flameCut.m_wEnlargedSpace")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_wEnlargedSpace=atoi(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_wEnlargedSpace");
	}
#endif
	else if(CSysPara::GetPropID("holeIncrement.m_fDatum")==pItem->m_idProp)
	{
		g_sysPara.UpdateHoleIncrement(atof(valueStr));
		UpdateHoleIncrementProperty(pPropList,pItem);
		bUpdateHoleInc=TRUE;
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fM12")==pItem->m_idProp)
	{
		g_sysPara.holeIncrement.m_fM12=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fM16")==pItem->m_idProp)
	{	
		g_sysPara.holeIncrement.m_fM16=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fM20")==pItem->m_idProp)
	{	
		g_sysPara.holeIncrement.m_fM20=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fM24")==pItem->m_idProp)
	{	
		g_sysPara.holeIncrement.m_fM24=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fMSH")==pItem->m_idProp)
	{	
		g_sysPara.holeIncrement.m_fMSH=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if(pItem->m_idProp>=CSysPara::GetPropID("crMode.crLS12") &&
		pItem->m_idProp<=CSysPara::GetPropID("crMode.crMarK"))
	{
		COLORREF curClr = 0;
		char tem_str[100]="";
		sprintf(tem_str,"%s",valueStr);
		memmove(tem_str, tem_str+3, 97);//跳过RGB
		sscanf(tem_str,"%X",&curClr);
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crLS12")&&g_sysPara.crMode.crLS12!=curClr)
		{
			g_sysPara.crMode.crLS12=curClr;
			g_sysPara.WriteSysParaToReg("M12Color");
		}
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crLS16")&&g_sysPara.crMode.crLS16!=curClr)
		{	
			g_sysPara.crMode.crLS16=curClr;
			g_sysPara.WriteSysParaToReg("M16Color");
		}
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crLS20")&&g_sysPara.crMode.crLS20!=curClr)
		{	
			g_sysPara.crMode.crLS20=curClr;
			g_sysPara.WriteSysParaToReg("M20Color");
		}
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crLS24")&&g_sysPara.crMode.crLS24!=curClr)
		{	
			g_sysPara.crMode.crLS24=curClr;
			g_sysPara.WriteSysParaToReg("M24Color");
		}
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crOtherLS")&&g_sysPara.crMode.crOtherLS!=curClr)
		{	
			g_sysPara.crMode.crOtherLS=curClr;
			g_sysPara.WriteSysParaToReg("OtherColor");
		}
		if(pItem->m_idProp==CSysPara::GetPropID("crMode.crMarK")&&g_sysPara.crMode.crMark!=curClr)
		{
			g_sysPara.crMode.crMark=curClr;
			g_sysPara.WriteSysParaToReg("MarkColor");
		}
	}
	else if(CSysPara::GetPropID("jgDrawing.sAngleCardPath")==pItem->m_idProp)
	{
		g_sysPara.jgDrawing.sAngleCardPath=valueStr;
		g_pPartEditor->SetProcessCardPath(g_sysPara.jgDrawing.sAngleCardPath);
	}
	else 
	{
		BOOL bUpdateJgPara=TRUE;
	if(CSysPara::GetPropID("font.fDimTextSize")==pItem->m_idProp)
		g_sysPara.font.fDimTextSize=atof(valueStr);
	else if(CSysPara::GetPropID("font.fPartNoTextSize")==pItem->m_idProp)
		g_sysPara.font.fPartNoTextSize=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.iDimPrecision")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("1.0mm"))
			g_sysPara.jgDrawing.iDimPrecision=0;
		else if(valueStr.CompareNoCase("0.5mm"))
			g_sysPara.jgDrawing.iDimPrecision=1;
		else //if(valueStr.CompareNoCase("0.1mm"))
			g_sysPara.jgDrawing.iDimPrecision=2;
	}
	else if(CSysPara::GetPropID("jgDrawing.fRealToDraw")==pItem->m_idProp)
		g_sysPara.jgDrawing.fRealToDraw=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.fDimArrowSize")==pItem->m_idProp)
		g_sysPara.jgDrawing.fDimArrowSize=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.fTextXFactor")==pItem->m_idProp)
		g_sysPara.jgDrawing.fTextXFactor=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.iPartNoFrameStyle")==pItem->m_idProp)
		g_sysPara.jgDrawing.iPartNoFrameStyle=valueStr[0]-'0';
	else if(CSysPara::GetPropID("jgDrawing.fPartNoMargin")==pItem->m_idProp)
		g_sysPara.jgDrawing.fPartNoMargin=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.fPartNoCirD")==pItem->m_idProp)
		g_sysPara.jgDrawing.fPartNoCirD=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.fPartGuigeTextSize")==pItem->m_idProp)
		g_sysPara.jgDrawing.fPartGuigeTextSize=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.iMatCharPosType")==pItem->m_idProp)
		g_sysPara.jgDrawing.iMatCharPosType=valueStr[0]-'0';
	//角钢构件图设置
	else if(CSysPara::GetPropID("jgDrawing.fLsDistThreshold")==pItem->m_idProp)
		g_sysPara.jgDrawing.fLsDistThreshold=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.fLsDistZoomCoef")==pItem->m_idProp)
		g_sysPara.jgDrawing.fLsDistZoomCoef=atof(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.bOneCardMultiPart")==pItem->m_idProp)
		g_sysPara.jgDrawing.bOneCardMultiPart=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bModulateLongJg")==pItem->m_idProp)
		g_sysPara.jgDrawing.bModulateLongJg=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.iJgZoomSchema")==pItem->m_idProp)
		g_sysPara.jgDrawing.iJgZoomSchema=valueStr[0]-'0';
	else if(CSysPara::GetPropID("jgDrawing.bMaxExtendAngleLength")==pItem->m_idProp)
		g_sysPara.jgDrawing.bMaxExtendAngleLength=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.iJgGDimStyle")==pItem->m_idProp)
	{
		g_sysPara.jgDrawing.iJgGDimStyle=valueStr[0]-'0';
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.jgDrawing.iJgGDimStyle==2)	//自动判断
			oper.InsertEditPropItem(pItem,"jgDrawing.nMaxBoltNumStartDimG","","",-1,TRUE);
	}
	else if(CSysPara::GetPropID("jgDrawing.nMaxBoltNumStartDimG")==pItem->m_idProp)
		g_sysPara.jgDrawing.nMaxBoltNumStartDimG=atoi(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.iLsSpaceDimStyle")==pItem->m_idProp)
	{
		g_sysPara.jgDrawing.iLsSpaceDimStyle=valueStr[0]-'0';
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.jgDrawing.iLsSpaceDimStyle==2)	//沿X轴方向标注支持的最大螺栓个数
			oper.InsertEditPropItem(pItem,"jgDrawing.nMaxBoltNumAlongX","","",-1,TRUE);
	}
	else if(CSysPara::GetPropID("jgDrawing.nMaxBoltNumAlongX")==pItem->m_idProp)
		g_sysPara.jgDrawing.nMaxBoltNumAlongX=atoi(valueStr);
	else if(CSysPara::GetPropID("jgDrawing.bDimCutAngle")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimCutAngle=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimCutAngleMap")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimCutAngleMap=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimPushFlatMap")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimPushFlatMap=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiHe")==pItem->m_idProp)
	{
		g_sysPara.jgDrawing.bDimKaiHe=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.jgDrawing.bDimKaiHe)
		{	
			oper.InsertCmbListPropItem(pItem,"jgDrawing.bDimKaiheAngle","","","",-1,TRUE);
			oper.InsertCmbListPropItem(pItem,"jgDrawing.bDimKaiheSumLen","","","",-1,TRUE);
			oper.InsertCmbListPropItem(pItem,"jgDrawing.bDimKaiheSegLen","","","",-1,TRUE);
			oper.InsertCmbListPropItem(pItem,"jgDrawing.bDimKaiheScopeMap","","","",-1,TRUE);
		}
	}
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiheAngleMap")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimKaiheAngleMap=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiheSumLen")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimKaiheSumLen=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiheAngle")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimKaiheAngle=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiheSegLen")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimKaiheSegLen=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimKaiheScopeMap")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimKaiheScopeMap=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bJgUseSimpleLsMap")==pItem->m_idProp)
		g_sysPara.jgDrawing.bJgUseSimpleLsMap=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimLsAbsoluteDist")==pItem->m_idProp)
	{
		g_sysPara.jgDrawing.bDimLsAbsoluteDist=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
		pPropList->DeleteAllSonItems(pItem);
		if(g_sysPara.jgDrawing.bDimLsAbsoluteDist)
			oper.InsertCmbListPropItem(pItem,"jgDrawing.bMergeLsAbsoluteDist","","","",-1,TRUE);
	}
	else if(CSysPara::GetPropID("jgDrawing.bMergeLsAbsoluteDist")==pItem->m_idProp)
		g_sysPara.jgDrawing.bMergeLsAbsoluteDist=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimRibPlatePartNo")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimRibPlatePartNo=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.bDimRibPlateSetUpPos")==pItem->m_idProp)
		g_sysPara.jgDrawing.bDimRibPlateSetUpPos=valueStr.CompareNoCase("是")==0?TRUE:FALSE;
	else if(CSysPara::GetPropID("jgDrawing.iCutAngleDimType")==pItem->m_idProp)
		g_sysPara.jgDrawing.iCutAngleDimType=valueStr[0]-'0';
	else if(CSysPara::GetPropID("jgDrawing.fKaiHeJiaoThreshold")==pItem->m_idProp)
		g_sysPara.jgDrawing.fKaiHeJiaoThreshold=atof(valueStr);
	else
		bUpdateJgPara=FALSE;
		if(bUpdateJgPara)
		{
			CBuffer buffer;
			g_sysPara.AngleDrawingParaToBuffer(buffer);
			g_pPartEditor->UpdateJgDrawingPara(buffer.GetBufferPtr(),buffer.GetLength());
			g_pPartEditor->ReDraw();
		}
	}
	//
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(bUpdateHoleInc)
		pView->AmendHoleIncrement();
	return TRUE;
}

BOOL SyssettingButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	if(CSysPara::GetPropID("nc.m_sNcDriverPath")==pItem->m_idProp)
	{
		CFileDialog FileDlg(TRUE,"drv",NULL,NULL,"角钢NC驱动(*.drv)|*.drv||");
		CString currPath = pItem->m_lpNodeInfo->m_strPropValue;
		FileDlg.m_ofn.lpstrTitle = "Select file";
		if (currPath.GetLength() > 0)
			FileDlg.m_ofn.lpstrInitialDir = currPath.Left(
			currPath.GetLength() - currPath.ReverseFind('\\'));
		if(IDOK==FileDlg.DoModal())
		{
			g_sysPara.nc.m_sNcDriverPath=FileDlg.GetPathName();
			pPropList->SetItemPropValue(pItem->m_idProp,g_sysPara.nc.m_sNcDriverPath);
		}
	}
	else if(CSysPara::GetPropID("jgDrawing.sAngleCardPath")==pItem->m_idProp)
	{
		CFileDialog FileDlg(TRUE,"pcd","ProcessCard",NULL,"角钢工艺卡(*.pcd)|*.pcd||");
		CString currPath = pItem->m_lpNodeInfo->m_strPropValue;
		FileDlg.m_ofn.lpstrTitle = "选择角钢工艺卡";
		if (currPath.GetLength() > 0)
			FileDlg.m_ofn.lpstrInitialDir = currPath.Left(
			currPath.GetLength() - currPath.ReverseFind('\\'));
		if(IDOK==FileDlg.DoModal())
		{
			g_sysPara.jgDrawing.sAngleCardPath=FileDlg.GetPathName();
			g_pPartEditor->SetProcessCardPath(g_sysPara.jgDrawing.sAngleCardPath);
			pPropList->SetItemPropValue(pItem->m_idProp,g_sysPara.jgDrawing.sAngleCardPath);
			g_pPartEditor->ReDraw();
		}
	}
	else if(CSysPara::GetPropID("holeIncrement.m_fDatum")==pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		if(dlg.DoModal()!=IDOK)
			return FALSE;
		double fDatum=g_sysPara.holeIncrement.m_fDatum;
		if(dlg.m_arrIsCanUse[0])
			g_sysPara.holeIncrement.m_fM12=dlg.m_fM12Increment;
		else
			g_sysPara.holeIncrement.m_fM12=fDatum;
		if(dlg.m_arrIsCanUse[1])
			g_sysPara.holeIncrement.m_fM16=dlg.m_fM16Increment;
		else
			g_sysPara.holeIncrement.m_fM16=fDatum;
		if(dlg.m_arrIsCanUse[2])
			g_sysPara.holeIncrement.m_fM20=dlg.m_fM20Increment;
		else
			g_sysPara.holeIncrement.m_fM20=fDatum;
		if(dlg.m_arrIsCanUse[3])
			g_sysPara.holeIncrement.m_fM24=dlg.m_fM24Increment;
		else
			g_sysPara.holeIncrement.m_fM24=fDatum;
		if(dlg.m_arrIsCanUse[4])
			g_sysPara.holeIncrement.m_fMSH=dlg.m_fSpcIncrement;
		else
			g_sysPara.holeIncrement.m_fMSH=fDatum;
		UpdateHoleIncrementProperty(pPropList,pItem);
		//
		CPPEView *pView=(CPPEView*)theApp.GetView();
		pView->AmendHoleIncrement();
	}
	else if (CSysPara::GetPropID("nc.LaserPara.Config") == pItem->m_idProp)
	{
		CPlankConfigDlg dlg;
		dlg.DoModal();
	}
	else 
		return FALSE;
	return TRUE;
}
BOOL CPPEView::DisplaySysSettingProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL)
		return FALSE;
	const int GROUP_NCINFO = 1, GROUP_PROCESSCARD_INFO = 2, GROUP_DISPLAY = 3;
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
#if defined(__PNC_)||defined(__PROCESS_PLATE_)
	pPropDlg->m_arrPropGroupLabel.SetSize(3);
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_NCINFO-1,"常规");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_PROCESSCARD_INFO - 1, "输出");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_DISPLAY - 1, "显示");
#else
	pPropDlg->m_arrPropGroupLabel.SetSize(3);
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_NCINFO-1,"常规");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_PROCESSCARD_INFO-1,"工艺卡");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_DISPLAY - 1, "显示");
#endif
	pPropDlg->RefreshTabCtrl(0);
	//
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->SetModifyValueFunc(ModifySyssettingProperty);
	pPropList->SetButtonClickFunc(SyssettingButtonClick);
	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL,*pRootItem = pPropList->GetRootItem();
	CPropertyListOper<CSysPara> oper(pPropList, &g_sysPara);
	//文件属性
	pParentItem = oper.InsertPropItem(pRootItem, "Model");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_NCINFO);
	oper.InsertEditPropItem(pParentItem, "model.m_sCompanyName");
	oper.InsertEditPropItem(pParentItem, "model.m_sPrjCode");
	oper.InsertEditPropItem(pParentItem, "model.m_sPrjName");
	oper.InsertEditPropItem(pParentItem, "model.m_sTaType");
	oper.InsertEditPropItem(pParentItem, "model.m_sTaAlias");
	oper.InsertEditPropItem(pParentItem, "model.m_sTaStampNo");
	oper.InsertEditPropItem(pParentItem, "model.m_sOperator");
	oper.InsertEditPropItem(pParentItem, "model.m_sAuditor");
	oper.InsertEditPropItem(pParentItem, "model.m_sCritic");
	pParentItem->m_bHideChildren = FALSE;
	//NC
	pParentItem = oper.InsertPropItem(pRootItem, "NC");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_NCINFO);
#if defined(__PNC_)||defined(__PROCESS_PLATE_)
	oper.InsertEditPropItem(pParentItem,"nc.m_fBaffleHigh");
	oper.InsertEditPropItem(pParentItem,"nc.m_fMKHoleD");
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"nc.m_bDispMkRect");
	if(g_sysPara.nc.m_bDispMkRect)
	{
		oper.InsertEditPropItem(pPropItem,"nc.m_fMKRectL");
		oper.InsertEditPropItem(pPropItem,"nc.m_fMKRectW");
	}
	oper.InsertEditPropItem(pParentItem,"nc.m_fLimitSH");
	pPropItem=oper.InsertBtnEditPropItem(pParentItem,"holeIncrement.m_fDatum");
	UpdateHoleIncrementProperty(pPropList,pPropItem);
#endif
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
	{	//支持螺栓优化功能
		pPropItem=oper.InsertCmbListPropItem(pParentItem, "nc.m_bSortByHoleD");
		if (TRUE)
			oper.InsertCmbListPropItem(pPropItem, "nc.m_iGroupSortType");
		//不开放自动优化螺栓孔，优化耗时长，影响操作流畅度
		//oper.InsertCmbListPropItem(pParentItem, "nc.m_bAutoSortHole");		
	}
	//钢板切割模式
	oper.InsertCmbListPropItem(pParentItem, "m_cDisplayCutType");
	//火焰切割
	pPropItem=oper.InsertButtonPropItem(pParentItem,"flameCut");
	oper.InsertCmbListPropItem(pPropItem,"flameCut.m_bInitPosFarOrg");
	oper.InsertCmbListPropItem(pPropItem,"flameCut.m_bCutPosInInitPos");
	oper.InsertCmbEditPropItem(pPropItem,"flameCut.m_sIntoLineLen");
	oper.InsertCmbEditPropItem(pPropItem,"flameCut.m_sOutLineLen");
	oper.InsertEditPropItem(pPropItem,"flameCut.m_wEnlargedSpace");
	pPropItem->m_bHideChildren=(g_sysPara.m_cDisplayCutType!=0);
	//等离子切割
	pPropItem=oper.InsertButtonPropItem(pParentItem,"plasmaCut");
	oper.InsertCmbListPropItem(pPropItem,"plasmaCut.m_bInitPosFarOrg");
	oper.InsertCmbListPropItem(pPropItem,"plasmaCut.m_bCutPosInInitPos");
	oper.InsertCmbEditPropItem(pPropItem,"plasmaCut.m_sIntoLineLen");
	oper.InsertCmbEditPropItem(pPropItem,"plasmaCut.m_sOutLineLen");
	pPropItem->m_bHideChildren=(g_sysPara.m_cDisplayCutType==0);
#endif
#ifndef __PNC_
	oper.InsertFilePathPropItem(pParentItem,"nc.m_sNcDriverPath");
#endif
#if defined(__PNC_)||defined(__PROCESS_PLATE_)
	//钢板输出设置
	pParentItem = oper.InsertPropItem(pRootItem, "OutPutSet");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_PROCESSCARD_INFO);
	oper.InsertCmbListPropItem(pParentItem, "nc.m_iDxfMode");
#ifdef __PROCESS_PLATE_
	pPropItem = oper.InsertCmbListPropItem(pParentItem, "nc.m_iNcMode");
	if(g_sysPara.IsValidNcFlag(CNCPart::CUT_MODE))
		oper.InsertEditPropItem(pPropItem, "nc.m_fShapeAddDist");
	if(g_sysPara.IsValidNcFlag(CNCPart::PROCESS_MODE))
	{
		oper.InsertEditPropItem(pPropItem, "nc.m_sThickToPBJ");
		oper.InsertEditPropItem(pPropItem, "nc.m_sThickToPMZ");
		oper.InsertCmbListPropItem(pPropItem, "nc.m_bNeedSH");
		oper.InsertCmbListPropItem(pPropItem, "nc.m_bNeedMKRect");
	}
#else
	oper.InsertCmbListPropItem(pParentItem, "nc.m_iNcMode");
	//切割下料
	CPropTreeItem *pGroupItem = oper.InsertEditPropItem(pParentItem, "CutInfo");
	oper.InsertEditPropItem(pGroupItem, "nc.m_fShapeAddDist");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.bFlameCut");
	if (g_sysPara.nc.m_bFlameCut)
	{
		oper.InsertEditPropItem(pPropItem, "nc.FlamePara.m_sThick");
		oper.InsertCmbListPropItem(pPropItem, "nc.FlamePara.m_dwFileFlag");
	}
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.bPlasmaCut");
	if (g_sysPara.nc.m_bPlasmaCut)
	{
		oper.InsertEditPropItem(pPropItem, "nc.PlasmaPara.m_sThick");
		oper.InsertCmbListPropItem(pPropItem, "nc.PlasmaPara.m_dwFileFlag");
	}
	pGroupItem->m_bHideChildren = (!g_sysPara.IsValidNcFlag(CNCPart::CUT_MODE));
	//板床加工
	pGroupItem = oper.InsertEditPropItem(pParentItem, "ProcessInfo");
	oper.InsertCmbListPropItem(pGroupItem, "nc.m_bNeedSH");
	oper.InsertCmbListPropItem(pGroupItem, "nc.m_bNeedMKRect");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.bPunchPress");
	if (g_sysPara.nc.m_bPunchPress)
	{
		oper.InsertEditPropItem(pPropItem, "nc.PunchPara.m_sThick");
		oper.InsertCmbListPropItem(pPropItem, "nc.PunchPara.m_dwFileFlag");
	}
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.bDrillPress");
	if (g_sysPara.nc.m_bDrillPress)
	{
		oper.InsertEditPropItem(pPropItem, "nc.DrillPara.m_sThick");
		oper.InsertCmbListPropItem(pPropItem, "nc.DrillPara.m_dwFileFlag");
	}
	pGroupItem->m_bHideChildren = (!g_sysPara.IsValidNcFlag(CNCPart::PROCESS_MODE));
	//激光复合机
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "LaserCutInfo");
	oper.InsertEditPropItem(pGroupItem, "nc.LaserPara.m_sThick");
	oper.InsertButtonPropItem(pGroupItem, "nc.LaserPara.Config");
	pPropItem=oper.InsertCmbListPropItem(pGroupItem, "nc.LaserPara.m_dwFileFlag");
	pPropItem->SetReadOnly(TRUE);
	pGroupItem->m_bHideChildren = (!g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE));
	//文件设置
	pParentItem = oper.InsertPropItem(pRootItem, "FileSet");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_PROCESSCARD_INFO);
	//PBJ文件设置
	pPropItem = oper.InsertEditPropItem(pParentItem, "PbjPara");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bIncVertex");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bIncSH");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bAutoSplitFile");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bMergeHole");
	pPropItem = oper.InsertEditPropItem(pParentItem, "PmzPara");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_iPmzMode");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_bIncVertex");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_bPmzCheck");
#endif
#endif
#ifndef __PNC_
	//角钢工艺卡
	pParentItem=oper.InsertPropItem(pRootItem,"JgDrawing");
	pParentItem->m_dwPropGroup=GetSingleWord(GROUP_PROCESSCARD_INFO);
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.iJgZoomSchema");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bMaxExtendAngleLength");
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"jgDrawing.iJgGDimStyle");
	pPropItem->m_bHideChildren=FALSE;
	if(g_sysPara.jgDrawing.iJgGDimStyle==2)	//自动判断
		oper.InsertEditPropItem(pPropItem,"jgDrawing.nMaxBoltNumStartDimG");
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"jgDrawing.iLsSpaceDimStyle");
	pPropItem->m_bHideChildren=FALSE;
	if(g_sysPara.jgDrawing.iLsSpaceDimStyle==2)	//沿X轴方向标注支持的最大螺栓个数
		oper.InsertEditPropItem(pPropItem,"jgDrawing.nMaxBoltNumAlongX");
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimLsAbsoluteDist");
	pPropItem->m_bHideChildren=FALSE;
	if(g_sysPara.jgDrawing.bDimLsAbsoluteDist)
		oper.InsertCmbListPropItem(pPropItem,"jgDrawing.bMergeLsAbsoluteDist");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimPushFlatMap");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimCutAngle");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bOneCardMultiPart");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bJgUseSimpleLsMap");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimRibPlatePartNo");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimRibPlateSetUpPos");
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimCutAngleMap");
	//角钢切角标注样式 wht 10-11-02
	oper.InsertCmbListPropItem(pParentItem,"jgDrawing.iCutAngleDimType");
	//开合角标注阈值 wht 11-05-06
	oper.InsertEditPropItem(pParentItem,"jgDrawing.fKaiHeJiaoThreshold");
	//
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"jgDrawing.bDimKaiHe");
	if(g_sysPara.jgDrawing.bDimKaiHe)
	{	
		oper.InsertCmbListPropItem(pPropItem,"jgDrawing.bDimKaiheAngle");
		oper.InsertCmbListPropItem(pPropItem,"jgDrawing.bDimKaiheSumLen");
		oper.InsertCmbListPropItem(pPropItem,"jgDrawing.bDimKaiheSegLen");
		oper.InsertCmbListPropItem(pPropItem,"jgDrawing.bDimKaiheScopeMap");
	}
	//角钢工艺卡路径
	oper.InsertFilePathPropItem(pParentItem,"jgDrawing.sAngleCardPath");
#endif
	//颜色方案
	pParentItem = oper.InsertPropItem(pRootItem, "CRMODE");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_DISPLAY);
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS12");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS16");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS20");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS24");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crOtherLS");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crMarK");
	//字体设置
	pParentItem = oper.InsertPropItem(pRootItem, "Font");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_DISPLAY);
	oper.InsertEditPropItem(pParentItem, "font.fTextHeight");
#ifndef __PNC_
	oper.InsertEditPropItem(pParentItem, "font.fDimTextSize");
	oper.InsertEditPropItem(pParentItem, "font.fPartNoTextSize");
#endif
	pPropList->Redraw();
	return TRUE;
}