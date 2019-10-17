#include "StdAfx.h"
#include "AdjustPlateMCS.h"
#include "CadToolFunc.h"

CAdjustPlateMCS::CAdjustPlateMCS(CPlateProcessInfo *pPlate)
{
	m_pPlateInfo=pPlate;
	m_xEntIdList.Empty();
	for(unsigned long *pId=pPlate->m_cloneEntIdList.GetFirst();pId;pId=pPlate->m_cloneEntIdList.GetNext())
		m_xEntIdList.append(MkCadObjId(*pId));
	m_curRect=GetCadEntRect(m_xEntIdList);
	m_origin.Set(m_curRect.topLeft.x,m_curRect.bottomRight.y);
}

CAdjustPlateMCS::~CAdjustPlateMCS(void)
{

}



bool CAdjustPlateMCS::Rotation()
{
	int n = m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	int cur_edge = m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge;
	int next_edge = (cur_edge + 1) % n;
	int i = 2;
	while (IsConcavePt(next_edge) && i<n)
	{
		next_edge = cur_edge + i;
		next_edge = next_edge % n;
		i++;
	}
	ACAD_LINEID *pLineS = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetValue(cur_edge + 1);
	ACAD_LINEID *pLineE = m_pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetValue(next_edge + 1);
	if (pLineS == NULL || pLineE == NULL)
		return false;
	CLockDocumentLife lockLife;
	//1.将特定边旋转至水平
	AcGeMatrix3d rotationMat;
	f3dPoint ptS = pLineS->m_ptStart;
	f3dPoint ptE = pLineE->m_ptStart;
	GEPOINT src_vec= ptE - ptS;
	if(m_pPlateInfo->xPlate.mcsFlg.ciOverturn>0)
		src_vec= ptS - ptE;
	GEPOINT dest_vec(1,0,0);
	double fDegAngle=Cal2dLineAng(0,0,dest_vec.x,dest_vec.y)-Cal2dLineAng(0,0,src_vec.x,src_vec.y);
	rotationMat.setToRotation(fDegAngle,AcGeVector3d::kZAxis,AcGePoint3d(ptS.x, ptS.y,0));
	AcDbEntity *pEnt=NULL;
	ARRAY_LIST<AcDbObjectId> entIdList;
	for(ULONG *pId=m_pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=m_pPlateInfo->m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		Acad::ErrorStatus es=acdbOpenAcDbEntity(pEnt,entId,AcDb::kForWrite);
		if(es!=Acad::eOk)
			AfxMessageBox(CXhChar50("%d",es));
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		pEnt->transformBy(rotationMat);
		entIdList.append(entId);
	}
	//2.将钢板移动至坐标系原点
	f2dRect rect=GetCadEntRect(entIdList);
	AcGeMatrix3d moveMat;
	ads_point ptFrom,ptTo;
	ptFrom[X]=rect.topLeft.x;
	ptFrom[Y]=rect.bottomRight.y;
	ptFrom[Z]=0;
	ptTo[X]=m_origin.x;
	ptTo[Y]=m_origin.y;
	ptTo[Z]=0;
	moveMat.setToTranslation(AcGeVector3d(ptTo[X]-ptFrom[X],ptTo[Y]-ptFrom[Y],ptTo[Z]-ptFrom[Z]));
	for(ULONG *pId=m_pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=m_pPlateInfo->m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForWrite);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		pEnt->transformBy(moveMat);
	}
	//3. 旋转钢印号位置
	pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_pPlateInfo->m_xMkDimPoint.idCadEnt), AcDb::kForWrite);
	if (pEnt)
	{
		CAcDbObjLife entLife(pEnt);
		pEnt->transformBy(rotationMat);
		pEnt->transformBy(moveMat);
	}
	//4.刷新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
	//5.更新钢板对应实体位置
	m_pPlateInfo->UpdateEdgeEntPos();
	return true;
}

void CAdjustPlateMCS::AnticlockwiseRotation()
{
	int n=m_pPlateInfo->xPlate.vertex_list.GetNodeNum();
	//圆弧边不能做为基准边，旋转时跳过圆弧边 wht 19-03-07
	for(int i=0;i<n;i++)
	{
		BYTE ciBottomEdge = m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge;
		if (m_pPlateInfo->xPlate.mcsFlg.ciOverturn > 0)
			m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge = (m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge + 1) % n;
		else
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
		if (m_pPlateInfo->xPlate.mcsFlg.ciOverturn > 0)
			m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge = (m_pPlateInfo->xPlate.mcsFlg.ciBottomEdge - 1 + n) % n;
		else
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
	BOOL bOverturn=m_pPlateInfo->xPlate.mcsFlg.ciOverturn;
	bOverturn=!bOverturn;
	m_pPlateInfo->xPlate.mcsFlg.ciOverturn=bOverturn;
	CLockDocumentLife lockLife;
	//1.将特定边旋转至水平
	AcGeMatrix3d mirrorMat;
	AcGeLine3d line3d;
	AcGePoint3d start(m_origin.x,m_origin.y,0),end(m_origin.x,m_origin.y+1000,0);
	line3d.set(start,end);
	mirrorMat.setToMirroring(line3d);
	AcDbEntity *pEnt=NULL;
	ARRAY_LIST<AcDbObjectId> entIdList;
	for(ULONG *pId=m_pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=m_pPlateInfo->m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		Acad::ErrorStatus es=acdbOpenAcDbEntity(pEnt,entId,AcDb::kForWrite);
		if(es!=Acad::eOk)
			AfxMessageBox(CXhChar50("%d",es));
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		pEnt->transformBy(mirrorMat);
		entIdList.append(entId);
	}
	//2.将钢板移动至坐标系原点
	f2dRect rect=GetCadEntRect(entIdList);
	AcGeMatrix3d moveMat;
	ads_point ptFrom,ptTo;
	ptFrom[X]=rect.topLeft.x;
	ptFrom[Y]=rect.bottomRight.y;
	ptFrom[Z]=0;
	ptTo[X]=m_origin.x;
	ptTo[Y]=m_origin.y;
	ptTo[Z]=0;
	moveMat.setToTranslation(AcGeVector3d(ptTo[X]-ptFrom[X],ptTo[Y]-ptFrom[Y],ptTo[Z]-ptFrom[Z]));
	for(ULONG *pId=m_pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=m_pPlateInfo->m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForWrite);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		pEnt->transformBy(moveMat);
	}
	//3.刷新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
	//4.更新钢板对应实体位置
	m_pPlateInfo->UpdateEdgeEntPos();
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