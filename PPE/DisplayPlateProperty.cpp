#include "StdAfx.h"
#include "PPE.h"
#include "PPEView.h"
#include "MainFrm.h"
#include "PartPropertyDlg.h"
#include "PartLib.h"
#include "reportvectordlg.h"
#include "PropertyListOper.h"
#include "CfgPartNoDlg.h"
#include "PPEModel.h"
#include "SyncPropertyDlg.h"

void UpdateHuoquFaceItems(CPropertyList *pPropList,CPropTreeItem *pParentItem)
{
	CProcessPlate *pPlate=(CProcessPlate*)pPropList->m_pObj;
	if(pPlate==NULL)
		return;
	CPropertyListOper<CProcessPlate> oper(pPropList,pPlate);
	CPropTreeItem *pFindItem=pPropList->FindItemByPropId(CProcessPlate::GetPropID("HuoquFace[0]"),NULL);
	if(pFindItem)
	{
		pPropList->DeleteAllSonItems(pFindItem);
		pPropList->DeleteItemByPropId(pFindItem->m_idProp);
	}
	pFindItem=pPropList->FindItemByPropId(CProcessPlate::GetPropID("HuoquFace[1]"),NULL);
	if(pFindItem)
	{
		pPropList->DeleteAllSonItems(pFindItem);
		pPropList->DeleteItemByPropId(pFindItem->m_idProp);
	}
	//
	CPropTreeItem *pFaceItem=NULL,*pPropItem=NULL;
	if(pPlate->m_cFaceN>=2)
	{
		pFaceItem=oper.InsertEditPropItem(pParentItem,"HuoquFace[0]");
		pFaceItem->SetReadOnly();
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuFaceNorm[0]");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[0].x");
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[0].y");
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[0].z");
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuLine[0].startPt");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].startPt.x");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].startPt.y");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].startPt.z");
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuLine[0].endPt");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].endPt.x");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].endPt.y");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[0].endPt.z");
	}
	if(pPlate->m_cFaceN==3)
	{
		pFaceItem=oper.InsertEditPropItem(pParentItem,"HuoquFace[1]");
		pFaceItem->SetReadOnly();
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuFaceNorm[1]");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[1].x");
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[1].y");
		oper.InsertEditPropItem(pPropItem,"HuoQuFaceNorm[1].z");
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuLine[1].startPt");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].startPt.x");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].startPt.y");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].startPt.z");
		pPropItem=oper.InsertButtonPropItem(pFaceItem,"HuoQuLine[1].endPt");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].endPt.x");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].endPt.y");
		oper.InsertEditPropItem(pPropItem,"HuoQuLine[1].endPt.z");
	}
}

