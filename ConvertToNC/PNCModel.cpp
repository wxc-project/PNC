#include "stdafx.h"
#include "PNCModel.h"
#include "TSPAlgorithm.h"
#include "PNCSysPara.h"
#include "XeroExtractor.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "LayerTable.h"
#include "PolygonObject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  ASSIST_RADIUS	2

CPNCModel model;
CExpoldeModel g_explodeModel;
CDocManagerReactor *g_pDocManagerReactor;
//////////////////////////////////////////////////////////////////////////
//ACAD_LINEID
ACAD_LINEID::ACAD_LINEID(long lineId /*= 0*/)
{
	m_ciSerial = 0;
	m_lineId = lineId;
	m_fLen = 0;
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
ACAD_LINEID::ACAD_LINEID(AcDbObjectId id, double len)
{
	m_ciSerial = 0;
	m_fLen = len;
	m_lineId = id.asOldId();
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
void ACAD_LINEID::Init(AcDbObjectId id, GEPOINT &start, GEPOINT &end)
{
	m_ciSerial = 0;
	m_lineId = id.asOldId();
	m_ptStart = start;
	m_ptEnd = end;
	m_fLen = DISTANCE(m_ptStart, m_ptEnd);
	m_bReverse = FALSE;
	m_bMatch = FALSE;
}
BOOL ACAD_LINEID::UpdatePos()
{
	AcDbEntity *pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_lineId), AcDb::kForRead);
	CAcDbObjLife life(pEnt);
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbLine::desc()))
	{
		AcDbLine *pLine = (AcDbLine*)pEnt;
		m_ptStart.Set(pLine->startPoint().x, pLine->startPoint().y, 0);
		m_ptEnd.Set(pLine->endPoint().x, pLine->endPoint().y, 0);
	}
	else if (pEnt->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc = (AcDbArc*)pEnt;
		AcGePoint3d startPt, endPt;
		pArc->getStartPoint(startPt);
		pArc->getEndPoint(endPt);
		m_ptStart.Set(startPt.x, startPt.y, 0);
		m_ptEnd.Set(endPt.x, endPt.y, 0);
	}
	else if (pEnt->isKindOf(AcDbEllipse::desc()))
	{
		AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
		AcGePoint3d startPt, endPt;
		pEllipse->getStartPoint(startPt);
		pEllipse->getEndPoint(endPt);
		m_ptStart.Set(startPt.x, startPt.y, 0);
		m_ptEnd.Set(endPt.x, endPt.y, 0);
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
//CPlateProcessInfo
//AcDbObjectId MkCadObjId(unsigned long idOld){ return AcDbObjectId((AcDbStub*)idOld); }
CPlateProcessInfo::CPlateProcessInfo()
{
	m_fZoomScale=100;
	boltList.Empty();
	m_bIslandDetection=FALSE;
	plateInfoBlockRefId=0;
	partNoId=0;
	partNumId=0;
	m_bHasInnerDimPos=FALSE;
	m_bEnableReactor = TRUE;
	m_bNeedExtract = FALSE;
}
double RecogHoleDByBlockRef(AcDbBlockTableRecord *pTempBlockTableRecord, double scale); //From XeroExtractor.cpp
CPlateObject::CAD_ENTITY* CPlateProcessInfo::AppendRelaEntity(AcDbEntity *pEnt)
{
	if(pEnt==NULL)
		return NULL;
	long entId = pEnt->id().asOldId();
	long objectId = pEnt->objectId().asOldId();
	CAD_ENTITY* pRelaEnt=m_xHashRelaEntIdList.GetValue(pEnt->id().asOldId());
	if(pRelaEnt==NULL)
	{
		pRelaEnt=m_xHashRelaEntIdList.Add(pEnt->id().asOldId());
		pRelaEnt->idCadEnt=pEnt->id().asOldId();
		if (pEnt->isKindOf(AcDbLine::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_LINE;
		else if (pEnt->isKindOf(AcDbArc::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_ARC;
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
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_CIRCLE;
			pRelaEnt->m_fSize = fHoldD;
			pRelaEnt->pos.Set(center.x, center.y, center.z);
		}
		else if (pEnt->isKindOf(AcDbSpline::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_SPLINE;
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d center = pEllipse->center();
			AcGeVector3d minorAxis = pEllipse->minorAxis();
			double radiusRatio = pEllipse->radiusRatio();
			GEPOINT axis(minorAxis.x, minorAxis.y, minorAxis.z);
			pRelaEnt->m_fSize = axis.mod()*2;
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_ELLIPSE;
			pRelaEnt->pos.Set(center.x, center.y, center.z);
		}
		else if (pEnt->isKindOf(AcDbText::desc()))
		{
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_TEXT;
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
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_MTEXT;
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_BLOCKREF;
			AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
			AcDbObjectId blockId = pReference->blockTableRecord();
			AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
			acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
			if (pTempBlockTableRecord)
			{
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
				//记录图块的位置和大小，方便后期处理同一位置有多个图块的情况
				BOLT_BLOCK* pBoltD = g_pncSysPara.hashBoltDList.GetValue(sName);
				if (pBoltD)
				{
					strcpy(pRelaEnt->sText, sName);
					pRelaEnt->pos.x = (float)pReference->position().x;
					pRelaEnt->pos.y = (float)pReference->position().y;
					double fHoleD = RecogHoleDByBlockRef(pTempBlockTableRecord, pReference->scaleFactors().sx);
					if (fHoleD > 0)
						pRelaEnt->m_fSize = fHoleD;
				}
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
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_DIM_D;
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
			pRelaEnt->ciEntType=RELA_ACADENTITY::TYPE_OTHER;
	}
	return pRelaEnt;
}
int CPlateProcessInfo::GetRelaEntIdList(ARRAY_LIST<AcDbObjectId> &entIdList)
{
	entIdList.Empty();
	for(CPlateObject::CAD_ENTITY *pCadEnt=EnumFirstEnt();pCadEnt;pCadEnt=EnumNextEnt())
		entIdList.append(MkCadObjId(pCadEnt->idCadEnt));
	return (int)entIdList.Size();
}
//根据钢板的轮廓点初始化归属钢板的图元集合
void CPlateProcessInfo::InternalExtractPlateRelaEnts()
{
	m_xHashRelaEntIdList.Empty();
	m_xHashRelaEntIdList.SetValue(partNoId.asOldId(),CAD_ENTITY(partNoId.asOldId()));
	if(vertexList.GetNodeNum()<3)
		return;
	//根据标注位置进行缩放
	if (!dim_pos.IsZero() && g_pncSysPara.m_ciRecogMode != CPNCSysPara::FILTER_BY_PIXEL)
	{	
		f2dRect rect = GetPnDimRect();
		ZoomAcadView(rect, m_fZoomScale);
	}
	//根据闭合区域拾取归属于钢板的图元集合
	double minDistance = 0;
	for (int i = 0; i < 2; i++)
	{
		//获取钢板等距扩大的轮廓点集合
		if (i == 0)
			minDistance = 0.5;
		else if (m_bHasInnerDimPos)
			minDistance = 50;
		else
			continue;
		ATOM_LIST<VERTEX> list;
		CalEquidistantShape(minDistance, &list);
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
			if (i == 0)
			{	//第一次拾取在钢板区域内的图元
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
			else
			{	//第二次扩大范围选择文本
				if (pEnt->isKindOf(AcDbText::desc()))
					AppendRelaEntity(pEnt);
			}
		}
	}
}

void CPlateProcessInfo::ExtractPlateRelaEnts()
{
	InternalExtractPlateRelaEnts();
	//判断钢板实体集合中是否有椭圆弧
	//存在椭圆弧时需要根据椭圆弧更新钢板轮廓点重新提取钢板关联实体，否则可能导致漏孔 wht 19-08-14
	CPlateObject::CAD_ENTITY *pCadEnt = NULL;
	for (pCadEnt = EnumFirstEnt(); pCadEnt; pCadEnt = EnumNextEnt())
	{
		if (pCadEnt->ciEntType == RELA_ACADENTITY::TYPE_ELLIPSE)
			break;
	}
	if (pCadEnt)
	{
		for (pCadEnt = EnumFirstEnt(); pCadEnt; pCadEnt = EnumNextEnt())
		{
			CAcDbObjLife objLife(MkCadObjId(pCadEnt->idCadEnt));
			AcDbEntity *pEnt = objLife.GetEnt();
			if (pEnt == NULL || !pEnt->isKindOf(AcDbEllipse::desc()))
				continue;
			//提取圆弧轮廓边信息
			BYTE ciEdgeType = 0;
			f3dArcLine arcLine;
			if (g_pncSysPara.RecogArcEdge(pEnt, arcLine, ciEdgeType))
				UpdateVertexPropByArc(arcLine, ciEdgeType);
		}
		//根据椭圆弧修正钢板外形区域后重新提取钢板关联实体 wht 19-08-14
		InternalExtractPlateRelaEnts();
	}
}

void CPlateProcessInfo::PreprocessorBoltEnt(CHashSet<CAD_ENTITY*> &hashInvalidBoltCirPtrSet, int *piInvalidCirCountForText)
{	//合并圆心相同的圆，同一圆心只保留直径最大的圆
	CHashStrList<CAD_ENTITY*> hashEntPtrByCenterStr;	//记录圆心对应的直径最大的螺栓实体
	hashInvalidBoltCirPtrSet.Empty();
	for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
	{
		if (pEnt->ciEntType != RELA_ACADENTITY::TYPE_BLOCKREF &&
			pEnt->ciEntType != RELA_ACADENTITY::TYPE_CIRCLE)
			continue;	//仅处理螺栓图块和圆
		if (pEnt->ciEntType == RELA_ACADENTITY::TYPE_BLOCKREF)
		{	
			if (pEnt->m_fSize == 0 || strlen(pEnt->sText) <= 0)
				continue;	//非螺栓图块
		}
		else if (pEnt->ciEntType == RELA_ACADENTITY::TYPE_CIRCLE)
		{
			if ((m_bCirclePlate && cir_center.IsEqual(pEnt->pos)) ||
				(pEnt->m_fSize<0 || pEnt->m_fSize>CPlateExtractor::MAX_BOLT_HOLE))
			{	//环型板的同心圆和直径超过100的圆都不可能时螺栓孔 wxc 20-04-15
				hashInvalidBoltCirPtrSet.SetValue((DWORD)pEnt, pEnt);
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
				hashInvalidBoltCirPtrSet.SetValue((DWORD)pEnt, pEnt);
			}
			else
			{	//**ppEntPtr螺栓直径较小为螺栓内圆需要忽略
				hashInvalidBoltCirPtrSet.SetValue((DWORD)(*ppEntPtr), *ppEntPtr);
				hashEntPtrByCenterStr.DeleteNode(sKey);
				hashEntPtrByCenterStr.SetValue(sKey, pEnt);
			}
		}
		else
			hashEntPtrByCenterStr.SetValue(sKey, pEnt);
	}
	if (g_pncSysPara.m_ciBoltRecogMode == CPNCSysPara::BOLT_RECOG_DEFAULT)
	{
		int nInvalidCount = 0;
		for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
		{
			if (pEnt->ciEntType != RELA_ACADENTITY::TYPE_TEXT && pEnt->ciEntType != RELA_ACADENTITY::TYPE_MTEXT)
				continue;
			//标注字符串中包含"钻孔或冲孔"，
			if (strstr(pEnt->sText, "钻") != NULL || strstr(pEnt->sText, "冲") != NULL ||
				strstr(pEnt->sText, "Φ") != NULL || strstr(pEnt->sText, "%%C") != NULL ||
				strstr(pEnt->sText, "%%c") != NULL|| strstr(pEnt->sText, "焊") != NULL)
				continue;
			int nLen = strlen(pEnt->sText);
			if (nLen < 3)
			{
				bool bDigit = true;
				for (int i = 0; i < nLen; i++)
				{
					if (!isdigit(pEnt->sText[i]))
					{
						bDigit = false;
						break;
					}
				}
				if (bDigit)
					continue;	//文字为小于100的数字，不可能为件号 wht 19-12-11
			}
			//排除件号标注圆圈
			SCOPE_STRU scope;
			for (CAD_ENTITY **ppCirEnt = hashEntPtrByCenterStr.GetFirst(); ppCirEnt; ppCirEnt = hashEntPtrByCenterStr.GetNext())
			{
				CAD_ENTITY *pCirEnt = *ppCirEnt;
				if (pCirEnt->ciEntType == RELA_ACADENTITY::TYPE_BLOCKREF)
					continue;	//螺栓图符内有文字时不需要过滤 wht 19-12-10
				scope.ClearScope();
				double r = pCirEnt->m_fSize*0.5;
				scope.fMinX = pCirEnt->pos.x - r;
				scope.fMaxX = pCirEnt->pos.x + r;
				scope.fMinY = pCirEnt->pos.y - r;
				scope.fMaxY = pCirEnt->pos.y + r;
				scope.fMinZ = scope.fMaxZ = 0;
				if (scope.IsIncludePoint(pEnt->pos))
				{
					hashInvalidBoltCirPtrSet.SetValue((DWORD)pCirEnt, pCirEnt);
					hashEntPtrByCenterStr.DeleteCursor();
					nInvalidCount++;
				}
			}
		}
		if (piInvalidCirCountForText)
			*piInvalidCirCountForText = nInvalidCount;
	}
}
//根据钢板相关的图元集合更新基本信息、螺栓信息及顶点(火曲)信息
BOOL CPlateProcessInfo::UpdatePlateInfo(BOOL bRelatePN/*=FALSE*/)
{
	if(vertexList.GetNodeNum()<3)
		return FALSE;
	//根据Spline提取火曲线标记信息(分段线段集合)
	AcDbEntity *pEnt = NULL;
	CSymbolRecoginzer symbols;
	CHashSet<CAD_ENTITY*> bendDimTextSet;
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		if( pRelaObj->ciEntType!=RELA_ACADENTITY::TYPE_SPLINE&&
			pRelaObj->ciEntType!=RELA_ACADENTITY::TYPE_TEXT)
			continue;
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pRelaObj->ciEntType == RELA_ACADENTITY::TYPE_SPLINE)
			symbols.AppendSymbolEnt((AcDbSpline*)pEnt);
		else if(pRelaObj->ciEntType==RELA_ACADENTITY::TYPE_TEXT)
		{
			if (g_pncSysPara.IsMatchBendRule(pRelaObj->sText))
				bendDimTextSet.SetValue(pRelaObj->idCadEnt,pRelaObj);
		}
	}
	//
	boltList.Empty();
	xPlate.m_cFaceN=1;
	DeleteAssisstPts();
	CHashSet<CAD_ENTITY*> hashInvalidBoltCirPtrSet;
	int nInvalidCirCountForText = 0;
	PreprocessorBoltEnt(hashInvalidBoltCirPtrSet, &nInvalidCirCountForText);
	if (nInvalidCirCountForText > 0)
		logerr.Log("%s#钢板，已过滤%d个可能为件号标注的圆，请确认！",(char*)xPlate.GetPartNo(),nInvalidCirCountForText);
	//baseInfo应定义在For循环外，否则多行多次提取会导致之前的提取到的结果被冲掉 wht 19-10-22
	BASIC_INFO baseInfo;
	ARRAY_LIST<CXhChar200> errorList;
	for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
	{
		if (hashInvalidBoltCirPtrSet.GetValue((DWORD)pRelaObj))
			continue;	//过滤掉无效的螺栓实体（大圆内的小圆、件号标注） wht 19-07-20
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
			if (baseInfo.m_sTaType.GetLength() > 0 && model.m_sTaType.GetLength() <= 0)
				model.m_sTaType.Copy(baseInfo.m_sTaType);
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
		//提取螺栓信息
		BOLT_HOLE boltInfo;
		if(g_pncSysPara.RecogBoltHole(pEnt,boltInfo))
		{
			BOLT_INFO *pBoltInfo=boltList.append();
			pBoltInfo->posX=boltInfo.posX;
			pBoltInfo->posY=boltInfo.posY;
			pBoltInfo->bolt_d=boltInfo.d;
			pBoltInfo->hole_d_increment=boltInfo.increment;
			pBoltInfo->cFuncType=(boltInfo.ciSymbolType == 0) ? 0 : 2;
			//对于圆圈表示的螺栓，先根据圆圈直径和标准螺栓直径粗判是否归属于标准螺栓,然后再根据文字标注进行精判 wxc-2020-04-01
			if (pEnt->isKindOf(AcDbCircle::desc()))
			{
				bool bStandardBolt = true;
				double fHoleD = boltInfo.d;
				short wBoltD = ftoi(fHoleD - 2.1);
				if (wBoltD <= 12)
					wBoltD = 12;
				else if (wBoltD <= 16)
					wBoltD = 16;
				else if (wBoltD <= 20)
					wBoltD = 20;
				else if (wBoltD <= 24)
					wBoltD = 24;
				else
					bStandardBolt = false;
				if (bStandardBolt)
				{
					pBoltInfo->bolt_d = wBoltD;
					pBoltInfo->hole_d_increment = (float)(fHoleD - wBoltD);
					pBoltInfo->cFuncType = 0;
				}
			}
			//有文字孔径标注的螺栓图形为螺栓图符时也需要根据文字标注更新孔径 wht 19-11-28
			int nPush = m_xHashRelaEntIdList.push_stack();
			for (CAD_ENTITY *pOtherRelaObj = m_xHashRelaEntIdList.GetFirst(); pOtherRelaObj; pOtherRelaObj = m_xHashRelaEntIdList.GetNext())
			{
				if (pOtherRelaObj->ciEntType != RELA_ACADENTITY::TYPE_TEXT &&
					pOtherRelaObj->ciEntType != RELA_ACADENTITY::TYPE_DIM_D)
					continue;
				if (strlen(pOtherRelaObj->sText) <= 0 ||
					(strstr(pOtherRelaObj->sText, "%%C") == NULL && strstr(pOtherRelaObj->sText, "%%c") == NULL &&
						strstr(pOtherRelaObj->sText, "Φ") == NULL))
					continue;	//没有螺栓直径信息
				if (strstr(pOtherRelaObj->sText, "-"))
				{	//重新提取原有文件，找出可能错误的钢板时使用以下代码 wht 20-04-27
					/*if (DISTANCE(pOtherRelaObj->pos, GEPOINT(boltInfo.posX, boltInfo.posY)) < 50)
					{
						CXhChar200 errorStr("%s#钢板，%s文字(半径50)附近的螺栓孔经识别可能有误，请确认！", (char*)xPlate.GetPartNo(),(char*)pOtherRelaObj->sText);
						errorList.Append(errorStr);
					}*/
					continue;	//正常的钢印信息标注，非特殊螺栓直径标注,例如：4-Φ18    
				}
				if (DISTANCE(pOtherRelaObj->pos, GEPOINT(boltInfo.posX, boltInfo.posY)) < 50)
				{
					CString ss(pOtherRelaObj->sText);
					if (ss.Find("钻") >= 0)
						pBoltInfo->cFlag = 1;
					ss.Replace("%%C", "|");
					ss.Replace("%%c", "|");
					ss.Replace("Φ", "|");
					bool bNeedDrillHole = ss.Replace("钻", "")>0;
					ss.Replace("孔", "");
					bool bNeedPunchHole = ss.Replace("冲", "")>0;
					CXhChar100 sText(ss);
					std::vector<CXhChar16> numKeyArr;
					for (char* sKey = strtok(sText, "|，,:："); sKey; sKey = strtok(NULL, "|，,:："))
						numKeyArr.push_back(CXhChar16(sKey));
					if (numKeyArr.size() == 1)
					{
						double hole_d = atof(numKeyArr[0]);
						double org_holt_d = boltInfo.d + boltInfo.increment;
						double dd = fabs(hole_d - org_holt_d);
						if (dd > EPS2)
							logerr.Log("%s#钢板，根据文字(%s)识别的直径与螺栓孔图面直径存在误差值(%.1f)，请确认！", (char*)xPlate.GetPartNo(), (char*)pOtherRelaObj->sText, dd);
						//钻孔或冲孔或非标准图块，识别为挂线孔 wht 20-04-27
						if (bNeedPunchHole || bNeedDrillHole || boltInfo.ciSymbolType != 0)
						{
							pBoltInfo->bolt_d = hole_d;
							pBoltInfo->hole_d_increment = 0;
							pBoltInfo->cFuncType = 2;
						}
					}
				}
			}
			m_xHashRelaEntIdList.pop_stack(nPush);
			//对于特殊孔进行代孔处理
			if (g_pncSysPara.m_bReplaceSH && pBoltInfo->cFuncType == 2)
			{
				pBoltInfo->bolt_d = g_pncSysPara.m_nReplaceHD;
				pBoltInfo->hole_d_increment = 1.5;
			}
			continue;
		}
		//提取圆弧轮廓边信息
		BYTE ciEdgeType=0;
		f3dArcLine arcLine;
		if(g_pncSysPara.RecogArcEdge(pEnt,arcLine,ciEdgeType))
		{
			UpdateVertexPropByArc(arcLine,ciEdgeType);
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
		}
		//提取火曲线信息
		if(pEnt->isKindOf(AcDbLine::desc()))
		{	
			f3dLine line;
			AcDbLine* pAcDbLine=(AcDbLine*)pEnt;
			Cpy_Pnt(line.startPt, pAcDbLine->startPoint());
			Cpy_Pnt(line.endPt, pAcDbLine->endPoint());
			if(g_pncSysPara.IsBendLine((AcDbLine*)pEnt,&symbols))
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
				if(minDisBendDim.m_pRelaObj)
				{
					CAD_ENTITY *pBendDim = (CAD_ENTITY *)minDisBendDim.m_pRelaObj;
					BOOL bFrontBend = FALSE;
					g_pncSysPara.ParseBendText(pBendDim->sText,fDegree,bFrontBend);
					if (fDegree > 0)
					{
						fDegree *= bFrontBend ? 1 : -1;
						GEPOINT bend_face_norm(0, 0, 1);
						RotateVectorAroundVector(bend_face_norm, fDegree*RADTODEG_COEF, line_vec);
						normalize(bend_face_norm);
						xPlate.HuoQuFaceNorm[xPlate.m_cFaceN - 2] = bend_face_norm;
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
	//重新提取原有文件，找出可能错误的钢板时使用以下代码 wht 20-04-27
	/*if (errorList.GetSize() > 0)
	{
		for (CXhChar200 *pError = errorList.GetFirst(); pError; pError = errorList.GetNext())
		{
			logerr.Log(*pError);
		}
	}*/
	if (m_bCirclePlate)
	{	//圆形钢板时，查找是否有内轮廓
		for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
		{
			if(pRelaObj->ciEntType!= RELA_ACADENTITY::TYPE_CIRCLE)
				continue;
			CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
			if ((pEnt = objLife.GetEnt()) == NULL || !pEnt->isKindOf(AcDbCircle::desc()))
				continue;
			if (pRelaObj->pos.IsEqual(cir_center))
			{
				AcDbCircle* pCir = (AcDbCircle*)pEnt;
				xPlate.m_fInnerRadius = pCir->radius();
				xPlate.m_tInnerOrigin = cir_center;
				break;
			}
		}
	}
	return m_xHashRelaEntIdList.GetNodeNum()>1;
}
f2dRect CPlateProcessInfo::GetPnDimRect()
{
	//文本坐标 this->dim_pos 已做居中处理
	f2dRect rect;
	rect.topLeft.Set(dim_pos.x-10, dim_pos.y+10);
	rect.bottomRight.Set(dim_pos.x+10, dim_pos.y-10);
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
void CALLBACK EXPORT CloseBoundaryPopupWnd(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
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
#ifdef __ALFA_TEST_
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
	double fScale=(fExternLen-fMinExtern)/10;
	//
	HWND hMainWnd=adsw_acadMainWnd();
	for(int j=0;j<2;j++)
	{
		f3dPoint cur_dim_pos=dim_pos;
		if(j==1)
		{	//尝试使用inner_dim_pos提取 wht 19-02-01
			if(m_bHasInnerDimPos)
			{
				cur_dim_pos=inner_dim_pos;
				rect.topLeft.Set(cur_dim_pos.x-150,cur_dim_pos.y+10);
				rect.bottomRight.Set(cur_dim_pos.x+10,cur_dim_pos.y-10);
			}
			else
				break;
		}
		for(int i=0;i<=10;i++)
		{
			if(i>0)
				fExternLen-=fScale;
			m_fZoomScale=fExternLen/g_pncSysPara.m_fMapScale;
			ZoomAcadView(rect,m_fZoomScale);
			ads_point base_pnt;
			base_pnt[X]=cur_dim_pos.x;
			base_pnt[Y]=cur_dim_pos.y;
			base_pnt[Z]=cur_dim_pos.z;
			int nTimer=SetTimer(hMainWnd,1,100,CloseBoundaryPopupWnd);
			int resCode= RTNORM;
			if (bSendCommand)
			{
				CXhChar50 sCmd("-boundary %.2f,%.2f\n ", cur_dim_pos.x, cur_dim_pos.y);
#ifdef _ARX_2007
				SendCommandToCad(_bstr_t(sCmd));
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
		if(initLastObjId!=plineId)
		{
			m_bHasInnerDimPos=(j==1);
			break;
		}
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
	acdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
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
			pVer=tem_vertes.append();
			pVer->pos.Set(location.x,location.y,location.z);
			pVer->ciEdgeType=2;
			pVer->arc.center=center;
			pVer->arc.work_norm=norm;
			pVer->arc.radius=acgeArc.radius();
			pVer->arc.fSectAngle = fabs(acgeArc.endAng() - acgeArc.startAng());
		}
	}
	pPline->erase(Adesk::kTrue);	//删除polyline对象
	pPline->close();
	//
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		VERTEX* pCurVer=tem_vertes.GetByIndex(i);
		if(pCurVer->ciEdgeType==2&&ftoi(pCurVer->arc.radius)==ASSIST_RADIUS)
		{
			VERTEX* pNextVer=tem_vertes.GetByIndex((i+1)%nVertNum);
			if(pNextVer->ciEdgeType==1)
				pNextVer->pos=pCurVer->arc.center;
			else if(pNextVer->ciEdgeType==2&&ftoi(pNextVer->arc.radius)!=ASSIST_RADIUS)
				pNextVer->pos=pCurVer->arc.center;
		}
	}
	for(pVer=tem_vertes.GetFirst();pVer;pVer=tem_vertes.GetNext())
	{
		if(pVer->ciEdgeType==2&&ftoi(pVer->arc.radius)==ASSIST_RADIUS)
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
			m_bCirclePlate = TRUE;
			cir_center = pt;
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
	ARRAY_LIST<ACAD_LINEID> objectLineArr;
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
			ACAD_LINEID* pLine = NULL;
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
				pLine->m_lineId = objId.asOldId();
			}
		}
	}
	if (objectLineArr.GetSize() > 3)
		return InitProfileByAcdbLineList(objectLineArr);
	return FALSE;
}
BOOL CPlateProcessInfo::InitProfileByAcdbPolyLine(AcDbObjectId idAcdbPline)
{
	CAcDbObjLife objLife(idAcdbPline);
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPolyline::desc()))
		return FALSE;
	ATOM_LIST<VERTEX> tem_vertes;
	AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
	int nVertNum = pPline->numVerts();
	for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
	{
		AcGePoint3d location;
		pPline->getPointAt(iVertIndex, location);
		if (IsHasVertex(tem_vertes, GEPOINT(location.x, location.y, location.z)))
			continue;
		GEPOINT ptS, ptE;
		if (pPline->segType(iVertIndex) == AcDbPolyline::kLine)
		{
			AcGeLineSeg3d acgeLine;
			pPline->getLineSegAt(iVertIndex, acgeLine);
			Cpy_Pnt(ptS, acgeLine.startPoint());
			Cpy_Pnt(ptE, acgeLine.endPoint());
			VERTEX* pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
		}
		else if (pPline->segType(iVertIndex) == AcDbPolyline::kArc)
		{
			AcGeCircArc3d acgeArc;
			pPline->getArcSegAt(iVertIndex, acgeArc);
			GEPOINT center, norm;
			Cpy_Pnt(ptS, acgeArc.startPoint());
			Cpy_Pnt(ptE, acgeArc.endPoint());
			Cpy_Pnt(center, acgeArc.center());
			Cpy_Pnt(norm, acgeArc.normal());
			VERTEX* pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
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
BOOL CPlateProcessInfo::InitProfileByAcdbLineList(ARRAY_LIST<ACAD_LINEID>& xLineArr)
{
	if (xLineArr.GetSize() <= 0)
		return FALSE;
	return InitProfileByAcdbLineList(xLineArr[0], xLineArr);
}
BOOL CPlateProcessInfo::InitProfileByAcdbLineList(ACAD_LINEID& startLine, ARRAY_LIST<ACAD_LINEID>& xLineArr)
{
	if (xLineArr.GetSize() <= 0)
		return FALSE;
	//查找起始轮廓线
	ACAD_LINEID *pLine = NULL, *pFirLine = NULL;
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
		if (fabs(DISTANCE(ptS, ptE)) < EPS2)
		{
			bFinish = TRUE;
			break;
		}
		ATOM_LIST<ACAD_LINEID*> linkLineList;
		ACAD_LINEID* pLine = NULL;
		for (pLine = xLineArr.GetFirst(); pLine; pLine = xLineArr.GetNext())
		{
			if (pLine->m_bMatch)
				continue;
			if (pLine->m_ptStart.IsEqual(ptE, EPS2))
				linkLineList.append(pLine);
			else if (pLine->m_ptEnd.IsEqual(ptE, EPS2))
				linkLineList.append(pLine);
		}
		if (linkLineList.GetNodeNum() <= 0)
			break;	//没有找到相连线段
		if (linkLineList.GetNodeNum() == 1)
		{	//只有一根相连线段
			pLine = linkLineList[0];
			pLine->m_bMatch = TRUE;
			pLine->m_ciSerial = ciSerial++;
			if (pLine->m_ptStart.IsEqual(ptE, EPS2))
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
				if (linkLineList[i]->m_ptStart.IsEqual(ptE, EPS2))
					cur_vec = (linkLineList[i]->m_ptEnd - linkLineList[i]->m_ptStart);
				else
					cur_vec = (linkLineList[i]->m_ptStart - linkLineList[i]->m_ptEnd);
				normalize(cur_vec);
				double fCosa = vec * cur_vec;
				maxValue.Update(fCosa, linkLineList[i]);
			}
			pLine = (ACAD_LINEID*)maxValue.m_pRelaObj;
			pLine->m_bMatch = TRUE;
			pLine->m_ciSerial = ciSerial++;
			if (pLine->m_ptStart.IsEqual(ptE, EPS2))
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
	CQuickSort<ACAD_LINEID>::QuickSort(xLineArr.m_pData, xLineArr.GetSize(), ACAD_LINEID::compare_func);
	for (int i = 0; i < xLineArr.GetSize(); i++)
	{
		ACAD_LINEID objItem = xLineArr[i];
		if (!objItem.m_bMatch)
			continue;
		VERTEX* pVer = tem_vertes.append();
		pVer->pos = objItem.vertex;
		//记录圆弧信息
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, MkCadObjId(objItem.m_lineId), AcDb::kForRead);
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
			if (radius <= 0 || DISTANCE(ptS, ptE) < EPS)
				continue;	//不合理圆弧
			pVer->ciEdgeType = 2;
			pVer->arc.center = center;
			pVer->arc.work_norm = norm;
			pVer->arc.radius = radius;
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
void CPlateProcessInfo::CreatePPiFile(const char* file_path)
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
	CXhChar100 sAllRelPart(xPlate.GetPartNo());
	if(m_sRelatePartNo.GetLength()>0)
		sAllRelPart.Printf("%s,%s",(char*)xPlate.GetPartNo(),(char*)m_sRelatePartNo);
	//设置当前工作路径
	SetCurrentDirectory(file_path);
	if(g_pncSysPara.m_iPPiMode==0)
	{//一板一号模式：一个PPI文件包含一个件号
		for(char* sKey=strtok(sAllRelPart,",");sKey;sKey=strtok(NULL,","))
		{
			if(strlen(sKey)<=0)
				continue;
			xPlate.SetPartNo(sKey);
			CBuffer buffer;
			xPlate.ToPPIBuffer(buffer);
			CString sFilePath;
			//使用SetCurrentDirectory设置工作路径后，使用相对路径写文件
			//可以避免文件夹中带"."无法保存或者路径名太长无法保存的问题 wht 19-04-24
			//sFilePath.Format("%s%s.ppi",file_path,(char*)xPlate.GetPartNo());
			sFilePath.Format(".\\%s.ppi", (char*)xPlate.GetPartNo());	
			FILE* fp=fopen(sFilePath,"wb");
			if(fp)
			{
				long file_len=buffer.GetLength();
				fwrite(&file_len,sizeof(long),1,fp);
				fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
				fclose(fp);
			}
			else
			{
				DWORD nRetCode = GetLastError();
				logerr.Log("%s#钢板ppi文件保存失败,错误号#%d！路径：%s%s", (char*)xPlate.GetPartNo(), nRetCode, file_path,sFilePath);
			}
		}
	}
	else if(g_pncSysPara.m_iPPiMode==1)
	{//一板多号模式：一个PPI文件包括多个件号
		if(m_sRelatePartNo.GetLength()>0)
			xPlate.m_sRelatePartNo.Copy(m_sRelatePartNo);
		sAllRelPart.Replace(","," ");
		CBuffer buffer;
		xPlate.ToPPIBuffer(buffer);
		CString sFilePath;
		//使用SetCurrentDirectory设置工作路径后，使用相对路径写文件
		//可以避免文件夹中带"."无法保存或者路径名太长无法保存的问题 wht 19-04-24
		//sFilePath.Format("%s%s.ppi",file_path,(char*)sAllRelPart);
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
			CHashList<VERTEX_TAG> xVerTagHash;
			for(BOLT_INFO* pLs=xPlate.m_xBoltInfoList.GetFirst();pLs;pLs=xPlate.m_xBoltInfoList.GetNext())
			{
				f3dPoint ls_pt(pLs->posX,pLs->posY,0),perp;
				double fDist=0;
				if(SnapPerp(&perp,edge_line,ls_pt,&fDist)==FALSE||edge_line.PtInLine(perp,1)!=2)
					continue;	//螺栓投影点不在轮廓边上
				VERTEX_TAG* pTag=xVerTagHash.GetValue(ftoi(fDist));
				if(pTag==NULL)
					pTag=xVerTagHash.Add(ftoi(fDist));
				pTag->nLsNum+=1;
			}
			for(VERTEX_TAG* pTag=xVerTagHash.GetFirst();pTag;pTag=xVerTagHash.GetNext())
			{
				if(pTag->nLsNum>1 && pTag->nLsNum>vertex_arr[i].feature)
					vertex_arr[i].feature=pTag->nLsNum;
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
	if(bVertexOnly)
	{
		PROFILE_VER* pPreVertex=xPlate.vertex_list.GetTail();
		for(PROFILE_VER *pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext())
		{
			f3dPoint pt=pPreVertex->vertex;
			coord_trans(pt,ucs,FALSE);
			scope.VerifyVertex(pt);
			if(pPreVertex->type==2)
			{
				f3dPoint axis_len(ucs.axis_x);
				f3dArcLine arcLine;
				arcLine.CreateMethod2(pPreVertex->vertex,pVertex->vertex,pPreVertex->work_norm,pPreVertex->sector_angle);
				f3dPoint startPt=f3dPoint(arcLine.Center())+axis_len*arcLine.Radius();
				f3dPoint endPt=f3dPoint(arcLine.Center())-axis_len*arcLine.Radius();
				coord_trans(startPt,ucs,FALSE);
				coord_trans(endPt,ucs,FALSE);
				scope.VerifyVertex(startPt);
				scope.VerifyVertex(endPt);
			}
			pPreVertex=pVertex;
		}
	}
	else
	{
		for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext())
			scope.VerifyVertex(f3dPoint(pVertex->vertex.x,pVertex->vertex.y));
		for(BOLT_INFO *pHole=xPlate.m_xBoltInfoList.GetFirst();pHole;pHole=xPlate.m_xBoltInfoList.GetNext())
		{
			double radius=0.5*pHole->bolt_d;
			scope.VerifyVertex(f3dPoint(pHole->posX-radius,pHole->posY-radius));
			scope.VerifyVertex(f3dPoint(pHole->posX+radius,pHole->posY+radius));
		}
		if(xPlate.IsDisplayMK() && bDisplayMK)
			scope.VerifyVertex(f3dPoint(xPlate.mkpos.x,xPlate.mkpos.y));
	}
	return scope;
}
void CPlateProcessInfo::DrawPlate(f3dPoint *pOrgion/*=NULL*/,BOOL bCreateDimPos/*=FALSE*/,BOOL bDrawAsBlock/*=FALSE*/, GEPOINT *pPlateCenter /*= NULL*/)
{	
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	if(pBlockTableRecord==NULL)
	{
		AfxMessageBox("获取块表记录失败!");
		return;
	}
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
	BOOL bOnlyIncTextEnt=TRUE;
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId;
	AcDbText *pPartNoText=NULL;
	m_cloneEntIdList.Empty();
	m_hashColneEntIdBySrcId.Empty();
	AcDbBlockTableRecord *pCurBlockTblRec = pBlockTableRecord;
	if (bDrawAsBlock&&pPlateCenter&&DRAGSET.BeginBlockRecord())
		pCurBlockTblRec = DRAGSET.RecordingBlockTableRecord();
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,MkCadObjId(pRelaObj->idCadEnt),AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		AcDbEntity *pClone = (AcDbEntity *)pEnt->clone();
		if(pClone)
		{
			if(vertexList.GetNodeNum()<3)
				pClone->setColorIndex(1);	//
			if(pOrgion)
			{
				pClone->transformBy(moveMat);		//平移
				pClone->transformBy(rotationMat);	//旋转
			}
			if (pPlateCenter&&bDrawAsBlock)
				pClone->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			DRAGSET.AppendAcDbEntity(pCurBlockTblRec,entId,pClone);
			m_cloneEntIdList.append(entId.asOldId());
			m_hashColneEntIdBySrcId.SetValue(pRelaObj->idCadEnt,entId.asOldId());
			pClone->close();
		}
		if(pEnt->isKindOf(AcDbText::desc()))
		{	
			AcDbText* pText=(AcDbText*)pEnt;
			CXhChar100 sText;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if(pPartNoText==NULL&&strstr(sText,GetPartNo()))
				pPartNoText=pText;
		}
		else 
			bOnlyIncTextEnt=FALSE;
	}
	if(bCreateDimPos)
	{	//生成标注点
		AcDbObjectId pointId;
		AcDbPoint *pPoint=new AcDbPoint(AcGePoint3d(dim_pos.x,dim_pos.y,dim_pos.z));
		if(DRAGSET.AppendAcDbEntity(pCurBlockTblRec,pointId,pPoint))
		{
			pPoint->transformBy(moveMat);		//平移
			pPoint->transformBy(rotationMat);	//旋转
			if (pPlateCenter&&bDrawAsBlock)
				pPoint->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			m_xMkDimPoint.idCadEnt=pointId.asOldId();
			AcGePoint3d pos=pPoint->position();
			m_xMkDimPoint.pos.Set(pos.x,pos.y,0);
		}
		pPoint->close();
	}
	//对于提取失败的钢板做特殊处理(如果hashEntIdList中只有文本标注默认提取失败)
	if(pPartNoText&&bOnlyIncTextEnt)
	{
		CXhChar100 sText;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pPartNoText->textString()));
#else
		sText.Copy(pPartNoText->textString());
#endif
		double heigh=pPartNoText->height();
		double angle=pPartNoText->rotation();
		double len=DrawTextLength(sText,heigh,pPartNoText->textStyle());
		f3dPoint dim_norm(-sin(angle),cos(angle));
		f3dPoint origin=dim_pos;
		origin+=dim_vec*len*0.5;
		origin+=dim_norm*heigh*0.5;
		AcGeVector3d norm(0,0,1);
		AcGePoint3d acad_centre;
		Cpy_Pnt(acad_centre,origin);
		AcDbCircle *pCircle=new AcDbCircle(acad_centre,norm,len*0.5);
		pCircle->setColorIndex(1);
		DRAGSET.AppendAcDbEntity(pCurBlockTblRec,entId,pCircle);
		pCircle->close();
	}
	if(bDrawAsBlock&&pPlateCenter&&pCurBlockTblRec!=pBlockTableRecord)
		DRAGSET.EndBlockRecord(pBlockTableRecord,*pPlateCenter);
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
	for(VERTEX *vertex = vertexList.GetFirst();vertex;vertex=vertexList.GetNext())
	{
		VERTEX *vertexPre = vertexList.GetPrev();
		if (vertexPre == NULL)
		{
			vertexPre = vertexList.GetTail();
			vertexList.GetFirst();
		}
		else
			vertexList.GetNext();
		VERTEX *vertexNext = vertexList.GetNext();
		if (vertexNext == NULL)
		{
			vertexNext = vertexList.GetFirst();
			vertexList.GetTail();
		}
		else
			vertexList.GetPrev();
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
		*pDestVertex = *vertex;
		pDestVertex->pos = GEPOINT(curPt + vec*offset);
		double radius = pDestVertex->arc.radius;
		if (pDestVertex->ciEdgeType >= 2)
			pDestVertex->arc.radius = radius + offset;
		//pDestVertex->pos = (double*)curPt; //此处不能赋值，否则无法设置间隙值无效 wht 19-09-29
		pDestVertex->arc.radius = radius;
		if (pDestVertex->ciEdgeType >= 2)
		{	//为圆弧或椭圆弧添加分片轮廓点，否则提取螺栓孔时可能漏孔 wht 19-08-14
			f3dArcLine arcLine;
			if (vertex->ciEdgeType == 2)	//指定圆弧夹角
				arcLine.CreateMethod2(curPt, nextPt, vertex->arc.work_norm, vertex->arc.fSectAngle);
			else if (vertex->ciEdgeType == 3)
				arcLine.CreateEllipse(vertex->arc.center, curPt, nextPt, vertex->arc.column_norm,
					vertex->arc.work_norm, vertex->arc.radius);
			//计算由于圆弧带来的面积区域变化
			int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
			double angle = arcLine.SectorAngle() / nSlices;
			for (int i = 1; i < nSlices; i++)
			{
				f3dPoint pos = arcLine.PositionInAngle(angle*i);
				VERTEX *pNewVertex=pDestList->append();
				pNewVertex->ciEdgeType = 1;
				pNewVertex->pos.Set(pos.x,pos.y,0);
			}
		}
	}
}
f2dRect CPlateProcessInfo::GetMinWrapRect(double minDistance/*=0*/,fPtList *pVertexList/*=NULL*/)
{
	f2dRect rect;
	rect.SetRect(f2dPoint(0,0),f2dPoint(0,0));
	if(m_bHasInnerDimPos)
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for(CAD_ENTITY *pEnt=EnumFirstEnt();pEnt;pEnt=EnumNextEnt())
			entIdList.append(MkCadObjId(pEnt->idCadEnt));
		rect=GetCadEntRect(entIdList,minDistance);
		datumStartVertex.srcPos.Set(rect.topLeft.x,rect.bottomRight.y);
		datumEndVertex.srcPos.Set(rect.bottomRight.x,rect.bottomRight.y);
		datumStartVertex.offsetPos=datumStartVertex.srcPos-GEPOINT(rect.topLeft.x,rect.topLeft.y);
		datumEndVertex.offsetPos=datumEndVertex.srcPos-GEPOINT(rect.topLeft.x,rect.topLeft.y);
		return rect;
	}
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
	if (m_bHasInnerDimPos)
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for (CAD_ENTITY *pEnt = EnumFirstEnt(); pEnt; pEnt = EnumNextEnt())
			entIdList.append(MkCadObjId(pEnt->idCadEnt));
		rect = GetCadEntRect(entIdList, 0);
		datumStartVertex.srcPos.Set(rect.topLeft.x, rect.bottomRight.y);
		datumEndVertex.srcPos.Set(rect.bottomRight.x, rect.bottomRight.y);
		datumStartVertex.offsetPos = datumStartVertex.srcPos - GEPOINT(rect.topLeft.x, rect.topLeft.y);
		datumEndVertex.offsetPos = datumEndVertex.srcPos - GEPOINT(rect.topLeft.x, rect.topLeft.y);
	}
	else
	{
		int n = vertexList.GetNodeNum();
		if (n < 3)
			return false;
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
	}
	return true;
}
void CPlateProcessInfo::InitEdgeEntIdMap()
{
	AcDbEntity *pEnt=NULL;
	//1.初始化原始直线与克隆直线
	ARRAY_LIST<ACAD_LINEID> lineIdList;
	CHashList<ACAD_LINEID> hashCloneEntIdByLineId;
	for(CAD_ENTITY *pCadEnt=EnumFirstEnt();pCadEnt;pCadEnt=EnumNextEnt())
	{
		AcDbObjectId entId=MkCadObjId(pCadEnt->idCadEnt);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			ACAD_LINEID *pLineId = lineIdList.append();
			GEPOINT start(pLine->startPoint().x, pLine->startPoint().y, 0), end(pLine->endPoint().x, pLine->endPoint().y, 0);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = lineIdList.append();
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = lineIdList.append();
			pLineId->Init(entId, start, end);
		}
	}
	for(ULONG *pId=m_cloneEntIdList.GetFirst();pId;pId=m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			GEPOINT start(pLine->startPoint().x, pLine->startPoint().y, 0), end(pLine->endPoint().x, pLine->endPoint().y, 0);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			pLineId->Init(entId, start, end);
		}
	}
	//2.初始化轮廓边对应的CAD实体
	int n=vertexList.GetNodeNum();
	m_hashCloneEdgeEntIdByIndex.Empty();
	for(ACAD_LINEID *pLineId=hashCloneEntIdByLineId.GetFirst();pLineId;pLineId=hashCloneEntIdByLineId.GetNext())
		pLineId->UpdatePos();
	for(int i=0;i<n;i++)
	{
		VERTEX *pCurVertex=vertexList.GetByIndex(i);
		VERTEX *pNextVertex=vertexList.GetByIndex((i+1)%n);
		GEPOINT start=pCurVertex->pos,end=pNextVertex->pos;
		for(ACAD_LINEID *pLineId=lineIdList.GetFirst();pLineId;pLineId=lineIdList.GetNext())
		{
			ULONG *pCloneEntId=m_hashColneEntIdBySrcId.GetValue(pLineId->m_lineId);
			ACAD_LINEID *pCloneLineId=pCloneEntId?hashCloneEntIdByLineId.GetValue(*pCloneEntId):NULL;
			if(pCloneLineId==NULL)
				continue;
			if(pLineId->m_ptStart.IsEqual(start)&&pLineId->m_ptEnd.IsEqual(end))
			{
				m_hashCloneEdgeEntIdByIndex.SetValue(i+1,*pCloneLineId);
				break;
			}
			else if(pLineId->m_ptStart.IsEqual(end)&&pLineId->m_ptEnd.IsEqual(start))
			{
				GEPOINT temp=pCloneLineId->m_ptStart;
				pCloneLineId->m_ptStart=pCloneLineId->m_ptEnd;
				pCloneLineId->m_ptEnd=temp;
				pCloneLineId->m_bReverse=TRUE;
				m_hashCloneEdgeEntIdByIndex.SetValue(i+1,*pCloneLineId);
				break;
			}
		}
	}
}
void CPlateProcessInfo::UpdateEdgeEntPos()
{
	for(ACAD_LINEID *pLineId=m_hashCloneEdgeEntIdByIndex.GetFirst();pLineId;pLineId=m_hashCloneEdgeEntIdByIndex.GetNext())
		pLineId->UpdatePos();
}

//根据克隆钢板中MKPOS位置，更新钢印号位置 wht 19-03-05
bool CPlateProcessInfo::SyncSteelSealPos()
{
	if (m_hashCloneEdgeEntIdByIndex.GetNodeNum() <= 0)
		return false;
	//1.获取Clone实体前两条边
	ACAD_LINEID *pFirstLineId = m_hashCloneEdgeEntIdByIndex.GetValue(1);
	ACAD_LINEID *pSecondLineId = m_hashCloneEdgeEntIdByIndex.GetValue(2);
	if (pFirstLineId == NULL || pSecondLineId == NULL)
		return false;
	//2.获取Src实体对应的前两条边
	AcDbObjectId srcFirstId=0, srcSecondId=0;
	for (ULONG *pId = m_hashColneEntIdBySrcId.GetFirst(); pId; pId = m_hashColneEntIdBySrcId.GetNext())
	{
		long idSrcEnt= m_hashColneEntIdBySrcId.GetCursorKey();
		if (*pId == pFirstLineId->m_lineId)
			srcFirstId = MkCadObjId(idSrcEnt);
		else if (*pId == pSecondLineId->m_lineId)
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
	ACAD_LINEID srcFirstLineId, srcSecondLineId;
	srcFirstLineId.m_lineId = srcFirstId.asOldId();
	srcFirstLineId.m_bReverse = pFirstLineId->m_bReverse;
	srcFirstLineId.UpdatePos();
	srcSecondLineId.m_lineId = srcSecondId.asOldId();
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
		Acad::ErrorStatus es=acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
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
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
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
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForWrite);
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
	acdbOpenAcDbEntity(pEnt, partNumId, AcDb::kForWrite);
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
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar500 sText;
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pMText->contents()));
#else
		sText.Copy(pMText->contents());
#endif
		CXhChar500 sTempText(sText);
		for (char* sKey = strtok(sTempText, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
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
				break;
			}
			
		}
	}
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
	else if(m_xHashRelaEntIdList.GetNodeNum()>0)
	{
		for (CPlateObject::CAD_ENTITY *pEnt = EnumFirstEnt(); pEnt; pEnt = EnumNextEnt())
			VerifyVertexByCADEntId(scope, MkCadObjId(pEnt->idCadEnt));
	}
	else
	{
		for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
			scope.VerifyVertex(pVer->pos);
	}
	return scope;
}
bool CPlateProcessInfo::IsMarkPosCadEnt(int idCadEnt)
{
	if (idCadEnt == m_xMkDimPoint.idCadEnt)
		return true;	//计算钢板区域时不算号料孔实体 wht 19-10-16
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
//ExtractPlateInfo
//ExtractPlateProfile
CPNCModel::CPNCModel()
{
	Empty();
	DisplayProcess = NULL;
}
CPNCModel::~CPNCModel()
{

}
void CPNCModel::Empty()
{
	m_hashPlateInfo.Empty();
	m_xAllEntIdSet.Empty();
}
//根据bpoly命令初始化钢板的轮廓边
void CPNCModel::ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//添加一个特定图层
	CLockDocumentLife lockCurDocumentLife;
	CXhChar16 sNewLayer("pnc_layer"),sLineType=g_pncSysPara.m_sProfileLineType;
	CreateNewLayer(sNewLayer, sLineType,AcDb::kLnWt013,1,m_idNewLayer,m_idSolidLine);
	m_hashSolidLineTypeId.SetValue(m_idSolidLine.asOldId(),m_idSolidLine);
	AcDbObjectId lineTypeId=GetLineTypeId("人民币");
	if(lineTypeId.isValid())
		m_hashSolidLineTypeId.SetValue(lineTypeId.asOldId(),lineTypeId);
	//提取火曲线特征信息(分段线段集合)
	AcDbEntity *pEnt = NULL;
	CSymbolRecoginzer symbols;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pEnt->isKindOf(AcDbSpline::desc()))
			symbols.AppendSymbolEnt((AcDbSpline*)pEnt);
	}
	//剔除孤立线条以及焊缝线
	const int WELD_MAX_HEIGHT = 20;
	ATOM_LIST<ACAD_CIRCLE> cirList;
	CHashStrList< ATOM_LIST<ACAD_LINEID> > hashLineArrByPosKeyStr;
	for(AcDbObjectId objId=selectedEntIdSet.GetFirst();objId;objId=selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbLine::desc()) &&
			!pEnt->isKindOf(AcDbArc::desc()) &&
			!pEnt->isKindOf(AcDbEllipse::desc()) &&
			!pEnt->isKindOf(AcDbCircle::desc()))
			continue;
		if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			GEPOINT center;
			Cpy_Pnt(center, pCir->center());
			cirList.append(ACAD_CIRCLE(objId, pCir->radius(), center));
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
			ptS.z = ptE.z = 0;	//手动初始化为0，防止点坐标的Z值无限小时，CXhChar50构造函数有的处理为0.0，有的处理为 -0.0
			double len = DISTANCE(ptS, ptE);
			//记录始端点坐标处的连接线
			CXhChar50 startKeyStr(ptS);
			ATOM_LIST<ACAD_LINEID> *pStartLineList = hashLineArrByPosKeyStr.GetValue(startKeyStr);
			if (pStartLineList == NULL)
				pStartLineList = hashLineArrByPosKeyStr.Add(startKeyStr);
			pStartLineList->append(ACAD_LINEID(objId, len));
			//记录终端点坐标处的连接线
			CXhChar50 endKeyStr(ptE);
			ATOM_LIST<ACAD_LINEID> *pEndLineList = hashLineArrByPosKeyStr.GetValue(endKeyStr);
			if (pEndLineList == NULL)
				pEndLineList = hashLineArrByPosKeyStr.Add(endKeyStr);
			pEndLineList->append(ACAD_LINEID(objId, len));
		}
	}
	for(ATOM_LIST<ACAD_LINEID> *pList=hashLineArrByPosKeyStr.GetFirst();pList;pList=hashLineArrByPosKeyStr.GetNext())
	{
		if(pList->GetNodeNum()!=1)
			continue;
		ACAD_LINEID *pLineId=pList->GetFirst();
		if(pLineId&&pLineId->m_fLen<WELD_MAX_HEIGHT)
			selectedEntIdSet.DeleteNode(pLineId->m_lineId);
	}
	selectedEntIdSet.Clean();
	//在圆弧或椭圆弧处添加辅助图元
	double fMaxEdge=0,fMinEdge=200000;
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	CHashSet<AcDbObjectId> xAssistCirSet;
	for(AcDbObjectId objId=selectedEntIdSet.GetFirst();objId;objId=selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if(g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//按线型过滤
			AcDbObjectId lineId = GetEntLineTypeId(pEnt);
			if (lineId.isValid() && m_hashSolidLineTypeId.GetValue(lineId.asOldId()) == NULL)
				continue;
		}
		else if(g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//按图层过滤
			CXhChar50 sLayerName= GetEntLayerName(pEnt);
			if(g_pncSysPara.IsNeedFilterLayer(sLayerName))
				continue;
		}
		else if(g_pncSysPara.m_ciRecogMode==CPNCSysPara::FILTER_BY_COLOR)
		{	//按颜色过滤
			int ciColorIndex = GetEntColorIndex(pEnt);
			if (ciColorIndex != g_pncSysPara.m_ciProfileColorIndex)
				continue;
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
			AcDbCircle *pCircle1=new AcDbCircle(startPt,norm,2);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle1);
			xAssistCirSet.SetValue(circleId.asOldId(),circleId);
			pCircle1->close();
			//终点处添加辅助小圆
			AcDbCircle *pCircle2=new AcDbCircle(endPt,norm,2);
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
			AcDbLine* pLine=(AcDbLine*)pEnt;
			Cpy_Pnt(ptS,pLine->startPoint());
			Cpy_Pnt(ptE,pLine->endPoint());
			double fDist=DISTANCE(ptS,ptE);
			if(fDist>fMaxEdge)
				fMaxEdge=fDist;
			if(fDist<fMinEdge)
				fMinEdge=fDist;
		}
	}
	pBlockTableRecord->close();
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
			pEnt->setLayer(m_idNewLayer);
			continue;
		}
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//按图层过滤
			if (g_pncSysPara.IsNeedFilterLayer(sLayerName))
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
			//过滤干扰图元（标注、图块等）
			if (!g_pncSysPara.IsProfileEnt(pEnt))
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
		}
		else if(g_pncSysPara.m_ciRecogMode==CPNCSysPara::FILTER_BY_COLOR)
		{	//按颜色过滤
			int iColor=GetEntColorIndex(pEnt);
			if( iColor!=g_pncSysPara.m_ciProfileColorIndex||
				iColor==g_pncSysPara.m_ciBendLineColorIndex)
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
			//过滤干扰图元（标注、图块等）
			if(!g_pncSysPara.IsProfileEnt(pEnt))
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//按线型过滤
			//过滤非轮廓边线型图元
			AcDbObjectId curLineId = GetEntLineTypeId(pEnt);
			if (curLineId.isValid() && m_hashSolidLineTypeId.GetValue(curLineId.asOldId()) == NULL)
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
			//过滤火曲线
			if (pEnt->isKindOf(AcDbLine::desc()) && g_pncSysPara.IsBendLine((AcDbLine*)pEnt, &symbols))
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
			//过滤干扰图元（标注、图块等）
			if (!g_pncSysPara.IsProfileEnt(pEnt))
			{
				pEnt->setLayer(m_idNewLayer);
				continue;
			}
		}
	}
	//检测识别钢板轮廓边信息
	BOOL bSendCommand = FALSE;
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	bSendCommand = TRUE;
#endif
	CShieldCadLayer shieldLayer(sNewLayer, TRUE, bSendCommand);	//屏蔽不需要的图层
	for (CPlateProcessInfo* pInfo = m_hashPlateInfo.GetFirst(); pInfo; pInfo = m_hashPlateInfo.GetNext())
	{
		//初始化孤岛检测状态
		for (ACAD_CIRCLE *pCir = cirList.GetFirst(); pCir; pCir = cirList.GetNext())
		{
			if (pCir->IsInCircle(pInfo->dim_pos))
			{
				pInfo->m_bIslandDetection = TRUE;
				break;
			}
		}
		//识别钢板轮廓边
		pInfo->InitProfileByBPolyCmd(fMinEdge, fMaxEdge, bSendCommand);
	}
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
}
//通过像素模拟进行提取钢板轮廓
void CPNCModel::ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	CLogErrorLife logErrLife;
	//根据Spline提取火曲线特征信息(分段线段集合)
	AcDbEntity *pEnt = NULL;
	CSymbolRecoginzer symbols;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if((pEnt=objLife.GetEnt())==NULL)
			continue;
		if (pEnt->isKindOf(AcDbSpline::desc()))
			symbols.AppendSymbolEnt((AcDbSpline*)pEnt);
	}
	//根据框选图元提取轮廓边元素,记录框选区域
	SCOPE_STRU scope;
	CHashSet<AcDbObjectId> hashScreenProfileId;
	ATOM_LIST<f3dArcLine> arrArcLine;
	for (AcDbObjectId objId = selectedEntIdSet.GetFirst(); objId; objId = selectedEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt())==NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()) && g_pncSysPara.IsBendLine((AcDbLine*)pEnt, &symbols))
			continue;	//过滤火曲线
		GEPOINT ptS, ptE, ptC, norm;
		f3dArcLine arc_line;
		AcGePoint3d acad_pt;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
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
				scope.VerityArcLine(arc_line);
				m_xAllLineHash.SetValue(objId.asOldId(), objId);
			}
			else
				logerr.Log("error");
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
				scope.VerityArcLine(arc_line);
				m_xAllLineHash.SetValue(objId.asOldId(), objId);
			}
			else
				logerr.Log("error");
		}
		else if (pEnt->isKindOf(AcDbPolyline::desc()))
		{	//多段线
			hashScreenProfileId.SetValue(objId.asOldId(), objId);
		}
		else if (pEnt->isKindOf(AcDbRegion::desc()))
		{	//面域

		}
	}
	//初始化坐标系,将轮廓线绘制到虚拟画布
	double fSacle = 0.6, fGap = 1 / fSacle;
	GECS ocs;
	ocs.origin.Set(scope.fMinX - fGap, scope.fMaxY + fGap, 0);
	ocs.axis_x.Set(1, 0, 0);
	ocs.axis_y.Set(0, -1, 0);
	ocs.axis_z.Set(0, 0, -1);
	CVectorMonoImage xImage;
	xImage.DisplayProcess = DisplayProcess;
	xImage.SetOCS(ocs, fSacle);
	if (DisplayProcess)
		DisplayProcess(0, "初始化工作进行中......");
	int nNum = arrArcLine.GetNodeNum();
	for (int i = 0; i < arrArcLine.GetNodeNum(); i++)
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * i / nNum), "初始化工作进行中......");
		xImage.DrawArcLine(arrArcLine[i], arrArcLine[i].feature);
	}
	if (DisplayProcess)
		DisplayProcess(100, "初始化工作完成");
	//识别钢板外轮廓边
	xImage.DetectProfilePixelsByVisit();
	//xImage.DetectProfilePixelsByTrack();
	//提取钢板轮廓点集合
	for (OBJECT_ITEM* pItem = xImage.m_hashRelaObj.GetFirst(); pItem; pItem = xImage.m_hashRelaObj.GetNext())
	{
		if (pItem->m_nPixelNum > 1)
			hashScreenProfileId.SetValue(pItem->m_idRelaObj, MkCadObjId(pItem->m_idRelaObj));
	}
	InitPlateVextexs(hashScreenProfileId);
}
void CPNCModel::InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts)
{
	CHashSet<AcDbObjectId> acdbPolylineSet;
	CHashSet<AcDbObjectId> acdbArclineSet;
	AcDbObjectId objId;
	for (objId = hashProfileEnts.GetFirst(); objId; objId = hashProfileEnts.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		if (pEnt->isKindOf(AcDbPolyline::desc()))
			acdbPolylineSet.SetValue(objId.asOldId(), objId);
		else
			acdbArclineSet.SetValue(objId.asOldId(), objId);
	}
	//根据多段线提取钢板轮廓点
	if (DisplayProcess)
		DisplayProcess(0, "提取钢板轮廓点");
	int nStep = 0, nNum = acdbPolylineSet.GetNodeNum() + acdbArclineSet.GetNodeNum();
	for (objId = acdbPolylineSet.GetFirst(); objId; objId = acdbPolylineSet.GetNext(), nStep++)
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * nStep / nNum), "提取钢板轮廓点");
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
	//根据轮廓直线提取钢板轮廓点(两次提取)
	CHashList<ACAD_LINEID> hashUnmatchLine;
	for (int index = 0; index < 2; index++)
	{
		ARRAY_LIST<ACAD_LINEID> line_arr;
		if (index == 0)
		{	//第一次从筛选的轮廓图元中提取闭合外形
			line_arr.SetSize(0, acdbArclineSet.GetNodeNum());
			for (objId = acdbArclineSet.GetFirst(); objId; objId = acdbArclineSet.GetNext())
				line_arr.append(ACAD_LINEID(objId.asOldId()));
		}
		else
		{	//第二次从所有轮廓图元中提取剩余的闭合外形
			line_arr.SetSize(0, m_xAllLineHash.GetNodeNum());
			for (objId = m_xAllLineHash.GetFirst(); objId; objId = m_xAllLineHash.GetNext())
				line_arr.append(ACAD_LINEID(objId.asOldId()));
		}
		for (int i = 0; i < line_arr.GetSize(); i++)
			line_arr[i].UpdatePos();
		int nSize = (index == 0) ? line_arr.GetSize() : hashUnmatchLine.GetNodeNum();
		while (nSize > 0)
		{
			if (DisplayProcess)
				DisplayProcess(int(100 * nStep / nNum), "提取钢板轮廓点");
			ACAD_LINEID* pStartLine = line_arr.GetFirst();
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
					for (CPlateObject::VERTEX* pVer = tem_plate.vertexList.GetFirst(); pVer;
						pVer = tem_plate.vertexList.GetNext())
						pCurPlateInfo->vertexList.append(*pVer);
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
						m_xAllLineHash.DeleteNode(line_arr[i].m_lineId);
					else
						hashUnmatchLine.SetValue(line_arr[i].m_lineId, line_arr[i]);
				}
				else
					hashUnmatchLine.DeleteNode(line_arr[i].m_lineId);
				//删除处理过的图元
				line_arr.RemoveAt(i);
				nCount--;
				i--;
				nStep++;
			}
			nSize = (index == 0) ? line_arr.GetSize() : hashUnmatchLine.GetNodeNum();
		}
	}
	if (DisplayProcess)
		DisplayProcess(100, "提取钢板轮廓点");
}
//处理钢板一板多号的情况
void CPNCModel::MergeManyPartNo()
{
	for(CPlateProcessInfo* pPlateProcess=EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=EnumNextPlate(TRUE))
	{
		if(pPlateProcess->vertexList.GetNodeNum()<=3)
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
				pNewPlateInfo->partNoId = objId;
				pNewPlateInfo->partNumId = (g_pncSysPara.m_iDimStyle == 0) ? objId : pPlateInfo->partNumId;
				pNewPlateInfo->xPlate.cMaterial = pPlateInfo->xPlate.cMaterial;
				pNewPlateInfo->xPlate.m_fThick = pPlateInfo->xPlate.m_fThick;
				pNewPlateInfo->xPlate.m_nProcessNum = pPlateInfo->xPlate.m_nProcessNum;
				pNewPlateInfo->xPlate.m_nSingleNum = pPlateInfo->xPlate.m_nSingleNum;
				//特殊工艺信息
				pNewPlateInfo->xBomPlate.siZhiWan = pPlateInfo->xBomPlate.siZhiWan;
				pNewPlateInfo->xBomPlate.bWeldPart = pPlateInfo->xBomPlate.bWeldPart;
			}
		}
		m_hashPlateInfo.pop_stack();
	}
}

