#pragma once
#include "XeroNcPart.h"
#include "XeroExtractor.h"
#include "PNCDocReactor.h"
//////////////////////////////////////////////////////////////////////////
//CPNCModel
class CPNCModel
{
	CHashStrList<CPlateProcessInfo> m_hashPlateInfo;
public:
	PROJECT_INFO m_xPrjInfo;
	CString m_sCurWorkFile;		//当前正在操作的文件
public:
	CPNCModel(void);
	~CPNCModel(void);
	//
	void Empty();
	bool ExtractPlates(CString sDwgFile, BOOL bSupportSelectEnts=FALSE);
	void CreatePlatePPiFile(const char* work_path=NULL);
	//绘制钢板
	void DrawPlates();
	void DrawPlatesToLayout();
	void DrawPlatesToCompare();
	void DrawPlatesToProcess();
	void DrawPlatesToClone();
	void DrawPlatesToFiltrate();
	//
	int GetPlateNum(){return m_hashPlateInfo.GetNodeNum();}
	int PushPlateStack() { return m_hashPlateInfo.push_stack(); }
	bool PopPlateStack() { return m_hashPlateInfo.pop_stack(); }
	BOOL DeletePlate(const char* sPartNo) { return m_hashPlateInfo.DeleteNode(sPartNo); }
	CPlateProcessInfo* AppendPlate(char* sPartNo){return m_hashPlateInfo.Add(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(char* sPartNo){return m_hashPlateInfo.GetValue(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(AcDbObjectId partNoEntId);
	CPlateProcessInfo* EnumFirstPlate(){return m_hashPlateInfo.GetFirst();}
	CPlateProcessInfo* EnumNextPlate(){return m_hashPlateInfo.GetNext();}
	void WritePrjTowerInfoToCfgFile(const char* cfg_file_path);
};
//////////////////////////////////////////////////////////////////////////
//
class CSortedModel
{
	ARRAY_LIST<CPlateProcessInfo*> platePtrList;
public:
	struct PARTGROUP
	{
		ATOM_LIST<CPlateProcessInfo*> sameGroupPlateList;
		CXhChar50 sKey;
		//
		double GetMaxHight();
		double GetMaxWidth();
		CPlateProcessInfo *EnumFirstPlate();
		CPlateProcessInfo *EnumNextPlate();
	};
	CHashStrList<PARTGROUP> hashPlateGroup;
public:
	CSortedModel(CPNCModel *pModel);
	//
	CPlateProcessInfo *EnumFirstPlate();
	CPlateProcessInfo *EnumNextPlate();
	void DividPlatesBySeg();
	void DividPlatesByThickMat();
	void DividPlatesByThick();
	void DividPlatesByMat();
	void DividPlatesByPartNo();
};
//////////////////////////////////////////////////////////////////////////
extern CPNCModel model;