BOOL ModifyPlateProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CProcessPlate *pPlate=(CProcessPlate*)pPropList->m_pObj;
	if(pPlate==NULL)
		return FALSE;
	BOOL bUpdatePartItemText=FALSE;
	if(CProcessPlate::GetPropID("m_Seg")==pItem->m_idProp)
		pPlate->SetSegI(SEGI(valueStr.GetBuffer()));
	else if(CProcessPlate::GetPropID("m_sPartNo")==pItem->m_idProp)
	{
		CProcessPart *pPart=model.FromPartNo(valueStr);
		if(pPart==NULL)
		{
			model.ModifyPartNo(pPlate->GetPartNo(),valueStr);
			bUpdatePartItemText=TRUE;
		}
		else if(pPart!=pPlate)
		{
			AfxMessageBox(CXhChar50("已存在件号为%s的构件,请重新设置件号!",valueStr));
			return FALSE;
		}
	}
	else if(CProcessPlate::GetPropID("m_cMaterial")==pItem->m_idProp)
	{
		pPlate->cMaterial = QuerySteelBriefMatMark(valueStr.GetBuffer());
		bUpdatePartItemText=TRUE;
	}
	else if(CProcessPlate::GetPropID("m_fThick")==pItem->m_idProp)
	{
		pPlate->m_fThick=(float)atof(valueStr);
		bUpdatePartItemText=TRUE;
	}
	else if(CProcessPlate::GetPropID("m_fWeight")==pItem->m_idProp)
		pPlate->m_fWeight=(float)atof(valueStr);
	else if(CProcessPlate::GetPropID("m_sNote")==pItem->m_idProp)
		pPlate->SetNotes(valueStr.GetBuffer());
	else if(CProcessPlate::GetPropID("m_cFaceN")==pItem->m_idProp)
	{
		pPlate->m_cFaceN=valueStr[0]-'0';
		UpdateHuoquFaceItems(pPropList,pItem->m_pParent);
	}
	else if(CProcessPlate::GetPropID("m_ciDeformType")==pItem->m_idProp)
		pPlate->m_ciDeformType=valueStr[0]-'0';
	else if(CProcessPlate::GetPropID("m_ciRollProcessType")==pItem->m_idProp)
		pPlate->m_ciRollProcessType=valueStr[0]-'0';
	else if(CProcessPlate::GetPropID("m_ciRollOffsetType")==pItem->m_idProp)
		pPlate->m_ciRollOffsetType=valueStr[0]-'0';
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[0].x")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[0].x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[0].y")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[0].y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[0].z")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[0].z=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].startPt.x")==pItem->m_idProp)
		pPlate->HuoQuLine[0].startPt.x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].startPt.y")==pItem->m_idProp)
		pPlate->HuoQuLine[0].startPt.y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].startPt.z")==pItem->m_idProp)
		pPlate->HuoQuLine[0].startPt.z=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].endPt.x")==pItem->m_idProp)
		pPlate->HuoQuLine[0].endPt.x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].endPt.y")==pItem->m_idProp)
		pPlate->HuoQuLine[0].endPt.y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[0].endPt.z")==pItem->m_idProp)
		pPlate->HuoQuLine[0].endPt.z=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[1].x")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[1].x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[1].y")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[1].y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[1].z")==pItem->m_idProp)
		pPlate->HuoQuFaceNorm[1].z=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].startPt.x")==pItem->m_idProp)
		pPlate->HuoQuLine[1].startPt.x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].startPt.y")==pItem->m_idProp)
		pPlate->HuoQuLine[1].startPt.y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].startPt.z")==pItem->m_idProp)
		pPlate->HuoQuLine[1].startPt.z=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].endPt.x")==pItem->m_idProp)
		pPlate->HuoQuLine[1].endPt.x=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].endPt.y")==pItem->m_idProp)
		pPlate->HuoQuLine[1].endPt.y=atof(valueStr);
	else if(CProcessPlate::GetPropID("HuoQuLine[1].endPt.z")==pItem->m_idProp)
		pPlate->HuoQuLine[1].endPt.z=atof(valueStr);
	else 
		return FALSE;
	if(bUpdatePartItemText)
	{
		CPartTreeDlg *pTreeDlg=((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
		if(pTreeDlg)
			pTreeDlg->UpdateTreeItemText();
	}
	//更新数据至编辑器
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SyncPartInfo(true);
	return TRUE;
}

BOOL PlateButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	CProcessPlate *pPlate=(CProcessPlate*)pPropList->m_pObj;
	if(pPlate==NULL)
		return FALSE;
	CPropertyListOper<CProcessPlate> oper(pPropList,pPlate);

	CReportVectorDlg vecDlg;
	if(CProcessPlate::GetPropID("HuoQuFaceNorm[0]")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲法线";
		vecDlg.m_fVectorX = pPlate->HuoQuFaceNorm[0].x;
		vecDlg.m_fVectorY = pPlate->HuoQuFaceNorm[0].y;
		vecDlg.m_fVectorZ = pPlate->HuoQuFaceNorm[0].z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuFaceNorm[0].Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuFaceNorm[0].x");
			oper.SetItemPropValue("HuoQuFaceNorm[0].y");
			oper.SetItemPropValue("HuoQuFaceNorm[0].z");
		}
	}
	else if(CProcessPlate::GetPropID("HuoQuLine[0].startPt")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲线始端坐标";
		vecDlg.m_fVectorX = pPlate->HuoQuLine[0].startPt.x;
		vecDlg.m_fVectorY = pPlate->HuoQuLine[0].startPt.y;
		vecDlg.m_fVectorZ = pPlate->HuoQuLine[0].startPt.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuLine[0].startPt.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuLine[0].startPt.x");
			oper.SetItemPropValue("HuoQuLine[0].startPt.y");
			oper.SetItemPropValue("HuoQuLine[0].startPt.z");
		}
	}
	else if(CProcessPlate::GetPropID("HuoQuLine[0].endPt")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲线始端坐标";
		vecDlg.m_fVectorX = pPlate->HuoQuLine[0].endPt.x;
		vecDlg.m_fVectorY = pPlate->HuoQuLine[0].endPt.y;
		vecDlg.m_fVectorZ = pPlate->HuoQuLine[0].endPt.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuLine[0].endPt.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuLine[0].endPt.x");
			oper.SetItemPropValue("HuoQuLine[0].endPt.y");
			oper.SetItemPropValue("HuoQuLine[0].endPt.z");
		}
	}
	else if(CProcessPlate::GetPropID("HuoQuFaceNorm[1]")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲法线";
		vecDlg.m_fVectorX = pPlate->HuoQuFaceNorm[1].x;
		vecDlg.m_fVectorY = pPlate->HuoQuFaceNorm[1].y;
		vecDlg.m_fVectorZ = pPlate->HuoQuFaceNorm[1].z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuFaceNorm[1].Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuFaceNorm[1].x");
			oper.SetItemPropValue("HuoQuFaceNorm[1].y");
			oper.SetItemPropValue("HuoQuFaceNorm[1].z");
		}
	}
	else if(CProcessPlate::GetPropID("HuoQuLine[1].startPt")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲线始端坐标";
		vecDlg.m_fVectorX = pPlate->HuoQuLine[1].startPt.x;
		vecDlg.m_fVectorY = pPlate->HuoQuLine[1].startPt.y;
		vecDlg.m_fVectorZ = pPlate->HuoQuLine[1].startPt.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuLine[1].startPt.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuLine[1].startPt.x");
			oper.SetItemPropValue("HuoQuLine[1].startPt.y");
			oper.SetItemPropValue("HuoQuLine[1].startPt.z");
		}
	}
	else if(CProcessPlate::GetPropID("HuoQuLine[1].endPt")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "火曲线始端坐标";
		vecDlg.m_fVectorX = pPlate->HuoQuLine[1].endPt.x;
		vecDlg.m_fVectorY = pPlate->HuoQuLine[1].endPt.y;
		vecDlg.m_fVectorZ = pPlate->HuoQuLine[1].endPt.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pPlate->HuoQuLine[1].endPt.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("HuoQuLine[1].endPt.x");
			oper.SetItemPropValue("HuoQuLine[1].endPt.y");
			oper.SetItemPropValue("HuoQuLine[1].endPt.z");
		}
	}
	else if(pItem->m_idProp==CProcessPlate::GetPropID("m_dwInheritPropFlag"))
	{
		CSyncPropertyDlg dlg;
		dlg.m_pProcessPart=pPlate;
		dlg.DoModal();
	}
	else 
		return FALSE;
	//TODO:此处需要后续添加提交保存文件的代码 wjh-2014.8.27
	//g_partInfoHodler.UpdatePartInfoToEditor();
	return TRUE;
}

