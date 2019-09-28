#include "StdAfx.h"
#include "PPEView.h"
#include "PPE.h"
#include "MainFrm.h"
#include "PartPropertyDlg.h"
#include "PartLib.h"
#include "CutWingAngleDlg.h"
#include "PropertyListOper.h"
#include "PPEModel.h"
#include "SyncPropertyDlg.h"

static f3dPoint StringToPoint(CString valueStr)
{
	int i=0;
	f3dPoint pt;
	valueStr.Remove('(');
	valueStr.Remove(')');
	for(char* token=strtok(valueStr.GetBuffer(),",");token;token=strtok(NULL,","))
	{
		if(i==0)
			pt.x=atof(token);
		else if(i==1)
			pt.y=atof(token);
		if(i==3)
			pt.z=atof(token);
		i++;
	}
	return pt;
}
//From PPEDoc.cpp
char restore_JG_guige(char* guige, double &wing_wide, double &wing_thick);
BOOL ModifyLineAngleProperty(CPropertyList *pPropList,CPropTreeItem* pItem, CString &valueStr)
{
	CProcessAngle *pLineAngle=(CProcessAngle*)pPropList->m_pObj;
	if(pLineAngle==NULL)
		return FALSE;
	CPropertyListOper<CProcessAngle> oper(pPropList,pLineAngle);

	BOOL bUpdatePartItemText=FALSE;
	if(pItem->m_idProp==CProcessAngle::GetPropID("m_Seg"))
		pLineAngle->SetSegI(SEGI(valueStr.GetBuffer()));
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_sPartNo"))
	{
		CProcessPart *pPart=model.FromPartNo(valueStr);
		if(pPart==NULL)
		{
			pLineAngle->SetPartNo(valueStr);
			bUpdatePartItemText=TRUE;
		}
		else if(pPart!=pLineAngle)
		{
			AfxMessageBox(CXhChar50("已存在件号为%s的构件,请重新设置件号!",valueStr));
			return FALSE;
		}
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_cMaterial"))	//角钢材质
	{
		pLineAngle->cMaterial = QuerySteelBriefMatMark(valueStr.GetBuffer());
		bUpdatePartItemText=TRUE;
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_sSpec")) 
	{	//角钢规格
		double wing_wide,wing_thick;
		restore_JG_guige(valueStr.GetBuffer(),wing_wide,wing_thick);
		pLineAngle->m_fWidth=(float)wing_wide;
		pLineAngle->m_fThick=(float)wing_thick;
		bUpdatePartItemText=TRUE;
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_wLength"))
		pLineAngle->m_wLength=atoi(valueStr);
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_fWeight"))
		pLineAngle->m_fWeight=(float)atof(valueStr);
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_sNote"))
		pLineAngle->SetNotes(valueStr.GetBuffer());
	//压扁
	else if(pItem->m_idProp==CProcessAngle::GetPropID("wing_push_X1_Y2"))
	{
		pLineAngle->wing_push_X1_Y2=valueStr[0]-'0';
		pPropList->DeleteItemByPropId(CProcessAngle::GetPropID("start_push_pos"));
		pPropList->DeleteItemByPropId(CProcessAngle::GetPropID("end_push_pos"));
		if(pLineAngle->wing_push_X1_Y2>0)
		{
			oper.InsertEditPropItem(pItem,"start_push_pos");
			oper.InsertEditPropItem(pItem,"end_push_pos");
		}
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("start_push_pos"))
		pLineAngle->start_push_pos=atoi(valueStr);
	else if(pItem->m_idProp==CProcessAngle::GetPropID("end_push_pos"))
		pLineAngle->end_push_pos=atoi(valueStr);
	//清根、铲背、焊接、坡口
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_bCutRoot"))
	{
		if(valueStr.CompareNoCase("是")==0)
			pLineAngle->m_bCutRoot=TRUE;
		else 
			pLineAngle->m_bCutRoot=FALSE;
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_bCutBer"))
	{
		if(valueStr.CompareNoCase("是")==0)
			pLineAngle->m_bCutBer=TRUE;
		else 
			pLineAngle->m_bCutBer=FALSE;
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_bWeld"))
	{
		if(valueStr.CompareNoCase("是")==0)
			pLineAngle->m_bWeld=TRUE;
		else 
			pLineAngle->m_bWeld=FALSE;
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_bNeedFillet"))
	{
		if(valueStr.CompareNoCase("是")==0)
			pLineAngle->m_bNeedFillet=TRUE;
		else 
			pLineAngle->m_bNeedFillet=FALSE;
	}
	//开合角
	else if(pItem->m_idProp==CProcessAngle::GetPropID("kaihe_base_wing_x0_y1"))
		pLineAngle->kaihe_base_wing_x0_y1=valueStr[0]-'0';
	/*else if(pItem->m_idProp>=CProcessAngle::GetPropID("x_wing_ls[0]")
		&&pItem->m_idProp<=CProcessAngle::GetPropID("y_wing_ls[99]"))
	{
		f3dPoint pt=StringToPoint(valueStr);
		int nCount=max(pLineAngle->x_wing_ls.GetNodeNum(),pLineAngle->y_wing_ls.GetNodeNum());
		for(int i=0;i<nCount;i++)
		{
			CXhChar50 sXWingKey("x_wing_ls[%d]",i),sYWingKey("y_wing_ls[%d]",i);
			if(CProcessAngle::GetPropID(sXWingKey)==pItem->m_idProp)
			{
				pLineAngle->x_wing_ls[i].posX=(float)pt.x;
				pLineAngle->x_wing_ls[i].posY=(float)pt.y;
				break;
			}
			else if(CProcessAngle::GetPropID(sYWingKey)==pItem->m_idProp)
			{
				pLineAngle->y_wing_ls[i].posX=(float)pt.x;
				pLineAngle->y_wing_ls[i].posY=(float)pt.y;
				break;
			}
		}
	}*/
	else
		return FALSE;
	if(bUpdatePartItemText)
	{
		CPartTreeDlg *pTreeDlg=((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
		if(pTreeDlg)
			pTreeDlg->UpdateTreeItemText();
	}
	//更新数据至编辑器，重新进行绘制
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SyncPartInfo(true,false);
	g_pPartEditor->Draw();
	g_p2dDraw->RenderDrawing();
	return TRUE;
}

BOOL LineAngleButtonClick(CPropertyList* pPropList,CPropTreeItem* pItem)
{
	CProcessAngle *pLineAngle=(CProcessAngle*)pPropList->m_pObj;
	if(pLineAngle==NULL)
		return FALSE;
	CPropertyListOper<CProcessAngle> oper(pPropList,pLineAngle);

	CCutWingAngleDlg cutdlg;
	if(pItem->m_idProp==CProcessAngle::GetPropID("startCutWing"))
	{
		cutdlg.m_iCutXWingAngle = pLineAngle->cut_wing[0];
		if(cutdlg.m_iCutXWingAngle==1||cutdlg.m_iCutXWingAngle==2)
		{
			cutdlg.m_fCutWingWide = pLineAngle->cut_wing_para[0][1];
			cutdlg.m_fCutWingLen  = pLineAngle->cut_wing_para[0][0];
			cutdlg.m_fCutWingLen2 = pLineAngle->cut_wing_para[0][2];
		}
			
		cutdlg.m_fCutAngleXLen		 = pLineAngle->cut_angle[0][0];
		cutdlg.m_fCutAngleXWide	 = pLineAngle->cut_angle[0][1];
		cutdlg.m_fCutAngleYLen		 = pLineAngle->cut_angle[1][0];
		cutdlg.m_fCutAngleYWide	 = pLineAngle->cut_angle[1][1];
		if(cutdlg.DoModal()==IDOK)
		{
			pLineAngle->cut_wing[0] = cutdlg.m_iCutXWingAngle;
			if(fabs(cutdlg.m_fCutWingWide)<EPS&&fabs(cutdlg.m_fCutWingLen)<EPS&&fabs(cutdlg.m_fCutWingLen2)<EPS)
				pLineAngle->cut_wing[0]=0;	//切肢数据均为0时不进行切肢操作 wht 11-01-19
			if(cutdlg.m_iCutXWingAngle==1||cutdlg.m_iCutXWingAngle==2)
			{
				pLineAngle->cut_wing_para[0][1] = cutdlg.m_fCutWingWide;
				pLineAngle->cut_wing_para[0][0] = cutdlg.m_fCutWingLen ;
				pLineAngle->cut_wing_para[0][2] = cutdlg.m_fCutWingLen2;
			}
			pLineAngle->cut_angle[0][0] = cutdlg.m_fCutAngleXLen;
			pLineAngle->cut_angle[0][1] = cutdlg.m_fCutAngleXWide;
			pLineAngle->cut_angle[1][0] = cutdlg.m_fCutAngleYLen;
			pLineAngle->cut_angle[1][1] = cutdlg.m_fCutAngleYWide;
			
			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("startCutWingX");
			oper.SetItemPropValue("startCutWingY");
		}
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("endCutWing"))
	{
		cutdlg.m_iCutXWingAngle = pLineAngle->cut_wing[1];
		if(cutdlg.m_iCutXWingAngle==1||cutdlg.m_iCutXWingAngle==2)
		{
			cutdlg.m_fCutWingWide = pLineAngle->cut_wing_para[1][1];
			cutdlg.m_fCutWingLen  = pLineAngle->cut_wing_para[1][0];
			cutdlg.m_fCutWingLen2 = pLineAngle->cut_wing_para[1][2];
		}
			
		cutdlg.m_fCutAngleXLen		 = pLineAngle->cut_angle[2][0];
		cutdlg.m_fCutAngleXWide	 = pLineAngle->cut_angle[2][1];
		cutdlg.m_fCutAngleYLen		 = pLineAngle->cut_angle[3][0];
		cutdlg.m_fCutAngleYWide	 = pLineAngle->cut_angle[3][1];
		if(cutdlg.DoModal()==IDOK)
		{
			pLineAngle->cut_wing[1] = cutdlg.m_iCutXWingAngle;
			if(fabs(cutdlg.m_fCutWingWide)<EPS&&fabs(cutdlg.m_fCutWingLen)<EPS&&fabs(cutdlg.m_fCutWingLen2)<EPS)
				pLineAngle->cut_wing[1]=0;	//切肢数据均为0时不进行切肢操作 wht 11-01-19
			if(cutdlg.m_iCutXWingAngle==1||cutdlg.m_iCutXWingAngle==2)
			{
				pLineAngle->cut_wing_para[1][1] = cutdlg.m_fCutWingWide;
				pLineAngle->cut_wing_para[1][0] = cutdlg.m_fCutWingLen ;
				pLineAngle->cut_wing_para[1][2] = cutdlg.m_fCutWingLen2;
			}
			pLineAngle->cut_angle[2][0] = cutdlg.m_fCutAngleXLen;
			pLineAngle->cut_angle[2][1] = cutdlg.m_fCutAngleXWide;
			pLineAngle->cut_angle[3][0] = cutdlg.m_fCutAngleYLen;
			pLineAngle->cut_angle[3][1] = cutdlg.m_fCutAngleYWide;

			oper.SetItemPropValue(pItem->m_idProp);
			oper.SetItemPropValue("endCutWingX");
			oper.SetItemPropValue("endCutWingY");
		}
	}
	else if(pItem->m_idProp==CProcessAngle::GetPropID("m_dwInheritPropFlag"))
	{
		CSyncPropertyDlg dlg;
		dlg.m_pProcessPart=pLineAngle;
		dlg.DoModal();
	}
	else
		return FALSE;
	//更新数据至编辑器，重新进行绘制
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SyncPartInfo(true,false);
	g_pPartEditor->Draw();
	g_p2dDraw->RenderDrawing();
	return TRUE;
}

CString MakeMaterialMarkSetString()
{
	CString matStr;
	for(int i=0;i<CSteelMatLibrary::GetCount();i++)
	{
		matStr+=CSteelMatLibrary::RecordAt(i).mark;
		if(i<CSteelMatLibrary::GetCount()-1)
			matStr+='|';
	}
	return matStr;
}
CString MakeAngleSetString(char cType='L')
{
	CString sizeStr;
	CXhChar100 sText;
	for(int i=0;i<jgguige_N;i++)
	{
		if(jgguige_table[i].cType!=cType)
			continue;
		sText.Printf("%.f",jgguige_table[i].wing_wide);
		SimplifiedNumString(sText);
		sizeStr+=sText;
		sText.Printf("X%.f",jgguige_table[i].wing_thick);
		SimplifiedNumString(sText);
		sizeStr+=sText;
		if(i<jgguige_N-1)
			sizeStr+='|';
	}
	return sizeStr;
}
BOOL CPPEView::DisplayLineAngleProperty()
{
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	if(pPropDlg==NULL || m_pProcessPart==NULL)
		return FALSE;
	CProcessAngle* pLineAngle=(CProcessAngle*)GetCurProcessPart();
	CPropertyList *pPropList = pPropDlg->GetPropertyList();
	CPropertyListOper<CProcessAngle> oper(pPropList,pLineAngle);

	CPropTreeItem* pRootItem=pPropList->GetRootItem();
	const int GROUP_BASICINFO=1,GROUP_NCINFO=2,GROUP_PROCESSCARD_INFO=3;
	pPropDlg->m_arrPropGroupLabel.RemoveAll();
	pPropDlg->RefreshTabCtrl(0);
	pPropList->CleanCallBackFunc();
	pPropList->CleanTree();
	pPropList->m_nObjClassTypeId = 0;
	pPropList->m_pObj=pLineAngle;
	pPropList->SetModifyValueFunc(ModifyLineAngleProperty);
	pPropList->SetButtonClickFunc(LineAngleButtonClick);

	CPropTreeItem *pParentItem=NULL,*pPropItem=NULL,*pSonItem=NULL;
	pParentItem=oper.InsertPropItem(pRootItem,"basicInfo");
	oper.InsertEditPropItem(pParentItem,"m_sPartNo");
	oper.InsertEditPropItem(pParentItem,"m_Seg");
	oper.InsertCmbListPropItem(pParentItem,"m_cMaterial",MakeMaterialMarkSetString());
	oper.InsertCmbEditPropItem(pParentItem,"m_sSpec",MakeAngleSetString());
	oper.InsertEditPropItem(pParentItem,"m_wLength");
	oper.InsertEditPropItem(pParentItem,"m_fWeight");
	oper.InsertEditPropItem(pParentItem,"m_sNote");
	if(theApp.starter.m_bChildProcess&&theApp.starter.IsIncPartPattern())
		oper.InsertButtonPropItem(pParentItem,"m_dwInheritPropFlag");

	pParentItem=oper.InsertPropItem(pRootItem,"processInfo");
	//始端切角切肢
	pPropItem=oper.InsertButtonPropItem(pParentItem,"startCutWing");
	pSonItem=oper.InsertEditPropItem(pPropItem,"startCutWingX");
	pSonItem->SetReadOnly();
	pSonItem=oper.InsertEditPropItem(pPropItem,"startCutWingY");
	pSonItem->SetReadOnly();
	//终端切角切肢
	pPropItem=oper.InsertButtonPropItem(pParentItem,"endCutWing");
	pSonItem=oper.InsertEditPropItem(pPropItem,"endCutWingX");
	pSonItem->SetReadOnly();
	pSonItem=oper.InsertEditPropItem(pPropItem,"endCutWingY");
	pSonItem->SetReadOnly();
	//压扁
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"wing_push_X1_Y2");
	if(pLineAngle->IsWingPush())
	{
		oper.InsertEditPropItem(pPropItem,"start_push_pos");
		oper.InsertEditPropItem(pPropItem,"end_push_pos");
	}
	//清根、铲背、焊接、坡口
	oper.InsertCmbListPropItem(pParentItem,"m_bCutRoot");
	oper.InsertCmbListPropItem(pParentItem,"m_bCutBer");
	oper.InsertCmbListPropItem(pParentItem,"m_bWeld");
	oper.InsertCmbListPropItem(pParentItem,"m_bNeedFillet");
	//开合角
	pPropItem=oper.InsertCmbListPropItem(pParentItem,"kaihe_base_wing_x0_y1");
	pParentItem=oper.InsertPropItem(pRootItem,"boltInfo");
	//螺栓孔坐标
	pPropItem=oper.InsertEditPropItem(pParentItem,"x_wing_ls");
	pPropItem->SetReadOnly();
	/*int nBoltNum=pLineAngle->x_wing_ls.GetNodeNum();
	for(int i=0;i<nBoltNum;i++)
		oper.InsertEditPropItem(pPropItem,CXhChar50("x_wing_ls[%d]",i));*/
	pPropItem=oper.InsertEditPropItem(pParentItem,"y_wing_ls");
	pPropItem->SetReadOnly();
	/*nBoltNum=pLineAngle->y_wing_ls.GetNodeNum();
	for(int i=0;i<nBoltNum;i++)
		oper.InsertEditPropItem(pPropItem,CXhChar50("y_wing_ls[%d]",i));*/
	pPropList->Redraw();
	return TRUE;
}