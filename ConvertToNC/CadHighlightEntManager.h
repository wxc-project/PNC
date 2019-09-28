#pragma once
#include "ArrayList.h"
#include "HashTable.h"

class CCadHighlightEntManager
{
public:
	static CHashList<AcDbObjectId>hashHighlightEnts;
	static void ReleaseHighlightEnts();
	static void SetEntSetHighlight(ARRAY_LIST<AcDbObjectId> &entIdList);
};

