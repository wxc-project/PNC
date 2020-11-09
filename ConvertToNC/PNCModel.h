#pragma once
#include "XeroNcPart.h"
#include "BomFile.h"
#include "TblDef.h"
#include "PNCDocReactor.h"
//////////////////////////////////////////////////////////////////////////
//CPNCModel
class CPNCModel
{
	CHashStrList<CPlateProcessInfo> m_hashPlateInfo;
	CHashSet<AcDbObjectId> m_hashSolidLineTypeId;	//记录有效的实体线型id wht 19-01-03
public:
	CHashSet<AcDbObjectId> m_xAllEntIdSet;
	CHashSet<AcDbObjectId> m_xAllLineHash;
	CHashStrList<CBoltEntGroup> m_xBoltEntHash;
	PROJECT_INFO m_xPrjInfo;
	CXhChar500 m_sWorkPath;		//当前模型对应的工作路径 wht 19-04-02
	CString m_sCurWorkFile;		//当前正在操作的文件
	static const float ASSIST_RADIUS;
	static const float DIST_ERROR;
	static const float WELD_MAX_HEIGHT;
	static BOOL m_bSendCommand;
	//
	static CString MakePosKeyStr(GEPOINT pos);
	static double StandardHoleD(double fDiameter);
private:
	bool AppendBoltEntsByBlock(ULONG idBlockEnt);
	bool AppendBoltEntsByCircle(ULONG idCirEnt);
	bool AppendBoltEntsByPolyline(ULONG idPolyline);
	bool AppendBoltEntsByConnectLines(vector<CAD_LINE> vectorConnLine);
	bool AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine);
