#pragma once
#include <acdocman.h>
class CPNCDocReactor : public AcApDocManagerReactor
{
public:
	CPNCDocReactor();
	~CPNCDocReactor();
	//
	virtual void documentActivated(AcApDocument* pActivatedDoc);
	virtual void documentDestroyed(const char* fileName);
};

extern CPNCDocReactor *g_pPNCDocReactor;