CString MakeMaterialMarkSetString();
BOOL CPPEView::DisplayPlateProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL || m_pProcessPart==NULL)
		return FALSE;
	CProcessPlate* pPlate=(CProcessPlate*)GetCurProcessPart();
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<CProcessPlate> oper(pPropList,pPlate);

	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	const int GROUP_BASICINFO=1,GROUP_NCINFO=2,GROUP_PROCESSCARD_INFO=3;
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pPlate;
	pPropList->SetModifyValueFunc(ModifyPlateProperty);
	pPropList->SetButtonClickFunc(PlateButtonClick);

	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL;
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo");
	oper.InsertEditPropItem(pParentItem,"m_sPartNo");
	oper.InsertEditPropItem(pParentItem,"m_Seg");
	oper.InsertCmbListPropItem(pParentItem,"m_cMaterial",MakeMaterialMarkSetString());
	oper.InsertEditPropItem(pParentItem,"m_fThick");
	oper.InsertEditPropItem(pParentItem,"m_sNote");
	pPropItem=oper.InsertEditPropItem(pParentItem,"m_sRelPartNo");
	pPropItem->SetReadOnly(TRUE);
	if(theApp.starter.m_bChildProcess&&theApp.starter.IsIncPartPattern())
		oper.InsertButtonPropItem(pParentItem,"m_dwInheritPropFlag");

	pParentItem=oper.InsertPropItem(pRootItem,"profile");
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"m_bDeformed");
	pPropItem->SetReadOnly();
	if(pPlate->m_bIncDeformed==FALSE)
	{
		oper.InsertCmbListPropItem(pParentItem,"m_ciDeformType");
		oper.InsertCmbListPropItem(pParentItem,"m_ciRollProcessType");
		oper.InsertCmbListPropItem(pParentItem,"m_ciRollOffsetType");
	}
	oper.InsertCmbListPropItem(pParentItem,"m_cFaceN");
	UpdateHuoquFaceItems(pPropList,pParentItem);

	if(pPlate->m_xBoltInfoList.GetNodeNum()>0)
	{
		pParentItem=oper.InsertPropItem(pRootItem,"bolt_info");
		if(pPlate->GetBoltNumByD(24)>0)
		{
			pPropItem=oper.InsertEditPropItem(pParentItem,"M24");
			pPropItem->SetReadOnly();
		}
		if(pPlate->GetBoltNumByD(22)>0)
		{
			pPropItem=oper.InsertEditPropItem(pParentItem,"M22");
			pPropItem->SetReadOnly();
			
		}
		if(pPlate->GetBoltNumByD(20)>0)
		{
			pPropItem=oper.InsertEditPropItem(pParentItem,"M20");
			pPropItem->SetReadOnly();

		}
		if(pPlate->GetBoltNumByD(18)>0)
		{
			pPropItem=oper.InsertEditPropItem(pParentItem,"M18");
			pPropItem->SetReadOnly();

		}
		if(pPlate->GetBoltNumByD(16)>0)
		{
			pPropItem=oper.InsertEditPropItem(pParentItem,"M16");
			pPropItem->SetReadOnly();

		}
	}
	pPropList->Redraw();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// BOLT_INFO
