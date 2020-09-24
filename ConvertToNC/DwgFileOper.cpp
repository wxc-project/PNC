#include "StdAfx.h"
#include "DwgFileOper.h"

////////////////////////////////////////////////////////////////////////////////////
// CDwgFileOper
void openDocHelper(void *pData)
{
	if (acDocManager->isApplicationContext())
	{
		AcApDocument* pDoc = acDocManager->curDocument();
#ifdef _ARX_2007
		Acad::ErrorStatus result = acDocManager->appContextOpenDocument(_bstr_t((const char *)pData));
#else
		Acad::ErrorStatus result = acDocManager->appContextOpenDocument((const char *)pData);
#endif
		//        if(result != Acad::eOk)
		//                acutPrintf("\nERROR: %s\n", acadErrorStatusText(result));
	}
	else
	{
#ifdef _ARX_2007
		acutPrintf(L"\nERROR: in Document context :%s\n", acDocManager->curDocument()->fileName());
#else
		acutPrintf("\nERROR: in Document context :%s\n", acDocManager->curDocument()->fileName());
#endif
	}
	return;
}

AcApDocument * CDwgFileOper::GetCurDoc(const char*sFileName)
{
	CLockDocumentLife lockDoc;
	//打开DWG文件
	CXhChar500 file_path;
	AcApDocument *pDoc = NULL;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, sFileName))
			break;
	}
	return pDoc;
}

void CDwgFileOper::OpenOrActiveFile(const char* sFileName, bool bExecuteDxfOutBeforeClose/*=FALSE*/)
{	
	if (sFileName == NULL || strlen(sFileName)==0)
		return;
	CLockDocumentLife lockDoc;
	//打开DWG文件
	CXhChar500 file_path;
	AcApDocument *pDoc = NULL;
	int nDocCount = 0;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		nDocCount++;
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, sFileName))
			break;
	}

	Acad::ErrorStatus retCode;
	if (strstr(file_path, sFileName))	//激活指定文件
		retCode = acDocManager->activateDocument(pDoc);
	else
	{	//打开指定文件
		if (acDocManager->isApplicationContext())
		{
#ifdef _ARX_2007
			retCode = acDocManager->appContextOpenDocument((ACHAR*)_bstr_t(sFileName));
#else
			retCode = acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
		}
		else
		{
			int a = 10;
			//acDocManager->executeInApplicationContext()
		}
	}
}

bool CDwgFileOper::SaveDxfFile(const char* sFileName)
{
	if (sFileName == NULL)
		return false;
	//打开DWG文件
	CXhChar500 file_path;
	AcApDocument *pDoc = NULL;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, sFileName))
			break;
	}
	if (strstr(file_path, sFileName))	//激活指定文件
	{
		acDocManager->activateDocument(pDoc);
		//保存
		GetCurDwg()->save();
		return true;
	}
	else
		return false;
}

void CDwgFileOper::CloseAllFile(BOOL bSaveFile)
{
	CAcadSysVarLife varLife("FILEDIA", 0);
	AcApDocument *pDoc = NULL;
	CXhChar500 file_path;
	//Acad::ErrorStatus retCode;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		acDocManager->closeDocument(pDoc);
	}
}


void CDwgFileOper::RetreivedEntSetFromFile(const char* sFileName, CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	OpenOrActiveFile(sFileName);
	//
	CAcModuleResourceOverride resOverride;
	ads_name ent_sel_set, entname;
	CHashSet<AcDbObjectId> textIdHash;
#ifdef _ARX_2007
	SendCommandToCad(L"zoom\na\n");	//全显
#else
	SendCommandToCad("zoom\na\n");	//全显
#endif
	//根据工艺卡块识别角钢
#ifdef _ARX_2007
	acedSSGet(L"ALL", NULL, NULL, NULL, ent_sel_set);
#else
	acedSSGet("ALL", NULL, NULL, NULL, ent_sel_set);
#endif
	AcDbObjectId entId, blockId;
	long ll;
	f3dPoint orig_pt;
	acedSSLength(ent_sel_set, &ll);
	selectedEntIdSet.Empty();
	for (long i = 0; i < ll; i++)
	{
		acedSSName(ent_sel_set, i, entname);
		acdbGetObjectId(entId, entname);
		selectedEntIdSet.SetValue(entId.asOldId(), entId);
	}
}