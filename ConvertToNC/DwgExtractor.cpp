#include "StdAfx.h"
#include "DwgExtractor.h"
#include "CadToolFunc.h"
#include "PNCSysPara.h"
#include "PNCSysSettingDlg.h"
#include "PNCCryptCoreCode.h"
#include "LayerTable.h"
#include "PolygonObject.h"
#include "DefCard.h"

//////////////////////////////////////////////////////////////////////////
//CDwgExtractor
//////////////////////////////////////////////////////////////////////////
CDwgExtractor::CDwgExtractor() 
{
	m_ciType = 0;
}
double CDwgExtractor::StandardHoleD(double fDiameter)
{
	//�Լ���õ��Ŀ׾�����Բ������ȷ��С����һλ
	double fHoleD = fDiameter;
	int nValue = (int)floor(fHoleD);	//��������
	double fValue = fHoleD - nValue;	//С������
	if (fValue < EPS2)	//�׾�Ϊ����
		fHoleD = nValue;
	else if (fValue > EPS_COS2)
		fHoleD = nValue + 1;
	else if (fabs(fValue - 0.5) < EPS2)
		fHoleD = nValue + 0.5;
	else
		fHoleD = ftoi(fHoleD);
	return fHoleD;
}
CString CDwgExtractor::GetCurDocFile()
{
	CString file_name;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc)
		file_name = pDoc->fileName();
	return file_name;
}
//////////////////////////////////////////////////////////////////////////
//CPlateExtractor
//////////////////////////////////////////////////////////////////////////
CPlateExtractor* CPlateExtractor::m_pExtractor = NULL;
CPlateExtractor::CPlateExtractor()
{
	m_ciType = IExtractor::PLATE;
}
CPlateExtractor* CPlateExtractor::GetExtractor()
{
	if (m_pExtractor == NULL)
		m_pExtractor = new CPlateExtractor();
	return m_pExtractor;
}
bool CPlateExtractor::ExtractPlates(CHashStrList<CPlateProcessInfo>& hashPlateInfo, BOOL bSupportSelectEnts)
{
	CLogErrorLife logErrLife;
	m_xAllEntIdSet.Empty();
	m_pHashPlate = &hashPlateInfo;
	//��������ϵ
	ShiftCSToWCS(TRUE);
	//��ѡͼԪ
	CHashSet<AcDbObjectId> selectedEntList;
	if (bSupportSelectEnts)
	{	//�����ֶ���ѡ
		//��������ȡ����ʱ����Ҫѡ�����е�ͼ�η�����ڵĹ���ʹ��
		if (g_pncSysPara.m_ciRecogMode != CPNCSysPara::FILTER_BY_PIXEL)
			SelCadEntSet(m_xAllEntIdSet, TRUE);
		if (!SelCadEntSet(selectedEntList))
			return false;
	}
	else
	{	//��������ͼԪ
		SelCadEntSet(m_xAllEntIdSet, TRUE);
		for (AcDbObjectId entId = m_xAllEntIdSet.GetFirst(); entId; entId = m_xAllEntIdSet.GetNext())
			selectedEntList.SetValue(entId.asOldId(), entId);
	}
	//�ӿ�ѡ��Ϣ����ȡ�иְ�ı�ʶ��ͳ�Ƹְ弯��
	CHashSet<AcDbObjectId> textIdHash;
	AcDbEntity *pEnt = NULL;
	int index = 1, nNum = selectedEntList.GetNodeNum();
	DisplayCadProgress(0, "����ͼֽ���ֱ�ע,ʶ��ְ������Ϣ.....");
	for (AcDbObjectId entId = selectedEntList.GetFirst(); entId.isValid(); entId = selectedEntList.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()) && !pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		textIdHash.SetValue(entId.asOldId(), entId);
		//
		BASIC_INFO baseInfo;
		GEPOINT dim_pos, dim_vec;
		CXhChar50 sPartNo;
		if (pEnt->isKindOf(AcDbText::desc()) || pEnt->isKindOf(AcDbMText::desc()))
		{
			if (!g_pncSysPara.ParsePartNoText(pEnt, sPartNo))
				continue;
			if (strlen(sPartNo) <= 0)
			{
				CXhChar500 sText = GetCadTextContent(pEnt);
				logerr.Log("�ְ���Ϣ{%s}�������õ�����ʶ����򣬵�ʶ��ʧ������ϵ�ź��ͷ�!", (char*)sText);
				continue;
			}
			else
				dim_pos = GetCadTextDimPos(pEnt, &dim_vec);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//�ӿ��н����ְ���Ϣ
			AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
			if (g_pncSysPara.RecogBasicInfo(pBlockRef, baseInfo))
			{
				AcGePoint3d txt_pos = pBlockRef->position();
				dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
				dim_vec.Set(cos(pBlockRef->rotation()), sin(pBlockRef->rotation()));
			}
			else
				continue;
			if (baseInfo.m_sPartNo.GetLength() > 0)
				sPartNo.Copy(baseInfo.m_sPartNo);
			else
			{
				logerr.Log("ͨ��ͼ��ʶ��ְ������Ϣ,ʶ��ʧ������ϵ�ź��ͷ�!");
				continue;
			}
		}
		CPlateProcessInfo *pExistPlate = hashPlateInfo.GetValue(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//������ͬ���������ı���Ӧ��ʵ�岻��ͬ��ʾ�����ظ� wht 19-07-22
			pExistPlate->repeatEntList.SetValue(entId.asOldId(), entId);
			logerr.Log("����{%s}���ظ���ȷ��!", (char*)sPartNo);
			continue;
		}
		//
		CPlateProcessInfo* pPlateProcess = hashPlateInfo.Add(sPartNo);
		pPlateProcess->dim_pos = dim_pos;
		pPlateProcess->dim_vec = dim_vec;
		if (baseInfo.m_sPartNo.GetLength() > 0)
		{
			pPlateProcess->plateInfoBlockRefId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = baseInfo.m_cMat;
			pPlateProcess->xPlate.m_fThick = (float)baseInfo.m_nThick;
			pPlateProcess->xPlate.m_nProcessNum = baseInfo.m_nNum;
			pPlateProcess->xPlate.m_nSingleNum = baseInfo.m_nNum;
			pPlateProcess->xPlate.mkpos = dim_pos;
			pPlateProcess->xPlate.mkVec = dim_vec;
			if (m_xPrjInfo.m_sTaStampNo.GetLength() <= 0 && baseInfo.m_sTaStampNo.GetLength() > 0)
				m_xPrjInfo.m_sTaStampNo.Copy(baseInfo.m_sTaStampNo);
			if (m_xPrjInfo.m_sTaType.GetLength() <= 0 && baseInfo.m_sTaType.GetLength() > 0)
				m_xPrjInfo.m_sTaType.Copy(baseInfo.m_sTaType);
			if (m_xPrjInfo.m_sPrjCode.GetLength() <= 0 && baseInfo.m_sPrjCode.GetLength() > 0)
				m_xPrjInfo.m_sPrjCode.Copy(baseInfo.m_sPrjCode);
		}
		else
		{
			pPlateProcess->partNoId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = 'S';
		}
	}
	DisplayCadProgress(100);
	if (selectedEntList.GetNodeNum() <= 0)
	{
		AfxMessageBox("δѡ����ȡ���ݣ��޷�ִ����ȡ������");
		return false;
	}
	else if (hashPlateInfo.GetNodeNum() <= 0)
	{
		if (AfxMessageBox("���ļ�����������ʶ�����ã��Ƿ��������ʶ�����ã�", MB_YESNO) == IDYES)
		{
			CAcModuleResourceOverride resOverride;
			CPNCSysSettingDlg dlg;
			dlg.m_iSelTabGroup = 1;
			dlg.DoModal();
		}
		return false;
	}
	//��ȡ�ְ����˨�ף���˨�顢Բ�����ǡ����Ρ���Բ��
	ExtractPlateBoltEnts(selectedEntList);
	//��ȡ�ְ��������
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		ExtractPlateProfileEx(selectedEntList);
	else
		ExtractPlateProfile(selectedEntList);
#ifndef __UBOM_ONLY_
	//����ְ�һ���ŵ����
	MergeManyPartNo();
	//���������պ�������¸ְ�Ļ�����Ϣ+��˨��Ϣ+��������Ϣ
	int nSum = 0;
	nNum = hashPlateInfo.GetNodeNum();
	DisplayCadProgress(0, "�޶��ְ���Ϣ<����+��˨+����>.....");
	for (CPlateProcessInfo* pPlateProcess = hashPlateInfo.GetFirst(); pPlateProcess; pPlateProcess = hashPlateInfo.GetNext(), nSum++)
	{
		DisplayCadProgress(int(100 * nSum / nNum));
		pPlateProcess->ExtractRelaEnts();
		pPlateProcess->CheckProfileEdge();
		pPlateProcess->UpdateBoltHoles();
		if (!pPlateProcess->UpdatePlateInfo())
			logerr.Log("����%s��ѡ���˴���ı߽�,������ѡ��.(λ�ã�%s)", (char*)pPlateProcess->GetPartNo(), (char*)CXhChar50(pPlateProcess->dim_pos));
		if (pPlateProcess->IsValid())
			pPlateProcess->InitPPiInfo();
	}
	DisplayCadProgress(100);
	//����ȡ�ĸְ���Ϣ�����������ļ���
	if (g_pncSysPara.m_iPPiMode == 0)
		SplitManyPartNo();
#else
	//UBOMֻ��Ҫ���¸ְ�Ļ�����Ϣ
	for (CPlateProcessInfo* pPlateProcess = EnumFirstPlate(); pPlateProcess; pPlateProcess = EnumNextPlate())
	{
		if (!pPlateProcess->IsValid())
		{
			pPlateProcess->CreateRgnByText();
#ifdef __ALFA_TEST_
			logerr.Log("(%s)�ְ����������ȡʧ��!", (char*)pPlateProcess->GetPartNo());
#endif
		}
	}
	MergeManyPartNo();
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//������纸ͼ��ĸְ�
			AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
			AcDbObjectId blockId = pReference->blockTableRecord();
			AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
			acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
			if (pTempBlockTableRecord == NULL)
				continue;
			pTempBlockTableRecord->close();
			CXhChar50 sName;
