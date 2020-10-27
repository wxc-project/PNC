#include "stdafx.h"
#include "PNCModel.h"
#include "TSPAlgorithm.h"
#include "PNCSysPara.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "LayerTable.h"
#include "PolygonObject.h"
#include "DimStyle.h"
#include "XhMath.h"
#include "PNCCryptCoreCode.h"
#ifdef __TIMER_COUNT_
#include "TimerCount.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  LEN_SCALE		0.8
#define  MIN_DISTANCE	25

CPNCModel model;
//////////////////////////////////////////////////////////////////////////
//CAD_ENTITY
CAD_ENTITY::CAD_ENTITY(ULONG idEnt /*= 0*/)
{
	idCadEnt = idEnt;
	ciEntType = TYPE_OTHER;
	strcpy(sText, "");
	m_fSize = 0;
}
bool CAD_ENTITY::IsInScope(GEPOINT &pt)
{
	double dist = DISTANCE(pt, pos);
	if (dist < m_fSize)
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
//CAD_LINE
CAD_LINE::CAD_LINE(ULONG lineId /*= 0*/) :
	CAD_ENTITY(lineId)
{
	ciEntType = TYPE_LINE;
	m_ciSerial = 0;
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
CAD_LINE::CAD_LINE(AcDbObjectId id, double len) :
	CAD_ENTITY(id.asOldId())
{
	ciEntType = TYPE_LINE;
	m_ciSerial = 0;
	m_fSize = len;
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
void CAD_LINE::Init(AcDbObjectId id, GEPOINT &start, GEPOINT &end)
{
	ciEntType = TYPE_LINE;
	m_ciSerial = 0;
	idCadEnt = id.asOldId();
	m_ptStart = start;
	m_ptEnd = end;
	m_fSize = DISTANCE(m_ptStart, m_ptEnd);
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
BOOL CAD_LINE::UpdatePos()
{
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(idCadEnt), AcDb::kForRead);
	CAcDbObjLife life(pEnt);
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbLine::desc()))
	{
		AcDbLine *pLine = (AcDbLine*)pEnt;
		m_ptStart.Set(pLine->startPoint().x, pLine->startPoint().y, 0);
		m_ptEnd.Set(pLine->endPoint().x, pLine->endPoint().y, 0);
		ciEntType = TYPE_LINE;
	}
	else if (pEnt->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc = (AcDbArc*)pEnt;
		AcGePoint3d startPt, endPt;
		pArc->getStartPoint(startPt);
		pArc->getEndPoint(endPt);
		m_ptStart.Set(startPt.x, startPt.y, 0);
		m_ptEnd.Set(endPt.x, endPt.y, 0);
		ciEntType = TYPE_ARC;
	}
	else if (pEnt->isKindOf(AcDbEllipse::desc()))
	{
		AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
		AcGePoint3d startPt, endPt;
		pEllipse->getStartPoint(startPt);
		pEllipse->getEndPoint(endPt);
		m_ptStart.Set(startPt.x, startPt.y, 0);
		m_ptEnd.Set(endPt.x, endPt.y, 0);
		ciEntType = TYPE_ELLIPSE;
	}
	else if (pEnt->isKindOf(AcDbPolyline::desc()))
	{
		AcDbPolyline* pPolyLine = (AcDbPolyline*)pEnt;
		int nVertNum = pPolyLine->numVerts();
		for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
		{
			AcGePoint3d location;
			pPolyLine->getPointAt(iVertIndex, location);
			if (iVertIndex == 0)
				m_ptStart.Set(location.x, location.y, 0);
			if (iVertIndex == nVertNum - 1)
				m_ptEnd.Set(location.x, location.y, 0);
		}
		ciEntType = TYPE_POLYLINE;
	}
	else
		return FALSE;
	if (m_bReverse)
	{
		GEPOINT temp = m_ptStart;
		m_ptStart = m_ptEnd;
		m_ptEnd = temp;
	}
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//CPlateObject
CPlateObject::CPlateObject()
{

}
CPlateObject::~CPlateObject()
{
	vertexList.Empty();
}
void CPlateObject::CreateRgn()
{
	ARRAY_LIST<f3dPoint> vertices;
	for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
		vertices.append(pVer->pos);
	if (vertices.GetSize() > 0)
		region.CreatePolygonRgn(vertices.m_pData, vertices.GetSize());
}
//判断坐标点是否在钢板内
bool CPlateObject::IsInPlate(const double* poscoord)
{
	if (region.GetVertexCount() < 3)
		CreateRgn();
	if (region.GetAxisZ().IsZero())
		return false;
	//return region.PtInRgn(poscoord)==1;
	int iRet = region.PtInRgn2(poscoord);
	if (iRet == 1 || iRet == 2)
		return true;
	return false;
}
//判断直线是否在钢板内
bool CPlateObject::IsInPlate(const double* start, const double* end)
{
	if (region.GetVertexCount() < 3)
		CreateRgn();
	if (region.GetAxisZ().IsZero())
		return false;
	//int iRet=region.LineInRgn(start,end);
	int iRet = region.LineInRgn2(start, end);
	if (iRet == 1)
		return true;
	else if (iRet == 2)
	{	//部分在多边形区域内
		f3dPoint pt, inters1, inters2;
		for (int i = 0; i < region.GetVertexCount(); i++)
		{
			GEPOINT prePt = region.GetVertexAt(i);
			GEPOINT curPt = region.GetVertexAt((i + 1) % region.GetVertexCount());
			if (Int3dll(prePt, curPt, start, end, pt) > 0)
			{
				if (inters1.IsZero())
					inters1 = pt;
				else
					inters2 = pt;
			}
		}
		double fExternLen = 0, fSumLen = DISTANCE(GEPOINT(start), GEPOINT(end));
		if (!inters2.IsZero())
			fExternLen = DISTANCE(inters1, inters2);
		else if (IsInPlate(start))
			fExternLen = DISTANCE(GEPOINT(start), inters1);
		else if (IsInPlate(end))
			fExternLen = DISTANCE(GEPOINT(end), inters1);
		if (ftoi(fExternLen) <= ftoi(fSumLen) && fExternLen / fSumLen > LEN_SCALE)
			return true;
		else
			return false;
	}
	else
		return false;
}
//判断提取成功的轮廓点是否按逆时针排序
BOOL CPlateObject::IsValidVertexs()
{
	if (!IsValid())
		return FALSE;
	int i = 0, n = vertexList.GetNodeNum();
	double wrap_area = 0;
	DYN_ARRAY<GEPOINT> pnt_arr(n);
	for (VERTEX* pVertex = vertexList.GetFirst(); pVertex; pVertex = vertexList.GetNext(), i++)
		pnt_arr[i] = pVertex->pos;
	for (i = 1; i < n - 1; i++)
	{
		double result = DistOf2dPtLine(pnt_arr[i + 1], pnt_arr[0], pnt_arr[i]);
		if (result > 0)		// 后点在线左侧，正三角形面积
			wrap_area += CalTriArea(pnt_arr[0].x, pnt_arr[0].y, pnt_arr[i].x, pnt_arr[i].y, pnt_arr[i + 1].x, pnt_arr[i + 1].y);
		else if (result < 0)	// 后点在线右侧，负三角形面积
			wrap_area -= CalTriArea(pnt_arr[0].x, pnt_arr[0].y, pnt_arr[i].x, pnt_arr[i].y, pnt_arr[i + 1].x, pnt_arr[i + 1].y);
	}
	if (wrap_area > 0)
		return TRUE;
	else
		return FALSE;
}
void CPlateObject::ReverseVertexs()
{
	int n = vertexList.GetNodeNum();
	ARRAY_LIST<VERTEX> vertexArr;
	vertexArr.SetSize(n);
	VERTEX* pVertex = NULL;
	int i = 0;
	for (pVertex = vertexList.GetTail(); pVertex; pVertex = vertexList.GetPrev())
	{
		vertexArr[i] = *pVertex;
		i++;
	}
	//
	vertexList.Empty();
	for (i = 0; i < n; i++)
	{
		pVertex = vertexList.append();
		*pVertex = vertexArr[i];
	}
}
void CPlateObject::DeleteAssisstPts()
{	//去除辅助型顶点
	for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		if (pVer->tag.lParam == -1)
			vertexList.DeleteCursor();
	}
	vertexList.Clean();
}
void CPlateObject::UpdateVertexPropByArc(f3dArcLine& arcLine, int type)
{
	BOOL bFind = FALSE;
	int i = 0, iStart = -1, iEnd = -1;
	VERTEX* pVer = NULL;
	//根据圆弧更新顶点信息
	for (pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		if (pVer->pos.IsEqual(arcLine.Start(), CPNCModel::DIST_ERROR) && iStart==-1)
		{
			iStart = i;
			pVer->pos = arcLine.Start();
		}
		else if (pVer->pos.IsEqual(arcLine.End(), CPNCModel::DIST_ERROR) && iEnd == -1)
		{
			iEnd = i;
			pVer->pos = arcLine.End();
		}
		if (iStart > -1 && iEnd > -1 && iStart != iEnd)
		{
			bFind = TRUE;
			break;
		}
		i++;
	}
	if (bFind == FALSE)
		return;
	int tag = 1, nNum = vertexList.GetNodeNum();
	if ((iStart > iEnd || (iStart == 0 && iEnd == nNum - 1)) && (iStart + 1) % nNum != iEnd)
	{	//
		tag *= -1;
		int index = iEnd;
		iEnd = iStart;
		iStart = index;
	}
	pVer = vertexList.GetByIndex(iStart);
	if (pVer)
	{
		pVer->ciEdgeType = type;
		pVer->arc.fSectAngle = arcLine.SectorAngle();
		pVer->arc.radius = arcLine.Radius();
		pVer->arc.center = arcLine.Center();
		pVer->arc.work_norm = arcLine.WorkNorm()*tag;
		pVer->arc.column_norm = arcLine.ColumnNorm();
	}
	//删除不需要的顶点
	if ((iStart + 1) % nNum == iEnd || (iEnd + 1) % nNum == iStart)
		return;
	for (int i = iStart + 1; i < iEnd; i++)
		vertexList.DeleteAt(i);
	vertexList.Clean();
}
BOOL CPlateObject::RecogWeldLine(const double* ptS, const double* ptE)
{
	return RecogWeldLine(f3dLine(ptS, ptE));
}
BOOL CPlateObject::RecogWeldLine(f3dLine slop_line)
{
	f3dPoint slop_vec = slop_line.endPt - slop_line.startPt;
	normalize(slop_vec);
	VERTEX* pPreVer = vertexList.GetTail();
	for (VERTEX* pCurVer = vertexList.GetFirst(); pCurVer; pCurVer = vertexList.GetNext())
	{
		f3dPoint vec = pCurVer->pos - pPreVer->pos;
		normalize(vec);
		if (fabs(vec*slop_vec) < eps_cos)
		{
			pPreVer = pCurVer;
			continue;
		}
		double fDist = 0;
		f3dPoint inters, midPt = (pCurVer->pos + pPreVer->pos)*0.5;
		SnapPerp(&inters, slop_line, midPt, &fDist);
		if (fDist < MIN_DISTANCE && slop_line.PtInLine(inters) == 2)
			break;
		pPreVer = pCurVer;
	}
	if (pPreVer)
	{
		pPreVer->m_bWeldEdge = true;
		return TRUE;
	}
	return FALSE;
}
BOOL CPlateObject::IsClose(int* pIndex /*= NULL*/)
{
	if (!IsValid())
		return FALSE;
	GEPOINT tagPT = vertexList.GetFirst()->pos;
	for (int index = 0; index < vertexList.GetNodeNum(); index++)
	{
		if (vertexList[index].tag.lParam == 0 || vertexList[index].tag.lParam == -1)
			continue;
		CAcDbObjLife objLife(MkCadObjId(vertexList[index].tag.dwParam));
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			return FALSE;
		AcDbCurve* pCurve = (AcDbCurve*)pEnt;
		AcGePoint3d acad_ptS, acad_ptE;
		if (pEnt->isKindOf(AcDbPolyline::desc()))
		{
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			if (pPline->isClosed())
				return TRUE;
		}
		else
		{
			pCurve->getStartPoint(acad_ptS);
			pCurve->getEndPoint(acad_ptE);
		}
		GEPOINT linePtS(acad_ptS.x, acad_ptS.y, 0), linePtE(acad_ptE.x, acad_ptE.y, 0);
		if (linePtS.IsEqual(tagPT, EPS2))
			tagPT = linePtE;
		else if (linePtE.IsEqual(tagPT, EPS2))
			tagPT = linePtS;
		else
		{
			if (pIndex)
				*pIndex = index;
			return FALSE;
		}
	}
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
BOOL CPlateProcessInfo::m_bCreatePPIFile = TRUE;	//默认输出ppi文件 wht 20-10-10
CPlateProcessInfo::CPlateProcessInfo()
{
	boltList.Empty();
	m_bIslandDetection=FALSE;
	plateInfoBlockRefId=0;
	partNoId=0;
	partNumId=0;
	m_bEnableReactor = TRUE;
	m_bNeedExtract = FALSE;
	m_ciModifyState = 0;
}
CPNCModel* CPlateProcessInfo::get_pBelongModel() const
{
	if (_pBelongModel == NULL)
		return &model;
	return _pBelongModel;
}
CPNCModel* CPlateProcessInfo::set_pBelongModel(CPNCModel* pBelongModel)
{
	_pBelongModel = pBelongModel;
	return _pBelongModel;
}
//检测钢板原图的轮廓状态：是否闭合
void CPlateProcessInfo::CheckProfileEdge()
{
	if (!IsValid())
		return;
	//删除圆弧的简化辅助点
	DeleteAssisstPts();
	//修订钢板圆弧轮廓边信息
	AcDbEntity *pEnt = NULL;
	CHashSet<CAD_ENTITY*> xRelaLineSet;
	for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
	{
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
			xRelaLineSet.SetValue(pRelaObj->idCadEnt, pRelaObj);
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{	//
			BYTE ciEdgeType = 0;
			f3dArcLine arcLine;
			if (g_pncSysPara.RecogArcEdge(pEnt, arcLine, ciEdgeType))
				UpdateVertexPropByArc(arcLine, ciEdgeType);
			xRelaLineSet.SetValue(pRelaObj->idCadEnt, pRelaObj);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			BYTE ciEdgeType = 0;
			f3dArcLine arcLine;
			if (!g_pncSysPara.RecogArcEdge(pEnt, arcLine, ciEdgeType))
				continue;
			if (fabs(arcLine.SectorAngle()-2*Pi)<EPS2)
			{	//完整的椭圆，判断是否为法兰内圆
				if (cir_plate_para.m_bCirclePlate && pRelaObj->pos.IsEqual(cir_plate_para.cir_center))
				{
					cir_plate_para.m_fInnerR = arcLine.Radius();
					cir_plate_para.norm = arcLine.WorkNorm();
					cir_plate_para.column_norm = arcLine.ColumnNorm();
				}
			}
			else
			{
				UpdateVertexPropByArc(arcLine, ciEdgeType);
				xRelaLineSet.SetValue(pRelaObj->idCadEnt, pRelaObj);
			}
		}
		else if (pEnt->isKindOf(AcDbCircle::desc()) && cir_plate_para.m_bCirclePlate)
		{	//圆形钢板时，查找是否有内轮廓
			AcDbCircle* pCir = (AcDbCircle*)pEnt;
			if (pRelaObj->pos.IsEqual(cir_plate_para.cir_center) && pCir->radius() < cir_plate_para.m_fRadius)
				cir_plate_para.m_fInnerR = pCir->radius();
		}
	}
	//匹配钢板轮廓点对应的原始图元
	int n = vertexList.GetNodeNum();
	for (int i = 0; i < n; i++)
	{
		VERTEX *pCurVertex = vertexList.GetByIndex(i);
		VERTEX *pNextVertex = vertexList.GetByIndex((i + 1) % n);
		GEPOINT vertexS = pCurVertex->pos, vertexE = pNextVertex->pos;
		CAD_ENTITY *pCadLine = NULL;
		for (pCadLine = xRelaLineSet.GetFirst(); pCadLine; pCadLine = xRelaLineSet.GetNext())
		{
			CAcDbObjLife objLife(MkCadObjId(pCadLine->idCadEnt));
			pEnt = objLife.GetEnt();
			if(pEnt==NULL)
				continue;
			AcDbCurve* pCurve = (AcDbCurve*)pEnt;
			AcGePoint3d acad_ptS, acad_ptE;
			pCurve->getStartPoint(acad_ptS);
			pCurve->getEndPoint(acad_ptE);
			GEPOINT ptS, ptE;
			Cpy_Pnt(ptS, acad_ptS);
			Cpy_Pnt(ptE, acad_ptE);
			ptS.z = ptE.z = 0;
			if ((vertexS.IsEqual(ptS, CPNCModel::DIST_ERROR) && vertexE.IsEqual(ptE, CPNCModel::DIST_ERROR))||
				(vertexS.IsEqual(ptE, CPNCModel::DIST_ERROR) && vertexE.IsEqual(ptS, CPNCModel::DIST_ERROR)))
				break;
		}
		if (pCadLine)
			pCurVertex->tag.dwParam = pCadLine->idCadEnt;
	}
}
//更新钢板的螺栓孔信息
void CPlateProcessInfo::UpdateBoltHoles(CHashStrList<CXhChar16>* pHashPartLabelByLabel /*= NULL*/)
{
	if (!IsValid())
		return;
	//
	boltList.Empty();
	//从单个图元(图块、圆圈)中提取螺栓信息
	int nInvalidCirCountForText = 0;
	int nPartLabelCount = 0;
	PreprocessorBoltEnt(&nInvalidCirCountForText, &nPartLabelCount, pHashPartLabelByLabel);
	if (nInvalidCirCountForText > 0)
	{
		logerr.Log("%s#钢板，已过滤%d个可能为件号标注的圆，请确认！", (char*)xPlate.GetPartNo(), nInvalidCirCountForText);
		//件号在圆圈内时不提示 wht 20-09-24
		if (nInvalidCirCountForText > nPartLabelCount)
			m_dwErrorType |= ERROR_TEXT_INSIDE_OF_HOLE;
	}
	AcDbEntity *pEnt = NULL;
	for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
	{
		if (m_xHashInvalidBoltCir.GetValue((DWORD)pRelaObj))
		{
			m_xHashRelaEntIdList.DeleteCursor();
			continue;	//过滤掉无效的螺栓实体（大圆内的小圆、件号标注） wht 19-07-20
		}
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		BOLT_HOLE boltInfo;
		if (!g_pncSysPara.RecogBoltHole(pEnt, boltInfo, m_pBelongModel))
			continue;
		BOLT_INFO *pBoltInfo = boltList.append();
		pBoltInfo->posX = boltInfo.posX;
		pBoltInfo->posY = boltInfo.posY;
		pBoltInfo->bolt_d = boltInfo.d;
		pBoltInfo->hole_d_increment = boltInfo.increment;
		pBoltInfo->cFuncType = (boltInfo.ciSymbolType == 0) ? 0 : 2;
		pBoltInfo->feature = pRelaObj->idCadEnt;	//记录螺栓对应的CAD实体
		//对于圆圈表示的螺栓，根据圆圈直径和标准螺栓直径判断是否归属于标准螺栓, wxc-2020-04-01
		if (g_pncSysPara.IsRecogCirByBoltD() && pEnt->isKindOf(AcDbCircle::desc()))
		{
			double fHoleD = boltInfo.d;
			short wBoltD = 0;
			if (fabs(fHoleD - g_pncSysPara.standard_hole.m_fLS12) < EPS2)
				wBoltD = 12;
			else if (fabs(fHoleD - g_pncSysPara.standard_hole.m_fLS16) < EPS2)
				wBoltD = 16;
			else if (fabs(fHoleD - g_pncSysPara.standard_hole.m_fLS20) < EPS2)
				wBoltD = 20;
			else if (fabs(fHoleD - g_pncSysPara.standard_hole.m_fLS24) < EPS2)
				wBoltD = 24;
			if (wBoltD > 0)
			{
				pBoltInfo->bolt_d = wBoltD;
				pBoltInfo->hole_d_increment = (float)(fHoleD - wBoltD);
				pBoltInfo->cFuncType = 0;
			}
		}
	}
	m_xHashRelaEntIdList.Clean();
	//提取三角螺栓图符、腰圆孔螺栓图符
	if (g_pncSysPara.IsRecongPolylineLs() > 0)
	{
		for (CBoltEntGroup* pBoltGroup = m_pBelongModel->m_xBoltEntHash.GetFirst(); pBoltGroup; 
			pBoltGroup = m_pBelongModel->m_xBoltEntHash.GetNext())
		{
			if(pBoltGroup->m_ciType<2)
				continue;	//图符&圆圈已处理
			if(pBoltGroup->m_bMatch)
				continue;	//已匹配
			if(!IsInPlate(GEPOINT(pBoltGroup->m_fPosX,pBoltGroup->m_fPosY)))
				continue;
			short wBoltD = 0;
			double fHoleD = pBoltGroup->m_fHoleD;
			if (g_pncSysPara.m_ciPolylineLsMode == 2)
			{	//用户指定标准螺栓直径
				if (pBoltGroup->m_ciType == CBoltEntGroup::BOLT_TRIANGLE)
					fHoleD = g_pncSysPara.standard_hole.m_fLS_SJ;
				else if (pBoltGroup->m_ciType == CBoltEntGroup::BOLT_SQUARE)
					fHoleD = g_pncSysPara.standard_hole.m_fLS_ZF;
				else if (pBoltGroup->m_ciType == CBoltEntGroup::BOLT_WAIST_ROUND)
					fHoleD = g_pncSysPara.standard_hole.m_fLS_YY;
			}
			if (fabs(fHoleD - 13.5) < 1)
				wBoltD = 12;
			else if (fabs(fHoleD - 17.5) < 1)
				wBoltD = 16;
			else if (fabs(fHoleD - 21.5) < 1)
				wBoltD = 20;
			else if (fabs(fHoleD - 25.5) < 1)
				wBoltD = 24;
			//添加螺栓,按照标准螺栓进行处理
			pBoltGroup->m_bMatch = TRUE;
			BOLT_INFO *pBoltInfo = boltList.append();
			pBoltInfo->posX = pBoltGroup->m_fPosX;
			pBoltInfo->posY = pBoltGroup->m_fPosY;
			pBoltInfo->bolt_d = fHoleD;
			pBoltInfo->hole_d_increment = 0;
			pBoltInfo->cFuncType = 2;
			if (wBoltD > 0)
			{
				pBoltInfo->bolt_d = wBoltD;
				pBoltInfo->hole_d_increment = (float)(fHoleD - wBoltD);
				pBoltInfo->cFuncType = 0;
			}
		}
	}
	//统一处理根据特殊孔文字标注更新螺栓孔信息	wxc-2020.5.9
	if (g_pncSysPara.IsRecogHoleDimText())
	{
		for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
		{
			if (pRelaObj->ciEntType != TYPE_TEXT &&
				pRelaObj->ciEntType != TYPE_DIM_D)
				continue;
			if (strlen(pRelaObj->sText) <= 0)
				continue;
			if (strstr(pRelaObj->sText, "%%C") == NULL && strstr(pRelaObj->sText, "%%c") == NULL &&
				strstr(pRelaObj->sText, "Φ") == NULL)
				continue;	//没有螺栓直径信息
			//查找与螺栓标注距离最近的螺栓孔
			CMinDouble xMinDis;
			for (BOLT_INFO *pBoltInfo = boltList.GetFirst(); pBoltInfo; pBoltInfo = boltList.GetNext())
			{
				double dist = DISTANCE(pRelaObj->pos, GEPOINT(pBoltInfo->posX, pBoltInfo->posY));
				xMinDis.Update(dist, pBoltInfo);
			}
			if (xMinDis.m_pRelaObj == NULL)
				continue;
			BOLT_INFO *pCurBolt = (BOLT_INFO *)xMinDis.m_pRelaObj;
			CString ss(pRelaObj->sText);
			bool bNeedDrillHole = (ss.Find("钻") >= 0);
			bool bNeedPunchHole = (ss.Find("冲") >= 0);
			if (pCurBolt->cFuncType == 0 && pRelaObj->ciEntType == TYPE_TEXT &&
				!bNeedPunchHole && !bNeedDrillHole)
				continue;	//标准螺栓，没有特殊加工工艺文字标注时不做处理
			//搜索半径根据当前孔半径动态设置
			//孔半径外扩10mm为搜索半径(M12螺栓单排为40，字体高度一般为3mm)，搜索范围太大会影响多个螺栓孔 wht 20-04-23
			double org_hole_d = pCurBolt->bolt_d + pCurBolt->hole_d_increment;
			double org_hole_d2 = org_hole_d;
			CAcDbObjLife objLife(MkCadObjId(pCurBolt->feature));
			if ((pEnt = objLife.GetEnt()) && pEnt->isKindOf(AcDbBlockReference::desc()))
			{
				CBoltEntGroup* pLsBlockEnt = m_pBelongModel->m_xBoltEntHash.GetValue(CXhChar50("%d", pEnt->id().asOldId()));
				if (pLsBlockEnt)
					org_hole_d2 = pLsBlockEnt->m_fHoleD;
			}
			const double TEXT_SEARCH_R = org_hole_d * 0.5 + 25;
			if (xMinDis.number < TEXT_SEARCH_R)
			{
				ss.Replace("%%C", "|");
				ss.Replace("%%c", "|");
				ss.Replace("Φ", "|");
				ss.Replace("钻", "");
				ss.Replace("孔", "");
				ss.Replace("冲", "");
				CXhChar100 sText(ss);
				std::vector<CXhChar16> numKeyArr;
				if (pRelaObj->ciEntType == TYPE_TEXT)
				{	//特殊螺栓文字说明
					for (char* sKey = strtok(sText, "-|，,:：Xx*"); sKey; sKey = strtok(NULL, "-|，,:：Xx*"))
						numKeyArr.push_back(CXhChar16(sKey));
				}
				else
				{	//特殊孔直径标注
					for (char* sKey = strtok(sText, "|"); sKey; sKey = strtok(NULL, "|"))
						numKeyArr.push_back(CXhChar16(sKey));
				}
				if (numKeyArr.size() == 1)
				{
					double hole_d = atof(numKeyArr[0]);
					double dd1 = fabs(hole_d - org_hole_d);
					double dd2 = fabs(hole_d - org_hole_d2);
					if (dd1 < EPS2 || dd2 < EPS2)
					{	//钻孔或冲孔或非标准图块，识别为挂线孔 wht 20-04-27
						pCurBolt->bolt_d = hole_d;
						pCurBolt->hole_d_increment = 0;
						pCurBolt->cFuncType = 2;
						pCurBolt->cFlag = bNeedDrillHole ? 1 : 0;
					}
					else
					{
						logerr.Log("%s#钢板，根据文字(%s)识别的直径与螺栓孔图面直径存在误差值(%.1f)，无法使用文字更新孔状态，请确认！",
							(char*)xPlate.GetPartNo(), (char*)pRelaObj->sText, dd1);
					}
				}
			}
			else
			{
				CXhChar100 sText(pRelaObj->sText);
				std::vector<CXhChar16> numKeyArr;
				for (char* sKey = strtok(sText, "-|，,:：Xx*"); sKey; sKey = strtok(NULL, "-|，,:：Xx*"))
					numKeyArr.push_back(CXhChar16(sKey));
				if (numKeyArr.size() == 1)
				{	//当前文字标注是有效的孔径标注是才需要提醒用户 wht 20-04-28
					logerr.Log("%s#钢板，文字(%s)与螺栓孔距离过远，无法使用文字更新孔状态，请调整后重试！",
						(char*)xPlate.GetPartNo(), (char*)pRelaObj->sText);
				}
			}
		}
	}
}
//
CAD_ENTITY* __AppendRelaEntity(CPNCModel *pBelongModel, AcDbEntity *pEnt, 
							   CHashList<CAD_ENTITY> *pHashRelaEntIdList /*= NULL*/)
{
	if (pEnt == NULL || pHashRelaEntIdList == NULL)
		return NULL;
	CAD_ENTITY* pRelaEnt= pHashRelaEntIdList->GetValue(pEnt->id().asOldId());
	if(pRelaEnt==NULL)
	{
		pRelaEnt= pHashRelaEntIdList->Add(pEnt->id().asOldId());
		pRelaEnt->idCadEnt=pEnt->id().asOldId();
		if (pEnt->isKindOf(AcDbLine::desc()))
			pRelaEnt->ciEntType = TYPE_LINE;
		else if (pEnt->isKindOf(AcDbArc::desc()))
			pRelaEnt->ciEntType = TYPE_ARC;
		else if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle* pCircle = (AcDbCircle*)pEnt;
			AcGePoint3d center = pCircle->center();
			double fHoldD = pCircle->radius() * 2;
			int nValue = (int)floor(fHoldD);	//整数部分
			double fValue = fHoldD - nValue;	//小数部分
			if (fValue < EPS2)	//孔径为整数
				fHoldD = nValue;
			else if (fValue > EPS_COS2)
				fHoldD = nValue + 1;
			else if (fabs(fValue - 0.5) < EPS2)
				fHoldD = nValue + 0.5;
			else
				fHoldD = ftoi(fHoldD);
			//记录圆圈的大小，方便后期处理圆心圆的情况(同一个位置处有多个圆圈)
			pRelaEnt->ciEntType = TYPE_CIRCLE;
			pRelaEnt->m_fSize = fHoldD;
			pRelaEnt->pos.Set(center.x, center.y, center.z);
		}
		else if (pEnt->isKindOf(AcDbSpline::desc()))
			pRelaEnt->ciEntType = TYPE_SPLINE;
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d center = pEllipse->center();
			AcGeVector3d minorAxis = pEllipse->minorAxis();
			double radiusRatio = pEllipse->radiusRatio();
			GEPOINT axis(minorAxis.x, minorAxis.y, minorAxis.z);
			pRelaEnt->m_fSize = axis.mod()*2;
			pRelaEnt->ciEntType = TYPE_ELLIPSE;
			pRelaEnt->pos.Set(center.x, center.y, center.z);
		}
		else if (pEnt->isKindOf(AcDbText::desc()))
		{
			pRelaEnt->ciEntType = TYPE_TEXT;
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			strncpy(pRelaEnt->sText, _bstr_t(pText->textString()), 99);
#else
			strncpy(pRelaEnt->sText, pText->textString(), 99);
#endif
			//取文字中心位置为作为文字位置坐标，否则在判断文字是否在圆内时可能误将尺寸标注识别为件号标注 wht 19-08-14
			Cpy_Pnt(pRelaEnt->pos, pText->position());
			GEPOINT pos;
			if (GetCadTextEntPos(pText, pos,false))
				pRelaEnt->pos.Set(pos.x, pos.y, pos.z);
		}
		else if (pEnt->isKindOf(AcDbMText::desc()))
			pRelaEnt->ciEntType = TYPE_MTEXT;
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{
			pRelaEnt->ciEntType = TYPE_BLOCKREF;
			if (pBelongModel == NULL)
				pBelongModel = &model;
			CBoltEntGroup* pLsBlockEnt = pBelongModel->m_xBoltEntHash.GetValue(CXhChar50("%d", pEnt->id().asOldId()));
			if (pLsBlockEnt)
			{	//记录图块的位置和大小，方便后期处理同一位置有多个图块的情况
				pRelaEnt->pos.x = pLsBlockEnt->m_fPosX;
				pRelaEnt->pos.y = pLsBlockEnt->m_fPosY;
				pRelaEnt->m_fSize = pLsBlockEnt->m_fHoleD;
			}
		}
		else if (pEnt->isKindOf(AcDbDiametricDimension::desc()))
		{
			AcDbDiametricDimension *pDimD = (AcDbDiametricDimension*)pEnt;
			CXhChar50 sDimText;
#ifdef _ARX_2007
			sDimText.Copy((char*)_bstr_t(pDimD->dimensionText()));
#else
			sDimText.Copy(pDimD->dimensionText());
#endif
			pRelaEnt->ciEntType = TYPE_DIM_D;
			strcpy(pRelaEnt->sText,sDimText);
			double fD = 0;
			pDimD->measurement(fD);
			pRelaEnt->m_fSize = fD;
			if (strlen(pRelaEnt->sText) <= 0 && pRelaEnt->m_fSize > 0)
				sprintf(pRelaEnt->sText, "Φ%f", pRelaEnt->m_fSize);
			AcGePoint3d farChordPt = pDimD->farChordPoint();
			AcGePoint3d chordPt = pDimD->chordPoint();
			pRelaEnt->pos.x = (chordPt.x + farChordPt.x)*0.5;
			pRelaEnt->pos.y = (chordPt.y + farChordPt.y)*0.5;
		}
		else
			pRelaEnt->ciEntType=TYPE_OTHER;
	}
	return pRelaEnt;
}

