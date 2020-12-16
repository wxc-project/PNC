#include "stdafx.h"
#include "XeroNcPart.h"
#include "TSPAlgorithm.h"
#include "PNCSysPara.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "PNCCryptCoreCode.h"
#include "XhMath.h"
#include "DimStyle.h"
#ifdef __TIMER_COUNT_
#include "TimerCount.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//静态函数
//过滤重复点
static BOOL IsHasVertex(ATOM_LIST<VERTEX>& tem_vertes, GEPOINT pt)
{
	for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
	{
		VERTEX* pVer = tem_vertes.GetByIndex(i);
		if (pVer->pos.IsEqual(pt, EPS2))
			return TRUE;
	}
	return FALSE;
}
//通过bpoly命令提取钢板轮廓点
void CALLBACK EXPORT CloseBoundaryPopupWnd(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	HWND hMainWnd = adsw_acadMainWnd();
	HWND hPopupWnd = GetLastActivePopup(hMainWnd);
	if (hPopupWnd != NULL && hPopupWnd != hMainWnd)
	{
		::SendMessage(hPopupWnd, WM_CLOSE, 0, 0);
		KillTimer(hMainWnd, 1);
	}
}
//
static BOOL IsInSector(double start_ang, double sector_ang, double verify_ang, BOOL bClockWise = TRUE)
{
	double ang = 0;
	if (!bClockWise)	//逆时针
		ang = ftoi(verify_ang - start_ang) % 360;
	else			//顺时针
		ang = ftoi(start_ang - verify_ang) % 360;
	if (ang < 0)
		ang += 360;
	if (sector_ang > ang || fabs(sector_ang - ang) < EPS)
		return TRUE;
	else
		return FALSE;
}
//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
BOOL CPlateProcessInfo::m_bCreatePPIFile = TRUE;	//默认输出ppi文件 wht 20-10-10
CPlateProcessInfo::CPlateProcessInfo()
{
	boltList.Empty();
	plateInfoBlockRefId = 0;
	partNoId = 0;
	partNumId = 0;
	m_bEnableReactor = TRUE;
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
//获取同一钢板区域内的关联件号
CXhChar200 CPlateProcessInfo::GetRelatePartNo()
{
	CXhChar200 sRelatePartNo;
	for (AcDbObjectId objId = relateEntList.GetFirst(); objId; objId = relateEntList.GetNext())
	{
		if(this->partNoId==objId)
			continue;
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		CXhChar16 sPartNo;
		if (g_pncSysPara.ParsePartNoText(pEnt, sPartNo))
		{
			if (sRelatePartNo.GetLength() <= 0)
				sRelatePartNo.Copy(sPartNo);
			else
				sRelatePartNo.Append(CXhChar16(",%s", (char*)sPartNo));
		}
	}
	return sRelatePartNo;
}
//判断提取成功的轮廓点是否按逆时针排序
BOOL CPlateProcessInfo::IsValidVertexs()
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
void CPlateProcessInfo::ReverseVertexs()
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
void CPlateProcessInfo::DeleteAssisstPts()
{	//去除辅助型顶点
	for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		if (pVer->tag.lParam == -1)
			vertexList.DeleteCursor();
	}
	vertexList.Clean();
}
void CPlateProcessInfo::UpdateVertexPropByArc(f3dArcLine& arcLine, int type)
{
	BOOL bFind = FALSE;
	int i = 0, iStart = -1, iEnd = -1;
	VERTEX* pVer = NULL;
	//根据圆弧更新顶点信息
	for (pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		if (pVer->pos.IsEqual(arcLine.Start(), CPNCModel::DIST_ERROR) && iStart == -1)
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
BOOL CPlateProcessInfo::RecogWeldLine(const double* ptS, const double* ptE)
{
	return RecogWeldLine(f3dLine(ptS, ptE));
}
BOOL CPlateProcessInfo::RecogWeldLine(f3dLine slop_line)
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
BOOL CPlateProcessInfo::IsClose(int* pIndex /*= NULL*/)
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
//
void CPlateProcessInfo::CreateRgn()
{
	ARRAY_LIST<f3dPoint> vertices;
	int nNum = vertexList.GetNodeNum();
	for (int i = 0; i < nNum; i++)
	{
		VERTEX* pCur = vertexList.GetByIndex(i);
		vertices.append(pCur->pos);
		if (pCur->ciEdgeType == 2)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			VERTEX* pNext = vertexList.GetByIndex((i + 1) % nNum);
			GEPOINT ptS = pCur->pos, ptE = pNext->pos;
			f3dArcLine arcLine;
			if (arcLine.CreateMethod3(ptS, ptE, pCur->arc.work_norm, pCur->arc.radius, pCur->arc.center))
			{
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
					vertices.append(arcLine.PositionInAngle(angle*i));
			}
		}
	}
	if (vertices.GetSize() > 0)
		m_xPolygon.CreatePolygonRgn(vertices.m_pData, vertices.GetSize());
}
//根据件号位置计算文本区域，便于UBOM识别基本信息
void CPlateProcessInfo::CreateRgnByText()
{
	CAcDbObjLife objLife(partNoId);
	AcDbEntity *pEnt = objLife.GetEnt();
	if (pEnt == NULL || !pEnt->isKindOf(AcDbText::desc()))
		return;
	CXhChar50 sText;
	AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
	sText.Copy(_bstr_t(pText->textString()));
#else
	sText.Copy(pText->textString());
#endif
	f3dPoint text_norm, dim_norm, dim_vec(1, 0, 0);
	Cpy_Pnt(text_norm, pText->normal());
	if (text_norm.IsZero())
		text_norm.Set(0, 0, 1);
	double height = pText->height();
	double angle = pText->rotation();
	double len = DrawTextLength(sText, height, pText->textStyle());
	dim_norm.Set(-sin(angle), cos(angle));
	RotateVectorAroundVector(dim_vec, angle, text_norm);
	//
	ARRAY_LIST<f3dPoint> vertexList;
	vertexList.append(f3dPoint(dim_pos + dim_norm * height * 3 - dim_vec * len * 2));
	vertexList.append(f3dPoint(dim_pos - dim_norm * height * 3 - dim_vec * len * 2));
	vertexList.append(f3dPoint(dim_pos - dim_norm * height * 3 + dim_vec * len * 2));
	vertexList.append(f3dPoint(dim_pos + dim_norm * height * 3 + dim_vec * len * 2));
	m_xPolygon.CreatePolygonRgn(vertexList.m_pData, vertexList.GetSize());
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
	for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
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
			if (fabs(arcLine.SectorAngle() - 2 * Pi) < EPS2)
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
			if (!pRelaObj->pos.IsEqual(cir_plate_para.cir_center, EPS2))
				continue;
			if (pCir->radius() < cir_plate_para.m_fRadius)
				cir_plate_para.m_fInnerR = pCir->radius();	//内圆半径
			else if (pCir->radius() == cir_plate_para.m_fRadius)
			{	//记录圆板轮廓点关联的图元ID
				for (int i = 0; i < vertexList.GetNodeNum(); i++)
					vertexList[i].tag.dwParam = pRelaObj->idCadEnt;
			}
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
			if (pEnt == NULL)
				continue;
			AcDbCurve* pCurve = (AcDbCurve*)pEnt;
			AcGePoint3d acad_ptS, acad_ptE;
			pCurve->getStartPoint(acad_ptS);
			pCurve->getEndPoint(acad_ptE);
			GEPOINT ptS, ptE;
			Cpy_Pnt(ptS, acad_ptS);
			Cpy_Pnt(ptE, acad_ptE);
			ptS.z = ptE.z = 0;
			if ((vertexS.IsEqual(ptS, CPNCModel::DIST_ERROR) && vertexE.IsEqual(ptE, CPNCModel::DIST_ERROR)) ||
				(vertexS.IsEqual(ptE, CPNCModel::DIST_ERROR) && vertexE.IsEqual(ptS, CPNCModel::DIST_ERROR)))
				break;
		}
		if (pCadLine)
			pCurVertex->tag.dwParam = pCadLine->idCadEnt;
	}
}
//更新钢板的螺栓孔信息
void CPlateProcessInfo::UpdateBoltHoles()
{
	if (!IsValid())
		return;
	boltList.Empty();
	//筛选同一位置包含多个图符或圆圈，同一圆心只保留直径最大的圆，以及件号圆圈
	std::map<CString, CAD_ENT_PTR> mapCirAndBlock;
	std::map<CString, CAD_ENT_PTR>::iterator iter;
	std::set<CAD_ENT_PTR> setInvalidBoltCir;
	for (CAD_ENTITY *pEnt = EnumFirstRelaEnt(); pEnt; pEnt = EnumNextRelaEnt())
	{
		if (pEnt->ciEntType != TYPE_BLOCKREF && pEnt->ciEntType != TYPE_CIRCLE)
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
			if (idCircleLineType.isValid() && idCircleLineType != GetLineTypeId("CONTINUOUS"))
			{	//根据制图规范，螺栓圆圈应该都是实线 wxc 20-06-19
				setInvalidBoltCir.insert(pEnt);
				continue;
			}
			if ((cir_plate_para.m_bCirclePlate && cir_plate_para.cir_center.IsEqual(pEnt->pos)) ||
				(pEnt->m_fSize<0 || pEnt->m_fSize>g_pncSysPara.m_nMaxHoleD))
			{	//环型板的同心圆和直径超过设定的最大直径的圆都不可能时螺栓孔 wxc 20-04-15
				setInvalidBoltCir.insert(pEnt);
				continue;
			}
		}
		CString sPosKey = CPNCModel::MakePosKeyStr(pEnt->pos);
		iter = mapCirAndBlock.find(sPosKey);
		if (iter == mapCirAndBlock.end())
			mapCirAndBlock.insert(std::make_pair(sPosKey, pEnt));
		else
		{
			if (iter->second->m_fSize > pEnt->m_fSize)
				setInvalidBoltCir.insert(pEnt);
			else
			{
				setInvalidBoltCir.insert(iter->second);
				mapCirAndBlock[sPosKey] = pEnt;
			}
		}
	}
	int nInvalidCirCountForText = 0;
	if (g_pncSysPara.IsFilterPartNoCir())
	{
		for (CAD_ENTITY *pEnt = EnumFirstRelaEnt(); pEnt; pEnt = EnumNextRelaEnt())
		{
			if (pEnt->ciEntType != TYPE_TEXT && pEnt->ciEntType != TYPE_MTEXT)
				continue;
			//标注字符串中包含"钻孔或冲孔"，
			if (strstr(pEnt->sText, "钻") != NULL || strstr(pEnt->sText, "冲") != NULL ||
				strstr(pEnt->sText, "Φ") != NULL || strstr(pEnt->sText, "%%C") != NULL ||
				strstr(pEnt->sText, "%%c") != NULL)
				continue;
			//排除件号标注圆圈(375# ， 375 ，正焊375)
			for (iter = mapCirAndBlock.begin(); iter != mapCirAndBlock.end(); iter++)
			{
				if (iter->second->ciEntType == TYPE_BLOCKREF)
					continue;	//螺栓图符内有文字时不需要过滤 wht 19-12-10
				if (g_pncSysPara.m_fPartNoCirD > 0 &&
					fabs(iter->second->m_fSize - g_pncSysPara.m_fPartNoCirD) >= EPS2)
					continue;	//用户指定件号圆圈孔径时，不满足孔径的不过滤 wxc-20-07-23
				SCOPE_STRU scope;
				double r = iter->second->m_fSize*0.5;
				scope.fMinX = iter->second->pos.x - r;
				scope.fMaxX = iter->second->pos.x + r;
				scope.fMinY = iter->second->pos.y - r;
				scope.fMaxY = iter->second->pos.y + r;
				scope.fMinZ = scope.fMaxZ = 0;
				if (scope.IsIncludePoint(pEnt->pos))
				{
					setInvalidBoltCir.insert(iter->second);
					nInvalidCirCountForText++;
				}
			}
		}
	}
	if (nInvalidCirCountForText > 0)
		logerr.Log("%s#钢板，已过滤%d个可能为件号标注的圆，请确认！", (char*)xPlate.GetPartNo(), nInvalidCirCountForText);
	//从单个图元(图块、圆圈)中提取螺栓信息
	for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
	{
		if (setInvalidBoltCir.find(pRelaObj)!=setInvalidBoltCir.end())
			continue;	//过滤掉无效的螺栓实体（大圆内的小圆、件号标注） wht 19-07-20
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
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
	//提取三角螺栓图符、腰圆孔螺栓图符
	if (g_pncSysPara.IsRecongPolylineLs() > 0)
	{
		for (CBoltEntGroup* pBoltGroup = m_pBelongModel->m_xBoltEntHash.GetFirst(); pBoltGroup;
			pBoltGroup = m_pBelongModel->m_xBoltEntHash.GetNext())
		{
			if (pBoltGroup->m_ciType < 2)
				continue;	//图符&圆圈已处理
			if (pBoltGroup->m_bMatch)
				continue;	//已匹配
			if (!IsInPartRgn(GEPOINT(pBoltGroup->m_fPosX, pBoltGroup->m_fPosY)))
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
			if (pBoltGroup->m_ciType == CBoltEntGroup::BOLT_WAIST_ROUND)
			{
				pBoltInfo->waistLen = pBoltGroup->GetWaistLen();
				pBoltInfo->waistVec = pBoltGroup->GetWaistVec();
			}
			else if (wBoltD > 0)
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
		for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
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
			AcDbEntity *pEnt = objLife.GetEnt();
			if (pEnt && pEnt->isKindOf(AcDbBlockReference::desc()))
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
//根据钢板的轮廓点初始化归属钢板的图元集合
void CPlateProcessInfo::ExtractRelaEnts()
{
	EmptyRelaEnts();
	AppendRelaEntity(partNoId.asOldId());
	if (!IsValid())
		return;
	//根据钢板轮廓区域进行缩放
	SCOPE_STRU scope;
	for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		scope.VerifyVertex(pVer->pos);
		//通过像素识别的轮廓边已记录了关联图元
		if (pVer->tag.lParam == 0 || pVer->tag.lParam == -1)
			continue;
		AppendRelaEntity(pVer->tag.lParam);
	}
	ZoomAcadView(scope, 10);
	//根据闭合区域拾取归属于钢板的图元集合
	ATOM_LIST<VERTEX> list;
	CalEquidistantShape(CPNCModel::DIST_ERROR * 2, &list);
#ifdef __ALFA_TEST_
	/*CLockDocumentLife lockCurDocment;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	if (pBlockTableRecord)
	{
		int nNum = list.GetNodeNum();
		for (int i = 0; i < nNum; i++)
		{
			VERTEX* pCurVer = list.GetByIndex(i);
			VERTEX* pNextVer = list.GetByIndex((i + 1) % nNum);
			CreateAcadLine(pBlockTableRecord, pCurVer->pos, pNextVer->pos, 0, 0, RGB(125, 255, 0));
		}
		pBlockTableRecord->close();
	}*/
#endif
	vector<GEPOINT> vectorProfilePt;
	for (VERTEX* pVer = list.GetFirst(); pVer; pVer = list.GetNext())
		vectorProfilePt.push_back(pVer->pos);
	CCadPartObject::ExtractRelaEnts(vectorProfilePt);
}

bool CPlateProcessInfo::RecogRollEdge(CHashSet<CAD_ENTITY*> &rollEdgeDimTextSet, f3dLine &line)
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
			VERTEX* pStartVertex = NULL, *pEndVertex = NULL, *pEndPrevVertex = NULL;
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
	if (!IsValid())
		return FALSE;
	//根据Spline提取火曲线标记信息(分段线段集合)
	AcDbEntity *pEnt = NULL;
	CSymbolRecoginzer symbols;
	CHashSet<CAD_ENTITY*> bendDimTextSet;
	CHashSet<CAD_ENTITY*> rollEdgeDimTextSet;
	CHashSet<BOOL> weldMarkLineSet;
	CHashStrList< ATOM_LIST<CAD_LINE> > hashLineArrByPosKeyStr;
	for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
	{
		if (pRelaObj->ciEntType != TYPE_SPLINE &&
			pRelaObj->ciEntType != TYPE_TEXT &&
			pRelaObj->ciEntType != TYPE_LINE &&
			pRelaObj->ciEntType != TYPE_ARC &&
			pRelaObj->ciEntType != TYPE_ELLIPSE)
			continue;
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pRelaObj->ciEntType == TYPE_SPLINE)
			symbols.AppendSymbolEnt((AcDbSpline*)pEnt);
		else if (pRelaObj->ciEntType == TYPE_TEXT)
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
	for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
	{
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		//提取基本信息
		if (plateInfoBlockRefId == NULL && g_pncSysPara.RecogBasicInfo(pEnt, baseInfo))
		{	//plateInfoBlockRefId有效时已从块中提取到了基本属性 wht 19-01-04
			if (baseInfo.m_cMat > 0)
				xPlate.cMaterial = baseInfo.m_cMat;
			if (baseInfo.m_nThick > 0)
				xPlate.m_fThick = (float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
				xPlate.m_nSingleNum = xPlate.m_nProcessNum = baseInfo.m_nNum;
			if (baseInfo.m_idCadEntNum != 0)
				partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			if (baseInfo.m_sTaType.GetLength() > 0 && m_pBelongModel->m_xPrjInfo.m_sTaType.GetLength() <= 0)
				m_pBelongModel->m_xPrjInfo.m_sTaType.Copy(baseInfo.m_sTaType);
			if (baseInfo.m_sPartNo.GetLength() > 0 && bRelatePN)
			{
				if (xPlate.GetPartNo().GetLength() <= 0)
					xPlate.SetPartNo(baseInfo.m_sPartNo);
				relateEntList.SetValue(pRelaObj->idCadEnt, MkCadObjId(pRelaObj->idCadEnt));
			}
			continue;
		}
		//提取钢印区
		f3dPoint ptArr[4];
		if (g_pncSysPara.RecogMkRect(pEnt, ptArr, 4))
		{
			xPlate.mkpos.Set((ptArr[0].x + ptArr[2].x) / 2, (ptArr[0].y + ptArr[2].y) / 2, (ptArr[0].z + ptArr[2].z) / 2);
			if (DISTANCE(ptArr[0], ptArr[1]) > DISTANCE(ptArr[1], ptArr[2]))
				xPlate.mkVec = ptArr[0] - ptArr[1];
			else
				xPlate.mkVec = ptArr[1] - ptArr[2];
			normalize(xPlate.mkVec);
			continue;
		}
		//提取火曲线信息
		if (pEnt->isKindOf(AcDbLine::desc()) && weldMarkLineSet.GetValue(pRelaObj->idCadEnt) == NULL)
		{
			f3dLine line;
			AcDbLine* pAcDbLine = (AcDbLine*)pEnt;
			Cpy_Pnt(line.startPt, pAcDbLine->startPoint());
			Cpy_Pnt(line.endPt, pAcDbLine->endPoint());
			if (g_pncSysPara.IsBendLine((AcDbLine*)pEnt, &symbols))
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
			else if (g_pncSysPara.IsSlopeLine((AcDbLine*)pEnt))
			{
				RecogWeldLine(line);
				continue;
			}
		}
	}
	return GetRelaEntCount() > 1;
}
f2dRect CPlateProcessInfo::GetPnDimRect(double fRectW /*= 10*/, double fRectH /*= 10*/)
{
	//文本坐标 this->dim_pos 已做居中处理
	f2dRect rect;
	rect.topLeft.Set(dim_pos.x - fRectW, dim_pos.y + fRectH);
	rect.bottomRight.Set(dim_pos.x + fRectW, dim_pos.y - fRectH);
	return rect;
}
//判断钢板是否为圆型钢板
bool CPlateProcessInfo::RecogCirclePlate(ATOM_LIST<VERTEX>& vertex_list)
{
	GEPOINT center;
	bool bValidCir = false;
	if (vertex_list.GetNodeNum() == 2)
	{
		bValidCir = true;
		for (int i = 0; i < vertex_list.GetNodeNum(); i++)
		{
			VERTEX* pCur = vertex_list.GetByIndex(i);
			if (center.IsZero())
				center = pCur->arc.center;
			double fAngle = RadToDegree(pCur->arc.fSectAngle);
			if (pCur->ciEdgeType != 2
				|| fabs(180 - fAngle) > 2
				|| !center.IsEqual(pCur->arc.center, EPS2))
			{
				bValidCir = false;
				break;
			}
		}
		if (bValidCir)
		{
			cir_plate_para.m_bCirclePlate = TRUE;
			cir_plate_para.cir_center = vertex_list[0].arc.center;
			cir_plate_para.m_fRadius = vertex_list[0].arc.radius;
		}
	}
	else if (vertex_list.GetNodeNum() == 4)
	{
		bValidCir = true;
		for (int i = 0; i < vertex_list.GetNodeNum(); i++)
		{
			VERTEX* pCur = vertex_list.GetByIndex(i);
			if (center.IsZero())
				center = pCur->arc.center;
			double fAngle = RadToDegree(pCur->arc.fSectAngle);
			if (pCur->ciEdgeType != 2
				|| fabs(90 - fAngle) > 2
				|| !center.IsEqual(pCur->arc.center, EPS2))
			{
				bValidCir = false;
				break;
			}
		}
		if (bValidCir)
		{
			cir_plate_para.m_bCirclePlate = TRUE;
			cir_plate_para.cir_center = vertex_list[0].arc.center;
			cir_plate_para.m_fRadius = vertex_list[0].arc.radius;
		}
	}
	return bValidCir;
}
//
void CPlateProcessInfo::InitProfileByBPolyCmd(double fMinExtern, double fMaxExtern, BOOL bIslandDetection /*= FALSE*/)
{
#ifdef __ALFA_TEST_
	//用于测试查看文本的坐标位置
	/*CLockDocumentLife lockCurDocument;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y + 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y + 10), f3dPoint(dim_pos.x + 10, dim_pos.y - 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x + 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y - 10));
	CreateAcadLine(pBlockTableRecord, f3dPoint(dim_pos.x - 10, dim_pos.y - 10), f3dPoint(dim_pos.x - 10, dim_pos.y + 10));
	pBlockTableRecord->close();*/
#endif
	//
	ads_name seqent;
	AcDbObjectId initLastObjId, plineId;
	acdbEntLast(seqent);
	acdbGetObjectId(initLastObjId, seqent);
	//将件号标注位置进行缩放,找到合适的缩放比例,通过boundary命令提取轮廓边
	f2dRect rect = GetPnDimRect();
	double fExternLen = fMaxExtern * 3;
	fExternLen = (fExternLen > 2200) ? 2200 : fExternLen;
	if (g_pncSysPara.m_bUseMaxEdge)
		fExternLen = g_pncSysPara.m_nMaxEdgeLen;
	double fScale = (fExternLen - fMinExtern) / 10;
	//
	HWND hMainWnd = adsw_acadMainWnd();
	f3dPoint cur_dim_pos = dim_pos;
	for (int i = 0; i <= 10; i++)
	{
		if (i > 0)
			fExternLen -= fScale;
		double fZoomScale = fExternLen / g_pncSysPara.m_fMapScale;
		ZoomAcadView(rect, fZoomScale);
		ads_point base_pnt;
		base_pnt[X] = cur_dim_pos.x;
		base_pnt[Y] = cur_dim_pos.y;
		base_pnt[Z] = cur_dim_pos.z;
		UINT_PTR nTimer = SetTimer(hMainWnd, 1, 100, CloseBoundaryPopupWnd);
		int resCode = RTNORM;
		if (CPNCModel::m_bSendCommand)
		{
			CXhChar50 sCmd, sPos("%.2f,%.2f", cur_dim_pos.x, cur_dim_pos.y);
			if (bIslandDetection)
				sCmd.Printf("-boundary %s\n ", (char*)sPos);
			else
				sCmd.Printf("-boundary a i n\n \n%s\n ", (char*)sPos);
#ifdef _ARX_2007
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			if (bIslandDetection)
				resCode = acedCommand(RTSTR, L"-boundary", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
			else
				resCode = acedCommand(RTSTR, L"-boundary", RTSTR, L"a", RTSTR, L"i", RTSTR, L"n", RTSTR, L"", RTSTR, L"", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
#else
			if (bIslandDetection)
				resCode = acedCommand(RTSTR, "-boundary", RTPOINT, base_pnt, RTSTR, "", RTNONE);
			else
				resCode = acedCommand(RTSTR, "-boundary", RTSTR, "a", RTSTR, "i", RTSTR, "n", RTSTR, "", RTSTR, "", RTPOINT, base_pnt, RTSTR, "", RTNONE);
#endif
		}
		if (nTimer == 1)
			KillTimer(hMainWnd, nTimer);
		if (resCode != RTNORM)
			return;
		acdbEntLast(seqent);
		acdbGetObjectId(plineId, seqent);
		if (initLastObjId != plineId)
			break;
	}
	if (initLastObjId == plineId)
	{	//执行空命令行(代表输入回车)，避免重复执行上一条命令 wxc-2019.6.13
		if (CPNCModel::m_bSendCommand)
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
	XhAcdbOpenAcDbEntity(pEnt, plineId, AcDb::kForWrite);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPolyline::desc()))
	{
		if (pEnt)
			pEnt->close();
		//执行空命令行(代表输入回车)，避免重复执行上一条命令 wxc-2019.6.13
		if (CPNCModel::m_bSendCommand)
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
	AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
	int nVertNum = pPline->numVerts();
	ATOM_LIST<VERTEX> tem_vertes;
	VERTEX* pVer = NULL;
	for (int iVertIndex = 0; iVertIndex < nVertNum; iVertIndex++)
	{
		AcGePoint3d location;
		pPline->getPointAt(iVertIndex, location);
		if (IsHasVertex(tem_vertes, GEPOINT(location.x, location.y, location.z)))
			continue;
		f3dPoint startPt, endPt;
		if (pPline->segType(iVertIndex) == AcDbPolyline::kLine)
		{
			AcGeLineSeg3d acgeLine;
			pPline->getLineSegAt(iVertIndex, acgeLine);
			Cpy_Pnt(startPt, acgeLine.startPoint());
			Cpy_Pnt(endPt, acgeLine.endPoint());
			pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
		}
		else if (pPline->segType(iVertIndex) == AcDbPolyline::kArc)
		{
			AcGeCircArc3d acgeArc;
			pPline->getArcSegAt(iVertIndex, acgeArc);
			f3dPoint center, norm;
			Cpy_Pnt(startPt, acgeArc.startPoint());
			Cpy_Pnt(endPt, acgeArc.endPoint());
			Cpy_Pnt(center, acgeArc.center());
			Cpy_Pnt(norm, acgeArc.normal());
			double fAngle = fabs(acgeArc.endAng() - acgeArc.startAng());
			if (fAngle > Pi)
				fAngle = 2 * Pi - fAngle;
			pVer = tem_vertes.append();
			pVer->pos.Set(location.x, location.y, location.z);
			pVer->ciEdgeType = 2;
			pVer->arc.center = center;
			pVer->arc.work_norm = norm;
			pVer->arc.radius = acgeArc.radius();
			pVer->arc.fSectAngle = fAngle;
		}
	}
	pPline->erase(Adesk::kTrue);	//删除polyline对象
	pPline->close();
	//
	nVertNum = tem_vertes.GetNodeNum();
	for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
	{
		VERTEX* pCurVer = tem_vertes.GetByIndex(i);
		if (pCurVer->ciEdgeType == 2 && ftoi(pCurVer->arc.radius) == CPNCModel::ASSIST_RADIUS)
		{
			VERTEX* pNextVer = tem_vertes.GetByIndex((i + 1) % nVertNum);
			if (pNextVer->ciEdgeType == 1)
				pNextVer->pos = pCurVer->arc.center;
			else if (pNextVer->ciEdgeType == 2 && ftoi(pNextVer->arc.radius) != CPNCModel::ASSIST_RADIUS)
				pNextVer->pos = pCurVer->arc.center;
		}
	}
	for (pVer = tem_vertes.GetFirst(); pVer; pVer = tem_vertes.GetNext())
	{
		if (pVer->ciEdgeType == 2 && ftoi(pVer->arc.radius) == CPNCModel::ASSIST_RADIUS)
			tem_vertes.DeleteCursor();
	}
	tem_vertes.Clean();
	//判断钢板是否为圆型钢板
	if (RecogCirclePlate(tem_vertes))
	{	//将整圆简化为四个90度圆弧
		tem_vertes.Empty();
		double fRadius = cir_plate_para.m_fRadius;
		GEPOINT center = cir_plate_para.cir_center;
		for (int i = 0; i < 4; i++)
		{
			VERTEX* pVer = tem_vertes.append();
			if (i == 0)
				pVer->pos.Set(center.x + fRadius, center.y, center.z);
			else if (i == 1)
				pVer->pos.Set(center.x, center.y + fRadius, center.z);
			else if (i == 2)
				pVer->pos.Set(center.x - fRadius, center.y, center.z);
			else if (i == 3)
				pVer->pos.Set(center.x, center.y - fRadius, center.z);
			pVer->ciEdgeType = 2;
			pVer->arc.radius = fRadius;
			pVer->arc.center = center;
			pVer->arc.fSectAngle = 0.5*Pi;
			pVer->arc.work_norm.Set(0, 0, 1);
		}
	}
	//填充钢板的轮廓点
	EmptyVertexs();
	for (int i = 0; i < tem_vertes.GetNodeNum(); i++)
	{
		VERTEX* pCur = tem_vertes.GetByIndex(i);
		pVer = vertexList.append(*pCur);
		if (pVer->ciEdgeType == 2 && !cir_plate_para.m_bCirclePlate)
		{	//将圆弧简化为多条线段，减小钢板轮廓区域误差
			pVer->ciEdgeType = 1;
			VERTEX* pNext = tem_vertes.GetByIndex((i + 1) % tem_vertes.GetNodeNum());
			f3dPoint startPt = pCur->pos, endPt = pNext->pos;
			double len = 0.5*DISTANCE(startPt, endPt);
			if (len < EPS2)
				continue;	//圆弧起始点、终止点距离过近导致死机（CreateMethod3需改进） wht 19-01-02
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
	//对轮廓点进行合理性检查(是否按逆时针排序)
	if (!IsValidVertexs())
		ReverseVertexs();
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
	EmptyVertexs();
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
	EmptyVertexs();
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
	EmptyVertexs();
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
	return TRUE;
}
void CPlateProcessInfo::InitMkPos(GEPOINT &mk_pos, GEPOINT &mk_vec)
{
	GEPOINT dim_pos, dim_vec;
	for (AcDbObjectId objId = relateEntList.GetFirst(); objId; objId = relateEntList.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
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
	f2dRect rect = GetMinWrapRect();
	xPlate.m_wLength = (rect.Width() > rect.Height()) ? ftoi(rect.Width()) : ftoi(rect.Height());
	xPlate.m_fWidth = (rect.Width() > rect.Height()) ? (float)ftoi(rect.Height()) : (float)ftoi(rect.Width());
	//初始化钢板的工艺信息
	VERTEX* pVer = NULL;
	PROFILE_VER* pNewVer = NULL;
	xPlate.vertex_list.Empty();
	for (pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
	{
		pNewVer = xPlate.vertex_list.Add(0);
		pNewVer->type = pVer->ciEdgeType;
		pNewVer->vertex = pVer->pos;
		if (pVer->ciEdgeType > 1)
		{
			pNewVer->radius = pVer->arc.radius;
			pNewVer->sector_angle = pVer->arc.fSectAngle;
			pNewVer->center = pVer->arc.center;
			pNewVer->work_norm = pVer->arc.work_norm;
			pNewVer->column_norm = pVer->arc.column_norm;
		}
		pNewVer->m_bWeldEdge = pVer->m_bWeldEdge;
		pNewVer->m_bRollEdge = pVer->m_bRollEdge;
		pNewVer->manu_space = pVer->manu_space;
		//
		pNewVer->vertex *= g_pncSysPara.m_fMapScale;
		pNewVer->center *= g_pncSysPara.m_fMapScale;
	}
	if (cir_plate_para.m_bCirclePlate && cir_plate_para.m_fInnerR > 0)
	{
		xPlate.m_fInnerRadius = cir_plate_para.m_fInnerR;
		xPlate.m_tInnerOrigin = cir_plate_para.cir_center;
		xPlate.m_tInnerColumnNorm = cir_plate_para.column_norm;
	}
	BOLT_INFO* pBolt = NULL, *pNewBolt = NULL;
	xPlate.m_xBoltInfoList.Empty();
	for (pBolt = boltList.GetFirst(); pBolt; pBolt = boltList.GetNext())
	{
		pNewBolt = xPlate.m_xBoltInfoList.Add(0);
		pNewBolt->CloneBolt(pBolt);
		pNewBolt->posX *= (float)g_pncSysPara.m_fMapScale;
		pNewBolt->posY *= (float)g_pncSysPara.m_fMapScale;
	}
	if (xPlate.mkpos.IsZero())
		InitMkPos(xPlate.mkpos, xPlate.mkVec);
	xPlate.mkpos *= g_pncSysPara.m_fMapScale;
	xPlate.m_bIncDeformed = g_pncSysPara.m_bIncDeformed;
	if (xPlate.mcsFlg.ciBottomEdge == (BYTE)-1)
		InitBtmEdgeIndex();
}
void CPlateProcessInfo::CreatePPiFile(const char* file_path)
{
	if (!m_bCreatePPIFile)
		return;	//不输出ppi文件
	//设置当前工作路径
	SetCurrentDirectory(file_path);
	CXhChar200 sRelatePartNo = GetRelatePartNo();
	CXhChar200 sAllRelPart(xPlate.GetPartNo());
	if (g_pncSysPara.m_iPPiMode == 1 && sRelatePartNo.GetLength() > 0)
	{	//一板多号模式：一个PPI文件包括多个件号
		xPlate.m_sRelatePartNo.Copy(sRelatePartNo);
		sAllRelPart.Printf("%s,%s", (char*)xPlate.GetPartNo(), (char*)sRelatePartNo);
		sAllRelPart.Replace(",", " ");
	}
	CBuffer buffer;
	xPlate.ToPPIBuffer(buffer);
	CString sFilePath;
	sFilePath.Format(".\\%s.ppi", (char*)sAllRelPart);
	FILE* fp = fopen(sFilePath, "wb");
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
		logerr.Log("%s#钢板ppi文件保存失败,错误号#%d！路径：%s%s", (char*)xPlate.GetPartNo(), nRetCode, file_path, sFilePath);
	}
}
//属性成员拷贝
void CPlateProcessInfo::CopyAttributes(CPlateProcessInfo* pSrcPlate)
{
	CXhChar16 sDestPartNo = GetPartNo();
	pSrcPlate->xPlate.ClonePart(&xPlate);
	xPlate.cQuality = pSrcPlate->xPlate.cQuality;
	if (sDestPartNo.GetLength() > 1)
		xPlate.SetPartNo(sDestPartNo);	//保留原来的件号（分解一板多号的时候） wxc-20.09.09
	//
	dim_pos = pSrcPlate->dim_pos;
	dim_vec = pSrcPlate->dim_vec;
	EmptyVertexs();
	VERTEX* pSrcVertex = NULL;
	for (VERTEX* pSrcVertex = pSrcPlate->vertexList.GetFirst(); pSrcVertex;
		pSrcVertex = pSrcPlate->vertexList.GetNext())
		vertexList.append(*pSrcVertex);
	boltList.Empty();
	for (BOLT_INFO* pSrcBolt = pSrcPlate->boltList.GetFirst(); pSrcBolt; pSrcBolt = pSrcPlate->boltList.GetNext())
		boltList.append(*pSrcBolt);
	//
	EmptyRelaEnts();
	for (CAD_ENTITY* pSrcCadEnt = pSrcPlate->EnumFirstRelaEnt(); pSrcCadEnt;
		pSrcCadEnt = pSrcPlate->EnumNextRelaEnt())
		AddRelaEnt(pSrcCadEnt->idCadEnt, *pSrcCadEnt);
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
	int i = 0, n = xPlate.vertex_list.GetNodeNum(), bottom_edge = -1, weld_edge = -1;
	DYN_ARRAY<f3dPoint> vertex_arr(n);
	for (PROFILE_VER* pVertex = xPlate.vertex_list.GetFirst(); pVertex; pVertex = xPlate.vertex_list.GetNext(), i++)
	{
		vertex_arr[i] = pVertex->vertex;
		if (pVertex->type > 1)
			vertex_arr[i].feature = -1;
		if (pVertex->m_bWeldEdge)
			weld_edge = i;
	}
	if (g_pncSysPara.m_iAxisXCalType == 1)
	{	//优先根据螺栓确定X轴所在轮廓边
		for (i = 0; i < n; i++)
		{
			if (vertex_arr[i].feature == -1)
				continue;
			f3dLine edge_line(vertex_arr[i], vertex_arr[(i + 1) % n]);
			CHashList<f3dPoint> xVerTagHash;
			for (BOLT_INFO* pLs = xPlate.m_xBoltInfoList.GetFirst(); pLs; pLs = xPlate.m_xBoltInfoList.GetNext())
			{
				f3dPoint ls_pt(pLs->posX, pLs->posY, 0), perp;
				double fDist = 0;
				if (SnapPerp(&perp, edge_line, ls_pt, &fDist) == FALSE || edge_line.PtInLine(perp, 1) != 2)
					continue;	//螺栓投影点不在轮廓边上
				f3dPoint* pTag = xVerTagHash.GetValue(ftoi(fDist));
				if (pTag == NULL)
					pTag = xVerTagHash.Add(ftoi(fDist));
				pTag->feature += 1;
			}
			for (f3dPoint* pTag = xVerTagHash.GetFirst(); pTag; pTag = xVerTagHash.GetNext())
			{
				if (pTag->feature > 1 && pTag->feature > vertex_arr[i].feature)
					vertex_arr[i].feature = pTag->feature;
			}
		}
		int fMaxLs = 0;
		for (i = 0; i < n; i++)
		{
			if (vertex_arr[i].feature > fMaxLs)
			{
				bottom_edge = i;
				fMaxLs = vertex_arr[i].feature;
			}
		}
	}
	else if (g_pncSysPara.m_iAxisXCalType == 2)
	{	//以焊接边作为加工坐标系的X坐标轴
		bottom_edge = weld_edge;
	}
	if (bottom_edge > -1)
		xPlate.mcsFlg.ciBottomEdge = (BYTE)bottom_edge;
	else
	{	//以最长边作为加工坐标系的X坐标轴
		GEPOINT vertice, prev_vec, edge_vec;
		double prev_edge_dist = 0, edge_dist = 0, max_edge = 0;
		for (i = 0; i < n; i++)
		{
			if (vertex_arr[i].feature == -1)
				continue;
			edge_vec = vertex_arr[(i + 1) % n] - vertex_arr[i];
			edge_dist = edge_vec.mod();
			if (edge_dist < EPS2)
				continue;
			edge_vec /= edge_dist;	//单位化边矢量
			if (i > 0 && prev_vec*edge_vec > EPS_COS)	//连续共线边轮廓
				edge_dist += edge_dist + prev_edge_dist;
			if (edge_dist > max_edge)
			{
				max_edge = edge_dist;
				bottom_edge = i;
			}
			prev_edge_dist = edge_dist;
			prev_vec = edge_vec;
		}
		xPlate.mcsFlg.ciBottomEdge = (BYTE)bottom_edge;
	}
}
//获取钢板调整后的加工坐标
void CPlateProcessInfo::BuildPlateUcs()
{
	int i = 0, n = xPlate.vertex_list.GetNodeNum();
	if (n <= 0)
		return;
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	for (PROFILE_VER* pVertex = xPlate.vertex_list.GetFirst(); pVertex; pVertex = xPlate.vertex_list.GetNext(), i++)
		vertex_arr[i] = pVertex->vertex;
	if (xPlate.mcsFlg.ciBottomEdge == -1 || xPlate.mcsFlg.ciBottomEdge >= n)
	{	//初始化加工坐标系
		WORD bottom_edge;
		GEPOINT vertice, prev_vec, edge_vec;
		double prev_edge_dist = 0, edge_dist = 0, max_edge = 0;
		for (i = 0; i < n; i++)
		{
			edge_vec = vertex_arr[(i + 1) % n] - vertex_arr[i];
			edge_dist = edge_vec.mod();
			edge_vec /= edge_dist;	//单位化边矢量
			if (!dim_vec.IsZero() && fabs(dim_vec*edge_vec) > EPS_COS)
			{	//优先查找与文字标注方向相同的边作为基准边
				bottom_edge = i;
				break;
			}
			if (i > 0 && prev_vec*edge_vec > EPS_COS)	//连续共线边轮廓
				edge_dist += edge_dist + prev_edge_dist;
			if (edge_dist > max_edge)
			{
				max_edge = edge_dist;
				bottom_edge = i;
			}
			prev_edge_dist = edge_dist;
			prev_vec = edge_vec;
		}
		//
		xPlate.mcsFlg.ciBottomEdge = (BYTE)bottom_edge;
	}
	//根据bottom_edge_i计算加工坐标系
	int iEdge = xPlate.mcsFlg.ciBottomEdge;
	f3dPoint prev_pt = vertex_arr[(iEdge - 1 + n) % n];
	f3dPoint cur_pt = vertex_arr[iEdge];
	f3dPoint next_pt = vertex_arr[(iEdge + 1) % n];
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
	ucs.axis_y.Set(-ucs.axis_x.y, ucs.axis_x.x);
	ucs.axis_z = ucs.axis_x^ucs.axis_y;
	//调整坐标系原点
	SCOPE_STRU scope = GetPlateScope(TRUE);
	ucs.origin += scope.fMinX*ucs.axis_x;
	//如果钢板进行反转，更新钢板坐标系
	if (xPlate.mcsFlg.ciOverturn == TRUE)
	{
		ucs.origin += ((scope.fMaxX - scope.fMinX)*ucs.axis_x);
		ucs.axis_x *= -1.0;
		ucs.axis_z *= -1.0;
	}
}
SCOPE_STRU CPlateProcessInfo::GetPlateScope(BOOL bVertexOnly, BOOL bDisplayMK/*=TRUE*/)
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
		if (!bVertexOnly)
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
	else if (IsValid())
	{
		for (VERTEX* pVer = vertexList.GetFirst(); pVer; pVer = vertexList.GetNext())
			scope.VerifyVertex(pVer->pos);
	}
	else
	{	//件号标注区域
		f2dRect rect = GetPnDimRect();
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
bool CPlateProcessInfo::DrawPlate(f3dPoint *pOrgion/*=NULL*/, BOOL bCreateDimPos/*=FALSE*/,
	BOOL bDrawAsBlock/*=FALSE*/, GEPOINT *pPlateCenter /*= NULL*/,
	double scale /*= 0*/, BOOL bSupportRotation /*= TRUE*/)
{
	CLockDocumentLife lockCurDocumentLife;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	if (pBlockTableRecord == NULL)
	{
		AfxMessageBox("获取块表记录失败!");
		return false;
	}
	AcGeMatrix3d scaleMat;
	AcGeMatrix3d moveMat, rotationMat;
	if (pOrgion)
	{	//计算平移基准点及旋转角度
		ads_point ptFrom, ptTo;
		ptFrom[X] = datumStartVertex.srcPos.x;
		ptFrom[Y] = datumStartVertex.srcPos.y;
		ptFrom[Z] = 0;
		ptTo[X] = pOrgion->x + datumStartVertex.offsetPos.x;
		ptTo[Y] = pOrgion->y + datumStartVertex.offsetPos.y;
		ptTo[Z] = 0;
		moveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
		//
		GEPOINT src_vec = datumEndVertex.srcPos - datumStartVertex.srcPos;
		GEPOINT dest_vec = datumEndVertex.offsetPos - datumStartVertex.offsetPos;
		double fDegAngle = Cal2dLineAng(0, 0, dest_vec.x, dest_vec.y) - Cal2dLineAng(0, 0, src_vec.x, src_vec.y);
		rotationMat.setToRotation(fDegAngle, AcGeVector3d::kZAxis, AcGePoint3d(ptTo[X], ptTo[Y], ptTo[Z]));
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
	for (CAD_ENTITY *pRelaObj = EnumFirstRelaEnt(); pRelaObj; pRelaObj = EnumNextRelaEnt())
	{
		CAcDbObjLife objLife(MkCadObjId(pRelaObj->idCadEnt));
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt == NULL)
			continue;
		AcDbEntity *pClone = (AcDbEntity *)pEnt->clone();
		if (pClone)
		{
			if (!IsValid())
				pClone->setColorIndex(1);	//
			if (pOrgion)
			{
				pClone->transformBy(moveMat);		//平移
				if (bSupportRotation)
					pClone->transformBy(rotationMat);	//旋转
			}
			if (pPlateCenter&&bDrawAsBlock)
				pClone->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			if (fabs(scale) > 0 && fabs(scale - 1) > EPS)
				pClone->transformBy(scaleMat);		//按指定比例缩放钢板 wht 20-01-22
			AcDbObjectId entId;
			DRAGSET.AppendAcDbEntity(pCurBlockTblRec, entId, pClone);
			m_cloneEntIdList.append(entId.asOldId());
			m_hashColneEntIdBySrcId.SetValue(pRelaObj->idCadEnt, entId.asOldId());
			pClone->close();
		}
	}
	if (bCreateDimPos)
	{	//生成标注点
		AcDbObjectId pointId;
		AcDbPoint *pPoint = new AcDbPoint(AcGePoint3d(dim_pos.x, dim_pos.y, dim_pos.z));
		if (DRAGSET.AppendAcDbEntity(pCurBlockTblRec, pointId, pPoint))
		{
			if (pOrgion)
			{
				pPoint->transformBy(moveMat);		//平移
				pPoint->transformBy(rotationMat);	//旋转
			}
			if (pPlateCenter&&bDrawAsBlock)
				pPoint->transformBy(blockMoveMat);	//作为块输出时，移动至原点位置，设置块插入点为矩形中心点 wht 19-07-25
			m_xMkDimPoint.idCadEnt = pointId.asOldId();
			AcGePoint3d pos = pPoint->position();
			m_xMkDimPoint.pos.Set(pos.x, pos.y, 0);
		}
		pPoint->close();
	}
	//对于提取失败或不闭合的钢板做特殊处理
	int index = -1;
	if (!IsValid() || !IsClose(&index))
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
				if (pPlateCenter && bDrawAsBlock)
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
	if (IsValid())
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
		for (int i = 0; i < xPlate.m_cFaceN - 1; i++)
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
			if (pHole->bWaistBolt)
			{	//绘制腰圆孔
				GEPOINT vecH = pHole->waistVec, vecN(-vecH.y, vecH.x, 0);
				GEPOINT ptUpS, ptUpE, ptDwS, ptDwE, ptC(pHole->posX, pHole->posY);
				ptUpS = ptC - vecH * pHole->waistLen*0.5 + vecN * fHoleR;
				ptUpE = ptC + vecH * pHole->waistLen*0.5 + vecN * fHoleR;
				ptDwS = ptC - vecH * pHole->waistLen*0.5 - vecN * fHoleR;
				ptDwE = ptC + vecH * pHole->waistLen*0.5 - vecN * fHoleR;
				entId = CreateAcadLine(pBlockTableRecord, ptUpS, ptUpE, 0, 0, clr);
				m_newAddEntIdList.append(entId.asOldId());
				entId = CreateAcadLine(pBlockTableRecord, ptDwS, ptDwE, 0, 0, clr);
				m_newAddEntIdList.append(entId.asOldId());
				f3dArcLine arcline;
				arcline.CreateMethod3(ptUpS, ptDwS, GEPOINT(0, 0, 1), fHoleR, ptC);
				entId = CreateAcadArcLine(pBlockTableRecord, arcline, 0, clr);
				m_newAddEntIdList.append(entId.asOldId());
				arcline.CreateMethod3(ptDwE, ptUpE, GEPOINT(0, 0, 1), fHoleR, ptC);
				entId = CreateAcadArcLine(pBlockTableRecord, arcline, 0, clr);
				m_newAddEntIdList.append(entId.asOldId());
			}
			else
			{	//绘制圆圈
				entId = CreateAcadCircle(pBlockTableRecord, f3dPoint(pHole->posX, pHole->posY, 0), fHoleR, 0, clr);
				m_newAddEntIdList.append(entId.asOldId());
			}
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

void CPlateProcessInfo::CalEquidistantShape(double minDistance, ATOM_LIST<VERTEX> *pDestList)
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
	if (minDistance > -eps && minDistance < eps)
	{	//轮廓边不进行伸缩
		for (VERTEX *vertex = tem_vertes.GetFirst(); vertex; vertex = tem_vertes.GetNext())
			pDestList->append(*vertex);
		return;
	}
	//对直线进行延展
	for (VERTEX *vertex = tem_vertes.GetFirst(); vertex; vertex = tem_vertes.GetNext())
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
		double angle = cal_angle_of_2vec(preLineVec, nextLineVec);
		double offset = minDistance / sin(angle / 2);
		f3dPoint vec = preLineVec + nextLineVec;
		normalize(vec);
		if (DistOf2dPtLine(nextPt, prePt, curPt) < 0)
			vec *= -1;	//凹点调整偏移方向
		//添加新轮廓点并增大轮廓点 wht 19-08-14
		VERTEX *pDestVertex = pDestList->append();
		pDestVertex->pos = GEPOINT(curPt + vec * offset);
	}
	//去除凹点，交叉点
	for (;;)
	{
		GEPOINT pre, now, next;
		BOOL bAllProtrusive = TRUE;
		for (int j = 0; j < pDestList->GetNodeNum(); j++)
		{
			if (j == 0)
				pre = pDestList->GetByIndex(pDestList->GetNodeNum() - 1)->pos;
			else
				pre = pDestList->GetByIndex(j - 1)->pos;
			now = pDestList->GetByIndex(j)->pos;
			if (j + 1 >= pDestList->GetNodeNum())
				next = pDestList->GetByIndex(0)->pos;
			else
				next = pDestList->GetByIndex(j + 1)->pos;
			if (DISTANCE(pre, now) < 0.5)
			{	//两点过于接近删除一点
				pDestList->DeleteAt(j);
				j--;
				continue;
			}
			f3dLine line(pre, next);
			double dd = DistOf2dPtLine(next, pre, now);
			if (dd < -eps || line.PtInLine(now)>0)
			{
				if (dd < -eps)
					bAllProtrusive = FALSE;
				pDestList->DeleteAt(j);
				j--;
			}
		}
		if (bAllProtrusive)	//边界多边形是凸多边形
			break;
	}
}
f2dRect CPlateProcessInfo::GetMinWrapRect(double minDistance/*=0*/, fPtList *pVertexList/*=NULL*/, 
				double dfMaxRectW /*= 0*/)
{
	f2dRect rect;
	rect.SetRect(f2dPoint(0, 0), f2dPoint(0, 0));
	int i = 0, n = vertexList.GetNodeNum();
	if (n < 3)
		return rect;
	GECS tmucs, minUcs;
	SCOPE_STRU scope, min_scope;
	typedef VERTEX* VERTEX_PTR;
	ATOM_LIST<VERTEX> list;
	if (minDistance > EPS)
		CalEquidistantShape(minDistance, &list);
	else
	{
		for (VERTEX *pVertex = vertexList.GetFirst(); pVertex; pVertex = vertexList.GetNext())
			list.append(*pVertex);
	}
	n = list.GetNodeNum();
	//定义轮廓点数组时使用list.GetNodeNum(计算等距点时会将圆弧按小直线段处理),否则会导致内存越界死机 wht 19-08-18
	DYN_ARRAY<VERTEX_PTR> vertex_arr;
	vertex_arr.Resize(n);
	for (VERTEX *pVertex = list.GetFirst(); pVertex; pVertex = list.GetNext(), i++)
		vertex_arr[i] = pVertex;
	tmucs.axis_z.Set(0, 0, 1);
	double minarea = 10000000000000000;	//预置任一大数
	fPtList ptList;
	fPtList minRectPtList;
	for (i = 0; i < n; i++)
	{
		tmucs.axis_x = vertex_arr[(i + 1) % n]->pos - vertex_arr[i]->pos;
		if (tmucs.axis_x.IsZero())
			continue;	//接近重点
		tmucs.origin = vertex_arr[i]->pos;
		tmucs.axis_x.z = tmucs.origin.z = 0;
		tmucs.axis_y = tmucs.axis_z^tmucs.axis_x;
		normalize(tmucs.axis_x);
		normalize(tmucs.axis_y);
		if (vertex_arr[i]->m_bRollEdge)
			tmucs.origin -= tmucs.axis_y*vertex_arr[i]->manu_space;
		scope.ClearScope();
		ptList.Empty();
		for (int j = 0; j < n; j++)
		{
			f3dPoint vertice = vertex_arr[j]->pos;
			coord_trans(vertice, tmucs, FALSE);
			scope.VerifyVertex(vertice);
			ptList.append(vertice);
			if (vertex_arr[j]->ciEdgeType > 1)
			{
				f3dPoint ptS = vertex_arr[j]->pos;
				f3dPoint ptE = vertex_arr[(j + 1) % n]->pos;
				f3dArcLine arcLine;
				if (vertex_arr[j]->ciEdgeType == 2)	//指定圆弧夹角
					arcLine.CreateMethod2(ptS, ptE, vertex_arr[j]->arc.work_norm, vertex_arr[j]->arc.fSectAngle);
				else if (vertex_arr[j]->ciEdgeType == 3)
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
		if(minarea <= scope.wide()*scope.high())
			continue;
		if (dfMaxRectW == 0 || (dfMaxRectW > 0 && scope.high() < dfMaxRectW))
		{	//用户指定了打印排版图框的宽度时，需保证钢板的最小矩形区域的高度小于指定宽度 wxc-20.11.16
			minarea = scope.wide()*scope.high();
			min_scope = scope;
			minUcs = tmucs;
			datumStartVertex.index = i;
			datumEndVertex.index = (i + 1) % n;
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

	rect.topLeft.Set(min_scope.fMinX, min_scope.fMaxY);
	rect.bottomRight.Set(min_scope.fMaxX, min_scope.fMinY);
	datumStartVertex.srcPos = vertex_arr[datumStartVertex.index]->pos;
	datumEndVertex.srcPos = vertex_arr[datumEndVertex.index]->pos;
	GEPOINT topLeft(rect.topLeft.x, rect.topLeft.y, 0);
	GEPOINT startPt = datumStartVertex.srcPos, endPt = datumEndVertex.srcPos;
	coord_trans(startPt, minUcs, FALSE);
	coord_trans(endPt, minUcs, FALSE);
	datumStartVertex.offsetPos = startPt - topLeft;
	datumEndVertex.offsetPos = endPt - topLeft;
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
	for (int i = 0; i < vertexList.GetNodeNum(); i++)
	{
		VERTEX *pCurVertex = vertexList.GetByIndex(i);
		if (pCurVertex->tag.lParam == 0 || pCurVertex->tag.lParam == -1)
			continue;
		ULONG *pCloneEntId = m_hashColneEntIdBySrcId.GetValue(pCurVertex->tag.dwParam);
		if (pCloneEntId == NULL)
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
	AcDbObjectId srcFirstId = 0, srcSecondId = 0;
	for (ULONG *pId = m_hashColneEntIdBySrcId.GetFirst(); pId; pId = m_hashColneEntIdBySrcId.GetNext())
	{
		long idSrcEnt = m_hashColneEntIdBySrcId.GetCursorKey();
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
		Acad::ErrorStatus es = XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
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
	GEPOINT mk_vec(1, 0, 0);
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
void CPlateProcessInfo::RefreshPlateNum(int nNewNum)
{
	if (partNumId == NULL)
		return;
	CLockDocumentLife lockCurDocLife;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, partNumId, AcDb::kForWrite);
	CAcDbObjLife entLife(pEnt);
	CString sSrcText, sDesText;
	sSrcText = GetCadTextContent(pEnt), sDesText;
	if (sSrcText.GetLength() <= 0)
		return;
	sDesText = sSrcText;
	//获取件数的若干个关键码
	CXhChar50 sNumKey = g_pncSysPara.m_sPnNumKey, sNumSubStr;
	std::vector<CXhChar16> numKeyArr;
	for (char* sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
		numKeyArr.push_back(CXhChar16(sKey));
	//解析字符串，获取件数所在位置
	if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar200 sContents(sSrcText);
		for (char* sSubStr = strtok(sContents, "\\P"); sSubStr; sSubStr = strtok(NULL, "\\P"))
		{
			if (strstr(sSubStr, numKeyArr[0]) && strstr(sSubStr, CXhChar16("%d", xBomPlate.nSumPart)))
			{
				sNumSubStr.Copy(sSubStr);
				break;
			}
		}
	}
	else
	{
		if (strstr(g_pncSysPara.m_sPnNumKey, "件") ||
			strstr(g_pncSysPara.m_sPnNumKey, "块"))
		{	//数量标识前或后带有空格
			for (size_t i = 0; i < numKeyArr.size(); i++)
			{
				if (strstr(sSrcText, numKeyArr[i]) == NULL)
					continue;
				int iPos = sSrcText.Find(numKeyArr[i]);
				if (iPos > 1 && sSrcText[iPos - 1] == ' ')
					sSrcText.Delete(iPos - 1);
			}
			sDesText = sSrcText;
		}
		//
		CXhChar100 sContents(sSrcText);
		sContents.Replace("　", " ");
		if (g_pncSysPara.m_iDimStyle == 0 || strstr(sContents, g_pncSysPara.m_sPnKey))
			sContents.Replace(g_pncSysPara.m_sPnKey, "| ");
		for (char* sSubStr = strtok(sContents, " \t"); sSubStr; sSubStr = strtok(NULL, " \t"))
		{
			for (size_t i = 0; i < numKeyArr.size(); i++)
			{
				if (strstr(sSubStr, numKeyArr[i]) && strstr(sSubStr, CXhChar16("%d", xBomPlate.nSumPart)))
				{
					sNumSubStr.Copy(sSubStr);
					break;
				}
			}
		}
	}
	if (sNumSubStr.GetLength() <= 0)
	{
		logerr.Log("钢板(%s),更新加工数失败!", (char*)GetPartNo());
		return;
	}
	//更新加工数
	CString sNewSubStr = sNumSubStr;
	sNewSubStr.Replace(CXhChar16("%d", xBomPlate.nSumPart), CXhChar16("%d", nNewNum));
	sDesText.Replace(sNumSubStr, sNewSubStr);
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sDesText.GetBuffer()));
#else
		pText->setTextString(sDesText.GetBuffer());
#endif
		//修改加工数后设置为红色 wht 20-07-29
		int color_index = GetNearestACI(RGB(255, 0, 0));
		pText->setColorIndex(color_index);
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		pMText->setContents(_bstr_t(sDesText.GetBuffer()));
#else
		pMText->setContents(sDesText.GetBuffer());
#endif
		//修改加工数后设置为红色 wht 20-07-29
		int color_index = GetNearestACI(RGB(255, 0, 0));
		pMText->setColorIndex(color_index);
	}
	//
	xBomPlate.nSumPart = nNewNum;
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
		else if (g_pncSysPara.m_iDimStyle == 0 &&
			strstr(sValueG, g_pncSysPara.m_sPnKey) != NULL &&
			strstr(sValueG, g_pncSysPara.m_sMatKey) != NULL &&
			strstr(g_pncSysPara.m_sPnKey, "#") != NULL)
		{
			CString sValurLeft, sValueRight;
			CString sValueStr(sValueG);
			sValurLeft = sValueStr.Left(sValueStr.Find(g_pncSysPara.m_sThickKey));
			int nSpecKeyIndex = sValueStr.Find(g_pncSysPara.m_sThickKey);//规格字符的索引（规格字符前的长度）
			//计算规格结束后的下一个字符的索引（规格最后一位前的长度）
			for (int nSpecEndIndex = nSpecKeyIndex + 1; nSpecEndIndex < sValueStr.GetLength(); nSpecEndIndex++)
			{
				char cSpecChar = sValueStr.GetAt(nSpecEndIndex);
				if (cSpecChar <'0' || cSpecChar>'9')
				{
					nSpecEndIndex++;
					break;
				}
			}
			if (nSpecEndIndex > 0 && nSpecEndIndex < sValueStr.GetLength() &&
				nSpecEndIndex - nSpecKeyIndex >= 2)
			{
				int nRightLenth = sValueStr.GetLength() - nSpecEndIndex;
				sValueRight = sValueStr.Right(nRightLenth);
			}
			sValueS.Printf("%s -%.0f %s", sValurLeft.GetBuffer(), xBomPlate.thick, sValueRight.GetBuffer());
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
			sValueS.Printf("-%.0f %s %d件", xPlate.m_fThick, (char*)sValueM, xBomPlate.nSumPart);
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
		else if (g_pncSysPara.m_iDimStyle == 0 &&
			strstr(sValueG, g_pncSysPara.m_sPnKey) != NULL &&
			strstr(sValueG, g_pncSysPara.m_sMatKey) != NULL &&
			strstr(g_pncSysPara.m_sPnKey, "#") != NULL)
		{
			CString sValurLeft, sValueRight;
			CString sValueStr(sValueG);
			sValurLeft = sValueStr.Left(sValueStr.Find(g_pncSysPara.m_sMatKey));
			int nSpecKeyIndex = sValueStr.Find(g_pncSysPara.m_sThickKey);
			if (nSpecKeyIndex > 0)
			{
				int nRightLenth = sValueStr.GetLength() - nSpecKeyIndex;
				sValueRight = sValueStr.Right(nRightLenth);
			}
			sValueS.Printf("%s%s %s", sValurLeft.GetBuffer(), (char*)xBomPlate.sMaterial, sValueRight.GetBuffer());
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
				sValueS.Printf("%s", xBomPlate.sMaterial);
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
//补充钢板加工数
void CPlateProcessInfo::FillPlateNum(int nNewNum, AcDbObjectId partNoId)
{
	if (g_pncSysPara.m_iDimStyle != 0 || partNoId == NULL)
		return;
	CLockDocumentLife lockCurDocLife;
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, partNoId, AcDb::kForWrite);
	if (pEnt == NULL)
		return;
	CAcDbObjLife entLife(pEnt);
	CString sContent = GetCadTextContent(pEnt), sNum;
	if (sContent.GetLength() <= 0)
		return;
	sNum.Format(" (%d件)", nNewNum);
	sContent += sNum;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sContent.GetBuffer()));
#else
		pText->setTextString(sContent.GetBuffer());
#endif
		int color_index = GetNearestACI(RGB(228, 0, 127));
		pText->setColorIndex(color_index);
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		pMText->setContents(_bstr_t(sContent.GetBuffer()));
#else
		pMText->setContents(sContent.GetBuffer());
#endif
		int color_index = GetNearestACI(RGB(228, 0, 127));
		pMText->setColorIndex(color_index);
	}
	if (this->partNoId == partNoId)
	{
		xBomPlate.nSumPart = nNewNum;
		m_ciModifyState |= MODIFY_MANU_NUM;
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
	else if (GetRelaEntCount() > 1)
	{
		for (CAD_ENTITY *pEnt = EnumFirstRelaEnt(); pEnt; pEnt = EnumNextRelaEnt())
			VerifyVertexByCADEntId(scope, MkCadObjId(pEnt->idCadEnt));
	}
	else
		scope = GetPlateScope(TRUE);
	return scope;
}