#ifdef _ARX_2007
			ACHAR* sValue = new ACHAR[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy((char*)_bstr_t(sValue));
			delete[] sValue;
#else
			char *sValue = new char[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy(sValue);
			delete[] sValue;
#endif
			if (!g_pncSysPara.IsHasPlateWeldTag(sName))
				continue;
			AcGePoint3d pos = pReference->position();
			CPlateProcessInfo* pPlateInfo = GetPlateInfo(GEPOINT(pos.x, pos.y));
			if (pPlateInfo == NULL)
				continue;
			pPlateInfo->xBomPlate.bWeldPart = TRUE;
		}
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()))
			continue;
		GEPOINT dim_pos = GetCadTextDimPos(pEnt);
		CPlateProcessInfo* pPlateInfo = GetPlateInfo(dim_pos);
		if (pPlateInfo == NULL)
			continue;	//
		BASIC_INFO baseInfo;
		if (g_pncSysPara.RecogBasicInfo(pEnt, baseInfo))
		{
			if (baseInfo.m_cMat > 0)
				pPlateInfo->xPlate.cMaterial = baseInfo.m_cMat;
			if (baseInfo.m_cQuality > 0)
				pPlateInfo->xPlate.cQuality = baseInfo.m_cQuality;
			if (baseInfo.m_nThick > 0)
				pPlateInfo->xPlate.m_fThick = (float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
			{
				pPlateInfo->xPlate.m_nSingleNum = pPlateInfo->xPlate.m_nProcessNum = baseInfo.m_nNum;
				pPlateInfo->partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			}
		}
		//�������������ڻ�����Ϣ�����磺141#����
		CXhChar100 sText = GetCadTextContent(pEnt);
		if (g_pncSysPara.IsHasPlateBendTag(sText))
			pPlateInfo->xBomPlate.siZhiWan = 1;
		if (g_pncSysPara.IsHasPlateWeldTag(sText))
			pPlateInfo->xBomPlate.bWeldPart = TRUE;
	}
	SplitManyPartNo();
	for (CPlateProcessInfo* pPlateInfo = EnumFirstPlate(); pPlateInfo; pPlateInfo = EnumNextPlate())
	{
		pPlateInfo->xBomPlate.sPartNo = pPlateInfo->GetPartNo();
		pPlateInfo->xBomPlate.cMaterial = pPlateInfo->xPlate.cMaterial;
		pPlateInfo->xBomPlate.cQualityLevel = pPlateInfo->xPlate.cQuality;
		pPlateInfo->xBomPlate.thick = pPlateInfo->xPlate.m_fThick;
		pPlateInfo->xBomPlate.nSumPart = pPlateInfo->xPlate.m_nProcessNum;	//�ӹ���
		pPlateInfo->xBomPlate.AddPart(pPlateInfo->xPlate.m_nSingleNum);		//������
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("�ְ�%s��Ϣ��ȡʧ��!", (char*)pPlateInfo->xBomPlate.sPartNo);
		else
			pPlateInfo->xBomPlate.sSpec.Printf("-%.f", pPlateInfo->xBomPlate.thick);
	}
#endif
	return true;
}
//��ѡ�е�ͼԪ���޳���Ч�ķ�������ͼԪ�������������̺����ߵȣ�
void CPlateExtractor::FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols)
{
	AcDbEntity *pEnt = NULL;
	std::set<CString> setKeyStr;
	std::multimap<CString, CAD_LINE> hashLineArrByPosKeyStr;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbLine::desc()) &&
			!pEnt->isKindOf(AcDbArc::desc()) &&
			!pEnt->isKindOf(AcDbEllipse::desc()))
			continue;
		AcDbCurve* pCurve = (AcDbCurve*)pEnt;
		AcGePoint3d acad_ptS, acad_ptE;
		pCurve->getStartPoint(acad_ptS);
		pCurve->getEndPoint(acad_ptE);
		GEPOINT ptS, ptE;
		Cpy_Pnt(ptS, acad_ptS);
		Cpy_Pnt(ptE, acad_ptE);
		ptS.z = ptE.z = 0;	//�ֶ���ʼ��Ϊ0����ֹ�������Zֵ����Сʱ��CXhChar50���캯���еĴ���Ϊ0.0���еĴ���Ϊ -0.0
		double len = DISTANCE(ptS, ptE);
		setKeyStr.insert(MakePosKeyStr(ptS));
		setKeyStr.insert(MakePosKeyStr(ptE));
		hashLineArrByPosKeyStr.insert(std::make_pair(MakePosKeyStr(ptS), CAD_LINE(objId, len)));
		hashLineArrByPosKeyStr.insert(std::make_pair(MakePosKeyStr(ptE), CAD_LINE(objId, len)));
	}
	//�޳���������ͼԪ����¼������������Ϣ
	int index = 1, nNum = selectedEntIdSet.GetNodeNum() + setKeyStr.size() + m_xBoltEntHash.GetNodeNum();
	DisplayCadProgress(0, "�޳���Ч�ķ�������ͼԪ.....");
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pEnt->isKindOf(AcDbSpline::desc()))
		{
			if (pSymbols)
				pSymbols->AppendSymbolEnt((AcDbSpline*)pEnt);
			//ȷ����������Ϊ�����߱�ʶ���ٴ�ѡ�����Ƴ�����������������Բ���޷�ʶ�� wht 20-07-18
			AcDbSpline *pSpline = (AcDbSpline*)pEnt;
			if (pSpline->numControlPoints() <= 6 && pSpline->numFitPoints() <= 4)
				selectedEntIdSet.DeleteNode(objId.asOldId());
		}
		else if (!g_pncSysPara.IsProfileEnt(pEnt))
			selectedEntIdSet.DeleteNode(objId.asOldId());
	}
	selectedEntIdSet.Clean();
	//�޳����������Լ�������
	for (std::set<CString>::iterator iter = setKeyStr.begin(); iter != setKeyStr.end(); ++iter, index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		if (hashLineArrByPosKeyStr.count(*iter) != 1)
			continue;
		std::multimap<CString, CAD_LINE>::iterator mapIter;
		mapIter = hashLineArrByPosKeyStr.find(*iter);
		if (mapIter != hashLineArrByPosKeyStr.end() && mapIter->second.m_fSize < WELD_MAX_HEIGHT)
			selectedEntIdSet.DeleteNode(mapIter->second.idCadEnt);
	}
	selectedEntIdSet.Clean();
	//�޳���˨�ıպ�����
	for (CBoltEntGroup* pBoltGroup = m_xBoltEntHash.GetFirst(); pBoltGroup; pBoltGroup = m_xBoltEntHash.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		if (selectedEntIdSet.GetValue(pBoltGroup->m_idEnt))
			selectedEntIdSet.DeleteNode(pBoltGroup->m_idEnt);
		for (size_t i = 0; i < pBoltGroup->m_xCirArr.size(); i++)
		{
			if (selectedEntIdSet.GetValue(pBoltGroup->m_xCirArr[i]))
				selectedEntIdSet.DeleteNode(pBoltGroup->m_xCirArr[i]);
		}
		for (size_t i = 0; i < pBoltGroup->m_xArcArr.size(); i++)
		{
			if (selectedEntIdSet.GetValue(pBoltGroup->m_xArcArr[i]))
				selectedEntIdSet.DeleteNode(pBoltGroup->m_xArcArr[i]);
		}
		for (size_t i = 0; i < pBoltGroup->m_xLineArr.size(); i++)
		{
			if (selectedEntIdSet.GetValue(pBoltGroup->m_xLineArr[i]))
				selectedEntIdSet.DeleteNode(pBoltGroup->m_xLineArr[i]);
		}
	}
	selectedEntIdSet.Clean();
	DisplayCadProgress(100);
	}
//����ͼ����ȡ��˨
bool CPlateExtractor::AppendBoltEntsByBlock(ULONG idBlockEnt)
{
	CAcDbObjLife objLife(MkCadObjId(idBlockEnt));
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL)
		return false;
	if (!pEnt->isKindOf(AcDbBlockReference::desc()))
		return false;
	AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
	AcDbObjectId blockId = pReference->blockTableRecord();
	AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
	acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
	if (pTempBlockTableRecord == NULL)
		return false;
	pTempBlockTableRecord->close();
	CXhChar50 sName;
#ifdef _ARX_2007
	ACHAR* sValue = new ACHAR[50];
	pTempBlockTableRecord->getName(sValue);
	sName.Copy((char*)_bstr_t(sValue));
	delete[] sValue;
#else
	char *sValue = new char[50];
	pTempBlockTableRecord->getName(sValue);
	sName.Copy(sValue);
	delete[] sValue;
