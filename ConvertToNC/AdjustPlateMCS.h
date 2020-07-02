#pragma once
#include "PNCModel.h"

class CAdjustPlateMCS
{
	CPlateProcessInfo *m_pPlateInfo;
	ARRAY_LIST<AcDbObjectId> m_xEntIdList;
	f2dPoint m_origin;
protected:
	bool IsValidDockVertex(BYTE ciEdgeIndex);
	bool IsConcavePt(BYTE ciEdgeIndex);
	void MoveCloneEnts(AcGeMatrix3d moveMat);
	void UpdateCloneEdgePos();
	bool Rotation();
public:
	CAdjustPlateMCS(CPlateProcessInfo *pPlate);
	~CAdjustPlateMCS(void);
	//
	void AnticlockwiseRotation();
	void ClockwiseRotation();
	void Mirror();
};

