#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "..\..\LDS\LDS\BOM\BOM.h"
#include "Variant.h"
#include "LogFile.h"
#include "FileIO.h"
#include "BomTblTitleCfg.h"
#include "..\ConvertToNC\PNCModel.h"
#include <vector>
#include "BomFile.h"

using std::vector;
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
class CAngleProcessInfo
{
private:
	f3dPoint orig_pt;
	POLYGON region;
	SCOPE_STRU scope;
public:
	static const BYTE TYPE_JG = 1;		//角钢
	static const BYTE TYPE_YG = 2;		//圆钢
	static const BYTE TYPE_TUBE = 3;	//钢管
	static const BYTE TYPE_FLAT = 4;	//扁铁
	static const BYTE TYPE_JIG = 5;		//夹具
	static const BYTE TYPE_GGS = 6;		//钢格栅
	static const BYTE TYPE_PLATE = 7;	//钢板
	static const BYTE TYPE_WIRE_TUBE = 8;//套管
	BYTE m_ciType;				//
	//
	PART_ANGLE m_xAngle;
	AcDbObjectId keyId;
	AcDbObjectId partNumId;		//加工数ID
	AcDbObjectId singleNumId;	//单基数ID
	AcDbObjectId sumWeightId;	//总重ID
	CXhChar200 m_sTowerType;
	//加工数、单基数、总重修改状态 wht 20-07-29
	static const BYTE MODIFY_MANU_NUM	= 0x01;
	static const BYTE MODIFY_SINGLE_NUM = 0x02;
	static const BYTE MODIFY_SUM_WEIGHT = 0x04;
	BYTE m_ciModifyState;
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
	bool PtInDrawRect(f3dPoint pt);
	void RefreshAngleNum();
	void RefreshAngleSingleNum();
	void RefreshAngleSumWeight();
	SCOPE_STRU GetCADEntScope(){return scope;}
};
//////////////////////////////////////////////////////////////////////////
//
class CProjectTowerType;
class CDwgFileInfo 
{
private:
	BOOL m_bJgDwgFile;
	CProjectTowerType* m_pProject;
	CHashList<CAngleProcessInfo> m_hashJgInfo;
	CPNCModel m_xPncMode;
	CBomFile m_xPrintBomFile;	//DWG文件对应的打印清单 wht 20-07-21
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//文件名称
	BOOL RetrieveAngles(BOOL bSupportSelectEnts =FALSE);
	BOOL RetrievePlates(BOOL bSupportSelectEnts =FALSE);
protected:
	int GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//角钢DWG操作
	int GetJgNum(){return m_hashJgInfo.GetNodeNum();}
	void EmptyJgList() { m_hashJgInfo.Empty(); }
	CAngleProcessInfo* EnumFirstJg(){return m_hashJgInfo.GetFirst();}
	CAngleProcessInfo* EnumNextJg(){return m_hashJgInfo.GetNext();}
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
	void ModifyAngleDwgPartNum();
	void ModifyAngleDwgSingleNum();
	void ModifyAngleDwgSumWeight();
	//钢板DWG操作
	int GetPlateNum(){return m_xPncMode.GetPlateNum();}
	void EmptyPlateList() { m_xPncMode.Empty(); }
	CPlateProcessInfo* EnumFirstPlate(){return m_xPncMode.EnumFirstPlate(FALSE);}
	CPlateProcessInfo* EnumNextPlate(){return m_xPncMode.EnumNextPlate(FALSE);}
	CPlateProcessInfo* FindPlateByPt(f3dPoint text_pos);
	CPlateProcessInfo* FindPlateByPartNo(const char* sPartNo);
	void ModifyPlateDwgPartNum();
	CPNCModel *GetPncModel() { return &m_xPncMode; }
	//
	void SetBelongModel(CProjectTowerType *pProject){m_pProject=pProject;}
	CProjectTowerType* BelongModel() const{return m_pProject;}
	BOOL IsJgDwgInfo();
	BOOL IsPlateDwgInfo();
	BOOL ExtractDwgInfo(const char* sFileName,BOOL bJgDxf,BOOL bExtractPart);
	BOOL ExtractThePlate();
	//打印清单
	BOOL ImportPrintBomExcelFile(const char* sFileName);
	void EmptyPrintBom() { m_xPrintBomFile.Empty(); }
	int PrintBomPartCount() { return m_xPrintBomFile.GetPartNum(); }
	BOMPART* EnumFirstPrintPart() { return m_xPrintBomFile.EnumFirstPart(); }
	BOMPART* EnumNextPrintPart() { return m_xPrintBomFile.EnumNextPart(); }
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
	static const int COMPARE_PARTS_DWG = 6;	//角钢钢板在一张dwg文件中
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
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf,BOOL bExtractPart);
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
	//功能模块
	static const BYTE FUNC_BOM_COMPARE		 = 1;	//0X01料单校审
	static const BYTE FUNC_BOM_AMEND		 = 2;	//0X02修正料单
	static const BYTE FUNC_DWG_COMPARE		 = 3;	//0X04DWG数据校审
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X08修正加工数
	static const BYTE FUNC_DWG_AMEND_WEIGHT	 = 5;	//0X10修正重量
	static const BYTE FUNC_DWG_AMEND_SING_N  = 6;	//0X20修正单基数
	static const BYTE FUNC_DWG_BATCH_PRINT	 = 7;	//0x40批量打印 wht 20-05-26
	DWORD m_dwFunctionFlag;
	
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
	CBomImportCfg m_xBomImoprtCfg;
	CXhChar500 m_sNotPrintFilter;		//支持设置批量打印时不需要打印的构件 wht 20-07-27
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
	BOOL IsHasTheProcess(const char* sText, BYTE ciType);
	BOOL IsZhiWan(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_ZHI_WAN); }
	BOOL IsPushFlat(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_PUSH_FLAT); }
	BOOL IsCutAngle(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_CUT_ANGLE); }
	BOOL IsCutRoot(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_CUT_ROOT); }
	BOOL IsCutBer(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_CUT_BER); }
	BOOL IsKaiJiao(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_KAI_JIAO); }
	BOOL IsHeJiao(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_HE_JIAO); }
	BOOL IsFootNail(const char* sText) { return m_xBomImoprtCfg.IsHasTheProcess(sText, CBomImportCfg::TYPE_FOO_NAIL); }