#endif
	if (sName.GetLength() <= 0)
		return false;
	BOLT_BLOCK* pBoltD = g_pncSysPara.GetBlotBlockByName(sName);
	if (pBoltD == NULL)
		return false;	//�����õ���˨ͼ��
	double fHoleD = 0, fScale = pReference->scaleFactors().sx;
	AcDbBlockTableRecordIterator *pIterator = NULL;
	pTempBlockTableRecord->newIterator(pIterator);
	for (; !pIterator->done(); pIterator->step())
	{
		AcDbEntity *pSubEnt = NULL;
		pIterator->getEntity(pSubEnt, AcDb::kForRead);
		if (pSubEnt == NULL)
			continue;
		pSubEnt->close();
		if (pSubEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pSubEnt;
			if (fHoleD < pCir->radius() * 2)
				fHoleD = pCir->radius() * 2;
		}
		else if (pSubEnt->isKindOf(AcDbPolyline::desc()))
		{	//��������Բ������������������Ϊ�������
			AcDbPolyline *pPolyLine = (AcDbPolyline*)pSubEnt;
			if (pPolyLine->numVerts() <= 0 || !pPolyLine->isClosed())
				continue;
			AcGePoint3d point;
			pPolyLine->getPointAt(0, point);
			double fRadius = GEPOINT(point.x, point.y).mod();
			if (fHoleD < fRadius * 2)
				fHoleD = fRadius * 2;
		}
		else if (pSubEnt->isKindOf(AcDbLine::desc()))
		{	//ֱ�߶�(��������|����)
			AcDbLine* pLine = (AcDbLine*)pSubEnt;
			AcGePoint3d startPt, endPt;
			pLine->getStartPoint(startPt);
			pLine->getEndPoint(endPt);
			GEPOINT ptC, ptS, ptE, ptM, line_vec, up_vec, dw_vec;
			ptS.Set(startPt.x, startPt.y, 0);
			ptE.Set(endPt.x, endPt.y, 0);
			ptM = (ptS + ptE) * 0.5;
			line_vec = (ptE - ptS).normalized();
			up_vec.Set(-line_vec.y, line_vec.x, line_vec.z);
			normalize(up_vec);
			dw_vec = up_vec * -1;
			double fLen = DISTANCE(ptS, ptE);
			for (int i = 0; i < 4; i++)
			{
				if (i == 0)
					ptC = ptM + up_vec * (0.5*fLen / SQRT_3);
				else if (i == 1)
					ptC = ptM + dw_vec * (0.5*fLen / SQRT_3);
				else if (i == 2)
					ptC = ptM + up_vec * 0.5*fLen;
				else
					ptC = ptM + dw_vec * 0.5*fLen;
				if (ptC.IsEqual(GEPOINT(0, 0, 0), EPS2))
				{
					double fSize = (i < 2) ? (fLen / SQRT_3 * 2) : (fLen * SQRT_2);
					if (fHoleD < fSize)
						fHoleD = fSize;
					break;
				}
			}
		}
		else
			continue;
	}
	if (pIterator)
	{
		delete pIterator;
		pIterator = NULL;
	}
	if (fHoleD <= 0)
	{
		logerr.Log("��˨ͼ��(%s),������˨ֱ��ʧ��!", (char*)sName);
		return false;
	}
	//�����˨��
	fHoleD = fabs(fHoleD*fScale);
	CXhChar50 sKey("%d", pEnt->id().asOldId());
	CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(sKey);
	if (pBoltEnt == NULL)
	{
		pBoltEnt = m_xBoltEntHash.Add(sKey);
		pBoltEnt->m_ciType = CBoltEntGroup::BOLT_BLOCK;
		pBoltEnt->m_idEnt = pEnt->id().asOldId();
		pBoltEnt->m_fPosX = (float)pReference->position().x;
		pBoltEnt->m_fPosY = (float)pReference->position().y;
		pBoltEnt->m_fHoleD = (float)StandardHoleD(fHoleD);
	}
	return true;
}
//����ԲȦ��ȡ��˨
bool CPlateExtractor::AppendBoltEntsByCircle(ULONG idCirEnt)
{
	CAcDbObjLife objLife(MkCadObjId(idCirEnt));
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL)
		return false;
	if (!pEnt->isKindOf(AcDbCircle::desc()))
		return false;
	AcDbCircle* pCircle = (AcDbCircle*)pEnt;
	if (int(pCircle->radius()) <= 0)
		return false;	//ȥ����
	GEPOINT center(pCircle->center().x, pCircle->center().y, pCircle->center().z);
	double fDiameter = pCircle->radius() * 2;
	if (g_pncSysPara.m_ciMKPos == 2 &&
		fabs(g_pncSysPara.m_fMKHoleD - fDiameter) < EPS2)
		return false;	//ȥ����λ��
	if (fDiameter > g_pncSysPara.m_nMaxHoleD)
	{	//�������Բֱ�����ж��Ƿ񰴻��崦��
		CPlateProcessInfo* pPlateInfo = NULL;
		for (pPlateInfo = EnumFirstPlate(); pPlateInfo; pPlateInfo = EnumNextPlate())
		{
			double fDist = DISTANCE(pPlateInfo->dim_pos, center);
			if (fDist < pCircle->radius())
				return false;	//Բ���м��ű�ע��Ĭ��Ϊ����
		}
	}
	//�����˨��
	CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(MakePosKeyStr(center));
	if (pBoltEnt == NULL)
	{
		pBoltEnt = m_xBoltEntHash.Add(MakePosKeyStr(center));
		pBoltEnt->m_ciType = CBoltEntGroup::BOLT_CIRCLE;
		pBoltEnt->m_fPosX = (float)center.x;
		pBoltEnt->m_fPosY = (float)center.y;
		pBoltEnt->m_fHoleD = (float)StandardHoleD(fDiameter);
		pBoltEnt->m_xCirArr.push_back(idCirEnt);
	}
	else
	{
		pBoltEnt->m_fHoleD = (float)max(pBoltEnt->m_fHoleD, StandardHoleD(fDiameter));
		pBoltEnt->m_xCirArr.push_back(idCirEnt);
	}
	return true;
}
//���ݶ������ȡ��˨
bool CPlateExtractor::AppendBoltEntsByPolyline(ULONG idPolyline)
{
	CAcDbObjLife objLife(MkCadObjId(idPolyline));
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL)
		return false;
	if (!pEnt->isKindOf(AcDbPolyline::desc()))
		return false;
	AcDbPolyline *pPolyline = (AcDbPolyline*)pEnt;
	if (!pPolyline->isClosed())
		return false;
	vector<GEPOINT> ptArr;
	double dfLen = 0, dfDiameter = 0;
	int nVertNum = pPolyline->numVerts();
	if (nVertNum == 3)
	{
		for (int iVertIndex = 0; iVertIndex < 3; iVertIndex++)
		{
			if (pPolyline->segType(iVertIndex) != AcDbPolyline::kLine)
				return false;
			AcGeLineSeg3d acgeLine;
			pPolyline->getLineSegAt(iVertIndex, acgeLine);
			if (dfLen == 0)
				dfLen = acgeLine.length();
			if (fabs(dfLen - acgeLine.length()) > EPS2)
				return false;
			AcGePoint3d location;
			pPolyline->getPointAt(iVertIndex, location);
			ptArr.push_back(GEPOINT(location.x, location.y));
		}
		//�����˨��
		if (ptArr.size() != 3)
			return false;
		GEPOINT off_vec = (ptArr[1] - ptArr[0]) + (ptArr[2] - ptArr[0]);
		normalize(off_vec);
		dfDiameter = 2 * (dfLen / SQRT_3);
		GEPOINT center = ptArr[0] + off_vec * dfDiameter*0.5;
		CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(MakePosKeyStr(center));
		if (pBoltEnt == NULL)
		{
			pBoltEnt = m_xBoltEntHash.Add(MakePosKeyStr(center));
			pBoltEnt->m_ciType = CBoltEntGroup::BOLT_TRIANGLE;
			pBoltEnt->m_fPosX = (float)center.x;
			pBoltEnt->m_fPosY = (float)center.y;
			pBoltEnt->m_fHoleD = (float)StandardHoleD(dfDiameter);
			pBoltEnt->m_idEnt = idPolyline;
		}
		else
			pBoltEnt->m_fHoleD = (float)max(pBoltEnt->m_fHoleD, StandardHoleD(dfDiameter));
		return true;
	}
	else if (nVertNum == 4)
	{
		int nLineNum = 0, nArcNum = 0;
		for (int ii = 0; ii < 4; ii++)
		{
			if (pPolyline->segType(ii) == AcDbPolyline::kLine)
				nLineNum++;
			else if (pPolyline->segType(ii) == AcDbPolyline::kArc)
				nArcNum++;
			AcGePoint3d location;
			pPolyline->getPointAt(ii, location);
			ptArr.push_back(GEPOINT(location.x, location.y));
		}
		if (nLineNum == 4)
		{	//����
			for (int iVertIndex = 0; iVertIndex < 4; iVertIndex++)
			{
				AcGeLineSeg3d acgeLine;
				pPolyline->getLineSegAt(iVertIndex, acgeLine);
				if (dfLen == 0)
					dfLen = acgeLine.length();
				if (dfLen > 30 || fabs(dfLen - acgeLine.length()) > EPS2)
					return false;
			}
			dfDiameter = DISTANCE(ptArr[0], ptArr[2]);
		}
		else if (nLineNum == 2 && nArcNum == 2)
		{	//��Բ
			for (int iVertIndex = 0; iVertIndex < 4; iVertIndex++)
			{
				if (pPolyline->segType(iVertIndex) == AcDbPolyline::kArc)
				{
					AcGeCircArc3d acgeArc;
					pPolyline->getArcSegAt(iVertIndex, acgeArc);
					GEPOINT center;
					Cpy_Pnt(center, acgeArc.center());
					double fAngle = fabs(acgeArc.endAng() - acgeArc.startAng());
					if (dfDiameter == 0)
						dfDiameter = acgeArc.radius() * 2;
					if (dfDiameter > 25 || fabs(fAngle - Pi) > EPS2)
						return false;
				}
			}
		}
		else
			return false;
		//�����˨��
		GEPOINT center = (ptArr[0] + ptArr[2])*0.5;
		CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(MakePosKeyStr(center));
		if (pBoltEnt == NULL)
		{
			pBoltEnt = m_xBoltEntHash.Add(MakePosKeyStr(center));
			if (nLineNum == 4)
				pBoltEnt->m_ciType = CBoltEntGroup::BOLT_SQUARE;
			else
				pBoltEnt->m_ciType = CBoltEntGroup::BOLT_WAIST_ROUND;
			pBoltEnt->m_fPosX = (float)center.x;
			pBoltEnt->m_fPosY = (float)center.y;
			pBoltEnt->m_fHoleD = (float)StandardHoleD(dfDiameter);
			pBoltEnt->m_idEnt = idPolyline;
		}
		else
			pBoltEnt->m_fHoleD = (float)max(pBoltEnt->m_fHoleD, StandardHoleD(dfDiameter));
		return true;
	}
	return false;
}
//�������Ӷ��߶���ȡ��˨
bool CPlateExtractor::AppendBoltEntsByConnectLines(vector<CAD_LINE> vectorConnLine)
{
	if (vectorConnLine.size() <= 0)
		return false;
	typedef CAD_LINE* CADLinePtr;
	std::set<CString> setKeyStr;
	std::vector<CADLinePtr> vectorArc;
	std::map<CString, CAD_LINE> mapLineByPosStr;		//
	std::multimap<CString, CAD_ENTITY> mapTriangle;
	std::multimap<CString, CAD_ENTITY> mapSquare;
	typedef std::multimap<CString, CAD_ENTITY>::iterator ITERATOR;
	//����ÿ���߶μ�������������κ������κ�����ĵ�λ��
	GEPOINT ptS, ptE, ptM, line_vec, up_off_vec, dw_off_vec;
	for (size_t i = 0; i < vectorConnLine.size(); i++)
	{
		vectorConnLine[i].UpdatePos();
		//
		ptS = vectorConnLine[i].m_ptStart;
		ptE = vectorConnLine[i].m_ptEnd;
		ptM = (ptS + ptE) * 0.5;
		line_vec = (ptE - ptS).normalized();
		up_off_vec.Set(-line_vec.y, line_vec.x, line_vec.z);
		normalize(up_off_vec);
		dw_off_vec = up_off_vec * -1;
		double fLen = DISTANCE(ptS, ptE);
		if (vectorConnLine[i].ciEntType == TYPE_LINE)
		{
			//��ֱ�ߵ�ʼ�ն�������Ϊ��ֵ��¼ֱ��
			CString sPosKey;
			sPosKey.Format("S(%d,%d)E(%d,%d)", ftoi(ptS.x), ftoi(ptS.y), ftoi(ptE.x), ftoi(ptE.y));
			mapLineByPosStr.insert(std::make_pair(sPosKey, vectorConnLine[i]));
			//�����ֱ����������κ�����ĵ�λ��
			CAD_ENTITY xEntity;
			xEntity.idCadEnt = vectorConnLine[i].idCadEnt;
			xEntity.m_fSize = fLen / SQRT_3 * 2;
			xEntity.pos = ptM + up_off_vec * (0.5*fLen / SQRT_3);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapTriangle.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
			xEntity.pos = ptM + dw_off_vec * (0.5*fLen / SQRT_3);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapTriangle.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
			//�����ֱ����������κ�����ĵ�λ��
			xEntity.m_fSize = fLen * SQRT_2;
			xEntity.pos = ptM + up_off_vec * 0.5*fLen;
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapSquare.insert(std::make_pair(MakePosKeyStr(xEntity.pos), vectorConnLine[i]));
			xEntity.pos = ptM + dw_off_vec * 0.5*fLen;
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapSquare.insert(std::make_pair(MakePosKeyStr(xEntity.pos), vectorConnLine[i]));
		}
		else if (vectorConnLine[i].ciEntType == TYPE_ARC)
		{	//������Բ���ĵ�
			vectorArc.push_back(&vectorConnLine[i]);
		}
	}
	//��ȡ������Ҫ��������κͷ���ֱ�߶�
	std::set<CString>::iterator set_iter;
	for (set_iter = setKeyStr.begin(); set_iter != setKeyStr.end(); ++set_iter)
	{
		CString sKey = *set_iter;
		ITERATOR begIter = mapTriangle.lower_bound(sKey);
		ITERATOR endIter = mapTriangle.upper_bound(sKey);
		//����������˨��Ŀǰֻ�����׼��˨�׾���
		if (mapTriangle.count(sKey) == 3 && begIter->second.m_fSize < 30)
		{
			CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(sKey);
			if (pBoltEnt == NULL)
			{
				pBoltEnt = m_xBoltEntHash.Add(sKey);
				pBoltEnt->m_ciType = CBoltEntGroup::BOLT_TRIANGLE;
				pBoltEnt->m_fPosX = (float)begIter->second.pos.x;
				pBoltEnt->m_fPosY = (float)begIter->second.pos.y;
				pBoltEnt->m_fHoleD = (float)StandardHoleD(begIter->second.m_fSize);
				while (begIter != endIter)
				{
					pBoltEnt->m_xLineArr.push_back(begIter->second.idCadEnt);
					begIter++;
				}
			}
			else
				pBoltEnt->m_fHoleD = (float)max(pBoltEnt->m_fHoleD, StandardHoleD(begIter->second.m_fSize));
		}
		//���������Σ�Ŀǰֻ�����׼��˨�׾���
		if (mapSquare.count(sKey) == 4 && begIter->second.m_fSize < 30)
		{
			CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(sKey);
			if (pBoltEnt == NULL)
			{
				pBoltEnt = m_xBoltEntHash.Add(sKey);
				pBoltEnt->m_ciType = CBoltEntGroup::BOLT_SQUARE;
				pBoltEnt->m_fPosX = (float)begIter->second.pos.x;
				pBoltEnt->m_fPosY = (float)begIter->second.pos.y;
				pBoltEnt->m_fHoleD = (float)StandardHoleD(begIter->second.m_fSize);
				while (begIter != endIter)
				{
					pBoltEnt->m_xLineArr.push_back(begIter->second.idCadEnt);
					begIter++;
				}
			}
			else
				pBoltEnt->m_fHoleD = (float)max(pBoltEnt->m_fHoleD, StandardHoleD(begIter->second.m_fSize));
		}
	}
	//������Բͼ�Σ������еİ�Բ��������Դ���
	while (!vectorArc.empty())
	{
		CADLinePtr headArc = vectorArc.at(0);
		if (headArc->m_bMatch)
		{	//����Գɹ�
			vectorArc.erase(vectorArc.begin());
			continue;
		}
		GEPOINT ptHeadC, ptMatchC, ptHeadV, ptMatchV;
		ptHeadC = (headArc->m_ptStart + headArc->m_ptEnd) * 0.5;
		ptHeadV = (headArc->m_ptEnd - headArc->m_ptStart).normalized();
		size_t iMatch = 1;
		for (; iMatch < vectorArc.size(); iMatch++)
		{
			CADLinePtr matchArc = vectorArc.at(iMatch);
			ptMatchC = (matchArc->m_ptStart + matchArc->m_ptEnd) * 0.5;
			ptMatchV = (matchArc->m_ptEnd - matchArc->m_ptStart).normalized();
			double fDistance = DISTANCE(ptHeadC, ptMatchC);
			if (fabs(ptHeadV*ptMatchV) > EPS_COS2 && fDistance <= g_pncSysPara.m_fRoundLineLen)
				break;
		}
		if (iMatch < vectorArc.size())
		{	//�ҵ���Եİ�Բ��
			CADLinePtr matchArc = vectorArc.at(iMatch);
			matchArc->m_bMatch = TRUE;
			//
			GEPOINT center = (ptHeadC + ptMatchC)*0.5;
			CBoltEntGroup* pBoltEnt = m_xBoltEntHash.Add(MakePosKeyStr(center));
			pBoltEnt->m_ciType = CBoltEntGroup::BOLT_WAIST_ROUND;
			pBoltEnt->m_fPosX = (float)center.x;
			pBoltEnt->m_fPosY = (float)center.y;
			pBoltEnt->m_fHoleD = (float)StandardHoleD(headArc->m_fSize);
			pBoltEnt->m_xArcArr.push_back(headArc->idCadEnt);
			pBoltEnt->m_xArcArr.push_back(matchArc->idCadEnt);
		}
		vectorArc.erase(vectorArc.begin());
	}
	//������Բ���Ĺ���ֱ�߶�
	for (CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetFirst(); pBoltEnt; pBoltEnt = m_xBoltEntHash.GetNext())
	{
		if (pBoltEnt->m_ciType != CBoltEntGroup::BOLT_WAIST_ROUND)
			continue;
		if (pBoltEnt->m_xLineArr.size() == 2 && pBoltEnt->m_xArcArr.size() == 2)
			continue;	//�Ѵ���
		if (pBoltEnt->m_xArcArr.size() != 2)
			continue;
		GEPOINT ptS[2], ptE[2];
		for (size_t i = 0; i < pBoltEnt->m_xArcArr.size(); i++)
		{
			AcDbEntity *pEnt = NULL;
			XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(pBoltEnt->m_xArcArr[i]), AcDb::kForRead);
			CAcDbObjLife life(pEnt);
			if (pEnt == NULL || !pEnt->isKindOf(AcDbArc::desc()))
				continue;
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			ptS[i].Set(startPt.x, startPt.y, 0);
			ptE[i].Set(endPt.x, endPt.y, 0);
		}
		//���ҹ���ֱ��
		CString sKeyLine1[2], sKeyLine2[2];
		if (DISTANCE(ptS[0], ptS[1]) < DISTANCE(ptS[0], ptE[1]))
		{
			sKeyLine1[0].Format("S(%d,%d)E(%d,%d)", ftoi(ptS[0].x), ftoi(ptS[0].y), ftoi(ptS[1].x), ftoi(ptS[1].y));
			sKeyLine1[1].Format("S(%d,%d)E(%d,%d)", ftoi(ptS[1].x), ftoi(ptS[1].y), ftoi(ptS[0].x), ftoi(ptS[0].y));
			sKeyLine2[0].Format("S(%d,%d)E(%d,%d)", ftoi(ptE[0].x), ftoi(ptE[0].y), ftoi(ptE[1].x), ftoi(ptE[1].y));
			sKeyLine2[1].Format("S(%d,%d)E(%d,%d)", ftoi(ptE[1].x), ftoi(ptE[1].y), ftoi(ptE[0].x), ftoi(ptE[0].y));
		}
		else
		{
			sKeyLine1[0].Format("S(%d,%d)E(%d,%d)", ftoi(ptS[0].x), ftoi(ptS[0].y), ftoi(ptE[1].x), ftoi(ptE[1].y));
			sKeyLine1[1].Format("S(%d,%d)E(%d,%d)", ftoi(ptE[1].x), ftoi(ptE[1].y), ftoi(ptS[0].x), ftoi(ptS[0].y));
			sKeyLine2[0].Format("S(%d,%d)E(%d,%d)", ftoi(ptE[0].x), ftoi(ptE[0].y), ftoi(ptS[1].x), ftoi(ptS[1].y));
			sKeyLine2[1].Format("S(%d,%d)E(%d,%d)", ftoi(ptS[1].x), ftoi(ptS[1].y), ftoi(ptE[0].x), ftoi(ptE[0].y));
		}
		std::map<CString, CAD_LINE>::const_iterator cn_iter1, cn_iter2;
		cn_iter1 = mapLineByPosStr.find(sKeyLine1[0]);
		cn_iter2 = mapLineByPosStr.find(sKeyLine1[1]);
		if (cn_iter1 != mapLineByPosStr.end())
			pBoltEnt->m_xLineArr.push_back(cn_iter1->second.idCadEnt);
		else if (cn_iter2 != mapLineByPosStr.end())
			pBoltEnt->m_xLineArr.push_back(cn_iter2->second.idCadEnt);
		//
		cn_iter1 = mapLineByPosStr.find(sKeyLine2[0]);
		cn_iter2 = mapLineByPosStr.find(sKeyLine2[1]);
		if (cn_iter1 != mapLineByPosStr.end())
			pBoltEnt->m_xLineArr.push_back(cn_iter1->second.idCadEnt);
		else if (cn_iter2 != mapLineByPosStr.end())
			pBoltEnt->m_xLineArr.push_back(cn_iter2->second.idCadEnt);
	}
	return true;
}
//���ݹ������߶���ȡ��˨
bool CPlateExtractor::AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine)
{
	if (vectorAloneLine.size() <= 0)
		return false;
	for (size_t i = 0; i < vectorAloneLine.size(); i++)
		vectorAloneLine[i].UpdatePos();
	//����ʮ�ֽ����߶ν���ȷ����˨
	vector<CAD_LINE>::iterator iterS, iterE;
	while (vectorAloneLine.size() > 0)
	{
		iterS = vectorAloneLine.begin();
		GEPOINT linePtM1, lineVec1, linePtM2, lineVec2;
		linePtM1 = (iterS->m_ptStart + iterS->m_ptEnd)*0.5;
		lineVec1 = (iterS->m_ptStart - iterS->m_ptEnd).normalized();
		for (iterE = ++iterS; iterE != vectorAloneLine.end(); ++iterE)
		{
			linePtM2 = (iterE->m_ptStart + iterE->m_ptEnd)*0.5;
			lineVec2 = (iterE->m_ptStart - iterE->m_ptEnd).normalized();
			if (fabs(lineVec1*lineVec2) > EPS_COS2 && linePtM1.IsEqual(linePtM2, CPlateExtractor::DIST_ERROR))
			{	//���߶�ʮ���ཻ�ҽ����е�
				CXhChar50 sKey(linePtM1);
				CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(sKey);
				if (pBoltEnt == NULL)
				{
					pBoltEnt = m_xBoltEntHash.Add(sKey);
					pBoltEnt->m_ciType = 3;
					pBoltEnt->m_fPosX = (float)linePtM1.x;
					pBoltEnt->m_fPosY = (float)linePtM1.y;
				}
				break;
			}
		}
		//ɾ���������ͼԪ
		if (iterE != vectorAloneLine.end())
			vectorAloneLine.erase(iterE);
		vectorAloneLine.erase(iterS);
	}
	//������˨��λ�ý������ţ����ұպ�����
	for (CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetFirst(); pBoltEnt; pBoltEnt = m_xBoltEntHash.GetNext())
	{
		if (pBoltEnt->m_ciType < 3)
			continue;
		f2dRect rect;
		rect.topLeft.Set(pBoltEnt->m_fPosX - 24, pBoltEnt->m_fPosY + 24);
		rect.bottomRight.Set(pBoltEnt->m_fPosX + 24, pBoltEnt->m_fPosY - 24);
		HWND hMainWnd = adsw_acadMainWnd();
		ads_point L_T, R_B;
		L_T[X] = rect.topLeft.x;
		L_T[Y] = rect.topLeft.y;
		L_T[Z] = 0;
		R_B[X] = rect.bottomRight.x;
		R_B[Y] = rect.bottomRight.y;
		R_B[Z] = 0;
		if (CPlateExtractor::m_bSendCommand)
		{
			CXhChar16 sPosLT("%.2f,%.2f", L_T[X], L_T[Y]);
			CXhChar16 sPosRB("%.2f,%.2f", R_B[X], R_B[Y]);
			CXhChar50 sCmd;
			sCmd.Printf("ZOOM W %s\n%s\n ", (char*)sPosLT, (char*)sPosRB);
#ifdef _ARX_2007
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			ads_command(RTSTR, L"ZOOM", RTSTR, "W", RTPOINT, L_T, RTPOINT, R_B, RTNONE);
#else
			ads_command(RTSTR, "ZOOM", RTSTR, "W", RTPOINT, L_T, RTPOINT, R_B, RTNONE);
#endif
		}
		CHashSet<AcDbObjectId> relaEntList;
		if (!SelCadEntSet(relaEntList, L_T, R_B))
			continue;
		//TODO:������˨����Ĺ���ͼԪ����ȡ�պ����ͣ���ʼ����˨�׾�

	}
	return true;
}
void CPlateExtractor::ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//ʶ��ͼ��ʽ��˨&ԲȦ��˨&�����˨
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		XhAcdbOpenAcDbEntity(pEnt, objId, AcDb::kForRead);
		if (pEnt == NULL)
			continue;
		pEnt->close();
		if (pEnt->isKindOf(AcDbBlockReference::desc()))
			AppendBoltEntsByBlock(objId.asOldId());
		else if (pEnt->isKindOf(AcDbCircle::desc()))
			AppendBoltEntsByCircle(objId.asOldId());
		else if (pEnt->isKindOf(AcDbPolyline::desc()))
			AppendBoltEntsByPolyline(objId.asOldId());
	}
	//ʶ���ɢ�Ժ�Ķ������˨
	if (g_pncSysPara.IsRecongPolylineLs())
	{	//��ȡ���߶μ���
		std::vector<CAD_LINE> vectorLine;
		std::map<CString, int> mapLineNumInPos;
		typedef std::map<CString, int>::const_iterator ITERATOR;
		for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
		{
			XhAcdbOpenAcDbEntity(pEnt, objId, AcDb::kForRead);
			if (pEnt == NULL)
				continue;
			pEnt->close();
			if (!pEnt->isKindOf(AcDbLine::desc()) &&
				!pEnt->isKindOf(AcDbArc::desc()))
				continue;
			AcDbCurve* pCurve = (AcDbCurve*)pEnt;
			AcGePoint3d acad_ptS, acad_ptE;
			pCurve->getStartPoint(acad_ptS);
			pCurve->getEndPoint(acad_ptE);
			GEPOINT ptS, ptE;
			Cpy_Pnt(ptS, acad_ptS);
			Cpy_Pnt(ptE, acad_ptE);
			ptS.z = ptE.z = 0;
			double len = DISTANCE(ptS, ptE);
			if (len > g_pncSysPara.m_fRoundLineLen)
				continue;	//���˹����߶�
			if (pEnt->isKindOf(AcDbArc::desc()))
			{	//
				AcDbArc* pArc = (AcDbArc*)pEnt;
				double fAngle = fabs(pArc->endAngle() - pArc->startAngle());
				if (fabs(fAngle - Pi) > 0.1)
					continue;	//��180�ȵ�Բ��
			}
			vectorLine.push_back(CAD_LINE(objId, ptS, ptE));
			mapLineNumInPos[MakePosKeyStr(ptS)] += 1;
			mapLineNumInPos[MakePosKeyStr(ptE)] += 1;
		}
		//��ȡʼ�ն˶������ӵ��߶ν��
		std::vector<CAD_LINE> vectorConnLine;
		for (size_t ii = 0; ii < vectorLine.size(); ii++)
		{
			ITERATOR iterS = mapLineNumInPos.find(MakePosKeyStr(vectorLine[ii].m_ptStart));
			ITERATOR iterE = mapLineNumInPos.find(MakePosKeyStr(vectorLine[ii].m_ptEnd));
			if (iterS != mapLineNumInPos.end() && iterE != mapLineNumInPos.end())
			{
				if (iterS->second == 2 && iterE->second == 2)
					vectorConnLine.push_back(vectorLine[ii]);
			}
		}
		AppendBoltEntsByConnectLines(vectorConnLine);
	}
}
//����bpoly�����ʼ���ְ��������
void CPlateExtractor::ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//���һ���ض�ͼ��
	CLockDocumentLife lockCurDocumentLife;
	CHashSet<AcDbObjectId> hashSolidLineTypeId;	//��¼��Ч��ʵ������id wht 19-01-03
	AcDbObjectId idSolidLine, idNewLayer;
	CXhChar16 sNewLayer("pnc_layer"), sLineType = g_pncSysPara.m_sProfileLineType;
	CreateNewLayer(sNewLayer, sLineType, AcDb::kLnWt013, 1, idNewLayer, idSolidLine);
	hashSolidLineTypeId.SetValue(idSolidLine.asOldId(), idSolidLine);
	AcDbObjectId lineTypeId = GetLineTypeId("�����");
	if (lineTypeId.isValid())
		hashSolidLineTypeId.SetValue(lineTypeId.asOldId(), lineTypeId);
