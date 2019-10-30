#pragma once
#include "f_ent_list.h"
#include "HashTable.h"
#include "list.h"
#include "XeroExtractor.h"
#include "ProcessPart.h"
#include "ArrayList.h"
#include "CadToolFunc.h"

//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
struct VERTEX_TAG{
	long nDist; 
	long nLsNum;
	VERTEX_TAG(){nDist=0;nLsNum=0;}
};
struct ACAD_LINEID{
	AcDbObjectId m_lineId;
	double m_fLen;
	GEPOINT m_ptStart,m_ptEnd;
	BOOL m_bReverse;
	ACAD_LINEID(){
		m_lineId=0;
		m_fLen=0;
		m_bReverse=FALSE;
	}
	ACAD_LINEID(AcDbObjectId id,double len){
		m_fLen=len;
		m_lineId=id;
		m_bReverse=FALSE;
	}
	ACAD_LINEID(AcDbObjectId id,GEPOINT &start,GEPOINT &end){
		Init(id,start,end);
	}
	void Init(AcDbObjectId id,GEPOINT &start,GEPOINT &end){
		m_lineId=id;
		m_ptStart=start;
		m_ptEnd=end;
		m_fLen=DISTANCE(m_ptStart,m_ptEnd);
		m_bReverse=FALSE;
	}
	BOOL UpdatePos(){
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,m_lineId,AcDb::kForRead);
		CAcDbObjLife life(pEnt);
		if(pEnt==NULL)
			return FALSE;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			m_ptStart.Set(pLine->startPoint().x, pLine->startPoint().y, 0);
			m_ptEnd.Set(pLine->endPoint().x, pLine->endPoint().y, 0);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			m_ptStart.Set(startPt.x, startPt.y, 0);
			m_ptEnd.Set(endPt.x, endPt.y, 0);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			m_ptStart.Set(startPt.x, startPt.y, 0);
			m_ptEnd.Set(endPt.x, endPt.y, 0);
		}
		else
			return FALSE;
		if(m_bReverse)
		{
			GEPOINT temp=m_ptStart;
			m_ptStart=m_ptEnd;
			m_ptEnd=temp;
		}
		return TRUE;
	}
};
struct ACAD_CIRCLE{
	AcDbObjectId cirEntId;
	GEPOINT center;
	double radius;
	ACAD_CIRCLE(){radius=0;cirEntId=0;}
	ACAD_CIRCLE(AcDbObjectId &objId,double r,GEPOINT &center_pt){
		cirEntId=objId;
		radius=r;
		center=center_pt;
	}
	bool IsInCircle(f3dPoint &pos){
		double dist=DISTANCE(pos,center);
		if(dist<radius)
			return true;
		else
			return false;
	}
};
struct RELA_ACADENTITY{
	static const BYTE TYPE_OTHER	=0;
	static const BYTE TYPE_LINE		=1;
	static const BYTE TYPE_ARC		=2;
	static const BYTE TYPE_CIRCLE	=3;
	static const BYTE TYPE_ELLIPSE	=4;
	static const BYTE TYPE_SPLINE	=5;
	static const BYTE TYPE_TEXT		=6;
	static const BYTE TYPE_MTEXT	=7;
	static const BYTE TYPE_BLOCKREF =8;
	BYTE ciEntType;
	AcDbObjectId idCadEnt;
	RELA_ACADENTITY(){ciEntType=0;}
	RELA_ACADENTITY(AcDbObjectId idEnt){idCadEnt=idEnt;ciEntType=0;}
};
AcDbObjectId MkCadObjId(unsigned long idOld);//{ return AcDbObjectId((AcDbStub*)idOld); }
class CPlateProcessInfo : public CPlateObject
{
	struct LAYOUT_VERTEX{
		int index;			//轮廓点索引
		GEPOINT srcPos;		//轮廓点坐标
		GEPOINT offsetPos;	//相对于最小包络记性左上角的偏移位置
		LAYOUT_VERTEX(){index=0;}
	};
private:
	GECS ucs;
	double m_fZoomScale;		//缩放比例
	LAYOUT_VERTEX datumStartVertex,datumEndVertex;	//布局基准轮廓点
public:
	BOOL m_bEnableReactor;
	CProcessPlate xPlate;
	AcDbObjectId partNoId;
	AcDbObjectId partNumId;
	AcDbObjectId plateInfoBlockRefId;
	BOOL m_bIslandDetection;	//是否开启孤岛检测 wht 19-01-03
	f3dPoint dim_pos,dim_vec;
	BOOL m_bHasInnerDimPos;
	f3dPoint inner_dim_pos;		//根据件号位置找到的最近的螺栓孔位置，用于螺栓垫板提取 wht 19-02-01
	CXhChar100 m_sRelatePartNo;
	BASIC_INFO m_xBaseInfo;
	CHashSet<AcDbObjectId> pnTxtIdList;
	ATOM_LIST<BOLT_INFO> boltList;
	CHashList<ACAD_LINEID> m_hashCloneEdgeEntIdByIndex;
	CHashList<ULONG> m_hashColneEntIdBySrcId;
	ARRAY_LIST<ULONG> m_cloneEntIdList;
	BOOL m_bNeedExtract;	//记录当前构件是否需要提取，分批多次提取时使用 wht 19-04-02
	void InitEdgeEntIdMap();
	void UpdateEdgeEntPos();
private:
	void InitBtmEdgeIndex();
	void BuildPlateUcs();
	void PreprocessorBoltEnt(CHashSet<CAD_ENTITY*> &hashInvalidBoltCirPtrSet, int *piInvalidCirCountForText);
	void InternalExtractPlateRelaEnts();
public:
	CPlateProcessInfo();
	SCOPE_STRU GetPlateScope(BOOL bVertexOnly,BOOL bDisplayMK=TRUE);
	bool InitLayoutVertexByBottomEdgeIndex(f2dRect &rect);
	f2dRect GetMinWrapRect(double minDistance=0, fPtList *pVertexList=NULL);
	CXhChar16 GetPartNo(){return xPlate.GetPartNo();}
	void InitProfileByBPolyCmd(double fMinExtern,double fMaxExtern);//通过bpoly命令提取钢板信息
	BOOL InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntList);//通过选中实体初始化钢板信息
	void ExtractPlateRelaEnts();
	BOOL UpdatePlateInfo(BOOL bRelatePN=FALSE);
	f2dRect GetPnDimRect();
	void CreatePPiFile(const char* file_path);
	void DrawPlate(f3dPoint *pOrgion=NULL,BOOL bCreateDimPos=FALSE,BOOL bDrawAsBlock=FALSE,GEPOINT *pPlateCenter=NULL);
	void InitMkPos(GEPOINT &mk_pos,GEPOINT &mk_vec);
	void CalEquidistantShape(double minDistance,ATOM_LIST<VERTEX> *pDestList);
	CAD_ENTITY* AppendRelaEntity(AcDbEntity *pEnt);
	int GetRelaEntIdList(ARRAY_LIST<AcDbObjectId> &entIdList);
	bool SyncSteelSealPos();
	bool AutoCorrectedSteelSealPos();
	bool GetSteelSealPos(GEPOINT &pos);
	bool UpdateSteelSealPos(GEPOINT &pos);
	void RefreshPlateNum();
	SCOPE_STRU GetCADEntScope(BOOL bIsColneEntScope=FALSE);
};
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
//////////////////////////////////////////////////////////////////////////
//CPNCModel
class CPNCModel
{
	CHashStrList<CPlateProcessInfo> m_hashPlateInfo;
	AcDbObjectId m_idSolidLine,m_idNewLayer;
	CHashSet<AcDbObjectId> m_hashSolidLineTypeId;	//记录有效的实体线型id wht 19-01-03
public:
	CHashSet<AcDbObjectId> m_xAllEntIdSet;
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaAlias;		//代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar500 m_sWorkPath;		//当前模型对应的工作路径 wht 19-04-02
public:
	CPNCModel(void);
	~CPNCModel(void);
	void Empty();
	void ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void MergeManyPartNo();
	void LayoutPlates(BOOL bRelayout);
	AcDbObjectId GetEntLineTypeId(AcDbEntity *pEnt,char* sLayer=NULL);
	//
	int GetPlateNum(){return m_hashPlateInfo.GetNodeNum();}
	int PushPlateStack() { return m_hashPlateInfo.push_stack(); }
	bool PopPlateStack() { return m_hashPlateInfo.pop_stack(); }
	CPlateProcessInfo* PartFromPartNo(const char* sPartNo) { return m_hashPlateInfo.GetValue(sPartNo); }
	CPlateProcessInfo* AppendPlate(char* sPartNo){return m_hashPlateInfo.Add(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(char* sPartNo){return m_hashPlateInfo.GetValue(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(AcDbObjectId partNoEntId);
	CPlateProcessInfo* EnumFirstPlate(BOOL bOnlyNewExtractedPlate)
	{
		if (bOnlyNewExtractedPlate)
		{
			CPlateProcessInfo* pPlate = m_hashPlateInfo.GetFirst();
			while (pPlate&&!pPlate->m_bNeedExtract)
				pPlate = m_hashPlateInfo.GetNext();
			return pPlate;
		}
		else
			return m_hashPlateInfo.GetFirst();
	}
	CPlateProcessInfo* EnumNextPlate(BOOL bOnlyNewExtractedPlate)
	{
		if (bOnlyNewExtractedPlate)
		{
			CPlateProcessInfo* pPlate = m_hashPlateInfo.GetNext();
			while (pPlate&&!pPlate->m_bNeedExtract)
				pPlate = m_hashPlateInfo.GetNext();
			return pPlate;
		}
		else
			return m_hashPlateInfo.GetNext();
	}
	void WritePrjTowerInfoToCfgFile(const char* cfg_file_path);
};
extern CPNCModel model;

class CSortedModel
{
	ARRAY_LIST<CPlateProcessInfo*> platePtrList;
public:
	CSortedModel(CPNCModel *pModel);

	CPlateProcessInfo *EnumFirstPlate();
	CPlateProcessInfo *EnumNextPlate();
};