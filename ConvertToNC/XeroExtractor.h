#pragma once

#include "IExtractor.h"
#include "list.h"
#include "f_ent_list.h"
#include "HashTable.h"
#include "SquareTree.h"
#include "ArrayList.h"

//////////////////////////////////////////////////////////////////////////
//�������ʶ����
struct SYMBOL_ENTITY{
	BYTE ciSymbolType;	//0x01.������S�ͷ���
	CXhSimpleList<GELINE> listSonlines;
};
class CSymbolRecoginzer : public ISymbolRecognizer {
	CXhSimpleList<SYMBOL_ENTITY> listSymbols;
public:
	//
	void AppendSymbolEnt(AcDbSpline* pSpline);
	//ͨ����Ƿ���ʶ�������
	virtual bool IsHuoquLine(GELINE* pLine,DWORD cbFilterFlag=0);
};

//////////////////////////////////////////////////////////////////////////
//���Ÿ��û��ĸְ�ʶ���ƻ��ӿں���
enum ENTITY_TYPE {
	TYPE_OTHER = 0,
	TYPE_LINE,
	TYPE_ARC,
	TYPE_CIRCLE,
	TYPE_ELLIPSE,
	TYPE_SPLINE,
	TYPE_TEXT,
	TYPE_MTEXT,
	TYPE_BLOCKREF,
	TYPE_DIM_D
};
struct CAD_ENTITY {
	ENTITY_TYPE ciEntType;
	unsigned long idCadEnt;
	char sText[100];	//ciEntType==TYPE_TEXTʱ��¼�ı����� wht 18-12-30
	GEPOINT pos;
	double m_fSize;
	//
	CAD_ENTITY(ULONG idEnt = 0);
	bool IsInScope(GEPOINT &pt);
};
struct CAD_LINE : public CAD_ENTITY
{
	BYTE m_ciSerial;
	GEPOINT m_ptStart, m_ptEnd;
	GEPOINT vertex;
	BOOL m_bReverse;
	BOOL m_bMatch;
public:
	CAD_LINE(ULONG lineId = 0);
	CAD_LINE(AcDbObjectId id, double len);
	CAD_LINE(AcDbObjectId id, GEPOINT &start, GEPOINT &end) { Init(id, start, end); }
	void Init(AcDbObjectId id, GEPOINT &start, GEPOINT &end);
	BOOL UpdatePos();
	//
	static int compare_func(const CAD_LINE& obj1, const CAD_LINE& obj2)
	{
		if (obj1.m_bMatch && !obj2.m_bMatch)
			return -1;
		else if (!obj1.m_bMatch && obj2.m_bMatch)
			return 1;
		else
		{
			if (obj1.m_ciSerial > obj2.m_ciSerial)
				return 1;
			else if (obj1.m_ciSerial < obj2.m_ciSerial)
				return -1;
			else
				return 0;
		}
	}
};
//////////////////////////////////////////////////////////////////////////
//���Ÿ��û��ĸְ�ʶ���ƻ��ӿں���
//CPlateObject
class CPlateObject
{
	POLYGON region;
public:
	struct CIR_PLATE {
		BOOL m_bCirclePlate;	//�Ƿ�ΪԲ�Ͱ�
		GEPOINT cir_center;
		GEPOINT norm, column_norm;
		double m_fRadius;
		double m_fInnerR;
		CIR_PLATE() {
			m_bCirclePlate = FALSE;
			m_fRadius = m_fInnerR = 0;
			norm.Set(0, 0, 1);
			column_norm.Set(0, 0, 1);
		}
	}cir_plate_para;
	struct VERTEX{
		GEPOINT pos;
		char ciEdgeType;	//1:��ֱͨ�� 2:Բ�� 3:��Բ��
		bool m_bWeldEdge;
		bool m_bRollEdge;
		short manu_space;
		union ATTACH_DATA{	//�򵥸�����
			DWORD dwParam;
			long  lParam;
			void* pParam;
		}tag;
		struct ARC_PARAM{	//Բ������
			double radius;		//ָ��Բ���뾶(��Բ��Ҫ)
			double fSectAngle;	//ָ�����ν�(Բ����Ҫ)
			GEPOINT center,work_norm,column_norm;
		}arc;
		VERTEX(){
			ciEdgeType=1;
			m_bWeldEdge=m_bRollEdge=false;
			manu_space=0;
			arc.radius = arc.fSectAngle = 0;
			tag.dwParam = 0;
		}
	};
	ATOM_LIST<VERTEX> vertexList;
	CAD_ENTITY m_xMkDimPoint;	//�ְ��ע���ݵ� wht 19-03-02
protected:
	virtual BOOL IsValidVertexs();
	virtual void ReverseVertexs();
	virtual void DeleteAssisstPts();
	virtual void UpdateVertexPropByArc(f3dArcLine& arcLine,int type);
	virtual void CreateRgn();
public:
	CPlateObject();
	~CPlateObject();
	//
	virtual bool IsInPlate(const double* poscoord);
	virtual bool IsInPlate(const double* start,const double* end);
	virtual BOOL RecogWeldLine(const double* ptS, const double* ptE);
	virtual BOOL RecogWeldLine(f3dLine slop_line);
	virtual BOOL IsValid() { return vertexList.GetNodeNum() >= 3; }
	virtual BOOL IsClose(int* pIndex = NULL);
};
//////////////////////////////////////////////////////////////////////////
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
	int id;
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
	RECOG_SCHEMA() { id = 0; m_iDimStyle = 0; m_bEditable = FALSE; m_bEnable = FALSE; }
};
//////////////////////////////////////////////////////////////////////////
//���Ÿ��û���ʶ���ƻ��ӿں���
class CPlateExtractor
{
public:
	ATOM_LIST<RECOG_SCHEMA> m_recogSchemaList;
	int m_iDimStyle;		//0.���б�ע 1.���б�ע
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//����
	CXhChar50 m_sReverseBendKey;//����
	CHashStrList<BOLT_BLOCK> hashBoltDList;
	//�ض�ͼ��
	CXhChar16 m_sBendLineLayer;		//������ͼ��
	CXhChar16 m_sSlopeLineLayer;	//�¿���ͼ��
protected:
	int  GetPnKeyNum(const char* sText);
public:
	CPlateExtractor();
	virtual ~CPlateExtractor();
	//
	int GetKeyMemberNum();	//��ע��Ա��
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
	virtual void Init();
	//
	virtual BOOL IsBendLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
	virtual BOOL IsSlopeLine(AcDbLine* pAcDbLine, ISymbolRecognizer* pRecognizer = NULL);
	virtual BOOL RecogBoltHole(AcDbEntity* pEnt,BOLT_HOLE& hole);
	virtual BOOL RecogBasicInfo(AcDbEntity* pEnt,BASIC_INFO& basicInfo);
	virtual BOOL RecogArcEdge(AcDbEntity* pEnt,f3dArcLine& arcLine,BYTE& ciEdgeType);
};
//////////////////////////////////////////////////////////////////////////
//���Ÿ��û��ĽǸ�ʶ���ƻ��ӿں���
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