#ifdef __TIMER_COUNT_
	DWORD dwPreTick = timer.Start();
#endif
	//���˷�������ͼԪ(ͼ�顢���������������� �Լ��������ߵ�)
	CSymbolRecoginzer symbols;
	FilterInvalidEnts(selectedEntIdSet, &symbols);
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(1, dwPreTick);
#endif
	//����������ʶ�������˲�����������ͼԪ������Բ������Բ������Ӹ���ͼԪ
	double fMaxEdge = 0, fMinEdge = 200000;
	AcDbEntity *pEnt = NULL;
	std::map<CString, CAD_ENTITY> mapCircle;
	CHashSet<AcDbObjectId> xAssistCirSet;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//�����͹���
			AcDbObjectId lineId = GetEntLineTypeId(pEnt);
			if (lineId.isValid() && hashSolidLineTypeId.GetValue(lineId.asOldId()) == NULL)
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//��ͼ�����
			CXhChar50 sLayerName = GetEntLayerName(pEnt);
			if (g_pncSysPara.IsNeedFilterLayer(sLayerName))
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
		{	//����ɫ����
			int ciColorIndex = GetEntColorIndex(pEnt);
			if (ciColorIndex != g_pncSysPara.m_ciProfileColorIndex ||
				ciColorIndex == g_pncSysPara.m_ciBendLineColorIndex)
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		}
		GEPOINT ptS, ptE;
		if (pEnt->isKindOf(AcDbEllipse::desc()))
		{	//��Բ������Ӹ���ֱ��
			AcDbEllipse* pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			//
			AcDbObjectId lineId;
			AcDbLine *pLine = new AcDbLine(startPt, endPt);
			pBlockTableRecord->appendAcDbEntity(lineId, pLine);
			xAssistCirSet.SetValue(lineId.asOldId(), lineId);
			pLine->close();
			selectedEntIdSet.DeleteNode(objId.asOldId());	//��Բ���ռ�ֱ�ߴ���
			//
			Cpy_Pnt(ptS, startPt);
			Cpy_Pnt(ptE, endPt);
			double fDist = DISTANCE(ptS, ptE);
			if (fDist > fMaxEdge)
				fMaxEdge = fDist;
			if (fDist < fMinEdge)
				fMinEdge = fDist;
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{	//Բ������Ӹ���СԲ
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			//��Բ��ʼ����Ӹ���СԲ
			AcDbObjectId circleId;
			AcGeVector3d norm(0, 0, 1);
			AcDbCircle *pCircle1 = new AcDbCircle(startPt, norm, ASSIST_RADIUS);
			pBlockTableRecord->appendAcDbEntity(circleId, pCircle1);
			xAssistCirSet.SetValue(circleId.asOldId(), circleId);
			pCircle1->close();
			//�յ㴦��Ӹ���СԲ
			AcDbCircle *pCircle2 = new AcDbCircle(endPt, norm, ASSIST_RADIUS);
			pBlockTableRecord->appendAcDbEntity(circleId, pCircle2);
			xAssistCirSet.SetValue(circleId.asOldId(), circleId);
			pCircle2->close();
			//
			if (pArc->radius() > fMaxEdge)
				fMaxEdge = pArc->radius();
			if (pArc->radius() < fMinEdge)
				fMinEdge = pArc->radius();
		}
		else if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine* pLine = (AcDbLine*)pEnt;
			if (g_pncSysPara.IsBendLine(pLine, &symbols))
			{	//���˻�����
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
			//ͳ���������
			Cpy_Pnt(ptS, pLine->startPoint());
			Cpy_Pnt(ptE, pLine->endPoint());
			double fDist = DISTANCE(ptS, ptE);
			if (fDist > fMaxEdge)
				fMaxEdge = fDist;
			if (fDist < fMinEdge)
				fMinEdge = fDist;
		}
		else if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			//
			GEPOINT center;
			Cpy_Pnt(center, pCir->center());
			CString sPosKey = MakePosKeyStr(center);
			std::map<CString, CAD_ENTITY>::iterator iter;
			iter = mapCircle.find(sPosKey);
			if (iter == mapCircle.end())
			{
				mapCircle[sPosKey].ciEntType = TYPE_CIRCLE;
				mapCircle[sPosKey].idCadEnt = objId.asOldId();
				mapCircle[sPosKey].m_fSize = pCir->radius();
				mapCircle[sPosKey].pos = center;
			}
			else if (iter->second.m_fSize < pCir->radius())
			{
				selectedEntIdSet.DeleteNode(mapCircle[sPosKey].idCadEnt);
				//
				mapCircle[sPosKey].idCadEnt = objId.asOldId();
				mapCircle[sPosKey].m_fSize = pCir->radius();
			}
			else
				selectedEntIdSet.DeleteNode(objId.asOldId());
		}
	}
	pBlockTableRecord->close();
	selectedEntIdSet.Clean();
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(2, dwPreTick);
#endif
	//������ͼԪ�Ƶ��ض�ͼ��
	CHashList<CXhChar50> hashLayerList;
	for (AcDbObjectId objId = m_xAllEntIdSet.GetFirst(); objId; objId = m_xAllEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		//��¼ͼԪ��ԭʼͼ�㣬������ڸ�ԭ
		CXhChar50 sLayerName = GetEntLayerName(pEnt);
		hashLayerList.SetValue(objId.asOldId(), sLayerName);
		//�����ѡ��ʵ�壺ͳһ�Ƶ��ض�ͼ��
		if (selectedEntIdSet.GetValue(pEnt->id().asOldId()).isNull())
		{
			pEnt->setLayer(idNewLayer);
			continue;
		}
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(3, dwPreTick);
#endif
	//���ʶ��ְ���������Ϣ
	CShieldCadLayer shieldLayer(sNewLayer, TRUE, CPlateExtractor::m_bSendCommand);	//���β���Ҫ��ͼ��
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate())
	{
		//��ʼ���µ����״̬
		BOOL bIslandDetection = FALSE;
		std::map<CString, CAD_ENTITY>::iterator iter;
		for (iter = mapCircle.begin(); iter != mapCircle.end(); ++iter)
		{
			if (iter->second.IsInScope(pInfo->dim_pos))
			{
				bIslandDetection = TRUE;
				break;
			}
		}
		//ʶ��ְ�������
		pInfo->InitProfileByBPolyCmd(fMinEdge, fMaxEdge, bIslandDetection);
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(4, dwPreTick);
#endif
	//ɾ������СԲ
	for (AcDbObjectId objId = xAssistCirSet.GetFirst(); objId; objId = xAssistCirSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		pEnt->erase(Adesk::kTrue);
	}
	//��ԭͼԪ����ͼ��
	for (CXhChar50 *pLayer = hashLayerList.GetFirst(); pLayer; pLayer = hashLayerList.GetNext())
	{
		long handle = hashLayerList.GetCursorKey();
		AcDbObjectId objId = m_xAllEntIdSet.GetValue(handle);
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
#ifdef _ARX_2007
		pEnt->setLayer(_bstr_t(*pLayer));
#else
		pEnt->setLayer(*pLayer);
#endif
	}
#ifdef __TIMER_COUNT_
	timer.Relay(5, dwPreTick);
	timer.End();
	for (DWORD* pdwTickCount = timer.hashTicks.GetFirst(); pdwTickCount; pdwTickCount = timer.hashTicks.GetNext())
	{
		UINT uiTickIdentity = timer.hashTicks.GetCursorKey();
		logerr.Log("timer#%2d=%.3f", uiTickIdentity, (*pdwTickCount)*0.001);
	}
	logerr.Log("Summary time cost =%.3f", (timer.GetEndTicks() - timer.GetStartTicks())*0.001);
#endif
}
//ͨ������ģ�������ȡ�ְ�����
void CPlateExtractor::ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	CLockDocumentLife lockCurDocumentLife;
	CLogErrorLife logErrLife;
