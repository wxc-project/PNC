#include "stdafx.h"
#include "PNCModel.h"
#include "XhMath.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"

typedef CPlateProcessInfo* CPlateInfoPtr;
int ComparePlatePtrByPartNo(const CPlateInfoPtr &plate1, const CPlateInfoPtr &plate2)
{
	CXhChar16 sPartNo1 = plate1->xPlate.GetPartNo();
	CXhChar16 sPartNo2 = plate2->xPlate.GetPartNo();
	return ComparePartNoString(sPartNo1, sPartNo2, "SHGPT");
}
//////////////////////////////////////////////////////////////////////////
// CSortedModel
CSortedModel::CSortedModel(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	platePtrList.Empty();
	for (CPlateProcessInfo *pPlate = pModel->EnumFirstPlate(); pPlate; pPlate = pModel->EnumNextPlate())
		platePtrList.append(pPlate);
	CHeapSort<CPlateInfoPtr>::HeapSort(platePtrList.m_pData, platePtrList.Size(), ComparePlatePtrByPartNo);
}
CPlateProcessInfo *CSortedModel::EnumFirstPlate()
{
	CPlateProcessInfo **ppPlate = platePtrList.GetFirst();
	if (ppPlate)
		return *ppPlate;
	else
		return NULL;
}
CPlateProcessInfo *CSortedModel::EnumNextPlate()
{
	CPlateProcessInfo **ppPlate = platePtrList.GetNext();
	if (ppPlate)
		return *ppPlate;
	else
		return NULL;
}
void CSortedModel::DividPlatesBySeg()
{
	hashPlateGroup.Empty();
	SEGI segI, temSegI;
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		CXhChar16 sPartNo = pPlate->GetPartNo();
		ParsePartNo(sPartNo, &segI, NULL, "SHPGT");
		CXhChar16 sSegStr = segI.ToString();
		if (sSegStr.GetLength() > 3)
		{
			if (ParsePartNo(sSegStr, &temSegI, NULL, "SHPGT"))
				segI = temSegI;
		}
		PARTGROUP* pPlateGroup = hashPlateGroup.GetValue(segI.ToString());
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateGroup.Add(segI.ToString());
			pPlateGroup->sKey.Copy(segI.ToString());
		}
		pPlateGroup->sameGroupPlateList.append(pPlate);
	}
}

void CSortedModel::DividPlatesByThickMat()
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		int thick = ftoi(pPlate->xPlate.GetThick());
		CXhChar50 sKey("%C_%d", pPlate->xPlate.cMaterial, thick);
		PARTGROUP* pPlateGroup = hashPlateGroup.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateGroup.Add(sKey);
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->sameGroupPlateList.append(pPlate);
	}
}

void CSortedModel::DividPlatesByThick()
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		int thick = ftoi(pPlate->xPlate.GetThick());
		CXhChar50 sKey("%d", thick);
		PARTGROUP* pPlateGroup = hashPlateGroup.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateGroup.Add(sKey);
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->sameGroupPlateList.append(pPlate);
	}
}

void CSortedModel::DividPlatesByMat()
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		CXhChar16 sKey = CProcessPart::QuerySteelMatMark(pPlate->xPlate.cMaterial);
		PARTGROUP* pPlateGroup = hashPlateGroup.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateGroup.Add(sKey);
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->sameGroupPlateList.append(pPlate);
	}
}

void CSortedModel::DividPlatesByPartNo()
{
	PARTGROUP* pPlateGroup = hashPlateGroup.Add("ALL");
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
		pPlateGroup->sameGroupPlateList.append(pPlate);
}

double CSortedModel::PARTGROUP::GetMaxHight()
{
	CMaxDouble maxHeight;
	for (CPlateProcessInfo* pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		SCOPE_STRU scope = pPlate->GetCADEntScope();
		maxHeight.Update(scope.high());
	}
	return maxHeight.number;
}
double CSortedModel::PARTGROUP::GetMaxWidth()
{
	CMaxDouble maxHeight;
	for (CPlateProcessInfo* pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		SCOPE_STRU scope = pPlate->GetCADEntScope();
		maxHeight.Update(scope.wide());
	}
	return maxHeight.number;
}
CPlateProcessInfo* CSortedModel::PARTGROUP::EnumFirstPlate()
{
	CPlateProcessInfo** ppPlate = sameGroupPlateList.GetFirst();
	if (ppPlate)
		return *ppPlate;
	else
		return NULL;
}
CPlateProcessInfo* CSortedModel::PARTGROUP::EnumNextPlate()
{
	CPlateProcessInfo** ppPlate = sameGroupPlateList.GetNext();
	if (ppPlate)
		return *ppPlate;
	else
		return NULL;
}