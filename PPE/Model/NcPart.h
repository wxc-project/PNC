#pragma once
#include "PPEModel.h"
#include "NcPlate.h"
#include "DxfFile.h"

struct DRILL_BOLT_INFO
{
	int nBoltNum;
	double fHoleD;
	double fMinDist;
	DRILL_BOLT_INFO() { fHoleD = 0; nBoltNum = 0; fMinDist = 0; }
};
class CDrillBolt
{
public:
	static const int DRILL_NULL		= -1;	//不需要冲孔
	static const int DRILL_T2		= 0;	//T2冲头
	static const int DRILL_T3		= 1;	//T3冲头
	BYTE biMode;	//-1.不需要 0.T2 1.T3
	int nBoltD;
	double fHoleD;
	ATOM_LIST<BOLT_INFO> boltList;
	CDrillBolt() { biMode = 0; fHoleD = 0.0; nBoltD = 0; }
	void OptimizeBoltOrder(f3dPoint& startPos,BYTE ciAlgType =0);
};
struct PLATE_GROUP
{
	DWORD thick;
	char cMaterial;
	CXhChar50 sKey;
	CXhPtrSet<CProcessPlate> plateSet;
};

class CNCPart
{
	static void InitScopeToDxf(CDxfFile& dxf_file, SCOPE_STRU& scope);
	static void AddLineToDxf(CDxfFile& dxf_file, GEPOINT ptS, GEPOINT ptE, int clrIndex = -1);
	static void AddArcLineToDxf(CDxfFile& dxf_file, GEPOINT ptS, GEPOINT ptE, GEPOINT ptC, double radius, int clrIndex = -1);
	static void AddCircleToDxf(CDxfFile& dxf_file, GEPOINT ptC, double radius, int clrIndex = -1);
	static void AddEllipseToDxf(CDxfFile& dxf_file, f3dArcLine ellipse, int clrIndex = -1);
	static void AddTextToDxf(CDxfFile& dxf_file, const char* sText, GEPOINT dimPos, double fFontH, double fRotAngle = 0, int clrIndex = -1);
	static void AddEdgeToDxf(CDxfFile& dxf_file, PROFILE_VER* pPrevVertex, PROFILE_VER* pCurVertex, GEPOINT axis_z, BOOL bOverturn);
	static void AddHoleToDxf(CDxfFile& dxf_file, BOLT_INFO* pHole);
	//解析钢板的备注信息
	static void ParseNoteInfo(const char* sPartInfoKeyStr, CStringArray& sNoteArr, CProcessPlate* pPlate);
public:
	//钢板NC模式
	static const BYTE FLAME_MODE	= 0x01;	 //火焰切割
	static const BYTE PLASMA_MODE	= 0x02;	 //等离子切割
	static const BYTE PUNCH_MODE	= 0x04;	 //冲床加工
	static const BYTE DRILL_MODE	= 0x08;	 //钻床加工
	static const BYTE LASER_MODE	= 0x10;	 //激光复合机
	//钢板NC输出文件
	static const int PLATE_DXF_FILE = 0x01;  //钢板DXF类型文件
	static const int PLATE_NC_FILE	= 0x02;	 //钢板NC类型文件
	static const int PLATE_TXT_FILE = 0x04;	 //钢板TXT类型文件
	static const int PLATE_CNC_FILE = 0x08;	 //钢板CNC类型文件
	static const int PLATE_PMZ_FILE = 0x10;	 //钢板PMZ类型文件
	static const int PLATE_PBJ_FILE = 0x20;  //钢板PBJ类型文件
	static const int PLATE_TTP_FILE = 0x40;	 //钢板TTP类型文件
	static const int PLATE_WKF_FILE = 0x80;	 //济南法特WKF文件
	//螺栓优化排序算法
	static const BYTE ALG_GREEDY	= 1;	//贪吃算法
	static const BYTE ALG_BACKTRACK	= 2;	//回溯算法
	static const BYTE ALG_ANNEAL	= 3;	//退火算法
public:
	static BOOL m_bDeformedProfile;		//考虑火曲变形
	static CString m_sExportPartInfoKeyStr;	//钢板输出明细
public:
	static void DeformedPlateProfile(CProcessPlate *pPlate);
	static int GetLineLenFromExpression(double fThick, const char* sValue);
	//钢板操作
	static bool CreatePlateTtpFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlateDxfFile(CProcessPlate *pPlate,const char* file_path,int dxf_mode);
	static bool CreatePlateWkfFile(CProcessPlate *pPlate, const char* file_path);
#ifdef __PNC_
	static void ImprotPlateCncOrTextFile(CProcessPlate *pPlate, const char* file_path);
	static void InitStoreMode(CHashList<CDrillBolt>& hashDrillBoltByD, ARRAY_LIST<double> &holeDList, BOOL bIncSH = TRUE);
	static void RefreshPlateHoles(CProcessPlate *pPlate, BYTE ciNcMode, BYTE ciAlgType = 0);
	static void OptimizeBolt(CProcessPlate *pPlate, CHashList<CDrillBolt>& hashDrillBoltByD, BYTE ciNcMode, BYTE ciAlgType = 0);
	static bool CreatePlatePbjFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlatePmzFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlatePmzCheckFile(CProcessPlate *pPlate, const char* file_path);
	static bool CreatePlateTxtFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlateNcFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlateCncFile(CProcessPlate *pPlate,const char* file_path);
#endif
	//角钢操作
	static void CreateAngleNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessAngle> &angleSet,const char* drv_path,const char* sPartNoPrefix,const char* work_dir);
	//生成PPI文件
	static bool CreatePPIFile(CProcessPart *pPart, const char* file_path);
};
extern BOOL GetSysParaFromReg(const char* sEntry, char* sValue);