#ifdef __TIMER_COUNT_
	DWORD dwPreTick = timer.Start();
#endif
	//���˷�������ͼԪ(ͼ�顢���������������� �Լ��������ߵ�)
	CSymbolRecoginzer symbols;
	FilterInvalidEnts(selectedEntIdSet, &symbols);
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(1, dwPreTick);
#endif
	//��ȡ������Ԫ��,��¼��ѡ����
	SCOPE_STRU scope;
	AcDbEntity *pEnt = NULL;
	CHashSet<AcDbObjectId> hashScreenProfileId, hashAllLineId;
	ATOM_LIST<f3dArcLine> arrArcLine;
	int index = 1, nNum = selectedEntIdSet.GetNodeNum() * 2;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext(), index++)
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		GEPOINT ptS, ptE, ptC, norm;
		f3dArcLine arc_line;
		AcGePoint3d acad_pt;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			if (g_pncSysPara.IsBendLine(pLine, &symbols))	//���˻�����
				continue;
			pLine->getStartPoint(acad_pt);
			Cpy_Pnt(ptS, acad_pt);
			pLine->getEndPoint(acad_pt);
			Cpy_Pnt(ptE, acad_pt);
			arc_line = f3dArcLine(f3dLine(ptS, ptE));
			arc_line.feature = objId.asOldId();
			arrArcLine.append(arc_line);
			//
			scope.VerifyVertex(ptS);
			scope.VerifyVertex(ptE);
			hashAllLineId.SetValue(objId.asOldId(), objId);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			pArc->getStartPoint(acad_pt);
			Cpy_Pnt(ptS, acad_pt);
			pArc->getEndPoint(acad_pt);
			Cpy_Pnt(ptE, acad_pt);
			Cpy_Pnt(ptC, pArc->center());
			Cpy_Pnt(norm, pArc->normal());
			double radius = pArc->radius();
			double angle = (pArc->endAngle() - pArc->startAngle());
			if (radius <= 0 || fabs(angle) <= 0 || DISTANCE(ptS, ptE) < EPS)
				continue;	//������Բ��
			if (arc_line.CreateMethod3(ptS, ptE, norm, radius, ptC))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				//
				scope.VerityArcLine(arc_line);
				hashAllLineId.SetValue(objId.asOldId(), objId);
			}
			else
				logerr.Log("ArcLine error");
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{	//���¸ְ嶥�����(��Բ)
			AcDbEllipse* pEllipse = (AcDbEllipse*)pEnt;
			pEllipse->getStartPoint(acad_pt);
			Cpy_Pnt(ptS, acad_pt);
			pEllipse->getEndPoint(acad_pt);
			Cpy_Pnt(ptE, acad_pt);
			Cpy_Pnt(ptC, pEllipse->center());
			Cpy_Pnt(norm, pEllipse->normal());
			GEPOINT min_vec, maj_vec, column_norm;
			Cpy_Pnt(min_vec, pEllipse->minorAxis());
			Cpy_Pnt(maj_vec, pEllipse->majorAxis());
			double min_R = min_vec.mod();
			double maj_R = maj_vec.mod();
			double cosa = min_R / maj_R;
			double sina = SQRT(1 - cosa * cosa);
			column_norm = norm;
			RotateVectorAroundVector(column_norm, sina, cosa, min_vec);
			if (min_R <= 0 || DISTANCE(ptS, ptE) < EPS)
				continue;
			if (arc_line.CreateEllipse(ptC, ptS, ptE, column_norm, norm, min_R))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				//
				scope.VerityArcLine(arc_line);
				hashAllLineId.SetValue(objId.asOldId(), objId);
			}
			else
				logerr.Log("Ellipse error");
		}
		else if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle* pCircle = (AcDbCircle*)pEnt;
			double fRadius = pCircle->radius();
			//�򻯳��ĸ���Բ
			Cpy_Pnt(ptC, pCircle->center());
			Cpy_Pnt(norm, pCircle->normal());
			ptS.Set(ptC.x + fRadius, ptC.y, ptC.z);
			if (arc_line.CreateMethod1(ptC, ptS, norm, Pi*0.5))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				scope.VerityArcLine(arc_line);
			}
			ptS.Set(ptC.x, ptC.y + fRadius, ptC.z);
			if (arc_line.CreateMethod1(ptC, ptS, norm, Pi*0.5))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				scope.VerityArcLine(arc_line);
			}
			ptS.Set(ptC.x - fRadius, ptC.y, ptC.z);
			if (arc_line.CreateMethod1(ptC, ptS, norm, Pi*0.5))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				scope.VerityArcLine(arc_line);
			}
			ptS.Set(ptC.x, ptC.y - fRadius, ptC.z);
			if (arc_line.CreateMethod1(ptC, ptS, norm, Pi*0.5))
			{
				arc_line.feature = objId.asOldId();
				arrArcLine.append(arc_line);
				scope.VerityArcLine(arc_line);
			}
		}
		else if (pEnt->isKindOf(AcDbPolyline::desc()))
		{	//�����
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			if (pPline->isClosed())
			{	//�պϵĶ���ߣ�Ĭ��Ϊ�����ĸְ�����
				hashScreenProfileId.SetValue(objId.asOldId(), objId);
			}
			else
			{	//���պϵĶ����,����Ϊ�ְ�������һ����
				hashAllLineId.SetValue(objId.asOldId(), objId);
				int nVertNum = pPline->numVerts();
				for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
				{
					AcGePoint3d location;
					pPline->getPointAt(iVertIndex, location);
					if (pPline->segType(iVertIndex) == AcDbPolyline::kLine)
					{
						AcGeLineSeg3d acgeLine;
						pPline->getLineSegAt(iVertIndex, acgeLine);
						Cpy_Pnt(ptS, acgeLine.startPoint());
						Cpy_Pnt(ptE, acgeLine.endPoint());
						arc_line = f3dArcLine(f3dLine(ptS, ptE));
						arc_line.feature = objId.asOldId();
						arrArcLine.append(arc_line);
						scope.VerifyVertex(ptS);
						scope.VerifyVertex(ptE);
					}
					else if (pPline->segType(iVertIndex) == AcDbPolyline::kArc)
					{
						AcGeCircArc3d acgeArc;
						pPline->getArcSegAt(iVertIndex, acgeArc);
						Cpy_Pnt(ptS, acgeArc.startPoint());
						Cpy_Pnt(ptE, acgeArc.endPoint());
						Cpy_Pnt(ptC, acgeArc.center());
						Cpy_Pnt(norm, acgeArc.normal());
						double radius = acgeArc.radius();
						if (arc_line.CreateMethod3(ptS, ptE, norm, radius, ptC))
						{
							arc_line.feature = objId.asOldId();
							arrArcLine.append(arc_line);
							//
							scope.VerityArcLine(arc_line);
							hashAllLineId.SetValue(objId.asOldId(), objId);
						}
						else
							logerr.Log("ArcLine error");
					}
				}
			}
		}
		else if (pEnt->isKindOf(AcDbRegion::desc()))
		{	//����

		}
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(2, dwPreTick);
#endif
	//��ʼ������ϵ,�������߻��Ƶ����⻭��
	GECS ocs;
	ocs.origin.Set(scope.fMinX - 5, scope.fMaxY + 5, 0);
	ocs.axis_x.Set(1, 0, 0);
	ocs.axis_y.Set(0, -1, 0);
	ocs.axis_z.Set(0, 0, -1);
	CVectorMonoImage xImage;
	xImage.DisplayProcess = DisplayCadProgress;
	xImage.SetOCS(ocs, 0.2);	//g_pncSysPara.m_fPixelScale
	for (int i = 0; i < arrArcLine.GetNodeNum(); i++)
		xImage.DrawArcLine(arrArcLine[i], arrArcLine[i].feature);
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(3, dwPreTick);
#endif
	//ʶ��ְ���������
	xImage.DetectProfilePixelsByVisit();
	//xImage.DetectProfilePixelsByTrack();
