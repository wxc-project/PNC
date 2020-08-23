#pragma once
#include "list.h"
#include "f_ent_list.h"
#include "HashTable.h"
#include "XhCharString.h"
#include <vector>
using std::vector;
//////////////////////////////////////////////////////////////////////////
//�������ʶ����
struct ISymbolRecognizer {
	virtual bool IsHuoquLine(GELINE* pLine, DWORD cbFilterFlag = 0) = 0;
};
class CSymbolRecoginzer : public ISymbolRecognizer
{
	struct SYMBOL_ENTITY
	{
		BYTE ciSymbolType;	//0x01.������S�ͷ���
		CXhSimpleList<GELINE> listSonlines;
	};
	CXhSimpleList<SYMBOL_ENTITY> listSymbols;
public:
	void AppendSymbolEnt(AcDbSpline* pSpline);
	//ͨ����Ƿ���ʶ�������
	bool IsHuoquLine(GELINE* pLine,DWORD cbFilterFlag=0);
};
//////////////////////////////////////////////////////////////////////////
//�ְ����ͼʶ����
struct BOLT_BLOCK
{
	CXhChar50 sGroupName;	//��������
	CXhChar16 sBlockName;
	short diameter;
	double hole_d;
	BOLT_BLOCK() {
		Init("", "",0, 0);
	}
	BOLT_BLOCK(const char* name, short md=0, double holeD=0) {
		Init("", name, md, holeD);
	}
	BOLT_BLOCK(const char* groupName, const char* name, short md=0, double holeD=0){
		Init(groupName, name, md, holeD);
	}
	void Init(const char* groupName, const char* name, short md, double holeD) {
		sGroupName.Copy(groupName);
		sBlockName.Copy(name);
		diameter = md;
		hole_d = holeD;
	}
};
struct RECOG_SCHEMA{
	CXhChar50 m_sSchemaName;//ʶ��ģʽ����
	int m_iDimStyle;		//0.���б�ע 1.���б�ע
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//����
	CXhChar50 m_sReverseBendKey;//����
	BOOL m_bEditable;
	BOOL m_bEnable;
	RECOG_SCHEMA() {m_iDimStyle = 0; m_bEditable = FALSE; m_bEnable = FALSE; }
};
//
class CPlateExtractor
{
protected:
	RECOG_SCHEMA* InsertRecogSchema(char* name, int dimStyle, char* partNoKey,
		char* matKey, char* thickKey, char* partCountKey = NULL,
		char* frontBendKey = NULL, char* reverseBendKey = NULL);
public:
	int m_iDimStyle;		//0.���б�ע 1.���б�ע
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//����
	CXhChar50 m_sReverseBendKey;//����
	ATOM_LIST<RECOG_SCHEMA> m_recogSchemaList;
	CHashStrList<BOLT_BLOCK> hashBoltDList;
public:
	CPlateExtractor();
	virtual ~CPlateExtractor();
	//
	void ActiveRecogSchema(RECOG_SCHEMA *pSchema);
	int  GetPnKeyNum(const char* sText);
	int  GetKeyMemberNum();	//��ע��Ա��
	BOOL IsMatchPNRule(const char* sText);
	BOOL IsMatchThickRule(const char* sText);
	BOOL IsMatchMatRule(const char* sText);
	BOOL IsMatchNumRule(const char* sText);
	BOOL IsMatchBendRule(const char* sText);
	BOOL IsBriefMatMark(char cMat);
	//��������ʱ���ؽ���������ͣ������ų������Ӽ����� wht 19-07-22
	static const int PART_LABEL_EMPTY = 0;	//�ռ���
	static const int PART_LABEL_VALID = 1;	//���ü���
	static const int PART_LABEL_WELD  = 2;	//���Ӽ���
	BOOL ParsePartNoText(AcDbEntity *pAcadText, CXhChar16& sPartNo);
	BYTE ParsePartNoText(const char* sText,CXhChar16& sPartNo);
	void ParseThickText(const char* sText,int& nThick);
	void ParseMatText(const char* sText,char& cMat,char& cQuality);
	void ParseNumText(const char* sText,int& nNum);
	void ParseBendText(const char* sText,double &degree,BOOL &bFrontBend);
	//
	virtual void Init();
	virtual BOOL IsBendLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
	virtual BOOL IsSlopeLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
};
#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//�Ǹֹ��տ�ʶ����
class CJgCardExtractor
{
public:
	f2dRect part_no_rect;
	f2dRect mat_rect;
	f2dRect guige_rect;
	f2dRect length_rect;
	f2dRect piece_weight_rect;
	f2dRect danji_num_rect;
	f2dRect jiagong_num_rect;
	f2dRect note_rect;
	f2dRect sum_weight_rect;
	f2dRect cut_root_rect;
	f2dRect cut_ber_rect;
	f2dRect push_flat_rect;
	f2dRect weld_rect;
	f2dRect kai_jiao_rect;
	f2dRect he_jiao_rect;
	f2dRect cut_angle_rect;
	f2dRect cut_angle_SX_rect, cut_angle_EX_rect;
	f2dRect cut_angle_SY_rect, cut_angle_EY_rect;
	f2dRect huoqu_fst_rect, huoqu_sec_rect;
	f2dRect draw_rect;
	double fMaxX, fMaxY, fMinX, fMinY;
	double fTextHigh;
	double fPnDistX, fPnDistY;
public:
	CJgCardExtractor();
	~CJgCardExtractor();
	//
	static const int CARD_READ_FAIL = 0;
	static const int CARD_READ_SUCCEED = 1;
	static const int CARD_READ_ERROR_PARTNO = 2;	//���տ��д��ڶ��������Ҫ���ü��ű��� wht 20-05-08
	BYTE InitJgCardInfo(const char* sJgCardPath);
	f3dPoint GetJgCardOrigin(f3dPoint partNo_pt);
};
#endif