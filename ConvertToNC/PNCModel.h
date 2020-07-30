#pragma once
#include "f_ent_list.h"
#include "HashTable.h"
#include "list.h"
#include "XeroExtractor.h"
#include "ProcessPart.h"
#include "ArrayList.h"
#include "CadToolFunc.h"
#include "XhMath.h"
#include "..\..\LDS\LDS\BOM\BOM.h"
#include "DocManagerReactor.h"
#include <vector>

using std::vector;
//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
class CPlateProcessInfo : public CPlateObject
{
	struct LAYOUT_VERTEX{
		int index;			//轮廓点索引
		GEPOINT srcPos;		//轮廓点坐标
		GEPOINT offsetPos;	//相对于最小包络记性左上角的偏移位置
		LAYOUT_VERTEX(){index=0;}
		void Init() {
			index = 0;
			srcPos.Set();
			offsetPos.Set();
		}
	};
private:
	GECS ucs;
	LAYOUT_VERTEX datumStartVertex,datumEndVertex;	//布局基准轮廓点
public:
	BOOL m_bEnableReactor;
	CProcessPlate xPlate;
	PART_PLATE xBomPlate;
	AcDbObjectId partNoId;
	AcDbObjectId partNumId;
	AcDbObjectId plateInfoBlockRefId;
	BOOL m_bIslandDetection;	//是否开启孤岛检测 wht 19-01-03
	GEPOINT dim_pos,dim_vec;
	CXhChar100 m_sRelatePartNo;
	BASIC_INFO m_xBaseInfo;
	CHashSet<AcDbObjectId> pnTxtIdList;
	ATOM_LIST<BOLT_INFO> boltList;
	//钢板关联实体
	CHashList<CAD_ENTITY> m_xHashRelaEntIdList;	
	CHashSet<CAD_ENTITY*> m_xHashInvalidBoltCir;		//记录无效的圆圈，方便后期输出对比
	CHashList<CAD_LINE> m_hashCloneEdgeEntIdByIndex;
	CHashList<ULONG> m_hashColneEntIdBySrcId;
	ARRAY_LIST<ULONG> m_cloneEntIdList;
	ARRAY_LIST<ULONG> m_newAddEntIdList;
	AcDbObjectId m_layoutBlockId;	//自动排版时添加的块引用
	BOOL m_bNeedExtract;	//记录当前构件是否需要提取，分批多次提取时使用 wht 19-04-02
	//加工数、单基数、总重修改状态 wht 20-07-29
	static const BYTE MODIFY_MANU_NUM	= 0x01;
	static const BYTE MODIFY_SINGLE_NUM = 0x02;
	static const BYTE MODIFY_SUM_WEIGHT = 0x04;
	BYTE m_ciModifyState;
private:
	void InitBtmEdgeIndex();
	void BuildPlateUcs();
	void PreprocessorBoltEnt(int *piInvalidCirCountForText);
	CAD_ENTITY* AppendRelaEntity(AcDbEntity *pEnt);
public:
	CPlateProcessInfo();
	//
	CXhChar16 GetPartNo() { return xPlate.GetPartNo(); }
	//获取钢板相关区域
	f2dRect GetPnDimRect(double fRectW = 10, double fRectH = 10);
	f2dRect GetMinWrapRect(double minDistance = 0, fPtList *pVertexList = NULL);
	SCOPE_STRU GetPlateScope(BOOL bVertexOnly,BOOL bDisplayMK=TRUE);
	SCOPE_STRU GetCADEntScope(BOOL bIsColneEntScope = FALSE);
	//初始化钢板轮廓边信息
	void InitProfileByBPolyCmd(double fMinExtern,double fMaxExtern, BOOL bSendCommand = FALSE);//通过bpoly命令提取钢板信息
	BOOL InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntList);//通过选中实体初始化钢板信息
	BOOL InitProfileByAcdbCircle(AcDbObjectId idAcdbCircle);
	BOOL InitProfileByAcdbPolyLine(AcDbObjectId idAcdbPline);
	BOOL InitProfileByAcdbLineList(ARRAY_LIST<CAD_LINE>& xLineArr);
	BOOL InitProfileByAcdbLineList(CAD_LINE& startLine, ARRAY_LIST<CAD_LINE>& xLineArr);
	//更新钢板信息
	void CalEquidistantShape(double minDistance, ATOM_LIST<VERTEX> *pDestList);
	void ExtractPlateRelaEnts();
	BOOL UpdatePlateInfo(BOOL bRelatePN=FALSE);
	void CheckProfileEdge();
	//生成中性文件
	void CreatePPiFile(const char* file_path);
	void CopyAttributes(CPlateProcessInfo* pSrcPlate);
	//绘制钢板
	bool InitLayoutVertexByBottomEdgeIndex(f2dRect &rect);
	void InitEdgeEntIdMap();
	void InitLayoutVertex(SCOPE_STRU& scope, BYTE ciLayoutType);
	void DrawPlate(f3dPoint *pOrgion=NULL,BOOL bCreateDimPos=FALSE,BOOL bDrawAsBlock=FALSE,GEPOINT *pPlateCenter=NULL);
	void DrawPlateProfile(f3dPoint *pOrgion = NULL);
	//钢板钢印位置处理
	void InitMkPos(GEPOINT &mk_pos, GEPOINT &mk_vec);
	bool SyncSteelSealPos();
	bool AutoCorrectedSteelSealPos();
	bool GetSteelSealPos(GEPOINT &pos);
	bool UpdateSteelSealPos(GEPOINT &pos);
	//刷新钢板显示数量
	void RefreshPlateNum();
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
	CHashSet<AcDbObjectId> m_hashSolidLineTypeId;	//记录有效的实体线型id wht 19-01-03
