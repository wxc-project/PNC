#pragma once
#include "ArrayList.h"
#include "ProcessPart.h"

#ifdef __PNC_
struct CUT_PT {
	static const BYTE EDGE_LINE = 0;
	static const BYTE EDGE_ARC = 1;
	static const BYTE HOLE_CIR = 2;	//��Բ
	BYTE cByte;
	GEPOINT vertex;		//�е�����
	GEPOINT center;		//Բ��
	double radius;		//�뾶
	BOOL bClockwise;	//˳ʱ��
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
	BYTE m_ciStartId;		//��ʼ�и����������
	GEPOINT m_xStartPt;		//�и���ʼ��
public:
	struct CUT_EDGE_PATH
	{
		GEPOINT extraInVertex, extraOutVertex;		//���������㣬������
		GEPOINT cutInVertex, cutOutVertex;			//����㡢������
		GEPOINT cutStartVertex;						//�и�ԭ��
		ARRAY_LIST<CUT_PT> cutPtArr;
	};
	struct CUT_HOLE_PATH
	{
		GEPOINT ignitionPt;		//���λ��
		GEPOINT orgPt;
		double fHoleR;
		ARRAY_LIST<CUT_PT> cutPtArr;
		CUT_HOLE_PATH() { fHoleR = 0; }
	};
public:
	int m_iNo;				//�����
	BYTE m_cCSMode;			//0.��X+��Y+ 1.��Y+��X-
	BOOL m_bClockwise;		//TRUE ˳ʱ�룬FALSE ��ʱ��
	int m_nInLineLen;		//���볤��
	int m_nOutLineLen;		//��������
	int m_nExtraInLen;		//��������볤��
	int m_nExtraOutLen;		//�������������
	int m_nEnlargedSpace;	//����������ֵ
	BOOL m_bCutSpecialHole;	//�Ƿ��и���
	BOOL m_bCutFullCircle;	//�Ƿ�����Բ
	BOOL m_bGrindingArc;	//���Ǵ�ĥ
	CUT_EDGE_PATH m_xCutEdge;				//�и�������·��
	ATOM_LIST<CUT_HOLE_PATH> m_xCutHole;	//�и���·��
private:
	void AppendCutPt(GEPOINT& prePt, GEPOINT curPt);
	void AppendCutPt(GEPOINT& prePt, GEPOINT curPt, GEPOINT center, GEPOINT norm);
	void InitCutHoleInfo(GEPOINT& prevPt, CProcessPlate& destPlate);
	void InitCutEdgePtInfo(GEPOINT& prevPt, CProcessPlate& destPlate);
	void InitCutEdgePtInfoEx(GEPOINT& prevPt, CProcessPlate& destPlate);
public:
	CNCPlate(CProcessPlate *pPlate, int iNo = 0);
	//
	void SetCutStartPt(GEPOINT start_pt) { m_xStartPt = start_pt; }
	GEPOINT GetCutStartPt() { return m_xStartPt; }
	GEPOINT ProcessPoint(const double* coord);
	void InitPlateNcInfo();
	bool CreatePlateTxtFile(const char* file_path);
	bool CreatePlateNcFile(const char* file_path);
	//
	static GECS GetMCS(CProcessPlate& tempPlate, double fMinDistance, BOOL bGrindingArc);
public:
	static bool InitVertextListByNcFile(CProcessPlate *pPlate,const char* file_path);
};
#endif