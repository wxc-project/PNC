#pragma once
#include "PNCModel.h"

#ifndef __UBOM_ONLY_
class CDrawDamBoard
{
	ARRAY_LIST<AcDbObjectId> m_xEntIdList;
	AcDbObjectId m_rectId;	//��ӡ���ο��Ӧ��ʵ��Id
public:
	static int BOARD_THICK;		//=10;
	static int BOARD_HEIGHT;	//=500;
	static BOOL m_bDrawAllBamBoard;
public:
	CDrawDamBoard();
	~CDrawDamBoard(void);
	bool IsValidDamBoard();
	bool IsValidSteelSealRect();
	void DrawDamBoard(CPlateProcessInfo *pPlate);
	void EraseDamBoard();
	AcDbObjectId GetSteelSealRectId() { return m_rectId; }
	void DrawSteelSealRect(CPlateProcessInfo *pPlate);
	void EraseSteelSealRect();
};

class CDamBoardManager
{
	CHashList<CDrawDamBoard> m_hashBoardList;
public:
	CDamBoardManager(){;}
	~CDamBoardManager(){;}
	CDrawDamBoard* DrawDamBoard(CPlateProcessInfo *pPlate);
	BOOL EraseDamBoard(CPlateProcessInfo *pPlate);
	AcDbObjectId GetSteelSealRectId(CPlateProcessInfo *pPlate);
	CDrawDamBoard* DrawSteelSealRect(CPlateProcessInfo *pPlate);
	BOOL EraseSteelSealRect(CPlateProcessInfo *pPlate);
	void DrawAllDamBoard(CPNCModel *pModel);
	void EraseAllDamBoard();
	void DrawAllSteelSealRect(CPNCModel *pModel);
	void EraseAllSteelSealRect();
};
#endif
