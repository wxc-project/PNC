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
	GECS dcs;						//在构件工艺编辑器环境的绝对坐标系下描述的构件绘图坐标系
	static BOOL m_bDispBoltOrder;	//是否显示螺栓顺序
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
	int data_type;	//0.标题文本1.实时文本2.实时数据3.草图区域
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
	int angle_AB;	//角钢AB面与XY肢的对应关系
	void OverturnPart(){m_bOverturn=!m_bOverturn;}
	static char m_sAngleProcessCardPath[MAX_PATH];	//角钢工艺卡路径
};
class CProcessPlateDraw : public CProcessPartDraw{
	GECS mcs;					//Machine Coord System尤其是钢板，表示钢板NC数据生成时的坐标系
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
	static char m_sPlateProcessCardPath[MAX_PATH];	//钢板工艺卡路径
	//NC模式需要的函数
	void RotateAntiClockwise();
	void RotateClockwise();
	void OverturnPart();
	GECS GetMCS();
	void InitMkRect();
	void DrawCuttingTrack(I2dDrawing *p2dDraw,ISolidSet *pSolidSet,double interval=0.5);
};
