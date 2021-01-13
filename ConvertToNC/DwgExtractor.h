#pragma once
#include "PNCModel.h"

class CDwgExtractor : public IExtractor
{
public:
	CDwgExtractor();
	static double StandardHoleD(double fDiameter);
	CString GetCurDocFile();
	//
	virtual bool ExtractPlates(CHashStrList<CPlateProcessInfo>& hashPlateInfo, BOOL bSupportSelectEnts) {
		return false;
	}
#ifdef __UBOM_ONLY_
	virtual bool ExtractAngles(CHashList<CAngleProcessInfo>& hashJgInfo, BOOL bSupportSelectEnts) {
		return false;
	}
#endif
};
//
class CPlateExtractor : public CDwgExtractor
{
private:
	static CPlateExtractor* m_pExtractor;
	CHashSet<AcDbObjectId> m_xAllEntIdSet;
	CHashStrList<CPlateProcessInfo>* m_pHashPlate;
private:
	CPlateExtractor();
	//
	bool AppendBoltEntsByBlock(ULONG idBlockEnt);
	bool AppendBoltEntsByCircle(ULONG idCirEnt);
	bool AppendBoltEntsByPolyline(ULONG idPolyline);
	bool AppendBoltEntsByConnectLines(vector<CAD_LINE> vectorConnLine);
	bool AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine);
	void InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts, CHashSet<AcDbObjectId>& hashAllLines);
	//
	void ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols);
	void MergeManyPartNo();
	void SplitManyPartNo();
	//
	CPlateProcessInfo* EnumFirstPlate() {
		return m_pHashPlate ? m_pHashPlate->GetFirst() : NULL;
	}
	CPlateProcessInfo* EnumNextPlate() {
		return m_pHashPlate ? m_pHashPlate->GetNext() : NULL;
	}
	CPlateProcessInfo* GetPlateInfo(GEPOINT text_pos) {
		CPlateProcessInfo* pPlateInfo = NULL;
		for (pPlateInfo = EnumFirstPlate(); pPlateInfo; pPlateInfo = EnumNextPlate())
		{
			if (pPlateInfo->IsInPartRgn(text_pos))
				break;
		}
		return pPlateInfo;
	}
public:
	static CPlateExtractor* GetExtractor();
	virtual bool ExtractPlates(CHashStrList<CPlateProcessInfo>& hashPlateInfo, BOOL bSupportSelectEnts);
};
//////////////////////////////////////////////////////////////////////////
#ifdef __UBOM_ONLY_
class CAngleExtractor : public CDwgExtractor
{
private:
	static CAngleExtractor* m_pExtractor;
	CHashList<CAngleProcessInfo>* m_pHashAngle;
private:
	CAngleExtractor();
	//
	CAngleProcessInfo* EnumFirstAngle() {
		return m_pHashAngle ? m_pHashAngle->GetFirst() : NULL;
	}
	CAngleProcessInfo* EnumNextAngle() {
		return m_pHashAngle ? m_pHashAngle->GetNext() : NULL;
	}
	CAngleProcessInfo* GetAngleInfo(GEPOINT text_pos) {
		CAngleProcessInfo* pAngleInfo = NULL;
		for (pAngleInfo = EnumFirstAngle(); pAngleInfo; pAngleInfo = EnumNextAngle())
		{
			if (pAngleInfo->IsInPartRgn(text_pos))
				break;
		}
		return pAngleInfo;
	}
public:
	static CAngleExtractor* GetExtractor();
	virtual bool ExtractAngles(CHashList<CAngleProcessInfo>& hashJgInfo, BOOL bSupportSelectEnts);
};
#endif
