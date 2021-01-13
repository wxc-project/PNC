#pragma once
#include "XeroNcPart.h"
#include "XeroExtractor.h"
#include "BomFile.h"
#include "TblDef.h"
#include "PNCDocReactor.h"
//////////////////////////////////////////////////////////////////////////
//CPNCModel
class CPNCModel
{
	CHashStrList<CPlateProcessInfo> m_hashPlateInfo;
public:
	PROJECT_INFO m_xPrjInfo;
	CString m_sCurWorkFile;		//��ǰ���ڲ������ļ�
public:
	CPNCModel(void);
	~CPNCModel(void);
	//
	void Empty();
	bool ExtractPlates(CString sDwgFile, BOOL bSupportSelectEnts=FALSE);
	void CreatePlatePPiFile(const char* work_path);
	//���Ƹְ�
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
	CHashStrList<CPlateProcessInfo> m_hashPlateInfo;
	CBomFile m_xPrintBomFile;	//DWG�ļ���Ӧ�Ĵ�ӡ�嵥 wht 20-07-21
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//�ļ�����
	BOOL RetrieveAngles(BOOL bSupportSelectEnts = FALSE);
	BOOL RetrievePlates(BOOL bSupportSelectEnts = FALSE);
protected:
	void InsertSubJgCard(CAngleProcessInfo* pJgInfo, BOMPART* pBomPart);
	void DimGridData(AcDbBlockTableRecord *pBlockTableRecord, GEPOINT orgPt, GRID_DATA_STRU& grid_data, const char* sText);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//
	void SetBelongModel(CProjectTowerType *pProject) { m_pProject = pProject; }
	CProjectTowerType* BelongModel() const { return m_pProject; }
	//�Ǹ�DWG����
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
	//�ְ�DWG����
	int GetPlateNum() { return m_hashPlateInfo.GetNodeNum(); }
	void EmptyPlateList() { m_hashPlateInfo.Empty(); }
	CPlateProcessInfo* EnumFirstPlate() { return m_hashPlateInfo.GetFirst(); }
	CPlateProcessInfo* EnumNextPlate() { return m_hashPlateInfo.GetNext(); }
	CPlateProcessInfo* FindPlateByPt(f3dPoint text_pos);
	CPlateProcessInfo* FindPlateByPartNo(const char* sPartNo);
	void ModifyPlateDwgPartNum();
	void ModifyPlateDwgSpec();
	void ModifyPlateDwgMaterial();
	void FillPlateDwgData();
	//��ӡ�嵥
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
	//���ڱ�ǱȽ�����
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
	void CompareData(BOMPART* pLoftPart, BOMPART* pDesPart, CHashStrList<BOOL> &hashBoolByPropName, BOOL bBomToBom = FALSE);
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
	//��ʼ������
	BOOL ReadTowerPrjInfo(const char* sFileName);
	void InitBomInfo(const char* sFileName, BOOL bLoftBom);
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName);
	CDwgFileInfo* FindDwgBomInfo(const char* sFileName);
	void DeleteDwgBomInfo(CDwgFileInfo* pDwgInfo);
	//У�����
	int CompareOrgAndLoftParts();
	int CompareLoftAndPartDwgs(BYTE ciTypeJ0_P1_A2 = 2);
	int CompareLoftAndPartDwg(const char* sFileName);
	void ExportCompareResult(int iCompare);
	//У��������
	DWORD GetResultCount() { return m_hashCompareResultByPartNo.GetNodeNum(); }
	COMPARE_PART_RESULT* GetResult(const char* part_no) { return m_hashCompareResultByPartNo.GetValue(part_no); }
	COMPARE_PART_RESULT* EnumFirstResult() { return m_hashCompareResultByPartNo.GetFirst(); }
	COMPARE_PART_RESULT* EnumNextResult() { return m_hashCompareResultByPartNo.GetNext(); }
};
//////////////////////////////////////////////////////////////////////////
//CUbomModel
enum CLIENT_SERIAL {
	ID_AnHui_HongYuan = 1,		//���պ�Դ0x0F
	ID_AnHui_TingYang = 2,		//����͡��0X1D
	ID_SiChuan_ChengDu = 3,		//�е罨�ɶ�����0X04
	ID_JiangSu_HuaDian = 4,		//���ջ���0x01
	ID_ChengDu_DongFang = 5,	//�ɶ�����0X24
	ID_QingDao_HaoMai = 6,		//�ൺ����0X4D
	ID_QingDao_QLGJG = 7,		//�ൺǿ���սṹ0x4C
	ID_QingDao_ZAILI = 8,		//�ൺ����0x4D
	ID_WUZHOU_DINGYI = 9,		//���޶���0X05
	ID_SHANDONG_HAUAN = 10,		//ɽ������0X40
	ID_QingDao_DingXing = 11,	//�ൺ����0x4C
	ID_QingDao_BaiSiTe = 12,	//�ൺ��˹��0x4C
	ID_QingDao_HuiJinTong = 13,	//�ൺ���ͨ0x4C
	ID_LuoYang_LongYu = 14,		//��������0x4C
	ID_JiangSu_DianZhuang = 15,	//���յ�װ0x5F
	ID_HeNan_YongGuang = 16,	//��������0x4C
	ID_GuangDong_AnHeng = 17,	//�㶫����0x4C
	ID_GuangDong_ChanTao = 18,	//�㶫����0x4C
	ID_ChongQing_JiangDian = 19,//���콭��0x44
	ID_QingDao_WuXiao = 20,		//�ൺ����0x44
	ID_OTHER = 100,
};
class CUbomModel
{
public:
	//����ģ��
	static const BYTE FUNC_BOM_COMPARE = 1;	//0X00000001�ϵ�У��
	static const BYTE FUNC_BOM_AMEND = 2;	//0X00000002�����ϵ�
	static const BYTE FUNC_DWG_COMPARE = 3;	//0X00000004DWG����У��
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X00000008�����ӹ���
	static const BYTE FUNC_DWG_AMEND_WEIGHT = 5;	//0X00000010��������
	static const BYTE FUNC_DWG_AMEND_SING_N = 6;	//0X00000020����������
	static const BYTE FUNC_DWG_BATCH_PRINT = 7;	//0x00000040������ӡ wht 20-05-26
	static const BYTE FUNC_DWG_AMEND_SPEC = 8;	//0x00000080�������
	static const BYTE FUNC_DWG_AMEND_MAT = 9;	//0x00000100��������
	static const BYTE FUNC_DWG_AMEND_TA_NUM = 10;	//0x00000200��������
	static const BYTE FUNC_DWG_FILL_DADA = 11;	//0x00000400�������
	DWORD m_dwFunctionFlag;
	//���ƿͻ�
	UINT m_uiCustomizeSerial;
	CXhChar50 m_sCustomizeName;
	std::map<CString, int> m_xMapClientInfo;
	std::multimap<int, std::pair<CString,CString> > m_xMapClientCfgFile;
	//���ò���
	double m_fMaxLenErr;				//����������ֵ
	BOOL m_bCmpQualityLevel;			//�����ȼ�У��
	BOOL m_bEqualH_h;					//Q345�Ƿ����Q355
	BOOL m_bExeRppWhenArxLoad;			//����Arx��ִ��rpp�����ʾ�Ի��� wht 20-04-24
	BOOL m_bExtractPltesWhenOpenFile;	//�򿪸ְ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
	BOOL m_bExtractAnglesWhenOpenFile;	//�򿪽Ǹ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
	UINT m_uiJgCadPartLabelMat;			//�Ǹֹ��տ���ȡ���Ų��ʷ�:	0:����Ӳ��ʷ�	1:���	2:�Ҳ�
	CXhChar100 m_sJgCadName;			//�Ǹֹ��տ�����
	CXhChar50 m_sJgCadPartLabel;		//�Ǹֹ��տ��еļ��ű���
	CXhChar50 m_sJgCardBlockName;		//�Ǹֹ��տ������� wht 19-09-24
	CXhChar500 m_sNotPrintFilter;		//֧������������ӡʱ����Ҫ��ӡ�Ĺ��� wht 20-07-27
	BYTE m_ciPrintSortType;				//0.���ϵ�����|1.����������
	std::map<CString, CString> m_xMapPrjCell;	//ָ����Ԫ��Ĺ�����Ϣ
	//���ݴ洢
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
