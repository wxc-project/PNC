#pragma once
#include "afxtoolbar.h"

// CPPEToolBar

class CPPEToolBar : public CMFCToolBar
{
	DECLARE_DYNAMIC(CPPEToolBar)
	void ValidateAuthorize(UINT uiResID);
public:
	CPPEToolBar();
	virtual ~CPPEToolBar();
	virtual BOOL LoadToolBar(UINT uiResID, UINT uiColdResID = 0, UINT uiMenuResID = 0, BOOL bLocked = FALSE,
		UINT uiDisabledResID = 0, UINT uiMenuDisabledResID = 0,  UINT uiHotResID = 0);
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT) -1);
protected:
	DECLARE_MESSAGE_MAP()
};
BOOL CreateToolBar(CWnd *pParentWnd,CPPEToolBar &toolBar,UINT uToolBarID,const char* strToolBarName,BOOL bSupportCustomize);

