#include "StdAfx.h"
#include "SteelSealReactor.h"
#include "CadToolFunc.h"
#include "PNCModel.h"

CSteelSealReactor::CSteelSealReactor()
{
}


CSteelSealReactor::~CSteelSealReactor()
{
}

void CSteelSealReactor::modified(const AcDbObject* dbObj)
{
	if (dbObj == NULL)
		return;
	if (dbObj->isKindOf(AcDbBlockReference::desc()))
	{
		AcDbBlockReference *pRectBlockRef = (AcDbBlockReference*)dbObj;
		AcDbObjectId dictObjId=pRectBlockRef->extensionDictionary();
		AcDbDictionary *pDict=NULL;
		acdbOpenObject(pDict, dictObjId, AcDb::kForRead);
		if (pDict == NULL)
			return;
		CAcDbObjLife dictLife(pDict);
		AcDbObjectId xrecObjId,plateEntId;
		AcDbXrecord *pXrec = NULL;
		resbuf *pRb=NULL, *pNextRb=NULL;
#ifdef _ARX_2007
		if (pDict->getAt(L"TOWER_XREC", (AcDbObject* &)pXrec, AcDb::kForWrite) == Acad::eOk)
#else
		if (pDict->getAt("TOWER_XREC", (AcDbObject* &)pXrec, AcDb::kForWrite) == Acad::eOk)
#endif
		{
			pXrec->rbChain(&pRb);
			if (pRb == NULL)
				return;
			CAcDbObjLife dictLife(pXrec);
			plateEntId = AcDbObjectId((AcDbStub*)pRb->resval.rlong);
			ads_relrb(pRb);
		}
		if (plateEntId == AcDbObjectId::kNull)
			return;
		CPlateProcessInfo *pPlateInfo =model.GetPlateInfo(plateEntId);
		if (pPlateInfo == NULL||!pPlateInfo->m_bEnableReactor)
			return;
		//通过块位置更新数据点的位置
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, AcDbObjectId((AcDbStub*)pPlateInfo->m_xMkDimPoint.idCadEnt),AcDb::kForWrite);
		if (pEnt == NULL||!pEnt->isKindOf(AcDbPoint::desc()))
			return;
		AcDbPoint *pPoint = (AcDbPoint*)pEnt;
		AcGePoint3d pos=pRectBlockRef->position();
		pPoint->setPosition(pos);
		pEnt->close();
		//更新字盒子位置之后，同步更新PPI文件中钢印号位置
		pPlateInfo->SyncSteelSealPos();
		//更新PPI文件
		CString file_path;
		GetCurWorkPath(file_path);
		pPlateInfo->CreatePPiFile(file_path);
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}