#ifdef __ALFA_TEST_
	//xImage.WriteBmpFileByVertStrip("F:\\11.bmp");
	//xImage.WriteBmpFileByPixelHash("F:\\22.bmp");
#endif
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(4, dwPreTick);
#endif
	//��ȡ�ְ������㼯��
	for (OBJECT_ITEM* pItem = xImage.m_hashRelaObj.GetFirst(); pItem; pItem = xImage.m_hashRelaObj.GetNext())
	{
		if (pItem->m_nPixelNum > 1)
			hashScreenProfileId.SetValue(pItem->m_idRelaObj, MkCadObjId(pItem->m_idRelaObj));
	}
	InitPlateVextexs(hashScreenProfileId, hashAllLineId);
#ifdef __TIMER_COUNT_
	timer.Relay(5, dwPreTick);
	timer.End();
	for (DWORD* pdwTickCount = timer.hashTicks.GetFirst(); pdwTickCount; pdwTickCount = timer.hashTicks.GetNext())
	{
		UINT uiTickIdentity = timer.hashTicks.GetCursorKey();
		logerr.Log("timer#%2d =%.3f", uiTickIdentity, (*pdwTickCount)*0.001);
	}
	logerr.Log("Summary time cost =%.3f", (timer.GetEndTicks() - timer.GetStartTicks())*0.001);
