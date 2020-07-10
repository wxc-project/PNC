// PEC.h : PEC DLL 的主头文件
//

#pragma once

#include "HashTable.h"
#include "ProcessPartDraw.h"
#include "IPEC.h"

class CPEC : public IPEC
{
private:	//数据存储及文件管理
	int m_iCurFileIndex;						//当前正在编辑的文件索引
	CXhChar200 m_sFolderPath;					//主文件夹
	CStringArray m_arrPlateFileName;			//钢板构件数组
	CStringArray m_arrAngleFileName;			//角钢构件数组
	CString GetFilePathByIndex(int index);
	CString GetFileNameByIndex(int index);
	BOOL LoadPartFromFile(int iCurFileIndex);
private:	//UI环境
	int m_iNo;			//构件编辑器序号
	GECS m_workCS;		//工作坐标系，编辑构件时使用
	WORD m_wDrawMode;	//绘图模式 1.编辑模式，2.NC模式，3.工艺草图模式
	BYTE m_cPartType;	//当前编辑的构件类型
	HWND m_hWnd;
	I2dDrawing *m_p2dDraw;
	ISolidDraw *m_pSolidDraw;
	ISolidSet *m_pSolidSet;
	ISolidSnap *m_pSolidSnap;
	ISolidOper *m_pSolidOper;
	CSuperHashList<CProcessPart> m_hashProcessPartBySerial;
	CSuperHashList<CProcessPartDraw> m_hashPartDrawBySerial;
	bool (*FirePartModify)(WORD cmdType);
	//
private:
	WORD m_wCurCmdType;	//当前任务类型
	//构件属性(点、线、圆、螺栓孔)
	void FinishViewVertex(IDbEntity* pEnt);
	void FinishViewLine(IDbEntity* pEnt);
	void FinishViewCir(IDbEntity* pEnt);
	void FinishViewLsCir(IDbEntity* pEnt);
	////插入、删除轮廓点
	//BOOL FinishInsertPlankVertex(fAtom *pAtom);
	//BOOL FinishDelPartFeature(fAtom *pAtom);
	//操作函数通过设置任务类型调用
	void RotateAntiClockwisePlate();
	void RotateClockwisePlate();
	void OverturnPart();
	void NewBoltHole();
	void CalPlateWrapProfile(CProcessPlate *pPlate,double angle_lim=60.0);
	void CalPlateWrapProfile();
	void SetSpecWCS();
	void ViewPartFeature();
	void DimSize();
	void DefPlateCutInPt();
	void DrawCuttingTrack();
public:
	CPEC(int iNo);
	~CPEC();
	WORD GetCurCmdType(){return m_wCurCmdType;}
	virtual char GetCurPartType();
	//钢板操作
	virtual void GetPlateNcUcs(GECS& cs);
	//UI 绘图环境
	virtual bool InitDrawEnviornment(HWND hWnd,I2dDrawing* p2dDraw,ISolidDraw* pSolidDraw,
									 ISolidSet* pSolidSet,ISolidSnap* pSolidSnap,ISolidOper* pSolidOper);
	virtual int SetDrawMode(WORD mode);	//1.编辑模式，2.NC模式，3.工艺草图模式
	virtual int GetDrawMode() {return m_wDrawMode;}
	virtual bool AddProcessPart(CBuffer &processPartBuffer,DWORD key);		//向编辑器中增加当前绘制显示的工艺构件
	virtual bool GetProcessPart(CBuffer &partBuffer,int serial=0);
	virtual bool UpdateProcessPart(CBuffer &partBuffer,int serial=0);
	virtual void ClearProcessParts();	//清空编辑器中当前工艺构件集
	virtual void Draw();
	virtual void ReDraw(int serial=0);
	virtual bool SetWorkCS(GECS cs);
	virtual void SetCallBackPartModifiedFunc(bool (*func)(WORD cmdType)){FirePartModify=func;}
	virtual bool SetCallBackProcessPartModified();
	virtual bool SetCallBackOperationFunc();		//Mouse or keyboard input->CWnd->DrawSolid->CWnd->PartsEditor
	virtual int GetSerial(){return m_iNo;}
	virtual void ExecuteCommand(WORD taskType);
	//参数设置
	virtual void SetDrawBoltMode(BOOL bDisplayOrder){CProcessPartDraw::m_bDispBoltOrder=bDisplayOrder;}
	virtual void SetProcessCardPath(const char* sCardDxfFilePath,BYTE angle0_plate1=0);
	virtual void UpdateJgDrawingPara(const char* para_buf,int buf_len);
	//
	static BOOL GetSysParaFromReg(const char* sEntry,char* sValue);
};
