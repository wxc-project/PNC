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
	//对计算得到的孔径进行圆整，精确到小数点一位
	double fHoleD = fDiameter;
	int nValue = (int)floor(fHoleD);	//整数部分
	double fValue = fHoleD - nValue;	//小数部分
	if (fValue < EPS2)	//孔径为整数
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
	//调整坐标系
	ShiftCSToWCS(TRUE);
	//框选图元
	CHashSet<AcDbObjectId> selectedEntList;
	if (bSupportSelectEnts)
	{	//进行手动框选
		//非智能提取轮廓时，需要选择所有的图形方便后期的过滤使用
		if (g_pncSysPara.m_ciRecogMode != CPNCSysPara::FILTER_BY_PIXEL)
			SelCadEntSet(m_xAllEntIdSet, TRUE);
		if (!SelCadEntSet(selectedEntList))
			return false;
	}
	else
	{	//处理所有图元
		SelCadEntSet(m_xAllEntIdSet, TRUE);
		for (AcDbObjectId entId = m_xAllEntIdSet.GetFirst(); entId; entId = m_xAllEntIdSet.GetNext())
			selectedEntList.SetValue(entId.asOldId(), entId);
	}
	//从框选信息中提取中钢板的标识，统计钢板集合
	CHashSet<AcDbObjectId> textIdHash;
	AcDbEntity *pEnt = NULL;
	int index = 1, nNum = selectedEntList.GetNodeNum();
	DisplayCadProgress(0, "查找图纸文字标注,识别钢板件号信息.....");
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
				logerr.Log("钢板信息{%s}满足设置的文字识别规则，但识别失败请联系信狐客服!", (char*)sText);
				continue;
			}
			else
				dim_pos = GetCadTextDimPos(pEnt, &dim_vec);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//从块中解析钢板信息
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
				logerr.Log("通过图块识别钢板基本信息,识别失败请联系信狐客服!");
				continue;
			}
		}
		CPlateProcessInfo *pExistPlate = hashPlateInfo.GetValue(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//件号相同，但件号文本对应的实体不相同提示件号重复 wht 19-07-22
			pExistPlate->repeatEntList.SetValue(entId.asOldId(), entId);
			logerr.Log("件号{%s}有重复请确认!", (char*)sPartNo);
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
		AfxMessageBox("未选择提取内容，无法执行提取操作！");
		return false;
	}
	else if (hashPlateInfo.GetNodeNum() <= 0)
	{
		if (AfxMessageBox("该文件不满足文字识别配置！是否调整文字识别配置？", MB_YESNO) == IDYES)
		{
			CAcModuleResourceOverride resOverride;
			CPNCSysSettingDlg dlg;
			dlg.m_iSelTabGroup = 1;
			dlg.DoModal();
		}
		return false;
	}
	//提取钢板的螺栓孔（螺栓块、圆、三角、矩形、腰圆）
	ExtractPlateBoltEnts(selectedEntList);
	//提取钢板的轮廓边
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		ExtractPlateProfileEx(selectedEntList);
	else
		ExtractPlateProfile(selectedEntList);
#ifndef __UBOM_ONLY_
	//处理钢板一板多号的情况
	MergeManyPartNo();
	//根据轮廓闭合区域更新钢板的基本信息+螺栓信息+轮廓边信息
	int nSum = 0;
	nNum = hashPlateInfo.GetNodeNum();
	DisplayCadProgress(0, "修订钢板信息<基本+螺栓+火曲>.....");
	for (CPlateProcessInfo* pPlateProcess = hashPlateInfo.GetFirst(); pPlateProcess; pPlateProcess = hashPlateInfo.GetNext(), nSum++)
	{
		DisplayCadProgress(int(100 * nSum / nNum));
		pPlateProcess->ExtractRelaEnts();
		pPlateProcess->CheckProfileEdge();
		pPlateProcess->UpdateBoltHoles();
		if (!pPlateProcess->UpdatePlateInfo())
			logerr.Log("件号%s板选择了错误的边界,请重新选择.(位置：%s)", (char*)pPlateProcess->GetPartNo(), (char*)CXhChar50(pPlateProcess->dim_pos));
		if (pPlateProcess->IsValid())
			pPlateProcess->InitPPiInfo();
	}
	DisplayCadProgress(100);
	//将提取的钢板信息导出到中性文件中
	if (g_pncSysPara.m_iPPiMode == 0)
		SplitManyPartNo();
