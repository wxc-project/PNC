#pragma once
#include <dbmain.h>
class CSteelSealReactor : public AcDbObjectReactor
{
public:
	CSteelSealReactor();
	~CSteelSealReactor();
	virtual void modified(const AcDbObject* dbObj);
};