CAD_ENTITY* CPlateProcessInfo::AppendRelaEntity(AcDbEntity* pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList /*= NULL*/)
{
	if (pEnt == NULL)
		return NULL;
	if (pHashRelaEntIdList == NULL)
		pHashRelaEntIdList = &m_xHashRelaEntIdList;
	ASSERT(pHashRelaEntIdList != NULL);
	return XhAppendRelaEntity(m_pBelongModel, pEnt, pHashRelaEntIdList);
}
//根据钢板的轮廓点初始化归属钢板的图元集合
void CPlateProcessInfo::ExtractPlateRelaEnts()
{
	m_xHashRelaEntIdList.Empty();
	m_xHashRelaEntIdList.SetValue(partNoId.asOldId(), CAD_ENTITY(partNoId.asOldId()));
	if (!IsValid())
		return;
	//根据钢板轮廓区域进行缩放
	SCOPE_STRU scope;
	for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		scope.VerifyVertex(pVer->pos);
		//通过像素识别的轮廓边已记录了关联图元
		if(pVer->tag.lParam==0||pVer->tag.lParam==-1)
			continue;
		AcDbEntity *pEnt = NULL;
		XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(pVer->tag.dwParam), AcDb::kForRead);
		if (pEnt)
		{
			AppendRelaEntity(pEnt);
			pEnt->close();
		}
	}
	ZoomAcadView(scope, 10);
	//根据闭合区域拾取归属于钢板的图元集合
	ATOM_LIST<VERTEX> list;
	CalEquidistantShape(CPNCModel::DIST_ERROR * 2, &list);
#ifdef __ALFA_TEST_
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	int nNum = list.GetNodeNum();
	for (int i = 0; i < nNum; i++)
	{
		VERTEX* pCurVer = list.GetByIndex(i);
		VERTEX* pNextVer = list.GetByIndex((i + 1) % nNum);
		CreateAcadLine(pBlockTableRecord, pCurVer->pos, pNextVer->pos, 0, 0, RGB(125, 255, 0));
	}
	pBlockTableRecord->close();
#endif
	struct resbuf* pList = NULL, *pPoly = NULL;
	for (VERTEX* pVer = list.GetFirst(); pVer; pVer = list.GetNext())
	{
		if (pList == NULL)
			pPoly = pList = acutNewRb(RTPOINT);
		else
		{
			pList->rbnext = acutNewRb(RTPOINT);
			pList = pList->rbnext;
		}
		pList->restype = RTPOINT;
		pList->resval.rpoint[X] = pVer->pos.x;
		pList->resval.rpoint[Y] = pVer->pos.y;
		pList->resval.rpoint[Z] = 0;
	}
	pList->rbnext = NULL;
	//根据闭合区域拾取图元
	CHashSet<AcDbObjectId> selEntList;
	SelCadEntSet(selEntList, pPoly);
	for (AcDbObjectId entId = selEntList.GetFirst(); entId.isValid(); entId = selEntList.GetNext())
	{
		CAcDbObjLife objLife(entId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{	//过滤不在区域内的直线
			AcDbLine* pLine = (AcDbLine*)pEnt;
			GEPOINT startPt, endPt;
			Cpy_Pnt(startPt, pLine->startPoint());
			Cpy_Pnt(endPt, pLine->endPoint());
			startPt.z = endPt.z = 0;
			if (!IsInPlate(startPt, endPt))
				continue;
		}
		else if (pEnt->isKindOf(AcDbText::desc()))
		{	//过滤不在区域内的文本
			GEPOINT dimPt = GetCadTextDimPos(pEnt);
			if (!IsInPlate(dimPt))
				continue;
		}
		AppendRelaEntity(pEnt);
	}
}