#else
	//UBOM只需要更新钢板的基本信息
	for (CPlateProcessInfo* pPlateProcess = EnumFirstPlate(); pPlateProcess; pPlateProcess = EnumNextPlate())
	{
		if (!pPlateProcess->IsValid())
		{
			pPlateProcess->CreateRgnByText();
#ifdef __ALFA_TEST_
			logerr.Log("(%s)钢板的轮廓点提取失败!", (char*)pPlateProcess->GetPartNo());
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
		{	//处理带电焊图块的钢板
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
		//焊接字样可能在基本信息中例如：141#正焊
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
		pPlateInfo->xBomPlate.nSumPart = pPlateInfo->xPlate.m_nProcessNum;	//加工数
		pPlateInfo->xBomPlate.AddPart(pPlateInfo->xPlate.m_nSingleNum);		//单基数
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("钢板%s信息提取失败!", (char*)pPlateInfo->xBomPlate.sPartNo);
		else
			pPlateInfo->xBomPlate.sSpec.Printf("-%.f", pPlateInfo->xBomPlate.thick);
	}
#endif
	return true;
}
//从选中的图元中剔除无效的非轮廓边图元（孤立线条、短焊缝线等）
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
		ptS.z = ptE.z = 0;	//手动初始化为0，防止点坐标的Z值无限小时，CXhChar50构造函数有的处理为0.0，有的处理为 -0.0
		double len = DISTANCE(ptS, ptE);
		setKeyStr.insert(MakePosKeyStr(ptS));
		setKeyStr.insert(MakePosKeyStr(ptE));
		hashLineArrByPosKeyStr.insert(std::make_pair(MakePosKeyStr(ptS), CAD_LINE(objId, len)));
		hashLineArrByPosKeyStr.insert(std::make_pair(MakePosKeyStr(ptE), CAD_LINE(objId, len)));
	}
	//剔除非轮廓边图元，记录火曲线特征信息
	int index = 1, nNum = selectedEntIdSet.GetNodeNum() + setKeyStr.size() + m_xBoltEntHash.GetNodeNum();
	DisplayCadProgress(0, "剔除无效的非轮廓边图元.....");
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
			//确认样条曲线为火曲线标识后，再从选择集中移除，否则样条曲线椭圆弧无法识别 wht 20-07-18
			AcDbSpline *pSpline = (AcDbSpline*)pEnt;
			if (pSpline->numControlPoints() <= 6 && pSpline->numFitPoints() <= 4)
				selectedEntIdSet.DeleteNode(objId.asOldId());
		}
		else if (!g_pncSysPara.IsProfileEnt(pEnt))
			selectedEntIdSet.DeleteNode(objId.asOldId());
	}
	selectedEntIdSet.Clean();
	//剔除孤立线条以及焊缝线
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
	//剔除螺栓的闭合区域
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
//根据图块提取螺栓
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
		return false;	//非设置的螺栓图块
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
		{	//按照外切圆处理，多段线区域的中心为块的坐标
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
		{	//直线段(处理三角|方形)
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
		logerr.Log("螺栓图块(%s),计算螺栓直径失败!", (char*)sName);
		return false;
	}
	//添加螺栓块
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
//根据圆圈提取螺栓
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
		return false;	//去除点
	GEPOINT center(pCircle->center().x, pCircle->center().y, pCircle->center().z);
	double fDiameter = pCircle->radius() * 2;
	if (g_pncSysPara.m_ciMKPos == 2 &&
		fabs(g_pncSysPara.m_fMKHoleD - fDiameter) < EPS2)
		return false;	//去除号位孔
	if (fDiameter > g_pncSysPara.m_nMaxHoleD)
	{	//超过最大圆直径，判断是否按环板处理
		CPlateProcessInfo* pPlateInfo = NULL;
		for (pPlateInfo = EnumFirstPlate(); pPlateInfo; pPlateInfo = EnumNextPlate())
		{
			double fDist = DISTANCE(pPlateInfo->dim_pos, center);
			if (fDist < pCircle->radius())
				return false;	//圆内有件号标注，默认为环板
		}
	}
	//添加螺栓块
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
//根据多段线提取螺栓
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
		//添加螺栓块
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
		{	//方形
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
		{	//腰圆
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
		//添加螺栓块
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
//根据连接短线段提取螺栓
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
	//根据每条线段计算出构成三角形和正方形后的中心点位置
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
			//将直线的始终端坐标作为键值记录直线
			CString sPosKey;
			sPosKey.Format("S(%d,%d)E(%d,%d)", ftoi(ptS.x), ftoi(ptS.y), ftoi(ptE.x), ftoi(ptE.y));
			mapLineByPosStr.insert(std::make_pair(sPosKey, vectorConnLine[i]));
			//计算该直线组成三角形后的中心点位置
			CAD_ENTITY xEntity;
			xEntity.idCadEnt = vectorConnLine[i].idCadEnt;
			xEntity.m_fSize = fLen / SQRT_3 * 2;
			xEntity.pos = ptM + up_off_vec * (0.5*fLen / SQRT_3);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapTriangle.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
			xEntity.pos = ptM + dw_off_vec * (0.5*fLen / SQRT_3);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapTriangle.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
			//计算该直线组成正方形后的中心点位置
			xEntity.m_fSize = fLen * SQRT_2;
			xEntity.pos = ptM + up_off_vec * 0.5*fLen;
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapSquare.insert(std::make_pair(MakePosKeyStr(xEntity.pos), vectorConnLine[i]));
			xEntity.pos = ptM + dw_off_vec * 0.5*fLen;
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapSquare.insert(std::make_pair(MakePosKeyStr(xEntity.pos), vectorConnLine[i]));
		}
		else if (vectorConnLine[i].ciEntType == TYPE_ARC)
		{	//计算腰圆中心点
			vectorArc.push_back(&vectorConnLine[i]);
		}
	}
	//提取出符合要求的三角形和方形直线段
	std::set<CString>::iterator set_iter;
	for (set_iter = setKeyStr.begin(); set_iter != setKeyStr.end(); ++set_iter)
	{
		CString sKey = *set_iter;
		ITERATOR begIter = mapTriangle.lower_bound(sKey);
		ITERATOR endIter = mapTriangle.upper_bound(sKey);
		//处理三角螺栓（目前只处理标准螺栓孔径）
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
		//处理正方形（目前只处理标准螺栓孔径）
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
	//处理腰圆图形，将所有的半圆弧进行配对处理
	while (!vectorArc.empty())
	{
		CADLinePtr headArc = vectorArc.at(0);
		if (headArc->m_bMatch)
		{	//已配对成功
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
		{	//找到配对的半圆弧
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
	//补齐椭圆弧的过渡直线段
	for (CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetFirst(); pBoltEnt; pBoltEnt = m_xBoltEntHash.GetNext())
	{
		if (pBoltEnt->m_ciType != CBoltEntGroup::BOLT_WAIST_ROUND)
			continue;
		if (pBoltEnt->m_xLineArr.size() == 2 && pBoltEnt->m_xArcArr.size() == 2)
			continue;	//已处理
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
		//查找过渡直线
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
//根据孤立短线段提取螺栓
bool CPlateExtractor::AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine)
{
	if (vectorAloneLine.size() <= 0)
		return false;
	for (size_t i = 0; i < vectorAloneLine.size(); i++)
		vectorAloneLine[i].UpdatePos();
	//根据十字交叉线段交点确定螺栓
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
			{	//两线段十字相交且交于中点
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
		//删除处理过的图元
		if (iterE != vectorAloneLine.end())
			vectorAloneLine.erase(iterE);
		vectorAloneLine.erase(iterS);
	}
	//根据螺栓孔位置进行缩放，查找闭合线型
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
		//TODO:根据螺栓区域的关联图元，提取闭合线型，初始化螺栓孔径

	}
	return true;
}
void CPlateExtractor::ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//识别图块式螺栓&圆圈螺栓&多段螺栓
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
	//识别打散以后的多段线螺栓
	if (g_pncSysPara.IsRecongPolylineLs())
	{	//获取短线段集合
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
				continue;	//过滤过长线段
			if (pEnt->isKindOf(AcDbArc::desc()))
			{	//
				AcDbArc* pArc = (AcDbArc*)pEnt;
				double fAngle = fabs(pArc->endAngle() - pArc->startAngle());
				if (fabs(fAngle - Pi) > 0.1)
					continue;	//非180度的圆弧
			}
			vectorLine.push_back(CAD_LINE(objId, ptS, ptE));
			mapLineNumInPos[MakePosKeyStr(ptS)] += 1;
			mapLineNumInPos[MakePosKeyStr(ptE)] += 1;
		}
		//获取始终端都有连接的线段结合
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
//根据bpoly命令初始化钢板的轮廓边
void CPlateExtractor::ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//添加一个特定图层
	CLockDocumentLife lockCurDocumentLife;
	CHashSet<AcDbObjectId> hashSolidLineTypeId;	//记录有效的实体线型id wht 19-01-03
	AcDbObjectId idSolidLine, idNewLayer;
	CXhChar16 sNewLayer("pnc_layer"), sLineType = g_pncSysPara.m_sProfileLineType;
	CreateNewLayer(sNewLayer, sLineType, AcDb::kLnWt013, 1, idNewLayer, idSolidLine);
	hashSolidLineTypeId.SetValue(idSolidLine.asOldId(), idSolidLine);
	AcDbObjectId lineTypeId = GetLineTypeId("人民币");
	if (lineTypeId.isValid())
		hashSolidLineTypeId.SetValue(lineTypeId.asOldId(), lineTypeId);
#ifdef __TIMER_COUNT_
	DWORD dwPreTick = timer.Start();
#endif
	//过滤非轮廓边图元(图块、孤立线条、焊缝线 以及样条曲线等)
	CSymbolRecoginzer symbols;
	FilterInvalidEnts(selectedEntIdSet, &symbols);
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(1, dwPreTick);
#endif
	//根据轮廓边识别规则过滤不满足条件的图元，并在圆弧或椭圆弧处添加辅助图元
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
		{	//按线型过滤
			AcDbObjectId lineId = GetEntLineTypeId(pEnt);
			if (lineId.isValid() && hashSolidLineTypeId.GetValue(lineId.asOldId()) == NULL)
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//按图层过滤
			CXhChar50 sLayerName = GetEntLayerName(pEnt);
			if (g_pncSysPara.IsNeedFilterLayer(sLayerName))
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_COLOR)
		{	//按颜色过滤
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
		{	//椭圆弧处添加辅助直线
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
			selectedEntIdSet.DeleteNode(objId.asOldId());	//椭圆按照简化直线处理
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
		{	//圆弧处添加辅助小圆
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			//在圆弧始点添加辅助小圆
			AcDbObjectId circleId;
			AcGeVector3d norm(0, 0, 1);
			AcDbCircle *pCircle1 = new AcDbCircle(startPt, norm, ASSIST_RADIUS);
			pBlockTableRecord->appendAcDbEntity(circleId, pCircle1);
			xAssistCirSet.SetValue(circleId.asOldId(), circleId);
			pCircle1->close();
			//终点处添加辅助小圆
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
			{	//过滤火曲线
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
			//统计最长轮廓边
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
	//将干扰图元移到特定图层
	CHashList<CXhChar50> hashLayerList;
	for (AcDbObjectId objId = m_xAllEntIdSet.GetFirst(); objId; objId = m_xAllEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		//记录图元的原始图层，方便后期复原
		CXhChar50 sLayerName = GetEntLayerName(pEnt);
		hashLayerList.SetValue(objId.asOldId(), sLayerName);
		//处理非选中实体：统一移到特定图层
		if (selectedEntIdSet.GetValue(pEnt->id().asOldId()).isNull())
		{
			pEnt->setLayer(idNewLayer);
			continue;
		}
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(3, dwPreTick);
#endif
	//检测识别钢板轮廓边信息
	CShieldCadLayer shieldLayer(sNewLayer, TRUE, CPlateExtractor::m_bSendCommand);	//屏蔽不需要的图层
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate())
	{
		//初始化孤岛检测状态
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
		//识别钢板轮廓边
		pInfo->InitProfileByBPolyCmd(fMinEdge, fMaxEdge, bIslandDetection);
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(4, dwPreTick);
#endif
	//删除辅助小圆
	for (AcDbObjectId objId = xAssistCirSet.GetFirst(); objId; objId = xAssistCirSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		pEnt->erase(Adesk::kTrue);
	}
	//还原图元所在图层
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
//通过像素模拟进行提取钢板轮廓
void CPlateExtractor::ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	CLockDocumentLife lockCurDocumentLife;
	CLogErrorLife logErrLife;
#ifdef __TIMER_COUNT_
	DWORD dwPreTick = timer.Start();
#endif
	//过滤非轮廓边图元(图块、孤立线条、焊缝线 以及样条曲线等)
	CSymbolRecoginzer symbols;
	FilterInvalidEnts(selectedEntIdSet, &symbols);
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(1, dwPreTick);
#endif
	//提取轮廓边元素,记录框选区域
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
			if (g_pncSysPara.IsBendLine(pLine, &symbols))	//过滤火曲线
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
				continue;	//不合理圆弧
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
		{	//更新钢板顶点参数(椭圆)
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
			//简化成四个半圆
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
		{	//多段线
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			if (pPline->isClosed())
			{	//闭合的多段线，默认为完整的钢板轮廓
				hashScreenProfileId.SetValue(objId.asOldId(), objId);
			}
			else
			{	//不闭合的多段线,可能为钢板轮廓的一部分
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
		{	//面域

		}
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(2, dwPreTick);
#endif
	//初始化坐标系,将轮廓线绘制到虚拟画布
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
	//识别钢板外轮廓边
	xImage.DetectProfilePixelsByVisit();
	//xImage.DetectProfilePixelsByTrack();
#ifdef __ALFA_TEST_
	//xImage.WriteBmpFileByVertStrip("F:\\11.bmp");
	//xImage.WriteBmpFileByPixelHash("F:\\22.bmp");
#endif
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(4, dwPreTick);
#endif
	//提取钢板轮廓点集合
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
	//根据多段线提取钢板轮廓点
	int nStep = 0, nNum = acdbPolylineSet.GetNodeNum() + acdbArclineSet.GetNodeNum() + acdbCircleSet.GetNodeNum();
	DisplayCadProgress(0, "修订钢板轮廓点信息.....");
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
			//圆环信息
			pCurPlateInfo->cir_plate_para.m_bCirclePlate = TRUE;
			pCurPlateInfo->cir_plate_para.cir_center = pFirVer->arc.center;
			pCurPlateInfo->cir_plate_para.m_fRadius = pFirVer->arc.radius;
			break;
		}
	}
	//根据轮廓直线提取钢板轮廓点(两次提取)
	CHashList<CAD_LINE> hashUnmatchLine;
	for (int index = 0; index < 2; index++)
	{
		ARRAY_LIST<CAD_LINE> line_arr;
		if (index == 0)
		{	//第一次从筛选的轮廓图元中提取闭合外形
			line_arr.SetSize(0, acdbArclineSet.GetNodeNum());
			for (objId = acdbArclineSet.GetFirst(); objId; objId = acdbArclineSet.GetNext())
				line_arr.append(CAD_LINE(objId.asOldId()));
		}
		else
		{	//第二次从所有轮廓图元中提取剩余的闭合外形
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
			{	//匹配成功,将轮廓点放入对应的钢板内
				CPlateProcessInfo* pCurPlateInfo = NULL;
				for (pCurPlateInfo = EnumFirstPlate(); pCurPlateInfo; pCurPlateInfo = EnumNextPlate())
				{
					if (!tem_plate.IsInPartRgn(pCurPlateInfo->dim_pos))
						continue;
					pCurPlateInfo->EmptyVertexs();
					VERTEX* pVer = NULL, *pFirVer = tem_plate.vertexList.GetFirst();
					for (pVer = tem_plate.vertexList.GetFirst(); pVer; pVer = tem_plate.vertexList.GetNext())
						pCurPlateInfo->vertexList.append(*pVer);
					//圆环信息
					if (tem_plate.cir_plate_para.m_bCirclePlate)
					{
						pCurPlateInfo->cir_plate_para.m_bCirclePlate = TRUE;
						pCurPlateInfo->cir_plate_para.cir_center = pFirVer->arc.center;
						pCurPlateInfo->cir_plate_para.m_fRadius = pFirVer->arc.radius;
					}
					break;
				}
			}
			//删除已处理过的直线图元
			int nCount = line_arr.GetSize();
			for (int i = 0; i < nCount; i++)
			{
				if (!line_arr[i].m_bMatch)
					continue;
				if (index == 0)
				{	//记录第一次提取匹配失败的轮廓线
					if (bSucceed)
						hashAllLines.DeleteNode(line_arr[i].idCadEnt);
					else
						hashUnmatchLine.SetValue(line_arr[i].idCadEnt, line_arr[i]);
				}
				else
					hashUnmatchLine.DeleteNode(line_arr[i].idCadEnt);
				//删除处理过的图元
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
//处理钢板一板多号的情况
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
//拆分一板多号的情况
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
	//选择所有实体图元
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, bSupportSelectEnts ? FALSE : TRUE);
	//根据角钢工艺卡块识别角钢
	int index = 1, nNum = allEntIdSet.GetNodeNum() * 2;
	DisplayCadProgress(0, "识别角钢工艺卡.....");
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		//根据角钢工艺卡块提取角钢信息
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
		//添加角钢记录
		CAngleProcessInfo* pJgInfo = hashJgInfo.Add(entId.handle());
		pJgInfo->Empty();
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
	}
	//处理角钢工艺卡块打碎的情况：根据"件号"标题提取角钢信息
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
		textIdHash.SetValue(entId.handle(), entId);	//记录角钢工艺卡的实时文本
		//根据件号关键字识别角钢
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
			continue;	//无效的件号标题
		CAngleProcessInfo* pJgInfo = GetAngleInfo(testPt);
		if (pJgInfo == NULL)
			pJgInfo = hashJgInfo.Add(entId.handle());
		//更新角钢信息 wht 20-07-29
		if (pJgInfo)
		{	//添加角钢记录
			//根据工艺卡模板中件号标记点计算该角钢工艺卡的原点位置
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
		logerr.Log("%s文件提取角钢失败", GetCurDocFile().GetBuffer());
		return false;
	}
	//根据角钢数据位置获取角钢信息
	index = 1;
	nNum = textIdHash.GetNodeNum();
	DisplayCadProgress(0, "初始化角钢信息.....");
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		CXhChar50 sValue = GetCadTextContent(pEnt);
		GEPOINT text_pos = GetCadTextDimPos(pEnt);
		if (strlen(sValue) <= 0)	//过滤空字符
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
	{	//根据焊接肋板初始化角钢焊接属性 wht 20-09-29
		//初始化件号标注文字内容
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
			if (pCadText == NULL)	//未找到文字的圆圈需移除
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
	//对提取的角钢信息进行合理性检查
	CHashStrList<BOOL> hashJgByPartNo;
	for (CAngleProcessInfo* pJgInfo = hashJgInfo.GetFirst(); pJgInfo; pJgInfo = hashJgInfo.GetNext())
	{
		if (pJgInfo->m_xAngle.sPartNo.GetLength() <= 0)
			hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		else
		{
			if (hashJgByPartNo.GetValue(pJgInfo->m_xAngle.sPartNo))
				logerr.Log("件号(%s)重复", (char*)pJgInfo->m_xAngle.sPartNo);
			else
				hashJgByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
#ifdef __ALFA_TEST_
		if (!pJgInfo->m_bInJgBlock)
			logerr.Log("(%s)角钢工艺卡是打碎的!", (char*)pJgInfo->m_xAngle.sPartNo);
#endif
	}
	hashJgInfo.Clean();
	if (hashJgInfo.GetNodeNum() <= 0)
	{
		logerr.Log("%s文件提取角钢失败", GetCurDocFile().GetBuffer());
		return false;
	}
	return true;
}
#endif