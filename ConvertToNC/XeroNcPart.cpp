#include "stdafx.h"
#include "XeroNcPart.h"
#include "CadToolFunc.h"
#include "PNCCryptCoreCode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//
CAD_ENTITY* __AppendRelaEntity(AcDbEntity *pEnt,CHashList<CAD_ENTITY> *pHashRelaEntIdList /*= NULL*/)
{
	if (pEnt == NULL || pHashRelaEntIdList == NULL)
		return NULL;
	CAD_ENTITY* pRelaEnt = pHashRelaEntIdList->GetValue(pEnt->id().asOldId());
	if (pRelaEnt == NULL)
	{
		pRelaEnt = pHashRelaEntIdList->Add(pEnt->id().asOldId());
		pRelaEnt->idCadEnt = pEnt->id().asOldId();
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
			pRelaEnt->m_fSize = axis.mod() * 2;
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
			if (GetCadTextEntPos(pText, pos, false))
				pRelaEnt->pos.Set(pos.x, pos.y, pos.z);
		}
		else if (pEnt->isKindOf(AcDbMText::desc()))
			pRelaEnt->ciEntType = TYPE_MTEXT;
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{
			pRelaEnt->ciEntType = TYPE_BLOCKREF;
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
			strcpy(pRelaEnt->sText, sDimText);
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
			pRelaEnt->ciEntType = TYPE_OTHER;
	}
	return pRelaEnt;
}
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
//CBoltEntGroup
CBoltEntGroup::CBoltEntGroup()
{
	m_ciType = 0;
	m_idEnt = 0;
	m_bMatch = FALSE;
	m_fPosX = m_fPosY = 0;
	m_fHoleD = 0;
}
//
double CBoltEntGroup::GetWaistLen()
{
	if (m_ciType != BOLT_WAIST_ROUND)
		return 0;
	if (m_xLineArr.size() != 2)
		return 0;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xLineArr[0]), AcDb::kForRead);
	CAcDbObjLife life(pEnt);
	if (!pEnt->isKindOf(AcDbLine::desc()))
		return 0;
	GEPOINT ptS, ptE;
	AcDbLine *pLine = (AcDbLine*)pEnt;
	ptS.Set(pLine->startPoint().x, pLine->startPoint().y, 0);
	ptE.Set(pLine->endPoint().x, pLine->endPoint().y, 0);
	return DISTANCE(ptS, ptE);
}
GEPOINT CBoltEntGroup::GetWaistVec()
{
	GEPOINT vec;
	if (m_ciType != BOLT_WAIST_ROUND)
		return vec;
	if (m_xLineArr.size() != 2)
		return vec;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xLineArr[0]), AcDb::kForRead);
	CAcDbObjLife life(pEnt);
	if (!pEnt->isKindOf(AcDbLine::desc()))
		return vec;
	GEPOINT ptS, ptE;
	AcDbLine *pLine = (AcDbLine*)pEnt;
	ptS.Set(pLine->startPoint().x, pLine->startPoint().y, 0);
	ptE.Set(pLine->endPoint().x, pLine->endPoint().y, 0);
	return (ptE - ptS).normalized();
}
//////////////////////////////////////////////////////////////////////////
//CCadPartObject
CCadPartObject::CCadPartObject()
{

}
CCadPartObject::~CCadPartObject()
{

}
//判断坐标点是否在构件内
bool CCadPartObject::IsInPartRgn(const double* poscoord)
{
	if (m_xPolygon.GetVertexCount() < 3)
		CreateRgn();
	if (m_xPolygon.GetAxisZ().IsZero())
		return false;
	int iRet = m_xPolygon.PtInRgn2(poscoord);
	if (iRet == 1 || iRet == 2)
		return true;
	return false;
}
//判断直线是否在构件内
bool CCadPartObject::IsInPartRgn(const double* start, const double* end)
{
	if (m_xPolygon.GetVertexCount() < 3)
		CreateRgn();
	if (m_xPolygon.GetAxisZ().IsZero())
		return false;
	int iRet = m_xPolygon.LineInRgn2(start, end);
	if (iRet == 1)
		return true;
	else if (iRet == 2)
	{	//部分在多边形区域内
		f3dPoint pt, inters1, inters2;
		for (int i = 0; i < m_xPolygon.GetVertexCount(); i++)
		{
			GEPOINT prePt = m_xPolygon.GetVertexAt(i);
			GEPOINT curPt = m_xPolygon.GetVertexAt((i + 1) % m_xPolygon.GetVertexCount());
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
		else if (IsInPartRgn(start))
			fExternLen = DISTANCE(GEPOINT(start), inters1);
		else if (IsInPartRgn(end))
			fExternLen = DISTANCE(GEPOINT(end), inters1);
		if (ftoi(fExternLen) <= ftoi(fSumLen) && fExternLen / fSumLen > LEN_SCALE)
			return true;
		else
			return false;
	}
	else
		return false;
}
//根据构件区域提取关联图元
void CCadPartObject::ExtractRelaEnts(vector<GEPOINT>& ptArr)
{
	struct resbuf* pList = NULL, *pPoly = NULL;
	for(size_t i=0;i<ptArr.size();i++)
	{
		if (pList == NULL)
			pPoly = pList = acutNewRb(RTPOINT);
		else
		{
			pList->rbnext = acutNewRb(RTPOINT);
			pList = pList->rbnext;
		}
		pList->restype = RTPOINT;
		pList->resval.rpoint[X] = ptArr[i].x;
		pList->resval.rpoint[Y] = ptArr[i].y;
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
			if (!IsInPartRgn(startPt, endPt))
				continue;
		}
		else if (pEnt->isKindOf(AcDbText::desc()))
		{	//过滤不在区域内的文本
			GEPOINT dimPt = GetCadTextDimPos(pEnt);
			if (!IsInPartRgn(dimPt))
				continue;
		}
		AppendRelaEntity(pEnt);
	}
}
//添加构件的关联图元
void CCadPartObject::AppendRelaEntity(long idEnt)
{
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(idEnt), AcDb::kForRead);
	if (pEnt)
	{
		AppendRelaEntity(pEnt);
		pEnt->close();
	}
}
void CCadPartObject::AppendRelaEntity(AcDbEntity* pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList /*= NULL*/)
{
	if (pEnt == NULL)
		return;
	if (pHashRelaEntIdList == NULL)
		pHashRelaEntIdList = &m_xHashRelaEntIdList;
	ASSERT(pHashRelaEntIdList != NULL);
	XhAppendRelaEntity(pEnt, pHashRelaEntIdList);
}
//删除关联图元
void CCadPartObject::EraseRelaEnts()
{
	if (m_xHashRelaEntIdList.GetNodeNum() <= 0)
		return;
	CLockDocumentLife lockCurDocLife;
	for (CAD_ENTITY* pRelaEnt = EnumFirstRelaEnt(); pRelaEnt; pRelaEnt = EnumNextRelaEnt())
	{
		AcDbEntity *pEnt = NULL;
		XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(pRelaEnt->idCadEnt), AcDb::kForWrite);
		if (pEnt)
		{
			pEnt->erase(Adesk::kTrue);
			pEnt->close();
		}
	}
	m_xHashRelaEntIdList.Empty();
}