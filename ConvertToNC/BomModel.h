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

using std::vector;
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
class CProjectTowerType;
class CBomFile
{
	CProjectTowerType* m_pProject;
	CSuperHashStrList<BOMPART> m_hashPartByPartNo;
public:
	CXhChar500 m_sFileName;		//�ļ�����
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
	BOOL ImportPrintExcelFile();
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
	static const BYTE TYPE_JG = 1;		//�Ǹ�
	static const BYTE TYPE_YG = 2;		//Բ��
	static const BYTE TYPE_TUBE = 3;	//�ֹ�
	static const BYTE TYPE_FLAT = 4;	//����
	static const BYTE TYPE_JIG = 5;		//�о�
	static const BYTE TYPE_GGS = 6;		//�ָ�դ
	static const BYTE TYPE_PLATE = 7;	//�Ǹ�
	BYTE m_ciType;				//
	//
	PART_ANGLE m_xAngle;
	AcDbObjectId keyId;
	AcDbObjectId partNumId;		//�ӹ���ID
	AcDbObjectId singleNumId;	//������ID
	AcDbObjectId sumWeightId;	//����ID
	CXhChar200 m_sTowerType;
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
class CDwgFileInfo 
{
private:
	BOOL m_bJgDwgFile;
	CProjectTowerType* m_pProject;
	CHashList<CAngleProcessInfo> m_hashJgInfo;
	CPNCModel m_xPncMode;
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//�ļ�����
protected:
	BOOL RetrieveAngles();
	BOOL RetrievePlates();
	int GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//�Ǹ�DWG����
	int GetJgNum(){return m_hashJgInfo.GetNodeNum();}
	CAngleProcessInfo* EnumFirstJg(){return m_hashJgInfo.GetFirst();}
	CAngleProcessInfo* EnumNextJg(){return m_hashJgInfo.GetNext();}
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
	void ModifyAngleDwgPartNum();
	void ModifyAngleDwgSingleNum();
	void ModifyAngleDwgSumWeight();
	//�ְ�DWG����
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
	//���ڱ�ǱȽ�����
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
	//��ʼ������
	BOOL IsTmaBomFile(const char* sFileName,BOOL bDisplayMsgBox = FALSE);
	BOOL IsErpBomFile(const char* sFileName, BOOL bDisplayMsgBox = FALSE);
	void InitBomInfo(const char* sFileName,BOOL bLoftBom);
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf);
	CDwgFileInfo* FindDwgBomInfo(const char* sFileName);
	//У�����
	int CompareOrgAndLoftParts();
	int CompareLoftAndAngleDwgs();
	int CompareLoftAndPlateDwgs();
	int CompareLoftAndAngleDwg(const char* sFileName);
	int CompareLoftAndPlateDwg(const char* sFileName);
	void ExportCompareResult(int iCompare);
	//У��������
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
	//����ģ��
	static const BYTE FUNC_BOM_COMPARE		 = 1;	//0X01�ϵ�У��
	static const BYTE FUNC_BOM_AMEND		 = 2;	//0X02�����ϵ�
	static const BYTE FUNC_DWG_COMPARE		 = 3;	//0X04DWG����У��
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X08�����ӹ���
	static const BYTE FUNC_DWG_AMEND_WEIGHT	 = 5;	//0X10��������
	static const BYTE FUNC_DWG_AMEND_SING_N  = 6;	//0X20����������
	static const BYTE FUNC_DWG_BATCH_PRINT	 = 7;	//0x40������ӡ wht 20-05-26
	DWORD m_dwFunctionFlag;
	//��������
	const static BYTE TYPE_ZHI_WAN		= 0;
	const static BYTE TYPE_CUT_ANGLE	= 1;
	const static BYTE TYPE_CUT_ROOT		= 2;
	const static BYTE TYPE_CUT_BER		= 3;
	const static BYTE TYPE_PUSH_FLAT	= 4;
	const static BYTE TYPE_KAI_JIAO		= 5;
	const static BYTE TYPE_HE_JIAO		= 6;
	const static BYTE TYPE_FOO_NAIL		= 7;
	CXhChar100 m_sProcessDescArr[8];
	//
	struct BOM_TITLE
	{
		CXhChar16 m_sKey;
		CXhChar16 m_sTitle;
		int m_nWidth;	//������
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
	CBomTblTitleCfg m_xTmaTblCfg, m_xErpTblCfg, m_xJgPrintCfg;
	CXhChar500 m_sAngleCompareItemArr;	//�Ǹ�У����
	CXhChar500 m_sPlateCompareItemArr;	//�ְ�У����
	CHashStrList<BOOL> hashCompareItemOfAngle;
	CHashStrList<BOOL> hashCompareItemOfPlate;
public:
	CBomModel(void);
	~CBomModel(void);
	//
	void InitBomTblCfg();
	int InitBomTitle();
	CDwgFileInfo *FindDwgFile(const char* file_path);
	bool IsValidFunc(int iFuncType);
	DWORD AddFuncType(int iFuncType);
	bool ExtractAngleCompareItems();
	bool ExtractPlateCompareItems();
	//
	BOOL IsHasTheProcess(const char* sText, BYTE ciType);
	BOOL IsZhiWan(const char* sText) { return IsHasTheProcess(sText, TYPE_ZHI_WAN); }
	BOOL IsPushFlat(const char* sText) { return IsHasTheProcess(sText, TYPE_PUSH_FLAT); }
	BOOL IsCutAngle(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_ANGLE); }
	BOOL IsCutRoot(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_ROOT); }
	BOOL IsCutBer(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_BER); }
	BOOL IsKaiJiao(const char* sText) { return IsHasTheProcess(sText, TYPE_KAI_JIAO); }
	BOOL IsHeJiao(const char* sText) { return IsHasTheProcess(sText, TYPE_HE_JIAO); }
	BOOL IsFootNail(const char* sText) { return IsHasTheProcess(sText, TYPE_FOO_NAIL); }
