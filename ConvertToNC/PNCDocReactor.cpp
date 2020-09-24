#include "StdAfx.h"
#include "PNCDocReactor.h"
#include "CadToolFunc.h"
#include "PNCDockBarManager.h"
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
	CPartListDlg* pPartDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
	if (pPartDlg)
		pPartDlg->ClearPartList();
#endif
}

void CDocManagerReactor::documentDestroyed(const char* fileName)
{
	if (strlen(fileName) <= 0)
		return;
	CAcModuleResourceOverride useThisRes;
#ifndef __UBOM_ONLY_
	if (model.m_sCurWorkFile.CompareNoCase(fileName) == 0)
	{
		CPartListDlg* pPartDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
		if (pPartDlg)
			pPartDlg->ClearPartList();
	}
#endif
}