void CPlateProcessInfo::PreprocessorBoltEnt(int* piInvalidCirCountForText, int* piInvalidCirCountForLabel,
											CHashStrList<CXhChar16>* pHashPartLabelByLabel)
{	//合并圆心相同的圆，同一圆心只保留直径最大的圆
	CHashStrList<CAD_ENTITY*> hashEntPtrByCenterStr;	//记录圆心对应的直径最大的螺栓实体
	m_xHashInvalidBoltCir.Empty();
	for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
	{
		if (pEnt->ciEntType != TYPE_BLOCKREF &&
			pEnt->ciEntType != TYPE_CIRCLE)
			continue;	//仅处理螺栓图块和圆
		if (pEnt->ciEntType == TYPE_BLOCKREF)
		{	
			if (pEnt->m_fSize == 0 || strlen(pEnt->sText) <= 0)
				continue;	//非螺栓图块
		}
		else if (pEnt->ciEntType == TYPE_CIRCLE)
		{
			CAcDbObjLife objLife(MkCadObjId(pEnt->idCadEnt));
			AcDbObjectId idCircleLineType = GetEntLineTypeId(objLife.GetEnt());
			if (idCircleLineType.isValid() && idCircleLineType!= GetLineTypeId("CONTINUOUS"))
			{	//根据制图规范，螺栓圆圈应该都是实线 wxc 20-06-19
				m_xHashInvalidBoltCir.SetValue((DWORD)pEnt, pEnt);
				continue;
			}
			if ((cir_plate_para.m_bCirclePlate && cir_plate_para.cir_center.IsEqual(pEnt->pos)) ||
				(pEnt->m_fSize<0 || pEnt->m_fSize>g_pncSysPara.m_nMaxHoleD))
			{	//环型板的同心圆和直径超过100的圆都不可能时螺栓孔 wxc 20-04-15
				m_xHashInvalidBoltCir.SetValue((DWORD)pEnt, pEnt);
				continue;
			}
		}
		//同一个位置有多个螺栓图符或圆孔时，取直径最大的螺栓图符或圆孔，避免添加重叠螺栓 wht 19 - 11 - 25
		CXhChar50 sKey("%.2f,%.2f,%.2f", pEnt->pos.x, pEnt->pos.y, pEnt->pos.z);
		CAD_ENTITY **ppEntPtr = hashEntPtrByCenterStr.GetValue(sKey);
		if (ppEntPtr)
		{
			if ((*ppEntPtr)->m_fSize > pEnt->m_fSize)
			{	//pEnt直径较小为螺栓内圆需要忽略
				m_xHashInvalidBoltCir.SetValue((DWORD)pEnt, pEnt);
			}
			else
			{	//**ppEntPtr螺栓直径较小为螺栓内圆需要忽略
				m_xHashInvalidBoltCir.SetValue((DWORD)(*ppEntPtr), *ppEntPtr);
				hashEntPtrByCenterStr.DeleteNode(sKey);
				hashEntPtrByCenterStr.SetValue(sKey, pEnt);
			}
		}
		else
			hashEntPtrByCenterStr.SetValue(sKey, pEnt);
	}
	if (g_pncSysPara.IsFilterPartNoCir())
	{
		int nInvalidCount = 0;
		int nInvalidCountForLabel = 0;
		for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
		{
			if (pEnt->ciEntType != TYPE_TEXT && pEnt->ciEntType != TYPE_MTEXT)
				continue;
			//标注字符串中包含"钻孔或冲孔"，
			if (strstr(pEnt->sText, "钻") != NULL || strstr(pEnt->sText, "冲") != NULL ||
				strstr(pEnt->sText, "Φ") != NULL || strstr(pEnt->sText, "%%C") != NULL ||
				strstr(pEnt->sText, "%%c") != NULL|| strstr(pEnt->sText, "焊") != NULL)
				continue;
			//排除件号标注圆圈
			SCOPE_STRU scope;
			for (CAD_ENTITY **ppCirEnt = hashEntPtrByCenterStr.GetFirst(); ppCirEnt; ppCirEnt = hashEntPtrByCenterStr.GetNext())
			{
				CAD_ENTITY *pCirEnt = *ppCirEnt;
				if (pCirEnt->ciEntType == TYPE_BLOCKREF)
					continue;	//螺栓图符内有文字时不需要过滤 wht 19-12-10
				if(g_pncSysPara.m_fPartNoCirD>0 && fabs(pCirEnt->m_fSize - g_pncSysPara.m_fPartNoCirD) >= EPS2)
					continue;	//用户指定件号圆圈孔径时，不满足孔径的不过滤 wxc-20-07-23
				scope.ClearScope();
				double r = pCirEnt->m_fSize*0.5;
				scope.fMinX = pCirEnt->pos.x - r;
				scope.fMaxX = pCirEnt->pos.x + r;
				scope.fMinY = pCirEnt->pos.y - r;
				scope.fMaxY = pCirEnt->pos.y + r;
				scope.fMinZ = scope.fMaxZ = 0;
				if (scope.IsIncludePoint(pEnt->pos))
				{
					m_xHashInvalidBoltCir.SetValue((DWORD)pCirEnt, pCirEnt);
					hashEntPtrByCenterStr.DeleteCursor();
					nInvalidCount++;
					if (pHashPartLabelByLabel && pHashPartLabelByLabel->GetValue(pEnt->sText))
						nInvalidCountForLabel++;
				}
			}
		}
		if (piInvalidCirCountForText)
			*piInvalidCirCountForText = nInvalidCount;
		if (piInvalidCirCountForLabel)
			*piInvalidCirCountForLabel = nInvalidCountForLabel;
	}
}
bool CPlateProcessInfo::RecogRollEdge(CHashSet<CAD_ENTITY*> &rollEdgeDimTextSet,f3dLine &line)
{	//提取卷边信息
	CMinDouble minDisBendDim;
	double dist = 0, fDegree = 0;
	GEPOINT line_vec = (line.endPt - line.startPt).normalized();
	for (CAD_ENTITY* pBendDim = rollEdgeDimTextSet.GetFirst(); pBendDim; pBendDim = rollEdgeDimTextSet.GetNext())
	{
		CAcDbObjLife objLife(MkCadObjId(pBendDim->idCadEnt));
		AcDbEntity* pHuoquText = objLife.GetEnt();
		if (pHuoquText == NULL || !pHuoquText->isKindOf(AcDbText::desc()))
			continue;
		GEPOINT text_vec;
		f3dPoint perp, text_pt;
		text_pt = GetCadTextDimPos(pHuoquText, &text_vec);
		//if(fabs(text_vec*line_vec)< EPS_COS2)
			//continue;	//火曲文字标注方向与火曲线方向不平行
		SnapPerp(&perp, line, text_pt, &dist);
		minDisBendDim.Update(dist, pBendDim);
	}

	BOOL bFindStart = FALSE, bFindEnd = FALSE;
	if (minDisBendDim.m_pRelaObj)
	{
		CAD_ENTITY* pBendDim = (CAD_ENTITY*)minDisBendDim.m_pRelaObj;
		BOOL bFrontBend = FALSE;
		f3dLine datum_line = line;
		//适当延长火曲线与轮廓边求交，便面火曲线绘制不规范导致的无法提取 wht 20-09-26
		datum_line.startPt += line_vec * 20;
		datum_line.endPt += line_vec * 20;
		if (g_pncSysPara.ParseRollEdgeText(pBendDim->sText, fDegree, bFrontBend))
		{
			f3dLine cur_line;
			COORD3D inter_pt;
			VERTEX* pStartVertex = NULL, * pEndVertex = NULL, * pEndPrevVertex = NULL;
			VERTEX* pPrevVertex = vertexList.GetTail();
			for (VERTEX* pVertex = vertexList.GetFirst(); pVertex; pVertex = vertexList.GetNext())
			{
				cur_line.startPt = pPrevVertex->pos;
				cur_line.endPt = pVertex->pos;
				int retCode = Int3dll(line, cur_line, inter_pt);
				if (retCode == 1 || retCode == 2)
				{
					VERTEX newVertex = *pVertex;
					newVertex.pos = inter_pt;
					if (!bFindStart)
					{
						newVertex.m_bRollEdge = true;
						newVertex.manu_space = 50;
						if (!bFrontBend)
							newVertex.manu_space = -50;
						pStartVertex = vertexList.insert(newVertex);
						bFindStart = TRUE;
					}
					else
					{
						bFindEnd = TRUE;
						pEndVertex = vertexList.insert(newVertex);
					}
				}
				if (bFindStart && bFindEnd)
					break;
				pPrevVertex = pVertex;
			}
		}
	}
	return bFindStart && bFindEnd;
}
//根据钢板相关的图元集合更新基本信息、螺栓信息及顶点(火曲)信息
BOOL CPlateProcessInfo::UpdatePlateInfo(BOOL bRelatePN/*=FALSE*/)
{
	if(!IsValid())
		return FALSE;
	//根据Spline提取火曲线标记信息(分段线段集合)
	AcDbEntity *pEnt = NULL;
	CSymbolRecoginzer symbols;
	CHashSet<CAD_ENTITY*> bendDimTextSet;
	CHashSet<CAD_ENTITY*> rollEdgeDimTextSet;
	CHashSet<BOOL> weldMarkLineSet;
	CHashStrList< ATOM_LIST<CAD_LINE> > hashLineArrByPosKeyStr;
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		if( pRelaObj->ciEntType!=TYPE_SPLINE&&
			pRelaObj->ciEntType!=TYPE_TEXT&&
			pRelaObj->ciEntType!=TYPE_LINE&&
			pRelaObj->ciEntType!=TYPE_ARC&&
			pRelaObj->ciEntType!=TYPE_ELLIPSE)
			continue;
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pRelaObj->ciEntType == TYPE_SPLINE)
			symbols.AppendSymbolEnt((AcDbSpline*)pEnt);
		else if(pRelaObj->ciEntType==TYPE_TEXT)
		{
			if (g_pncSysPara.IsMatchBendRule(pRelaObj->sText))
				bendDimTextSet.SetValue(pRelaObj->idCadEnt, pRelaObj);
			else if (g_pncSysPara.IsMatchRollEdgeRule(pRelaObj->sText))
				rollEdgeDimTextSet.SetValue(pRelaObj->idCadEnt, pRelaObj);
		}
		else
		{
			AcDbCurve* pCurve = (AcDbCurve*)pEnt;
			AcGePoint3d acad_ptS, acad_ptE;
			pCurve->getStartPoint(acad_ptS);
			pCurve->getEndPoint(acad_ptE);
			GEPOINT ptS, ptE;
			Cpy_Pnt(ptS, acad_ptS);
			Cpy_Pnt(ptE, acad_ptE);
			ptS.z = ptE.z = 0;
			double len = DISTANCE(ptS, ptE);
			//记录始端点坐标处的连接线
			CXhChar50 startKeyStr(ptS);
			ATOM_LIST<CAD_LINE> *pStartLineList = hashLineArrByPosKeyStr.GetValue(startKeyStr);
			if (pStartLineList == NULL)
				pStartLineList = hashLineArrByPosKeyStr.Add(startKeyStr);
			pStartLineList->append(CAD_LINE(pEnt->objectId(), len));
			//记录终端点坐标处的连接线
			CXhChar50 endKeyStr(ptE);
			ATOM_LIST<CAD_LINE> *pEndLineList = hashLineArrByPosKeyStr.GetValue(endKeyStr);
			if (pEndLineList == NULL)
				pEndLineList = hashLineArrByPosKeyStr.Add(endKeyStr);
			pEndLineList->append(CAD_LINE(pEnt->objectId(), len));
		}
	}
	for (ATOM_LIST<CAD_LINE> *pList = hashLineArrByPosKeyStr.GetFirst(); pList; pList = hashLineArrByPosKeyStr.GetNext())
	{
		if (pList->GetNodeNum() != 1)
			continue;
		CAD_LINE *pLineId = pList->GetFirst();
		if (pLineId&&pLineId->m_fSize < CPNCModel::WELD_MAX_HEIGHT)
			weldMarkLineSet.SetValue(pLineId->idCadEnt, TRUE);
	}
	xPlate.m_cFaceN = 1;
	BASIC_INFO baseInfo;
	for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
	{
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		//提取基本信息
		if(plateInfoBlockRefId==NULL&&g_pncSysPara.RecogBasicInfo(pEnt,baseInfo))
		{	//plateInfoBlockRefId有效时已从块中提取到了基本属性 wht 19-01-04
			if(baseInfo.m_cMat>0)
				xPlate.cMaterial=baseInfo.m_cMat;
			if(baseInfo.m_nThick>0)
				xPlate.m_fThick=(float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
				xPlate.m_nSingleNum = xPlate.m_nProcessNum = baseInfo.m_nNum;
			if (baseInfo.m_idCadEntNum != 0)
				partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			if (baseInfo.m_sTaType.GetLength() > 0 && m_pBelongModel->m_sTaType.GetLength() <= 0)
				m_pBelongModel->m_sTaType.Copy(baseInfo.m_sTaType);
			if(baseInfo.m_sPartNo.GetLength()>0&&bRelatePN)
			{
				if(xPlate.GetPartNo().GetLength()<=0)
					xPlate.SetPartNo(baseInfo.m_sPartNo);
				else if(m_sRelatePartNo.GetLength()<=0)
					m_sRelatePartNo.Copy(baseInfo.m_sPartNo);
				else
					m_sRelatePartNo.Append(CXhChar16(",%s",(char*)baseInfo.m_sPartNo));
				pnTxtIdList.SetValue(pRelaObj->idCadEnt,MkCadObjId(pRelaObj->idCadEnt));
			}
			continue;
		}
		//提取钢印区
		f3dPoint ptArr[4];
		if(g_pncSysPara.RecogMkRect(pEnt,ptArr,4))
		{
			xPlate.mkpos.Set((ptArr[0].x+ptArr[2].x)/2,(ptArr[0].y+ptArr[2].y)/2,(ptArr[0].z+ptArr[2].z)/2);
			if(DISTANCE(ptArr[0],ptArr[1])>DISTANCE(ptArr[1],ptArr[2]))
				xPlate.mkVec=ptArr[0]-ptArr[1];
			else
				xPlate.mkVec=ptArr[1]-ptArr[2];
			normalize(xPlate.mkVec);
			continue;
		}
		//提取火曲线信息
		if(pEnt->isKindOf(AcDbLine::desc())&&weldMarkLineSet.GetValue(pRelaObj->idCadEnt)==NULL)
		{	
			f3dLine line;
			AcDbLine* pAcDbLine=(AcDbLine*)pEnt;
			Cpy_Pnt(line.startPt, pAcDbLine->startPoint());
			Cpy_Pnt(line.endPt, pAcDbLine->endPoint());
			if(g_pncSysPara.IsBendLine((AcDbLine*)pEnt,&symbols))
			{
				//RecogRollEdge算法待完善，目前PNC中不需要特殊处理卷边点wxc-2020.10.14
				//if (!RecogRollEdge(rollEdgeDimTextSet, line))
				{
					if (xPlate.m_cFaceN >= 3)
						continue;	//最多支持两条火曲线
					//记录火曲线
					xPlate.m_cFaceN += 1;
					xPlate.HuoQuLine[xPlate.m_cFaceN - 2] = line;
					//提取火曲角度，初始化火曲面法向
					CMinDouble minDisBendDim;
					double dist = 0, fDegree = 0;
					GEPOINT line_vec = (line.endPt - line.startPt).normalized();
					for (CAD_ENTITY *pBendDim = bendDimTextSet.GetFirst(); pBendDim; pBendDim = bendDimTextSet.GetNext())
					{
						CAcDbObjLife objLife(MkCadObjId(pBendDim->idCadEnt));
						AcDbEntity* pHuoquText = objLife.GetEnt();
						if (pHuoquText == NULL || !pHuoquText->isKindOf(AcDbText::desc()))
							continue;
						GEPOINT text_vec;
						f3dPoint perp, text_pt;
						text_pt = GetCadTextDimPos(pHuoquText, &text_vec);
						//if(fabs(text_vec*line_vec)< EPS_COS2)
							//continue;	//火曲文字标注方向与火曲线方向不平行
						SnapPerp(&perp, line, text_pt, &dist);
						minDisBendDim.Update(dist, pBendDim);
					}
					if (minDisBendDim.m_pRelaObj)
					{
						CAD_ENTITY* pBendDim = (CAD_ENTITY*)minDisBendDim.m_pRelaObj;
						BOOL bFrontBend = FALSE;
						g_pncSysPara.ParseBendText(pBendDim->sText, fDegree, bFrontBend);
						if (fDegree > 0)
						{
							fDegree *= bFrontBend ? 1 : -1;
							GEPOINT bend_face_norm(0, 0, 1);
							RotateVectorAroundVector(bend_face_norm, fDegree * RADTODEG_COEF, line_vec);
							normalize(bend_face_norm);
							xPlate.HuoQuFaceNorm[xPlate.m_cFaceN - 2] = bend_face_norm;
						}
					}
				}
			}
			else if(g_pncSysPara.IsSlopeLine((AcDbLine*)pEnt))
			{
				RecogWeldLine(line);
				continue;
			}
		}
	}
	return m_xHashRelaEntIdList.GetNodeNum()>1;
}
f2dRect CPlateProcessInfo::GetPnDimRect(double fRectW /*= 10*/, double fRectH /*= 10*/)
{
	//文本坐标 this->dim_pos 已做居中处理
	f2dRect rect;
	rect.topLeft.Set(dim_pos.x - fRectW, dim_pos.y + fRectH);
	rect.bottomRight.Set(dim_pos.x + fRectW, dim_pos.y - fRectH);
	return rect;
}
//过滤重复点
static BOOL IsHasVertex(ATOM_LIST<CPlateObject::VERTEX>& tem_vertes,GEPOINT pt)
{
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		CPlateObject::VERTEX* pVer=tem_vertes.GetByIndex(i);
		if(pVer->pos.IsEqual(pt,EPS2))
			return TRUE;
	}
	return FALSE;
}
//通过bpoly命令提取钢板轮廓点
void CALLBACK EXPORT CloseBoundaryPopupWnd(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	HWND hMainWnd = adsw_acadMainWnd();
	HWND hPopupWnd=GetLastActivePopup(hMainWnd);
	if(hPopupWnd!=NULL&&hPopupWnd!=hMainWnd)
	{
		::SendMessage(hPopupWnd ,WM_CLOSE,0,0);
		KillTimer(hMainWnd,1);
	}
}

