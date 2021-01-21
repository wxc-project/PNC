#pragma once
#include "PNCModel.h"

class CAdjustPlateMCS
{
	CPlateProcessInfo *m_pPlateInfo;
	f2dPoint m_origin;
protected:
	bool IsValidDockVertex(BYTE ciEdgeIndex);
	bool IsConcavePt(BYTE ciEdgeIndex);
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

