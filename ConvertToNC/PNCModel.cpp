#include "stdafx.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "LayerTable.h"
#include "PolygonObject.h"
#include "PNCCryptCoreCode.h"
#include "ComparePartNoString.h"
#include "FileIO.h"
#include "ExcelOper.h"
#include "BatchPrint.h"
#include "Expression.h"
#ifdef __TIMER_COUNT_
#include "TimerCount.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPNCModel model;
#ifdef __UBOM_ONLY_
CUbomModel g_xUbomModel;
#endif
//////////////////////////////////////////////////////////////////////////
//
int compare_func(const CXhChar16& str1, const CXhChar16& str2)
{
	CString keyStr1(str1), keyStr2(str2);
	return ComparePartNoString(keyStr1, keyStr2, "SHGPT");
}
//////////////////////////////////////////////////////////////////////////
//ExtractPlateInfo
//ExtractPlateProfile
const float CPNCModel::ASSIST_RADIUS = 1;
const float CPNCModel::DIST_ERROR = 0.5;
const float CPNCModel::WELD_MAX_HEIGHT = 20;
BOOL CPNCModel::m_bSendCommand = FALSE;
CPNCModel::CPNCModel()
{
	Empty();
}
CPNCModel::~CPNCModel()
{

}
void CPNCModel::Empty()
{
	m_hashPlateInfo.Empty();
	m_hashSolidLineTypeId.Empty();
	m_xAllEntIdSet.Empty();
	m_xAllLineHash.Empty();
	m_xBoltEntHash.Empty();
	m_sCurWorkFile.Empty();
}
//从选中的图元中剔除无效的非轮廓边图元（孤立线条、短焊缝线等）
void CPNCModel::FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols)
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
		if (mapIter != hashLineArrByPosKeyStr.end() && mapIter->second.m_fSize < CPNCModel::WELD_MAX_HEIGHT)
			selectedEntIdSet.DeleteNode(mapIter->second.idCadEnt);
	}
	selectedEntIdSet.Clean();
	//剔除螺栓的闭合区域
	for (CBoltEntGroup* pBoltGroup = m_xBoltEntHash.GetFirst();pBoltGroup; pBoltGroup = m_xBoltEntHash.GetNext(), index++)
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
CString CPNCModel::MakePosKeyStr(GEPOINT pos)
{
	CString sKeyStr;
	sKeyStr.Format("X%d-Y%d", ftoi(pos.x), ftoi(pos.y));
	return sKeyStr;
}
double CPNCModel::StandardHoleD(double fDiameter)
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
//根据图块提取螺栓
bool CPNCModel::AppendBoltEntsByBlock(ULONG idBlockEnt)
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
	SCOPE_STRU scope;
	AcDbBlockTableRecordIterator *pIterator = NULL;
	pTempBlockTableRecord->newIterator(pIterator);
	for (; !pIterator->done(); pIterator->step())
	{
		AcDbEntity *pSubEnt = NULL;
		pIterator->getEntity(pSubEnt, AcDb::kForRead);
		if (pSubEnt == NULL)
			continue;
		pSubEnt->close();
		AcDbCircle acad_cir;
		if (pSubEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pSubEnt;
			acad_cir.setCenter(pCir->center());
			acad_cir.setNormal(pCir->normal());
			acad_cir.setRadius(pCir->radius());
			VerifyVertexByCADEnt(scope, &acad_cir);
		}
		else if (pSubEnt->isKindOf(AcDbPolyline::desc()))
		{	//按照外切圆处理，多段线区域的中心为块的坐标
			AcDbPolyline *pPolyLine = (AcDbPolyline*)pSubEnt;
			if (pPolyLine->numVerts() <= 0)
				continue;
			AcGePoint3d point;
			pPolyLine->getPointAt(0, point);
			double fRadius = GEPOINT(point.x, point.y).mod();
			acad_cir.setCenter(AcGePoint3d(0, 0, 0));
			acad_cir.setNormal(AcGeVector3d(0, 0, 1));
			acad_cir.setRadius(fRadius);
			VerifyVertexByCADEnt(scope, &acad_cir);
		}
		else
			continue;
	}
	if (pIterator)
	{
		delete pIterator;
		pIterator = NULL;
	}
	double fHoleD = 0, fScale = pReference->scaleFactors().sx;
	fHoleD = max(scope.wide(), scope.high());
	fHoleD = fabs(fHoleD*fScale);
	//添加螺栓块
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
bool CPNCModel::AppendBoltEntsByCircle(ULONG idCirEnt)
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
		for (pPlateInfo = EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = EnumNextPlate(FALSE))
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
const double SQRT_2 = 1.414213562373095;
const double SQRT_3 = 1.732050807568877;
//根据多段线提取螺栓
bool CPNCModel::AppendBoltEntsByPolyline(ULONG idPolyline)
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
bool CPNCModel::AppendBoltEntsByConnectLines(vector<CAD_LINE> vectorConnLine)
{
	if (vectorConnLine.size() <= 0)
		return false;
	typedef CAD_LINE* CADLinePtr;
	std::set<CString> setKeyStr;
	std::vector<CADLinePtr> vectorArc;
	std::map<CString,CAD_LINE> mapLineByPosStr;		//
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
		up_off_vec.Set(-line_vec.y,line_vec.x,line_vec.z);
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
		//处理三角
		if (mapTriangle.count(sKey) == 3 )
		{
			ITERATOR begIter = mapTriangle.lower_bound(sKey);
			ITERATOR endIter = mapTriangle.upper_bound(sKey);
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
		//处理正方形
		if (mapSquare.count(sKey) == 4)
		{
			ITERATOR begIter = mapSquare.lower_bound(sKey);
			ITERATOR endIter = mapSquare.upper_bound(sKey);
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
bool CPNCModel::AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine)
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
			if (fabs(lineVec1*lineVec2) > EPS_COS2 && linePtM1.IsEqual(linePtM2, CPNCModel::DIST_ERROR))
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
		if(iterE!=vectorAloneLine.end())
			vectorAloneLine.erase(iterE);
		vectorAloneLine.erase(iterS);
	}
	//根据螺栓孔位置进行缩放，查找闭合线型
	for (CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetFirst(); pBoltEnt; pBoltEnt = m_xBoltEntHash.GetNext())
	{
		if(pBoltEnt->m_ciType<3)
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
		if (CPNCModel::m_bSendCommand)
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
		if(!SelCadEntSet(relaEntList,L_T,R_B))
			continue;
		//TODO:根据螺栓区域的关联图元，提取闭合线型，初始化螺栓孔径

	}
	return true;
}
void CPNCModel::ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	m_xBoltEntHash.Empty();
	//识别图块式螺栓&圆圈螺栓&多段螺栓
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		XhAcdbOpenAcDbEntity(pEnt, objId, AcDb::kForRead);
		if(pEnt==NULL)
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
void CPNCModel::ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//添加一个特定图层
	CLockDocumentLife lockCurDocumentLife;
	AcDbObjectId idSolidLine, idNewLayer;
	CXhChar16 sNewLayer("pnc_layer"),sLineType=g_pncSysPara.m_sProfileLineType;
	CreateNewLayer(sNewLayer, sLineType, AcDb::kLnWt013, 1, idNewLayer, idSolidLine);
	m_hashSolidLineTypeId.SetValue(idSolidLine.asOldId(), idSolidLine);
	AcDbObjectId lineTypeId=GetLineTypeId("人民币");
	if(lineTypeId.isValid())
		m_hashSolidLineTypeId.SetValue(lineTypeId.asOldId(),lineTypeId);
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
	double fMaxEdge=0,fMinEdge=200000;
	AcDbEntity *pEnt = NULL;
	std::map<CString, CAD_ENTITY> mapCircle;
	CHashSet<AcDbObjectId> xAssistCirSet;
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	for(AcDbObjectId objId=selectedEntIdSet.GetFirst();objId;objId=selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//按线型过滤
			AcDbObjectId lineId = GetEntLineTypeId(pEnt);
			if (lineId.isValid() && m_hashSolidLineTypeId.GetValue(lineId.asOldId()) == NULL)
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
		else if(g_pncSysPara.m_ciRecogMode==CPNCSysPara::FILTER_BY_COLOR)
		{	//按颜色过滤
			int ciColorIndex = GetEntColorIndex(pEnt);
			if (ciColorIndex != g_pncSysPara.m_ciProfileColorIndex||
				ciColorIndex == g_pncSysPara.m_ciBendLineColorIndex)
			{
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
		} 
		GEPOINT ptS, ptE;
		if(pEnt->isKindOf(AcDbEllipse::desc()))
		{	//椭圆弧处添加辅助直线
			AcDbEllipse* pEllipse=(AcDbEllipse*)pEnt;
			AcGePoint3d startPt,endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			//
			AcDbObjectId lineId;
			AcDbLine *pLine=new AcDbLine(startPt,endPt);
			pBlockTableRecord->appendAcDbEntity(lineId,pLine);
			xAssistCirSet.SetValue(lineId.asOldId(),lineId);
			pLine->close();
			selectedEntIdSet.DeleteNode(objId.asOldId());	//椭圆按照简化直线处理
			//
			Cpy_Pnt(ptS,startPt);
			Cpy_Pnt(ptE,endPt);
			double fDist=DISTANCE(ptS,ptE);
			if(fDist>fMaxEdge)
				fMaxEdge=fDist;
			if(fDist<fMinEdge)
				fMinEdge=fDist;
		}
		else if(pEnt->isKindOf(AcDbArc::desc()))
		{	//圆弧处添加辅助小圆
			AcDbArc* pArc=(AcDbArc*)pEnt;
			AcGePoint3d startPt,endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			//在圆弧始点添加辅助小圆
			AcDbObjectId circleId;
			AcGeVector3d norm(0,0,1);
			AcDbCircle *pCircle1=new AcDbCircle(startPt,norm, ASSIST_RADIUS);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle1);
			xAssistCirSet.SetValue(circleId.asOldId(),circleId);
			pCircle1->close();
			//终点处添加辅助小圆
			AcDbCircle *pCircle2=new AcDbCircle(endPt,norm, ASSIST_RADIUS);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle2);
			xAssistCirSet.SetValue(circleId.asOldId(),circleId);
			pCircle2->close();
			//
			if(pArc->radius()>fMaxEdge)
				fMaxEdge=pArc->radius();
			if(pArc->radius()<fMinEdge)
				fMinEdge=pArc->radius();
		}
		else if(pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine* pLine = (AcDbLine*)pEnt;
			if (g_pncSysPara.IsBendLine(pLine, &symbols))
			{	//过滤火曲线
				selectedEntIdSet.DeleteNode(objId.asOldId());
				continue;
			}
			//统计最长轮廓边
			Cpy_Pnt(ptS,pLine->startPoint());
			Cpy_Pnt(ptE,pLine->endPoint());
			double fDist=DISTANCE(ptS,ptE);
			if(fDist>fMaxEdge)
				fMaxEdge=fDist;
			if(fDist<fMinEdge)
				fMinEdge=fDist;
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
	dwPreTick = timer.Relay(2,dwPreTick);
#endif
	//将干扰图元移到特定图层
	CHashList<CXhChar50> hashLayerList;
	for(AcDbObjectId objId=m_xAllEntIdSet.GetFirst();objId;objId=m_xAllEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if((pEnt=objLife.GetEnt())==NULL)
			continue;
		//记录图元的原始图层，方便后期复原
		CXhChar50 sLayerName = GetEntLayerName(pEnt);
		hashLayerList.SetValue(objId.asOldId(),sLayerName);
		//处理非选中实体：统一移到特定图层
		if(selectedEntIdSet.GetValue(pEnt->id().asOldId()).isNull())
		{
			pEnt->setLayer(idNewLayer);
			continue;
		}
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(3, dwPreTick);
#endif
	//检测识别钢板轮廓边信息
	CShieldCadLayer shieldLayer(sNewLayer, TRUE, CPNCModel::m_bSendCommand);	//屏蔽不需要的图层
	for (CPlateProcessInfo* pInfo = m_hashPlateInfo.GetFirst(); pInfo; pInfo = m_hashPlateInfo.GetNext())
	{
		//初始化孤岛检测状态
		std::map<CString, CAD_ENTITY>::iterator iter;
		for (iter = mapCircle.begin(); iter!=mapCircle.end(); ++iter)
		{
			if(iter->second.IsInScope(pInfo->dim_pos))
			{
				pInfo->m_bIslandDetection = TRUE;
				break;
			}
		}
		//识别钢板轮廓边
		pInfo->InitProfileByBPolyCmd(fMinEdge, fMaxEdge, CPNCModel::m_bSendCommand);
	}
#ifdef __TIMER_COUNT_
	dwPreTick = timer.Relay(4, dwPreTick);
#endif
	//删除辅助小圆
	for(AcDbObjectId objId=xAssistCirSet.GetFirst();objId;objId=xAssistCirSet.GetNext())
	{
		CAcDbObjLife objLife(objId, TRUE);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		pEnt->erase(Adesk::kTrue);
	}
	//还原图元所在图层
	for(CXhChar50 *pLayer=hashLayerList.GetFirst();pLayer;pLayer=hashLayerList.GetNext())
	{
		long handle=hashLayerList.GetCursorKey();
		AcDbObjectId objId=m_xAllEntIdSet.GetValue(handle);
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
void CPNCModel::ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet)
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
	CHashSet<AcDbObjectId> hashScreenProfileId;
	ATOM_LIST<f3dArcLine> arrArcLine;
	int index = 1, nNum = selectedEntIdSet.GetNodeNum() * 2;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext(),index++)
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt())==NULL)
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
			m_xAllLineHash.SetValue(objId.asOldId(), objId);
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
				m_xAllLineHash.SetValue(objId.asOldId(), objId);
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
				m_xAllLineHash.SetValue(objId.asOldId(), objId);
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
				m_xAllLineHash.SetValue(objId.asOldId(), objId);
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
							m_xAllLineHash.SetValue(objId.asOldId(), objId);
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
	InitPlateVextexs(hashScreenProfileId);
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
void CPNCModel::InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts)
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
		for (pCurPlateInfo = EnumFirstPlate(FALSE); pCurPlateInfo; pCurPlateInfo = EnumNextPlate(FALSE))
		{
			if (!tem_plate.IsInPlate(pCurPlateInfo->dim_pos))
				continue;
			pCurPlateInfo->EmptyVertexs();
			for (CPlateObject::VERTEX* pVer = tem_plate.vertexList.GetFirst(); pVer;
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
		for (pCurPlateInfo = EnumFirstPlate(FALSE); pCurPlateInfo; pCurPlateInfo = EnumNextPlate(FALSE))
		{
			if (!tem_plate.IsInPlate(pCurPlateInfo->dim_pos))
				continue;
			pCurPlateInfo->EmptyVertexs();
			CPlateObject::VERTEX* pVer = NULL, *pFirVer = tem_plate.vertexList.GetFirst();
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
			line_arr.SetSize(0, m_xAllLineHash.GetNodeNum());
			for (objId = m_xAllLineHash.GetFirst(); objId; objId = m_xAllLineHash.GetNext())
				line_arr.append(CAD_LINE(objId.asOldId()));
		}
		for (int i = 0; i < line_arr.GetSize(); i++)
			line_arr[i].UpdatePos();
		int nSize = (index == 0) ? line_arr.GetSize() : hashUnmatchLine.GetNodeNum();
		while (nSize > 0)
		{
			DisplayCadProgress(int(100 * nStep / nNum));
			CAD_LINE* pStartLine = line_arr.GetFirst();
			if (index != 0)
				pStartLine = hashUnmatchLine.GetFirst();
			CPlateProcessInfo tem_plate;
			BOOL bSucceed = tem_plate.InitProfileByAcdbLineList(*pStartLine, line_arr);
			if (bSucceed)
			{	//匹配成功,将轮廓点放入对应的钢板内
				CPlateProcessInfo* pCurPlateInfo = NULL;
				for (pCurPlateInfo = EnumFirstPlate(FALSE); pCurPlateInfo; pCurPlateInfo = EnumNextPlate(FALSE))
				{
					if (!tem_plate.IsInPlate(pCurPlateInfo->dim_pos))
						continue;
					pCurPlateInfo->EmptyVertexs();
					CPlateObject::VERTEX* pVer = NULL, *pFirVer = tem_plate.vertexList.GetFirst();
					for (pVer = tem_plate.vertexList.GetFirst(); pVer;pVer = tem_plate.vertexList.GetNext())
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
						m_xAllLineHash.DeleteNode(line_arr[i].idCadEnt);
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
void CPNCModel::MergeManyPartNo()
{
	for(CPlateProcessInfo* pPlateProcess=EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=EnumNextPlate(TRUE))
	{
		if(!pPlateProcess->IsValid())
			continue;
		pPlateProcess->pnTxtIdList.SetValue(pPlateProcess->partNoId.asOldId(),pPlateProcess->partNoId);
		m_hashPlateInfo.push_stack();
		for(CPlateProcessInfo* pTemPlate=EnumNextPlate(TRUE);pTemPlate;pTemPlate=EnumNextPlate(TRUE))
		{
			if(!pPlateProcess->IsInPlate(pTemPlate->dim_pos))
				continue;
			if(pPlateProcess->m_sRelatePartNo.GetLength()<=0)
				pPlateProcess->m_sRelatePartNo.Copy(pTemPlate->GetPartNo());
			else
				pPlateProcess->m_sRelatePartNo.Append(CXhChar16(",%s",(char*)pTemPlate->GetPartNo()));
			pPlateProcess->pnTxtIdList.SetValue(pTemPlate->partNoId.asOldId(),pTemPlate->partNoId);
			m_hashPlateInfo.DeleteCursor();
		}
		m_hashPlateInfo.pop_stack();
	}
	m_hashPlateInfo.Clean();
}
//拆分一板多号的情况
void CPNCModel::SplitManyPartNo()
{
	for (CPlateProcessInfo* pPlateInfo =m_hashPlateInfo.GetFirst(); pPlateInfo; pPlateInfo = m_hashPlateInfo.GetNext())
	{
		if(pPlateInfo->pnTxtIdList.GetNodeNum() <= 1)
			continue;
		m_hashPlateInfo.push_stack();
		for (AcDbObjectId objId = pPlateInfo->pnTxtIdList.GetFirst(); objId; objId = pPlateInfo->pnTxtIdList.GetNext())
		{
			if (pPlateInfo->partNoId == objId)
				continue;
			CAcDbObjLife objLife(objId);
			AcDbEntity *pEnt = objLife.GetEnt();
			if (pEnt == NULL)
				continue;
			CXhChar16 sPartNo;
			if (g_pncSysPara.ParsePartNoText(pEnt, sPartNo))
			{
				CPlateProcessInfo* pNewPlateInfo = m_hashPlateInfo.Add(sPartNo);
				pNewPlateInfo->xPlate.SetPartNo(sPartNo);
				pNewPlateInfo->CopyAttributes(pPlateInfo);
				pNewPlateInfo->partNoId = objId;
				pNewPlateInfo->partNumId = (g_pncSysPara.m_iDimStyle == 0) ? objId : pPlateInfo->partNumId;
			}
		}
		m_hashPlateInfo.pop_stack();
	}
}
//
void CPNCModel::CreatePlatePPiFile(const char* work_path)
{
	for (CPlateProcessInfo* pPlateProcess = m_hashPlateInfo.GetFirst(); pPlateProcess; pPlateProcess = m_hashPlateInfo.GetNext())
	{	
		if (!pPlateProcess->IsValid())
			continue;
		pPlateProcess->CreatePPiFile(work_path);
	}
}
//自动排版
void CPNCModel::DrawPlatesToLayout(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f2dRect minRect;
	int minDistance = g_pncSysPara.m_nMinDistance;
	int hight = g_pncSysPara.m_nMapWidth;
	int paperLen = (g_pncSysPara.m_nMapLength <= 0) ? 100000 : g_pncSysPara.m_nMapLength;
	CSortedModel sortedModel(this,bOnlyNewExtractedPlate);
	sortedModel.DividPlatesByPartNo();
	double paperX = 0, paperY = 0;
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		CHashStrList<CDrawingRect> hashDrawingRectByLabel;
		for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
		{
			fPtList ptList;
			minRect = pPlate->GetMinWrapRect((double)minDistance, &ptList);
			CDrawingRect *pRect = hashDrawingRectByLabel.Add(pPlate->GetPartNo());
			pRect->m_pDrawing = pPlate;
			pRect->height = minRect.Height();
			pRect->width = minRect.Width();
			pRect->m_vertexArr.Empty();
			for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
				pRect->m_vertexArr.append(GEPOINT(*pPt));
			pRect->topLeft.Set(minRect.topLeft.x, minRect.topLeft.y);
		}
		paperX = 0;
		while (hashDrawingRectByLabel.GetNodeNum() > 0)
		{
			CDrawingRectLayout rectLayout;
			for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				rectLayout.drawRectArr.Add(*pRect);
			if (rectLayout.Relayout(hight, paperLen) == FALSE)
			{	//所有的板都布局失败
				for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				{
					CPlateProcessInfo *pPlateDraw = (CPlateProcessInfo*)pRect->m_pDrawing;
					if (pRect->m_bException)
						logerr.Log("钢板%s排版失败,钢板矩形宽度大于指定出图宽度!", (char*)pPlateDraw->GetPartNo());
					else
						logerr.Log("钢板%s排版失败!", (char*)pPlateDraw->GetPartNo());
				}
				break;
			}
			else
			{	//布局成功，但是其中可能某些板没有布局成功
				AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY), f3dPoint(paperX + paperLen, paperY));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY - hight), f3dPoint(paperX + paperLen, paperY - hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY), f3dPoint(paperX, paperY - hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX + paperLen, paperY), f3dPoint(paperX + paperLen, paperY - hight));
				pBlockTableRecord->close();
				f3dPoint topLeft;
				for (int i = 0; i < rectLayout.drawRectArr.GetSize(); i++)
				{
					CDrawingRect drawingRect = rectLayout.drawRectArr[i];
					CPlateProcessInfo *pPlate = (CPlateProcessInfo*)drawingRect.m_pDrawing;
					if (!drawingRect.m_bLayout)
						continue;
					topLeft = drawingRect.topLeft;
					topLeft.x += paperX;
					topLeft.y += paperY;
					GEPOINT center(topLeft.x + drawingRect.width*0.5, topLeft.y - drawingRect.height*0.5);
					pPlate->DrawPlate(&topLeft, FALSE, TRUE, &center);
					//
					hashDrawingRectByLabel.DeleteNode(pPlate->GetPartNo());
				}
				paperX += (paperLen + 50);
			}
		}
		paperY -= (hight + 50);
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
void CPNCModel::DrawPlatesToFiltrate(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f3dPoint datum_pos;
	double fSegSpace = 0;
	CSortedModel sortedModel(this, bOnlyNewExtractedPlate);
	if (g_pncSysPara.m_ciGroupType == 1)
		sortedModel.DividPlatesBySeg();			//根据段号对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 2)
		sortedModel.DividPlatesByThickMat();	//根据板厚材质对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 3)
		sortedModel.DividPlatesByMat();			//根据材质对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 4)
		sortedModel.DividPlatesByThick();		//根据板厚对钢板进行分组
	else
		sortedModel.DividPlatesByPartNo();		//根据件号对钢板进行分组
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;
		pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		datum_pos.x = 0;
		datum_pos.y += fSegSpace;
		double fRectH = pGroup->GetMaxHight() + 20;
		fSegSpace = fRectH + 50;
		for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
		{
			SCOPE_STRU scope = pPlate->GetCADEntScope();
			pPlate->InitLayoutVertex(scope, 0);
			pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
			datum_pos.x += scope.wide() + 50;
		}
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//钢板对比
void CPNCModel::DrawPlatesToCompare(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f3dPoint datum_pos;
	double fSegSpace = 0;
	CSortedModel sortedModel(this, bOnlyNewExtractedPlate);
	sortedModel.DividPlatesBySeg();			//根据段号对钢板进行分组
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;
		pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		if (g_pncSysPara.m_ciArrangeType == 0)
		{	//以行为主
			datum_pos.x = 0;
			datum_pos.y += fSegSpace;
			double fRectH = pGroup->GetMaxHight() + 20;
			fSegSpace = fRectH * 2 + 100;
			for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
			{
				SCOPE_STRU scope = pPlate->GetCADEntScope();
				pPlate->InitLayoutVertex(scope, 0);
				pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
				pPlate->DrawPlateProfile(&f3dPoint(datum_pos.x, datum_pos.y + fRectH));
				datum_pos.x += scope.wide() + 50;
			}
		}
		else
		{	//以列为主
			datum_pos.x += fSegSpace;
			datum_pos.y = 0;
			double fRectW = pGroup->GetMaxWidth() + 20;
			fSegSpace = fRectW * 2 + 500;
			for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
			{
				SCOPE_STRU scope = pPlate->GetCADEntScope();
				pPlate->InitLayoutVertex(scope, 1);
				pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
				pPlate->DrawPlateProfile(&f3dPoint(datum_pos.x + fRectW, datum_pos.y));
				datum_pos.y -= (scope.high() + 50);
			}
		}
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//下料预审
void CPNCModel::DrawPlatesToProcess(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	CSortedModel sortedModel(this, bOnlyNewExtractedPlate);
	int nSegCount = 0;
	SEGI prevSegI, curSegI;
	f3dPoint datum_pos;
	const int PAPER_WIDTH = 1500;
	//输出
	for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
	{
		CXhChar16 sPartNo = pPlate->GetPartNo();
		ParsePartNo(sPartNo, &curSegI, NULL, "SHPGT");
		CXhChar16 sSegStr = curSegI.ToString();
		if (sSegStr.GetLength() > 3)
		{	//段号字符串长度大于3时，再次从段号中提取一次分段号（处理5401-48类件号） wht 19-03-07
			SEGI segI;
			if (ParsePartNo(sSegStr, &segI, NULL, "SHPGT"))
				curSegI = segI;
		}
		f2dRect rect;
		pPlate->InitLayoutVertexByBottomEdgeIndex(rect);
		if (rect.Width() < EPS2 || rect.Height() <= EPS2)
			rect = pPlate->GetPnDimRect();
		double wide = (rect.Width() + rect.Height())*1.5;
		double high = rect.Height();
		f3dPoint leftBtm(datum_pos.x, datum_pos.y + high);
		pPlate->DrawPlate(&leftBtm, TRUE);
		datum_pos.x += wide;
		if (prevSegI.iSeg == 0)
			prevSegI = curSegI;
		else if (prevSegI.iSeg != curSegI.iSeg)
		{
			prevSegI = curSegI;
			datum_pos.x = 0;
			datum_pos.y -= PAPER_WIDTH;
		}
	}
	CXhPtrSet<CPlateProcessInfo> needUpdatePlateList;
	for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
	{
		//初始化轮廓边对应关系
		pPlate->InitEdgeEntIdMap();	
		//调整钢印位置
		if (pPlate->AutoCorrectedSteelSealPos())
			needUpdatePlateList.append(pPlate);
	}
	//更新字盒子位置之后，同步更新PPI文件中钢印号位置
	for (CPlateProcessInfo *pPlate = needUpdatePlateList.GetFirst(); pPlate; pPlate = needUpdatePlateList.GetNext())
	{	
		pPlate->SyncSteelSealPos();
		//更新PPI文件
		CString file_path;
		GetCurWorkPath(file_path);
		pPlate->CreatePPiFile(file_path);
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//钢板相关图元克隆
void CPNCModel::DrawPlatesToClone(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	DRAGSET.ClearEntSet();
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(bOnlyNewExtractedPlate); pPlate; pPlate = EnumNextPlate(bOnlyNewExtractedPlate))
		pPlate->DrawPlate();
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//
void CPNCModel::DrawPlates(BOOL bOnlyNewExtractedPlate /*= FALSE*/)
{
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PRINT)
		return DrawPlatesToLayout(bOnlyNewExtractedPlate);
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
		return DrawPlatesToProcess(bOnlyNewExtractedPlate);
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_COMPARE)
		return DrawPlatesToCompare(bOnlyNewExtractedPlate);
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_FILTRATE)
		return DrawPlatesToFiltrate(bOnlyNewExtractedPlate);
	else
		return DrawPlatesToClone(bOnlyNewExtractedPlate);
}

CPlateProcessInfo* CPNCModel::GetPlateInfo(AcDbObjectId partNoEntId)
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(FALSE); pPlate; pPlate = EnumNextPlate(FALSE))
	{
		if (pPlate->partNoId == partNoEntId)
			return pPlate;
	}
	return NULL;
}

CPlateProcessInfo* CPNCModel::GetPlateInfo(GEPOINT text_pos)
{
	CPlateProcessInfo* pPlateInfo = NULL;
	for (pPlateInfo = m_hashPlateInfo.GetFirst(); pPlateInfo; pPlateInfo = m_hashPlateInfo.GetNext())
	{
		if (pPlateInfo->IsInPlate(text_pos))
			break;
	}
	return pPlateInfo;
}

void CPNCModel::WritePrjTowerInfoToCfgFile(const char* cfg_file_path)
{
	if(cfg_file_path==NULL||strlen(cfg_file_path)<=0)
		return;
	FILE *fp=fopen(cfg_file_path,"wt");
	if(fp==NULL)
		return;
	if(m_xPrjInfo.m_sPrjName.GetLength()>0)
		fprintf(fp,"PROJECT_NAME=%s\n",(char*)m_xPrjInfo.m_sPrjName);
	if(m_xPrjInfo.m_sPrjCode.GetLength()>0)
		fprintf(fp,"PROJECT_CODE=%s\n",(char*)m_xPrjInfo.m_sPrjCode);
	if(m_xPrjInfo.m_sTaType.GetLength()>0)
		fprintf(fp,"TOWER_NAME=%s\n",(char*)m_xPrjInfo.m_sTaType);
	if(m_xPrjInfo.m_sTaAlias.GetLength()>0)
		fprintf(fp,"TOWER_CODE=%s\n",(char*)m_xPrjInfo.m_sTaAlias);
	if(m_xPrjInfo.m_sTaStampNo.GetLength()>0)
		fprintf(fp,"STAMP_NO=%s\n",(char*)m_xPrjInfo.m_sTaStampNo);
	fclose(fp);
}

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//CProjectTowerType
CProjectTowerType::CProjectTowerType()
{

}
CProjectTowerType::~CProjectTowerType()
{

}
//读取料单工程文件
void CProjectTowerType::ReadProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath, FALSE);
	double version = 1.0;
	file.Read(&version, sizeof(double));
	file.ReadString(m_sProjName);
	CXhChar100 sValue;
	file.ReadString(sValue);	//TMA料单
	InitBomInfo(sValue, TRUE);
	file.ReadString(sValue);	//ERP料单
	InitBomInfo(sValue, FALSE);
	//DWG文件信息
	DWORD n = 0;
	file.Read(&n, sizeof(DWORD));
	for (DWORD i = 0; i < n; i++)
	{
		int ibValue = 0;
		file.ReadString(sValue);
		file.Read(&ibValue, sizeof(int));
		AppendDwgBomInfo(sValue);
	}
}
//导出料单工程文件
void CProjectTowerType::WriteProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath, TRUE);
	double version = 1.0;
	file.Write(&version, sizeof(double));
	file.WriteString(m_sProjName);
	file.WriteString(m_xLoftBom.m_sFileName);	//TMA料单
	file.WriteString(m_xOrigBom.m_sFileName);	//ERP料单
	//DWG文件信息
	DWORD n = dwgFileList.GetNodeNum();
	file.Write(&n, sizeof(DWORD));
	for (CDwgFileInfo* pDwgInfo = dwgFileList.GetFirst(); pDwgInfo; pDwgInfo = dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->m_sFileName);
		int ibValue = 0;
		if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() <= 0)
			ibValue = 1;
		else if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() > 0)
			ibValue = 2;
		file.Write(&ibValue, sizeof(int));
	}
}
//从料单中读取杆塔的工程信息
BOOL CProjectTowerType::ReadTowerPrjInfo(const char* sFileName)
{
	DisplayCadProgress(0, "读取杆塔工程信息......");
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(sFileName))
	{
		DisplayCadProgress(100);
		return FALSE;
	}
	_Worksheet   excel_sheet;    // 工作表
	Sheets       excel_sheets;
	LPDISPATCH   pWorksheets = excelobj.GetWorksheets();
	ASSERT(pWorksheets != NULL);
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)1));
	excel_sheet.AttachDispatch(pWorksheet);
	VARIANT value;
	//
	DisplayCadProgress(10);
	CString sCell = g_xUbomModel.m_xMapPrjCell["工程名称"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sPrjName = VariantToString(value);
	//
	DisplayCadProgress(25);
	sCell = g_xUbomModel.m_xMapPrjCell["工程塔型"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaType = VariantToString(value);
	//
	DisplayCadProgress(40);
	sCell = g_xUbomModel.m_xMapPrjCell["材料标准"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sMatStandard = VariantToString(value);
	//
	DisplayCadProgress(55);
	sCell = g_xUbomModel.m_xMapPrjCell["下达单号"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaskNo = VariantToString(value);
	//
	DisplayCadProgress(70);
	sCell = g_xUbomModel.m_xMapPrjCell["塔规格"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaAlias = VariantToString(value);
	//
	DisplayCadProgress(80);
	sCell = g_xUbomModel.m_xMapPrjCell["合同号"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sContractNo = VariantToString(value);
	//
	DisplayCadProgress(95);
	sCell = g_xUbomModel.m_xMapPrjCell["基数"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaNum = VariantToString(value);
	//
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
	DisplayCadProgress(100);
	return TRUE;
}
//初始化BOM信息
void CProjectTowerType::InitBomInfo(const char* sFileName, BOOL bLoftBom)
{
	CXhChar100 sName;
	_splitpath(sFileName, NULL, NULL, sName, NULL);
	if (bLoftBom)
	{
		m_xLoftBom.m_sBomName.Copy(sName);
		m_xLoftBom.m_sFileName.Copy(sFileName);
		m_xLoftBom.SetBelongModel(this);
		m_xLoftBom.ImportExcelFile(&g_xBomCfg.m_xTmaTblCfg);
	}
	else
	{
		m_xOrigBom.m_sBomName.Copy(sName);
		m_xOrigBom.m_sFileName.Copy(sFileName);
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImportExcelFile(&g_xBomCfg.m_xErpTblCfg);
	}
}
//添加角钢DWGBOM信息
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName)
{
	if (strlen(sFileName) <= 0)
		return NULL;
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
	Acad::ErrorStatus eStatue;
	if (strstr(file_path, sFileName))	//激活指定文件
		eStatue = acDocManager->activateDocument(pDoc);
	else
	{		//打开指定文件
#ifdef _ARX_2007
		eStatue = acDocManager->appContextOpenDocument(_bstr_t(sFileName));
#else
		eStatue = acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	if (eStatue != Acad::eOk)
		return NULL;
	//读取DWG文件信息
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		CXhChar100 sName;
		_splitpath(sFileName, NULL, NULL, sName, NULL);
		pDwgFile = dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		pDwgFile->m_sFileName.Copy(sFileName);
		pDwgFile->m_sDwgName.Copy(sName);
	}
	return pDwgFile;
}
//删除DWG信息
void CProjectTowerType::DeleteDwgBomInfo(CDwgFileInfo* pDwgInfo)
{
	//关闭对应的DWG文件
	AcApDocument *pDoc = NULL;
	CXhChar500 file_path;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, pDwgInfo->m_sFileName))
			break;
	}
	if (pDoc)
		CloseDoc(pDoc);
	//数据模型中删除DWG信息
	for (CDwgFileInfo *pTempDwgInfo = dwgFileList.GetFirst(); pTempDwgInfo; pTempDwgInfo = dwgFileList.GetNext())
	{
		if (pTempDwgInfo == pDwgInfo)
		{
			dwgFileList.DeleteCursor();
			break;
		}
	}
	dwgFileList.Clean();
}
//
CDwgFileInfo* CProjectTowerType::FindDwgBomInfo(const char* sFileName)
{
	CDwgFileInfo* pDwgInfo = NULL;
	for (pDwgInfo = dwgFileList.GetFirst(); pDwgInfo; pDwgInfo = dwgFileList.GetNext())
	{
		if (strcmp(pDwgInfo->m_sFileName, sFileName) == 0)
			break;
	}
	return pDwgInfo;
}
//
void CProjectTowerType::CompareData(BOMPART* pSrcPart, BOMPART* pDesPart, 
				CHashStrList<BOOL> &hashBoolByPropName,BOOL bBomToBom /*= FALSE*/)
{
	hashBoolByPropName.Empty();
	//规格
	CString sSpec1(pSrcPart->GetSpec()), sSpec2(pDesPart->GetSpec());
	sSpec1 = sSpec1.Trim();
	sSpec2 = sSpec2.Trim();
	BYTE cPartType1 = pSrcPart->cPartType;
	if (pSrcPart->siSubType == BOMPART::SUB_TYPE_COMMON_PLATE)
		cPartType1 = BOMPART::PLATE;
	else if (pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_WIRE ||
		pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN ||
		pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN)
		cPartType1 = BOMPART::TUBE;
	BYTE cPartType2 = pDesPart->cPartType;
	if (pDesPart->siSubType == BOMPART::SUB_TYPE_COMMON_PLATE)
		cPartType2 = BOMPART::PLATE;
	else if (pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_WIRE ||
		pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN ||
		pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN)
		cPartType2 = BOMPART::TUBE;

	if (cPartType2 != cPartType1 ||
		sSpec1.CompareNoCase(sSpec2) != 0) //stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue(CBomConfig::KEY_SPEC, TRUE);
	//材质
	if (toupper(pSrcPart->cMaterial) != toupper(pDesPart->cMaterial))
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	else if (pSrcPart->cMaterial != pDesPart->cMaterial && !g_xUbomModel.m_bEqualH_h)
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	//质量等级
	if (g_xUbomModel.m_bCmpQualityLevel && pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	//单基数(双料单对比特殊处理)
	if (bBomToBom && pSrcPart->GetPartNum() != pDesPart->GetPartNum() &&
		(g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_SING_NUM) ||
		g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_SING_NUM)))
		hashBoolByPropName.SetValue(CBomConfig::KEY_SING_N, TRUE);
	//加工数(双料单对比特殊处理)
	if(bBomToBom && pSrcPart->nSumPart!=pDesPart->nSumPart &&
		(g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_NUM) ||
		g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_NUM)))
		hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_N, TRUE);
	//角钢定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
			hashBoolByPropName.SetValue(CBomConfig::KEY_SING_N, TRUE);
		//加工数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_N, TRUE);
		//基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_TA_NUM) &&
			pSrcPart->nTaNum != pDesPart->nTaNum)
			hashBoolByPropName.SetValue(CBomConfig::KEY_TA_NUM, TRUE);
		//总重
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_W, TRUE);
		//焊接
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_WELD, TRUE);
		//制弯
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomConfig::KEY_ZHI_WAN, TRUE);
		//长度对比
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_LEN))
		{
			if (g_xUbomModel.m_uiCustomizeSerial == ID_JiangSu_HuaDian)
			{	//江苏华电要求图纸长度大于放样长度
				if (pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue(CBomConfig::KEY_LEN, TRUE);
			}
			else
			{	//判断数值是否相等
				if (fabsl(pSrcPart->length - pDesPart->length) > g_xUbomModel.m_fMaxLenErr)
					hashBoolByPropName.SetValue(CBomConfig::KEY_LEN, TRUE);
			}
		}
		//切角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_ANGLE) &&
			pSrcJg->bCutAngle != pDesJg->bCutAngle)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_ANGLE, TRUE);
		//压扁
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_PUSH_FLAT) &&
			pSrcJg->nPushFlat != pDesJg->nPushFlat)
			hashBoolByPropName.SetValue(CBomConfig::KEY_PUSH_FLAT, TRUE);
		//铲背
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_BER) &&
			pSrcJg->bCutBer != pDesJg->bCutBer)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_BER, TRUE);
		//刨根
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_ROOT) &&
			pSrcJg->bCutRoot != pDesJg->bCutRoot)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_ROOT, TRUE);
		//开角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_KAI_JIAO) &&
			pSrcJg->bKaiJiao != pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue(CBomConfig::KEY_KAI_JIAO, TRUE);
		//合角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_HE_JIAO) &&
			pSrcJg->bHeJiao != pDesJg->bHeJiao)
			hashBoolByPropName.SetValue(CBomConfig::KEY_HE_JIAO, TRUE);
		//脚钉
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_FOOT_NAIL) &&
			pSrcJg->bHasFootNail != pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue(CBomConfig::KEY_FOO_NAIL, TRUE);
	}
	//钢板定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::PLATE)
	{
		PART_PLATE* pSrcJg = (PART_PLATE*)pSrcPart;
		PART_PLATE* pDesJg = (PART_PLATE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
		{	//青岛豪迈不比较单基数
			if (g_xUbomModel.m_uiCustomizeSerial != ID_QingDao_HaoMai)
				hashBoolByPropName.SetValue(CBomConfig::KEY_SING_N, TRUE);
		}
		//加工数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_N, TRUE);
		//总重
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_W, TRUE);
		//焊接
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_WELD, TRUE);
		//制弯
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomConfig::KEY_ZHI_WAN, TRUE);
	}
}
//比对BOM信息 0.相同 1.不同 2.文件有误
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS = 0.5;
	m_hashCompareResultByPartNo.Empty();
	if (m_xLoftBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少放样BOM信息!");
		return 2;
	}
	if (m_xOrigBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少工艺科BOM信息");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for (BOMPART *pLoftPart = m_xLoftBom.EnumFirstPart(); pLoftPart; pLoftPart = m_xLoftBom.EnumNextPart())
	{
		BOMPART *pOrgPart = m_xOrigBom.FindPart(pLoftPart->sPartNo);
		if (pOrgPart == NULL)
		{	//1.存在放样构件，不存在原始构件
			COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
			pResult->pOrgPart = NULL;
			pResult->pLoftPart = pLoftPart;
		}
		else
		{	//2. 对比同一件号构件属性
			hashBoolByPropName.Empty();
			CompareData(pLoftPart, pOrgPart, hashBoolByPropName, TRUE);
			if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
			{
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
				pResult->pOrgPart = pOrgPart;
				pResult->pLoftPart = pLoftPart;
				for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
			}
		}
	}
	//2.3 遍历导入的原始表,查找是否有漏件情况
	for (BOMPART *pPart = m_xOrigBom.EnumFirstPart(); pPart; pPart = m_xOrigBom.EnumNextPart())
	{
		if (m_xLoftBom.FindPart(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart = pPart;
		pResult->pLoftPart = NULL;
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("放样BOM和工艺科BOM信息相同!");
		return 0;
	}
	else
		return 1;
}
//进行漏号检测
int CProjectTowerType::CompareLoftAndPartDwgs(BYTE ciTypeJ0_P1_A2 /*= 2*/)
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashPartByPartNo;
	for (CDwgFileInfo* pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetAngleNum() > 0)
		{
			for (CAngleProcessInfo* pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = pDwgFile->EnumNextJg())
				hashPartByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
		if (pDwgFile->GetPlateNum() > 0)
		{
			for (CPlateProcessInfo* pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
				hashPartByPartNo.SetValue(pPlateInfo->xPlate.GetPartNo(), TRUE);
		}
	}
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (ciTypeJ0_P1_A2 == 0 && pPart->cPartType != BOMPART::ANGLE)
			continue;	//角钢漏号检测
		if (ciTypeJ0_P1_A2 == 1 && pPart->cPartType != BOMPART::PLATE)
			continue;	//钢板漏号检测
		if (hashPartByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart = NULL;
		pResult->pLoftPart = pPart;
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("已打开DWG文件没有漏号情况!");
		return 0;
	}
	else
		return 1;
}
//
int CProjectTowerType::CompareLoftAndPartDwg(const char* sFileName)
{
	const double COMPARE_EPS = 0.5;
	if (m_xLoftBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少BOM信息!");
		return 2;
	}
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		AfxMessageBox("未找到指定的角钢DWG文件!");
		return 2;
	}
	//进行比对
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	if (pDwgFile->GetAngleNum() > 0)
	{
		for (CAngleProcessInfo* pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo;
			pJgInfo = pDwgFile->EnumNextJg())
		{
			BOMPART *pDwgJg = &pJgInfo->m_xAngle;
			BOMPART *pLoftJg = m_xLoftBom.FindPart(pJgInfo->m_xAngle.sPartNo);
			if (pLoftJg == NULL)
			{	//1、存在DWG构件，不存在放样构件
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pJgInfo->m_xAngle.sPartNo);
				pResult->pOrgPart = pDwgJg;
				pResult->pLoftPart = NULL;
			}
			else
			{	//2、对比同一件号构件属性
				CompareData(pLoftJg, pDwgJg, hashBoolByPropName);
				if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
				{
					COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgJg->sPartNo);
					pResult->pOrgPart = pDwgJg;
					pResult->pLoftPart = pLoftJg;
					for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
						pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
				}
			}
		}
	}
	if (pDwgFile->GetPlateNum() > 0)
	{
		for (CPlateProcessInfo* pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo;
			pPlateInfo = pDwgFile->EnumNextPlate())
		{
			PART_PLATE* pDwgPlate = &(pPlateInfo->xBomPlate);
			BOMPART *pLoftPlate = m_xLoftBom.FindPart(pDwgPlate->sPartNo);
			if (pLoftPlate == NULL)
			{	//1、存在DWG构件，不存在放样构件
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
				pResult->pOrgPart = pDwgPlate;
				pResult->pLoftPart = NULL;
			}
			else
			{	//2、对比同一件号构件属性
				CompareData(pLoftPlate, pDwgPlate, hashBoolByPropName);
				if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
				{
					COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
					pResult->pOrgPart = pDwgPlate;
					pResult->pLoftPart = pLoftPlate;
					for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
						pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
				}
			}
		}
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("BOM信息和DWG信息相同!");
		return 0;
	}
	else
		return 1;
}
//
void CProjectTowerType::AddCompareResultSheet(LPDISPATCH pSheet, int iSheet, int iCompareType)
{
	//对校审结果进行排序
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->sPartNo);
		else
			keyStrArr.append(pResult->pOrgPart->sPartNo);
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData, keyStrArr.GetSize(), compare_func);
	//生成Excel
	if (iSheet == 1)
	{
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		excel_sheet.SetName("校审结果");
		CStringArray str_arr;
		for (size_t i = 0; i < g_xBomCfg.GetBomTitleCount(); i++)
		{
			if (g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_NOTES))
				continue;	//备注不显示
			str_arr.Add(g_xBomCfg.GetTitleName(i));
		}
		str_arr.Add("数据来源");
		int nCol = str_arr.GetSize();
		ARRAY_LIST<double> col_arr;
		col_arr.SetSize(nCol);
		for (int i = 0; i < nCol; i++)
			col_arr[i] = 10;
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr.m_pData, TRUE);
		//填充内容
		int iRow = 0;
		CVariant2dArray map(GetResultCount() * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart == NULL)
				continue;
			int iCol = 0;
			for (size_t ii = 0; ii < g_xBomCfg.GetBomTitleCount(); ii++)
			{
				if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_NOTES))
					continue;	//备注不显示
				BOOL bDiff = FALSE;
				if (pResult->hashBoolByPropName.GetValue(g_xBomCfg.GetTitleKey(ii)))
					bDiff = TRUE;
				if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_PN))
				{	//件号
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sPartNo));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_SPEC))
				{	//规格
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sSpec));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(pResult->pLoftPart->sSpec));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MAT))
				{	//材质
					map.SetValueAt(iRow, iCol, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_LEN))
				{	//长度
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_SING_N))
				{	//单基数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->GetPartNum()));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->GetPartNum()));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MANU_N))
				{	//加工数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->nSumPart));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->nSumPart));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MANU_W))
				{	//加工重量
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->fSumWeight)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->fSumWeight)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_WELD))
				{	//焊接
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->bWeldPart ? COleVariant("*") : COleVariant(""));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_ZHI_WAN))
				{	//制弯
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				}
				else if (pResult->pOrgPart->cPartType == BOMPART::ANGLE && pResult->pLoftPart->cPartType == BOMPART::ANGLE)
				{
					PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
					PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
					if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_ANGLE))
					{	//切角
						map.SetValueAt(iRow, iCol, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_PUSH_FLAT))
					{	//打扁
						map.SetValueAt(iRow, iCol, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_BER))
					{	//铲背
						map.SetValueAt(iRow, iCol, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_ROOT))
					{	//刨根
						map.SetValueAt(iRow, iCol, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_KAI_JIAO))
					{	//开角
						map.SetValueAt(iRow, iCol, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_HE_JIAO))
					{	//合角
						map.SetValueAt(iRow, iCol, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_FOO_NAIL))
					{	//带脚钉
						map.SetValueAt(iRow, iCol, pOrgJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
					}
				}
				if (bDiff)
				{	//数据不同，通过不同颜色进行标记
					CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("%C%d", 'A' + iCol, iRow + 2));
					CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("%C%d", 'A' + iCol, iRow + 3));
				}
				iCol += 1;
			}
			//数据来源
			map.SetValueAt(iRow, iCol, (iCompareType == 1) ? COleVariant(m_xOrigBom.m_sBomName) : COleVariant("DWG"));
			map.SetValueAt(iRow + 1, iCol, (iCompareType == 1) ? COleVariant(m_xLoftBom.m_sBomName) : COleVariant("Xls"));
			iRow += 2;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, iRow + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
	else if (iSheet == 2 || iSheet == 3)
	{
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		CStringArray str_arr;
		str_arr.SetSize(4);
		str_arr[0] = "件号"; str_arr[1] = "规格"; str_arr[2] = "材质"; str_arr[3] = "长度";
		double col_arr[4] = { 15,15,15,15 };
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
		if (iCompareType == COMPARE_BOM_FILE)
		{
			if (iSheet == 2)
				excel_sheet.SetName(CXhChar500("%s表多号", (char*)m_xOrigBom.m_sBomName));
			else
				excel_sheet.SetName(CXhChar100("%s表多号", (char*)m_xLoftBom.m_sBomName));
		}
		else
			excel_sheet.SetName((iSheet == 2) ? "DWG多号" : "DWG缺号");
		//填充内容
		int index = 0, nCol = 4, nResult = GetResultCount();
		CVariant2dArray map(nResult * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL)
				continue;
			BOMPART *pBomPart = NULL;
			if (iSheet == 2 && pResult->pOrgPart && pResult->pLoftPart == NULL)
				pBomPart = pResult->pOrgPart;
			if (iSheet == 3 && pResult->pLoftPart && pResult->pOrgPart == NULL)
				pBomPart = pResult->pLoftPart;
			if (pBomPart == NULL)
				continue;
			map.SetValueAt(index, 0, COleVariant(pBomPart->sPartNo));
			map.SetValueAt(index, 1, COleVariant(pBomPart->sSpec));
			map.SetValueAt(index, 2, COleVariant(CUbomModel::QueryMatMarkIncQuality(pBomPart)));
			map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pBomPart->length)));
			index++;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
}
void CProjectTowerType::AddDwgLackPartSheet(LPDISPATCH pSheet, int iCompareType)
{

	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet, FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("DWG缺少构件");
	//设置标题
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0] = "构件编号"; str_arr[1] = "设计规格"; str_arr[2] = "材质";
	str_arr[3] = "长度"; str_arr[4] = "单基数"; str_arr[5] = "加工数";
	double col_arr[6] = { 15,15,15,15,15,15 };
	CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
	//填充内容
	char cell_start[16] = "A1";
	char cell_end[16] = "A1";
	int nResult = GetResultCount();
	CVariant2dArray map(nResult * 2, 6);//获取Excel表格的范围
	int index = 0;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart)
			continue;
		map.SetValueAt(index, 0, COleVariant(pResult->pLoftPart->sPartNo));
		map.SetValueAt(index, 1, COleVariant(pResult->pLoftPart->sSpec));
		map.SetValueAt(index, 2, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
		map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pResult->pLoftPart->length)));
		map.SetValueAt(index, 4, COleVariant((long)pResult->pLoftPart->GetPartNum()));
		map.SetValueAt(index, 5, COleVariant((long)pResult->pLoftPart->nSumPart));
		index++;
	}
	_snprintf(cell_end, 15, "F%d", index + 1);
	if (index > 0)
		CExcelOper::SetRangeValue(excel_sheet, "A2", cell_end, map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", cell_end, COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet, "A1", cell_end, COleVariant(10.5));
}
//导出比较结果
void CProjectTowerType::ExportCompareResult(int iCompareType)
{
	int nSheet = 3;
	if (iCompareType == COMPARE_ALL_DWGS)
		nSheet = 1;
	if (iCompareType == COMPARE_DWG_FILE)
		nSheet = 2;
	LPDISPATCH pWorksheets = CExcelOper::CreateExcelWorksheets(nSheet);
	ASSERT(pWorksheets != NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for (int iSheet = 1; iSheet <= nSheet; iSheet++)
	{
		LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)iSheet));
		if (nSheet == 1)
			AddDwgLackPartSheet(pWorksheet, iCompareType);
		else
			AddCompareResultSheet(pWorksheet, iSheet, iCompareType);
	}
	excel_sheets.ReleaseDispatch();
}
//更新ERP料单中件号（材质为Q420,件号前加P，材质为Q345，件号前加H）
BOOL CProjectTowerType::ModifyErpBomPartNo(BYTE ciMatCharPosType)
{
	if (m_xOrigBom.m_sFileName.GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_xOrigBom.m_sFileName))
		return FALSE;
	LPDISPATCH pWorksheets = excelobj.GetWorksheets();
	if (pWorksheets == NULL)
	{
		AfxMessageBox("ERP料单文件打开失败!");
		return FALSE;
	}
	//获取Excel指定Sheet内容存储至sheetContentMap中
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)1));
	_Worksheet   excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet);
	excel_sheet.Select();
	Range excel_usedRange, excel_range;
	excel_usedRange.AttachDispatch(excel_sheet.GetUsedRange());
	excel_range.AttachDispatch(excel_usedRange.GetRows());
	long nRowNum = excel_range.GetCount();
	//excel_usedRange计算行数时会少一行，原因未知。暂时在此处增加行数 wht 20-04-24
	nRowNum += 10;
	excel_range.AttachDispatch(excel_usedRange.GetColumns());
	long nColNum = excel_range.GetCount();
	CVariant2dArray sheetContentMap(1, 1);
	CXhChar50 cell = CExcelOper::GetCellPos(nColNum, nRowNum);
	LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"), COleVariant(cell));
	excel_range.AttachDispatch(pRange);
	sheetContentMap.var = excel_range.GetValue();
	excel_usedRange.ReleaseDispatch();
	excel_range.ReleaseDispatch();
	//更新指定sheet内容
	int iPartNoCol = 1, iMartCol = 3;
	for (int i = 1; i <= sheetContentMap.RowsCount(); i++)
	{
		VARIANT value;
		CXhChar16 sPartNo, sMaterial, sNewPartNo;
		//件号
		sheetContentMap.GetValueAt(i, iPartNoCol, value);
		if (value.vt == VT_EMPTY)
			continue;
		sPartNo = VariantToString(value);
		//材质
		sheetContentMap.GetValueAt(i, iMartCol, value);
		sMaterial = VariantToString(value);
		//更新件号数据
		if (strstr(sMaterial, "Q345") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q420") && strstr(sPartNo, "P") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("P%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sP", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
	}
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
	return TRUE;
}

