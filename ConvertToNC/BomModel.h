#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "BOM.h"
#include "Variant.h"
#include "LogFile.h"
#include "FileIO.h"
#include "BomTblTitleCfg.h"
#include "..\ConvertToNC\PNCModel.h"
#include <vector>

using std::vector;
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
	AcDbObjectId keyId;
	AcDbObjectId partNumId;		//加工数ID
	AcDbObjectId singleNumId;	//单基数ID
	AcDbObjectId sumWeightId;	//总重ID
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
	void RefreshAngleSingleNum();
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
	void ModifyAngleDwgSingleNum();
	void ModifyAngleDwgSumWeight();
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
	BOOL ExtractThePlate();
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
	void AddDwgLackPartSheet(LPDISPATCH pSheet, int iCompareType);
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
	BOOL IsTmaBomFile(const char* sFileName,BOOL bDisplayMsgBox = FALSE);
	BOOL IsErpBomFile(const char* sFileName, BOOL bDisplayMsgBox = FALSE);
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
	const static char* KEY_PN;			//= "PartNo";
	const static char* KEY_MAT;			//= "Material";
	const static char* KEY_SPEC;		//= "Spec";
	const static char* KEY_LEN;			//= "Length";
	const static char* KEY_WIDE;		//= "Width";
	const static char* KEY_SING_N;		//= "SingNum";
	const static char* KEY_MANU_N;		//= "ManuNum"
	const static char* KEY_MANU_W;		//= "SumWeight"
	const static char* KEY_WELD;		//= "Weld"
	const static char* KEY_ZHI_WAN;		//= "ZhiWan"
	const static char* KEY_CUT_ANGLE;	//= "CutAngle"
	const static char* KEY_CUT_ROOT;	//= "CutRoot"
	const static char* KEY_CUT_BER;		//= "CutBer"
	const static char* KEY_PUSH_FLAT;	//= "PushFlat"
	const static char* KEY_KAI_JIAO;	//= "KaiJiao"
	const static char* KEY_HE_JIAO;		//= "HeJiao"
	const static char* KEY_FOO_NAIL;	//= "FootNail"
	const static char* KEY_NOTES;		//= "NOTE"
	//功能模块
	static const BYTE FUNC_BOM_COMPARE		 = 1;	//0X01料单校审
	static const BYTE FUNC_BOM_AMEND		 = 2;	//0X02修正料单
	static const BYTE FUNC_DWG_COMPARE		 = 3;	//0X04DWG数据校审
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X08修正加工数
	static const BYTE FUNC_DWG_AMEND_WEIGHT	 = 5;	//0X10修正重量
	static const BYTE FUNC_DWG_AMEND_SING_N  = 6;	//0X20修正单基数
	DWORD m_dwFunctionFlag;
	//
	struct BOM_TITLE
	{
		CXhChar16 m_sKey;
		CXhChar16 m_sTitle;
		int m_nWidth;	//标题宽度
		//
		BOM_TITLE(const char* sKey, const char* sTile, int nWidth)
		{
			m_sKey.Copy(sKey);
			m_sTitle.Copy(sTile);
			m_nWidth = nWidth;
		}
	};
	vector<BOM_TITLE> m_xBomTitleArr;
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
	CBomTblTitleCfg m_xTmaTblCfg, m_xErpTblCfg;
public:
	CBomModel(void);
	~CBomModel(void);
	//
	void InitBomTblCfg();
	int InitBomTitle();
	CDwgFileInfo *FindDwgFile(const char* file_path);
	bool IsValidFunc(int iFuncType);
	DWORD AddFuncType(int iFuncType);
	//
public:
	//客户ID
	static const BYTE ID_AnHui_HongYuan		= 1;	//安徽宏源(料单校审|修正料单|DWG校审|更新加工数)
	static const BYTE ID_AnHui_TingYang		= 2;	//安徽汀阳(料单校审|DWG校审|更新加工数|更新重量)
	static const BYTE ID_SiChuan_ChengDu	= 3;	//中电建成都铁塔(DWG校审|更新加工数)
	static const BYTE ID_JiangSu_HuaDian	= 4;	//江苏华电(料单校审)
	static const BYTE ID_ChengDu_DongFang	= 5;	//成都东方(DWG校审|更新单基数)
	static const BYTE ID_QingDao_HaoMai		= 6;	//青岛豪迈(DWG校审|更新加工数)
	static UINT m_uiCustomizeSerial;
	static CXhChar50 m_sCustomizeName;
	static BOOL m_bExeRppWhenArxLoad;				//加载Arx后执行rpp命令，显示对话框 wht 20-04-24
	static CXhChar200 m_sTMABomFileKeyStr;	//支持指定TMA Bom文件名关键字，TMA与ERP Excel文件格式一样时可通过关键字进行区分 wht 20-04-29
	static CXhChar200 m_sERPBomFileKeyStr;
	static CXhChar200 m_sTMABomSheetName;	//支持指定导入的Excel文件中的Sheet名称 wht 20-04-29
	static CXhChar200 m_sERPBomSheetName;

	//
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
extern CBomModel g_xUbomModel;
#endif
