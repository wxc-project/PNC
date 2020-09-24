#if !defined(__PNCCryptFuncMark__INCLUDED_)
#define __PNCCryptFuncMark__INCLUDED_

#include "stdafx.h"
#include "f_ent_list.h"
#include "PNCModel.h"

class CPNCCryptFuncMark
{
public:
	static int acdbOpenAcDbEntityFuncMark;
	static int AppendRelaEntityFuncMark;
};

//初始化加密数据函数
//No.1 Procedure Item
Acad::ErrorStatus
acdbOpenAcDbEntity_ByShell(AcDbEntity*& pEnt, AcDbObjectId id, AcDb::OpenMode mode,bool openErasedEntity = false);
//No.2 Procedure Item
CAD_ENTITY* AppendRelaEntity_ByShell(CPNCModel *pBelongModel, AcDbEntity* pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList);
//No.3 Procedure Item
//No.4 Procedure Item
//No.5 Procedure Item
//No.6 Procedure Item
//No.7 Procedure Item

//
DWORD Shell_OpenAcDbEnt(DWORD dwParam);
DWORD Shell_AppendRelaEntity(DWORD dwParam);

void InitPNCCryptData();

#ifdef __ENCRYPT_CORE_CODE_
	#define XhAcdbOpenAcDbEntity acdbOpenAcDbEntity_ByShell
	#define XhAppendRelaEntity AppendRelaEntity_ByShell
#else
	#define XhAcdbOpenAcDbEntity acdbOpenAcDbEntity
	#define XhAppendRelaEntity __AppendRelaEntity
#endif

#endif