void CPlateProcessInfo::InitProfileByBPolyCmd(double fMinExtern,double fMaxExtern, BOOL bSendCommand /*= FALSE*/)
{
	if (!m_bNeedExtract)
		return;
#ifdef __ALFA_TEST1_
	//用于测试查看文本的坐标位置
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y + 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y - 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y - 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y + 10));
	pBlockTableRecord->close();
#endif
	//
	ads_name seqent;
	AcDbObjectId initLastObjId,plineId;
	acdbEntLast(seqent);
	acdbGetObjectId(initLastObjId,seqent);
	//将件号标注位置进行缩放,找到合适的缩放比例,通过boundary命令提取轮廓边
	f2dRect rect=GetPnDimRect();
	double fExternLen=fMaxExtern*3;
	fExternLen=(fExternLen>2200)?2200:fExternLen;
	if (g_pncSysPara.m_bUseMaxEdge)
		fExternLen = g_pncSysPara.m_nMaxEdgeLen;
	double fScale=(fExternLen-fMinExtern)/10;
	//
	HWND hMainWnd=adsw_acadMainWnd();
	f3dPoint cur_dim_pos=dim_pos;
	for(int i=0;i<=10;i++)
	{
		if(i>0)
			fExternLen-=fScale;
		double fZoomScale = fExternLen / g_pncSysPara.m_fMapScale;
		ZoomAcadView(rect, fZoomScale);
		ads_point base_pnt;
		base_pnt[X]=cur_dim_pos.x;
		base_pnt[Y]=cur_dim_pos.y;
		base_pnt[Z]=cur_dim_pos.z;
		UINT_PTR nTimer=SetTimer(hMainWnd,1,100,CloseBoundaryPopupWnd);
		int resCode= RTNORM;
		if (bSendCommand)
		{
			CXhChar50 sCmd, sPos("%.2f,%.2f", cur_dim_pos.x, cur_dim_pos.y);
			if (m_bIslandDetection)
				sCmd.Printf("-boundary %s\n ", (char*)sPos);
			else
				sCmd.Printf("-boundary a i n\n \n%s\n ",(char*)sPos);
#ifdef _ARX_2007
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			if (m_bIslandDetection)
				resCode = acedCommand(RTSTR, L"-boundary", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
			else
				resCode = acedCommand(RTSTR, L"-boundary", RTSTR, L"a", RTSTR, L"i", RTSTR, L"n", RTSTR, L"", RTSTR, L"", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
#else
			if (m_bIslandDetection)
				resCode = acedCommand(RTSTR, "-boundary", RTPOINT, base_pnt, RTSTR, "", RTNONE);
			else
				resCode = acedCommand(RTSTR, "-boundary", RTSTR, "a", RTSTR, "i", RTSTR, "n", RTSTR, "", RTSTR, "", RTPOINT, base_pnt, RTSTR, "", RTNONE);
#endif
		}
		if(nTimer==1)
			KillTimer(hMainWnd,nTimer);
		if(resCode!=RTNORM)
			return;
		acdbEntLast(seqent);
		acdbGetObjectId(plineId,seqent);
		if(initLastObjId!=plineId)
			break;
	}
	if (initLastObjId == plineId)
	{	//执行空命令行(代表输入回车)，避免重复执行上一条命令 wxc-2019.6.13
		if (bSendCommand)
#ifdef _ARX_2007
			SendCommandToCad(L" \n ");
#else
			SendCommandToCad(" \n ");
#endif
		else
			acedCommand(RTSTR, "");
		return;	//执行boundary未新增实体(即未找到有效的封闭区域)
	}
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
	if(pEnt==NULL||!pEnt->isKindOf(AcDbPolyline::desc()))
	{
		if(pEnt)
			pEnt->close();
		//执行空命令行(代表输入回车)，避免重复执行上一条命令 wxc-2019.6.13
		if (bSendCommand)
#ifdef _ARX_2007
			SendCommandToCad(L" \n ");
#else
			SendCommandToCad(" \n ");
#endif
		else
			acedCommand(RTSTR, "");
		return;
	}
	//
	AcDbPolyline *pPline=(AcDbPolyline*)pEnt;
	int nVertNum = pPline->numVerts();
	ATOM_LIST<VERTEX> tem_vertes;
	VERTEX* pVer=NULL;
	for(int iVertIndex=0;iVertIndex<nVertNum;iVertIndex++)
	{
		AcGePoint3d location;
		pPline->getPointAt(iVertIndex,location);
		if(IsHasVertex(tem_vertes,GEPOINT(location.x,location.y,location.z)))
			continue;
		f3dPoint startPt,endPt;
		if(pPline->segType(iVertIndex)==AcDbPolyline::kLine)
		{
			AcGeLineSeg3d acgeLine;
			pPline->getLineSegAt(iVertIndex,acgeLine);
			Cpy_Pnt(startPt,acgeLine.startPoint());
			Cpy_Pnt(endPt,acgeLine.endPoint());
			pVer=tem_vertes.append();
			pVer->pos.Set(location.x,location.y,location.z);
		}
		else if(pPline->segType(iVertIndex)==AcDbPolyline::kArc)
		{
			AcGeCircArc3d acgeArc;
			pPline->getArcSegAt(iVertIndex,acgeArc);
			f3dPoint center,norm;
			Cpy_Pnt(startPt,acgeArc.startPoint());
			Cpy_Pnt(endPt,acgeArc.endPoint());
			Cpy_Pnt(center,acgeArc.center());
			Cpy_Pnt(norm,acgeArc.normal());
			double fAngle = fabs(acgeArc.endAng() - acgeArc.startAng());
			if (fAngle > Pi)
				fAngle = 2 * Pi - fAngle;
			pVer=tem_vertes.append();
			pVer->pos.Set(location.x,location.y,location.z);
			pVer->ciEdgeType=2;
			pVer->arc.center=center;
			pVer->arc.work_norm=norm;
			pVer->arc.radius=acgeArc.radius();
			pVer->arc.fSectAngle = fAngle;
		}
	}
	pPline->erase(Adesk::kTrue);	//删除polyline对象
	pPline->close();
	//
	nVertNum = tem_vertes.GetNodeNum();
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		VERTEX* pCurVer=tem_vertes.GetByIndex(i);
		if(pCurVer->ciEdgeType==2&&ftoi(pCurVer->arc.radius)==CPNCModel::ASSIST_RADIUS)
		{
			VERTEX* pNextVer=tem_vertes.GetByIndex((i+1)%nVertNum);
			if(pNextVer->ciEdgeType==1)
				pNextVer->pos=pCurVer->arc.center;
			else if(pNextVer->ciEdgeType==2&&ftoi(pNextVer->arc.radius)!= CPNCModel::ASSIST_RADIUS)
				pNextVer->pos=pCurVer->arc.center;
		}
	}
	for(pVer=tem_vertes.GetFirst();pVer;pVer=tem_vertes.GetNext())
	{
		if(pVer->ciEdgeType==2&&ftoi(pVer->arc.radius)== CPNCModel::ASSIST_RADIUS)
			tem_vertes.DeleteCursor();
	}
	tem_vertes.Clean();
	//判断钢板是否为圆型钢板
	if (tem_vertes.GetNodeNum() == 4)
	{
		bool bValidCir = true;
		GEPOINT pt;
		for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
		{
			VERTEX* pCur = tem_vertes.GetByIndex(i);
			if (pt.IsZero())
				pt = pCur->arc.center;
			double fAngle = RadToDegree(pCur->arc.fSectAngle);
			if (pCur->ciEdgeType != 2 
				|| fabs(90 - fAngle) > 2 
				|| !pt.IsEqual(pCur->arc.center,EPS2))
			{
				bValidCir = false;
				break;
			}
		}
		if (bValidCir)
		{
			cir_plate_para.m_bCirclePlate = TRUE;
			cir_plate_para.cir_center = tem_vertes[0].arc.center;
			cir_plate_para.m_fRadius = tem_vertes[0].arc.radius;
		}
	}
	//填充钢板的轮廓点
	vertexList.Empty();
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		VERTEX* pCur=tem_vertes.GetByIndex(i);
		pVer=vertexList.append(*pCur);
		if(pVer->ciEdgeType==2)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			pVer->ciEdgeType=1;
			VERTEX* pNext=tem_vertes.GetByIndex((i+1)%tem_vertes.GetNodeNum());
			f3dPoint startPt=pCur->pos,endPt=pNext->pos;
			double len = 0.5*DISTANCE(startPt,endPt);
			if(len<EPS2)
				continue;	//圆弧起始点、终止点距离过近导致死机（CreateMethod3需改进） wht 19-01-02
			f3dArcLine arcLine;
			if(arcLine.CreateMethod3(startPt,endPt,pCur->arc.work_norm,pCur->arc.radius,pCur->arc.center))
			{
				int nSlices=CalArcResolution(arcLine.Radius(),arcLine.SectorAngle(),1.0,5.0,18);
				double angle=arcLine.SectorAngle()/nSlices;
				for(int i=1;i<nSlices;i++)
				{
					VERTEX *pProVer= vertexList.append();
					pProVer->pos=arcLine.PositionInAngle(angle*i);
					pProVer->tag.lParam=-1;
				}
			}
		}
	}
	//对轮廓点进行合理性检查(是否按逆时针排序)
	if(!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
}
BOOL CPlateProcessInfo::InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntSet)
{	//1.根据选中实体初始化直线列表
	if (selectedEntSet.GetNodeNum() <= 0)
		return FALSE;
	ARRAY_LIST<CAD_LINE> objectLineArr;
	objectLineArr.SetSize(0, selectedEntSet.GetNodeNum());
	for (AcDbObjectId objId = selectedEntSet.GetFirst(); objId; objId = selectedEntSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbLine::desc()) &&
			!pEnt->isKindOf(AcDbArc::desc()) &&
			!pEnt->isKindOf(AcDbEllipse::desc()) &&
			!pEnt->isKindOf(AcDbPolyline::desc()))
			continue;
		if (pEnt->isKindOf(AcDbPolyline::desc()))	//多段线直接提取轮廓点
			return InitProfileByAcdbPolyLine(objId);
		else
		{
			AcDbCurve* pCurve = (AcDbCurve*)pEnt;
			AcGePoint3d acad_pt;
			pCurve->getStartPoint(acad_pt);
			GEPOINT ptS, ptE;
			Cpy_Pnt(ptS, acad_pt);
			pCurve->getEndPoint(acad_pt);
			Cpy_Pnt(ptE, acad_pt);
			//
			CAD_LINE* pLine = NULL;
			for (pLine = objectLineArr.GetFirst(); pLine; pLine = objectLineArr.GetNext())
			{	//过滤始终点重合的线段
				if ((pLine->m_ptStart.IsEqual(ptS, EPS2) && pLine->m_ptEnd.IsEqual(ptE, EPS2)) ||
					(pLine->m_ptStart.IsEqual(ptE, EPS2) && pLine->m_ptEnd.IsEqual(ptS, EPS2)))
					break;
			}
			if (pLine == NULL)
			{
				pLine = objectLineArr.append();
				pLine->m_ptStart = ptS;
				pLine->m_ptEnd = ptE;
				pLine->idCadEnt = objId.asOldId();
			}
		}
	}
	if (objectLineArr.GetSize() > 3)
		return InitProfileByAcdbLineList(objectLineArr);
	return FALSE;
}
BOOL CPlateProcessInfo::InitProfileByAcdbCircle(AcDbObjectId idAcdbCircle)
{
	CAcDbObjLife objLife(idAcdbCircle);
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL || !pEnt->isKindOf(AcDbCircle::desc()))
		return FALSE;
	GEPOINT center;
	AcDbCircle* pCircle = (AcDbCircle*)pEnt;
	Cpy_Pnt(center, pCircle->center());
	vertexList.Empty();
	for (int i = 0; i < 4; i++)
	{
		VERTEX* pVer = vertexList.append();
		if (i == 0)
			pVer->pos.Set(center.x + pCircle->radius(), center.y, center.z);
		else if (i == 1)
			pVer->pos.Set(center.x, center.y + pCircle->radius(), center.z);
		else if (i == 2)
			pVer->pos.Set(center.x - pCircle->radius(), center.y, center.z);
		else if (i == 3)
			pVer->pos.Set(center.x, center.y - pCircle->radius(), center.z);
		pVer->tag.dwParam = idAcdbCircle.asOldId();
		pVer->ciEdgeType = 2;
		pVer->arc.radius = pCircle->radius();
		pVer->arc.center = center;
		pVer->arc.fSectAngle = 0.5*Pi;
		pVer->arc.work_norm.Set(0, 0, 1);
	}
	if (!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
	return TRUE;
}
BOOL CPlateProcessInfo::InitProfileByAcdbPolyLine(AcDbObjectId idAcdbPline)
{
	CAcDbObjLife objLife(idAcdbPline);
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPolyline::desc()))
		return FALSE;
	AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
	if (!pPline->isClosed())
		return FALSE;	//非闭合的多段线
	ATOM_LIST<VERTEX> tem_vertes;
	int nVertNum = pPline->numVerts();
	for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
	{
		AcGePoint3d location;
		pPline->getPointAt(iVertIndex, location);
		if (IsHasVertex(tem_vertes, GEPOINT(location.x, location.y, location.z)))
			continue;
		if (pPline->segType(iVertIndex) == AcDbPolyline::kLine)
		{
			VERTEX* pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
			pVer->tag.dwParam = idAcdbPline.asOldId();
		}
		else if (pPline->segType(iVertIndex) == AcDbPolyline::kArc)
		{
			AcGeCircArc3d acgeArc;
			pPline->getArcSegAt(iVertIndex, acgeArc);
			GEPOINT center, norm;
			Cpy_Pnt(center, acgeArc.center());
			Cpy_Pnt(norm, acgeArc.normal());
			VERTEX* pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
			pVer->tag.dwParam = idAcdbPline.asOldId();
			pVer->ciEdgeType = 2;
			pVer->arc.center = center;
			pVer->arc.work_norm = norm;
			pVer->arc.radius = acgeArc.radius();
		}
	}
	//
	vertexList.Empty();
	for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
	{
		VERTEX* pCur = tem_vertes.GetByIndex(i);
		VERTEX* pNewVer = vertexList.append(*pCur);
		if (pNewVer->ciEdgeType == 2)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			pNewVer->ciEdgeType = 1;
			VERTEX* pNext = tem_vertes.GetByIndex((i + 1) % tem_vertes.GetNodeNum());
			GEPOINT startPt = pCur->pos, endPt = pNext->pos;
			double len = 0.5*DISTANCE(startPt, endPt);
			if (len < EPS2)
				continue;
			f3dArcLine arcLine;
			if (arcLine.CreateMethod3(startPt, endPt, pCur->arc.work_norm, pCur->arc.radius, pCur->arc.center))
			{
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					VERTEX *pProVer = vertexList.append();
					pProVer->pos = arcLine.PositionInAngle(angle*i);
					pProVer->tag.lParam = -1;
				}
			}
		}
	}
	if (!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
	return TRUE;
}
BOOL CPlateProcessInfo::InitProfileByAcdbLineList(ARRAY_LIST<CAD_LINE>& xLineArr)
{
	if (xLineArr.GetSize() <= 0)
		return FALSE;
	return InitProfileByAcdbLineList(xLineArr[0], xLineArr);
}
BOOL CPlateProcessInfo::InitProfileByAcdbLineList(CAD_LINE& startLine, ARRAY_LIST<CAD_LINE>& xLineArr)
{
	if (xLineArr.GetSize() <= 0)
		return FALSE;
	//查找起始轮廓线
	CAD_LINE *pLine = NULL, *pFirLine = NULL;
	for (pLine = xLineArr.GetFirst(); pLine; pLine = xLineArr.GetNext())
	{
		if (pLine->m_ptStart.IsEqual(startLine.m_ptStart) &&
			pLine->m_ptEnd.IsEqual(startLine.m_ptEnd))
		{
			pFirLine = pLine;
			break;
		}
	}
	if (pFirLine == NULL)
		pFirLine = xLineArr.GetFirst();
	//遍历匹配闭合轮廓边
	GEPOINT ptS, ptE, vec;
	BYTE ciSerial = 1;
	BOOL bFinish = FALSE;
	ATOM_LIST<VERTEX> tem_vertes;
	for (;;)
	{
		if (pFirLine&&pFirLine->m_bMatch == FALSE)
		{
			pFirLine->m_bMatch = TRUE;
			pFirLine->m_ciSerial = ciSerial++;
			pFirLine->vertex = pFirLine->m_ptStart;
			ptS = pFirLine->m_ptStart;
			ptE = pFirLine->m_ptEnd;
			vec = (ptE - ptS);
			normalize(vec);
		}
		if (fabs(DISTANCE(ptS, ptE)) < CPNCModel::DIST_ERROR)
		{
			bFinish = TRUE;
			break;
		}
		ATOM_LIST<CAD_LINE*> linkLineList;
		CAD_LINE* pLine = NULL;
		for (pLine = xLineArr.GetFirst(); pLine; pLine = xLineArr.GetNext())
		{
			if (pLine->m_bMatch)
				continue;
			if (pLine->m_ptStart.IsEqual(ptE, CPNCModel::DIST_ERROR))
				linkLineList.append(pLine);
			else if (pLine->m_ptEnd.IsEqual(ptE, CPNCModel::DIST_ERROR))
				linkLineList.append(pLine);
		}
		if (linkLineList.GetNodeNum() <= 0)
			break;	//没有找到相连线段
		if (linkLineList.GetNodeNum() == 1)
		{	//只有一根相连线段
			pLine = linkLineList[0];
			pLine->m_bMatch = TRUE;
			pLine->m_ciSerial = ciSerial++;
			if (pLine->m_ptStart.IsEqual(ptE, CPNCModel::DIST_ERROR))
			{
				pLine->vertex = pLine->m_ptStart;
				ptE = pLine->m_ptEnd;
			}
			else
			{
				pLine->vertex = pLine->m_ptEnd;
				ptE = pLine->m_ptStart;
			}
			vec = (ptE - pLine->vertex);
			normalize(vec);
		}
		else
		{	//含有多根相连线段
			CMaxDouble maxValue;
			for (int i = 0; i < linkLineList.GetNodeNum(); i++)
			{
				GEPOINT cur_vec;
				if (linkLineList[i]->m_ptStart.IsEqual(ptE, CPNCModel::DIST_ERROR))
					cur_vec = (linkLineList[i]->m_ptEnd - linkLineList[i]->m_ptStart);
				else
					cur_vec = (linkLineList[i]->m_ptStart - linkLineList[i]->m_ptEnd);
				normalize(cur_vec);
				double fCosa = vec * cur_vec;
				maxValue.Update(fCosa, linkLineList[i]);
			}
			pLine = (CAD_LINE*)maxValue.m_pRelaObj;
			pLine->m_bMatch = TRUE;
			pLine->m_ciSerial = ciSerial++;
			if (pLine->m_ptStart.IsEqual(ptE, CPNCModel::DIST_ERROR))
			{
				pLine->vertex = pLine->m_ptStart;
				ptE = pLine->m_ptEnd;
			}
			else
			{
				pLine->vertex = pLine->m_ptEnd;
				ptE = pLine->m_ptStart;
			}
			vec = (ptE - pLine->vertex);
			normalize(vec);
		}
	}
	if (bFinish == FALSE)
		return FALSE;
	//初始化钢板轮廓点
	CQuickSort<CAD_LINE>::QuickSort(xLineArr.m_pData, xLineArr.GetSize(), CAD_LINE::compare_func);
	for (int i = 0; i < xLineArr.GetSize(); i++)
	{
		CAD_LINE objItem = xLineArr[i];
		if (!objItem.m_bMatch)
			continue;
		VERTEX* pVer = tem_vertes.append();
		pVer->pos = objItem.vertex;
		pVer->tag.dwParam = objItem.idCadEnt;
		//记录圆弧信息
		AcDbEntity *pEnt = NULL;
		XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(objItem.idCadEnt), AcDb::kForRead);
		CAcDbObjLife objLife(pEnt);
		if (pEnt && pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			GEPOINT ptS, ptE, center, norm;
			ptS = (objItem.vertex.IsEqual(objItem.m_ptStart)) ? objItem.m_ptStart : objItem.m_ptEnd;
			ptE = (objItem.vertex.IsEqual(objItem.m_ptStart)) ? objItem.m_ptEnd : objItem.m_ptStart;
			Cpy_Pnt(center, pArc->center());
			Cpy_Pnt(norm, pArc->normal());
			if (objItem.vertex.IsEqual(objItem.m_ptEnd))
				norm *= -1;
			double radius = pArc->radius();
			double fAngle = fabs(pArc->endAngle() - pArc->startAngle());
			fAngle = (fAngle > Pi) ? 2 * Pi - fAngle : fAngle;
			if (radius <= 0 || DISTANCE(ptS, ptE) < EPS || fAngle <= 0)
				continue;	//不合理圆弧
			pVer->ciEdgeType = 2;
			pVer->arc.center = center;
			pVer->arc.work_norm = norm;
			pVer->arc.radius = radius;
			pVer->arc.fSectAngle = fAngle;
		}
		else if (pEnt && pEnt->isKindOf(AcDbPolyline::desc()))
		{
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			int nVertNum = pPline->numVerts();
			for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
			{
				AcGePoint3d location;
				pPline->getPointAt(iVertIndex, location);
				VERTEX* pNewVer = NULL;
				if (pVer->pos.IsEqual(GEPOINT(location.x, location.y, 0), CPNCModel::DIST_ERROR))
					pNewVer = pVer;
				else
					pNewVer = tem_vertes.append();
				pNewVer->pos.Set(location.x, location.y, 0);
				pNewVer->tag.dwParam = objItem.idCadEnt;
				if (pPline->segType(iVertIndex) == AcDbPolyline::kArc)
				{
					AcGeCircArc3d acgeArc;
					pPline->getArcSegAt(iVertIndex, acgeArc);
					GEPOINT center, norm;
					Cpy_Pnt(center, acgeArc.center());
					Cpy_Pnt(norm, acgeArc.normal());
					double fAngle = fabs(acgeArc.endAng() - acgeArc.startAng());
					fAngle = (fAngle > Pi) ? 2 * Pi - fAngle : fAngle;
					pNewVer->ciEdgeType = 2;
					pNewVer->arc.center = center;
					pNewVer->arc.work_norm = norm;
					pNewVer->arc.radius = acgeArc.radius();
					pNewVer->arc.fSectAngle = fAngle;
				}
			}
		}
	}
	//判断钢板是否为圆型钢板
	if (tem_vertes.GetNodeNum() == 4)
	{
		bool bValidCir = true;
		GEPOINT pt;
		for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
		{
			VERTEX* pCur = tem_vertes.GetByIndex(i);
			if (pt.IsZero())
				pt = pCur->arc.center;
			double fAngle = RadToDegree(pCur->arc.fSectAngle);
			if (pCur->ciEdgeType != 2
				|| fabs(90 - fAngle) > 2
				|| !pt.IsEqual(pCur->arc.center, EPS2))
			{
				bValidCir = false;
				break;
			}
		}
		if (bValidCir)
		{
			cir_plate_para.m_bCirclePlate = TRUE;
			cir_plate_para.cir_center = tem_vertes[0].arc.center;
			cir_plate_para.m_fRadius = tem_vertes[0].arc.radius;
		}
	}
	//初始化钢板轮廓点
	vertexList.Empty();
	for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
	{
		VERTEX* pCur = tem_vertes.GetByIndex(i);
		VERTEX* pNewVer = vertexList.append(*pCur);
		if (pNewVer->ciEdgeType == 2)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			pNewVer->ciEdgeType = 1;
			VERTEX* pNext = tem_vertes.GetByIndex((i + 1) % tem_vertes.GetNodeNum());
			GEPOINT startPt = pCur->pos, endPt = pNext->pos;
			double len = 0.5*DISTANCE(startPt, endPt);
			if (len < EPS2)
				continue;
			f3dArcLine arcLine;
			if (arcLine.CreateMethod3(startPt, endPt, pCur->arc.work_norm, pCur->arc.radius, pCur->arc.center))
			{
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					VERTEX *pProVer = vertexList.append();
					pProVer->pos = arcLine.PositionInAngle(angle*i);
					pProVer->tag.lParam = -1;
				}
			}
		}
	}
	if (!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
	return TRUE;
}
void CPlateProcessInfo::InitMkPos(GEPOINT &mk_pos,GEPOINT &mk_vec)
{
	GEPOINT dim_pos, dim_vec;
	for(AcDbObjectId objId=pnTxtIdList.GetFirst();objId;objId=pnTxtIdList.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if(pEnt==NULL)
			continue;
		GEPOINT pt = GetCadTextDimPos(pEnt, &dim_vec);
		if (dim_pos.IsZero())
			dim_pos = pt;
		else
			dim_pos = (dim_pos + pt)*0.5;
	}
	mk_pos = dim_pos;
	mk_vec = dim_vec;
}
//生成ppi中性文件
void CPlateProcessInfo::InitPPiInfo()
{
	//计算钢板的长与宽
	f2dRect rect=GetMinWrapRect();
	xPlate.m_wLength=(rect.Width()>rect.Height())?ftoi(rect.Width()):ftoi(rect.Height());
	xPlate.m_fWidth =(rect.Width()>rect.Height())?(float)ftoi(rect.Height()): (float)ftoi(rect.Width());
	//初始化钢板的工艺信息
	VERTEX* pVer=NULL;
	PROFILE_VER* pNewVer=NULL;
	xPlate.vertex_list.Empty();
	for(pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		pNewVer=xPlate.vertex_list.Add(0);
		pNewVer->type=pVer->ciEdgeType;
		pNewVer->vertex=pVer->pos;
		if(pVer->ciEdgeType>1)
		{
			pNewVer->radius=pVer->arc.radius;
			pNewVer->sector_angle=pVer->arc.fSectAngle;
			pNewVer->center=pVer->arc.center;
			pNewVer->work_norm=pVer->arc.work_norm;
			pNewVer->column_norm=pVer->arc.column_norm;
		}
		pNewVer->m_bWeldEdge=pVer->m_bWeldEdge;
		pNewVer->m_bRollEdge=pVer->m_bRollEdge;
		pNewVer->manu_space=pVer->manu_space;
		//
		pNewVer->vertex*=g_pncSysPara.m_fMapScale;
		pNewVer->center*=g_pncSysPara.m_fMapScale;
	}
	if(cir_plate_para.m_bCirclePlate && cir_plate_para.m_fInnerR>0)
	{
		xPlate.m_fInnerRadius = cir_plate_para.m_fInnerR;
		xPlate.m_tInnerOrigin = cir_plate_para.cir_center;
		xPlate.m_tInnerColumnNorm = cir_plate_para.column_norm;
	}
	BOLT_INFO* pBolt=NULL,*pNewBolt=NULL;
	xPlate.m_xBoltInfoList.Empty();
	for(pBolt=boltList.GetFirst();pBolt;pBolt=boltList.GetNext())
	{
		pNewBolt=xPlate.m_xBoltInfoList.Add(0);
		pNewBolt->CloneBolt(pBolt);
		pNewBolt->posX*=(float)g_pncSysPara.m_fMapScale;
		pNewBolt->posY*=(float)g_pncSysPara.m_fMapScale;
	}
	if(xPlate.mkpos.IsZero())
		InitMkPos(xPlate.mkpos,xPlate.mkVec);
	xPlate.mkpos*=g_pncSysPara.m_fMapScale;
	xPlate.m_bIncDeformed=g_pncSysPara.m_bIncDeformed;
	if(xPlate.mcsFlg.ciBottomEdge==(BYTE)-1)
		InitBtmEdgeIndex();
}
void CPlateProcessInfo::CreatePPiFile(const char* file_path)
{
	if (!m_bCreatePPIFile)
		return;	//不输出ppi文件
	//设置当前工作路径
	SetCurrentDirectory(file_path);
	CXhChar100 sAllRelPart(xPlate.GetPartNo());
	if (g_pncSysPara.m_iPPiMode == 1 && m_sRelatePartNo.GetLength() > 0)
	{	//一板多号模式：一个PPI文件包括多个件号
		xPlate.m_sRelatePartNo.Copy(m_sRelatePartNo);
		sAllRelPart.Printf("%s,%s", (char*)xPlate.GetPartNo(), (char*)m_sRelatePartNo);
		sAllRelPart.Replace(",", " ");
	}
	CBuffer buffer;
	xPlate.ToPPIBuffer(buffer);
	CString sFilePath;
	sFilePath.Format(".\\%s.ppi", (char*)sAllRelPart);
	FILE* fp=fopen(sFilePath,"wb");
	if (fp)
	{
		long file_len = buffer.GetLength();
		fwrite(&file_len, sizeof(long), 1, fp);
		fwrite(buffer.GetBufferPtr(), buffer.GetLength(), 1, fp);
		fclose(fp);
	}
	else
	{
		DWORD nRetCode = GetLastError();
		logerr.Log("%s#钢板ppi文件保存失败,错误号#%d！路径：%s%s", (char*)xPlate.GetPartNo(), nRetCode,file_path,sFilePath);
	}
}
//属性成员拷贝
void CPlateProcessInfo::CopyAttributes(CPlateProcessInfo* pSrcPlate)
{
	CXhChar16 sDestPartNo = GetPartNo();
	pSrcPlate->xPlate.ClonePart(&xPlate);
	xPlate.cQuality = pSrcPlate->xPlate.cQuality;
	if(sDestPartNo.GetLength()>1)
		xPlate.SetPartNo(sDestPartNo);	//保留原来的件号（分解一板多号的时候） wxc-20.09.09
	//
	dim_pos = pSrcPlate->dim_pos;
	dim_vec = pSrcPlate->dim_vec;
	vertexList.Empty();
	CPlateObject::VERTEX* pSrcVertex = NULL;
	for (CPlateObject::VERTEX* pSrcVertex = pSrcPlate->vertexList.GetFirst(); pSrcVertex;
		pSrcVertex = pSrcPlate->vertexList.GetNext())
		vertexList.append(*pSrcVertex);
	boltList.Empty();
	for (BOLT_INFO* pSrcBolt = pSrcPlate->boltList.GetFirst(); pSrcBolt; pSrcBolt = pSrcPlate->boltList.GetNext())
		boltList.append(*pSrcBolt);
	//
	m_xHashRelaEntIdList.Empty();
	for (CAD_ENTITY* pSrcCadEnt = pSrcPlate->m_xHashRelaEntIdList.GetFirst(); pSrcCadEnt;
		pSrcCadEnt = pSrcPlate->m_xHashRelaEntIdList.GetNext())
		m_xHashRelaEntIdList.SetValue(pSrcCadEnt->idCadEnt, *pSrcCadEnt);
	m_cloneEntIdList.Empty();
	for (ULONG *pSrcId = pSrcPlate->m_cloneEntIdList.GetFirst(); pSrcId;
		pSrcId = pSrcPlate->m_cloneEntIdList.GetNext())
		m_cloneEntIdList.append(*pSrcId);
	m_newAddEntIdList.Empty();
	for (ULONG *pSrcId = pSrcPlate->m_newAddEntIdList.GetFirst(); pSrcId;
		pSrcId = pSrcPlate->m_newAddEntIdList.GetNext())
		m_newAddEntIdList.append(*pSrcId);
#ifdef __UBOM_ONLY_
	CBuffer buffer(1024);
	pSrcPlate->xBomPlate.ToBuffer(buffer);
	buffer.SeekToBegin();
	xBomPlate.FromBuffer(buffer);
#endif
}
//初始化钢板加工坐标系X轴所在的轮廓边
void CPlateProcessInfo::InitBtmEdgeIndex()
{
	int i=0,n=xPlate.vertex_list.GetNodeNum(),bottom_edge=-1,weld_edge=-1;
	DYN_ARRAY<f3dPoint> vertex_arr(n);
	for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext(),i++)
	{
		vertex_arr[i]=pVertex->vertex;
		if(pVertex->type>1)
			vertex_arr[i].feature=-1;
		if(pVertex->m_bWeldEdge)
			weld_edge=i;
	}
	if(g_pncSysPara.m_iAxisXCalType==1)
	{	//优先根据螺栓确定X轴所在轮廓边
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature==-1)
				continue;
			f3dLine edge_line(vertex_arr[i],vertex_arr[(i+1)%n]);
			CHashList<f3dPoint> xVerTagHash;
			for(BOLT_INFO* pLs=xPlate.m_xBoltInfoList.GetFirst();pLs;pLs=xPlate.m_xBoltInfoList.GetNext())
			{
				f3dPoint ls_pt(pLs->posX,pLs->posY,0),perp;
				double fDist=0;
				if(SnapPerp(&perp,edge_line,ls_pt,&fDist)==FALSE||edge_line.PtInLine(perp,1)!=2)
					continue;	//螺栓投影点不在轮廓边上
				f3dPoint* pTag=xVerTagHash.GetValue(ftoi(fDist));
				if(pTag==NULL)
					pTag=xVerTagHash.Add(ftoi(fDist));
				pTag->feature+=1;
			}
			for(f3dPoint* pTag=xVerTagHash.GetFirst();pTag;pTag=xVerTagHash.GetNext())
			{
				if(pTag->feature>1 && pTag->feature >vertex_arr[i].feature)
					vertex_arr[i].feature=pTag->feature;
			}
		}
		int fMaxLs=0;
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature>fMaxLs)
			{
				bottom_edge=i;
				fMaxLs=vertex_arr[i].feature;
			}
		}
	}
	else if(g_pncSysPara.m_iAxisXCalType==2)
	{	//以焊接边作为加工坐标系的X坐标轴
		bottom_edge=weld_edge;
	}
	if(bottom_edge>-1)
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	else
	{	//以最长边作为加工坐标系的X坐标轴
		GEPOINT vertice,prev_vec,edge_vec;
		double prev_edge_dist=0, edge_dist = 0, max_edge = 0;
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature==-1)
				continue;
			edge_vec=vertex_arr[(i+1)%n]-vertex_arr[i];
			edge_dist = edge_vec.mod();
			if(edge_dist<EPS2)
				continue;
			edge_vec/=edge_dist;	//单位化边矢量
			if(i>0&&prev_vec*edge_vec>EPS_COS)	//连续共线边轮廓
				edge_dist+=edge_dist+prev_edge_dist;
			if(edge_dist>max_edge)
			{
				max_edge = edge_dist;
				bottom_edge=i;
			}
			prev_edge_dist=edge_dist;
			prev_vec=edge_vec;
		}
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	}
}
//获取钢板调整后的加工坐标
void CPlateProcessInfo::BuildPlateUcs()
{
	int i=0,n=xPlate.vertex_list.GetNodeNum();
	if(n<=0)
		return;
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext(),i++)
		vertex_arr[i]=pVertex->vertex;
	if(xPlate.mcsFlg.ciBottomEdge==-1||xPlate.mcsFlg.ciBottomEdge>=n)
	{	//初始化加工坐标系
		WORD bottom_edge;
		GEPOINT vertice,prev_vec,edge_vec;
		double prev_edge_dist=0, edge_dist = 0, max_edge = 0;
		for(i=0;i<n;i++)
		{
			edge_vec=vertex_arr[(i+1)%n]-vertex_arr[i];
			edge_dist = edge_vec.mod();
			edge_vec/=edge_dist;	//单位化边矢量
			if(!dim_vec.IsZero()&&fabs(dim_vec*edge_vec)>EPS_COS)
			{	//优先查找与文字标注方向相同的边作为基准边
				bottom_edge=i;
				break;
			}
			if(i>0&&prev_vec*edge_vec>EPS_COS)	//连续共线边轮廓
				edge_dist+=edge_dist+prev_edge_dist;
			if(edge_dist>max_edge)
			{
				max_edge = edge_dist;
				bottom_edge=i;
			}
			prev_edge_dist=edge_dist;
			prev_vec=edge_vec;
		}
		//
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	}
	//根据bottom_edge_i计算加工坐标系
	int iEdge=xPlate.mcsFlg.ciBottomEdge;
	f3dPoint prev_pt = vertex_arr[(iEdge - 1 + n) % n];
	f3dPoint cur_pt=vertex_arr[iEdge];
	f3dPoint next_pt=vertex_arr[(iEdge+1)%n];
	f3dPoint next_next_pt = vertex_arr[(iEdge + 2) % n];
	ucs.origin = cur_pt;
	ucs.axis_x = next_pt - cur_pt;
	if (DistOf2dPtLine(next_pt.x, next_pt.y, cur_pt.x, cur_pt.y, next_next_pt.x, next_next_pt.y) > 0)
	{	//next_pt点为凹点，以next_next_pt为基准计算加持边 wht 19-04-09
		ucs.axis_x = next_next_pt - cur_pt;
	}
	else if (DistOf2dPtLine(cur_pt.x, cur_pt.y, prev_pt.x, prev_pt.y, next_pt.x, next_pt.y) > 0)
	{	//cur_pt点位凹点，以prev_pt与next_pt连线位为基准计算加持边 wht 19-04-09
		ucs.axis_x = next_pt - prev_pt;
	}
	normalize(ucs.axis_x);
	ucs.axis_y.Set(-ucs.axis_x.y,ucs.axis_x.x);
	ucs.axis_z=ucs.axis_x^ucs.axis_y;
	//调整坐标系原点
	SCOPE_STRU scope=GetPlateScope(TRUE);
	ucs.origin+=scope.fMinX*ucs.axis_x;
	//如果钢板进行反转，更新钢板坐标系
	if(xPlate.mcsFlg.ciOverturn==TRUE)
	{
		ucs.origin+=((scope.fMaxX-scope.fMinX)*ucs.axis_x);
		ucs.axis_x*=-1.0;
		ucs.axis_z*=-1.0;
	}
}
SCOPE_STRU CPlateProcessInfo::GetPlateScope(BOOL bVertexOnly,BOOL bDisplayMK/*=TRUE*/)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	if (xPlate.vertex_list.GetNodeNum() >= 3)
	{
		PROFILE_VER* pPreVertex = xPlate.vertex_list.GetTail();
		for (PROFILE_VER *pVertex = xPlate.vertex_list.GetFirst(); pVertex; pVertex = xPlate.vertex_list.GetNext())
		{
			f3dPoint pt = pPreVertex->vertex;
			coord_trans(pt, ucs, FALSE);
			scope.VerifyVertex(pt);
			if (pPreVertex->type == 2)
			{
				f3dPoint axis_len(ucs.axis_x);
				f3dArcLine arcLine;
				arcLine.CreateMethod2(pPreVertex->vertex, pVertex->vertex, pPreVertex->work_norm, pPreVertex->sector_angle);
				f3dPoint startPt = f3dPoint(arcLine.Center()) + axis_len * arcLine.Radius();
				f3dPoint endPt = f3dPoint(arcLine.Center()) - axis_len * arcLine.Radius();
				coord_trans(startPt, ucs, FALSE);
				coord_trans(endPt, ucs, FALSE);
				scope.VerifyVertex(startPt);
				scope.VerifyVertex(endPt);
			}
			pPreVertex = pVertex;
		}
		if(!bVertexOnly)
		{	//处理螺栓孔和钢印号
			for (BOLT_INFO *pHole = xPlate.m_xBoltInfoList.GetFirst(); pHole; pHole = xPlate.m_xBoltInfoList.GetNext())
			{
				double radius = 0.5*pHole->bolt_d;
				scope.VerifyVertex(f3dPoint(pHole->posX - radius, pHole->posY - radius));
				scope.VerifyVertex(f3dPoint(pHole->posX + radius, pHole->posY + radius));
			}
			if (xPlate.IsDisplayMK() && bDisplayMK)
				scope.VerifyVertex(f3dPoint(xPlate.mkpos.x, xPlate.mkpos.y));
		}
	}
	else if(IsValid())
	{
		for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
			scope.VerifyVertex(pVer->pos);
	}
	else
	{	//件号标注区域
		f2dRect rect=GetPnDimRect();
		scope.VerifyVertex(rect.topLeft);
		scope.VerifyVertex(rect.bottomRight);
	}
	return scope;
}
void CPlateProcessInfo::InitLayoutVertex(SCOPE_STRU& scope, BYTE ciLayoutType)
{
	datumStartVertex.Init();
	datumEndVertex.Init();
	GEPOINT datumPt;
	if (ciLayoutType == 0)	//水平排布时，以左下角为基准点
		datumPt.Set(scope.fMinX, scope.fMinY);
	else                    //竖直排布时，以左上角为基准点
		datumPt.Set(scope.fMinX, scope.fMaxY);
	datumStartVertex.srcPos = datumPt;
	datumEndVertex.srcPos = datumStartVertex.srcPos;
}
bool CPlateProcessInfo::DrawPlate(f3dPoint *pOrgion/*=NULL*/,BOOL bCreateDimPos/*=FALSE*/,
								  BOOL bDrawAsBlock/*=FALSE*/, GEPOINT *pPlateCenter /*= NULL*/,
								  double scale /*= 0*/, BOOL bSupportRotation /*= TRUE*/)
{	
	CLockDocumentLife lockCurDocumentLife;
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	if(pBlockTableRecord==NULL)
	{
		AfxMessageBox("获取块表记录失败!");
		return false;
	}
	AcGeMatrix3d scaleMat;
	AcGeMatrix3d moveMat,rotationMat;
	if(pOrgion)
	{	//计算平移基准点及旋转角度
		ads_point ptFrom,ptTo;
		ptFrom[X]=datumStartVertex.srcPos.x;
		ptFrom[Y]=datumStartVertex.srcPos.y;
		ptFrom[Z]=0;
		ptTo[X]=pOrgion->x+datumStartVertex.offsetPos.x;
		ptTo[Y]=pOrgion->y+datumStartVertex.offsetPos.y;
		ptTo[Z]=0;
		moveMat.setToTranslation(AcGeVector3d(ptTo[X]-ptFrom[X],ptTo[Y]-ptFrom[Y],ptTo[Z]-ptFrom[Z]));
		//
		GEPOINT src_vec=datumEndVertex.srcPos-datumStartVertex.srcPos;
		GEPOINT dest_vec=datumEndVertex.offsetPos-datumStartVertex.offsetPos;
		double fDegAngle=Cal2dLineAng(0,0,dest_vec.x,dest_vec.y)-Cal2dLineAng(0,0,src_vec.x,src_vec.y);
		rotationMat.setToRotation(fDegAngle,AcGeVector3d::kZAxis,AcGePoint3d(ptTo[X],ptTo[Y],ptTo[Z]));
		//
		AcGePoint3d centerPt(pOrgion->x, pOrgion->y, 0);
		if (fabs(scale) > 0)
			scaleMat.setToScaling(scale, centerPt);
	}
	AcGeMatrix3d blockMoveMat;
	if (pPlateCenter)
	{
		ads_point ptFrom, ptTo;
		ptFrom[X] = pPlateCenter->x;
		ptFrom[Y] = pPlateCenter->y;
		ptFrom[Z] = 0;
		ptTo[X] = 0;
		ptTo[Y] = 0;
		ptTo[Z] = 0;
		blockMoveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
	}
	//
	m_cloneEntIdList.Empty();
	m_hashColneEntIdBySrcId.Empty();
	AcDbBlockTableRecord *pCurBlockTblRec = pBlockTableRecord;
	if (bDrawAsBlock&&pPlateCenter&&DRAGSET.BeginBlockRecord())
		pCurBlockTblRec = DRAGSET.RecordingBlockTableRecord();
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		AcDbEntity *pEnt = objLife.GetEnt();
		if(pEnt==NULL)
			continue;
		AcDbEntity *pClone = (AcDbEntity *)pEnt->clone();
		if(pClone)
		{
			if(!IsValid())
				pClone->setColorIndex(1);	//
			if(pOrgion)
			{
				pClone->transformBy(moveMat);		//平移
				if(bSupportRotation)
					pClone->transformBy(rotationMat);	//旋转
			}
			if (pPlateCenter&&bDrawAsBlock)
				pClone->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			if (fabs(scale) > 0 && fabs(scale-1)>EPS)
				pClone->transformBy(scaleMat);		//按指定比例缩放钢板 wht 20-01-22
			AcDbObjectId entId;
			DRAGSET.AppendAcDbEntity(pCurBlockTblRec,entId,pClone);
			m_cloneEntIdList.append(entId.asOldId());
			m_hashColneEntIdBySrcId.SetValue(pRelaObj->idCadEnt,entId.asOldId());
			pClone->close();
		}
	}
	if(bCreateDimPos)
	{	//生成标注点
		AcDbObjectId pointId;
		AcDbPoint *pPoint=new AcDbPoint(AcGePoint3d(dim_pos.x,dim_pos.y,dim_pos.z));
		if(DRAGSET.AppendAcDbEntity(pCurBlockTblRec,pointId,pPoint))
		{
			if (pOrgion)
			{
				pPoint->transformBy(moveMat);		//平移
				pPoint->transformBy(rotationMat);	//旋转
			}
			if (pPlateCenter&&bDrawAsBlock)
				pPoint->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			m_xMkDimPoint.idCadEnt=pointId.asOldId();
			AcGePoint3d pos=pPoint->position();
			m_xMkDimPoint.pos.Set(pos.x,pos.y,0);
		}
		pPoint->close();
	}
	//对于提取失败或不闭合的钢板做特殊处理
	int index = -1;
	if(!IsValid() || !IsClose(&index))
	{
		GEPOINT center = (index == -1) ? dim_pos : vertexList[index].pos;
		AcDbObjectId circleId = CreateAcadCircle(pCurBlockTblRec, center, 20, 0, RGB(255, 191, 0));
		if (pOrgion || (pPlateCenter && bDrawAsBlock))
		{
			AcDbEntity* pEnt = NULL;
			XhAcdbOpenAcDbEntity(pEnt, circleId, AcDb::kForWrite);
			if (pEnt)
			{
				if (pOrgion)
				{
					pEnt->transformBy(moveMat);
					pEnt->transformBy(rotationMat);
				}
				if(pPlateCenter && bDrawAsBlock)
					pEnt->transformBy(blockMoveMat);
				pEnt->close();
			}
		}
	}
	if (bDrawAsBlock&&pPlateCenter&&pCurBlockTblRec != pBlockTableRecord)
		DRAGSET.EndBlockRecord(pBlockTableRecord, *pPlateCenter, 1.0, &m_layoutBlockId);
	pBlockTableRecord->close();
	return true;
}
void CPlateProcessInfo::DrawPlateProfile(f3dPoint *pOrgion /*= NULL*/)
{
	CLockDocumentLife lockCurDocumentLife;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	if (pBlockTableRecord == NULL)
	{
		AfxMessageBox("获取块表记录失败!");
		return;
	}
	m_newAddEntIdList.Empty();
	AcDbObjectId entId;
	if(IsValid())
	{
		int n = vertexList.GetNodeNum();
		for (int i = 0; i < n; i++)
		{
			VERTEX *pCurVer = vertexList.GetByIndex(i);
			VERTEX *pNextVer = vertexList.GetByIndex((i + 1) % n);
			entId = DimText(pBlockTableRecord, pCurVer->pos, CXhChar16("%d", i),
				TextStyleTable::hzfs.textStyleId, 2, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull, g_pncSysPara.crMode.crEdge);
			m_newAddEntIdList.append(entId.asOldId());
			if (pCurVer->ciEdgeType == 1)
			{
				entId = CreateAcadLine(pBlockTableRecord, pCurVer->pos, pNextVer->pos, 0, 0, g_pncSysPara.crMode.crEdge);
				m_newAddEntIdList.append(entId.asOldId());
			}
			else
			{	//圆弧
				GEPOINT ptS = pCurVer->pos, ptE = pNextVer->pos, org = pCurVer->arc.center, norm = pCurVer->arc.work_norm;
				f3dArcLine arcline;
				if (pCurVer->arc.radius > 0 && DISTANCE(ptS, ptE) > EPS)
				{
				if (pCurVer->ciEdgeType == 2)
				{	//圆弧
					arcline.CreateMethod3(ptS, ptE, norm, pCurVer->arc.radius, org);
					entId = CreateAcadArcLine(pBlockTableRecord, arcline, 0, g_pncSysPara.crMode.crEdge);
				}
				else
				{	//椭圆
					arcline.CreateEllipse(org, ptS, ptE, pCurVer->arc.column_norm, norm, pCurVer->arc.radius);
					entId = CreateAcadEllipseLine(pBlockTableRecord, arcline, 0, g_pncSysPara.crMode.crEdge);
					m_newAddEntIdList.append(entId.asOldId());
				}
				}
				else
				{	//
					logerr.Log("钢板的圆弧轮廓边信息有误，请检查原图!");
					entId = CreateAcadLine(pBlockTableRecord, ptS, ptE, 0, 0, g_pncSysPara.crMode.crEdge);
				}
				m_newAddEntIdList.append(entId.asOldId());
			}
		}
		//绘制火曲线
		for (int i = 0; i < xPlate.m_cFaceN-1; i++)
		{
			f3dLine line;
			if (xPlate.GetBendLineAt(i, &line) != 0)
			{
				COLORREF clrHQ = -1;
				if (g_pncSysPara.m_ciBendLineColorIndex > 0 && g_pncSysPara.m_ciBendLineColorIndex < 255)
					clrHQ = GetColorFromIndex(g_pncSysPara.m_ciBendLineColorIndex);
				entId = CreateAcadLine(pBlockTableRecord, line.startPt, line.endPt, 0, 0, clrHQ);
				m_newAddEntIdList.append(entId.asOldId());
			}
		}
		if (cir_plate_para.m_bCirclePlate && cir_plate_para.m_fInnerR > 0)
		{
			entId = CreateAcadCircle(pBlockTableRecord, cir_plate_para.cir_center, cir_plate_para.m_fInnerR, 0, g_pncSysPara.crMode.crEdge);
			m_newAddEntIdList.append(entId.asOldId());
		}
		//绘制螺栓
		for (BOLT_INFO* pHole = boltList.GetFirst(); pHole; pHole = boltList.GetNext())
		{
			COLORREF clr = g_pncSysPara.crMode.crOtherLS;
			if (pHole->cFuncType < 2 && pHole->bolt_d == 12)
				clr = g_pncSysPara.crMode.crLS12;
			else if (pHole->cFuncType < 2 && pHole->bolt_d == 16)
				clr = g_pncSysPara.crMode.crLS16;
			else if (pHole->cFuncType < 2 && pHole->bolt_d == 20)
				clr = g_pncSysPara.crMode.crLS20;
			else if (pHole->cFuncType < 2 && pHole->bolt_d == 24)
				clr = g_pncSysPara.crMode.crLS24;
			double fHoleR = (pHole->bolt_d + pHole->hole_d_increment)*0.5;
			entId = CreateAcadCircle(pBlockTableRecord, f3dPoint(pHole->posX, pHole->posY, 0), fHoleR, 0, clr);
			m_newAddEntIdList.append(entId.asOldId());
		}
		if (!xPlate.mkpos.IsZero())
		{
			GEPOINT ptArr[4];
			xPlate.GetMkRect(20, 10, ptArr);
			for (int i = 0; i < 4; i++)
			{
				f3dPoint ptS(ptArr[i]);
				f3dPoint ptE(ptArr[(i + 1) % 4]);
				entId = CreateAcadLine(pBlockTableRecord, ptS, ptE, 0, 0, g_pncSysPara.crMode.crEdge);
				m_newAddEntIdList.append(entId.asOldId());
			}
		}
	}
	else
	{
		entId = DimText(pBlockTableRecord, dim_pos, GetPartNo(), TextStyleTable::hzfs.textStyleId, 2, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull, g_pncSysPara.crMode.crEdge);
		m_newAddEntIdList.append(entId.asOldId());
		entId = CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y + 10), 0, 0, g_pncSysPara.crMode.crEdge);
		m_newAddEntIdList.append(entId.asOldId());
		entId = CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y - 10), 0, 0, g_pncSysPara.crMode.crEdge);
		m_newAddEntIdList.append(entId.asOldId());
		entId = CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y - 10), 0, 0, g_pncSysPara.crMode.crEdge);
		m_newAddEntIdList.append(entId.asOldId());
		entId = CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y + 10), 0, 0, g_pncSysPara.crMode.crEdge);
		m_newAddEntIdList.append(entId.asOldId());
	}
	//
	if (pOrgion)
	{
		//计算平移基准点
		AcGeMatrix3d moveMat;
		ads_point ptFrom, ptTo;
		ptFrom[X] = datumStartVertex.srcPos.x;
		ptFrom[Y] = datumStartVertex.srcPos.y;
		ptFrom[Z] = 0;
		ptTo[X] = pOrgion->x + datumStartVertex.offsetPos.x;
		ptTo[Y] = pOrgion->y + datumStartVertex.offsetPos.y;
		ptTo[Z] = 0;
		moveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
		//对新增图元进行移动
		for (int i = 0; i < m_newAddEntIdList.GetSize(); i++)
		{
			entId = MkCadObjId(m_newAddEntIdList.At(i));
			AcDbEntity* pEnt = NULL;
			XhAcdbOpenAcDbEntity(pEnt, entId, AcDb::kForWrite);
			if (pEnt)
			{
				pEnt->transformBy(moveMat);
				pEnt->close();
			}
		}
	}
	pBlockTableRecord->close();
}
static BOOL IsInSector(double start_ang,double sector_ang,double verify_ang,BOOL bClockWise=TRUE)
{
	double ang=0;
	if(!bClockWise)	//逆时针
		ang=ftoi(verify_ang-start_ang)%360;
	else			//顺时针
		ang=ftoi(start_ang-verify_ang)%360;
	if(ang<0)
		ang+=360;
	if(sector_ang>ang||fabs(sector_ang-ang)<EPS)
		return TRUE;
	else
		return FALSE;
}

