#include "StdAfx.h"
#include "DrawDamBoard.h"
#include "CadToolFunc.h"
#include "PNCSysPara.h"
#include "DragEntSet.h"

#ifndef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
// CDrawDamBoard
int CDrawDamBoard::BOARD_THICK =10;
int CDrawDamBoard::BOARD_HEIGHT=500;
BOOL CDrawDamBoard::m_bDrawAllBamBoard=TRUE;
CDrawDamBoard::CDrawDamBoard()
{
	m_pPlate=NULL;
}
bool CDrawDamBoard::IsValidDamBoard()
{
	return (m_xEntIdList.Size()>0);
}
bool CDrawDamBoard::IsValidSteelSealRect()
{
	return m_rectId.isValid();
}
void CDrawDamBoard::DrawDamBoard(CPlateProcessInfo *pPlate)
{
	m_pPlate=pPlate;
	if(m_pPlate==NULL)
		return;
	ARRAY_LIST<AcDbObjectId> entIdList;
	for(unsigned long *pId=pPlate->m_cloneEntIdList.GetFirst();pId;pId=pPlate->m_cloneEntIdList.GetNext())
		entIdList.append(MkCadObjId(*pId));
	f2dRect rect=GetCadEntRect(entIdList);
	f2dPoint leftBtm(rect.topLeft.x,rect.bottomRight.y);
	const int ARROW_LEN=20;
	CLockDocumentLife lock;
	CBlockTableRecordLife rec(GetBlockTableRecord());
	fPtList vertexList;
	vertexList.append(leftBtm.x,leftBtm.y,0);
	vertexList.append(leftBtm.x,leftBtm.y+BOARD_HEIGHT,0);
	vertexList.append(leftBtm.x-BOARD_THICK,leftBtm.y+BOARD_HEIGHT,0);
	vertexList.append(leftBtm.x-BOARD_THICK,leftBtm.y,0);
	AcDbObjectId entId=CreateAcadHatch(rec.pBlockTblRec,vertexList,"ANSI31",0.3);
	m_xEntIdList.append(entId);
	entId=CreateAcadRect(rec.pBlockTblRec,f3dPoint(leftBtm.x-BOARD_THICK,leftBtm.y+BOARD_HEIGHT,0),BOARD_THICK,BOARD_HEIGHT);
	m_xEntIdList.append(entId);
	f3dPoint vertextArr[4];
	double width=max(rect.Width(),rect.Height());
	vertextArr[0].Set(leftBtm.x,leftBtm.y);
	vertextArr[1].Set(leftBtm.x+1.5*width,leftBtm.y);
	vertextArr[2].Set(leftBtm.x+1.5*width-ARROW_LEN,leftBtm.y+0.5*ARROW_LEN);
	vertextArr[3].Set(leftBtm.x+1.5*width-ARROW_LEN,leftBtm.y-0.5*ARROW_LEN);
	entId=CreateAcadLine(rec.pBlockTblRec,vertextArr[0],vertextArr[1]);
	m_xEntIdList.append(entId);
	entId=CreateAcadLine(rec.pBlockTblRec,vertextArr[1],vertextArr[2]);
	m_xEntIdList.append(entId);
	entId=CreateAcadLine(rec.pBlockTblRec,vertextArr[1],vertextArr[3]);
	m_xEntIdList.append(entId);
}

CDrawDamBoard::~CDrawDamBoard(void)
{
	
}

void CDrawDamBoard::EraseDamBoard()
{
	AcDbEntity *pEnt=NULL;
	for(AcDbObjectId *pEntId=m_xEntIdList.GetFirst();pEntId;pEntId=m_xEntIdList.GetNext())
	{
		pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,*pEntId,AcDb::kForWrite);
		if(pEnt==NULL)
			continue;
		pEnt->erase(Adesk::kTrue);
		pEnt->close();
	}
}