#endif
}
void CPlateExtractor::InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts, CHashSet<AcDbObjectId>& hashAllLines)
{
	CHashSet<AcDbObjectId> acdbPolylineSet;
	CHashSet<AcDbObjectId> acdbArclineSet;
	CHashSet<AcDbObjectId> acdbCircleSet;
	AcDbObjectId objId;
	for (objId = hashProfileEnts.GetFirst(); objId; objId = hashProfileEnts.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (pEnt->isKindOf(AcDbPolyline::desc()))
			acdbPolylineSet.SetValue(objId.asOldId(), objId);
		else if (pEnt->isKindOf(AcDbCircle::desc()))
			acdbCircleSet.SetValue(objId.asOldId(), objId);
		else
			acdbArclineSet.SetValue(objId.asOldId(), objId);
	}
	//���ݶ������ȡ�ְ�������
	int nStep = 0, nNum = acdbPolylineSet.GetNodeNum() + acdbArclineSet.GetNodeNum() + acdbCircleSet.GetNodeNum();
	DisplayCadProgress(0, "�޶��ְ���������Ϣ.....");
	for (objId = acdbPolylineSet.GetFirst(); objId; objId = acdbPolylineSet.GetNext(), nStep++)
	{
		DisplayCadProgress(int(100 * nStep / nNum));
		CPlateProcessInfo tem_plate;
		if (!tem_plate.InitProfileByAcdbPolyLine(objId))
			continue;
		CPlateProcessInfo* pCurPlateInfo = NULL;
		for (pCurPlateInfo = EnumFirstPlate(); pCurPlateInfo; pCurPlateInfo = EnumNextPlate())
		{
			if (!tem_plate.IsInPartRgn(pCurPlateInfo->dim_pos))
				continue;
			pCurPlateInfo->EmptyVertexs();
			for (VERTEX* pVer = tem_plate.vertexList.GetFirst(); pVer;
				pVer = tem_plate.vertexList.GetNext())
				pCurPlateInfo->vertexList.append(*pVer);
			break;
		}
	}
	for (objId = acdbCircleSet.GetFirst(); objId; objId = acdbCircleSet.GetNext(), nStep++)
	{
		DisplayCadProgress(int(100 * nStep / nNum));
		CPlateProcessInfo tem_plate;
		if (!tem_plate.InitProfileByAcdbCircle(objId))
			continue;
		CPlateProcessInfo* pCurPlateInfo = NULL;
		for (pCurPlateInfo = EnumFirstPlate(); pCurPlateInfo; pCurPlateInfo = EnumNextPlate())
		{
			if (!tem_plate.IsInPartRgn(pCurPlateInfo->dim_pos))
				continue;
			pCurPlateInfo->EmptyVertexs();
			VERTEX* pVer = NULL, *pFirVer = tem_plate.vertexList.GetFirst();
			for (pVer = tem_plate.vertexList.GetFirst(); pVer; pVer = tem_plate.vertexList.GetNext())
				pCurPlateInfo->vertexList.append(*pVer);
			//Բ����Ϣ
			pCurPlateInfo->cir_plate_para.m_bCirclePlate = TRUE;
			pCurPlateInfo->cir_plate_para.cir_center = pFirVer->arc.center;
			pCurPlateInfo->cir_plate_para.m_fRadius = pFirVer->arc.radius;
			break;
		}
	}
	//��������ֱ����ȡ�ְ�������(������ȡ)
	CHashList<CAD_LINE> hashUnmatchLine;
	for (int index = 0; index < 2; index++)
	{
		ARRAY_LIST<CAD_LINE> line_arr;
		if (index == 0)
		{	//��һ�δ�ɸѡ������ͼԪ����ȡ�պ�����
			line_arr.SetSize(0, acdbArclineSet.GetNodeNum());
			for (objId = acdbArclineSet.GetFirst(); objId; objId = acdbArclineSet.GetNext())
				line_arr.append(CAD_LINE(objId.asOldId()));
		}
		else
		{	//�ڶ��δ���������ͼԪ����ȡʣ��ıպ�����
			line_arr.SetSize(0, hashAllLines.GetNodeNum());
			for (objId = hashAllLines.GetFirst(); objId; objId = hashAllLines.GetNext())
				line_arr.append(CAD_LINE(objId.asOldId()));
		}
		for (int i = 0; i < line_arr.GetSize(); i++)
			line_arr[i].UpdatePos();
		int nSize = (index == 0) ? line_arr.GetSize() : hashUnmatchLine.GetNodeNum();
		while (nSize > 0)
		{
			DisplayCadProgress(int(100 * nStep / nNum));
			CAD_LINE* pStartLine = NULL;
			if (index != 0)
				pStartLine = hashUnmatchLine.GetFirst();
			CPlateProcessInfo tem_plate;
			BOOL bSucceed = tem_plate.InitProfileByAcdbLineList(pStartLine, line_arr);
			if (bSucceed)
			{	//ƥ��ɹ�,������������Ӧ�ĸְ���
				CPlateProcessInfo* pCurPlateInfo = NULL;
				for (pCurPlateInfo = EnumFirstPlate(); pCurPlateInfo; pCurPlateInfo = EnumNextPlate())
				{
					if (!tem_plate.IsInPartRgn(pCurPlateInfo->dim_pos))
						continue;
					pCurPlateInfo->EmptyVertexs();
					VERTEX* pVer = NULL, *pFirVer = tem_plate.vertexList.GetFirst();
					for (pVer = tem_plate.vertexList.GetFirst(); pVer; pVer = tem_plate.vertexList.GetNext())
						pCurPlateInfo->vertexList.append(*pVer);
					//Բ����Ϣ
					if (tem_plate.cir_plate_para.m_bCirclePlate)
					{
						pCurPlateInfo->cir_plate_para.m_bCirclePlate = TRUE;
						pCurPlateInfo->cir_plate_para.cir_center = pFirVer->arc.center;
						pCurPlateInfo->cir_plate_para.m_fRadius = pFirVer->arc.radius;
					}
					break;
				}
			}
			//ɾ���Ѵ������ֱ��ͼԪ
			int nCount = line_arr.GetSize();
			for (int i = 0; i < nCount; i++)
			{
				if (!line_arr[i].m_bMatch)
					continue;
				if (index == 0)
				{	//��¼��һ����ȡƥ��ʧ�ܵ�������
					if (bSucceed)
						hashAllLines.DeleteNode(line_arr[i].idCadEnt);
					else
						hashUnmatchLine.SetValue(line_arr[i].idCadEnt, line_arr[i]);
				}
				else
					hashUnmatchLine.DeleteNode(line_arr[i].idCadEnt);
				//ɾ���������ͼԪ
				line_arr.RemoveAt(i);
				nCount--;
				i--;
				nStep++;
			}
			nSize = (index == 0) ? line_arr.GetSize() : hashUnmatchLine.GetNodeNum();
		}
	}
	DisplayCadProgress(100);
}
//����ְ�һ���ŵ����
void CPlateExtractor::MergeManyPartNo()
{
	for (CPlateProcessInfo* pPlateProcess = EnumFirstPlate(); pPlateProcess; pPlateProcess = EnumNextPlate())
	{
		if (!pPlateProcess->IsValid())
			continue;
		pPlateProcess->relateEntList.SetValue(pPlateProcess->partNoId.asOldId(), pPlateProcess->partNoId);
		m_pHashPlate->push_stack();
		for (CPlateProcessInfo* pTemPlate = EnumNextPlate(); pTemPlate; pTemPlate = EnumNextPlate())
		{
			if (!pPlateProcess->IsInPartRgn(pTemPlate->dim_pos))
				continue;
			pPlateProcess->relateEntList.SetValue(pTemPlate->partNoId.asOldId(), pTemPlate->partNoId);
			m_pHashPlate->DeleteCursor();
		}
		m_pHashPlate->pop_stack();
	}
	m_pHashPlate->Clean();
}
//���һ���ŵ����
void CPlateExtractor::SplitManyPartNo()
{
	for (CPlateProcessInfo* pPlateInfo = m_pHashPlate->GetFirst(); pPlateInfo; pPlateInfo = m_pHashPlate->GetNext())
	{
		if (pPlateInfo->relateEntList.GetNodeNum() <= 1)
			continue;
		m_pHashPlate->push_stack();
		for (AcDbObjectId objId = pPlateInfo->relateEntList.GetFirst(); objId; objId = pPlateInfo->relateEntList.GetNext())
		{
			if (pPlateInfo->partNoId == objId)
				continue;
			CAcDbObjLife objLife(objId);
			AcDbEntity *pEnt = objLife.GetEnt();
			if (pEnt == NULL)
				continue;
			CXhChar50 sPartNo;
			if (g_pncSysPara.ParsePartNoText(pEnt, sPartNo))
			{
				CPlateProcessInfo* pNewPlateInfo = m_pHashPlate->Add(sPartNo);
				pNewPlateInfo->xPlate.SetPartNo(sPartNo);
				pNewPlateInfo->CopyAttributes(pPlateInfo);
				pNewPlateInfo->partNoId = objId;
				pNewPlateInfo->partNumId = (g_pncSysPara.m_iDimStyle == 0) ? objId : pPlateInfo->partNumId;
			}
		}
		m_pHashPlate->pop_stack();
	}
}
//////////////////////////////////////////////////////////////////////////
//CAngleExtractor
//////////////////////////////////////////////////////////////////////////
#ifdef __UBOM_ONLY_
CAngleExtractor* CAngleExtractor::m_pExtractor = NULL;
CAngleExtractor::CAngleExtractor()
{
	m_ciType = IExtractor::ANGLE;
	m_pHashAngle = NULL;
}
CAngleExtractor* CAngleExtractor::GetExtractor()
{
	if (m_pExtractor == NULL)
		m_pExtractor = new CAngleExtractor();
	return m_pExtractor;
}
bool CAngleExtractor::ExtractAngles(CHashList<CAngleProcessInfo>& hashJgInfo,BOOL bSupportSelectEnts)
{
	CAcModuleResourceOverride resOverride;
	m_pHashAngle = &hashJgInfo;
	//
	ShiftCSToWCS(TRUE);
	//ѡ������ʵ��ͼԪ
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, bSupportSelectEnts ? FALSE : TRUE);
	//���ݽǸֹ��տ���ʶ��Ǹ�
	int index = 1, nNum = allEntIdSet.GetNodeNum() * 2;
	DisplayCadProgress(0, "ʶ��Ǹֹ��տ�.....");
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		//���ݽǸֹ��տ�����ȡ�Ǹ���Ϣ
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
		AcDbObjectId blockId = pReference->blockTableRecord();
		acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
		if (pTempBlockTableRecord == NULL)
			continue;
		pTempBlockTableRecord->close();
		CXhChar50 sName;
