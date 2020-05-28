#pragma once

#include "IExtractor.h"
#include "list.h"
#include "f_ent_list.h"
#include "HashTable.h"
#include "SquareTree.h"
#include "ArrayList.h"

//////////////////////////////////////////////////////////////////////////
//特殊符号识别器
struct SYMBOL_ENTITY{
	BYTE ciSymbolType;	//0x01.火曲线S型符号
	CXhSimpleList<GELINE> listSonlines;
};
class CSymbolRecoginzer : public ISymbolRecognizer {
	CXhSimpleList<SYMBOL_ENTITY> listSymbols;
public:
	//
	void AppendSymbolEnt(AcDbSpline* pSpline);
	//通过标记符号识别火曲线
	virtual bool IsHuoquLine(GELINE* pLine,DWORD cbFilterFlag=0);
};

//////////////////////////////////////////////////////////////////////////
//开放给用户的钢板识别定制化接口函数
class CPlateObject
{
	POLYGON region;
public:
	struct VERTEX{
		GEPOINT pos;
		char ciEdgeType;	//1:普通直边 2:圆弧 3:椭圆弧
		bool m_bWeldEdge;
		bool m_bRollEdge;
		short manu_space;
		union ATTACH_DATA{	//简单附加数
			DWORD dwParam;
			long  lParam;
			void* pParam;
		}tag;
		struct ARC_PARAM{	//圆弧参数
			double radius;		//指定圆弧半径(椭圆需要)
			double fSectAngle;	//指定扇形角(圆弧需要)
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
	struct CAD_ENTITY{
		BYTE ciEntType;
		unsigned long idCadEnt;
		char sText[100];	//ciEntType==RELA_ACADENTITY::TYPE_TEXT时记录文本内容 wht 18-12-30
		GEPOINT pos;
		double m_fSize;
		CAD_ENTITY(long idEnt=0,BYTE ciType=0){idCadEnt=idEnt;ciEntType=ciType;strcpy(sText,"");m_fSize=0;}
	};
	ATOM_LIST<VERTEX> vertexList;
	CAD_ENTITY m_xMkDimPoint;	//钢板标注数据点 wht 19-03-02
protected:
	CHashList<CAD_ENTITY> m_xHashRelaEntIdList;	//钢板关联实体
	//
	virtual BOOL IsValidVertexs();
	virtual void ReverseVertexs();
	virtual void DeleteAssisstPts();
	virtual void UpdateVertexPropByArc(f3dArcLine& arcLine,int type);
	virtual void CreateRgn();
public:
	CPlateObject();
	~CPlateObject();
	//
	CAD_ENTITY *EnumFirstEnt(){return m_xHashRelaEntIdList.GetFirst();}
	CAD_ENTITY *EnumNextEnt(){return m_xHashRelaEntIdList.GetNext();}
	virtual bool IsInPlate(const double* poscoord);
	virtual bool IsInPlate(const double* start,const double* end);
	virtual BOOL RecogWeldLine(const double* ptS, const double* ptE);
	virtual BOOL RecogWeldLine(f3dLine slop_line);
};
//////////////////////////////////////////////////////////////////////////
struct BOLT_BLOCK
{
	CXhChar50 sGroupName;	//分组名称
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
	RECOG_SCHEMA() { id = 0; m_iDimStyle = 0; m_bEditable = FALSE; m_bEnable = FALSE; }
};
//////////////////////////////////////////////////////////////////////////
//开放给用户的识别定制化接口函数
class CPlateExtractor
{
public:
	ATOM_LIST<RECOG_SCHEMA> m_recogSchemaList;
	int m_iDimStyle;		//0.单行标注 1.多行标注
	CXhChar50 m_sPnKey;		//
	CXhChar50 m_sThickKey;	//
	CXhChar50 m_sMatKey;	//
	CXhChar50 m_sPnNumKey;	//
	CXhChar50 m_sFrontBendKey;	//正曲
	CXhChar50 m_sReverseBendKey;//反曲
	CHashStrList<BOLT_BLOCK> hashBoltDList;
	//特定图层
	CXhChar16 m_sBendLineLayer;		//火曲线图层
	CXhChar16 m_sSlopeLineLayer;	//坡口线图层
protected:
	int  GetPnKeyNum(const char* sText);
public:
	CPlateExtractor();
	virtual ~CPlateExtractor();
	//
	int GetKeyMemberNum();	//标注成员数
	BOOL IsMatchPNRule(const char* sText);
	BOOL IsMatchThickRule(const char* sText);
	BOOL IsMatchMatRule(const char* sText);
	BOOL IsMatchNumRule(const char* sText);
	BOOL IsMatchBendRule(const char* sText);
	BOOL IsBriefMatMark(char cMat);
	//解析件号时返回解析结果类型，用于排除焊接子件件号 wht 19-07-22
	static const int PART_LABEL_EMPTY = 0;	//空件号
	static const int PART_LABEL_VALID = 1;	//可用件号
	static const int PART_LABEL_WELD  = 2;	//焊接件号
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
	static const int MAX_BOLT_HOLE = 100;	//最大的螺栓孔径 wht 19-12-21
	virtual BOOL RecogBoltHole(AcDbEntity* pEnt,BOLT_HOLE& hole);
	virtual BOOL RecogBasicInfo(AcDbEntity* pEnt,BASIC_INFO& basicInfo);
	virtual BOOL RecogArcEdge(AcDbEntity* pEnt,f3dArcLine& arcLine,BYTE& ciEdgeType);
};
//////////////////////////////////////////////////////////////////////////
//开放给用户的角钢识别定制化接口函数
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
	static const int CARD_READ_ERROR_PARTNO = 2;	//工艺卡中存在多个件号需要配置件号标题 wht 20-05-08
	BYTE InitJgCardInfo(const char* sJgCardPath);
	f3dPoint GetJgCardOrigin(f3dPoint partNo_pt);
};