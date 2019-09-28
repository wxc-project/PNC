#include "StdAfx.h"
#include "MenuFunc.h"
#include "CadToolFunc.h"
#include "DragEntSet.h"
#include "ProcBarDlg.h"
#include "RevisionDlg.h"
#include "LogFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//
void RevisionPartProcess()
{
	CLogErrorLife logErrLife;
	InitDrawingEnv();
	if(!g_xUbomModel.InitBomModel())
	{
		logerr.Log("ÅäÖÃÎÄ¼þ¶ÁÈ¡Ê§°Ü!");
		return;
	}
	g_pRevisionDlg->InitRevisionDlg();
}