BOOL ModifyBoltInfoProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CPPEView *pView=theApp.GetView();
	if(pView==NULL)
		return FALSE;
	bool bModify=false;
	CXhPtrSet<BOLT_INFO> selectBolts;
	pView->m_xSelectEntity.GetSelectBolts(selectBolts,pView->GetCurProcessPart());
	BOLT_INFO *pBolt=NULL,*pFirstBolt=(BOLT_INFO*)pPropList->m_pObj;
	if(selectBolts.GetNodeNum()<=0 ||pFirstBolt==NULL)
		return FALSE;

	CPropertyListOper<BOLT_INFO> oper(pPropList,pFirstBolt);
	if(BOLT_INFO::GetPropID("bolt_d")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->bolt_d=atoi(valueStr);
		bModify=true;
	}
	else if(BOLT_INFO::GetPropID("hole_d_increment")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->hole_d_increment=(float)atof(valueStr);
		bModify=true;
	}
	else if(BOLT_INFO::GetPropID("cFuncType")==pItem->m_idProp)
	{
		BYTE biType=valueStr[0]-'0';
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->cFuncType=biType;
		bModify=true;
	}
	else if(BOLT_INFO::GetPropID("waistLen")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->waistLen=atoi(valueStr);
		pPropList->DeleteAllSonItems(pItem);
		if(pFirstBolt->waistLen>0)
		{
			CPropTreeItem *pPropItem=oper.InsertButtonPropItem(pItem,"waistVec");
			pPropItem->m_bHideChildren=FALSE;
			oper.InsertEditPropItem(pPropItem,"waistVec.x");
			oper.InsertEditPropItem(pPropItem,"waistVec.y");
			pPropList->Expand(pItem,pItem->m_iIndex);
		}
	}
	else if(BOLT_INFO::GetPropID("waistVec.x")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
		{
			pBolt->waistVec.x=atof(valueStr);
			normalize(pBolt->waistVec);//.normalized();
		}
		oper.SetItemPropValue("waistVec");
		oper.SetItemPropValue("waistVec.x");
		oper.SetItemPropValue("waistVec.y");
	}
	else if(BOLT_INFO::GetPropID("waistVec.y")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
		{
			pBolt->waistVec.y=atof(valueStr);
			normalize(pBolt->waistVec);//.normalized();
		}
		oper.SetItemPropValue("waistVec");
		oper.SetItemPropValue("waistVec.x");
		oper.SetItemPropValue("waistVec.y");
	}
	else if(BOLT_INFO::GetPropID("posX")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->posX=(float)atof(valueStr);
		oper.SetItemPropValue("pos");
		bModify=true;
	}
	else if(BOLT_INFO::GetPropID("posY")==pItem->m_idProp)
	{
		for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
			pBolt->posY=(float)atof(valueStr);
		oper.SetItemPropValue("pos");
		bModify=true;
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	CProcessPart* pPart=pView->GetCurProcessPart();
	for(pBolt=selectBolts.GetFirst();pBolt;pBolt=selectBolts.GetNext())
	{
		BOLT_INFO* pBoltInfo=pPart->FromHoleHiberId(pBolt->hiberId);
		pBoltInfo->CloneBolt(pBolt);
	}
	if(bModify)
	{	//更新螺栓信息后，重新进行绘制
		pView->SyncPartInfo(true,false);
		g_pPartEditor->Draw();
		g_p2dDraw->RenderDrawing();
		pView->m_xSelectEntity.UpdateSelectEnt(pFirstBolt->hiberId);
	}
	return TRUE;
}
BOOL BoltInfoButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	BOLT_INFO *pBolt=(BOLT_INFO*)pPropList->m_pObj;
	if(pBolt==NULL)
		return FALSE;
	CReportVectorDlg vecDlg;
	CPropertyListOper<BOLT_INFO> oper(pPropList,pBolt);
	if(BOLT_INFO::GetPropID("dwRayNo")==pItem->m_idProp)
	{
		CCfgPartNoDlg dlg;
		dlg.cfg_style=CFG_LSRAY_NO;
		dlg.cfgword.flag.word[0]=pBolt->dwRayNo;
		if(dlg.DoModal()==IDOK)
		{
			pBolt->dwRayNo=dlg.cfgword.flag.word[0];
			oper.SetItemPropValue(pItem->m_idProp);
		}
	}
	else if(BOLT_INFO::GetPropID("waistVec")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "腰圆方向";
		vecDlg.m_fVectorX = pBolt->waistVec.x;
		vecDlg.m_fVectorY = pBolt->waistVec.y;
		vecDlg.m_fVectorZ = pBolt->waistVec.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pBolt->waistVec.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,vecDlg.m_fVectorZ);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("waistVec.x");
			oper.SetItemPropValue("waistVec.y");
		}
	}
	else if(BOLT_INFO::GetPropID("pos")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "孔位置";
		vecDlg.m_fVectorX = pBolt->posX;
		vecDlg.m_fVectorY = pBolt->posY;
		vecDlg.m_fVectorZ = 0;
		if(vecDlg.DoModal()==IDOK)
		{
			pBolt->posX=(float)vecDlg.m_fVectorX;
			pBolt->posY=(float)vecDlg.m_fVectorY;
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("posX");
			oper.SetItemPropValue("posY");
		}
	}
	else 
		return FALSE;
	//TODO:此处需要后续添加提交保存文件的代码 wjh-2014.8.27
	//g_partInfoHodler.UpdatePartInfoToEditor();
	return TRUE;
}
BOOL CPPEView::DisplayBoltHoleProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL)
		return FALSE;
	CXhPtrSet<BOLT_INFO> selectBolts;
	m_xSelectEntity.GetSelectBolts(selectBolts,m_pProcessPart);
	if(selectBolts.GetNodeNum()<=0)
		return FALSE;
	BOLT_INFO *pFirstBolt=selectBolts.GetFirst();
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<BOLT_INFO> oper(pPropList,pFirstBolt,&selectBolts);

	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pFirstBolt;
	pPropList->SetModifyValueFunc(ModifyBoltInfoProperty);
	pPropList->SetButtonClickFunc(BoltInfoButtonClick);

	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL;
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo");
	pPropItem=oper.InsertEditPropItem(pParentItem,"id");
	pPropItem->SetReadOnly();
	oper.InsertCmbEditPropItem(pParentItem,"bolt_d");
	oper.InsertEditPropItem(pParentItem,"hole_d_increment");
	oper.InsertCmbListPropItem(pParentItem,"cFuncType");
	pPropItem=oper.InsertEditPropItem(pParentItem,"waistLen");
	pPropItem->m_bHideChildren=FALSE;
	if(pFirstBolt->waistLen>0)
	{
		pPropItem=oper.InsertButtonPropItem(pPropItem,"waistVec");
		pPropItem->m_bHideChildren=TRUE;
		oper.InsertEditPropItem(pPropItem,"waistVec.x");
		oper.InsertEditPropItem(pPropItem,"waistVec.y");
	}
	pPropItem=oper.InsertButtonPropItem(pParentItem,"pos");
	pPropItem->m_bHideChildren=FALSE;
	oper.InsertEditPropItem(pPropItem,"posX");
	oper.InsertEditPropItem(pPropItem,"posY");
	//螺栓射线不进行显示(调用CCfgPartNoDlg出现死机异常)
	//oper.InsertButtonPropItem(pParentItem,"dwRayNo");
	pPropList->Redraw();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
