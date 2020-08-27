#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "BomFile.h"
#include "PNCModel.h"

#ifdef __UBOM_ONLY_
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
	void SetOrig(f3dPoint pt)
	{
		orig_pt=pt;
	}
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
	//
	void SetBelongModel(CProjectTowerType *pProject) { m_pProject = pProject; }
	CProjectTowerType* BelongModel() const { return m_pProject; }
	//�Ǹ�DWG����
	int GetAngleNum(){return m_hashJgInfo.GetNodeNum();}
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
	void InitBomInfo(const char* sFileName,BOOL bLoftBom);
	CDwgFileInfo* AppendDwgBomInfo(const char* sFileName);
	CDwgFileInfo* FindDwgBomInfo(const char* sFileName);
	//У�����
	int CompareOrgAndLoftParts();
	int CompareLoftAndPartDwgs();
	int CompareLoftAndPartDwg(const char* sFileName);
	void ExportCompareResult(int iCompare);
	//У��������
	DWORD GetResultCount(){return m_hashCompareResultByPartNo.GetNodeNum();}
	COMPARE_PART_RESULT* GetResult(const char* part_no){return m_hashCompareResultByPartNo.GetValue(part_no);}
	COMPARE_PART_RESULT* EnumFirstResult(){return m_hashCompareResultByPartNo.GetFirst();}
	COMPARE_PART_RESULT* EnumNextResult(){return m_hashCompareResultByPartNo.GetNext();}
};
//////////////////////////////////////////////////////////////////////////
//CBomModel
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
	ID_OTHER = 100,
};
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
	//���ƿͻ�
	UINT m_uiCustomizeSerial;
	CXhChar50 m_sCustomizeName;
	//���ò���
	double m_fMaxLenErr;				//����������ֵ
	BOOL m_bCmpQualityLevel;			//�����ȼ�У��
	BOOL m_bEqualH_h;					//Q345�Ƿ����Q355
	BOOL m_bExeRppWhenArxLoad;			//����Arx��ִ��rpp�����ʾ�Ի��� wht 20-04-24
	BOOL m_bExtractPltesWhenOpenFile;	//�򿪸ְ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
	BOOL m_bExtractAnglesWhenOpenFile;	//�򿪽Ǹ��ļ���ִ����ȡ����,Ĭ��ΪTRUE wht 20-07-29
	CXhChar100 m_sJgCadName;			//�Ǹֹ��տ�����
	CXhChar50 m_sJgCadPartLabel;		//�Ǹֹ��տ��еļ��ű���
	CXhChar50 m_sJgCardBlockName;		//�Ǹֹ��տ������� wht 19-09-24
	CXhChar500 m_sNotPrintFilter;		//֧������������ӡʱ����Ҫ��ӡ�Ĺ��� wht 20-07-27
	BYTE m_ciPrintSortType;				//0.���ϵ�����|1.����������
	//���ݴ洢
	CHashListEx<CProjectTowerType> m_xPrjTowerTypeList;
public:
	CBomModel(void);
	~CBomModel(void);
	//
	bool IsValidFunc(int iFuncType);
	DWORD AddFuncType(int iFuncType);
	BOOL IsJgCardBlockName(const char* sBlockName);
	BOOL IsPartLabelTitle(const char* sText);
	BOOL IsNeedPrint(BOMPART *pPart, const char* sNotes);
	//
	static CXhChar16 QueryMatMarkIncQuality(BOMPART *pPart);
};
extern CBomModel g_xUbomModel;
//
void ImportUbomConfigFile();
void ExportUbomConfigFile();
#endif