void CPlateProcessInfo::CalEquidistantShape(double minDistance,ATOM_LIST<VERTEX> *pDestList)
{
	if (pDestList == NULL)
		return;
	//简化轮廓点（将圆弧简化为直线）
	ATOM_LIST<VERTEX> tem_vertes;
	for (int i = 0; i < vertexList.GetNodeNum(); i++)
	{
		VERTEX* pCur = vertexList.GetByIndex(i);
		VERTEX* pNewVer = tem_vertes.append(*pCur);
		if (pNewVer->ciEdgeType == 2)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			pNewVer->ciEdgeType = 1;
			VERTEX* pNext = vertexList.GetByIndex((i + 1) % vertexList.GetNodeNum());
			GEPOINT ptS = pCur->pos, ptE = pNext->pos;
			f3dArcLine arcLine;
			if (arcLine.CreateMethod3(ptS, ptE, pCur->arc.work_norm, pCur->arc.radius, pCur->arc.center))
			{
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					VERTEX *pProVer = tem_vertes.append();
					pProVer->pos = arcLine.PositionInAngle(angle*i);
				}
			}
		}
	}
	//处理带凹槽的钢板（连续凹点过小的情况）
	BOOL bGroove = FALSE;
	int iPreConcavePt = -1;
	int nNum = tem_vertes.GetNodeNum();
	for (int i = 0; i < nNum; i++)
	{
		VERTEX* pPrevVertex = tem_vertes.GetByIndex((i - 1 + nNum) % nNum);
		VERTEX* pCurrVertex = tem_vertes.GetByIndex(i);
		VERTEX* pNextVertex = tem_vertes.GetByIndex((i + 1) % nNum);
		if (DistOf2dPtLine(pNextVertex->pos, pPrevVertex->pos, pCurrVertex->pos) < EPS)
		{	//当前点为凹点
			if (iPreConcavePt > -1 && (i - iPreConcavePt + nNum) % nNum == 1)
			{
				bGroove = TRUE;
				break;
			}
			iPreConcavePt = i;
		}
	}
	if (bGroove)
	{	//删除过窄的凹槽点
		VERTEX* pPrevVertex = tem_vertes.GetByIndex(iPreConcavePt);
		VERTEX* pCurrVertex = tem_vertes.GetByIndex((iPreConcavePt + 1) % nNum);
		if (DISTANCE(pPrevVertex->pos, pCurrVertex->pos) < minDistance * 3)
		{
			for (VERTEX* pVer = tem_vertes.GetFirst(); pVer; pVer = tem_vertes.GetNext())
			{
				if (pVer == pPrevVertex || pVer == pCurrVertex)
					tem_vertes.DeleteCursor();
			}
			tem_vertes.Clean();
		}
	}
	//对直线进行延展
	for(VERTEX *vertex = tem_vertes.GetFirst();vertex;vertex= tem_vertes.GetNext())
	{
		VERTEX *vertexPre = tem_vertes.GetPrev();
		if (vertexPre == NULL)
		{
			vertexPre = tem_vertes.GetTail();
			tem_vertes.GetFirst();
		}
		else
			tem_vertes.GetNext();
		VERTEX *vertexNext = tem_vertes.GetNext();
		if (vertexNext == NULL)
		{
			vertexNext = tem_vertes.GetFirst();
			tem_vertes.GetTail();
		}
		else
			tem_vertes.GetPrev();
		f3dPoint curPt = vertex->pos;
		f3dPoint prePt = vertexPre->pos;
		f3dPoint nextPt = vertexNext->pos;
		f3dPoint preLineVec = curPt - prePt;
		normalize(preLineVec);
		f3dPoint nextLineVec = curPt - nextPt;
		normalize(nextLineVec);
		double angle = cal_angle_of_2vec(preLineVec,nextLineVec);
		double offset = minDistance/sin(angle/2);
		f3dPoint vec = preLineVec + nextLineVec;
		normalize(vec);
		if(DistOf2dPtLine(nextPt,prePt,curPt)<0)
			vec*=-1;	//凹点调整偏移方向
		//添加新轮廓点并增大轮廓点 wht 19-08-14
		VERTEX *pDestVertex = pDestList->append();
		pDestVertex->pos = GEPOINT(curPt + vec * offset);
	}
}
f2dRect CPlateProcessInfo::GetMinWrapRect(double minDistance/*=0*/,fPtList *pVertexList/*=NULL*/)
{
	f2dRect rect;
	rect.SetRect(f2dPoint(0,0),f2dPoint(0,0));
	int i = 0, n=vertexList.GetNodeNum();
	if (n < 3)
		return rect;
	GECS tmucs,minUcs;
	SCOPE_STRU scope,min_scope;
	typedef VERTEX* VERTEX_PTR;
	ATOM_LIST<VERTEX> list;
	if (minDistance > EPS)
		CalEquidistantShape(minDistance,&list);
	else
	{
		for(VERTEX *pVertex=vertexList.GetFirst();pVertex;pVertex=vertexList.GetNext())
			list.append(*pVertex);
	}
	n = list.GetNodeNum();
	//定义轮廓点数组时使用list.GetNodeNum(计算等距点时会将圆弧按小直线段处理),否则会导致内存越界死机 wht 19-08-18
	DYN_ARRAY<VERTEX_PTR> vertex_arr;
	vertex_arr.Resize(n);
	for(VERTEX *pVertex=list.GetFirst();pVertex;pVertex=list.GetNext(),i++)
		vertex_arr[i]=pVertex;
	tmucs.axis_z.Set(0,0,1);
	double minarea=10000000000000000;	//预置任一大数
	fPtList ptList;
	fPtList minRectPtList;
	for(i=0;i<n;i++)
	{
		tmucs.axis_x=vertex_arr[(i+1)%n]->pos-vertex_arr[i]->pos;
		if(tmucs.axis_x.IsZero())
			continue;	//接近重点
		tmucs.origin=vertex_arr[i]->pos;
		tmucs.axis_x.z = tmucs.origin.z = 0;
		tmucs.axis_y=tmucs.axis_z^tmucs.axis_x;
		normalize(tmucs.axis_x);
		normalize(tmucs.axis_y);
		if (vertex_arr[i]->m_bRollEdge)
			tmucs.origin -= tmucs.axis_y*vertex_arr[i]->manu_space;
		scope.ClearScope();
		ptList.Empty();
		for(int j=0;j<n;j++)
		{
			f3dPoint vertice = vertex_arr[j]->pos;
			coord_trans(vertice, tmucs, FALSE);
			scope.VerifyVertex(vertice);
			ptList.append(vertice);
			if(vertex_arr[j]->ciEdgeType>1)
			{
				f3dPoint ptS = vertex_arr[j]->pos;
				f3dPoint ptE = vertex_arr[(j + 1) % n]->pos;
				f3dArcLine arcLine;
				if (vertex_arr[j]->ciEdgeType == 2)	//指定圆弧夹角
					arcLine.CreateMethod2(ptS,ptE,vertex_arr[j]->arc.work_norm,vertex_arr[j]->arc.fSectAngle);
				else if (vertex_arr[j]->ciEdgeType == 3)
					arcLine.CreateEllipse(vertex_arr[j]->arc.center,ptS,ptE,vertex_arr[j]->arc.column_norm,
											vertex_arr[j]->arc.work_norm,vertex_arr[j]->arc.radius);
				//计算由于圆弧带来的面积区域变化
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					f3dPoint pos = arcLine.PositionInAngle(angle*i);
					coord_trans(pos,tmucs,FALSE);
					scope.VerifyVertex(pos);
					ptList.append(pos);
				}
				coord_trans(ptS, tmucs, FALSE);
				scope.VerifyVertex(ptS);
				coord_trans(ptE, tmucs, FALSE);
				scope.VerifyVertex(ptE);
				ptList.append(ptS);
				ptList.append(ptE);
			}
		}
		if(minarea>scope.wide()*scope.high())
		{
			minarea=scope.wide()*scope.high();
			min_scope=scope;
			minUcs = tmucs;
			datumStartVertex.index=i;
			datumEndVertex.index=(i+1)%n;
			minRectPtList.Empty();
			for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
				minRectPtList.append(*pPt);
		}
	}
	if (pVertexList)
	{
		pVertexList->Empty();
		for (f3dPoint *pPt = minRectPtList.GetFirst(); pPt; pPt = minRectPtList.GetNext())
			pVertexList->append(*pPt);
	}

	rect.topLeft.Set(min_scope.fMinX,min_scope.fMaxY);
	rect.bottomRight.Set(min_scope.fMaxX,min_scope.fMinY);
	datumStartVertex.srcPos=vertex_arr[datumStartVertex.index]->pos;
	datumEndVertex.srcPos=vertex_arr[datumEndVertex.index]->pos;
	GEPOINT topLeft(rect.topLeft.x,rect.topLeft.y,0);
	GEPOINT startPt=datumStartVertex.srcPos,endPt=datumEndVertex.srcPos;
	coord_trans(startPt,minUcs,FALSE);
	coord_trans(endPt,minUcs,FALSE);
	datumStartVertex.offsetPos=startPt-topLeft;
	datumEndVertex.offsetPos=endPt-topLeft;
	return rect;
}
bool CPlateProcessInfo::InitLayoutVertexByBottomEdgeIndex(f2dRect &rect)
{
	if (xPlate.mcsFlg.ciBottomEdge == 0xFF)
		InitBtmEdgeIndex();
	rect.SetRect(f2dPoint(0, 0), f2dPoint(0, 0));
	if (!IsValid())
		return false;
	int n = vertexList.GetNodeNum();
	int iCurIndex = xPlate.mcsFlg.ciBottomEdge;
	if (iCurIndex == 0xFF)
	{	//轮廓边未初始化,无法继续 wht 19-11-11
		return false;
	}
	SCOPE_STRU scope;
	DYN_ARRAY<VERTEX*> vertex_arr;
	vertex_arr.Resize(vertexList.GetNodeNum());
	ATOM_LIST<VERTEX> list;
	for (VERTEX *pVertex = vertexList.GetFirst(); pVertex; pVertex = vertexList.GetNext())
		list.append(*pVertex);
	int index = 0;
	for (VERTEX *pVertex = list.GetFirst(); pVertex; pVertex = list.GetNext(), index++)
		vertex_arr[index] = pVertex;
	GECS tmucs;
	tmucs.axis_z.Set(0, 0, 1);
	tmucs.axis_x = vertex_arr[(iCurIndex + 1) % n]->pos - vertex_arr[iCurIndex]->pos;
	if (tmucs.axis_x.IsZero())
		return false;	//接近重点
	tmucs.origin = vertex_arr[iCurIndex]->pos;
	tmucs.axis_x.z = tmucs.origin.z = 0;
	tmucs.axis_y = tmucs.axis_z^tmucs.axis_x;
	normalize(tmucs.axis_x);
	normalize(tmucs.axis_y);
	if (vertex_arr[iCurIndex]->m_bRollEdge)
		tmucs.origin -= tmucs.axis_y*vertex_arr[iCurIndex]->manu_space;
	scope.ClearScope();
	for (int j = 0; j < n; j++)
	{
		f3dPoint vertice = vertex_arr[j]->pos;
		coord_trans(vertice, tmucs, FALSE);
		scope.VerifyVertex(vertice);
		if (vertex_arr[j]->ciEdgeType > 1)
		{
			f3dPoint ptS = vertex_arr[j]->pos;
			f3dPoint ptE = vertex_arr[(j + 1) % n]->pos;
			f3dArcLine arcLine;
			if (vertex_arr[j]->ciEdgeType == 2)	//指定圆弧夹角
				arcLine.CreateMethod2(ptS, ptE, vertex_arr[j]->arc.work_norm, vertex_arr[j]->arc.fSectAngle);
			else if (vertex_arr[j]->ciEdgeType == 3)	//椭圆弧
				arcLine.CreateEllipse(vertex_arr[j]->arc.center, ptS, ptE, vertex_arr[j]->arc.column_norm,
					vertex_arr[j]->arc.work_norm, vertex_arr[j]->arc.radius);
			//计算由于圆弧带来的面积区域变化
			int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
			double angle = arcLine.SectorAngle() / nSlices;
			for (int i = 1; i < nSlices; i++)
			{
				f3dPoint pos = arcLine.PositionInAngle(angle*i);
				coord_trans(pos, tmucs, FALSE);
				scope.VerifyVertex(pos);
			}
			coord_trans(ptS, tmucs, FALSE);
			scope.VerifyVertex(ptS);
			coord_trans(ptE, tmucs, FALSE);
			scope.VerifyVertex(ptE);
		}
	}
	datumStartVertex.index = iCurIndex;
	datumEndVertex.index = (iCurIndex + 1) % n;
	//
	rect.topLeft.Set(scope.fMinX, scope.fMaxY);
	rect.bottomRight.Set(scope.fMaxX, scope.fMinY);
	datumStartVertex.srcPos = vertexList[datumStartVertex.index].pos;
	datumEndVertex.srcPos = vertexList[datumEndVertex.index].pos;
	GEPOINT topLeft(rect.topLeft.x, rect.topLeft.y, 0);
	GEPOINT startPt = datumStartVertex.srcPos, endPt = datumEndVertex.srcPos;
	coord_trans(startPt, tmucs, FALSE);
	coord_trans(endPt, tmucs, FALSE);
	datumStartVertex.offsetPos = startPt - topLeft;
	datumEndVertex.offsetPos = endPt - topLeft;
	return true;
}
void CPlateProcessInfo::InitEdgeEntIdMap()
{
	m_hashCloneEdgeEntIdByIndex.Empty();
	for(int i=0;i<vertexList.GetNodeNum();i++)
	{
		VERTEX *pCurVertex = vertexList.GetByIndex(i);
		if (pCurVertex->tag.lParam == 0 || pCurVertex->tag.lParam==-1)
			continue;
		ULONG *pCloneEntId = m_hashColneEntIdBySrcId.GetValue(pCurVertex->tag.dwParam);
		if(pCloneEntId==NULL)
			continue;
		CAD_LINE lineId(*pCloneEntId);
		if (lineId.UpdatePos())
		{
			if (lineId.m_ptEnd.IsEqual(pCurVertex->pos, EPS2))
			{
				GEPOINT temp = lineId.m_ptStart;
				lineId.m_ptStart = lineId.m_ptEnd;
				lineId.m_ptEnd = temp;
				lineId.m_bReverse = TRUE;
			}
			m_hashCloneEdgeEntIdByIndex.SetValue(i + 1, lineId);
		}
	}
}

