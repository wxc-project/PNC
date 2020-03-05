#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "BOM.h"
#include "Variant.h"
#include "LogFile.h"
#include "FileIO.h"
#include "BomTblTitleCfg.h"
#include "..\ConvertToNC\PNCModel.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
class CProjectTowerType;
class CBomFile
{
	CProjectTowerType* m_pProject;
	CSuperHashStrList<BOMPART> m_hashPartByPartNo;
public:
	CXhChar500 m_sFileName;		//文件名称
	CXhChar100 m_sBomName;
protected:
	BOOL ParseSheetContent(CVariant2dArray &sheetContentMap, CHashStrList<DWORD>& hashColIndex, int iStartRow);
public:
	CBomFile();
	~CBomFile();
	//
	void Empty(){
		m_hashPartByPartNo.Empty();
		m_sFileName.Empty();
	}
	void SetBelongModel(CProjectTowerType *pProject) {m_pProject = pProject; }
	CProjectTowerType* BelongModel() const { return m_pProject; }
	int GetPartNum() { return m_hashPartByPartNo.GetNodeNum(); }
	BOMPART* EnumFirstPart(){return m_hashPartByPartNo.GetFirst();}
	BOMPART* EnumNextPart(){return m_hashPartByPartNo.GetNext();}
	BOMPART* FindPart(const char* sKey){return m_hashPartByPartNo.GetValue(sKey);}
	//
	BOOL ImportTmaExcelFile();
	BOOL ImportErpExcelFile();
	CString GetPartNumStr();
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
	PART_ANGLE m_xAngle;
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
	BOOL m_bJgDwgFile;
	CProjectTowerType* m_pProject;
	CHashList<CAngleProcessInfo> m_hashJgInfo;
	CPNCModel m_xPncMode;
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//文件名称
protected:
	BOOL RetrieveAngles();
	BOOL RetrievePlates();
	int GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet);
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
	CPNCModel *GetPncModel() { return &m_xPncMode; }
	//
	void SetBelongModel(CProjectTowerType *pProject){m_pProject=pProject;}
	CProjectTowerType* BelongModel() const{return m_pProject;}
	BOOL IsJgDwgInfo(){return m_bJgDwgFile;}
	BOOL ExtractDwgInfo(const char* sFileName,BOOL bJgDxf);
};
//////////////////////////////////////////////////////////////////////////
//CProjectTowerType
class CProjectTowerType
{
public:
	//用于标记比较类型
	static const int COMPARE_BOM_FILE = 1;
	static const int COMPARE_ANGLE_DWG = 2;
	static const int COMPARE_PLATE_DWG = 3;
	static const int COMPARE_ANGLE_DWGS = 4;
	static const int COMPARE_PLATE_DWGS = 5;
	//
	struct COMPARE_PART_RESULT
	{
		BOMPART *pOrgPart;
		BOMPART *pLoftPart;
		CHashStrList<BOOL> hashBoolByPropName;
		COMPARE_PART_RESULT(){pOrgPart = NULL;pLoftPart = NULL;};
	};
private:
	void CompareData(BOMPART* pLoftPart, BOMPART* pDesPart, CHashStrList<BOOL> &hashBoolByPropName);
	void AddBomResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr);
	void AddAngleResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr);
	void AddPlateResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr);
	void AddDwgLackPartSheet(LPDISPATCH pSheet);
	void AddCompareResultSheet(LPDISPATCH pSheet, int index, int iCompareType);
public:
	DWORD key;
	CXhChar100 m_sProjName;
	CBomFile m_xLoftBom,m_xOrigBom;
	ATOM_LIST<CDwgFileInfo> dwgFileList;
	CHashStrList<COMPARE_PART_RESULT> m_hashCompareResultByPartNo;
public:
	CProjectTowerType();
	~CProjectTowerType();
	//
	void SetKey(DWORD keyID){key=keyID;}
	void ReadProjectFile(CString sFilePath);
	void WriteProjectFile(CString sFilePath);
	BOOL ModifyErpBomPartNo(BYTE ciMatCharPosType);
	BOOL ModifyTmaBomPartNo(BYTE ciMatCharPosType);
	CPlateProcessInfo *FindPlateInfoByPartNo(const char* sPartNo);
	CAngleProcessInfo *FindAngleInfoByPartNo(const char* sPartNo);
	//初始化操作
	BOOL IsTmaBomFile(const char* sFileName);
	BOOL IsErpBomFile(const char* sFileName);
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
//CBomModel
class CBomModel
{
public:
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
	CBomTblTitleCfg m_xTmaTblCfg, m_xErpTblCfg;
public:
	CBomModel(void);
	~CBomModel(void);
	//
	void InitBomTblCfg();
	CDwgFileInfo *FindDwgFile(const char* file_path);
public:
	static const BYTE ID_AnHui_HongYuan		= 1;	//安徽宏源
	static const BYTE ID_AnHui_TingYang		= 2;	//安徽汀阳
	static const BYTE ID_SiChuan_ChengDu	= 3;	//中电建成都铁塔
	static const BYTE ID_JiangSu_HuaDian	= 4;	//江苏华电
	static const BYTE ID_ChengDu_DongFang	= 5;	//成都东方
	static UINT m_uiCustomizeSerial;
	static CXhChar100 GetClientName();
	//
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
extern CBomModel g_xUbomModel;
#endif
