#pragma once
#include "XeroExtractor.h"
#include "ProcessPart.h"
#include "ArrayList.h"
#include "PNCDocReactor.h"
#include "..\..\LDS\LDS\BOM\BOM.h"

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
class CBoltEntGroup
{
public:
	static const BYTE BOLT_BLOCK		=0;
	static const BYTE BOLT_CIRCLE		=1;
	static const BYTE BOLT_TRIANGLE		=2;
	static const BYTE BOLT_SQUARE		=3;
	static const BYTE BOLT_WAIST_ROUND	=4;
	BYTE m_ciType;		//0.ͼ��|1.ԲȦ|2.������|3.������|4.��Բ��
	ULONG m_idEnt;
	BOOL m_bMatch;		//�Ƿ�ƥ�䵽�ְ�
	float m_fPosX;
	float m_fPosY;
	float m_fHoleD;
	vector<ULONG> m_xLineArr;
	vector<ULONG> m_xCirArr;
	vector<ULONG> m_xArcArr;
public:
	CBoltEntGroup() { 
		m_ciType = 0; 
		m_idEnt = 0;
		m_bMatch = FALSE; 
		m_fPosX = m_fPosY = m_fHoleD = 0;
	}
};
struct BASIC_INFO {
	char m_cMat;
	char m_cQuality;			//�����ȼ�
	int m_nThick;
	int m_nNum;
	long m_idCadEntNum;
	CXhChar16 m_sPartNo;
	CXhChar100 m_sPrjCode;		//���̱��
	CXhChar100 m_sPrjName;		//��������
	CXhChar50 m_sTaType;		//����
	CXhChar50 m_sTaAlias;		//����
	CXhChar50 m_sTaStampNo;		//��ӡ��
	CXhChar200 m_sBoltStr;		//��˨�ַ���
	CXhChar100 m_sTaskNo;		//�����
	BASIC_INFO() { m_nThick = m_nNum = 0; m_cMat = 0; m_cQuality = 0; m_idCadEntNum = 0; }
};
//////////////////////////////////////////////////////////////////////////
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
	struct VERTEX {
		GEPOINT pos;
		char ciEdgeType;	//1:��ֱͨ�� 2:Բ�� 3:��Բ��
		bool m_bWeldEdge;
		bool m_bRollEdge;
		short manu_space;
		union ATTACH_DATA {	//�򵥸�����
			DWORD dwParam;
			long  lParam;
			void* pParam;
		}tag;
		struct ARC_PARAM {	//Բ������
			double radius;		//ָ��Բ���뾶(��Բ��Ҫ)
			double fSectAngle;	//ָ�����ν�(Բ����Ҫ)
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
	CAD_ENTITY m_xMkDimPoint;	//�ְ��ע���ݵ� wht 19-03-02
protected:
	BOOL IsValidVertexs();
	void ReverseVertexs();
	void DeleteAssisstPts();
	void UpdateVertexPropByArc(f3dArcLine& arcLine, int type);
	void CreateRgn();
public:
	CPlateObject();
	~CPlateObject();
	//
	virtual bool IsInPlate(const double* poscoord);
	virtual bool IsInPlate(const double* start, const double* end);
	virtual BOOL RecogWeldLine(const double* ptS, const double* ptE);
	virtual BOOL RecogWeldLine(f3dLine slop_line);
	virtual BOOL IsValid() { return vertexList.GetNodeNum() >= 3; }
	virtual BOOL IsClose(int* pIndex = NULL);
};
//CPlateProcessInfo
class CPNCModel;
class CPlateProcessInfo : public CPlateObject
{
	struct LAYOUT_VERTEX{
		int index;			//����������
		GEPOINT srcPos;		//����������
		GEPOINT offsetPos;	//�������С����������Ͻǵ�ƫ��λ��
		LAYOUT_VERTEX(){index=0;}
		void Init() {
			index = 0;
			srcPos.Set();
			offsetPos.Set();
		}
	};
private:
	GECS ucs;
	double m_fZoomScale;		//���ű������ְ�����ʹ�� wht 20.09.01
	CPNCModel* _pBelongModel;
	LAYOUT_VERTEX datumStartVertex,datumEndVertex;	//���ֻ�׼������
public:
	BOOL m_bEnableReactor;
	CProcessPlate xPlate;
	PART_PLATE xBomPlate;
	AcDbObjectId partNoId;
	AcDbObjectId partNumId;
	AcDbObjectId plateInfoBlockRefId;
	BOOL m_bIslandDetection;	//�Ƿ����µ���� wht 19-01-03
	GEPOINT dim_pos,dim_vec;
	CXhChar200 m_sRelatePartNo;
	BASIC_INFO m_xBaseInfo;
	CHashSet<AcDbObjectId> pnTxtIdList;
	ATOM_LIST<BOLT_INFO> boltList;
	//�ְ����ʵ��
	CHashList<CAD_ENTITY> m_xHashRelaEntIdList;	
	CHashSet<CAD_ENTITY*> m_xHashInvalidBoltCir;		//��¼��Ч��ԲȦ�������������Ա�
	CHashList<CAD_LINE> m_hashCloneEdgeEntIdByIndex;
	CHashList<ULONG> m_hashColneEntIdBySrcId;
	ARRAY_LIST<ULONG> m_cloneEntIdList;
	ARRAY_LIST<ULONG> m_newAddEntIdList;
	AcDbObjectId m_layoutBlockId;	//�Զ��Ű�ʱ��ӵĿ�����
	BOOL m_bNeedExtract;	//��¼��ǰ�����Ƿ���Ҫ��ȡ�����������ȡʱʹ�� wht 19-04-02
	//�ӹ������������������޸�״̬ wht 20-07-29
	static const BYTE MODIFY_MANU_NUM	= 0x01;
	static const BYTE MODIFY_SINGLE_NUM = 0x02;
	static const BYTE MODIFY_SUM_WEIGHT = 0x04;
	BYTE m_ciModifyState;
	//
	f2dRect m_rectCard;			//���տ����ο�����������ӡ wht 20.01.27
	BOOL m_bHasCard;			//�жϸְ��Ƿ�����տ��� wht 20.01.27
	static const DWORD ERROR_NORMAL					= 0x0;	//�޴���
	static const DWORD ERROR_TEXT_OUTSIDE_OF_PLATE	= 0x01;	//���ֳ���
	static const DWORD ERROR_REPEAT_PART_LABEL		= 0x02;	//�ظ�����
	static const DWORD ERROR_TEXT_INSIDE_OF_HOLE	= 0x04;	//��������
	static const DWORD ERROR_LABEL_INSIDE_OF_HOLE	= 0x08;	//���ڼ���
	DWORD m_dwErrorType;
	DWORD m_dwCorrectState;	//�����״̬���ʹ�ã���ʱֻ������¼���������Ƿ������ɹ� wht 19.12.24
	//���Զ�����
	CPNCModel* get_pBelongModel() const;
	CPNCModel* set_pBelongModel(CPNCModel* pBelongModel);
	__declspec(property(put = set_pBelongModel, get = get_pBelongModel)) CPNCModel* m_pBelongModel;
private:
	void InitBtmEdgeIndex();
	void BuildPlateUcs();
	void PreprocessorBoltEnt(int* piInvalidCirCountForText, int* piInvalidCirCountForLabel,
							 CHashStrList<CXhChar16>* pHashPartLabelByLabel);
	CAD_ENTITY* AppendRelaEntity(AcDbEntity *pEnt, CHashList<CAD_ENTITY>* pHashRelaEntIdList = NULL);
	bool RecogRollEdge(CHashSet<CAD_ENTITY*>& rollEdgeDimTextSet, f3dLine& line);
public:
	CPlateProcessInfo();
	//
	CXhChar16 GetPartNo() { return xPlate.GetPartNo(); }
	//��ȡ�ְ��������
	f2dRect GetPnDimRect(double fRectW = 10, double fRectH = 10);
	f2dRect GetMinWrapRect(double minDistance = 0, fPtList *pVertexList = NULL);
	SCOPE_STRU GetPlateScope(BOOL bVertexOnly,BOOL bDisplayMK=TRUE);
	SCOPE_STRU GetCADEntScope(BOOL bIsColneEntScope = FALSE);
	//��ʼ���ְ���������Ϣ
	void InitProfileByBPolyCmd(double fMinExtern,double fMaxExtern, BOOL bSendCommand = FALSE);//ͨ��bpoly������ȡ�ְ���Ϣ
	BOOL InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntList);//ͨ��ѡ��ʵ���ʼ���ְ���Ϣ
	BOOL InitProfileByAcdbCircle(AcDbObjectId idAcdbCircle);
	BOOL InitProfileByAcdbPolyLine(AcDbObjectId idAcdbPline);
	BOOL InitProfileByAcdbLineList(ARRAY_LIST<CAD_LINE>& xLineArr);
	BOOL InitProfileByAcdbLineList(CAD_LINE& startLine, ARRAY_LIST<CAD_LINE>& xLineArr);
	//���¸ְ���Ϣ
	void CalEquidistantShape(double minDistance, ATOM_LIST<VERTEX> *pDestList);
	void ExtractPlateRelaEnts();
	BOOL UpdatePlateInfo(BOOL bRelatePN=FALSE);
	void UpdateBoltHoles(CHashStrList<CXhChar16>* pHashPartLabelByLabel = NULL);
	void CheckProfileEdge();
	//���������ļ�
	void InitPPiInfo();
	void CreatePPiFile(const char* file_path);
	void CopyAttributes(CPlateProcessInfo* pSrcPlate);
	//���Ƹְ�
	bool InitLayoutVertexByBottomEdgeIndex(f2dRect &rect);
	void InitEdgeEntIdMap();
	void InitLayoutVertex(SCOPE_STRU& scope, BYTE ciLayoutType);
	bool DrawPlate(f3dPoint *pOrgion=NULL,BOOL bCreateDimPos=FALSE,BOOL bDrawAsBlock=FALSE,
				   GEPOINT *pPlateCenter=NULL,double scale=0,BOOL bSupportRotation=TRUE);
	void DrawPlateProfile(f3dPoint *pOrgion = NULL);
	//�ְ��ӡλ�ô���
	void InitMkPos(GEPOINT &mk_pos, GEPOINT &mk_vec);
	bool SyncSteelSealPos();
	bool AutoCorrectedSteelSealPos();
	bool GetSteelSealPos(GEPOINT &pos);
	bool UpdateSteelSealPos(GEPOINT &pos);
	//ˢ�¸ְ���ʾ����
	void RefreshPlateNum();
	//�����Ƿ���Ҫ���ppi�ļ� wht 20-10-10
	static BOOL m_bCreatePPIFile;
};
//////////////////////////////////////////////////////////////////////////
//
class CPlateReactorLife
{	//�ְ巴Ӧ���������ڿ�����
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
	CHashSet<AcDbObjectId> m_hashSolidLineTypeId;	//��¼��Ч��ʵ������id wht 19-01-03
public:
	CHashSet<AcDbObjectId> m_xAllEntIdSet;
	CHashSet<AcDbObjectId> m_xAllLineHash;
	CHashStrList<CBoltEntGroup> m_xBoltEntHash;
	CXhChar100 m_sCompanyName;	//��Ƶ�λ
	CXhChar100 m_sPrjCode;		//���̱��
	CXhChar100 m_sPrjName;		//��������
	CXhChar50 m_sTaType;		//����
	CXhChar50 m_sTaAlias;		//����
	CXhChar50 m_sTaStampNo;		//��ӡ��
	CXhChar500 m_sWorkPath;		//��ǰģ�Ͷ�Ӧ�Ĺ���·�� wht 19-04-02
	CString m_sCurWorkFile;		//��ǰ���ڲ������ļ�
	static const float ASSIST_RADIUS;
	static const float DIST_ERROR;
	static const float WELD_MAX_HEIGHT;
	static BOOL m_bSendCommand;
	//
	static CString MakePosKeyStr(GEPOINT pos);
	static double StandardHoleD(double fDiameter);
private:
	bool AppendBoltEntsByBlock(ULONG idBlockEnt);
	bool AppendBoltEntsByCircle(ULONG idCirEnt);
	bool AppendBoltEntsByPolyline(ULONG idPolyline);
	bool AppendBoltEntsByConnectLines(vector<CAD_LINE> vectorConnLine);
	bool AppendBoltEntsByAloneLines(vector<CAD_LINE> vectorAloneLine);
public:
	CPNCModel(void);
	~CPNCModel(void);
	//
	void Empty();
	void ExtractPlateBoltEnts(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void ExtractPlateProfileEx(CHashSet<AcDbObjectId>& selectedEntIdSet);
	void FilterInvalidEnts(CHashSet<AcDbObjectId>& selectedEntIdSet, CSymbolRecoginzer* pSymbols);
	void InitPlateVextexs(CHashSet<AcDbObjectId>& hashProfileEnts);
	void MergeManyPartNo();
	void SplitManyPartNo();
	void CreatePlatePPiFile(const char* work_path);
	//���Ƹְ�
	void DrawPlates(BOOL bOnlyNewExtractedPlate=FALSE);
	void DrawPlatesToLayout(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToCompare(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToProcess(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToClone(BOOL bOnlyNewExtractedPlate = FALSE);
	void DrawPlatesToFiltrate(BOOL bOnlyNewExtractedPlate = FALSE);
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
	CSortedModel(CPNCModel *pModel, BOOL bOnlyNewExtractedPlate = FALSE);
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
extern CPNCModel model;