public:
	//客户ID
	static const BYTE ID_AnHui_HongYuan		= 1;	//安徽宏源(料单校审|修正料单|DWG校审|更新加工数)
	static const BYTE ID_AnHui_TingYang		= 2;	//安徽汀阳(料单校审|DWG校审|更新加工数|更新重量)
	static const BYTE ID_SiChuan_ChengDu	= 3;	//中电建成都铁塔(DWG校审|更新加工数)
	static const BYTE ID_JiangSu_HuaDian	= 4;	//江苏华电(料单校审)
	static const BYTE ID_ChengDu_DongFang	= 5;	//成都东方(DWG校审|更新单基数)
	static const BYTE ID_QingDao_HaoMai		= 6;	//青岛豪迈(DWG校审|更新加工数)
	static const BYTE ID_QingDao_QLGJG		= 7;	//青岛强力刚结构(DWG校审|更新加工数)
	static const BYTE ID_QingDao_ZAILI		= 8;	//青岛载力(DWG校审|更新加工数|更新基数|批量打印)
	static const BYTE ID_WUZHOU_DINGYI		= 9;	//五洲鼎益(料单校审|DWG校审)
	static const BYTE ID_SHANDONG_HAUAN		= 10;	//山东华安(批量打印)
	UINT m_uiCustomizeSerial;
	CXhChar50 m_sCustomizeName;
	BOOL m_bExeRppWhenArxLoad;				//加载Arx后执行rpp命令，显示对话框 wht 20-04-24
	BOOL m_bExtractPltesWhenOpenFile;		//打开钢板文件后执行提取操作,默认为TRUE wht 20-07-29
	BOOL m_bExtractAnglesWhenOpenFile;		//打开角钢文件后执行提取操作,默认为TRUE wht 20-07-29
	//
	BOOL IsAngleCompareItem(const char* title) { return m_xBomImoprtCfg.IsAngleCompareItem(title); }
	BOOL IsPlateCompareItem(const char* title) { return m_xBomImoprtCfg.IsPlateCompareItem(title); }
	size_t GetBomTitleCount() { return m_xBomImoprtCfg.GetBomTitleCount(); }
	BOOL IsTitleCol(int index, const char*title_key) { return m_xBomImoprtCfg.IsTitleCol(index,title_key); }
	CXhChar16 GetTitleKey(int index);
	CXhChar16 GetTitleName(int index);
	int GetTitleWidth(int index);
	BOOL IsValidTmaBomCfg() { return m_xBomImoprtCfg.m_xTmaTblCfg.IsValid(); }
	BOOL IsValidErpBomCfg() { return m_xBomImoprtCfg.m_xErpTblCfg.IsValid(); }
	BOOL IsValidPrintBomCfg() { return m_xBomImoprtCfg.m_xPrintTblCfg.IsValid(); }
	BOOL IsNeedPrint(BOMPART *pPart, const char* sNotes);
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
extern CBomModel g_xUbomModel;
#endif
