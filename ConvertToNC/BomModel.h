#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "ProcessPart.h"
#include "Variant.h"
#include "LogFile.h"
#include "FileIO.h"
#include "..\ConvertToNC\PNCModel.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
class CProjectTowerType;
class CBomFile
{
private:
	BOOL m_bLoftBom;
	CXhChar500 m_sFileName;		//文件名称
	CProjectTowerType* m_pProject;
	CSuperHashStrList<CProcessPart> m_hashPartByPartNo;
	//
	BOOL ImportTmaExcelFile();
	BOOL ImportErpExcelFile();
	BOOL ParseTmaSheetContent(CVariant2dArray &sheetContentMap);
	BOOL ParseTmaSheetContentCore(CVariant2dArray &sheetContentMap, CHashStrList<DWORD> &hashColIndexByColTitle, int iStartRow);
public:
	CBomFile();
	~CBomFile();
	//
	void Empty()
	{
		m_hashPartByPartNo.Empty();
		m_sFileName.Empty();
	}
	CProcessPart* EnumFirstPart(){return m_hashPartByPartNo.GetFirst();}
	CProcessPart* EnumNextPart(){return m_hashPartByPartNo.GetNext();}
	CProcessPart* FindPart(const char* sKey){return m_hashPartByPartNo.GetValue(sKey);}
	int GetPartNum(){return m_hashPartByPartNo.GetNodeNum();}
	CString GetPartNumStr();
	BOOL IsLoftBom(){return m_bLoftBom;}
	void SetBelongModel(CProjectTowerType *pProject){m_pProject=pProject;}
	CProjectTowerType* BelongModel() const{return m_pProject;}
	CXhChar500 GetFileName(){return m_sFileName;}
	void ImportBomFile(const char* sFileName,BOOL bLoftBom);
	void UpdateProcessPart(const char* sOldKey,const char* sNewKey);
};
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
class CAngleProcessInfo
{
private:
	f3dPoint orig_pt;
	POLYGON region;
	SCOPE_STRU scope;
public:
	CProcessAngle m_xAngle;
	AcDbObjectId keyId,partNumId,sumWeightId;
public:
	CAngleProcessInfo();
	~CAngleProcessInfo();
	//
	void CreateRgn();
	bool PtInAngleRgn(const double* poscoord);
	BYTE InitAngleInfo(f3dPoint data_pos,const char* sValue);
	void SetOrig(f3dPoint pt){orig_pt=pt;}
	f2dRect GetAngleDataRect(BYTE data_type);
	f3dPoint GetAngleDataPos(BYTE data_type);
	bool PtInDataRect(BYTE data_type,f3dPoint pt);
	void RefreshAngleNum();
	void RefreshAngleSumWeight();
	SCOPE_STRU GetCADEntScope(){return scope;}
};
//////////////////////////////////////////////////////////////////////////
//
class CDwgFileInfo 
{
private:
	CXhChar100 m_sFileName;	//文件名称
	BOOL m_bJgDwgFile;
	CProjectTowerType* m_pProject;
	AcDbObjectId m_idSolidLine;
	CHashList<CAngleProcessInfo> m_hashJgInfo;
	CPNCModel m_xPncMode;
	//
	BOOL RetrieveAngles();
	void CorrectAngles();
	//
	BOOL RetrievePlates();
	//
	int GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet);
	AcDbObjectId GetEntLineTypeId(AcDbEntity *pEnt);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//角钢DWG操作
	int GetJgNum(){return m_hashJgInfo.GetNodeNum();}
	CAngleProcessInfo* EnumFirstJg(){return m_hashJgInfo.GetFirst();}
	CAngleProcessInfo* EnumNextJg(){return m_hashJgInfo.GetNext();}
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
	void ModifyAngleDwgPartNum();
	//钢板DWG操作
	int GetPlateNum(){return m_xPncMode.GetPlateNum();}
	CPlateProcessInfo* EnumFirstPlate(){return m_xPncMode.EnumFirstPlate(FALSE);}
	CPlateProcessInfo* EnumNextPlate(){return m_xPncMode.EnumNextPlate(FALSE);}
	CPlateProcessInfo* FindPlateByPt(f3dPoint text_pos);
	CPlateProcessInfo* FindPlateByPartNo(const char* sPartNo);
	void ModifyPlateDwgPartNum();
	BOOL ReviseThePlate(const char* sPartNo);
	CPNCModel *GetPncModel() { return &m_xPncMode; }
	//
	void SetBelongModel(CProjectTowerType *pProject){m_pProject=pProject;}
	CProjectTowerType* BelongModel() const{return m_pProject;}
	CXhChar100 GetFileName(){return m_sFileName;}
	BOOL IsJgDwgInfo(){return m_bJgDwgFile;}
	BOOL InitDwgInfo(const char* sFileName,BOOL bJgDxf);
};
class CProjectTowerType
{
public:
	struct COMPARE_PART_RESULT
	{
		CProcessPart *pOrgPart;
		CProcessPart *pLoftPart;
		CHashStrList<BOOL> hashBoolByPropName;
		COMPARE_PART_RESULT(){pOrgPart = NULL;pLoftPart = NULL;};
	};
	//
	static const int COMPARE_BOM_FILE	= 1;
	static const int COMPARE_ANGLE_DWG	= 2;
	static const int COMPARE_PLATE_DWG	= 3;
	static const int COMPARE_ANGLE_DWGS	= 4;
	static const int COMPARE_PLATE_DWGS	= 5;

private:
	CDwgFileInfo* FindDwgFile(const char* sFileName,BOOL bAngleFile=FALSE){return NULL;}
	void AddBomResultSheet(LPDISPATCH pSheet,int index);
	void AddAngleResultSheet(LPDISPATCH pSheet,int index);
	void AddPlateResultSheet(LPDISPATCH pSheet,int index);
	void AddDwgLackPartSheet(LPDISPATCH pSheet);
public:
	DWORD key;
	CXhChar100 m_sProjName;
	CBomFile m_xLoftBom,m_xOrigBom;
	ATOM_LIST<CDwgFileInfo> dwgFileList;
	CHashStrList<COMPARE_PART_RESULT> m_hashCompareResultByPartNo;
public:
	CProjectTowerType();
	~CProjectTowerType();
	void SetKey(DWORD keyID){key=keyID;}
	void ReadProjectFile(CString sFilePath);
	void WriteProjectFile(CString sFilePath);
	BOOL ModifyErpBomPartNo(BYTE ciMatCharPosType);
	BOOL ModifyTmaBomPartNo(BYTE ciMatCharPosType);
	CPlateProcessInfo *FindPlateInfoByPartNo(const char* sPartNo);
	CAngleProcessInfo *FindAngleInfoByPartNo(const char* sPartNo);
	//初始化操作
	void InitBomInfo(const char* sFileName,BOOL bLoftBom);
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf);
	CDwgFileInfo* FindDwgBomInfo(const char* sFileName);
	//校审操作
	int CompareOrgAndLoftParts();
	int CompareLoftAndAngleDwgs();
	int CompareLoftAndPlateDwgs();
	int CompareLoftAndAngleDwg(const char* sFileName);
	int CompareLoftAndPlateDwg(const char* sFileName);
	void ExportCompareResult(int iCompare);
	//校审结果操作
	DWORD GetResultCount(){return m_hashCompareResultByPartNo.GetNodeNum();}
	COMPARE_PART_RESULT* GetResult(const char* part_no){return m_hashCompareResultByPartNo.GetValue(part_no);}
	COMPARE_PART_RESULT* EnumFirstResult(){return m_hashCompareResultByPartNo.GetFirst();}
	COMPARE_PART_RESULT* EnumNextResult(){return m_hashCompareResultByPartNo.GetNext();}
};
//////////////////////////////////////////////////////////////////////////
//
class CBomModel
{
public:
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
public:
	CBomModel(void);
	~CBomModel(void);
	CDwgFileInfo *FindDwgFile(const char* file_path);
	//
	static CXhChar16 QueryMatMarkIncQuality(CProcessPart *pPart);
	static void SendCommandToCad(CString sCmd);
};
extern CBomModel g_xUbomModel;
#endif
