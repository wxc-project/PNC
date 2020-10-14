#pragma once
#include "f_ent.h"
#include "f_ent_list.h"
#include "adsdef.h"
#include "XhCharString.h"
#include "BomModel.h"

#ifdef __UBOM_ONLY_
struct PLOT_CFG
{
public:
	CXhChar500 m_sDeviceName;
	CXhChar500 m_sPaperSize;
	BYTE m_ciPaperRot;			//ͼֽ����:'L'����'P'����
	CXhChar500 m_sFileNameTemplate;
	PLOT_CFG() { m_ciPaperRot = 'L'; }
	const static char *TOWER_TYPE;		// = "����";
	const static char *PART_LABEL;		// = "����";
	const static char *MAT_MARK;		// = "����";
	const static char *MAT_BRIEF_MARK;	// = "���ַ�";
	const static char *PRE_TOWER_COUNT;	// = "������";
	const static char *MANU_COUNT;		// = "�ӹ���";
	CXhChar500 GetPngFileNameByTemplate(const char* sTowerType, const char* sPartLabel, 
										const char* sMaterial, char cMatBriefMark, 
										int nNumPreTower, int nManuNum);
};

class CPrintSet
{
public:
	static CString GetPrintDeviceCmbItemStr();
	static CString GetPaperSizeCmbItemStr(const char* sDeviceName);
	static CXhChar500 GetPlotCfgName(bool bPromptInfo);
	static BOOL SetPlotMedia(PLOT_CFG* pPlotCfg, bool bPromptInfo);
};

struct PRINT_SCOPE
{
	CAngleProcessInfo *m_pAngle;
	CPlateProcessInfo *m_pPlate;
public:
	ads_point L_T;
	ads_point R_B;
	CXhChar200 m_sTowerType;
	CXhChar100 m_sPartLabel;
	CXhChar16 m_sMaterial;
	char m_cMatBriefMark;
	int m_nNumPreTower;
	int m_nManuNum;

	const static int MARGIN = 1;
	PRINT_SCOPE();
	PRINT_SCOPE(SCOPE_STRU scope, const char* sLabel, char cMatBriefMark,
				const char* sTowerType,const char* sMat,int numPreTower,int manuNum);
	PRINT_SCOPE(f2dRect rect, const char* sLabel, char cMatBriefMark,
		const char* sTowerType, const char* sMat, int numPreTower, int manuNum);
	void Init(SCOPE_STRU scope, const char* sLabel, char cMatBriefMark,
		const char* sTowerType, const char* sMat, int numPreTower, int manuNum);
	void Init(f2dRect rect, const char* sLabel, char cMatBriefMark,
		const char* sTowerType, const char* sMat, int numPreTower, int manuNum);
	void Init(CAngleProcessInfo *pAngle);
	void Init(CPlateProcessInfo *pPlate);
	const static BYTE INC_MAT_NONE		= 0;
	const static BYTE INC_MAT_PREFIX	= 1;
	const static BYTE INC_MAT_POSTFIX	= 2;
	CXhChar500 GetPartFileName(PLOT_CFG *pPlotCfg,const char* extension);
	SCOPE_STRU GetCadEntScope();
};


class CBatchPrint
{
private:
	bool m_bSendCmd;
	BYTE m_ciPrintType;
	bool m_bEmptyFiles;
	ATOM_LIST<PRINT_SCOPE> *m_pPrintScopeList;
	ATOM_LIST<PRINT_SCOPE> m_xPrintScopeList;
protected:
	bool PrintProcessCardToPNG();
	bool PrintProcessCardToPDF();
	bool PrintProcessCardToPaper();
	bool RunPrint(PRINT_SCOPE *pScope, PLOT_CFG* pPlotCfg, const char* file_path = NULL);
public:
	const static BYTE PRINT_TYPE_PAPER	= 1;
	const static BYTE PRINT_TYPE_PNG	= 2;
	const static BYTE PRINT_TYPE_PDF	= 3;
	CBatchPrint(ATOM_LIST<PRINT_SCOPE> *pPrintScopeList, bool bSendCmd, BYTE ciPrintType, bool bEmptyFiles = FALSE);
	bool Print();

public:
	static PLOT_CFG m_xPdfPlotCfg;
	static PLOT_CFG m_xPngPlotCfg;
	static PLOT_CFG m_xPaperPlotCfg;
};
#endif