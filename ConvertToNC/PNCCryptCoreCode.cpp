#include "StdAfx.h"
#include "PNCCryptCoreCode.h"
#include "XhLicAgent.h"
#include "dbmain.h"
#include "XeroExtractor.h"
#include "f_ent_list.h"

//初始化加密数据函数
//////////////////////////////////////////////////////////////////////////
//全局加密数据
int CPNCCryptFuncMark::acdbOpenAcDbEntityFuncMark;
int CPNCCryptFuncMark::AppendRelaEntityFuncMark;
//初始化加密数据函数
//No.1 Procedure Item
struct Shell_OpenAcDbEnt_Para {
	AcDbEntity* m_pEnt;
	AcDbObjectId m_id;
	AcDb::OpenMode m_mode;
	bool m_bOpenErasedEntity;
	Acad::ErrorStatus m_retCode;
	Shell_OpenAcDbEnt_Para(AcDbEntity* pEnt, AcDbObjectId id, AcDb::OpenMode mode, bool openErasedEntity = false){
		m_pEnt = pEnt;
		m_id = id;
		m_mode = mode;
		m_bOpenErasedEntity = openErasedEntity;
		m_retCode = Acad::ErrorStatus::eOk;
	}
};
Acad::ErrorStatus
acdbOpenAcDbEntity_ByShell(AcDbEntity*& pEnt, AcDbObjectId id, AcDb::OpenMode mode, bool openErasedEntity /*= false*/)
{
	Shell_OpenAcDbEnt_Para para(pEnt, id, mode, openErasedEntity);
	CallProcItem(0, CPNCCryptFuncMark::acdbOpenAcDbEntityFuncMark, (DWORD)&para);
	if (para.m_retCode == Acad::eOk)
		pEnt = para.m_pEnt;
	return para.m_retCode;
}
//No.2 Procedure Item
struct Shell_AppendRelaEntity_Para {
	CPNCModel *m_pBelongModel;
	AcDbEntity* m_pEnt;
	CHashList<CAD_ENTITY>* m_pHashRelaEntIdList;
	CAD_ENTITY* m_pRetEnt;
	Shell_AppendRelaEntity_Para(CPNCModel *pBelongModel, AcDbEntity* pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList)
	{
		m_pBelongModel = pBelongModel;
		m_pEnt = pEnt;
		m_pHashRelaEntIdList = pHashRelaEntIdList;
		m_pRetEnt = NULL;
	}
};
CAD_ENTITY* AppendRelaEntity_ByShell(CPNCModel *pBelongModel, AcDbEntity* pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList)
{
	Shell_AppendRelaEntity_Para para(pBelongModel,pEnt,pHashRelaEntIdList);
	CallProcItem(0, CPNCCryptFuncMark::AppendRelaEntityFuncMark, (DWORD)&para);
	return para.m_pRetEnt;
}
//No.3 Procedure Item

//No.4 Procedure Item

//No.5 Procedure Item

//No.6 Procedure Item

//No.7 Procedure Item


//////////////////////////////////////////////////////////////////////////
//初始化加密数据函数
//No.1 Procedure Item
DWORD Shell_OpenAcDbEnt(DWORD dwParam)
{
	Shell_OpenAcDbEnt_Para* pPara = (Shell_OpenAcDbEnt_Para*)dwParam;
	if(pPara)
		pPara->m_retCode = acdbOpenAcDbEntity(pPara->m_pEnt,pPara->m_id,pPara->m_mode,pPara->m_bOpenErasedEntity);
	return 0;
}
//No.2 Procedure Item
CAD_ENTITY* __AppendRelaEntity(CPNCModel *pBelongModel, AcDbEntity* pEnt, 
							   CHashList<CAD_ENTITY>* pHashRelaEntIdList);
DWORD Shell_AppendRelaEntity(DWORD dwParam)
{
	Shell_AppendRelaEntity_Para *pPara = (Shell_AppendRelaEntity_Para*)dwParam;
	if (pPara)
		pPara->m_pRetEnt = __AppendRelaEntity(pPara->m_pBelongModel,pPara->m_pEnt,pPara->m_pHashRelaEntIdList);
	return 0;
}
//No.3 Procedure Item

//No.4 Procedure Item

//No.5 Procedure Item

//No.6 Procedure Item

//No.7 Procedure Item

void InitPNCCryptData()
{
	const int CRYPTITEM_COUNT = 10;
	CRYPT_PROCITEM arrProcItem[CRYPTITEM_COUNT];
	arrProcItem[0].cType = 1;	//函数
	arrProcItem[0].val.func = Shell_OpenAcDbEnt;
	arrProcItem[1].cType = 1;	//函数
	arrProcItem[1].val.func = Shell_AppendRelaEntity;
	DWORD order_arr[CRYPTITEM_COUNT];
	bool result = InitOrders(0, arrProcItem, CRYPTITEM_COUNT);
	result = GetCallOrders(0, order_arr, CRYPTITEM_COUNT);
	CPNCCryptFuncMark::acdbOpenAcDbEntityFuncMark = order_arr[0];
	CPNCCryptFuncMark::AppendRelaEntityFuncMark = order_arr[1];
}
