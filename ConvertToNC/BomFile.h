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
		int m_iCfgCol;
		int m_nWidth;	//标题宽度
		//
		BOM_TITLE(int iCfgCol, int nWidth)
		{
			m_iCfgCol = iCfgCol;
			m_nWidth = nWidth;
		}
	};
	BOM_FILE_CFG m_xTmaTblCfg;
	BOM_FILE_CFG m_xErpTblCfg;
	BOM_FILE_CFG m_xPrintTblCfg;
	CXhChar500 m_sAngleCompareItemArr;	//角钢校审项
	CXhChar500 m_sPlateCompareItemArr;	//钢板校审项
	map<int,BOOL> hashCompareItemOfAngle;
	map<int,BOOL> hashCompareItemOfPlate;
	vector<BOM_TITLE> m_xBomTitleArr;
public:
	CBomConfig(void);
	~CBomConfig(void);
	//
	void Init();
	int InitBomTitle();
	size_t GetBomTitleCount() { return m_xBomTitleArr.size(); }
	BOOL IsTitleCol(int index, int iCfgCol);
	CXhChar16 GetTitleName(int index);
	CXhChar16 GetCfgColName(int iCfgCol);
	int GetTitleWidth(int index);
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
	BOOL IsAngleCompareItem(int iCol) { return hashCompareItemOfAngle[iCol]; }
	BOOL IsPlateCompareItem(int iCol) { return hashCompareItemOfPlate[iCol]; }
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
