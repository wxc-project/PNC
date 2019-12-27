#include "StdAfx.h"
#include "DockBarManager.h"
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"
#include "PNCSysPara.h"

#ifdef __SUPPORT_DOCK_UI_
CAcUiDialogPanel *g_pPartListDockBar = NULL;	//可停靠窗口
#endif
BOOL IsShowDisplayPartListDockBar()
{
#ifndef __UBOM_ONLY_
#ifdef __SUPPORT_DOCK_UI_
	if (g_pPartListDockBar == NULL)
		return FALSE;
	else
		return g_pPartListDockBar->IsWindowVisible();
#else
	if (g_pPartListDlg == NULL)
		return FALSE;
	else
		return g_pPartListDlg->IsWindowVisible();
#endif
#else
	return FALSE;
#endif
}

void DisplayPartListDockBar()
{	//显示可停靠的窗体
#ifndef __UBOM_ONLY_
	if (g_pncSysPara.m_bAutoLayout != CPNCSysPara::LAYOUT_SEG)
		return;
	if (IsShowDisplayPartListDockBar())
		return;
#ifdef __SUPPORT_DOCK_UI_
	CAcModuleResourceOverride resOverride;
	if (g_pPartListDockBar == NULL)
	{
		g_pPartListDockBar = new CAcUiDialogPanel();
		g_pPartListDockBar->Init(RUNTIME_CLASS(CPartListDlg), IDD_PART_LIST_DLG);
#if defined (_ARX_2007) && defined (__SUPPORT_DOCK_UI_) && !defined _UNICODE
		g_pPartListDockBar->Create(acedGetAcadFrame(), (LPCWSTR)_T("构件列表"));
#else
		g_pPartListDockBar->Create(acedGetAcadFrame(), _T("构件列表"));
#endif
		g_pPartListDockBar->SetWindowText(_T("构件列表"));// changes the text of the specified window's title bar (if it has one).
		g_pPartListDockBar->EnableDocking(CBRS_ALIGN_ANY);//CBRS_ALIGN_ANY   Allows docking on any side of the client area.  		
	}
	else
		g_pPartListDockBar->ShowDialog();

	acedGetAcadFrame()->FloatControlBar(g_pPartListDockBar, CPoint(200, 100), CBRS_ALIGN_RIGHT); //初始位置//CBRS_ALIGN_TOP   Orients the control bar vertically.
	acedGetAcadFrame()->DockControlBar(g_pPartListDockBar);
	acedGetAcadFrame()->ShowControlBar(g_pPartListDockBar, TRUE, FALSE);//void ShowControlBar( CControlBar* pBar, BOOL bShow, BOOL bDelay );
#else
	if (g_pPartListDlg == NULL)
		g_pPartListDlg = new CPartListDlg();
	if (g_pPartListDlg->GetSafeHwnd() == NULL)
		g_pPartListDlg->CreateDlg();	//g_pPartListDlg->Create(CPartListDlg::IDD);
	if (g_pPartListDlg->GetSafeHwnd())
	{
		g_pPartListDlg->UpdatePartList();
		g_pPartListDlg->ShowWindow(SW_SHOW);
	}
	else
	{
		//g_xPartListDlg.DoModal();
	}
#endif
#endif
}

void HidePartListDockBar()
{
#ifdef __SUPPORT_DOCK_UI_
	if (g_pPartListDockBar)
		g_pPartListDockBar->CloseDialog();
#endif
}
void DestoryPartListDockBar()
{
#ifndef __UBOM_ONLY_
#ifdef __SUPPORT_DOCK_UI_
	if (g_pPartListDockBar)
	{
		g_pPartListDockBar->DestroyWindow();
		delete g_pPartListDockBar;
		g_pPartListDockBar = NULL;
	}
#else
	if (g_pPartListDlg)
	{
		g_pPartListDlg->DestroyWindow();
		delete g_pPartListDlg;
		g_pPartListDlg = NULL;
	}
#endif
#endif
}
#ifndef __UBOM_ONLY_
CPartListDlg* GetPartListDlgPtr()
{
	if (!IsShowDisplayPartListDockBar())
		DisplayPartListDockBar();
	CPartListDlg *pPartListDlg = g_pPartListDlg;
	CLockDocumentLife lockDocument;
#ifdef __SUPPORT_DOCK_UI_
	pPartListDlg = g_pPartListDockBar != NULL ? (CPartListDlg*)g_pPartListDockBar->GetDlgPtr() : NULL;
#endif
	return pPartListDlg;
}
#endif
