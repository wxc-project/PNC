#include "StdAfx.h"
#include "AdjustPlateMCS.h"
#include "CadToolFunc.h"
#include "PNCCryptCoreCode.h"

CAdjustPlateMCS::CAdjustPlateMCS(CPlateProcessInfo *pPlate)
{
	m_pPlateInfo=pPlate;
	SCOPE_STRU scope;
	if (pPlate)
		scope = pPlate->GetCADEntScope(TRUE);
	m_origin.Set(scope.fMinX, scope.fMinY);
}

CAdjustPlateMCS::~CAdjustPlateMCS(void)
{

}

void CAdjustPlateMCS::UpdateCloneEdgePos()
{
	for (CAD_LINE *pLineId = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetFirst(); pLineId;
		pLineId = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetNext())
		pLineId->UpdatePos();
}

bool CAdjustPlateMCS::Rotation()
{
	UpdateCloneEdgePos();
	//
	int n = m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	int cur_edge = m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge;
	int next_edge = (cur_edge + 1) % n;
	int i = 2;
	while (IsConcavePt(next_edge) && i < n)
	{
		next_edge = cur_edge + i;
		next_edge = next_edge % n;
		i++;
	}
	CAD_LINE *pLineS = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetValue(cur_edge + 1);
	CAD_LINE *pLineE = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetValue(next_edge + 1);
	if (pLineS == NULL || pLineE == NULL)
		return false;
	CLockDocumentLife lockLife;
	//1.将特定边旋转至水平
	AcGeMatrix3d rotationMat;
	f3dPoint ptS = pLineS->m_ptStart;
	f3dPoint ptE = pLineE->m_ptStart;
	GEPOINT src_vec = (ptE - ptS).normalized();
	GEPOINT dest_vec(1, 0, 0);
	double fDegAngle = Cal2dLineAng(0, 0, dest_vec.x, dest_vec.y) - Cal2dLineAng(0, 0, src_vec.x, src_vec.y);
	rotationMat.setToRotation(fDegAngle, AcGeVector3d::kZAxis, AcGePoint3d(ptS.x, ptS.y, 0));
	m_pPlateInfo->MoveEnts(rotationMat);
	//2.将钢板移动至坐标系原点
	SCOPE_STRU scope = m_pPlateInfo->GetCADEntScope(TRUE);
	AcGeMatrix3d moveMat;
	ads_point ptFrom, ptTo;
	ptFrom[X] = scope.fMinX;
	ptFrom[Y] = scope.fMinY;
	ptFrom[Z] = 0;
	ptTo[X] = m_origin.x;
	ptTo[Y] = m_origin.y;
	ptTo[Z] = 0;
	moveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
	m_pPlateInfo->MoveEnts(moveMat);
	//3. 旋转钢印号位置
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, MkCadObjId(m_pPlateInfo->m_xMkDimPoint.idCadEnt), AcDb::kForWrite);
	if (pEnt)
	{
		pEnt->transformBy(rotationMat);
		pEnt->transformBy(moveMat);
		pEnt->close();
	}
	//4.更新钢板对应实体位置
	UpdateCloneEdgePos();
	return true;
}

void CAdjustPlateMCS::AnticlockwiseRotation()
{
	int n=m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	//圆弧边不能做为基准边，旋转时跳过圆弧边 wht 19-03-07
	for(int i=0;i<n;i++)
	{
		BYTE ciBottomEdge = m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge;
		m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge = (m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge - 1 + n) % n;
		if (IsValidDockVertex(m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge))
		{
			if(Rotation())
				break;
		}
	}
}

void CAdjustPlateMCS::ClockwiseRotation()
{
	int n=m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	//圆弧边不能做为基准边，旋转时跳过圆弧边 wht 19-03-07
	for (int i = 0; i < n; i++)
	{
		BYTE ciBottomEdge = m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge;
		m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge = (m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge + 1) % n;
		if (IsValidDockVertex(m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge))
		{
			if(Rotation())
				break;
		}
	}
}

void CAdjustPlateMCS::Mirror()
{
	BOOL bOverturn = m_pPlateInfo->xPlate.mcsFlg.ciOverturn;
	bOverturn = !bOverturn;
	m_pPlateInfo->xPlate.mcsFlg.ciOverturn = bOverturn;
	CLockDocumentLife lockLife;
	//1.将特定边旋转至水平
	AcGeMatrix3d mirrorMat;
	AcGeLine3d line3d;
	AcGePoint3d start(m_origin.x, m_origin.y, 0), end(m_origin.x, m_origin.y + 1000, 0);
	line3d.set(start, end);
	mirrorMat.setToMirroring(line3d);
	m_pPlateInfo->MoveEnts(mirrorMat);
	//2.将钢板移动至坐标系原点
	SCOPE_STRU scope = m_pPlateInfo->GetCADEntScope(TRUE);
	AcGeMatrix3d moveMat;
	ads_point ptFrom, ptTo;
	ptFrom[X] = scope.fMinX;
	ptFrom[Y] = scope.fMinY;
	ptFrom[Z] = 0;
	ptTo[X] = m_origin.x;
	ptTo[Y] = m_origin.y;
	ptTo[Z] = 0;
	moveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
	m_pPlateInfo->MoveEnts(moveMat);
	//3.更新钢板对应实体位置
	UpdateCloneEdgePos();
}

bool CAdjustPlateMCS::IsValidDockVertex(BYTE ciEdgeIndex)
{
	int i = 0, n = m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	PROFILE_VER *pCurVertex = NULL, *pPreVertex = m_pPlateInfo->xPlate.vertex_list.GetTail();
	for (PROFILE_VER *pVertex = m_pPlateInfo->xPlate.vertex_list.GetFirst(); pVertex;
		pVertex = m_pPlateInfo->xPlate.vertex_list.GetNext(), i++)
	{
		vertex_arr[i] = pVertex->vertex;
		if (i == ciEdgeIndex)
			pCurVertex = pVertex;
		if (i < ciEdgeIndex)
			pPreVertex = pVertex;
	}
	//if (pPreVertex->type != 1 || pCurVertex->type != 1)
	if (pCurVertex == NULL || pCurVertex->type != 1)
		return false;
	else
	{
		if (IsConcavePt(ciEdgeIndex))
			return false;
		else
			return true;
	}
}

bool CAdjustPlateMCS::IsConcavePt(BYTE ciEdgeIndex)
{
	int i = 0, n = m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	PROFILE_VER *pCurVertex = NULL, *pPreVertex = m_pPlateInfo->xPlate.vertex_list.GetTail();
	for (PROFILE_VER *pVertex = m_pPlateInfo->xPlate.vertex_list.GetFirst(); pVertex;
		pVertex = m_pPlateInfo->xPlate.vertex_list.GetNext(), i++)
	{
		vertex_arr[i] = pVertex->vertex;
	}
	f3dPoint ptPre = vertex_arr[(ciEdgeIndex - 1 + n) % n];
	f3dPoint ptCur = vertex_arr[ciEdgeIndex];
	f3dPoint ptNex = vertex_arr[(ciEdgeIndex + 1) % n];
	double result = DistOf2dPtLine(ptNex, ptPre, ptCur);
	if (result <= 0)	// 后点在线右侧，有凹角出现,或者三点共线
		return true;
	else
		return false;
}