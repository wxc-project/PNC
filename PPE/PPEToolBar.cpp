// LDSMFCToolBar.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "LicFuncDef.h"
#include "PPEToolBar.h"


// CPPEToolBar

IMPLEMENT_DYNAMIC(CPPEToolBar, CMFCToolBar)

CPPEToolBar::CPPEToolBar()
{

}

CPPEToolBar::~CPPEToolBar()
{
}


BEGIN_MESSAGE_MAP(CPPEToolBar, CMFCToolBar)
END_MESSAGE_MAP()



// CPPEToolBar 消息处理程序

int ToolbarButtonIndexFromID(CMFCToolBar* pToolbar,DWORD btnResID)
{
	CMFCToolBarButton* pBtn;
	for(int i=0;pToolbar->GetAllButtons().GetSize()>0;i++){
		if((pBtn=pToolbar->GetButton(i))==NULL)
			break;
		if(pBtn->m_nID==btnResID)
			return i;
	};
	return -1;
}
bool IsNeedLoadToolBar(UINT uiResID)
{
#ifndef __PNC_
	if(uiResID==IDR_TOOLBAR_CUT_PLATE||uiResID==IDR_TOOLBAR_PBJ)
		return FALSE;
	else
		return TRUE;
#else
	if(uiResID==IDR_TOOLBAR_CUT_PLATE && !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return FALSE;	//切割钢板
	else if(uiResID==IDR_TOOLBAR_PBJ  && !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return FALSE;
	else
		return TRUE;
#endif
}
BOOL CreateToolBar(CWnd *pParentWnd,CPPEToolBar &toolBar,UINT uToolBarID,const char* strToolBarName,BOOL bSupportCustomize)
{
	if(!IsNeedLoadToolBar(uToolBarID))
		return FALSE;
	if (!toolBar.CreateEx(pParentWnd, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,CRect(1,1,1,1),uToolBarID) ||
		!toolBar.LoadToolBar(uToolBarID))
	{
		TRACE0("Failed to create toolbar\n");
		return FALSE;   // fail to create
	}
	if(strlen(strToolBarName)>0)
		toolBar.SetWindowText(strToolBarName);

	if(bSupportCustomize)
	{
		CString strCustomize;
		BOOL bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
		ASSERT(bNameValid);
		toolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	}
	return TRUE;
}
void CPPEToolBar::ValidateAuthorize(UINT uiResID)
{
	int index;
#ifdef __PNC_
	if(uiResID==IDR_TOOLBAR_CUT_PLATE && !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
#endif
	{	//切割钢板工具栏
		//if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_NC_FILE_TXT))
		{
			if((index=ToolbarButtonIndexFromID(this,ID_FILE_SAVE_TXT))>=0)
				RemoveButton(index);
		}
		//if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_NC_FILE_NC))
		{
			if((index=ToolbarButtonIndexFromID(this,ID_FILE_SAVE_NC))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_FILE_SAVE_CNC))>=0)
				RemoveButton(index);
		}
		//if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_NC_FILE_TXT)&&
		//	 !VerifyValidFunction(LICFUNC::FUNC_IDENTITY_NC_FILE_NC))
		{
			if((index=ToolbarButtonIndexFromID(this,ID_DEF_CUT_IN_PT))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_PREVIEW_CUTTING_TRACK))>=0)
				RemoveButton(index);
		}
	}
#ifdef __PNC_
	else if(uiResID==IDR_TOOLBAR_PBJ && !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
#endif
	{
		//if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_NC_FILE_PBJ))
		{
			if((index=ToolbarButtonIndexFromID(this,ID_DISPLAY_BOLT_SORT))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_SMART_SORT_BOLT))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_BATCH_SORT_HOLE))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_ADJUST_HOLE_ORDER))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_FILE_SAVE_PBJ))>=0)
				RemoveButton(index);
			if((index=ToolbarButtonIndexFromID(this,ID_FILE_SAVE_PMZ))>=0)
				RemoveButton(index);
		}
	}
}
BOOL CPPEToolBar::LoadToolBar(UINT uiResID, UINT uiColdResID, UINT uiMenuResID, BOOL bLocked, UINT uiDisabledResID, UINT uiMenuDisabledResID, UINT uiHotResID)
{
	if(!IsNeedLoadToolBar(uiResID))
		return FALSE;
	BOOL retcode=CMFCToolBar::LoadToolBar(uiResID,uiColdResID,uiMenuResID,bLocked,uiDisabledResID,uiMenuDisabledResID,uiHotResID);
	ValidateAuthorize(uiResID);
	return retcode;
}
BOOL CPPEToolBar::LoadState(LPCTSTR lpszProfileName, int nIndex, UINT uiID)
{
	BOOL retcode=CMFCToolBar::LoadState(lpszProfileName, nIndex, uiID);
	ValidateAuthorize(m_uiOriginalResID);
	return retcode;
}

