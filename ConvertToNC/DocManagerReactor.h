#pragma once
#include <acdocman.h>
class CDocManagerReactor : public AcApDocManagerReactor
{
public:
	CDocManagerReactor();
	~CDocManagerReactor();
	//
	virtual void documentActivated(AcApDocument* pActivatedDoc);
	virtual void documentDestroyed(const char* fileName);
};

