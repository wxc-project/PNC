// DragEntSet.h: interface for the CDragEntSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRAGENTSET_H__D92C66C8_D139_42A1_B476_BA1DAF61598B__INCLUDED_)
#define AFX_DRAGENTSET_H__D92C66C8_D139_42A1_B476_BA1DAF61598B__INCLUDED_

#include "dbid.h"
#include "dbsymtb.h"
#include "adsdef.h"	// Added by ClassView
#include "f_ent_list.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef struct tagSETUPDRAWINGDIM_STRU
{
	AcDbObjectId startPointId;
	AcDbObjectId endPointId;
	double dimPosOffset;	//标注位置自基点的偏移量（自定位点沿Y轴方向的偏移量)
	double dist;			//间距
	BOOL bDisplayDimLine;	//是否显示尺寸线
	tagSETUPDRAWINGDIM_STRU(){memset(this,0,sizeof(tagSETUPDRAWINGDIM_STRU));}
}SETUPDRAWINGDIM_STRU;
typedef struct tagSETUPPOSDIM_STRU
{
	AcDbObjectId posPointId;
	double dimPosOffset;	//标注位置自基点的偏移量（自定位点沿Y轴方向的偏移量)
	double posDist;			//定位点自杆件起始端的定位距离
	BOOL bDisplayDimLine;	//是否显示尺寸线
	tagSETUPPOSDIM_STRU(){memset(this,0,sizeof(tagSETUPPOSDIM_STRU));}
}SETUPPOSDIM_STRU;
typedef struct tagSETUPANGULARDIM_STRU
{
	AcDbObjectId vertexPointId;		//夹角的角顶点标记点
	AcDbObjectId lineStartPointId;	//夹角的起始边标记点
	AcDbObjectId lineEndPointId;	//夹角的终止边标记点
	AcDbObjectId arcPointId;		//标注角度的标注圆弧标记点
	tagSETUPANGULARDIM_STRU(){memset(this,0,sizeof(tagSETUPANGULARDIM_STRU));}
}SETUPANGULARDIM_STRU;

class CDrawLineAngleTagInfo;
class CDrawCmdRunTagInfo
{
public:
	void *pDrawCmd;
	CDrawCmdRunTagInfo(){pDrawCmd=NULL;}
	virtual bool IsDrawLinePart(){return false;}
	virtual bool IsDrawAnglePart(){return false;}
	virtual f3dPoint DimDrawPos(){return f3dPoint(0,0,0);}
	virtual f3dPoint StartDrawPos(){return f3dPoint(0,0,0);}
	virtual f3dPoint EndDrawPos(){return f3dPoint(0,0,0);}
	CDrawLineAngleTagInfo* AngleDrawTagInfo(){return (CDrawLineAngleTagInfo*)this;}
};
class CDrawDimensionTagInfo : public CDrawCmdRunTagInfo
{
public:
	f3dPoint m_xDimDrawPos;
	virtual f3dPoint DimDrawPos(){return m_xDimDrawPos;}
};
class CDrawPartTagInfo : public CDrawCmdRunTagInfo
{
public:
	//构件绘制方式:
	//角钢件:'X':绘制里铁X肢;'x':绘制外铁X肢;'Y':绘制里铁Y肢;'y':绘制外铁Y肢;'Z'可其它表示绘制截面图
	//板件:0.平放绘制;1.竖直截面绘制
	//螺栓:0.竖直绘制;1.倒(平)放绘制;2.未找到螺栓图符,按圆孔绘制
	char m_cDrawStyle;
	CDrawPartTagInfo(){m_cDrawStyle=0;pDrawCmd=NULL;}
	virtual bool IsDrawLinePart(){return false;}
	virtual bool IsDrawAnglePart(){return false;}
	virtual f3dPoint DimDrawPos(){return f3dPoint(0,0,0);}
	virtual f3dPoint StartDrawPos(){return f3dPoint(0,0,0);}
	virtual f3dPoint EndDrawPos(){return f3dPoint(0,0,0);}
};
class CDrawLinePartTagInfo : public CDrawPartTagInfo
{
public:
	f3dPoint m_xStartDrawPos,m_xEndDrawPos;	//始终端的绘制基准位置
	virtual bool IsDrawLinePart(){return true;}
	virtual f3dPoint StartDrawPos(){return m_xStartDrawPos;}
	virtual f3dPoint EndDrawPos(){return m_xEndDrawPos;}
};
class CDrawLineAngleTagInfo : public CDrawLinePartTagInfo
{
public:
	long start_std_odd,end_std_odd;
	CDrawLineAngleTagInfo(){start_std_odd=end_std_odd=0;}
	virtual bool IsDrawAnglePart(){return true;}
};
class CDragEntSet  
{
	bool m_bStartRecord;//开始记录
	int m_nClearDegree;	//拖拽集合被清空次数
	AcDbObjectId blockId;
	AcDbBlockTableRecord *m_pRecordingBlockTableRecord;
public:
	CDrawCmdRunTagInfo *m_pDrawCmdTagInfo;
	f3dPoint axis_vec;
	ATOM_LIST<SETUPDRAWINGDIM_STRU> dimSpaceList;	//构件上的间距标注
	ATOM_LIST<SETUPPOSDIM_STRU> dimPosList;		//杆件上某一定位点的自杆件起始端的定位标注
	ATOM_LIST<SETUPANGULARDIM_STRU> dimAngleList;		//杆件上某一定位点的自杆件起始端的定位标注
	ads_name drag_ent_set;	//用于存储随时拖拽刚生成的实体集
	BOOL GetEntId(long i,AcDbObjectId &ent_id);
	BOOL GetEntName(long i,ads_name ent_name);
	long GetEntNum();
	BOOL Del(ads_name delEntName);
	BOOL Del(AcDbObjectId delEntId);
	BOOL Add(ads_name newEntName);
	BOOL Add(AcDbObjectId newEntId);
	bool SetReocrdState(bool start=true){return m_bStartRecord=start;}
	bool BeginBlockRecord(const char* blkname=NULL);
	AcDbBlockTableRecord *RecordingBlockTableRecord(){return m_pRecordingBlockTableRecord;}
	bool EndBlockRecord(AcDbBlockTableRecord* pBlockTableRecord,double* insert_pos,double zoomcoef=1.0);
	BOOL AppendAcDbEntity(AcDbBlockTableRecord *pBlockTableRecord,
		AcDbObjectId& outputId, AcDbEntity* pEntity);
	BOOL IsInDragSet(ads_name ent_name);
	void GetDragScope(SCOPE_STRU& scope);	//获取拖拽集合范围
	//从拖拽集合尾部删除指定个数的实体	wht 10-11-30
	BOOL DelEntFromTail(int nEntNum,BOOL bDeleteEnt=FALSE);	
	//获取拖拽集合尾部指定个数的实体所在的区域 wht 10-11-30
	BOOL GetTailEntScope(int nEntNum,ads_point &L_T,ads_point &R_B);
	//获取拖拽集合实体所在的区域 
	BOOL GetDragEntSetScope(ads_point &L_T,ads_point &R_B);
	void InitClearDegree(){m_nClearDegree=0;}
	int GetClearDegree(){return m_nClearDegree;}
	void ClearEntSet();
	CDragEntSet();
	virtual ~CDragEntSet();
};
extern CDragEntSet DRAGSET;

#endif // !defined(AFX_DRAGENTSET_H__D92C66C8_D139_42A1_B476_BA1DAF61598B__INCLUDED_)
