#include "StdAfx.h"
#include "DocManagerReactor.h"
#include "CadToolFunc.h"
#include "DockBarManager.h"
#include "ExplodeTxtDlg.h"

CDocManagerReactor::CDocManagerReactor()
{
}


CDocManagerReactor::~CDocManagerReactor()
{
}

void CDocManagerReactor::documentActivated(AcApDocument* pActivatedDoc)
{
	if (pActivatedDoc == NULL)
		return;
	//
	CAcModuleResourceOverride useThisRes;
#ifndef __UBOM_ONLY_
	CExplodeTxtDlg* pDlg=g_xDockBarManager.GetExplodeTxtDlgPtr();
	if (pDlg)
		pDlg->SetFocus();
#endif
}
