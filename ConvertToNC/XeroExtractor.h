#pragma once
#include "list.h"
#include "f_ent_list.h"
#include "HashTable.h"
#include "XhCharString.h"
#include "BoltBlockRecog.h"
#include "XeroNcPart.h"
#include "common.h"
//////////////////////////////////////////////////////////////////////////
//
class IExtractor
{
public:
	static BOOL m_bSendCommand;
	static const float ASSIST_RADIUS;
	static const float DIST_ERROR;
	static const float WELD_MAX_HEIGHT;
	static const BYTE PLATE = 1;
	static const BYTE ANGLE = 2;
	BYTE m_ciType;
	PROJECT_INFO m_xPrjInfo;
	CHashStrList<CBoltEntGroup> m_xBoltEntHash;
public:
	static CString MakePosKeyStr(GEPOINT pos) {
		CString sKeyStr;
		sKeyStr.Format("X%d-Y%d", ftoi(pos.x), ftoi(pos.y));
		return sKeyStr;
	}
	CBoltEntGroup* FindBoltGroup(const char* sKey) {
		return m_xBoltEntHash.GetValue(sKey);
	}
	CBoltEntGroup* EnumFirstBoltGroup() {
		return m_xBoltEntHash.GetFirst();
	}
	CBoltEntGroup* EnumNextBoltGroup() {
		return m_xBoltEntHash.GetNext();
	}
	//
	virtual bool ExtractPlates(CHashStrList<CPlateProcessInfo>& hashPlateInfo, BOOL bSupportSelectEnts) = 0;
#ifdef __UBOM_ONLY_
	virtual bool ExtractAngles(CHashList<CAngleProcessInfo>& hashJgInfo, BOOL bSupportSelectEnts) = 0;
#endif
};
class CExtractorLife
{
	ATOM_LIST<IExtractor*> m_listExtractor;
public:
	CExtractorLife();
	~CExtractorLife();
	//
	void Append(IExtractor* pExtracot);
	IExtractor* GetExtractor(BYTE ciType);
};
//////////////////////////////////////////////////////////////////////////
//特殊符号识别器
struct ISymbolRecognizer {
	virtual bool IsHuoquLine(GELINE* pLine, DWORD cbFilterFlag = 0) = 0;
};
class CSymbolRecoginzer : public ISymbolRecognizer
{
	struct SYMBOL_ENTITY
	{
		BYTE ciSymbolType;	//0x01.火曲线S型符号
		CXhSimpleList<GELINE> listSonlines;
	};
	CXhSimpleList<SYMBOL_ENTITY> listSymbols;
public:
	void AppendSymbolEnt(AcDbSpline* pSpline);
	//通过标记符号识别火曲线
	bool IsHuoquLine(GELINE* pLine,DWORD cbFilterFlag=0);
};
//////////////////////////////////////////////////////////////////////////
//钢板大样图识别规则
struct RECOG_SCHEMA{
	CXhChar50 m_sSchemaName;//识别模式名称
	int m_iDimStyle;		//0.单行标注 1.多行标注
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//正曲
	CXhChar50 m_sReverseBendKey;//反曲
	BOOL m_bEditable;
	BOOL m_bEnable;
	RECOG_SCHEMA() {m_iDimStyle = 0; m_bEditable = FALSE; m_bEnable = FALSE; }
};
//
class CPlateRecogRule
{
protected:
	RECOG_SCHEMA* InsertRecogSchema(char* name, int dimStyle, char* partNoKey,
		char* matKey, char* thickKey, char* partCountKey = NULL,
		char* frontBendKey = NULL, char* reverseBendKey = NULL);
	CBoltBlockRecog m_xBoltBlockRecog;
public:
	int m_iDimStyle;		//0.单行标注 1.多行标注
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//正曲
	CXhChar50 m_sReverseBendKey;//反曲
	ATOM_LIST<RECOG_SCHEMA> m_recogSchemaList;
public:
	CPlateRecogRule();
	virtual ~CPlateRecogRule();
	//
	void ActiveRecogSchema(RECOG_SCHEMA *pSchema);
	int  GetPnKeyNum(const char* sText);
	int  GetKeyMemberNum();	//标注成员数
	BOOL IsMatchPNRule(const char* sText);
	BOOL IsMatchThickRule(const char* sText);
	BOOL IsMatchMatRule(const char* sText);
	BOOL IsMatchNumRule(const char* sText);
	BOOL IsMatchBendRule(const char* sText);
	BOOL IsMatchRollEdgeRule(const char* sText);
	BOOL IsBriefMatMark(char cMat);
	//解析件号时返回解析结果类型，用于排除焊接子件件号 wht 19-07-22
	static const int PART_LABEL_EMPTY = 0;	//空件号
	static const int PART_LABEL_VALID = 1;	//可用件号
	static const int PART_LABEL_WELD  = 2;	//焊接件号
	void SplitMultiText(AcDbEntity* pEnt, vector<CString>& textArr);
	BOOL ParsePartNoText(AcDbEntity *pAcadText, CXhChar16& sPartNo);
	BYTE ParsePartNoText(const char* sText,CXhChar16& sPartNo);
	void ParseThickText(const char* sText,int& nThick);
	void ParseMatText(const char* sText,char& cMat,char& cQuality);
	void ParseNumText(const char* sText,int& nNum);
	bool ParseBendText(const char* sText,double &degree,BOOL &bFrontBend);
	bool ParseRollEdgeText(const char* sText, double& degree, BOOL& bFrontBend);
	//
	virtual void Init();
	virtual BOOL IsBendLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
	virtual BOOL IsSlopeLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
	//
	BOLT_BLOCK* GetBlotBlockByName(const char* sBlockName) { return m_xBoltBlockRecog.FromName(sBlockName); }
	BOLT_BLOCK* AddBoltBlock(const char* sBlockName) { return m_xBoltBlockRecog.Add(sBlockName); }
	BOLT_BLOCK *EnumFirstBlotBlock() { return m_xBoltBlockRecog.EnumFirst(); }
	BOLT_BLOCK *EnumNextBlotBlock() { return m_xBoltBlockRecog.EnumNext(); }
	CXhChar50 GetBoltBlockCurKey() { return m_xBoltBlockRecog.GetCurKey(); }
	BOLT_BLOCK *ModifyBoltBlockKeyStr(const char *oldkey, const char *newkey) { return m_xBoltBlockRecog.ModifyKeyStr(oldkey, newkey); }
	BOOL DeleteBoltBlockByName(const char* sName) { return m_xBoltBlockRecog.DeleteBoltBlockByName(sName); }
	void EmptyBoltBlockRecog() { m_xBoltBlockRecog.Empty(); }
	void CleanBoltBlockRecog() { m_xBoltBlockRecog.Clean(); }
	void DeleteCurBoltBlock(BOOL bClean=FALSE) { return m_xBoltBlockRecog.DeleteCursor(bClean); }
};
#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//角钢工艺卡识别规则
class CJgCardRecogRule
{
public:
	map<long, f2dRect> mapJgCardRect;	//数据点区域
	f2dRect draw_rect;	//绘图区域
	f2dRect frame_rect;	//图框区域
	double fTextHigh;
	double fPnDistX, fPnDistY;
public:
	CJgCardRecogRule();
	~CJgCardRecogRule();
	//
	static const int CARD_READ_FAIL = 0;
	static const int CARD_READ_SUCCEED = 1;
	static const int CARD_READ_ERROR_PARTNO = 2;	//工艺卡中存在多个件号需要配置件号标题 wht 20-05-08
	BYTE InitJgCardInfo(const char* sJgCardPath);
	f3dPoint GetJgCardOrigin(f3dPoint partNo_pt);
};
#endif

extern CExtractorLife g_xExtractorLife;
