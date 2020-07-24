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
#include "FileFormatSetDlg.h"
#include "folder_dialog.h"
#include "SelPlateNcModeDlg.h"
#include "SelDisplayNcDlg.h"
#include "FilterMkDlg.h"

void UpdateHoleIncrementProperty(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	if(pParentItem==NULL)
		return ;
	CPropertyListOper<CSysPara> oper(pPropList,&g_sysPara);
	pPropList->DeleteAllSonItems(pParentItem);
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
	//CutSH
	fValue=g_sysPara.holeIncrement.m_fCutSH;
	if(fabs(fValue-fDatum)>EPS)
		oper.InsertEditPropItem(pParentItem,"holeIncrement.m_fCutSH", "", "", -1, TRUE);
	//ProSH
	fValue = g_sysPara.holeIncrement.m_fProSH;
	if (fabs(fValue - fDatum) > EPS)
		oper.InsertEditPropItem(pParentItem, "holeIncrement.m_fProSH", "", "", -1, TRUE);
}
BOOL ModifySyssettingProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CPropertyListOper<CSysPara> oper(pPropList,&g_sysPara);
	BOOL bUpdateHoleInc = FALSE, bUpdateCutInfo = FALSE;
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
	else if (CSysPara::GetPropID("font.fDxfTextSize") == pItem->m_idProp)
	{
		g_sysPara.font.fDxfTextSize = atof(valueStr);
		g_sysPara.WriteSysParaToReg("DxfTextSize");
	}
	else if (CSysPara::GetPropID("font.fTextHeight") == pItem->m_idProp)
	{
		g_sysPara.font.fTextHeight = atof(valueStr);
		g_sysPara.WriteSysParaToReg("TextHeight");
	}
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
			oper.InsertCmbListPropItem(pItem, "nc.m_ciMkVect", "", "", "", -1, TRUE);
			oper.InsertEditPropItem(pItem,"nc.m_fMKRectL","","",-1,TRUE);
			oper.InsertEditPropItem(pItem,"nc.m_fMKRectW","","",-1,TRUE);
		}
	}
	else if (CSysPara::GetPropID("nc.m_ciMkVect") == pItem->m_idProp)
	{
		g_sysPara.nc.m_ciMkVect = valueStr[0] - '0';
		g_sysPara.WriteSysParaToReg("MKVectType");
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
		//
		oper.UpdatePropItemValue("CutLimitSH");
		oper.UpdatePropItemValue("ProLimitSH");
	}
	else if (CSysPara::GetPropID("OutputPath") == pItem->m_idProp)
	{
		model.m_sOutputPath.Copy(valueStr);
	}
	else if(CSysPara::GetPropID("nc.DrillPara.m_bReserveBigSH")==pItem->m_idProp)
	{
		if (valueStr.Compare("是") == 0)
			g_sysPara.nc.m_xDrillPara.m_bReserveBigSH = TRUE;
		else
			g_sysPara.nc.m_xDrillPara.m_bReserveBigSH = FALSE;
		CPropTreeItem* pFindItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.DrillPara.m_bSortHasBigSH"), NULL);
		if (pFindItem)
		{
			pFindItem->SetReadOnly(!g_sysPara.nc.m_xDrillPara.m_bReserveBigSH);
			oper.UpdatePropItemValue("nc.DrillPara.m_bSortHasBigSH");
		}
		//
		g_sysPara.WriteSysParaToReg("DrillNeedSH");
	}
	else if (CSysPara::GetPropID("nc.DrillPara.m_bSortHasBigSH") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xDrillPara.m_bSortHasBigSH = TRUE;
		else
			g_sysPara.nc.m_xDrillPara.m_bSortHasBigSH = FALSE;
		g_sysPara.WriteSysParaToReg("DrillSortHasBigSH");
	}
	else if (CSysPara::GetPropID("nc.DrillPara.m_bReduceSmallSH") == pItem->m_idProp)
	{
		if (valueStr.Compare("是") == 0)
			g_sysPara.nc.m_xDrillPara.m_bReduceSmallSH = TRUE;
		else
			g_sysPara.nc.m_xDrillPara.m_bReduceSmallSH = FALSE;
		g_sysPara.WriteSysParaToReg("DrillReduceSmallSH");
		bUpdateHoleInc = TRUE;
	}
	else if (CSysPara::GetPropID("nc.DrillPara.m_ciHoldSortType") == pItem->m_idProp)
	{
		g_sysPara.nc.m_xDrillPara.m_ciHoldSortType = valueStr[0] - '0';
		g_sysPara.WriteSysParaToReg("DrillHoldSortType");
	}
