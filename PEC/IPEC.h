#pragma once

#ifdef PEC_EXPORTS
#define PEC_API __declspec(dllexport)
#else
#define PEC_API __declspec(dllimport)
#endif

#include "I_DrawSolid.h"

//设计思路
//0.原则上应集成PlateNC,构件编辑器,试组装平台中构件工艺编辑，质检
//1.允许通用工件编辑环境中同时绘制多个构件(主要指钢板)
//2.建立CProcessPart类与CLDSPart类及数据交换标准中的构件基本&工艺信息缓存相对应
//3.建立CProcessPartDraw类,其中包含CProcessPart信息,同时还包含工艺草图绘制信息、工作区显示绘制信息、NC加工工位信息
//4.建立IPEC接口类及相应的实例类
struct PEC_API IPEC{
	//绘制模式
	static const WORD DRAW_MODE_EDIT		= 1;	//编辑模式
	static const WORD DRAW_MODE_NC			= 2;	//NC数据模式(钢板：显示挡板，角钢：显示AB面)
	static const WORD DRAW_MODE_PROCESSCARD	= 3;	//工艺草图模式(显示标注信息,角钢绘制启用长度压缩)
	//命令类型
	static const WORD CMD_VIEW_PART_FEATURE		= 1;
	static const WORD CMD_SPEC_WCS_ORIGIN		= 2;
	static const WORD CMD_SPEC_WCS_AXIS_X		= 3;
	static const WORD CMD_SPEC_WCS_AXIS_Y		= 4;
	static const WORD CMD_DEL_PART_FEATURE		= 5;
	static const WORD CMD_INSERT_PLATE_VERTEX	= 6;
	static const WORD CMD_CAL_PLATE_HUOQU_POS	= 7;
	static const WORD CMD_ROLL_PLATE_EDGE		= 8;
	static const WORD CMD_BEND_PLATE			= 9;
	static const WORD CMD_DIM_SIZE				= 10;
	static const WORD CMD_DEF_PLATE_CUT_IN_PT	= 11;
	static const WORD CMD_DRAW_CUTTING_TRACK	= 12;
	//
	static const WORD CMD_OVERTURN_PART				= 101;
	static const WORD CMD_ROTATEANTICLOCKWISE_PLATE	= 102;
	static const WORD CMD_ROTATECLOCKWISE_PLATE		= 103;
	static const WORD CMD_NWE_BOLT_HOLE				= 104;
	static const WORD CMD_CAL_PLATE_PROFILE			= 105;
	static const WORD CMD_OTHER						= 10000;

	virtual char GetCurPartType()=0;
	//钢板操作
	virtual void GetPlateNcUcs(GECS& cs)=0;
	//UI 绘图环境
	virtual bool InitDrawEnviornment(HWND hWnd,I2dDrawing* p2dDraw,ISolidDraw* pSolidDraw,
		ISolidSet* pSolidSet,ISolidSnap* pSolidSnap,ISolidOper* pSolidOper)=0;
	virtual int SetDrawMode(WORD mode)=0;	//1.编辑模式，2.NC模式，3.工艺草图模式
	virtual int GetDrawMode()=0;
	virtual bool AddProcessPart(CBuffer &processPartBuffer,DWORD key)=0;		//向编辑器中增加当前绘制显示的工艺构件
	virtual bool GetProcessPart(CBuffer &partBuffer,int serial=0)=0;
	virtual bool UpdateProcessPart(CBuffer &partBuffer,int serial=0)=0;
	virtual void ClearProcessParts()=0;		//清空编辑器中当前工艺构件集
	virtual void Draw()=0;
	virtual void ReDraw(int serial=0)=0;
	virtual bool SetWorkCS(GECS cs)=0;
	virtual void SetCallBackPartModifiedFunc(bool (*func)(WORD cmdType))=0;
	virtual bool SetCallBackProcessPartModified()=0;
	virtual bool SetCallBackOperationFunc()=0;		//Mouse or keyboard input->CWnd->DrawSolid->CWnd->PartsEditor
	virtual int GetSerial()=0;
	virtual void ExecuteCommand(WORD cmdType)=0;
	//参数设置
	virtual void SetProcessCardPath(const char* sCardDxfFilePath,BYTE angle0_plate1=0)=0;
	virtual void SetDrawBoltMode(BOOL bDisplayOrder)=0;
	virtual void UpdateJgDrawingPara(const char* para_buf,int buf_len)=0;
};

class PEC_API CPartsEditorFactory{
public:
	static IPEC* CreatePartsEditorInstance();
	static IPEC* PartsEditorFromSerial(long serial);
	static BOOL Destroy(long h);
};