#pragma once
#include "acdocman.h"
#include "XhCharString.h"
#include "CadObjLife.h"

class CDwgFileOper
{
public:
	static void RetreivedEntSetFromFile(const char* sFileName, CHashSet<AcDbObjectId> &selectedEntIdSet);
	static void OpenOrActiveFile(const char* sFileName, bool bExecuteDxfOutBeforeClose=TRUE);
	static bool SaveDxfFile(const char* sFileName);
	static void CloseAllFile(BOOL bSaveFile);
	static AcApDocument *GetCurDoc(const char*sFileName);
};