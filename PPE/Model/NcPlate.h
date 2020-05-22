#pragma once
#include "ArrayList.h"
#include "ProcessPart.h"

#ifdef __PNC_
struct CUT_PT {
	static const BYTE EDGE_LINE = 0;
	static const BYTE EDGE_ARC = 1;
	static const BYTE HOLE_CIR = 2;	//整圆
	BYTE cByte;
	GEPOINT vertex;		//切点坐标
	GEPOINT center;		//圆心
	double radius;		//半径
	BOOL bClockwise;	//顺时针
public:
	CUT_PT() { 
		cByte = 0; 
		radius = 0; 
		bClockwise = FALSE;
	}
};
class CNCPlate
{
	CProcessPlate *m_pPlate;
	BYTE m_ciStartId;		//起始切割轮廓点序号
	GEPOINT m_xStartPt;		//切割起始点
public:
	struct CUT_EDGE_PATH
	{
		GEPOINT extraInVertex, extraOutVertex;		//额外的引入点，引出点
		GEPOINT cutInVertex, cutOutVertex;			//引入点、引出点
		GEPOINT cutStartVertex;						//切割原点
		ARRAY_LIST<CUT_PT> cutPtArr;
	};
	struct CUT_HOLE_PATH
	{
		GEPOINT ignitionPt;		//点火位置
		GEPOINT orgPt;
		double fHoleR;
		ARRAY_LIST<CUT_PT> cutPtArr;
		CUT_HOLE_PATH() { fHoleR = 0; }
	};
public:
	int m_iNo;				//程序号
	BYTE m_cCSMode;			//0.横X+纵Y+ 1.横Y+纵X-
	BOOL m_bClockwise;		//TRUE 顺时针，FALSE 逆时针
	int m_nInLineLen;		//引入长度
	int m_nOutLineLen;		//引出长度
	int m_nExtraInLen;		//额外的引入长度
	int m_nExtraOutLen;		//额外的引出长度
	int m_nEnlargedSpace;	//轮廓边增大值
	BOOL m_bCutSpecialHole;	//是否切割大孔
	BOOL m_bCutFullCircle;	//是否切整圆
	CUT_EDGE_PATH m_xCutEdge;				//切割轮廓边路径
	ATOM_LIST<CUT_HOLE_PATH> m_xCutHole;	//切割大孔路径
public:
	CNCPlate(CProcessPlate *pPlate, int iNo = 0);
	//
	void SetCutStartPt(GEPOINT start_pt) { m_xStartPt = start_pt; }
	GEPOINT GetCutStartPt() { return m_xStartPt; }
	GEPOINT ProcessPoint(const double* coord);
	void InitPlateNcInfo();
	bool CreatePlateTxtFile(const char* file_path);
	bool CreatePlateNcFile(const char* file_path);
public:
	static bool InitVertextListByNcFile(CProcessPlate *pPlate,const char* file_path);
};
#endif