#ifdef __PNC_
	else if(CSysPara::GetPropID("nc.m_iDxfMode")==pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_iDxfMode = 1;
		else
			g_sysPara.nc.m_iDxfMode = 0;
	}

	else if (CSysPara::GetPropID("nc.bFlameCut") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.AddNcFlag(CNCPart::FLAME_MODE);
		else
			g_sysPara.DelNcFlag(CNCPart::FLAME_MODE);
		//刷新属性栏
		if (g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE))
			pPropList->Expand(pItem, pItem->m_iIndex);
		else
			pPropList->Collapse(pItem);
		oper.UpdatePropItemValue("nc.m_iNcMode");
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
			g_sysPara.AddNcFlag(CNCPart::PLASMA_MODE);
		else
			g_sysPara.DelNcFlag(CNCPart::PLASMA_MODE);
		//刷新属性栏
		if (g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE))
			pPropList->Expand(pItem, pItem->m_iIndex);
		else
			pPropList->Collapse(pItem);
		oper.UpdatePropItemValue("nc.m_iNcMode");
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
			g_sysPara.AddNcFlag(CNCPart::PUNCH_MODE);
		else
			g_sysPara.DelNcFlag(CNCPart::PUNCH_MODE);
		//刷新属性栏
		if (g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE))
		{
			if (pItem->m_bHideChildren)
				pPropList->Expand(pItem, pItem->m_iIndex);
		}
		else
			pPropList->Collapse(pItem);
		oper.UpdatePropItemValue("nc.m_iNcMode");
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xPunchPara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.PunchPara.m_bReserveBigSH") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xPunchPara.m_bReserveBigSH = TRUE;
		else
			g_sysPara.nc.m_xPunchPara.m_bReserveBigSH = FALSE;
		CPropTreeItem* pFindItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.PunchPara.m_bSortHasBigSH"), NULL);
		if (pFindItem)
		{
			pFindItem->SetReadOnly(!g_sysPara.nc.m_xPunchPara.m_bReserveBigSH);
			oper.UpdatePropItemValue("nc.PunchPara.m_bSortHasBigSH");
		}
		//
		g_sysPara.WriteSysParaToReg("PunchNeedSH");
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_bSortHasBigSH") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xPunchPara.m_bSortHasBigSH = TRUE;
		else
			g_sysPara.nc.m_xPunchPara.m_bSortHasBigSH = FALSE;
		g_sysPara.WriteSysParaToReg("PunchSortHasBigSH");
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_bReduceSmallSH") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xPunchPara.m_bReduceSmallSH = TRUE;
		else
			g_sysPara.nc.m_xPunchPara.m_bReduceSmallSH = FALSE;
		g_sysPara.WriteSysParaToReg("PunchReduceSmallSH");
		bUpdateHoleInc = TRUE;
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_ciHoldSortType") == pItem->m_idProp)
	{
		g_sysPara.nc.m_xPunchPara.m_ciHoldSortType = valueStr[0] - '0';
		g_sysPara.WriteSysParaToReg("PunchHoldSortType");
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_dwFileFlag") == pItem->m_idProp)
	{
		CXhChar100 sValue(valueStr);
		g_sysPara.nc.m_xPunchPara.m_dwFileFlag = 0;
		if (strstr(sValue, "DXF"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_DXF_FILE);
		if (strstr(sValue, "PBJ"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_PBJ_FILE);
		if (strstr(sValue, "WKF"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_WKF_FILE);
		if (strstr(sValue, "TTP"))
			g_sysPara.nc.m_xPunchPara.AddFileFlag(CNCPart::PLATE_TTP_FILE);
	}
	else if (CSysPara::GetPropID("nc.bDrillPress") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.AddNcFlag(CNCPart::DRILL_MODE);
		else
			g_sysPara.DelNcFlag(CNCPart::DRILL_MODE);
		//刷新属性栏
		if (g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE))
		{
			if (pItem->m_bHideChildren)
				pPropList->Expand(pItem, pItem->m_iIndex);
		}
		else
			pPropList->Collapse(pItem);
		oper.UpdatePropItemValue("nc.m_iNcMode");
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
	else if (CSysPara::GetPropID("nc.bLaser") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("启用") == 0)
			g_sysPara.AddNcFlag(CNCPart::LASER_MODE);
		else
			g_sysPara.DelNcFlag(CNCPart::LASER_MODE);
		//刷新属性栏
		if (g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
		{
			if (pItem->m_bHideChildren)
				pPropList->Expand(pItem, pItem->m_iIndex);
		}
		else
			pPropList->Collapse(pItem);
		oper.UpdatePropItemValue("nc.m_iNcMode");
	}
	else if (CSysPara::GetPropID("nc.LaserPara.m_sThick") == pItem->m_idProp)
		g_sysPara.nc.m_xLaserPara.m_sThick.Copy(valueStr);
	else if (CSysPara::GetPropID("nc.LaserPara.m_wEnlargedSpace") == pItem->m_idProp)
	{
		g_sysPara.nc.m_xLaserPara.m_wEnlargedSpace = atoi(valueStr);
		g_sysPara.WriteSysParaToReg("laserPara.m_wEnlargedSpace");
	}
	else if (CSysPara::GetPropID("nc.LaserPara.m_bOutputBendLine") == pItem->m_idProp)
	{
		if (valueStr.Compare("是") == 0)
			g_sysPara.nc.m_xLaserPara.m_bOutputBendLine = TRUE;
		else
			g_sysPara.nc.m_xLaserPara.m_bOutputBendLine = FALSE;
		g_sysPara.WriteSysParaToReg("laserPara.m_bOutputBendLine");
	}
	else if (CSysPara::GetPropID("nc.LaserPara.m_bOutputBendType") == pItem->m_idProp)
	{
		if (valueStr.Compare("是") == 0)
			g_sysPara.nc.m_xLaserPara.m_bOutputBendType = TRUE;
		else
			g_sysPara.nc.m_xLaserPara.m_bOutputBendType = FALSE;
		g_sysPara.WriteSysParaToReg("laserPara.m_bOutputBendType");
	}
	else if(CSysPara::GetPropID("nc.m_fBaffleHigh")==pItem->m_idProp)
	{	
		g_sysPara.nc.m_fBaffleHigh=atof(valueStr);
		g_sysPara.WriteSysParaToReg("SideBaffleHigh");
	}
	else if(CSysPara::GetPropID("nc.m_sNcDriverPath")==pItem->m_idProp)
		g_sysPara.nc.m_sNcDriverPath=valueStr;
	else if(CSysPara::GetPropID("pbj.m_bIncVertex")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			g_sysPara.pbj.m_bIncVertex=TRUE;
		else
			g_sysPara.pbj.m_bIncVertex=FALSE;
		g_sysPara.WriteSysParaToReg("PbjIncVertex");
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
	else if (CSysPara::GetPropID("nc.m_ciDisplayType")==pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("火焰") == 0)
			g_sysPara.nc.m_ciDisplayType = CNCPart::FLAME_MODE;
		else if (valueStr.CompareNoCase("等离子") == 0)
			g_sysPara.nc.m_ciDisplayType = CNCPart::PLASMA_MODE;
		else if (valueStr.CompareNoCase("冲床") == 0)
			g_sysPara.nc.m_ciDisplayType = CNCPart::PUNCH_MODE;
		else if (valueStr.CompareNoCase("钻床") == 0)
			g_sysPara.nc.m_ciDisplayType = CNCPart::DRILL_MODE;
		else if (valueStr.CompareNoCase("激光") == 0)
			g_sysPara.nc.m_ciDisplayType = CNCPart::LASER_MODE;
		else if (valueStr.CompareNoCase("原始") == 0)
			g_sysPara.nc.m_ciDisplayType = 0;
		else if (valueStr.CompareNoCase("复合模式") == 0)
		{
			CSelDisplayNcDlg dlg;
			dlg.DoModal();
		}
		g_sysPara.WriteSysParaToReg("m_ciDisplayType");
		//
		oper.UpdatePropItemValue("nc.m_ciDisplayType");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_sOutLineLen")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_sOutLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_sOutLineLen");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("plasmaCut.m_sIntoLineLen")==pItem->m_idProp)
	{	
		g_sysPara.plasmaCut.m_sIntoLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_sIntoLineLen");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bInitPosFarOrg")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_bInitPosFarOrg=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bInitPosFarOrg");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bCutPosInInitPos")==pItem->m_idProp)
	{
		g_sysPara.plasmaCut.m_bCutPosInInitPos=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bCutPosInInitPos");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("flameCut.m_sOutLineLen")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_sOutLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_sOutLineLen");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("flameCut.m_sIntoLineLen")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_sIntoLineLen.Copy(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_sIntoLineLen");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("flameCut.m_bInitPosFarOrg")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_bInitPosFarOrg=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("flameCut.m_bInitPosFarOrg");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("flameCut.m_bCutPosInInitPos")==pItem->m_idProp)
	{
		g_sysPara.flameCut.m_bCutPosInInitPos=valueStr[0]-'0';
		g_sysPara.WriteSysParaToReg("flameCut.m_bCutPosInInitPos");
		//
		bUpdateCutInfo = TRUE;
	}
	else if (CSysPara::GetPropID("nc.FlamePara.m_wEnlargedSpace")==pItem->m_idProp)
	{
		g_sysPara.nc.m_xFlamePara.m_wEnlargedSpace = atoi(valueStr);
		g_sysPara.WriteSysParaToReg("flameCut.m_wEnlargedSpace");
	}
	else if (CSysPara::GetPropID("flameCut.m_bGrindingArc") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xFlamePara.m_bGrindingArc = TRUE;
		else
			g_sysPara.nc.m_xFlamePara.m_bGrindingArc = FALSE;
		g_sysPara.WriteSysParaToReg("flameCut.m_bGrindingArc");
	}
	else if (CSysPara::GetPropID("flameCut.m_bCutSpecialHole") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xFlamePara.m_bCutSpecialHole = TRUE;
		else
			g_sysPara.nc.m_xFlamePara.m_bCutSpecialHole = FALSE;
		g_sysPara.WriteSysParaToReg("flameCut.m_bCutSpecialHole");
	}
	else if (CSysPara::GetPropID("nc.PlasmaPara.m_wEnlargedSpace") == pItem->m_idProp)
	{
		g_sysPara.nc.m_xPlasmaPara.m_wEnlargedSpace = atoi(valueStr);
		g_sysPara.WriteSysParaToReg("plasmaCut.m_wEnlargedSpace");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bGrindingArc") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xPlasmaPara.m_bGrindingArc = TRUE;
		else
			g_sysPara.nc.m_xPlasmaPara.m_bGrindingArc = FALSE;
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bGrindingArc");
	}
	else if (CSysPara::GetPropID("plasmaCut.m_bCutSpecialHole") == pItem->m_idProp)
	{
		if (valueStr.CompareNoCase("是") == 0)
			g_sysPara.nc.m_xPlasmaPara.m_bCutSpecialHole = TRUE;
		else
			g_sysPara.nc.m_xPlasmaPara.m_bCutSpecialHole = FALSE;
		g_sysPara.WriteSysParaToReg("plasmaCut.m_bCutSpecialHole");
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
	else if(CSysPara::GetPropID("holeIncrement.m_fCutSH")==pItem->m_idProp)
	{	
		g_sysPara.holeIncrement.m_fCutSH=atof(valueStr);
		bUpdateHoleInc=TRUE;
	}
	else if (CSysPara::GetPropID("holeIncrement.m_fProSH") == pItem->m_idProp)
	{
		g_sysPara.holeIncrement.m_fProSH = atof(valueStr);
		bUpdateHoleInc = TRUE;
	}
	else if(pItem->m_idProp>=CSysPara::GetPropID("crMode.crLS12") &&
		pItem->m_idProp<=CSysPara::GetPropID("crMode.crText"))
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
		if (pItem->m_idProp == CSysPara::GetPropID("crMode.crEdge"))
		{
			g_sysPara.crMode.crEdge = curClr;
			g_sysPara.WriteSysParaToReg("EdgeColor");
		}
		if (pItem->m_idProp == CSysPara::GetPropID("crMode.crHuoQu"))
		{
			g_sysPara.crMode.crHuoQu = curClr;
			g_sysPara.WriteSysParaToReg("HuoQuColor");
		}
		if (pItem->m_idProp == CSysPara::GetPropID("crMode.crText"))
		{
			g_sysPara.crMode.crText = curClr;
			g_sysPara.WriteSysParaToReg("TextColor");
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
	//更新切入点信息
	if (bUpdateCutInfo)
	{
#ifdef __PNC_
		for (CProcessPart* pProcessPart = model.EnumPartFirst(); pProcessPart; pProcessPart = model.EnumPartNext())
		{
			if (!pProcessPart->IsPlate())
				continue;
			model.SyncRelaPlateInfo((CProcessPlate*)pProcessPart);
		}
#endif
	}
	//
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(bUpdateHoleInc)
		pView->AmendHoleIncrement();
	return TRUE;
}

BOOL SyssettingButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	if (pPropList == NULL || pItem == NULL)
		return FALSE;
	CPropertyListOper<CSysPara> oper(pPropList, &g_sysPara);
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
		if (dlg.DoModal() == IDOK)
		{
			UpdateHoleIncrementProperty(pPropList, pItem);
			//
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->AmendHoleIncrement();
		}
	}
	else if (CSysPara::GetPropID("nc.FlamePara.m_xHoleIncrement") == pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		dlg.m_ciCurNcMode = CNCPart::FLAME_MODE;
		if (dlg.DoModal() == IDOK)
		{
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->AmendHoleIncrement();
		}
	}
	else if (CSysPara::GetPropID("nc.PlasmaPara.m_xHoleIncrement") == pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		dlg.m_ciCurNcMode = CNCPart::PLASMA_MODE;
		if (dlg.DoModal() == IDOK)
		{
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->AmendHoleIncrement();
		}
	}
	else if (CSysPara::GetPropID("nc.PunchPara.m_xHoleIncrement") == pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		dlg.m_ciCurNcMode = CNCPart::PUNCH_MODE;
		if (dlg.DoModal() == IDOK)
		{
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->AmendHoleIncrement();
		}
	}
	else if (CSysPara::GetPropID("nc.DrillPara.m_xHoleIncrement") == pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		dlg.m_ciCurNcMode = CNCPart::DRILL_MODE;
		dlg.DoModal();
	}
	else if (CSysPara::GetPropID("nc.LaserPara.m_xHoleIncrement") == pItem->m_idProp)
	{
		CHoleIncrementSetDlg dlg;
		dlg.m_ciCurNcMode = CNCPart::LASER_MODE;
		if (dlg.DoModal() == IDOK)
		{
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->AmendHoleIncrement();
		}
	}
	else if (CSysPara::GetPropID("nc.LaserPara.Config") == pItem->m_idProp)
	{
		CPlankConfigDlg dlg;
		dlg.DoModal();
	}
	else if (CSysPara::GetPropID("FileFormat") == pItem->m_idProp)
	{
		CFileFormatSetDlg dlg;
		if(dlg.DoModal()==IDOK)
			pPropList->SetItemPropValue(pItem->m_idProp, model.file_format.GetFileFormatStr());
	}
	else if (CSysPara::GetPropID("OutputPath") == pItem->m_idProp)
	{
		CString sFolder;
		if (InvokeFolderPickerDlg(sFolder))
		{
			model.m_sOutputPath.Copy(sFolder);
			pPropList->SetItemPropValue(pItem->m_idProp, sFolder);
		}
	}
	else if (CSysPara::GetPropID("nc.m_iNcMode") == pItem->m_idProp)
	{
		CSelPlateNcModeDlg dlg;
		if (dlg.DoModal())
		{
			CString sValue;
			CPropTreeItem *pFlameCutItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.bFlameCut"), NULL);
			if (g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE))
			{
				if (pFlameCutItem->m_bHideChildren)
					pPropList->Expand(pFlameCutItem, pFlameCutItem->m_iIndex);
			}	
			else
				pPropList->Collapse(pFlameCutItem);
			oper.UpdatePropItemValue("nc.bFlameCut");
			//
			CPropTreeItem *pPlasmaCutItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.bPlasmaCut"), NULL);
			if (g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE))
			{
				if (pPlasmaCutItem->m_bHideChildren)
					pPropList->Expand(pPlasmaCutItem, pPlasmaCutItem->m_iIndex);
			}
			else
				pPropList->Collapse(pPlasmaCutItem);
			oper.UpdatePropItemValue("nc.bPlasmaCut");
			//
			CPropTreeItem *pPunchItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.bPunchPress"), NULL);
			if (g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE))
			{
				if (pPunchItem->m_bHideChildren)
					pPropList->Expand(pPunchItem, pPunchItem->m_iIndex);
			}
			else
				pPropList->Collapse(pPunchItem);
			oper.UpdatePropItemValue("nc.bPunchPress");
			//
			CPropTreeItem *pDrillItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.bDrillPress"), NULL);
			if (g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE))
			{
				if (pDrillItem->m_bHideChildren)
					pPropList->Expand(pDrillItem, pDrillItem->m_iIndex);
			}
			else
				pPropList->Collapse(pDrillItem);
			oper.UpdatePropItemValue("nc.bDrillPress");
			//
			CPropTreeItem *pLaserItem = pPropList->FindItemByPropId(CSysPara::GetPropID("nc.bLaser"), NULL);
			if (g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
			{
				if (pLaserItem->m_bHideChildren)
					pPropList->Expand(pLaserItem, pLaserItem->m_iIndex);
			}
			else
				pPropList->Collapse(pLaserItem);
			oper.UpdatePropItemValue("nc.bLaser");
			//
			oper.UpdatePropItemValue("nc.m_iNcMode");
		}
	}
	else if (CSysPara::GetPropID("FileterMkSet")==pItem->m_idProp)
	{
		CFilterMkDlg dlg;
		if (dlg.DoModal() == IDOK)
		{
			CPPEView *pView = (CPPEView*)theApp.GetView();
			pView->FilterPlateMK();
		}
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
#if defined(__PNC_)
	pPropDlg->m_arrPropGroupLabel.SetSize(3);
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_NCINFO-1,"常规");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_PROCESSCARD_INFO - 1, "输出");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_DISPLAY - 1, "其他");
#else
	pPropDlg->m_arrPropGroupLabel.SetSize(3);
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_NCINFO-1,"常规");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_PROCESSCARD_INFO-1,"工艺卡");
	pPropDlg->m_arrPropGroupLabel.SetAt(GROUP_DISPLAY - 1, "显示");