void CDrawDamBoard::DrawSteelSealRect(CPlateProcessInfo *pPlate)
{
	if(m_pPlate&&m_pPlate!=pPlate)
		return;
	m_pPlate=pPlate;
	if(m_pPlate==NULL)
		return;
	CLockDocumentLife lock;
	CBlockTableRecordLife rec(GetBlockTableRecord());
	double fHalfL=g_pncSysPara.m_nMkRectLen*0.5,fHalfW=g_pncSysPara.m_nMkRectWidth*0.5;
	AcDbEntity* pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, MkCadObjId(pPlate->m_xMkDimPoint.idCadEnt), AcDb::kForRead);
	if (pEnt == NULL||!pEnt->isKindOf(AcDbPoint::desc()))
		return;
	pEnt->close();
	AcDbObjectId outputId;
	AcDbPoint *pPoint=(AcDbPoint*)pEnt;
	GEPOINT pos(pPoint->position().x, pPoint->position().y, 0);
	pPlate->m_xMkDimPoint.pos=pos;
	DRAGSET.BeginBlockRecord();
	pos.Set();
	f3dPoint vertex_arr[5];
	vertex_arr[0].Set(pos.x - fHalfL, pos.y + fHalfW);
	vertex_arr[1].Set(pos.x + fHalfL, pos.y + fHalfW);
	vertex_arr[2].Set(pos.x + fHalfL, pos.y - fHalfW);
	vertex_arr[3].Set(pos.x - fHalfL, pos.y - fHalfW);
	vertex_arr[4].Set(pos.x - fHalfL, pos.y + fHalfW);
	m_rectId = CreateAcadPolyLine(DRAGSET.RecordingBlockTableRecord(), vertex_arr, 5, 1, pPlate->partNoId.asOldId());
	DRAGSET.EndBlockRecord(rec.pBlockTblRec, pPlate->m_xMkDimPoint.pos, 1, &outputId, pPlate->partNoId.asOldId());
	if (!outputId.isNull())
	{
		m_rectId = outputId;
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt, m_rectId, AcDb::kForWrite);
		CAcDbObjLife rectLife(pEnt);
		if (pEnt!=NULL&&pEnt->isKindOf(AcDbBlockReference::desc()))
			pEnt->addReactor(g_pSteelSealReactor);
	}
}
void CDrawDamBoard::EraseSteelSealRect()
{
	AcDbEntity *pEnt=NULL;
	acdbOpenAcDbEntity(pEnt,m_rectId,AcDb::kForWrite);
	if(pEnt)
	{
		pEnt->erase(Adesk::kTrue);
		pEnt->close();
	}
}

//////////////////////////////////////////////////////////////////////////
// CDamBoardManager
CDrawDamBoard* CDamBoardManager::DrawDamBoard(CPlateProcessInfo *pPlate)
{
	if(pPlate==NULL)
		return NULL;
	CDrawDamBoard *pBoard=m_hashBoardList.GetValue((DWORD)pPlate);
	if(pBoard==NULL)
		pBoard=m_hashBoardList.Add((DWORD)pPlate);
	else
		pBoard->EraseDamBoard();
	pBoard->DrawDamBoard(pPlate);
	return pBoard;
}
BOOL CDamBoardManager::EraseDamBoard(CPlateProcessInfo *pPlate)
{
	CDrawDamBoard *pBoard=m_hashBoardList.GetValue((DWORD)pPlate);
	if(pBoard)
	{
		pBoard->EraseDamBoard();
		return m_hashBoardList.DeleteNode((DWORD)pPlate);
	}
	else
		return FALSE;
}
AcDbObjectId CDamBoardManager::GetSteelSealRectId(CPlateProcessInfo *pPlate)
{
	if (pPlate == NULL)
		return NULL;
	CDrawDamBoard *pBoard = m_hashBoardList.GetValue((DWORD)pPlate);
	if (pBoard != NULL)
		return pBoard->GetSteelSealRectId();
	else
		return 0;
}
CDrawDamBoard* CDamBoardManager::DrawSteelSealRect(CPlateProcessInfo *pPlate)
{
	if(pPlate==NULL)
		return NULL;
	CDrawDamBoard *pBoard=m_hashBoardList.GetValue((DWORD)pPlate);
	if(pBoard==NULL)
		pBoard=m_hashBoardList.Add((DWORD)pPlate);
	else
		pBoard->EraseSteelSealRect();
	pBoard->DrawSteelSealRect(pPlate);
	return pBoard;
}
BOOL CDamBoardManager::EraseSteelSealRect(CPlateProcessInfo *pPlate)
{
	CDrawDamBoard *pBoard=m_hashBoardList.GetValue((DWORD)pPlate);
	if(pBoard)
	{
		pBoard->EraseSteelSealRect();
		if(!pBoard->IsValidDamBoard())
			return m_hashBoardList.DeleteNode((DWORD)pPlate);
		else
			return TRUE;
	}
	else
		return FALSE;
}
void CDamBoardManager::DrawAllDamBoard(CPNCModel *pModel)
{
	if(pModel==NULL)
		return;
	EraseAllDamBoard();
	for(CPlateProcessInfo *pPlate=pModel->EnumFirstPlate(FALSE);pPlate;pPlate=pModel->EnumNextPlate(FALSE))
		DrawDamBoard(pPlate);
}
void CDamBoardManager::EraseAllDamBoard()
{
	for(CDrawDamBoard *pBoard=m_hashBoardList.GetFirst();pBoard;pBoard=m_hashBoardList.GetNext())
		pBoard->EraseDamBoard();
	m_hashBoardList.Empty();
}

void CDamBoardManager::DrawAllSteelSealRect(CPNCModel *pModel)
{
	if(pModel==NULL)
		return;
	EraseAllSteelSealRect();
	for(CPlateProcessInfo *pPlate=pModel->EnumFirstPlate(FALSE);pPlate;pPlate=pModel->EnumNextPlate(FALSE))
		DrawSteelSealRect(pPlate);
}
void CDamBoardManager::EraseAllSteelSealRect()
{
	for(CDrawDamBoard *pBoard=m_hashBoardList.GetFirst();pBoard;pBoard=m_hashBoardList.GetNext())
	{
		pBoard->EraseSteelSealRect();
		if(!pBoard->IsValidDamBoard())
			m_hashBoardList.DeleteCursor(FALSE);
	}
	m_hashBoardList.Clean();
}
#endif
