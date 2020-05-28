#pragma once
#ifdef __ALFA_TEST_
#include "HashTable.h"
#include "ProcessPart.h"
#include "LogFile.h"

extern CLogFile MyLogFile;
//
struct PLATE_COMPARE
{
	TCHAR m_sPartNo[16];
	CProcessPlate* m_pSrcPlate;
	CProcessPlate* m_pDestPlate;
public:
	PLATE_COMPARE()
	{
		memset(m_sPartNo, 0, sizeof(TCHAR) * 16);
		m_pSrcPlate = NULL;
		m_pDestPlate = NULL;
	}
	//
	void RunCompare();
};
//
void TestPnc();
#endif