#endif
	pPropDlg->RefreshTabCtrl(CSysPara::m_iCurDisplayPropGroup);
	//
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->SetModifyValueFunc(ModifySyssettingProperty);
	pPropList->SetButtonClickFunc(SyssettingButtonClick);
	CPropTreeItem *pRootItem = pPropList->GetRootItem();
	CPropTreeItem *pParentItem = NULL, *pGroupItem = NULL, *pPropItem = NULL, *pLeftItem = NULL;
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
	pGroupItem =oper.InsertCmbListPropItem(pParentItem,"nc.m_bDispMkRect");
	if(g_sysPara.nc.m_bDispMkRect)
	{
		oper.InsertCmbListPropItem(pGroupItem, "nc.m_ciMkVect");
		oper.InsertEditPropItem(pGroupItem,"nc.m_fMKRectL");
		oper.InsertEditPropItem(pGroupItem,"nc.m_fMKRectW");
	}
	oper.InsertButtonPropItem(pParentItem, "FileterMkSet");
	pGroupItem = oper.InsertEditPropItem(pParentItem,"nc.m_fLimitSH");
	if (g_sysPara.nc.m_fLimitSH > 0)
	{
		pPropItem = oper.InsertEditPropItem(pGroupItem, "CutLimitSH");
		pPropItem->SetReadOnly();
		pPropItem = oper.InsertEditPropItem(pGroupItem, "ProLimitSH");
		pPropItem->SetReadOnly();
	}