BOOL CProjectTowerType::ModifyTmaBomPartNo(BYTE ciMatCharPosType)
{
	ARRAY_LIST<BOMPART*> partPtrList;
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
		partPtrList.append(pPart);
	char cMaterial;
	CXhChar16 sPartNo, sMaterial, sNewPartNo;
	for (int i = 0; i < partPtrList.GetSize(); i++)
	{
		sPartNo = partPtrList[i]->sPartNo;
		cMaterial = partPtrList[i]->cMaterial;
		sMaterial = CProcessPart::QuerySteelMatMark(cMaterial);
		//更新件号数据
		if (strstr(sMaterial, "Q345") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q420") && strstr(sPartNo, "P") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("P%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sP", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
	}
	return TRUE;
}
CPlateProcessInfo *CProjectTowerType::FindPlateInfoByPartNo(const char* sPartNo)
{
	CPlateProcessInfo *pPlateInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetPlateNum() <= 0)
			continue;
		pPlateInfo = pDwgFile->FindPlateByPartNo(sPartNo);
		if (pPlateInfo)
			break;
	}
	return pPlateInfo;
}
CAngleProcessInfo *CProjectTowerType::FindAngleInfoByPartNo(const char* sPartNo)
{
	CAngleProcessInfo *pAngleInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetAngleNum() <= 0)
			continue;
		pAngleInfo = pDwgFile->FindAngleByPartNo(sPartNo);
		if (pAngleInfo)
			break;
	}
	return pAngleInfo;
}
//////////////////////////////////////////////////////////////////////////
//CBomModel
CUbomModel::CUbomModel(void)
{
	InitBomModel();
	//初始化客户信息
	m_xMapClientInfo["安徽宏源"] = ID_AnHui_HongYuan;
	m_xMapClientInfo["安徽汀阳"] = ID_AnHui_TingYang;
	m_xMapClientInfo["中电建成都铁塔"] = ID_SiChuan_ChengDu;
	m_xMapClientInfo["江苏华电"] = ID_JiangSu_HuaDian;
	m_xMapClientInfo["成都东方"] = ID_ChengDu_DongFang;
	m_xMapClientInfo["青岛豪迈"] = ID_QingDao_HaoMai;
	m_xMapClientInfo["青岛强力钢结构"] = ID_QingDao_QLGJG;
	m_xMapClientInfo["青岛载力"] = ID_QingDao_ZAILI;
	m_xMapClientInfo["五洲鼎益"] = ID_WUZHOU_DINGYI;
	m_xMapClientInfo["山东华安"] = ID_SHANDONG_HAUAN;
	m_xMapClientInfo["青岛鼎兴"] = ID_QingDao_DingXing;
	m_xMapClientInfo["青岛百斯特"] = ID_QingDao_BaiSiTe;
	m_xMapClientInfo["青岛汇金通"] = ID_QingDao_HuiJinTong;
	m_xMapClientInfo["洛阳龙羽"] = ID_LuoYang_LongYu;
	m_xMapClientInfo["江苏电装"] = ID_JiangSu_DianZhuang;
	m_xMapClientInfo["河南永光"] = ID_HeNan_YongGuang;
	m_xMapClientInfo["广东安恒"] = ID_GuangDong_AnHeng;
	m_xMapClientInfo["广东禅涛"] = ID_GuangDong_ChanTao;
	m_xMapClientInfo["重庆江电"] = ID_ChongQing_JiangDian;
	m_xMapClientInfo["青岛武晓"] = ID_QingDao_WuXiao;
	//初始化成都铁塔ERP料单中的指定单元
	m_xMapPrjCell["工程名称"] = "D1";
	m_xMapPrjCell["工程塔型"] = "F1";
	m_xMapPrjCell["材料标准"] = "H1";
	m_xMapPrjCell["下达单号"] = "L1";
	m_xMapPrjCell["塔规格"] = "J1";
	m_xMapPrjCell["合同号"] = "B1";
	m_xMapPrjCell["基数"] = "N1";

}
CUbomModel::~CUbomModel(void)
{

}
void CUbomModel::InitBomModel()
{
	m_dwFunctionFlag = 0;
	m_uiCustomizeSerial = 0;
	m_bExeRppWhenArxLoad = TRUE;
	m_bExtractPltesWhenOpenFile = TRUE;
	m_bExtractAnglesWhenOpenFile = TRUE;
	m_fMaxLenErr = 0.5;
	m_bCmpQualityLevel = TRUE;
	m_bEqualH_h = FALSE;
	m_sJgCadName.Empty();
	m_sJgCadPartLabel.Empty();
	m_sJgCardBlockName.Empty();
	m_sNotPrintFilter.Empty();
	m_ciPrintSortType = 0;
	m_uiJgCadPartLabelMat = 0;
	//
	m_xPrjTowerTypeList.Empty();
	CProjectTowerType* pProject = m_xPrjTowerTypeList.Add(0);
	pProject->m_sProjName.Copy("新建工程");
}
bool CUbomModel::IsValidFunc(int iFuncType)
{
	if (iFuncType > 0)
		return (m_dwFunctionFlag&GetSingleWord(iFuncType)) > 0;
	else
		return false;
}
CXhChar16 CUbomModel::QueryMatMarkIncQuality(BOMPART *pPart)
{
	CXhChar16 sMatMark;
	if (pPart)
	{
		sMatMark = CProcessPart::QuerySteelMatMark(pPart->cMaterial);
		if (pPart->cQualityLevel != 0 && !sMatMark.EqualNoCase(pPart->sMaterial))
			sMatMark.Append(pPart->cQualityLevel);
	}
	return sMatMark;
}
BOOL CUbomModel::IsNeedPrint(BOMPART *pPart, const char* sNotes)
{
	if (m_sNotPrintFilter.GetLength() <= 0)
		return TRUE;
	CXhChar500 sFilter(m_sNotPrintFilter);
	sFilter.Replace("&&", ",");
	CExpression expression;
	EXPRESSION_VAR *pVar = expression.varList.Append();
	strcpy(pVar->variableStr, "WIDTH");
	pVar->fValue = pPart->wide;
	pVar = expression.varList.Append();
	strcpy(pVar->variableStr, "NOTES");
	if (sNotes != NULL)
		pVar->fValue = strlen(sNotes);
	else
		pVar->fValue = strlen(pPart->sNotes);

	bool bRetCode = true;
	//1.将过滤条件拆分为多个逻辑表达式，目前只支持&&
	for (char* sKey = strtok(sFilter, ","); sKey; sKey = strtok(NULL, ","))
	{	//2.逐个解析表达式
		if (!expression.SolveLogicalExpression(sKey))
		{
			bRetCode = false;
			break;
		}
	}
	return !bRetCode;
}
BOOL CUbomModel::IsJgCardBlockName(const char* sBlockName)
{
	if (sBlockName != NULL && strcmp(sBlockName, "JgCard") == 0)
		return TRUE;
	else if (m_sJgCardBlockName.GetLength() > 0 && m_sJgCardBlockName.EqualNoCase(sBlockName))
		return TRUE;
	else
		return false;
}
BOOL CUbomModel::IsPartLabelTitle(const char* sText)
{
	if (m_sJgCadPartLabel.GetLength() > 0)
	{
		CString ss1(sText), ss2(m_sJgCadPartLabel);
		ss1 = ss1.Trim();
		ss2 = ss2.Trim();
		ss1.Remove(' ');
		ss2.Remove(' ');
		if (ss2.GetLength() > 1 && ss1.CompareNoCase(ss2) == 0)
			return true;
		else
			return false;
	}
	else
	{
		if (!(strstr(sText, "件") && strstr(sText, "号")) &&	//件号、零件编号
			!(strstr(sText, "编") && strstr(sText, "号")))		//编号、构件编号
			return false;
		if (strstr(sText, "文件") != NULL)
			return false;	//排除"文件编号"导致的提取错误 wht 19-05-13
		if (strstr(sText, ":") != NULL || strstr(sText, "：") != NULL)
			return false;	//排除钢板标注中的件号，避免将钢板错误提取为角钢 wht 20-07-29
		return true;
	}
}
#endif
