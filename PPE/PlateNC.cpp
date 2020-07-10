#include "StdAfx.h"
#include "PPE.h"
#include "MainFrm.h"
#include "PPEView.h"
#include "PropertyListOper.h"
#include "NcPart.h"
#include "AdjustOrderDlg.h"
#include "LicFuncDef.h"
#include "SysPara.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
// 钢板切割相关
void CPPEView::OnDefPlateCutInPt()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	g_pSolidDraw->ReleaseSnapStatus();
	if (g_sysPara.IsValidDisplayFlag(CNCPart::FLAME_MODE))
	{
		if(g_sysPara.flameCut.m_bCutPosInInitPos==0)
			m_cCurTask = TASK_DEF_PLATE_CUT_IN_PT;
		else
		{
			AfxMessageBox("请将切入点定位方式切换到[指定轮廓点]");
			return;
		}
	}
	else if (g_sysPara.IsValidDisplayFlag(CNCPart::PLASMA_MODE))
	{
		if(g_sysPara.plasmaCut.m_bCutPosInInitPos==0)
			m_cCurTask = TASK_DEF_PLATE_CUT_IN_PT;
		else
		{
			AfxMessageBox("请将切入点定位方式切换到[指定轮廓点]");
			return;
		}
	}
	else
	{
		AfxMessageBox("请切换显示模式到[火焰切割]或[等离子切割]");
		return;
	}
#endif
}
void CPPEView::OnUpdateDefPlateCutInPt(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
	{
		if(g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC&&
			m_pProcessPart&&m_pProcessPart->m_cPartType == CProcessPart::TYPE_PLATE &&
			(g_sysPara.IsValidDisplayFlag(CNCPart::FLAME_MODE)||g_sysPara.IsValidDisplayFlag(CNCPart::PLASMA_MODE)))
			bEnable = TRUE;
	}
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEView::OnPreviewCuttingTrack()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	g_pPartEditor->ExecuteCommand(IPEC::CMD_DRAW_CUTTING_TRACK);
#endif
}
void CPPEView::OnUpdatePreviewCuttingTrack(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
	{
		if (g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC &&
			m_pProcessPart&&m_pProcessPart->m_cPartType == CProcessPart::TYPE_PLATE&&
			(g_sysPara.IsValidDisplayFlag(CNCPart::FLAME_MODE) || g_sysPara.IsValidDisplayFlag(CNCPart::PLASMA_MODE)))
			bEnable = TRUE;
	}	
#endif
	pCmdUI->Enable(bEnable);
}
//CUT_POINT属性栏
BOOL ModifyCutPtProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CPPEView *pView=theApp.GetView();
	if(pView==NULL)
		return FALSE;
	CProcessPart* pPart=pView->GetCurProcessPart();
	if(pPart==NULL||!pPart->IsPlate())
		return FALSE;
	CProcessPlate *pPlate=(CProcessPlate*)pPart;
	CXhPtrSet<CUT_POINT> selectCutPts;
	pView->m_xSelectEntity.GetSelectCutPts(selectCutPts,pPart);
	CUT_POINT *pPt=NULL,*pFirstPt=(CUT_POINT*)pPropList->m_pObj;
	if(selectCutPts.GetNodeNum()<=0 ||pFirstPt==NULL)
		return FALSE;
	bool bModify=false;
	CPropertyListOper<CUT_POINT> oper(pPropList,pFirstPt);
	if(CUT_POINT::GetPropID("cInLineLen")==pItem->m_idProp)
	{
		for(pPt=selectCutPts.GetFirst();pPt;pPt=selectCutPts.GetNext())
			pPt->cInLineLen=atoi(valueStr);
		bModify=true;
	}
	else if(CUT_POINT::GetPropID("fInAngle")==pItem->m_idProp)
	{
		for(pPt=selectCutPts.GetFirst();pPt;pPt=selectCutPts.GetNext())
			pPt->fInAngle=(float)atof(valueStr);
		bModify=true;
	}
	else if(CUT_POINT::GetPropID("cOutLineLen")==pItem->m_idProp)
	{
		for(pPt=selectCutPts.GetFirst();pPt;pPt=selectCutPts.GetNext())
			pPt->cOutLineLen=atoi(valueStr);
		bModify=true;
	}
	else if(CUT_POINT::GetPropID("fOutAngle")==pItem->m_idProp)
	{
		for(pPt=selectCutPts.GetFirst();pPt;pPt=selectCutPts.GetNext())
			pPt->fOutAngle=(float)atof(valueStr);
		bModify=true;
	}
	else 
		return FALSE;
	//将更新的数据保存到文件中
	for(pPt=selectCutPts.GetFirst();pPt;pPt=selectCutPts.GetNext())
	{
		CUT_POINT* pCutPt=pPlate->FromCutPtHiberId(pPt->GetHiberId(pPlate->GetKey()));
		*pCutPt=*pPt;
	}
	pView->SyncPartInfo(bModify);
	return TRUE;
}