#ifdef __PROCESS_PLATE_
	pGroupItem =oper.InsertBtnEditPropItem(pParentItem,"holeIncrement.m_fDatum");
	UpdateHoleIncrementProperty(pPropList, pGroupItem);
#endif
#endif
#ifdef __PNC_
	//文件设置
	pParentItem = oper.InsertPropItem(pRootItem, "FileSet");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_NCINFO);
	//PBJ文件设置
	pPropItem = oper.InsertEditPropItem(pParentItem, "PbjPara");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bIncVertex");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bAutoSplitFile");
	oper.InsertCmbListPropItem(pPropItem, "pbj.m_bMergeHole");
	//PMZ文件设置
	pPropItem = oper.InsertEditPropItem(pParentItem, "PmzPara");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_iPmzMode");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_bIncVertex");
	oper.InsertCmbListPropItem(pPropItem, "pmz.m_bPmzCheck");
#else
	oper.InsertFilePathPropItem(pParentItem,"nc.m_sNcDriverPath");
#endif
#ifdef __PNC_
	//钢板输出设置
	pParentItem = oper.InsertPropItem(pRootItem, "OutPutSet");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_PROCESSCARD_INFO);
	oper.InsertBtnEditPropItem(pParentItem, "OutputPath");
	oper.InsertButtonPropItem(pParentItem, "FileFormat");
	oper.InsertCmbListPropItem(pParentItem, "nc.m_iDxfMode");
	oper.InsertButtonPropItem(pParentItem, "nc.m_iNcMode");
	//火焰切割
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "nc.bFlameCut");
	pGroupItem->m_bHideChildren = !g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE);
	oper.InsertEditPropItem(pGroupItem, "nc.FlamePara.m_wEnlargedSpace");
	oper.InsertButtonPropItem(pGroupItem, "nc.FlamePara.m_xHoleIncrement");
	oper.InsertEditPropItem(pGroupItem, "nc.FlamePara.m_sThick");
	oper.InsertCmbListPropItem(pGroupItem, "nc.FlamePara.m_dwFileFlag");
	pPropItem = oper.InsertButtonPropItem(pGroupItem, "flameCut","切割路径设置");
	oper.InsertCmbListPropItem(pPropItem, "flameCut.m_bCutPosInInitPos");
	oper.InsertCmbListPropItem(pPropItem, "flameCut.m_bInitPosFarOrg");
	oper.InsertCmbEditPropItem(pPropItem, "flameCut.m_sIntoLineLen");
	oper.InsertCmbEditPropItem(pPropItem, "flameCut.m_sOutLineLen");
	oper.InsertCmbListPropItem(pPropItem, "flameCut.m_bCutSpecialHole");
	oper.InsertCmbListPropItem(pPropItem, "flameCut.m_bGrindingArc");
	//等离子切割
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "nc.bPlasmaCut");
	pGroupItem->m_bHideChildren = !g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE);
	oper.InsertEditPropItem(pGroupItem, "nc.PlasmaPara.m_wEnlargedSpace");
	oper.InsertButtonPropItem(pGroupItem, "nc.PlasmaPara.m_xHoleIncrement");
	oper.InsertEditPropItem(pGroupItem, "nc.PlasmaPara.m_sThick");
	oper.InsertCmbListPropItem(pGroupItem, "nc.PlasmaPara.m_dwFileFlag");
	pPropItem = oper.InsertButtonPropItem(pGroupItem, "plasmaCut","切割路径设置");
	oper.InsertCmbListPropItem(pPropItem, "plasmaCut.m_bCutPosInInitPos");
	oper.InsertCmbListPropItem(pPropItem, "plasmaCut.m_bInitPosFarOrg");
	oper.InsertCmbEditPropItem(pPropItem, "plasmaCut.m_sIntoLineLen");
	oper.InsertCmbEditPropItem(pPropItem, "plasmaCut.m_sOutLineLen");
	oper.InsertCmbListPropItem(pPropItem, "plasmaCut.m_bCutSpecialHole");
	oper.InsertCmbListPropItem(pPropItem, "plasmaCut.m_bGrindingArc");
	//冲床加工
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "nc.bPunchPress");
	pGroupItem->m_bHideChildren = !g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE);
	oper.InsertCmbListPropItem(pGroupItem, "nc.PunchPara.m_bReserveBigSH");
	oper.InsertCmbListPropItem(pGroupItem, "nc.PunchPara.m_bReduceSmallSH");
	oper.InsertButtonPropItem(pGroupItem, "nc.PunchPara.m_xHoleIncrement");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.PunchPara.m_ciHoldSortType");
	pLeftItem = oper.InsertCmbListPropItem(pPropItem, "nc.PunchPara.m_bSortHasBigSH");
	pLeftItem->SetReadOnly(!g_sysPara.nc.m_xPunchPara.m_bReserveBigSH);
	oper.InsertEditPropItem(pGroupItem, "nc.PunchPara.m_sThick");
	oper.InsertCmbListPropItem(pGroupItem, "nc.PunchPara.m_dwFileFlag");
	//钻床加工
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "nc.bDrillPress");
	pGroupItem->m_bHideChildren = !g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE);
	oper.InsertCmbListPropItem(pGroupItem, "nc.DrillPara.m_bReserveBigSH");
	oper.InsertCmbListPropItem(pGroupItem, "nc.DrillPara.m_bReduceSmallSH");
	oper.InsertButtonPropItem(pGroupItem, "nc.DrillPara.m_xHoleIncrement");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.DrillPara.m_ciHoldSortType");
	pLeftItem = oper.InsertCmbListPropItem(pPropItem, "nc.DrillPara.m_bSortHasBigSH");
	pLeftItem->SetReadOnly(!g_sysPara.nc.m_xDrillPara.m_bReserveBigSH);
	oper.InsertEditPropItem(pGroupItem, "nc.DrillPara.m_sThick");
	oper.InsertCmbListPropItem(pGroupItem, "nc.DrillPara.m_dwFileFlag");
	//激光复合机
	pGroupItem = oper.InsertCmbListPropItem(pParentItem, "nc.bLaser");
	pGroupItem->m_bHideChildren = !g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE);
	oper.InsertEditPropItem(pGroupItem, "nc.LaserPara.m_wEnlargedSpace");
	oper.InsertButtonPropItem(pGroupItem, "nc.LaserPara.m_xHoleIncrement");
	oper.InsertButtonPropItem(pGroupItem, "nc.LaserPara.Config");
	oper.InsertEditPropItem(pGroupItem, "nc.LaserPara.m_sThick");
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "nc.LaserPara.m_dwFileFlag");
	pPropItem->SetReadOnly(TRUE);
	oper.InsertCmbListPropItem(pGroupItem, "nc.LaserPara.m_bOutputBendLine");
	oper.InsertCmbListPropItem(pGroupItem, "nc.LaserPara.m_bOutputBendType");
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
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crEdge");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crHuoQu");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS12");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS16");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS20");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crLS24");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crOtherLS");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crMarK");
	oper.InsertCmbColorPropItem(pParentItem, "crMode.crText");
	//字体设置
	pParentItem = oper.InsertPropItem(pRootItem, "Font");
	pParentItem->m_dwPropGroup = GetSingleWord(GROUP_DISPLAY);
	oper.InsertEditPropItem(pParentItem, "font.fDxfTextSize");
	oper.InsertEditPropItem(pParentItem, "font.fTextHeight");
#ifndef __PNC_
	oper.InsertEditPropItem(pParentItem, "font.fDimTextSize");
	oper.InsertEditPropItem(pParentItem, "font.fPartNoTextSize");
#endif
	pPropList->Redraw();
	return TRUE;
}