//根据克隆钢板中MKPOS位置，更新钢印号位置 wht 19-03-05
bool CPlateProcessInfo::SyncSteelSealPos()
{
	if (m_hashCloneEdgeEntIdByIndex.GetNodeNum() <= 0)
		return false;
	//1.获取Clone实体前两条边
	CAD_LINE *pFirstLineId = m_hashCloneEdgeEntIdByIndex.GetValue(1);
	CAD_LINE *pSecondLineId = m_hashCloneEdgeEntIdByIndex.GetValue(2);
	if (pFirstLineId == NULL || pSecondLineId == NULL)
		return false;
	//2.获取Src实体对应的前两条边
	AcDbObjectId srcFirstId=0, srcSecondId=0;
	for (ULONG *pId = m_hashColneEntIdBySrcId.GetFirst(); pId; pId = m_hashColneEntIdBySrcId.GetNext())
	{
		long idSrcEnt= m_hashColneEntIdBySrcId.GetCursorKey();
		if (*pId == pFirstLineId->idCadEnt)
			srcFirstId = MkCadObjId(idSrcEnt);
		else if (*pId == pSecondLineId->idCadEnt)
			srcSecondId = MkCadObjId(idSrcEnt);
		if (srcFirstId.asOldId() != 0 && srcSecondId.asOldId() != 0)
			break;
	}
	if (srcFirstId.asOldId() == 0 || srcSecondId.asOldId() == 0)
		return false;
	//3.建立坐标系
	UCS_STRU src_ucs, clone_ucs;
	pFirstLineId->UpdatePos();
	pSecondLineId->UpdatePos();
	clone_ucs.origin = pFirstLineId->m_ptEnd;
	clone_ucs.axis_x = pSecondLineId->m_ptEnd - pSecondLineId->m_ptStart;
	clone_ucs.axis_y = pFirstLineId->m_ptStart - pFirstLineId->m_ptEnd;
	normalize(clone_ucs.axis_x);
	normalize(clone_ucs.axis_y);
	clone_ucs.axis_z = clone_ucs.axis_x^clone_ucs.axis_y;
	normalize(clone_ucs.axis_z);
	clone_ucs.axis_y = clone_ucs.axis_z^clone_ucs.axis_x;
	normalize(clone_ucs.axis_y);
	//
	CAD_LINE srcFirstLineId, srcSecondLineId;
	srcFirstLineId.idCadEnt = srcFirstId.asOldId();
	srcFirstLineId.m_bReverse = pFirstLineId->m_bReverse;
	srcFirstLineId.UpdatePos();
	srcSecondLineId.idCadEnt = srcSecondId.asOldId();
	srcSecondLineId.m_bReverse = pSecondLineId->m_bReverse;
	srcSecondLineId.UpdatePos();
	src_ucs.origin = srcFirstLineId.m_ptEnd;
	src_ucs.axis_x = srcSecondLineId.m_ptEnd - srcSecondLineId.m_ptStart;
	src_ucs.axis_y = srcFirstLineId.m_ptStart - srcFirstLineId.m_ptEnd;
	normalize(src_ucs.axis_x);
	normalize(src_ucs.axis_y);
	src_ucs.axis_z = src_ucs.axis_x^src_ucs.axis_y;
	normalize(src_ucs.axis_z);
	src_ucs.axis_y = src_ucs.axis_z^src_ucs.axis_x;
	normalize(src_ucs.axis_y);
	//
	AcDbEntity *pEnt = NULL;
	if (m_xMkDimPoint.idCadEnt != 0)
	{
		Acad::ErrorStatus es=XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
		if (es != Acad::eOk)
		{
			if (pEnt)
				pEnt->close();
			return false;
		}
	}
	CAcDbObjLife life(pEnt);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	GEPOINT mk_pos(pPoint->position().x, pPoint->position().y, pPoint->position().z);
	coord_trans(mk_pos, clone_ucs, FALSE);
	coord_trans(mk_pos, src_ucs, TRUE);
	GEPOINT mk_vec(1,0,0);
	vector_trans(mk_vec, clone_ucs, FALSE);
	vector_trans(mk_vec, src_ucs, TRUE);
	xPlate.mkpos = mk_pos;
	xPlate.mkVec = mk_vec;
	return true;
}

