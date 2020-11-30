#pragma once
#include "XeroExtractor.h"
#include "ProcessPart.h"
#include "ArrayList.h"
#include "HashTable.h"
#include "XhCharString.h"
#include "..\..\LDS\LDS\BOM\BOM.h"

#define  LEN_SCALE		0.8
#define  MIN_DISTANCE	25
//////////////////////////////////////////////////////////////////////////
//
enum ENTITY_TYPE {
	TYPE_OTHER = 0,
	TYPE_LINE,
	TYPE_ARC,
	TYPE_CIRCLE,
	TYPE_ELLIPSE,
	TYPE_SPLINE,
	TYPE_POLYLINE,
	TYPE_TEXT,
	TYPE_MTEXT,
	TYPE_BLOCKREF,
	TYPE_DIM_D
};
struct CAD_ENTITY {
	ENTITY_TYPE ciEntType;
	unsigned long idCadEnt;
	char sText[100];	//ciEntType==TYPE_TEXT时记录文本内容 wht 18-12-30
	GEPOINT pos;
	double m_fSize;
	//
	CAD_ENTITY(ULONG idEnt = 0);
	bool IsInScope(GEPOINT &pt);
	//进行排序，方便STL的使用
	bool friend operator <(const CAD_ENTITY &item1, const CAD_ENTITY &item2){
		return item1.idCadEnt > item2.idCadEnt;
	}
};
typedef CAD_ENTITY* CAD_ENT_PTR;
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
class CBoltEntGroup
{
public:
	static const BYTE BOLT_BLOCK = 0;
	static const BYTE BOLT_CIRCLE = 1;
	static const BYTE BOLT_TRIANGLE = 2;
	static const BYTE BOLT_SQUARE = 3;
	static const BYTE BOLT_WAIST_ROUND = 4;
	BYTE m_ciType;		//0.图块|1.圆圈|2.三角形|3.正方形|4.腰圆孔
	ULONG m_idEnt;
	BOOL m_bMatch;		//是否匹配到钢板
	float m_fPosX;
	float m_fPosY;
	float m_fHoleD;
	vector<ULONG> m_xLineArr;
	vector<ULONG> m_xCirArr;
	vector<ULONG> m_xArcArr;
public:
	CBoltEntGroup();
	//
	double GetWaistLen();
	GEPOINT GetWaistVec();
};
struct BASIC_INFO {
	char m_cMat;
	char m_cQuality;			//质量等级
	int m_nThick;
	int m_nNum;
	long m_idCadEntNum;
	CXhChar16 m_sPartNo;
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar50 m_sTaType;		//塔型
	CXhChar100 m_sPrjCode;		//工程编号
	BASIC_INFO() { m_nThick = m_nNum = 0; m_cMat = 0; m_cQuality = 0; m_idCadEntNum = 0; }
};
struct PROJECT_INFO
{
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaNum;			//基数
	CXhChar50 m_sTaAlias;		//代号（塔规格）
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar100 m_sTaskNo;		//任务号
	CXhChar100 m_sContractNo;	//合同号
	CXhChar100 m_sMatStandard;	//材料标准
};
//////////////////////////////////////////////////////////////////////////
//CCadPartObject
class CCadPartObject {
	CHashList<CAD_ENTITY> m_xHashRelaEntIdList;
protected:
	POLYGON m_xPolygon;
	//
	void AppendRelaEntity(long idEnt);
	void AppendRelaEntity(AcDbEntity *pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList = NULL);
	void ExtractRelaEnts(vector<GEPOINT>& ptArr);
public:
	CCadPartObject();
	~CCadPartObject();
	//
	void EraseRelaEnts();	//删除DWG文件中的图元
	void EmptyRelaEnts() { m_xHashRelaEntIdList.Empty(); }
	int GetRelaEntCount() { return m_xHashRelaEntIdList.GetNodeNum(); }
	CAD_ENTITY* EnumFirstRelaEnt() { return m_xHashRelaEntIdList.GetFirst(); }
	CAD_ENTITY* EnumNextRelaEnt() { return m_xHashRelaEntIdList.GetNext(); }
	CAD_ENTITY* AddRelaEnt(long hKey, CAD_ENTITY ent) {
		return m_xHashRelaEntIdList.SetValue(hKey, ent);
	}
	bool IsInPartRgn(const double* poscoord);
	bool IsInPartRgn(const double* start, const double* end);
	//
	virtual void CreateRgn() = 0;
	virtual void ExtractRelaEnts() = 0;
};
//////////////////////////////////////////////////////////////////////////
//CPlateObject
class CPlateObject
{
public:
	struct CIR_PLATE {
		BOOL m_bCirclePlate;	//是否为圆型板
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
	struct VERTEX {
		GEPOINT pos;
		char ciEdgeType;	//1:普通直边 2:圆弧 3:椭圆弧
		bool m_bWeldEdge;
		bool m_bRollEdge;
		short manu_space;
		union ATTACH_DATA {	//简单附加数
			DWORD dwParam;
			long  lParam;
			void* pParam;
		}tag;
		struct ARC_PARAM {	//圆弧参数
			double radius;		//指定圆弧半径(椭圆需要)
			double fSectAngle;	//指定扇形角(圆弧需要)
			GEPOINT center, work_norm, column_norm;
		}arc;
		VERTEX() {
			ciEdgeType = 1;
			m_bWeldEdge = m_bRollEdge = false;
			manu_space = 0;
			arc.radius = arc.fSectAngle = 0;
			tag.dwParam = 0;
		}
	};
	ATOM_LIST<VERTEX> vertexList;
	CAD_ENTITY m_xMkDimPoint;	//钢板标注数据点 wht 19-03-02
protected:
	BOOL IsValidVertexs();
	void ReverseVertexs();
	void DeleteAssisstPts();
	void UpdateVertexPropByArc(f3dArcLine& arcLine, int type);
public:
	CPlateObject();
	~CPlateObject();
	//
	virtual BOOL RecogWeldLine(const double* ptS, const double* ptE);
	virtual BOOL RecogWeldLine(f3dLine slop_line);
	virtual BOOL IsValid() { return vertexList.GetNodeNum() >= 2; }
	virtual BOOL IsClose(int* pIndex = NULL);
};
//CPlateProcessInfo
class CPNCModel;
class CPlateProcessInfo : public CPlateObject,public CCadPartObject
{
	struct LAYOUT_VERTEX {
		int index;			//轮廓点索引
		GEPOINT srcPos;		//轮廓点坐标
		GEPOINT offsetPos;	//相对于最小包络记性左上角的偏移位置
		LAYOUT_VERTEX() { index = 0; }
		void Init() {
			index = 0;
			srcPos.Set();
			offsetPos.Set();
		}
	};
private:
	GECS ucs;
	double m_fZoomScale;		//缩放比例，钢板缩放使用 wht 20.09.01
	CPNCModel* _pBelongModel;
	LAYOUT_VERTEX datumStartVertex, datumEndVertex;	//布局基准轮廓点
public:
	BOOL m_bEnableReactor;
	CProcessPlate xPlate;
	PART_PLATE xBomPlate;
	AcDbObjectId partNoId;
	AcDbObjectId partNumId;
	AcDbObjectId plateInfoBlockRefId;
	BOOL m_bIslandDetection;	//是否开启孤岛检测 wht 19-01-03
	GEPOINT dim_pos, dim_vec;
	CXhChar200 m_sRelatePartNo;
	BASIC_INFO m_xBaseInfo;
	CHashSet<AcDbObjectId> pnTxtIdList;
	ATOM_LIST<BOLT_INFO> boltList;
	//钢板关联实体
	CHashList<CAD_LINE> m_hashCloneEdgeEntIdByIndex;
	CHashList<ULONG> m_hashColneEntIdBySrcId;
	ARRAY_LIST<ULONG> m_cloneEntIdList;
	ARRAY_LIST<ULONG> m_newAddEntIdList;
	AcDbObjectId m_layoutBlockId;	//自动排版时添加的块引用
	BOOL m_bNeedExtract;	//记录当前构件是否需要提取，分批多次提取时使用 wht 19-04-02
	//加工数、单基数、总重修改状态 wht 20-07-29
	static const BYTE MODIFY_MANU_NUM = 0x01;
	static const BYTE MODIFY_SINGLE_NUM = 0x02;
	static const BYTE MODIFY_SUM_WEIGHT = 0x04;
	static const BYTE MODIFY_DES_SPEC = 0x08;
	static const BYTE MODIFY_DES_MAT = 0x10;
	BYTE m_ciModifyState;
	//
	f2dRect m_rectCard;			//工艺卡矩形框，用于批量打印 wht 20.01.27
	BOOL m_bHasCard;			//判断钢板是否带工艺卡框 wht 20.01.27
	static const DWORD ERROR_NORMAL = 0x0;	//无错误
	static const DWORD ERROR_TEXT_OUTSIDE_OF_PLATE = 0x01;	//文字超边
	static const DWORD ERROR_REPEAT_PART_LABEL = 0x02;	//重复件号
	static const DWORD ERROR_TEXT_INSIDE_OF_HOLE = 0x04;	//孔内文字
	static const DWORD ERROR_LABEL_INSIDE_OF_HOLE = 0x08;	//孔内件号
	DWORD m_dwErrorType;
	DWORD m_dwCorrectState;	//与错误状态配对使用，暂时只用来记录文字缩放是否修正成功 wht 19.12.24
	//属性定义区
	CPNCModel* get_pBelongModel() const;
	CPNCModel* set_pBelongModel(CPNCModel* pBelongModel);
	__declspec(property(put = set_pBelongModel, get = get_pBelongModel)) CPNCModel* m_pBelongModel;
private:
	void InitBtmEdgeIndex();
	void BuildPlateUcs();
	bool RecogRollEdge(CHashSet<CAD_ENTITY*>& rollEdgeDimTextSet, f3dLine& line);
	bool RecogCirclePlate(ATOM_LIST<VERTEX>& vertex_list);
public:
	CPlateProcessInfo();
	//
	CXhChar16 GetPartNo() { return xPlate.GetPartNo(); }
	void EmptyVertexs() {
		vertexList.Empty();
		m_xPolygon.Empty();
	}
	//获取钢板相关区域
	f2dRect GetPnDimRect(double fRectW = 10, double fRectH = 10);
	f2dRect GetMinWrapRect(double minDistance = 0, fPtList *pVertexList = NULL, double dfMaxRectW = 0);
	SCOPE_STRU GetPlateScope(BOOL bVertexOnly, BOOL bDisplayMK = TRUE);
	SCOPE_STRU GetCADEntScope(BOOL bIsColneEntScope = FALSE);
	void CreateRgnByText();
	void CreateRgn();
	void ExtractRelaEnts();
	//初始化钢板轮廓边信息
	void InitProfileByBPolyCmd(double fMinExtern, double fMaxExtern, BOOL bSendCommand = FALSE);//通过bpoly命令提取钢板信息
	BOOL InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntList);//通过选中实体初始化钢板信息
	BOOL InitProfileByAcdbCircle(AcDbObjectId idAcdbCircle);
	BOOL InitProfileByAcdbPolyLine(AcDbObjectId idAcdbPline);
	BOOL InitProfileByAcdbLineList(ARRAY_LIST<CAD_LINE>& xLineArr);
	BOOL InitProfileByAcdbLineList(CAD_LINE& startLine, ARRAY_LIST<CAD_LINE>& xLineArr);
	//更新钢板信息
	void CalEquidistantShape(double minDistance, ATOM_LIST<VERTEX> *pDestList);
	BOOL UpdatePlateInfo(BOOL bRelatePN = FALSE);
	void UpdateBoltHoles();
	void CheckProfileEdge();
	//生成中性文件
	void InitPPiInfo();
	void CreatePPiFile(const char* file_path);
	void CopyAttributes(CPlateProcessInfo* pSrcPlate);
	//绘制钢板
	bool InitLayoutVertexByBottomEdgeIndex(f2dRect &rect);
	void InitEdgeEntIdMap();
	void InitLayoutVertex(SCOPE_STRU& scope, BYTE ciLayoutType);
	bool DrawPlate(f3dPoint *pOrgion = NULL, BOOL bCreateDimPos = FALSE, BOOL bDrawAsBlock = FALSE,
		GEPOINT *pPlateCenter = NULL, double scale = 0, BOOL bSupportRotation = TRUE);
	void DrawPlateProfile(f3dPoint *pOrgion = NULL);
	//钢板钢印位置处理
	void InitMkPos(GEPOINT &mk_pos, GEPOINT &mk_vec);
	bool SyncSteelSealPos();
	bool AutoCorrectedSteelSealPos();
	bool GetSteelSealPos(GEPOINT &pos);
	bool UpdateSteelSealPos(GEPOINT &pos);
	//刷新钢板显示数量
	void RefreshPlateNum(int nNewNum);
	void RefreshPlateSpec();
	void RefreshPlateMat();
	void FillPlateNum(int nNewNum);
	//控制是否需要输出ppi文件 wht 20-10-10
	static BOOL m_bCreatePPIFile;
};
typedef CPlateProcessInfo::VERTEX PLATE_VER;
//////////////////////////////////////////////////////////////////////////
//
class CPlateReactorLife
{	//钢板反应器生命周期控制类
	CPlateProcessInfo *m_pPlateInfo;
public:
	CPlateReactorLife(CPlateProcessInfo *pPlate, BOOL bEnable) {
		m_pPlateInfo = pPlate;
		if (m_pPlateInfo)
			m_pPlateInfo->m_bEnableReactor = bEnable;
	}
	~CPlateReactorLife() {
		if (m_pPlateInfo)
			m_pPlateInfo->m_bEnableReactor = !m_pPlateInfo->m_bEnableReactor;
	}
};

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
class CAngleProcessInfo :public CCadPartObject
{
private:
	f3dPoint orig_pt;
	//
	void InitProcessInfo(const char* sValue);
public:
	PART_ANGLE m_xAngle;
	AcDbObjectId keyId;
	AcDbObjectId partNumId;		//加工数ID
	AcDbObjectId singleNumId;	//单基数ID
	AcDbObjectId sumWeightId;	//总重ID
	AcDbObjectId specId;		//规格ID
	AcDbObjectId materialId;	//材质ID
	CXhChar200 m_sTowerType;
	bool m_bInJgBlock;
	//加工数、单基数、总重修改状态 wht 20-07-29
	static const BYTE MODIFY_MANU_NUM = 0x01;
	static const BYTE MODIFY_SINGLE_NUM = 0x02;
	static const BYTE MODIFY_SUM_WEIGHT = 0x04;
	static const BYTE MODIFY_DES_GUIGE = 0x08;
	static const BYTE MODIFY_DES_MAT = 0x10;
	static const BYTE MODIFY_TA_MUM = 0x20;
	BYTE m_ciModifyState;
public:
	CAngleProcessInfo();
	~CAngleProcessInfo();
	//
	void SetOrig(f3dPoint pt) { orig_pt = pt; }
	BYTE InitAngleInfo(f3dPoint data_pos, const char* sValue);
	bool PtInDataRect(BYTE data_type, const double* poscoord);
	bool PtInDrawRect(const double* poscoord);
	GEPOINT GetAngleDataPos(BYTE data_type);
	GEPOINT GetJgCardPosL_B();
	CXhChar100 GetJgProcessInfo();
	SCOPE_STRU GetCADEntScope();
	void CreateRgn();
	void ExtractRelaEnts();
	//
	void RefreshAngleNum();
	void RefreshAngleSingleNum();
	void RefreshAngleSumWeight();
	void RefreshAngleSpec();
	void RefreshAngleMaterial();
};
#endif