public:
	//�ͻ�ID
	static const BYTE ID_AnHui_HongYuan		= 1;	//���պ�Դ(�ϵ�У��|�����ϵ�|DWGУ��|���¼ӹ���)
	static const BYTE ID_AnHui_TingYang		= 2;	//����͡��(�ϵ�У��|DWGУ��|���¼ӹ���|��������)
	static const BYTE ID_SiChuan_ChengDu	= 3;	//�е罨�ɶ�����(DWGУ��|���¼ӹ���)
	static const BYTE ID_JiangSu_HuaDian	= 4;	//���ջ���(�ϵ�У��)
	static const BYTE ID_ChengDu_DongFang	= 5;	//�ɶ�����(DWGУ��|���µ�����)
	static const BYTE ID_QingDao_HaoMai		= 6;	//�ൺ����(DWGУ��|���¼ӹ���)
	static const BYTE ID_QingDao_QLGJG		= 7;	//�ൺǿ���սṹ(DWGУ��|���¼ӹ���)
	static const BYTE ID_QingDao_ZAILI		= 8;	//�ൺ����(DWGУ��|���¼ӹ���|���»���|������ӡ)
	static const BYTE ID_WUZHOU_DINGYI		= 9;	//���޶���(�ϵ�У��|DWGУ��)
	static const BYTE ID_SHANDONG_HAUAN		= 10;	//ɽ������(������ӡ)
	UINT m_uiCustomizeSerial;
	CXhChar50 m_sCustomizeName;
	BOOL m_bExeRppWhenArxLoad;				//����Arx��ִ��rpp�����ʾ�Ի��� wht 20-04-24
	CXhChar200 m_sTMABomFileKeyStr;	//֧��ָ��TMA Bom�ļ����ؼ��֣�TMA��ERP Excel�ļ���ʽһ��ʱ��ͨ���ؼ��ֽ������� wht 20-04-29
	CXhChar200 m_sERPBomFileKeyStr;
	CXhChar200 m_sTMABomSheetName;	//֧��ָ�������Excel�ļ��е�Sheet���� wht 20-04-29
	CXhChar200 m_sERPBomSheetName;
	CXhChar200 m_sJGPrintSheetName;	//
	//
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
extern CBomModel g_xUbomModel;
#endif
