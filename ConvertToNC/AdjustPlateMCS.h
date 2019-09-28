#pragma once
#include "PNCModel.h"

class CAdjustPlateMCS
{
	CPlateProcessInfo *m_pPlateInfo;
	ARRAY_LIST<AcDbObjectId> m_xEntIdList;
	f2dPoint m_origin;
	f2dRect m_curRect;
	bool IsValidVertex(BYTE ciEdgeIndex);
	bool Rotation();
public:
	CAdjustPlateMCS(CPlateProcessInfo *pPlate);
	~CAdjustPlateMCS(void);

	void AnticlockwiseRotation();
	void ClockwiseRotation();
	void Mirror();
};

