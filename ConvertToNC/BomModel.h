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
	static const BYTE TYPE_JG = 1;		//�Ǹ�
	static const BYTE TYPE_YG = 2;		//Բ��
	static const BYTE TYPE_TUBE = 3;	//�ֹ�
	static const BYTE TYPE_FLAT = 4;	//����
	static const BYTE TYPE_JIG = 5;		//�о�
	static const BYTE TYPE_GGS = 6;		//�ָ�դ
	static const BYTE TYPE_PLATE = 7;	//�ְ�
	static const BYTE TYPE_WIRE_TUBE = 8;//�׹�
	BYTE m_ciType;				//
	//
	PART_ANGLE m_xAngle;
	AcDbObjectId keyId;
	AcDbObjectId partNumId;		//�ӹ���ID
	AcDbObjectId singleNumId;	//������ID
	AcDbObjectId sumWeightId;	//����ID
	CXhChar200 m_sTowerType;
	//�ӹ������������������޸�״̬ wht 20-07-29
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
	CBomFile m_xPrintBomFile;	//DWG�ļ���Ӧ�Ĵ�ӡ�嵥 wht 20-07-21
public:
	CXhChar100 m_sDwgName;
	CXhChar500 m_sFileName;	//�ļ�����
	BOOL RetrieveAngles(BOOL bSupportSelectEnts =FALSE);
	BOOL RetrievePlates(BOOL bSupportSelectEnts =FALSE);
protected:
	int GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet);
public:
	CDwgFileInfo();
	~CDwgFileInfo();
	//�Ǹ�DWG����
	int GetJgNum(){return m_hashJgInfo.GetNodeNum();}
	void EmptyJgList() { m_hashJgInfo.Empty(); }
	CAngleProcessInfo* EnumFirstJg(){return m_hashJgInfo.GetFirst();}
	CAngleProcessInfo* EnumNextJg(){return m_hashJgInfo.GetNext();}
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
	void ModifyAngleDwgPartNum();
	void ModifyAngleDwgSingleNum();
	void ModifyAngleDwgSumWeight();
	//�ְ�DWG����
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
	static const int COMPARE_ANGLE_DWG = 2;
	static const int COMPARE_PLATE_DWG = 3;
	static const int COMPARE_ANGLE_DWGS = 4;
	static const int COMPARE_PLATE_DWGS = 5;
	static const int COMPARE_PARTS_DWG = 6;	//�Ǹְָ���һ��dwg�ļ���
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
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf,BOOL bExtractPart);
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
	//����ģ��
	static const BYTE FUNC_BOM_COMPARE		 = 1;	//0X01�ϵ�У��
	static const BYTE FUNC_BOM_AMEND		 = 2;	//0X02�����ϵ�
	static const BYTE FUNC_DWG_COMPARE		 = 3;	//0X04DWG����У��
	static const BYTE FUNC_DWG_AMEND_SUM_NUM = 4;	//0X08�����ӹ���
	static const BYTE FUNC_DWG_AMEND_WEIGHT	 = 5;	//0X10��������
	static const BYTE FUNC_DWG_AMEND_SING_N  = 6;	//0X20����������
	static const BYTE FUNC_DWG_BATCH_PRINT	 = 7;	//0x40������ӡ wht 20-05-26
	DWORD m_dwFunctionFlag;
	
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
	CBomImportCfg m_xBomImoprtCfg;
	CXhChar500 m_sNotPrintFilter;		//֧������������ӡʱ����Ҫ��ӡ�Ĺ��� wht 20-07-27
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
	BOOL m_bExtractPltesWhenOpenFile;		//�򿪸ְ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
	BOOL m_bExtractAnglesWhenOpenFile;		//�򿪽Ǹ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
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
