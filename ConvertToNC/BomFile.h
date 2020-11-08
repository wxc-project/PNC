#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "BomTblTitleCfg.h"
#include "..\..\LDS\LDS\BOM\BOM.h"
#include <vector>
using std::vector;

#ifdef __UBOM_ONLY_
struct BOM_FILE_CFG
{
public:
	CXhChar200 m_sFileTypeKeyStr;		//支持指定TMA Bom文件名关键字，TMA与ERP Excel文件格式一样时可通过关键字进行区分 wht 20-04-29
	CXhChar200 m_sBomSheetName;			//支持指定导入的Excel文件中的Sheet名称 wht 20-04-29
	CBomTblTitleCfg m_xTblCfg;			//需要导入的Sheet表头设置
	static const int MAX_SHEET_COUNT = 10;
	CBomTblTitleCfg m_xArrTblCfg[10];	//TMA_BOM导入多个sheet且Sheet格式不同时使用，关键字 TMA_BOM_1,TMA_BOM_2... wht 20-07-22
	BOOL IsCurFormat(const char* file_path, BOOL bDisplayMsgBox);
	BOOL IsValid() {
		if (m_xTblCfg.m_sColIndexArr.GetLength() > 0)
			return TRUE;
		else
			return FALSE;
	}
};
//CBomConfig
class CBomConfig
{
	bool ExtractAngleCompareItems();
	bool ExtractPlateCompareItems();
public:
	const static char* KEY_PN;			//= "PartNo";
	const static char* KEY_MAT;			//= "Material";
	const static char* KEY_SPEC;		//= "Spec";
	const static char* KEY_LEN;			//= "Length";
	const static char* KEY_WIDE;		//= "Width";
	const static char* KEY_TA_NUM;		//= "TaNum";
	const static char* KEY_SING_N;		//= "SingNum";
	const static char* KEY_MANU_N;		//= "ManuNum"
	const static char* KEY_SING_W;		//= "SingWeight"
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
	//工艺描述
	const static BYTE TYPE_ZHI_WAN		= 0;
	const static BYTE TYPE_CUT_ANGLE	= 1;
	const static BYTE TYPE_CUT_ROOT		= 2;
	const static BYTE TYPE_CUT_BER		= 3;
	const static BYTE TYPE_PUSH_FLAT	= 4;
	const static BYTE TYPE_KAI_JIAO		= 5;
	const static BYTE TYPE_HE_JIAO		= 6;
	const static BYTE TYPE_FOO_NAIL		= 7;
	CXhChar100 m_sProcessDescArr[8];
	CXhChar100 m_sProcessFlag;	//常见标识有"*、v、V、√、1",标识有该工艺 wht 20-08-17
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
	BOM_FILE_CFG m_xTmaTblCfg;
	BOM_FILE_CFG m_xErpTblCfg;
	BOM_FILE_CFG m_xPrintTblCfg;
	CXhChar500 m_sAngleCompareItemArr;	//角钢校审项
	CXhChar500 m_sPlateCompareItemArr;	//钢板校审项
	CHashStrList<BOOL> hashCompareItemOfAngle;
	CHashStrList<BOOL> hashCompareItemOfPlate;
public:
	CBomConfig(void);
	~CBomConfig(void);
	//
	void Init();
	int InitBomTitle();
	//
	BOOL IsEqualDefaultProcessFlag(const char* sValue);
	BOOL IsEqualProcessFlag(const char* sValue);
	BOOL IsHasTheProcess(const char* sText, BYTE ciType);
	BOOL IsZhiWan(const char* sText) { return IsHasTheProcess(sText, TYPE_ZHI_WAN); }
	BOOL IsPushFlat(const char* sText) { return IsHasTheProcess(sText, TYPE_PUSH_FLAT); }
	BOOL IsCutAngle(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_ANGLE); }
	BOOL IsCutRoot(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_ROOT); }
	BOOL IsCutBer(const char* sText) { return IsHasTheProcess(sText, TYPE_CUT_BER); }
	BOOL IsKaiJiao(const char* sText) { return IsHasTheProcess(sText, TYPE_KAI_JIAO); }
	BOOL IsHeJiao(const char* sText) { return IsHasTheProcess(sText, TYPE_HE_JIAO); }
	BOOL IsFootNail(const char* sText) { return IsHasTheProcess(sText, TYPE_FOO_NAIL); }
	BOOL IsTmaBomFile(const char* sFilePath, BOOL bDisplayMsgBox = FALSE);
	BOOL IsErpBomFile(const char* sFilePath, BOOL bDisplayMsgBox = FALSE);
	BOOL IsPrintBomFile(const char* sFilePath, BOOL bDisplayMsgBox = FALSE);
	BOOL IsAngleCompareItem(const char* title) { return (hashCompareItemOfAngle.GetValue(title)!=NULL); }
	BOOL IsPlateCompareItem(const char* title) { return (hashCompareItemOfPlate.GetValue(title) != NULL); }
	size_t GetBomTitleCount() { return m_xBomTitleArr.size(); }
	BOOL IsTitleCol(int index, const char*title);
	CXhChar16 GetTitleKey(int index);
	CXhChar16 GetTitleName(int index);
	int GetTitleWidth(int index);
};
//////////////////////////////////////////////////////////////////////////
//CBomFile
class CBomFile
{
	void* m_pBelongObj;
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
	void Empty() {
		m_hashPartByPartNo.Empty();
		m_sFileName.Empty();
	}
	void SetBelongModel(void *pBelongObj) { m_pBelongObj = pBelongObj; }
	void* BelongModel() const { return m_pBelongObj; }
	int GetPartNum() { return m_hashPartByPartNo.GetNodeNum(); }
	BOMPART* EnumFirstPart() { return m_hashPartByPartNo.GetFirst(); }
	BOMPART* EnumNextPart() { return m_hashPartByPartNo.GetNext(); }
	BOMPART* FindPart(const char* sKey) { return m_hashPartByPartNo.GetValue(sKey); }
	//
	BOOL ImportExcelFile(BOM_FILE_CFG *pTblCfg);
	CString GetPartNumStr();
	void UpdateProcessPart(const char* sOldKey, const char* sNewKey);
};

extern CBomConfig g_xBomCfg;
#endif