BOOL CPPEView::DisplayCutPointProperty()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return FALSE;
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL)
		return FALSE;
	CXhPtrSet<CUT_POINT> selectCutPts;
	m_xSelectEntity.GetSelectCutPts(selectCutPts,m_pProcessPart);
	if(selectCutPts.GetNodeNum()<=0)
		return FALSE;
	CUT_POINT *pFirstCutPt=selectCutPts.GetFirst();
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<CUT_POINT> oper(pPropList,pFirstCutPt,&selectCutPts);

	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pFirstCutPt;
	pPropList->SetModifyValueFunc(ModifyCutPtProperty);

	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL;
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo");
	pPropItem=oper.InsertEditPropItem(pParentItem,"hEntId");
	pPropItem->SetReadOnly();
	pPropItem=oper.InsertButtonPropItem(pParentItem,"InLine");
	pPropItem->m_bHideChildren=FALSE;
	oper.InsertEditPropItem(pPropItem,"cInLineLen");
	oper.InsertEditPropItem(pPropItem,"fInAngle");
	pPropItem=oper.InsertButtonPropItem(pParentItem,"OutLine");
	pPropItem->m_bHideChildren=FALSE;
	oper.InsertEditPropItem(pPropItem,"cOutLineLen");
	oper.InsertEditPropItem(pPropItem,"fOutAngle");
#endif
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// 钢板孔处理
//////////////////////////////////////////////////////////////////////////
void CPPEView::AdjustHoleOrder()
{
#ifdef __PNC_
	if (m_pProcessPart == NULL || !m_pProcessPart->IsPlate())
		return;
	CProcessPlate* pPlate = (CProcessPlate*)m_pProcessPart;
	CXhChar16 sPartNo = pPlate->GetPartNo();
	//进行调整操作
	CHashList<BOLT_INFO> boltHashList;
	BOLT_INFO* pBolt = NULL, *pNewBolt = NULL;
	for (pBolt = pPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pPlate->m_xBoltInfoList.GetNext())
	{
		pNewBolt = boltHashList.Add(pBolt->keyId);
		pNewBolt->CloneBolt(pBolt);
	}
	CAdjustOrderDlg dlg;
	dlg.pPlate = pPlate;
	if (dlg.DoModal() != IDOK)
		return;
	//重新写入螺栓
	CProcessPlate* pDestPlate = NULL, *pCompPlate = NULL;
	if (g_sysPara.IsValidDisplayFlag(CNCPart::PUNCH_MODE))
	{
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::PUNCH_MODE);
		pDestPlate->m_xBoltInfoList.Empty();
		for (int i = 0; i < dlg.keyList.GetNodeNum(); i++)
		{
			pBolt = boltHashList.GetValue(dlg.keyList[i]);
			if(pBolt==NULL)
				continue;
			pNewBolt = pDestPlate->m_xBoltInfoList.Add(0);
			pNewBolt->CloneBolt(pBolt);
		}
	}
	else if (g_sysPara.IsValidDisplayFlag(CNCPart::DRILL_MODE))
	{
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::DRILL_MODE);
		pDestPlate->m_xBoltInfoList.Empty();
		for (int i = 0; i < dlg.keyList.GetNodeNum(); i++)
		{
			pBolt = boltHashList.GetValue(dlg.keyList[i]);
			if (pBolt == NULL)
				continue;
			pNewBolt = pDestPlate->m_xBoltInfoList.Add(0);
			pNewBolt->CloneBolt(pBolt);
		}
	}
	else
		return;
	if (g_sysPara.nc.m_ciDisplayType != CNCPart::DRILL_MODE && g_sysPara.nc.m_ciDisplayType != CNCPart::PUNCH_MODE)
	{	//复合钢板复制排序螺栓
		pCompPlate = (CProcessPlate*)model.FromPartNo(sPartNo, g_sysPara.nc.m_ciDisplayType);
		pCompPlate->m_xBoltInfoList.Empty();
		for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
			pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
	}
	//刷新界面，并将修改信息保存到相应文件中
	UpdateCurWorkPartByPartNo(m_pProcessPart->GetPartNo());
	Refresh();
#endif
}
void CPPEView::SmartSortBolts(BYTE ciAlgType)
{
#ifdef __PNC_
	if (m_pProcessPart == NULL || !m_pProcessPart->IsPlate())
		return;
	CWaitCursor waitCursor;
	CXhChar16 sPartNo = m_pProcessPart->GetPartNo();
	CProcessPlate* pDestPlate = NULL, *pCompPlate = NULL;
	if (g_sysPara.IsValidDisplayFlag(CNCPart::PUNCH_MODE))
	{
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::PUNCH_MODE);
		CNCPart::RefreshPlateHoles(pDestPlate, CNCPart::PUNCH_MODE, ciAlgType);
	}
	else if (g_sysPara.IsValidDisplayFlag(CNCPart::DRILL_MODE))
	{
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::DRILL_MODE);
		CNCPart::RefreshPlateHoles(pDestPlate, CNCPart::DRILL_MODE, ciAlgType);
	}
	else
		return;
	if (g_sysPara.nc.m_ciDisplayType != CNCPart::DRILL_MODE && g_sysPara.nc.m_ciDisplayType != CNCPart::PUNCH_MODE)
	{	//复合钢板复制排序螺栓
		pCompPlate = (CProcessPlate*)model.FromPartNo(sPartNo, g_sysPara.nc.m_ciDisplayType);
		pCompPlate->m_xBoltInfoList.Empty();
		for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
			pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
	}
	//
	UpdateCurWorkPartByPartNo(m_pProcessPart->GetPartNo());
	Refresh();