bool CPlateProcessInfo::AutoCorrectedSteelSealPos()
{
	ARRAY_LIST<AcDbObjectId> entIdList;
	for (ULONG *pId = m_cloneEntIdList.GetFirst(); pId; pId = m_cloneEntIdList.GetNext())
		entIdList.append(MkCadObjId(*pId));
	f2dRect rect = GetCadEntRect(entIdList);
	f2dPoint leftBtm(rect.topLeft.x, rect.bottomRight.y);
	//检查钢印字盒是否超出范围
	GEPOINT pos;
	GetSteelSealPos(pos);
	double fHalfW = g_pncSysPara.m_nMkRectWidth*0.5;
	if (pos.y - leftBtm.y < fHalfW)
	{
		pos.y = leftBtm.y + fHalfW * 1.1;
		UpdateSteelSealPos(pos);
		return true;
	}
	else
		return false;
}

bool CPlateProcessInfo::GetSteelSealPos(GEPOINT &pos)
{
	AcDbEntity* pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
	CAcDbObjLife objLife(pEnt);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	m_xMkDimPoint.pos.Set(pPoint->position().x, pPoint->position().y, 0);
	pos = m_xMkDimPoint.pos;
	return true;
}

bool CPlateProcessInfo::UpdateSteelSealPos(GEPOINT &pos)
{
	AcDbEntity* pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForWrite);
	CAcDbObjLife life(pEnt);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	pPoint->setPosition(AcGePoint3d(pos.x, pos.y, pos.z));
	m_xMkDimPoint.pos.Set(pos.x, pos.y, 0);

	SyncSteelSealPos();
	return true;
}
//更新钢板加工数
void CPlateProcessInfo::RefreshPlateNum()
{
	if (partNumId == NULL)
		return;
	CLockDocumentLife lockCurDocLife;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, partNumId, AcDb::kForWrite);
	CAcDbObjLife entLife(pEnt);
	CXhChar100 sValueG, sValueS, sValueM;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		sValueG.Copy(_bstr_t(pText->textString()));