#ifdef _ARX_2007
		ACHAR* sValue = new ACHAR[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy((char*)_bstr_t(sValue));
		delete[] sValue;
#else
		char *sValue = new char[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy(sValue);
		delete[] sValue;
#endif
		if (!g_xUbomModel.IsJgCardBlockName(sName))
			continue;
		//��ӽǸּ�¼
		CAngleProcessInfo* pJgInfo = hashJgInfo.Add(entId.handle());
		pJgInfo->Empty();
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
	}
	//����Ǹֹ��տ����������������"����"������ȡ�Ǹ���Ϣ
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<PART_LABEL_DIM> labelDimList;
	ATOM_LIST<CAD_ENTITY> cadTextList;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			PART_LABEL_DIM *pLabelDim = labelDimList.append();
			pLabelDim->m_xCirEnt.ciEntType = TYPE_CIRCLE;
			pLabelDim->m_xCirEnt.idCadEnt = pEnt->objectId().asOldId();
			pLabelDim->m_xCirEnt.m_fSize = pCir->radius();
			Cpy_Pnt(pLabelDim->m_xCirEnt.pos, pCir->center());
			continue;
		}
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()))
			continue;
		textIdHash.SetValue(entId.handle(), entId);	//��¼�Ǹֹ��տ���ʵʱ�ı�
		//���ݼ��Źؼ���ʶ��Ǹ�
		CXhChar100 sText = GetCadTextContent(pEnt);
		GEPOINT testPt = GetCadTextDimPos(pEnt);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText *pText = (AcDbText*)pEnt;
			CAD_ENTITY *pCadText = cadTextList.append();
			pCadText->ciEntType = TYPE_TEXT;
			pCadText->idCadEnt = entId.asOldId();
			pCadText->m_fSize = pText->height();
			pCadText->pos = testPt;
			strncpy(pCadText->sText, sText, 99);
		}
		if (!g_xUbomModel.IsPartLabelTitle(sText))
			continue;	//��Ч�ļ��ű���
		CAngleProcessInfo* pJgInfo = GetAngleInfo(testPt);
		if (pJgInfo == NULL)
			pJgInfo = hashJgInfo.Add(entId.handle());
		//���½Ǹ���Ϣ wht 20-07-29
		if (pJgInfo)
		{	//��ӽǸּ�¼
			//���ݹ��տ�ģ���м��ű�ǵ����ýǸֹ��տ���ԭ��λ��
			GEPOINT orig_pt = g_pncSysPara.GetJgCardOrigin(testPt);
			pJgInfo->Empty();
			pJgInfo->keyId = entId;
			pJgInfo->m_bInJgBlock = false;
			pJgInfo->SetOrig(orig_pt);
		}
	}
	DisplayCadProgress(100);
	if (hashJgInfo.GetNodeNum() <= 0)
	{
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��", GetCurDocFile().GetBuffer());
		return false;
	}
	//���ݽǸ�����λ�û�ȡ�Ǹ���Ϣ
	index = 1;
	nNum = textIdHash.GetNodeNum();
	DisplayCadProgress(0, "��ʼ���Ǹ���Ϣ.....");
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		CXhChar50 sValue = GetCadTextContent(pEnt);
		GEPOINT text_pos = GetCadTextDimPos(pEnt);
		if (strlen(sValue) <= 0)	//���˿��ַ�
			continue;
		CAngleProcessInfo* pJgInfo = GetAngleInfo(text_pos);
		if (pJgInfo)
		{
			BYTE cType = pJgInfo->InitAngleInfo(text_pos, sValue);
			if (cType == ITEM_TYPE_SUM_PART_NUM)
				pJgInfo->partNumId = objId;
			else if (cType == ITEM_TYPE_SUM_WEIGHT)
				pJgInfo->sumWeightId = objId;
			else if (cType == ITEM_TYPE_PART_NUM)
				pJgInfo->singleNumId = objId;
			else if (cType == ITEM_TYPE_DES_MAT)
				pJgInfo->materialId = objId;
			else if (cType == ITEM_TYPE_DES_GUIGE)
				pJgInfo->specId = objId;
		}
	}
	DisplayCadProgress(100);
	if (!CAngleProcessInfo::bInitGYByGYRect)
	{	//���ݺ����߰��ʼ���Ǹֺ������� wht 20-09-29
		//��ʼ�����ű�ע��������
		for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
		{
			CAD_ENTITY *pCadText = NULL;
			for (pCadText = cadTextList.GetFirst(); pCadText; pCadText = cadTextList.GetNext())
			{
				if (pLabelDim->m_xCirEnt.IsInScope(pCadText->pos))
				{
					pLabelDim->m_xInnerText.ciEntType = pCadText->ciEntType;
					pLabelDim->m_xInnerText.idCadEnt = pCadText->idCadEnt;
					pLabelDim->m_xInnerText.m_fSize = pCadText->m_fSize;
					pLabelDim->m_xInnerText.pos = pCadText->pos;
					strcpy(pLabelDim->m_xInnerText.sText, pCadText->sText);
					break;
				}
			}
			if (pCadText == NULL)	//δ�ҵ����ֵ�ԲȦ���Ƴ�
				labelDimList.DeleteCursor();
		}
		labelDimList.Clean();
		for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
		{
			SEGI segI = pLabelDim->GetSegI();
			if (segI.iSeg <= 0)
				continue;
			CAngleProcessInfo* pJgInfo = GetAngleInfo(pLabelDim->m_xCirEnt.pos);
			if (pJgInfo && !pJgInfo->m_xAngle.bWeldPart)
				pJgInfo->m_xAngle.bWeldPart = TRUE;
		}
	}
	//����ȡ�ĽǸ���Ϣ���к����Լ��
	CHashStrList<BOOL> hashJgByPartNo;
	for (CAngleProcessInfo* pJgInfo = hashJgInfo.GetFirst(); pJgInfo; pJgInfo = hashJgInfo.GetNext())
	{
		if (pJgInfo->m_xAngle.sPartNo.GetLength() <= 0)
			hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		else
		{
			if (hashJgByPartNo.GetValue(pJgInfo->m_xAngle.sPartNo))
				logerr.Log("����(%s)�ظ�", (char*)pJgInfo->m_xAngle.sPartNo);
			else
				hashJgByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
#ifdef __ALFA_TEST_
		if (!pJgInfo->m_bInJgBlock)
			logerr.Log("(%s)�Ǹֹ��տ��Ǵ����!", (char*)pJgInfo->m_xAngle.sPartNo);
#endif
	}
	hashJgInfo.Clean();
	if (hashJgInfo.GetNodeNum() <= 0)
	{
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��", GetCurDocFile().GetBuffer());
		return false;
	}
	return true;
}
#endif