#endif
}
//显示螺栓孔序号（F2）
void CPPEView::OnDisplayBoltSort()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	if(m_pProcessPart==NULL || !m_pProcessPart->IsPlate())
		return;
	if(g_pPartEditor->GetDrawMode()!=IPEC::DRAW_MODE_NC)
		return;
	m_bDisplayLsOrder=!m_bDisplayLsOrder;
	g_pPartEditor->SetDrawBoltMode(m_bDisplayLsOrder);
	g_pSolidDraw->BuildDisplayList(this);
#endif
}
void CPPEView::OnUpdateDisplayBoltSort(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER) &&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
	if(bEnable)
		pCmdUI->SetCheck(m_bDisplayLsOrder);
}
//贪吃算法进行螺栓排序（F3）
void CPPEView::OnSmartSortBolts1()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	SmartSortBolts(CNCPart::ALG_GREEDY);
#endif
}
void CPPEView::OnUpdateSmartSortBolts1(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER) &&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
//回溯算法进行螺栓排序（F4）
void CPPEView::OnSmartSortBolts2()
{
#ifdef __PNC_
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	SmartSortBolts(CNCPart::ALG_BACKTRACK);
#endif
}
void CPPEView::OnUpdateSmartSortBolts2(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER) &&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
//退火算法进行螺栓排序（F5）
void CPPEView::OnSmartSortBolts3()
{
#ifdef __PNC_
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	SmartSortBolts(CNCPart::ALG_ANNEAL);
#endif
}
void CPPEView::OnUpdateSmartSortBolts3(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER) &&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
//批量进行智能螺栓排序
void CPPEView::OnBatchSortHole()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CXhChar16 sCurPartNo;
	if (m_pProcessPart)
		sCurPartNo = m_pProcessPart->GetPartNo();
	int index = 0, num = model.PartCount();
	model.DisplayProcess(0,"批量优化螺栓顺序进度");
	for (CProcessPart* pProcessPart = model.EnumPartFirst(); pProcessPart; pProcessPart = model.EnumPartNext(), index++)
	{
		model.DisplayProcess(ftoi(100 * index / num), "批量优化螺栓顺序进度");
		if (!pProcessPart->IsPlate())
			continue;
		CXhChar16 sPartNo = pProcessPart->GetPartNo();
		CProcessPlate *pDestPlate = NULL, *pCompPlate = NULL;
		//冲床工艺下的螺栓孔排序
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::PUNCH_MODE);
		CNCPart::RefreshPlateHoles(pDestPlate, CNCPart::PUNCH_MODE);
		if (g_sysPara.IsValidDisplayFlag(CNCPart::PUNCH_MODE) && g_sysPara.nc.m_ciDisplayType != CNCPart::PUNCH_MODE)
		{
			pCompPlate = (CProcessPlate*)model.FromPartNo(sPartNo, g_sysPara.nc.m_ciDisplayType);
			pCompPlate->m_xBoltInfoList.Empty();
			for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
				pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
		}
		//钻床工艺下的螺栓孔排序
		pDestPlate = (CProcessPlate*)model.FromPartNo(sPartNo, CNCPart::DRILL_MODE);
		CNCPart::RefreshPlateHoles(pDestPlate, CNCPart::DRILL_MODE);
		if (g_sysPara.IsValidDisplayFlag(CNCPart::DRILL_MODE) && g_sysPara.nc.m_ciDisplayType != CNCPart::DRILL_MODE)
		{
			pCompPlate = (CProcessPlate*)model.FromPartNo(sPartNo, g_sysPara.nc.m_ciDisplayType);
			pCompPlate->m_xBoltInfoList.Empty();
			for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
				pCompPlate->m_xBoltInfoList.Append(*pBolt, pDestPlate->m_xBoltInfoList.GetCursorKey());
		}
	}
	model.DisplayProcess(100,"批量优化螺栓顺序完成");
	//
	UpdateCurWorkPartByPartNo(sCurPartNo);
	Refresh();
#endif
}
void CPPEView::OnUpdateBatchSortHole(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER)&&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
//手动调整螺栓孔排序
void CPPEView::OnAdjustHoleOrder()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	AdjustHoleOrder();
#endif
}
void CPPEView::OnUpdateAdjustHoleOrder(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if (VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER) &&
		g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC)
		bEnable = TRUE;
#endif
	pCmdUI->Enable(bEnable);
}