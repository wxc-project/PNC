#pragma once
#include "ArrayList.h"
#include "ProcessPart.h"

#ifdef __PNC_
class CNCPlate
{
	CProcessPlate *m_pPlate;
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
	GEPOINT m_xStartPt;		//
	GEPOINT m_ptCutter;			//刀头位置
	struct CUT_PT{
		static const BYTE EDGE_LINE		= 0;
		static const BYTE EDGE_ARC		= 1;
		static const BYTE HOLE_CUT_IN	= 2;
		static const BYTE HOLE_CUT_OUT	= 3;
		static const BYTE HOLE_CIR		= 4;	//整圆
		BYTE cByte;
		BOOL bHasExtraInVertex,bHasExtraOutVertex;
		GEPOINT extraInVertex,extraOutVertex;	//额外的引入点，引出点
		GEPOINT vertex,vertex2,vertex3;			//引入点、引出点、切割原点
		GEPOINT centerPt;
		double fSectorAngle;
		double radius;		//半径
		BOOL bClockwise;	//顺时针
		CUT_PT(){cByte=0;fSectorAngle=0;bHasExtraInVertex=bHasExtraOutVertex=FALSE;}
	};
	CUT_PT m_cutPt;
	ATOM_LIST<CUT_PT> m_xCutPtList;
	ATOM_LIST<CUT_PT> m_xCutHoleList;	//孔切割数据 wht 19-09-24
public:
	CNCPlate(CProcessPlate *pPlate, int iNo = 0);
	/*CNCPlate(CProcessPlate *pPlate,GEPOINT cutter_pos,int iNo=0,BYTE cCSMode=0,bool bClockwise=false,
			 int nInLineLen=0,int nOutLineLen=0,int nExtraInLen=0,int nExtraOutLen=0,int nEnlargedSpace=0,
			 BOOL bCutSpecialHole=FALSE);*/
	//
	void InitPlateNcInfo();
	bool CreatePlateTxtFile(const char* file_path);
	bool CreatePlateNcFile(const char* file_path);
public:
	static GEPOINT ProcessPoint(const double* coord,BYTE cCSMode=0);
	static bool InitVertextListByNcFile(CProcessPlate *pPlate,const char* file_path);
};
#endif