BOOL ModifyEdgeProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CLogErrorLife logErrLife;
	CPPEView *pView=theApp.GetView();
	CXhPtrSet<PROFILE_VER> selectVertex;
	pView->m_xSelectEntity.GetSelectEdges(selectVertex,pView->GetCurProcessPart());
	PROFILE_VER *pFirstVertex=(PROFILE_VER*)pPropList->m_pObj,*pVertex=NULL;
	if(pFirstVertex==NULL || selectVertex.GetNodeNum()<=0)
		return FALSE;

	CPropertyListOper<PROFILE_VER> oper(pPropList,pFirstVertex);
	if(PROFILE_VER::GetPropID("type")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->type=atoi(valueStr);
		//
		pPropList->DeleteAllSonItems(pItem);
		CPropTreeItem *pPropItem=NULL;
		if(pFirstVertex->type>1)
		{
			oper.InsertEditPropItem(pItem,"sector_angle");
			pPropItem=oper.InsertButtonPropItem(pItem,"work_norm");
			oper.InsertEditPropItem(pPropItem,"work_norm.x");
			oper.InsertEditPropItem(pPropItem,"work_norm.y");
			oper.InsertEditPropItem(pPropItem,"work_norm.z");
		}
		if(pFirstVertex->type==3)
		{
			oper.InsertEditPropItem(pItem,"radius");
			pPropItem=oper.InsertButtonPropItem(pItem,"center");
			oper.InsertEditPropItem(pPropItem,"center.x");
			oper.InsertEditPropItem(pPropItem,"center.y");
			pPropItem=oper.InsertButtonPropItem(pItem,"column_norm");
			oper.InsertEditPropItem(pPropItem,"column_norm.x");
			oper.InsertEditPropItem(pPropItem,"column_norm.y");
			oper.InsertEditPropItem(pPropItem,"column_norm.z");
		}
	}
	else if(PROFILE_VER::GetPropID("sector_angle")==pItem->m_idProp)
	{
		double angle=atof(valueStr)*RADTODEG_COEF;
		if(angle<0)
		{
			logerr.Log("扇形角度不能为负！");
			return FALSE;
		}
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->sector_angle=angle;
	}
	else if(PROFILE_VER::GetPropID("work_norm.x")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->work_norm.x=atof(valueStr);
			pVertex->work_norm.normalized();
		}
		oper.SetItemPropValue("work_norm");
		oper.SetItemPropValue("work_norm.x");
		oper.SetItemPropValue("work_norm.y");
		oper.SetItemPropValue("work_norm.z");
	}
	else if(PROFILE_VER::GetPropID("work_norm.y")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->work_norm.y=atof(valueStr);
			pVertex->work_norm.normalized();
		}
		oper.SetItemPropValue("work_norm");
		oper.SetItemPropValue("work_norm.x");
		oper.SetItemPropValue("work_norm.y");
		oper.SetItemPropValue("work_norm.z");
	}
	else if(PROFILE_VER::GetPropID("work_norm.z")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->work_norm.z=atof(valueStr);
			pVertex->work_norm.normalized();
		}
		oper.SetItemPropValue("work_norm");
		oper.SetItemPropValue("work_norm.x");
		oper.SetItemPropValue("work_norm.y");
		oper.SetItemPropValue("work_norm.z");
	}
	else if(PROFILE_VER::GetPropID("radius")==pItem->m_idProp)
	{
		if(atof(valueStr)<0)
		{
			logerr.Log("半径不能为负！");
			return FALSE;
		}
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->radius=atof(valueStr);
	}
	else if(PROFILE_VER::GetPropID("center.x")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->center.x=atof(valueStr);
		oper.SetItemPropValue("center");
	}
	else if(PROFILE_VER::GetPropID("center.y")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->center.y=atof(valueStr);
		oper.SetItemPropValue("center");
	}
	else if(PROFILE_VER::GetPropID("column_norm.x")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->column_norm.x=atof(valueStr);
			pVertex->column_norm.normalized();
		}
		oper.SetItemPropValue("column_norm");
		oper.SetItemPropValue("column_norm.x");
		oper.SetItemPropValue("column_norm.y");
		oper.SetItemPropValue("column_norm.z");
	}
	else if(PROFILE_VER::GetPropID("column_norm.y")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->column_norm.y=atof(valueStr);
			pVertex->column_norm.normalized();
		}
		oper.SetItemPropValue("column_norm");
		oper.SetItemPropValue("column_norm.x");
		oper.SetItemPropValue("column_norm.y");
		oper.SetItemPropValue("column_norm.z");
	}
	else if(PROFILE_VER::GetPropID("column_norm.z")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->column_norm.z=atof(valueStr);
			pVertex->column_norm.normalized();
		}
		oper.SetItemPropValue("column_norm");
		oper.SetItemPropValue("column_norm.x");
		oper.SetItemPropValue("column_norm.y");
		oper.SetItemPropValue("column_norm.z");
	}
	else if(PROFILE_VER::GetPropID("edgeType")==pItem->m_idProp)
	{
		int type=valueStr[0]-'0';
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
		{
			pVertex->m_bWeldEdge=(type==1);
			pVertex->m_bRollEdge=(type==2);
		}
		pPropList->DeleteAllSonItems(pItem);
		if(type==2)
		{
			oper.InsertEditPropItem(pItem,"roll_edge_offset_dist");
			oper.InsertEditPropItem(pItem,"manu_space","卷边高度");
		}
		else if(type==1)
			oper.InsertEditPropItem(pItem,"manu_space","加工间隙");
	}
	else if(PROFILE_VER::GetPropID("roll_edge_offset_dist")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->roll_edge_offset_dist=(short)atoi(valueStr);
	}
	else if(PROFILE_VER::GetPropID("manu_space")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())		
			pVertex->manu_space=(short)atoi(valueStr);
	}
	else if(PROFILE_VER::GetPropID("local_point_vec")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->local_point_vec=valueStr[0]-'0';
		pPropList->DeleteAllSonItems(pItem);
		if(pFirstVertex->local_point_vec>0)
			oper.InsertEditPropItem(pItem,"local_point_y");
	}
	else if(PROFILE_VER::GetPropID("local_point_y")==pItem->m_idProp)
	{
		for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
			pVertex->local_point_y=atof(valueStr);
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	CProcessPart* pPart=pView->GetCurProcessPart();
	for(pVertex=selectVertex.GetFirst();pVertex;pVertex=selectVertex.GetNext())
	{
		PROFILE_VER* pVertextInfo=pPart->FromVertexHiberId(pVertex->hiberId);
		pVertextInfo->CloneVertex(pVertex);
	}
	pView->SyncPartInfo(true);
	pView->UpdateSelectEnt();
	return TRUE;
}
BOOL EdgeButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	PROFILE_VER *pVertex=(PROFILE_VER*)pPropList->m_pObj;
	if(pVertex==NULL)
		return FALSE;
	CReportVectorDlg vecDlg;
	CPropertyListOper<PROFILE_VER> oper(pPropList,pVertex);

	if(PROFILE_VER::GetPropID("work_norm")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "法线";
		vecDlg.m_fVectorX = pVertex->work_norm.x;
		vecDlg.m_fVectorY = pVertex->work_norm.y;
		vecDlg.m_fVectorZ = pVertex->work_norm.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pVertex->work_norm.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,0);
			pVertex->work_norm.normalized();
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("work_norm.x");
			oper.SetItemPropValue("work_norm.y");
			oper.SetItemPropValue("work_norm.z");
		}
	}
	else if(PROFILE_VER::GetPropID("column_norm")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "圆柱轴向";
		vecDlg.m_fVectorX = pVertex->column_norm.x;
		vecDlg.m_fVectorY = pVertex->column_norm.y;
		vecDlg.m_fVectorZ = pVertex->column_norm.z;
		if(vecDlg.DoModal()==IDOK)
		{
			pVertex->column_norm.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,0);
			pVertex->column_norm.normalized();
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("column_norm.x");
			oper.SetItemPropValue("column_norm.y");
			oper.SetItemPropValue("column_norm.z");
		}
	}
	else if(PROFILE_VER::GetPropID("center")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "圆心位置";
		vecDlg.m_fVectorX = pVertex->center.x;
		vecDlg.m_fVectorY = pVertex->center.y;
		vecDlg.m_fVectorZ = 0;
		if(vecDlg.DoModal()==IDOK)
		{
			pVertex->vertex.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,0);
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("center.x");
			oper.SetItemPropValue("center.y");
		}
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	CPPEView *pView=theApp.GetView();
	CProcessPart* pPart=pView->GetCurProcessPart();
	PROFILE_VER* pVertextInfo=pPart->FromVertexHiberId(pVertex->hiberId);
	pVertextInfo->CloneVertex(pVertex);
	pView->SyncPartInfo(true);
	return TRUE;
}
static f3dPoint GetEdgeEndPt(CProcessPlate* pPlate,HIBERID hiberId)
{
	PROFILE_VER* pEndVertex=NULL;
	for(PROFILE_VER* pVer=pPlate->vertex_list.GetFirst();pVer;pVer=pPlate->vertex_list.GetNext())
	{
		if(!hiberId.IsEqual(pVer->hiberId))
			continue;
		pEndVertex=pPlate->vertex_list.GetNext();
		break;
	}
	if(pEndVertex==NULL)
		pEndVertex=pPlate->vertex_list.GetFirst();
	return pEndVertex->vertex;
}
BOOL CPPEView::DisplayEdgeLineProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL)
		return FALSE;
	CXhPtrSet<PROFILE_VER> selectVertex;
	m_xSelectEntity.GetSelectEdges(selectVertex,m_pProcessPart);
	if(selectVertex.GetNodeNum()<=0)
		return FALSE;
	//设置属性栏参数
	PROFILE_VER* pVertex=selectVertex.GetFirst();
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<PROFILE_VER> oper(pPropList,pVertex,&selectVertex);
	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pVertex;
	pPropList->SetModifyValueFunc(ModifyEdgeProperty);
	pPropList->SetButtonClickFunc(EdgeButtonClick);
	//添加属性
	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL,*pSonItem=NULL;
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo");
	pPropItem=oper.InsertEditPropItem(pParentItem,"id");	//唯一标识号
	pPropItem->SetReadOnly();
	pPropItem=oper.InsertEditPropItem(pParentItem,"vertex");//始端坐标
	pPropItem->SetReadOnly();
	CXhChar50 sText;
	f3dPoint endPt=GetEdgeEndPt((CProcessPlate*)m_pProcessPart,pVertex->hiberId);
	pPropItem=oper.InsertEditPropItem(pParentItem,"endVertex");	//终端坐标
	sText.ConvertFromPoint(endPt,2);
	pPropItem->m_lpNodeInfo->m_strPropValue=sText;
	pPropItem->SetReadOnly();
	//
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"type");			//线的类型
	if(pVertex->type>1)
	{
		oper.InsertEditPropItem(pPropItem,"sector_angle");
		CPropTreeItem* pItem=pPropItem=oper.InsertButtonPropItem(pPropItem,"work_norm");
		oper.InsertEditPropItem(pItem,"work_norm.x");
		oper.InsertEditPropItem(pItem,"work_norm.y");
		oper.InsertEditPropItem(pItem,"work_norm.z");
	}
	if(pVertex->type==3)
	{
		oper.InsertEditPropItem(pPropItem,"radius");
		CPropTreeItem* pItem=oper.InsertButtonPropItem(pPropItem,"center");
		oper.InsertEditPropItem(pItem,"center.x");
		oper.InsertEditPropItem(pItem,"center.y");
		pItem=oper.InsertButtonPropItem(pPropItem,"column_norm");
		oper.InsertEditPropItem(pItem,"column_norm.x");
		oper.InsertEditPropItem(pItem,"column_norm.y");
		oper.InsertEditPropItem(pItem,"column_norm.z");
	}
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"edgeType");
	if(pVertex->manu_space>0)
	{
		if(pVertex->m_bRollEdge)
		{
			oper.InsertEditPropItem(pPropItem,"roll_edge_offset_dist");
			oper.InsertEditPropItem(pPropItem,"manu_space","卷边高度");
		}
		else if(pVertex->m_bWeldEdge)
			oper.InsertEditPropItem(pPropItem,"manu_space","加工间隙");
	}
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"local_point_vec");
	if(pVertex->local_point_vec>0)
		oper.InsertEditPropItem(pPropItem,"local_point_y");
	pPropList->Redraw();
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//
BOOL ModifyVertexProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CPPEView *pView=theApp.GetView();
	if(pView==NULL)
		return FALSE;
	CXhPtrSet<PROFILE_VER> selectVertexs;
	pView->m_xSelectEntity.GetSelectVertexs(selectVertexs,pView->GetCurProcessPart());
	PROFILE_VER *pVertex=NULL,*pFirstVertex=(PROFILE_VER*)pPropList->m_pObj;
	if(pFirstVertex==NULL || selectVertexs.GetNodeNum()<=0)
		return FALSE;
	CPropertyListOper<PROFILE_VER> oper(pPropList,pFirstVertex);
	if(PROFILE_VER::GetPropID("vertex.x")==pItem->m_idProp)
	{
		for(pVertex=selectVertexs.GetFirst();pVertex;pVertex=selectVertexs.GetNext())
			pVertex->vertex.x=atof(valueStr);
		oper.SetItemPropValue("vertex");
	}
	else if(PROFILE_VER::GetPropID("vertex.y")==pItem->m_idProp)
	{
		for(pVertex=selectVertexs.GetFirst();pVertex;pVertex=selectVertexs.GetNext())
			pVertex->vertex.y=atof(valueStr);
		oper.SetItemPropValue("vertex");
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	CProcessPart* pPart=pView->GetCurProcessPart();
	for(pVertex=selectVertexs.GetFirst();pVertex;pVertex=selectVertexs.GetNext())
	{
		PROFILE_VER* pVerInfo=pPart->FromVertexHiberId(pVertex->hiberId);
		pVerInfo->vertex=pVertex->vertex;
	}
	pView->SyncPartInfo(true);
	return TRUE;
}
BOOL VertexButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	PROFILE_VER *pVertex=(PROFILE_VER*)pPropList->m_pObj;
	if(pVertex==NULL)
		return FALSE;
	CReportVectorDlg vecDlg;
	CPropertyListOper<PROFILE_VER> oper(pPropList,pVertex);
	if(PROFILE_VER::GetPropID("vertex")==pItem->m_idProp)
	{
		vecDlg.m_sCaption = "顶点坐标";
		vecDlg.m_fVectorX = pVertex->vertex.x;
		vecDlg.m_fVectorY = pVertex->vertex.y;
		vecDlg.m_fVectorZ = 0;
		if(vecDlg.DoModal()==IDOK)
		{
			pVertex->vertex.Set(vecDlg.m_fVectorX,vecDlg.m_fVectorY,0);
			oper.SetItemPropValue("vertex");
			oper.SetItemPropValue("vertex.x");
			oper.SetItemPropValue("vertex.y");
		}
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	CPPEView *pView=theApp.GetView();
	CProcessPart* pPart=pView->GetCurProcessPart();
	PROFILE_VER* pVertextInfo=pPart->FromVertexHiberId(pVertex->hiberId);
	if(pVertextInfo==NULL)
		return FALSE;
	pVertextInfo->CloneVertex(pVertex);
	pView->SyncPartInfo(true,false);
	pView->Refresh();
	return TRUE; 
}
//显示顶点信息
BOOL CPPEView::DisplayVertexProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL)
		return FALSE;
	CXhPtrSet<PROFILE_VER> selectVertex;
	m_xSelectEntity.GetSelectVertexs(selectVertex,m_pProcessPart);
	if(selectVertex.GetNodeNum()<=0)
		return FALSE;
	PROFILE_VER* pVertex=selectVertex.GetFirst();
	//设置属性栏
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<PROFILE_VER> oper(pPropList,pVertex,&selectVertex);
	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pVertex;
	pPropList->SetModifyValueFunc(ModifyVertexProperty);
	pPropList->SetButtonClickFunc(VertexButtonClick);

	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL,*pSonItem=NULL;
	//基本信息
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo","外形顶点基本信息");
	pPropItem=oper.InsertEditPropItem(pParentItem,"id");	//唯一标识
	pPropItem->SetReadOnly();
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"cVertexType");	//顶点类型
	pPropItem->SetReadOnly();
	//顶点坐标
	pPropItem=oper.InsertBtnEditPropItem(pParentItem,"vertex","原始顶点坐标","原始顶点坐标");
	pPropItem->m_bHideChildren=FALSE;
	pSonItem=oper.InsertEditPropItem(pPropItem,"vertex.x");
	pSonItem->SetReadOnly();
	pSonItem=oper.InsertEditPropItem(pPropItem,"vertex.y");
	pSonItem->SetReadOnly();
	pPropList->Redraw();
	if (m_pProcessPart&&m_pProcessPart->IsPlate())
	{	//加工坐标系顶点坐标
		CProcessPlate *pPlate = (CProcessPlate*)m_pProcessPart;
		GECS mcs;
		CXhChar100 sText;
		pPlate->GetMCS(mcs);
		f3dPoint vertex = pVertex->vertex;
		coord_trans(vertex, mcs, FALSE);
		pPropItem = oper.InsertBtnEditPropItem(pParentItem, "vertexInMcs");
		pPropItem->m_bHideChildren = FALSE;
		pSonItem = oper.InsertEditPropItem(pPropItem, "vertexInMcs.x");
		sText.Printf("%f", vertex.x);
		SimplifiedNumString(sText);
		pSonItem->m_lpNodeInfo->m_strPropValue = sText;
		pSonItem->SetReadOnly();
		//
		pSonItem = oper.InsertEditPropItem(pPropItem, "vertexInMcs.y");
		sText.Printf("%f", vertex.y);
		SimplifiedNumString(sText);
		pSonItem->m_lpNodeInfo->m_strPropValue = sText;
		pSonItem->SetReadOnly();
	}
	pPropList->Redraw();
	return TRUE;
}