public:
	CPNCModel(void);
	~CPNCModel(void);
	//
	void Empty();
	void ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols);
	void InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts);
	void MergeManyPartNo();
	void SplitManyPartNo();
	void CreatePlatePPiFile(const char* work_path);
	//绘制钢板
	void DrawPlates(BOOL bOnlyNewExtractedPlate=FALSE);
	void DrawPlatesToLayout(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToCompare(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToProcess(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToClone(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToFiltrate(BOOL bOnlyNewExtractedPlate = FALSE);
	//
	int GetPlateNum(){return m_hashPlateInfo.GetNodeNum();}
	int PushPlateStack() { return m_hashPlateInfo.push_stack(); }
	bool PopPlateStack() { return m_hashPlateInfo.pop_stack(); }
	CPlateProcessInfo* PartFromPartNo(const char* sPartNo) { return m_hashPlateInfo.GetValue(sPartNo); }
	CPlateProcessInfo* AppendPlate(char* sPartNo){return m_hashPlateInfo.Add(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(char* sPartNo){return m_hashPlateInfo.GetValue(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(AcDbObjectId partNoEntId);
	CPlateProcessInfo* GetPlateInfo(GEPOINT text_pos);
	CPlateProcessInfo* EnumFirstPlate(BOOL bOnlyNewExtractedPlate)
	{
		if (bOnlyNewExtractedPlate)
		{
			CPlateProcessInfo* pPlate = m_hashPlateInfo.GetFirst();
			while (pPlate&&!pPlate->m_bNeedExtract)
				pPlate = m_hashPlateInfo.GetNext();
			return pPlate;
		}
		else
			return m_hashPlateInfo.GetFirst();
	}
	CPlateProcessInfo* EnumNextPlate(BOOL bOnlyNewExtractedPlate)
	{
		if (bOnlyNewExtractedPlate)
		{
			CPlateProcessInfo* pPlate = m_hashPlateInfo.GetNext();
			while (pPlate&&!pPlate->m_bNeedExtract)
				pPlate = m_hashPlateInfo.GetNext();
			return pPlate;
		}
		else
			return m_hashPlateInfo.GetNext();
	}
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
	CSortedModel(CPNCModel *pModel, BOOL bOnlyNewExtractedPlate = FALSE);
	//
	CPlateProcessInfo *EnumFirstPlate();
	CPlateProcessInfo *EnumNextPlate();
	void DividPlatesBySeg();
	void DividPlatesByThickMat();
	void DividPlatesByThick();
	void DividPlatesByMat();
	void DividPlatesByPartNo();
};

#ifdef __UBOM_ONLY_

struct PART_LABEL_DIM {
	CAD_ENTITY m_xCirEnt;
	CAD_ENTITY m_xInnerText;
	BOOL IsPartLabelDim();
	SEGI GetSegI();
};
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
class CProjectTowerType;
class CDwgFileInfo
{
private:
	CProjectTowerType* m_pProject;
	CHashList<CAngleProcessInfo> m_hashJgInfo;
	CPNCModel m_xPncMode;
	CBomFile m_xPrintBomFile;	//DWG文件对应的打印清单 wht 20-07-21
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//文件名称
	BOOL RetrieveAngles(BOOL bSupportSelectEnts = FALSE);
	BOOL RetrievePlates(BOOL bSupportSelectEnts = FALSE);
protected:
	void InsertSubJgCard(CAngleProcessInfo* pJgInfo);
	void DimGridData(AcDbBlockTableRecord *pBlockTableRecord, GEPOINT orgPt, GRID_DATA_STRU& grid_data, const char* sText);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//
	void SetBelongModel(CProjectTowerType *pProject) { m_pProject = pProject; }
	CProjectTowerType* BelongModel() const { return m_pProject; }
	//角钢DWG操作
	int GetAngleNum() { return m_hashJgInfo.GetNodeNum(); }
	void EmptyJgList() { m_hashJgInfo.Empty(); }
	CAngleProcessInfo* EnumFirstJg() { return m_hashJgInfo.GetFirst(); }
	CAngleProcessInfo* EnumNextJg() { return m_hashJgInfo.GetNext(); }
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
	void ModifyAngleDwgPartNum();
	void ModifyAngleDwgSingleNum();
	void ModifyAngleDwgSumWeight();
	void ModifyAngleDwgSpec();
	void ModifyAngleDwgMaterial();
	void FillAngleDwgData();
	//钢板DWG操作
	int GetPlateNum() { return m_xPncMode.GetPlateNum(); }
	void EmptyPlateList() { m_xPncMode.Empty(); }
	CPlateProcessInfo* EnumFirstPlate() { return m_xPncMode.EnumFirstPlate(FALSE); }
	CPlateProcessInfo* EnumNextPlate() { return m_xPncMode.EnumNextPlate(FALSE); }
	CPlateProcessInfo* FindPlateByPt(f3dPoint text_pos);
	CPlateProcessInfo* FindPlateByPartNo(const char* sPartNo);
	void ModifyPlateDwgPartNum();
	void ModifyPlateDwgSpec();
	void ModifyPlateDwgMaterial();
	void FillPlateDwgData();
	CPNCModel *GetPncModel() { return &m_xPncMode; }
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
	static const int COMPARE_DWG_FILE = 2;
	static const int COMPARE_ALL_DWGS = 3;
	//
	struct COMPARE_PART_RESULT
	{
		BOMPART *pOrgPart;
		BOMPART *pLoftPart;
		CHashStrList<BOOL> hashBoolByPropName;
		COMPARE_PART_RESULT() { pOrgPart = NULL; pLoftPart = NULL; };
	};
private:
	void CompareData(BOMPART* pLoftPart, BOMPART* pDesPart, CHashStrList<BOOL> &hashBoolByPropName);
	void AddDwgLackPartSheet(LPDISPATCH pSheet, int iCompareType);
	void AddCompareResultSheet(LPDISPATCH pSheet, int index, int iCompareType);
public:
	DWORD key;
	PROJECT_INFO m_xPrjInfo;
	CXhChar100 m_sProjName;
	CBomFile m_xLoftBom, m_xOrigBom;
	ATOM_LIST<CDwgFileInfo> dwgFileList;
	CHashStrList<COMPARE_PART_RESULT> m_hashCompareResultByPartNo;
public:
	CProjectTowerType();
	~CProjectTowerType();
	//
	void SetKey(DWORD keyID) { key = keyID; }
	void ReadProjectFile(CString sFilePath);
	void WriteProjectFile(CString sFilePath);
	BOOL ModifyErpBomPartNo(BYTE ciMatCharPosType);
	BOOL ModifyTmaBomPartNo(BYTE ciMatCharPosType);
	CPlateProcessInfo *FindPlateInfoByPartNo(const char* sPartNo);
	CAngleProcessInfo *FindAngleInfoByPartNo(const char* sPartNo);
	//初始化操作
	BOOL ReadTowerPrjInfo(const char* sFileName);
	void InitBomInfo(const char* sFileName, BOOL bLoftBom);
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName);
	CDwgFileInfo* FindDwgBomInfo(const char* sFileName);
	void DeleteDwgBomInfo(CDwgFileInfo* pDwgInfo);
	//校审操作
	int CompareOrgAndLoftParts();
	int CompareLoftAndPartDwgs(BYTE ciTypeJ0_P1_A2 = 2);
	int CompareLoftAndPartDwg(const char* sFileName);
	void ExportCompareResult(int iCompare);
	//校审结果操作
	DWORD GetResultCount() { return m_hashCompareResultByPartNo.GetNodeNum(); }
	COMPARE_PART_RESULT* GetResult(const char* part_no) { return m_hashCompareResultByPartNo.GetValue(part_no); }
	COMPARE_PART_RESULT* EnumFirstResult() { return m_hashCompareResultByPartNo.GetFirst(); }
	COMPARE_PART_RESULT* EnumNextResult() { return m_hashCompareResultByPartNo.GetNext(); }
};
//////////////////////////////////////////////////////////////////////////
//CUbomModel
enum CLIENT_SERIAL {
	ID_AnHui_HongYuan = 1,		//安徽宏源0x0F
	ID_AnHui_TingYang = 2,		//安徽汀阳0X1D
	ID_SiChuan_ChengDu = 3,		//中电建成都铁塔0X04
	ID_JiangSu_HuaDian = 4,		//江苏华电0x01
	ID_ChengDu_DongFang = 5,	//成都东方0X24
	ID_QingDao_HaoMai = 6,		//青岛豪迈0X4D
	ID_QingDao_QLGJG = 7,		//青岛强力刚结构0x4C
	ID_QingDao_ZAILI = 8,		//青岛载力0x4D
	ID_WUZHOU_DINGYI = 9,		//五洲鼎益0X05
	ID_SHANDONG_HAUAN = 10,		//山东华安0X40
	ID_QingDao_DingXing = 11,	//青岛鼎兴0x4C
	ID_QingDao_BaiSiTe = 12,	//青岛百斯特0x4C
	ID_QingDao_HuiJinTong = 13,	//青岛汇金通0x4C
	ID_LuoYang_LongYu = 14,		//洛阳龙羽0x4C
	ID_JiangSu_DianZhuang = 15,	//江苏电装0x5F
	ID_HeNan_YongGuang = 16,	//河南永光0x4C
	ID_GuangDong_AnHeng = 17,	//广东安恒0x4C
	ID_GuangDong_ChanTao = 18,	//广东禅涛0x4C
	ID_ChongQing_JiangDian = 19,//重庆江电0x44
	ID_QingDao_WuXiao = 20,		//青岛武晓0x44
	ID_OTHER = 100,
};
class CUbomModel
{
public:
	//功能模块
	static const BYTE FUNC_BOM_COMPARE = 1;	//0X00000001料单校审
	static const BYTE FUNC_BOM_AMEND = 2;	//0X00000002修正料单
	static const BYTE FUNC_DWG_COMPARE = 3;	//0X00000004DWG数据校审
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X00000008修正加工数
	static const BYTE FUNC_DWG_AMEND_WEIGHT = 5;	//0X00000010修正重量
	static const BYTE FUNC_DWG_AMEND_SING_N = 6;	//0X00000020修正单基数
	static const BYTE FUNC_DWG_BATCH_PRINT = 7;	//0x00000040批量打印 wht 20-05-26
	static const BYTE FUNC_DWG_AMEND_SPEC = 8;	//0x00000080修正规格
	static const BYTE FUNC_DWG_AMEND_MAT = 9;	//0x00000100修正材质
	static const BYTE FUNC_DWG_AMEND_TA_NUM = 10;	//0x00000200修正基数
	static const BYTE FUNC_DWG_FILL_DADA = 11;	//0x00000400填充数据
	DWORD m_dwFunctionFlag;
	//定制客户
	UINT m_uiCustomizeSerial;
	CXhChar50 m_sCustomizeName;
	std::map<CString, int> m_xMapClientInfo;
	std::multimap<int, CString> m_xMapClientCfgFile;
	//配置参数
	double m_fMaxLenErr;				//长度最大误差值
	BOOL m_bCmpQualityLevel;			//质量等级校审
	BOOL m_bEqualH_h;					//Q345是否等于Q355
	BOOL m_bExeRppWhenArxLoad;			//加载Arx后执行rpp命令，显示对话框 wht 20-04-24
	BOOL m_bExtractPltesWhenOpenFile;	//打开钢板文件后执行提取操作,默认为TRUE wht 20-07-29
	BOOL m_bExtractAnglesWhenOpenFile;	//打开角钢文件后执行提取操作,默认为TRUE wht 20-07-29
	UINT m_uiJgCadPartLabelMat;			//角钢工艺卡提取件号材质符:	0:不添加材质符	1:左侧	2:右侧
	CXhChar100 m_sJgCadName;			//角钢工艺卡名称
	CXhChar50 m_sJgCadPartLabel;		//角钢工艺卡中的件号标题
	CXhChar50 m_sJgCardBlockName;		//角钢工艺卡块名称 wht 19-09-24
	CXhChar500 m_sNotPrintFilter;		//支持设置批量打印时不需要打印的构件 wht 20-07-27
	BYTE m_ciPrintSortType;				//0.按料单排序|1.按件号排序
	std::map<CString, CString> m_xMapPrjCell;	//指定单元格的工程信息
	//数据存储
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
public:
	CUbomModel(void);
	~CUbomModel(void);
	//
	void InitBomModel();
	bool IsValidFunc(int iFuncType);
	BOOL IsJgCardBlockName(const char* sBlockName);
	BOOL IsPartLabelTitle(const char* sText);
	BOOL IsNeedPrint(BOMPART *pPart, const char* sNotes);
	//
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
#endif
//////////////////////////////////////////////////////////////////////////
extern CPNCModel model;
#ifdef __UBOM_ONLY_
extern CUbomModel g_xUbomModel;
#endif
