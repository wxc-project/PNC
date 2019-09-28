#pragma once
#include "PPEModel.h"
#include "NcPlate.h"

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
public:
	//钢板NC模式
	static const BYTE CUT_MODE		= 0x01;	 //切割下料
	static const BYTE PROCESS_MODE	= 0x02;	 //板床加工
	static const BYTE LASER_MODE	= 0x04;	 //激光复合机
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
	static BOOL m_bDisplayLsOrder;		//显示螺栓顺序（用于测试使用）
	static BOOL m_bSortHole;			//对螺栓进行排序（导出PBJ文件进行控制）
	static BOOL m_bDeformedProfile;		//考虑火曲变形
	static CString m_sExportPartInfoKeyStr;	//钢板输出明细
	static double m_fHoleIncrement;		//孔径增大值 wht 19-07-25
public:
	//static BOOL GetSysParaFromReg(const char* sEntry, char* sValue);
	static void InitStoreMode(CHashList<CDrillBolt>& hashDrillBoltByD,ARRAY_LIST<double> &holeDList,BOOL bIncSH=TRUE);
	static void RefreshPlateHoles(CProcessPlate *pPlate,BOOL bSortByHoleD=TRUE,BYTE ciAlgType = 0);
	static BOOL IsNeedCreateHoleFile(CProcessPlate *pPlate,BYTE ciHoleProcessType);
	static void DeformedPlateProfile(CProcessPlate *pPlate);
	//钢板操作
	static bool CreatePlateTtpFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateTtpFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateDxfFile(CProcessPlate *pPlate,const char* file_path,int dxf_mode);
	static void CreatePlateDxfFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	//添加济南法特钢板程序(*.wkf)导出 wht 19-06-11
	static bool CreatePlateWkfFile(CProcessPlate *pPlate, const char* file_path);
	static void CreatePlateWkfFiles(CPPEModel *pModel, CXhPtrSet<CProcessPlate> &plateSet, const char* work_dir);
#ifdef __PNC_
	static void InitDrillBoltHashTbl(CProcessPlate *pPlate, CHashList<CDrillBolt>& hashDrillBoltByD,
									 BOOL bMergeHole = FALSE, BOOL bIncSpecialHole=TRUE, BOOL bDrillGroupSort=FALSE);
	static void OptimizeBolt(CProcessPlate *pPlate,CHashList<CDrillBolt>& hashDrillBoltByD,
							 BOOL bSortByHoleD=TRUE,BYTE ciAlgType=0,BOOL bMergeHole=FALSE,BOOL bIncSpecialHole=TRUE);
	static void CreatePlatePncDxfFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlatePbjFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlatePbjFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlatePmzFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlatePmzCheckFile(CProcessPlate *pPlate, const char* file_path);
	static void CreatePlatePmzFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateTxtFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateTxtFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateNcFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateCncFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateCncFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static void CreatePlateFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir,int nFileType);
	static void ImprotPlateCncOrTextFile(CProcessPlate *pPlate,const char* file_path);
#endif
	static void CreateAllPlateFiles(int nFileType);
	//角钢操作
	static void CreateAngleNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessAngle> &angleSet,const char* drv_path,const char* sPartNoPrefix,const char* work_dir);
	static void CreateAllAngleNcFile(CPPEModel *pModel,const char* drv_path,const char* sPartNoPrefix,const char* work_dir);
	//生成PPI文件
	static bool CreatePPIFile(CProcessPart *pPart,const char* file_path);
	static void CreatePPIFiles(CPPEModel *pModel,CXhPtrSet<CProcessPart> &partSet,const char* work_dir);
	static void CreateAllPPIFiles(CPPEModel *pModel,const char* work_dir);
};
extern BOOL GetSysParaFromReg(const char* sEntry, char* sValue);

