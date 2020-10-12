#include "StdAfx.h"
#include "SteelSealReactor.h"
#include "CadToolFunc.h"
#include "PNCModel.h"
#include "PNCCryptCoreCode.h"

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
		//ͨ����λ�ø������ݵ��λ��
		AcDbEntity *pEnt = NULL;
		XhAcdbOpenAcDbEntity(pEnt, AcDbObjectId((AcDbStub*)pPlateInfo->m_xMkDimPoint.idCadEnt),AcDb::kForWrite);
		CAcDbObjLife objLife(pEnt);
		if (pEnt == NULL||!pEnt->isKindOf(AcDbPoint::desc()))
			return;
		AcDbPoint *pPoint = (AcDbPoint*)pEnt;
		AcGePoint3d pos=pRectBlockRef->position();
		pPoint->setPosition(pos);
		//�����ֺ���λ��֮��ͬ������PPI�ļ��и�ӡ��λ��
		pPlateInfo->SyncSteelSealPos();
		if (CPlateProcessInfo::m_bCreatePPIFile)
		{	//����PPI�ļ�
			CString file_path;
			GetCurWorkPath(file_path);
			pPlateInfo->CreatePPiFile(file_path);
		}
		//���½���
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}