void CPNCModel::LayoutPlates(BOOL bRelayout)
{
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	//
	f2dRect minRect;
	int minDistance=g_pncSysPara.m_nMinDistance;
	int hight=g_pncSysPara.m_nMapWidth;
	int paperLen=(g_pncSysPara.m_nMapLength<=0)?100000:g_pncSysPara.m_nMapLength;
	DRAGSET.ClearEntSet();
	CXhPtrSet<CPlateProcessInfo> needUpdatePlateList;
	CHashStrList<CDrawingRect> hashDrawingRectByLabel;
	for(CPlateProcessInfo *pPlate=m_hashPlateInfo.GetFirst();pPlate;pPlate=m_hashPlateInfo.GetNext())
	{
		fPtList ptList;
		minRect=pPlate->GetMinWrapRect((double)minDistance,&ptList);
		CDrawingRect *pRect=hashDrawingRectByLabel.Add(pPlate->GetPartNo());
		pRect->m_pDrawing=pPlate;
		pRect->height=minRect.Height();
		pRect->width=minRect.Width();
		pRect->m_vertexArr.Empty();
		for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
			pRect->m_vertexArr.append(GEPOINT(*pPt));
		pRect->topLeft.Set(minRect.topLeft.x,minRect.topLeft.y);
	}
	if(bRelayout==CPNCSysPara::LAYOUT_PRINT)
	{
		double paperX=0;
		while(hashDrawingRectByLabel.GetNodeNum()>0)
		{
			CDrawingRectLayout rectLayout;
			for(CDrawingRect *pRect=hashDrawingRectByLabel.GetFirst();pRect;pRect=hashDrawingRectByLabel.GetNext())
				rectLayout.drawRectArr.Add(*pRect);
			if (rectLayout.Relayout(hight, paperLen) == FALSE)
			{	//所有的板都布局失败
				for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				{
					CPlateProcessInfo *pPlateDraw = (CPlateProcessInfo*)pRect->m_pDrawing;
					if (pRect->m_bException)
						logerr.Log("钢板%s排版失败,钢板矩形宽度大于指定出图宽度!",(char*)pPlateDraw->GetPartNo());
					else
						logerr.Log("钢板%s排版失败!", (char*)pPlateDraw->GetPartNo());
				}
				break;
			}
			else
			{	//布局成功，但是其中可能某些板没有布局成功
				AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, 0), f3dPoint(paperX + paperLen, 0));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, -hight), f3dPoint(paperX + paperLen, -hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, 0), f3dPoint(paperX, -hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX + paperLen, 0), f3dPoint(paperX + paperLen, -hight));
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
					GEPOINT center(topLeft.x + drawingRect.width*0.5, topLeft.y - drawingRect.height*0.5);
					pPlate->DrawPlate(&topLeft, FALSE, TRUE, &center);
					//
					hashDrawingRectByLabel.DeleteNode(pPlate->GetPartNo());
				}
				paperX += (paperLen + 50);
			}
		}
		//已在排版函数中输出排版异常钢板 wht 18-11-27
		/*if(hashExceptionPlateByLabel.GetNodeNum()>0)
		{
			for(CPlateProcessInfo **ppPlateInfo=hashExceptionPlateByLabel.GetFirst();ppPlateInfo;ppPlateInfo=hashExceptionPlateByLabel.GetNext())
			{
				CPlateProcessInfo *pPlateInfo=*ppPlateInfo;
				logerr.Log("件号%s板排版异常",(char*)pPlateInfo->GetPartNo());
			}
		}*/
	}
	else if (bRelayout == CPNCSysPara::LAYOUT_SEG)
	{
		CSortedModel sortedModel(this);
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
			{	//根据轮廓点无法计算钢板矩形区域时根据CAD计算 wht 19-03-12
				ARRAY_LIST<AcDbObjectId> entIdList;
				for (CPlateProcessInfo::CAD_ENTITY *pEnt = pPlate->EnumFirstEnt(); pEnt; pEnt = pPlate->EnumNextEnt())
					entIdList.append(MkCadObjId(pEnt->idCadEnt));
				rect = GetCadEntRect(entIdList);
			}
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
		for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
		{
			if (pPlate->AutoCorrectedSteelSealPos())
				needUpdatePlateList.append(pPlate);
		}
	}
	else
	{
		for(CPlateProcessInfo *pPlate=m_hashPlateInfo.GetFirst();pPlate;pPlate=m_hashPlateInfo.GetNext())
			pPlate->DrawPlate(NULL);
	}
	//
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
	//
	if (bRelayout == CPNCSysPara::LAYOUT_SEG)
	{
		for (CPlateProcessInfo *pPlate = model.EnumFirstPlate(TRUE); pPlate; pPlate = model.EnumNextPlate(TRUE))
			pPlate->InitEdgeEntIdMap();	//初始化轮廓边对应关系
		for (CPlateProcessInfo *pPlate = needUpdatePlateList.GetFirst(); pPlate; pPlate = needUpdatePlateList.GetNext())
		{	//更新字盒子位置之后，同步更新PPI文件中钢印号位置
			pPlate->SyncSteelSealPos();
			//更新PPI文件
			CString file_path;
			GetCurWorkPath(file_path);
			pPlate->CreatePPiFile(file_path);
		}
	}
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