#else
		sValueG.Copy(pText->textString());
#endif
		if (g_pncSysPara.m_iDimStyle == 1 &&
			((strstr(sValueG, "数量:") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "数量:") != NULL) ||
			(strstr(sValueG, "数量：") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "数量：") != NULL)))
		{	//修改钢板加工数 wht 19-08-05
			sValueS.Printf("%s%d", (char*)g_pncSysPara.m_sPnNumKey, xBomPlate.feature1);
		}
		else
		{
			for (char* sKey = strtok(sValueG, " \t"); sKey; sKey = strtok(NULL, " \t"))
			{
				if (strstr(sKey, "Q"))
				{
					sValueM.Copy(sKey);
					break;
				}
			}
			sValueS.Printf("-%.0f %s %d件", xPlate.m_fThick, (char*)sValueM, xBomPlate.feature1);
		}
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sValueS));
#else
		pText->setTextString(sValueS);
#endif
		//修改加工数后设置为红色 wht 20-07-29
		int color_index = GetNearestACI(RGB(255, 0, 0));
		pText->setColorIndex(color_index);
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar500 sContents;
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sContents.Copy(_bstr_t(pMText->contents()));
#else
		sContents.Copy(pMText->contents());
#endif
		CString sText(sContents);
		for (char* sKey = strtok(sContents, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CXhChar200 sTemp(sKey);
			if (g_pncSysPara.m_iDimStyle == 1 &&
				((strstr(sTemp, "数量:") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "数量:") != NULL) ||
				(strstr(sTemp, "数量：") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "数量：") != NULL)))
			{
				sTemp.Replace("\\P", "");
				sValueS.Printf("%s%d", (char*)g_pncSysPara.m_sPnNumKey, xBomPlate.feature1);
				sText.Replace(sTemp, sValueS);	//更新数量行 wht 19-08-13
#ifdef _ARX_2007
				pMText->setContents(_bstr_t(sText));
#else
				pMText->setContents(sText);
#endif
				//修改加工数后设置为红色 wht 20-07-29
				int color_index = GetNearestACI(RGB(255, 0, 0));
				pMText->setColorIndex(color_index);
				break;
			}
			
		}
	}
	m_ciModifyState |= MODIFY_MANU_NUM;
}
//更新钢板规格
void CPlateProcessInfo::RefreshPlateSpec()
{
	if (partNoId == NULL)
		return;
	CLockDocumentLife lockCurDocLife;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, partNoId, AcDb::kForWrite);
	CAcDbObjLife entLife(pEnt);
	CXhChar100 sValueG, sValueS, sValueM, sValuePn;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		sValueG.Copy(_bstr_t(pText->textString()));
#else
		sValueG.Copy(pText->textString());
#endif
		if (g_pncSysPara.m_iDimStyle == 1 &&
			((strstr(sValueG, "规格:") != NULL && strstr(g_pncSysPara.m_sThickKey, "规格:") != NULL) ||
			(strstr(sValueG, "规格：") != NULL && strstr(g_pncSysPara.m_sThickKey, "规格：") != NULL)))
		{	//修改钢板加工数 wht 19-08-05
			sValueS.Printf("%s%.0f", (char*)g_pncSysPara.m_sThickKey, xBomPlate.thick);
		}
		else if (strstr(sValueG, "#") != NULL && strstr(g_pncSysPara.m_sPnKey, "#") != NULL)
		{
			sValuePn = strtok(sValueG, "#");
			for (char* sKey = strtok(NULL, " "); sKey; sKey = strtok(NULL, " "))
			{
				if (strstr(sKey, "Q"))
				{
					sValueM.Copy(sKey);
					break;
				}
			}
			sValueS.Printf("%s#%s -%.0f ", (char*)sValuePn, (char*)sValueM, xBomPlate.thick);
		}
		else
		{
			for (char* sKey = strtok(sValueG, " \t"); sKey; sKey = strtok(NULL, " \t"))
			{
				if (strstr(sKey, "Q"))
				{
					sValueM.Copy(sKey);
					break;
				}
			}
			sValueS.Printf("-%.0f %s %d件", xPlate.m_fThick, (char*)sValueM, xBomPlate.feature1);
		}
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sValueS));
#else
		pText->setTextString(sValueS);
#endif
		//修改加工数后设置为红色 wht 20-07-29
		int color_index = GetNearestACI(RGB(255, 0, 0));
		pText->setColorIndex(color_index);
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar500 sContents;
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sContents.Copy(_bstr_t(pMText->contents()));
#else
		sContents.Copy(pMText->contents());
#endif
		CString sText(sContents);
		for (char* sKey = strtok(sContents, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CXhChar200 sTemp(sKey);
			if (g_pncSysPara.m_iDimStyle == 1 &&
				((strstr(sTemp, "规格:") != NULL && strstr(g_pncSysPara.m_sThickKey, "规格:") != NULL) ||
				(strstr(sTemp, "规格：") != NULL && strstr(g_pncSysPara.m_sThickKey, "规格：") != NULL)))
			{
				sTemp.Replace("\\P", "");
				sValueS.Printf("%s%.0f", (char*)g_pncSysPara.m_sThickKey, xBomPlate.thick);
				sText.Replace(sTemp, sValueS);	//更新数量行 wht 19-08-13
#ifdef _ARX_2007
				pMText->setContents(_bstr_t(sText));
#else
				pMText->setContents(sText);
#endif
				//修改加工数后设置为红色 wht 20-07-29
				int color_index = GetNearestACI(RGB(255, 0, 0));
				pMText->setColorIndex(color_index);
				break;
			}

		}
	}
	m_ciModifyState |= MODIFY_DES_SPEC;
}
//更新钢板材质
void CPlateProcessInfo::RefreshPlateMat()
{
	if (partNoId == NULL)
		return;
	CLockDocumentLife lockCurDocLife;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, partNoId, AcDb::kForWrite);
	CAcDbObjLife entLife(pEnt);
	CXhChar100 sValueG, sValueS, sValueM, sValuePn;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		sValueG.Copy(_bstr_t(pText->textString()));
#else
		sValueG.Copy(pText->textString());
#endif
		if (g_pncSysPara.m_iDimStyle == 1 &&
			((strstr(sValueG, "材质:") != NULL && strstr(g_pncSysPara.m_sMatKey, "材质:") != NULL) ||
			(strstr(sValueG, "材质：") != NULL && strstr(g_pncSysPara.m_sMatKey, "材质：") != NULL)))
		{	//修改钢板加工数 wht 19-08-05
			sValueS.Printf("%s", xBomPlate.sMaterial);
		}
		else if (strstr(sValueG, "#") != NULL && strstr(g_pncSysPara.m_sPnKey, "#") != NULL)
		{
			sValuePn = strtok(sValueG, "#");
			sValueS.Printf("%s#%s -%.0f ", (char*)sValuePn, (char*)xBomPlate.sMaterial, xBomPlate.thick);
		}
		else
		{
			for (char* sKey = strtok(sValueG, " \t"); sKey; sKey = strtok(NULL, " \t"))
			{
				if (strstr(sKey, "Q"))
				{
					sValueM.Copy(sKey);
					break;
				}
			}
			sValueS.Printf("-%.0f %s %d件", xPlate.m_fThick, (char*)xBomPlate.sMaterial, xBomPlate.feature1);
		}
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sValueS));
#else
		pText->setTextString(sValueS);
#endif
		//修改加工数后设置为红色 wht 20-07-29
		int color_index = GetNearestACI(RGB(255, 0, 0));
		pText->setColorIndex(color_index);
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar500 sContents;
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sContents.Copy(_bstr_t(pMText->contents()));
#else
		sContents.Copy(pMText->contents());
#endif
		CString sText(sContents);
		for (char* sKey = strtok(sContents, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CXhChar200 sTemp(sKey);
			if (g_pncSysPara.m_iDimStyle == 1 &&
				((strstr(sTemp, "材质:") != NULL && strstr(g_pncSysPara.m_sMatKey, "材质:") != NULL) ||
				(strstr(sTemp, "材质：") != NULL && strstr(g_pncSysPara.m_sMatKey, "材质：") != NULL)))
			{
				sTemp.Replace("\\P", "");
				sValueS.Printf("%s",xBomPlate.sMaterial);
				sText.Replace(sTemp, sValueS);	//更新数量行 wht 19-08-13
#ifdef _ARX_2007
				pMText->setContents(_bstr_t(sText));
#else
				pMText->setContents(sText);
#endif
				//修改加工数后设置为红色 wht 20-07-29
				int color_index = GetNearestACI(RGB(255, 0, 0));
				pMText->setColorIndex(color_index);
				break;
			}

		}
	}
	m_ciModifyState |= MODIFY_DES_MAT;
}
SCOPE_STRU CPlateProcessInfo::GetCADEntScope(BOOL bIsColneEntScope /*= FALSE*/)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	if (bIsColneEntScope)
	{
		for (unsigned long *pId = m_cloneEntIdList.GetFirst(); pId; pId = m_cloneEntIdList.GetNext())
			VerifyVertexByCADEntId(scope, MkCadObjId(*pId));
	}
	else if (m_xHashRelaEntIdList.GetNodeNum() > 1)
	{
		for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
			VerifyVertexByCADEntId(scope, MkCadObjId(pEnt->idCadEnt));
	}
	else
		scope = GetPlateScope(TRUE);
	return scope;
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
	double fDiameter = pCircle->radius() * 2;
	if (g_pncSysPara.m_ciMKPos == 2 &&
		fabs(g_pncSysPara.m_fMKHoleD - fDiameter) < EPS2)
		return false;	//去除号位孔
	//添加螺栓块
	GEPOINT center(pCircle->center().x, pCircle->center().y, pCircle->center().z);
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
	std::set<CString> setKeyStr;
	std::map<CString,CAD_LINE> mapLineByPosStr;		//
	std::multimap<CString, CAD_ENTITY> mapTriangle;
	std::multimap<CString, CAD_ENTITY> mapSquare;
	std::multimap<CString, CAD_ENTITY> mapRound;
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
			CAD_ENTITY xEntity;
			xEntity.idCadEnt = vectorConnLine[i].idCadEnt;
			xEntity.m_fSize = fLen;
			xEntity.pos = ptM + up_off_vec * (0.5*g_pncSysPara.m_fRoundLineLen);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapRound.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
			xEntity.pos = ptM + dw_off_vec * (0.5*g_pncSysPara.m_fRoundLineLen);
			setKeyStr.insert(MakePosKeyStr(xEntity.pos));
			mapRound.insert(std::make_pair(MakePosKeyStr(xEntity.pos), xEntity));
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
		//处理腰圆图形
		if (mapRound.count(sKey) == 2)
		{
			ITERATOR begIter = mapRound.lower_bound(sKey);
			ITERATOR endIter = mapRound.upper_bound(sKey);
			CBoltEntGroup* pBoltEnt = m_xBoltEntHash.GetValue(sKey);
			if (pBoltEnt == NULL)
			{
				pBoltEnt = m_xBoltEntHash.Add(sKey);
				pBoltEnt->m_ciType = CBoltEntGroup::BOLT_WAIST_ROUND;
				pBoltEnt->m_fPosX = (float)begIter->second.pos.x;
				pBoltEnt->m_fPosY = (float)begIter->second.pos.y;
				pBoltEnt->m_fHoleD = (float)StandardHoleD(begIter->second.m_fSize);
				while (begIter != endIter)
				{
					pBoltEnt->m_xArcArr.push_back(begIter->second.idCadEnt);
					begIter++;
				}
			}
		}
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
			if (len < 10 || len>30)
				continue;	//过滤过小或过长线段
			if (pEnt->isKindOf(AcDbArc::desc()))
			{	//
				AcDbArc* pArc = (AcDbArc*)pEnt;
				double fAngle = fabs(pArc->endAngle() - pArc->startAngle());
				if (fabs(fAngle - Pi) > EPS2)
					continue;	//非180度的圆弧
			}
			vectorLine.push_back(CAD_LINE(objId, ptS, ptE));
			mapLineNumInPos[MakePosKeyStr(ptS)] += 1;
			mapLineNumInPos[MakePosKeyStr(ptE)] += 1;
		}
		//分别获取独立直线集合和连接直线结合
		std::vector<CAD_LINE> vectorConnLine, vectorAloneLine;
		for (size_t ii = 0; ii < vectorLine.size(); ii++)
		{
			ITERATOR iterS = mapLineNumInPos.find(MakePosKeyStr(vectorLine[ii].m_ptStart));
			ITERATOR iterE = mapLineNumInPos.find(MakePosKeyStr(vectorLine[ii].m_ptEnd));
			if (iterS != mapLineNumInPos.end() && iterE != mapLineNumInPos.end())
			{
				if (iterS->second == 1 && iterE->second == 1)
					vectorAloneLine.push_back(vectorLine[ii]);
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
	ATOM_LIST<CAD_ENTITY> cadCircleList;
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
			CAD_ENTITY* pCadCir = cadCircleList.append();
			pCadCir->ciEntType = TYPE_CIRCLE;
			pCadCir->m_fSize = pCir->radius();
			Cpy_Pnt(pCadCir->pos, pCir->center());
			//目前圆不做轮廓边处理
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
		for (CAD_ENTITY *pCir = cadCircleList.GetFirst(); pCir; pCir = cadCircleList.GetNext())
		{
			if (pCir->IsInScope(pInfo->dim_pos))
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
	xImage.WriteBmpFileByVertStrip("F:\\11.bmp");
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
			pCurPlateInfo->vertexList.Empty();
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
			pCurPlateInfo->vertexList.Empty();
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
					pCurPlateInfo->vertexList.Empty();
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
	if(m_sPrjName.GetLength()>0)
		fprintf(fp,"PROJECT_NAME=%s\n",(char*)m_sPrjName);
	if(m_sPrjCode.GetLength()>0)
		fprintf(fp,"PROJECT_CODE=%s\n",(char*)m_sPrjCode);
	if(m_sTaType.GetLength()>0)
		fprintf(fp,"TOWER_NAME=%s\n",(char*)m_sTaType);
	if(m_sTaAlias.GetLength()>0)
		fprintf(fp,"TOWER_CODE=%s\n",(char*)m_sTaAlias);
	if(m_sTaStampNo.GetLength()>0)
		fprintf(fp,"STAMP_NO=%s\n",(char*)m_sTaStampNo);
	fclose(fp);
}