public:
	CHashSet<AcDbObjectId> m_xAllEntIdSet;
	CHashSet<AcDbObjectId> m_xAllLineHash;
	CHashList<CAD_ENTITY> m_xBoltBlockHash;
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaAlias;		//代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar500 m_sWorkPath;		//当前模型对应的工作路径 wht 19-04-02
	CString m_sCurWorkFile;		//当前正在操作的文件
	static const float ASSIST_RADIUS;
	static const float DIST_ERROR;
	static const float WELD_MAX_HEIGHT;
	static BOOL m_bSendCommand;
public:
	CPNCModel(void);
	~CPNCModel(void);
	//
	void Empty();
	void ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols);
	void InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts);
	void MergeManyPartNo();
	void SplitManyPartNo();
	//绘制钢板
	void DrawPlates();
	void DrawPlatesToLayout();
	void DrawPlatesToCompare();
	void DrawPlatesToProcess();
	void DrawPlatesToClone();
	void DrawPlatesToFiltrate();
	//
	int GetPlateNum(){return m_hashPlateInfo.GetNodeNum();}
	int PushPlateStack() { return m_hashPlateInfo.push_stack(); }
	bool PopPlateStack() { return m_hashPlateInfo.pop_stack(); }
	CPlateProcessInfo* PartFromPartNo(const char* sPartNo) { return m_hashPlateInfo.GetValue(sPartNo); }
	CPlateProcessInfo* AppendPlate(char* sPartNo){return m_hashPlateInfo.Add(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(char* sPartNo){return m_hashPlateInfo.GetValue(sPartNo);}
	CPlateProcessInfo* GetPlateInfo(AcDbObjectId partNoEntId);
	CPlateProcessInfo* GetPlateInfo(GEPOINT text_pos);
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
//////////////////////////////////////////////////////////////////////////
//
class CSortedModel
{
	ARRAY_LIST<CPlateProcessInfo*> platePtrList;
public:
	struct PARTGROUP
	{
		ATOM_LIST<CPlateProcessInfo*> sameGroupPlateList;
		CXhChar50 sKey;
		//
		double GetMaxHight();
		double GetMaxWidth();
		CPlateProcessInfo *EnumFirstPlate();
		CPlateProcessInfo *EnumNextPlate();
	};
	CHashStrList<PARTGROUP> hashPlateGroup;
public:
	CSortedModel(CPNCModel *pModel);
	//
	CPlateProcessInfo *EnumFirstPlate();
	CPlateProcessInfo *EnumNextPlate();
	void DividPlatesBySeg();
	void DividPlatesByThickMat();
	void DividPlatesByThick();
	void DividPlatesByMat();
	void DividPlatesByPartNo();
};
//////////////////////////////////////////////////////////////////////////
//
class CDxfFolder
{
public:
	struct DXF_ITEM 
	{
		CXhChar50 m_sFileName;
		CString m_sFilePath;
	};
	CString m_sFolderPath;
	CXhChar50 m_sFolderName;
	vector<DXF_ITEM> m_xDxfFileSet;
public:
	CDxfFolder() {}
	~CDxfFolder() { m_xDxfFileSet.clear(); }
};
class CExpoldeModel {
	ATOM_LIST<CDxfFolder> dxfFolderList;
public:
	CExpoldeModel() { ; }
	//
	int GetNum() { return dxfFolderList.GetNodeNum(); }
	CDxfFolder* AppendFolder() { return dxfFolderList.append(); }
	CDxfFolder* EnumFirst() { return dxfFolderList.GetFirst(); }
	CDxfFolder* EnumNext() { return dxfFolderList.GetNext(); }
};
extern CExpoldeModel g_explodeModel;
extern CDocManagerReactor *g_pDocManagerReactor;