//////////////////////////////////////////////////////////////////////////
// CSortedModel
#include "SortFunc.h"
#include "ComparePartNoString.h"

typedef CPlateProcessInfo* CPlateInfoPtr;
int ComparePlatePtrByPartNo(const CPlateInfoPtr &plate1,const CPlateInfoPtr &plate2)
{
	CXhChar16 sPartNo1=plate1->xPlate.GetPartNo();
	CXhChar16 sPartNo2=plate2->xPlate.GetPartNo();
	return ComparePartNoString(sPartNo1,sPartNo2,"SHGPT");
}
CSortedModel::CSortedModel(CPNCModel *pModel)
{
	if(pModel==NULL)
		return;
	platePtrList.Empty();
	for(CPlateProcessInfo *pPlate=pModel->EnumFirstPlate(FALSE);pPlate;pPlate=pModel->EnumNextPlate(FALSE))
		platePtrList.append(pPlate);
	CHeapSort<CPlateInfoPtr>::HeapSort(platePtrList.m_pData,platePtrList.Size(),ComparePlatePtrByPartNo);
}
CPlateProcessInfo *CSortedModel::EnumFirstPlate()
{
	CPlateProcessInfo **ppPlate=platePtrList.GetFirst();
	if(ppPlate)
		return *ppPlate;
	else
		return NULL;
}
CPlateProcessInfo *CSortedModel::EnumNextPlate()
{
	CPlateProcessInfo **ppPlate=platePtrList.GetNext();
	if(ppPlate)
		return *ppPlate;
	else
		return NULL;
}
