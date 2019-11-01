#pragma once
#include "PNCModel.h"

class CAdjustPlateMCS
{
	BOOL m_bDrawClonePlateBoard;
	CPlateProcessInfo *m_pPlateInfo;
	ARRAY_LIST<AcDbObjectId> m_xEntIdList;
	f2dPoint m_origin;
	f2dRect m_curRect;
	bool IsValidDockVertex(BYTE ciEdgeIndex);
	bool IsConcavePt(BYTE ciEdgeIndex);
	bool Rotation();
public:
	CAdjustPlateMCS(CPlateProcessInfo *pPlate, BOOL bDrawClonePlateBoard = TRUE);
	~CAdjustPlateMCS(void);

	void AnticlockwiseRotation();
	void ClockwiseRotation();
	void Mirror();
	f2dRect GetRect() { return m_curRect; }
};

