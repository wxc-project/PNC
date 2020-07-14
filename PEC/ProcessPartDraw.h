#pragma once
#include "f_ent.h"
#include "ProcessPart.h"
#include "I_DrawSolid.h"
#include "IPEC.h"

class CProcessPartDraw {
protected:
	BOOL m_bMainPartDraw;
	CProcessPart *m_pPart;
	IPEC *m_pBelongEditor;
	void InitSolidDrawUCS(ISolidSet *pSolidSet);
	virtual void NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)=0;
	virtual void EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet,COLORREF color=RGB(0,0,0),COLORREF boltColor=RGB(255,0,0))=0;
	virtual void ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)=0;
public:
	CProcessPartDraw();
	CProcessPartDraw(IPEC* pEditor,CProcessPart* pPart);
	~CProcessPartDraw();
	BOOL m_bOverturn;
	GECS dcs;						//�ڹ������ձ༭�������ľ�������ϵ�������Ĺ�����ͼ����ϵ
	static BOOL m_bDispBoltOrder;	//�Ƿ���ʾ��˨˳��
	BOOL IsMainPartDraw(){return m_bMainPartDraw;}
	void SetMainPartDraw(BOOL bMainPartDraw){m_bMainPartDraw=bMainPartDraw;}
	CProcessPart* GetPart(){return m_pPart;}
	CProcessPart* SetPart(CProcessPart *pPart){ return m_pPart=pPart; }
	void SetBelongEditor(IPEC *pEditor) {m_pBelongEditor=pEditor;}
	IPEC* GetBelongEditor(){return m_pBelongEditor;}
	virtual void OverturnPart(){;}
	virtual void Draw(I2dDrawing *p2dDraw,ISolidSet *pSolidSet);
	virtual void ReDraw(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap,ISolidDraw* pSolidDraw){;}
	virtual void DimSize(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap);
};
typedef struct tagGRID_DATA_STRU{
	int data_type;	//0.�����ı�1.ʵʱ�ı�2.ʵʱ����3.��ͼ����
	long type_id;
	char sContent[MAX_PATH];
	double min_x,min_y,max_x,max_y;
	double fTextHigh,fHighToWide;
}GRID_DATA_STRU;
class CProcessAngleDraw : public CProcessPartDraw{
	f2dRect InsertDataByGridData(IDrawing *pDarwing,CProcessAngle *pAngle,f2dPoint insert_pos,GRID_DATA_STRU &grid_data);
	f2dRect InsertJgProcessCardTbl(IDrawing *pDarwing,CProcessAngle *pAngle,f2dPoint insert_pos=f2dPoint(0,0));
	f2dRect InsertJgProcessCardTbl2(IDrawing *pDrawing,CProcessAngle *pAngle,f2dPoint insert_pos=f2dPoint(0,0));
	void	InsertJgProcessSketch(IDrawing *pDrawing,CProcessAngle *pAngle,f2dRect rect);
protected:
	void NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet);
	void EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet,COLORREF color=RGB(0,0,0),COLORREF boltColor=RGB(255,0,0));
	void ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet);
	//
	void ReDraw(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap,ISolidDraw* pSolidDraw);
public:
	CProcessAngleDraw();
	~CProcessAngleDraw();
	int angle_AB;	//�Ǹ�AB����XY֫�Ķ�Ӧ��ϵ
	void OverturnPart(){m_bOverturn=!m_bOverturn;}
	static char m_sAngleProcessCardPath[MAX_PATH];	//�Ǹֹ��տ�·��
};
class CProcessPlateDraw : public CProcessPartDraw{
	GECS mcs;					//Machine Coord System�����Ǹְ壬��ʾ�ְ�NC��������ʱ������ϵ
protected:
	void NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet);
	void EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet,COLORREF color=RGB(0,0,0),COLORREF boltColor=RGB(255,0,0));
	void ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet);
	//
	void ReDraw(I2dDrawing* p2dDraw,ISolidSnap* pSolidSnap,ISolidDraw* pSolidDraw);
	void ReDrawEdge(IDrawing* pDrawing,ISolidDraw* pSolidDraw,PROFILE_VER* pStartVer,PROFILE_VER* pEndVer);
public:
	CProcessPlateDraw();
	~CProcessPlateDraw();
	static char m_sPlateProcessCardPath[MAX_PATH];	//�ְ幤�տ�·��
	//NCģʽ��Ҫ�ĺ���
	void RotateAntiClockwise();
	void RotateClockwise();
	void OverturnPart();
	GECS GetMCS();
	void InitMkRect();
	void DrawCuttingTrack(I2dDrawing *p2dDraw,ISolidSet *pSolidSet,double interval=0.5);
};
