#pragma once
#include "XhCharString.h"
#include "PropListItem.h"
#include "PPEModel.h"

struct FILE_INFO_PARA
{
	CXhChar100 m_sThick;
	DWORD m_dwFileFlag;
	//以下参数目前只用于控制激光复合机DXF文件输出 wht 19-10-22
	BOOL m_bOutputBendLine;
	BOOL m_bOutputBendType;
public:
	FILE_INFO_PARA();
	DWORD AddFileFlag(DWORD dwFlag);
	bool IsValidFile(DWORD dwFlag);
	BOOL SetOutputBendLine(BOOL bValue) { return (m_bOutputBendLine = bValue); }
	BOOL IsOutputBendLine() { return m_bOutputBendLine; }
	BOOL SetOutputBendType(BOOL bValue) { return (m_bOutputBendType = bValue); }
	BOOL IsOutputBendType() { return m_bOutputBendType; }
};
class CSysPara : public ISysPara
{
public:
	CSysPara(void);
	~CSysPara(void);
public:
	//DockPage停靠参数
	struct DOCKPAGEPOS
	{
		BOOL bDisplay;
		UINT uDockBarID;
	};
	struct DOCK_ENV{
		DOCKPAGEPOS pagePartList,pageProp;
		short m_nLeftPageWidth,m_nRightPageWidth;
	}dock;
	//
	double m_fHoleIncrement;	//孔径增量
	//NC数据参数
	struct NC_PARA{
		//钢板NC设置
		BOOL m_bAutoSortHole;	//是否自动优化螺栓孔顺序
		BOOL m_bSortByHoleD;	//是否按孔径进行分组优化
		BYTE m_ciGroupSortType;	//分组排序方案 0.按孔径从大到小|1.按距离从远到进
		double m_fBaffleHigh;	//挡板高度
		BOOL m_bDispMkRect;		//显示字盒
		double m_fMKHoleD,m_fMKRectW,m_fMKRectL;
		BYTE m_iNcMode;			//切割下料(无孔)|板床加工(有孔) |激光加工
		double m_fLimitSH;		//特殊孔径界限值
		BOOL m_bNeedSH;			//是否需要特殊孔
		BOOL m_bNeedMKRect;		//是否保留字盒
		BOOL m_iDxfMode;		//DXF文件分类 0.不分类 1.按厚度
		CXhChar50 m_sThickToPBJ;		//冲孔工艺板厚设定
		CXhChar50 m_sThickToPMZ;		//钻孔工艺板厚设定
		double m_fShapeAddDist;	//轮廓变增大值
		//钢板输出设置
		BOOL m_bFlameCut;		//火焰切割
		FILE_INFO_PARA m_xFlamePara;
		BOOL m_bPlasmaCut;		//等离子切割
		FILE_INFO_PARA m_xPlasmaPara;
		BOOL m_bPunchPress;		//冲床加工
		FILE_INFO_PARA m_xPunchPara;
		BOOL m_bDrillPress;		//钻床加工
		FILE_INFO_PARA m_xDrillPara;
		//
		FILE_INFO_PARA m_xLaserPara;
		//角钢NC设置
		CString m_sNcDriverPath;
		//钢板配置文件路径
		CString m_sPlateConfigFilePath;
	}nc;
	struct PBJ_PARA{
		BOOL m_bIncVertex;	//输出轮廓点
		BOOL m_iPbjMode;	//输出模式
		BOOL m_bAutoSplitFile;	//螺栓孔种类超过三种后，自动拆分生成多个文件
		BOOL m_bIncSH;		//输出特殊孔
		BOOL m_bMergeHole;	//将数量较少的标准孔合并使用同一冲头加工，将非标孔按标准孔加工 wht 19-07-25
	}pbj;
	struct PMZ_PARA {
		int m_iPmzMode;		//文件模式：0.单文件 1.多文件
		BOOL m_bIncVertex;	//输出轮廓点
		BOOL m_bPmzCheck;	//输出预审PMZ格式
	}pmz;
	//火焰切割
	BYTE m_cDisplayCutType;		//当前显示类型
	struct FLAME_CUT_PARA{
		CXhChar16 m_sOutLineLen;
		CXhChar16 m_sIntoLineLen;
		BOOL m_bInitPosFarOrg;
		BOOL m_bCutPosInInitPos;
		WORD m_wEnlargedSpace;
		BOOL m_bCutSpecialHole;
	}flameCut,plasmaCut;
	//孔径增大值
	struct HOLE_INCREMENT{
		double m_fDatum;	//常规
		double m_fM12;		//M12
		double m_fM16;		//M16
		double m_fM20;		//M20
		double m_fM24;		//M24
		double m_fCutSH;	//切割式特殊孔径增大值
		double m_fProSH;	//板床式特殊孔径增大值
	}holeIncrement;
	//颜色方案
	struct COLOR_MODE{
		COLORREF crLS12;
		COLORREF crLS16;
		COLORREF crLS20;
		COLORREF crLS24;
		COLORREF crOtherLS;
		COLORREF crMark;
		COLORREF crEdge;
		COLORREF crText;
	}crMode;
	//角钢工艺卡参数
	struct JGDRAWING_PARA{
		int iDimPrecision;			//尺寸精确度
		double fRealToDraw;			//基础绘图比例尺＝实际尺寸/绘图尺寸，如1:20时，fRealToDraw=20
		double	fDimArrowSize;		//尺寸标注箭头长
		double fTextXFactor;
		int		iPartNoFrameStyle;	//编号框类型 //0.圆圈 1.带圆弧的矩形框 2.矩形框	3.自动判断
		double	fPartNoMargin;		//构件编号与编号框之间的间隙值 
		double	fPartNoCirD;		//构件编号圈直径
		double fPartGuigeTextSize;	//构件规格文字高
		int iMatCharPosType;
		BOOL bModulateLongJg;		//调整角钢长度 暂未使用，由iJgZoomSchema代替该变量 wht 11-05-07
		int iJgZoomSchema;			//角钢绘制方案，0.1:1绘制 1.使用构件图比例 2.长宽同比缩放 3.长宽分别缩放
		BOOL bMaxExtendAngleLength;	//最大限度延伸角钢绘制长度
		double fLsDistThreshold;	//角钢长度自动调整螺栓间距阈值(大于此间距时就要进行调整);
		double fLsDistZoomCoef;		//螺栓间距缩放系数
		BOOL bOneCardMultiPart;		//角钢允许一卡多件情况
		int  iJgGDimStyle;			//0.始端标注  1.中间标注 2.自动判断
		int  nMaxBoltNumStartDimG;	//集中在始端标注准距支持的最大螺栓数
		int  iLsSpaceDimStyle;		//0.X轴方向	  1.Y轴方向  2.自动判断 3.不标注  4.无标注内容(X轴方向)  4.仅标尺寸线，无标注内容(X轴方向)主要用于江津(北)塔厂质检是盲检填数
		int  nMaxBoltNumAlongX;		//沿X轴方向标注支持的最大螺栓个数
		BOOL bDimCutAngle;			//标注角钢切角
		BOOL bDimCutAngleMap;		//标注角钢切角示意图
		BOOL bDimPushFlatMap;		//标注压扁示意图
		BOOL bJgUseSimpleLsMap;		//角钢使用简化螺栓图符
		BOOL bDimLsAbsoluteDist;	//标注螺栓绝对尺寸
		BOOL bMergeLsAbsoluteDist;	//合并相邻等距螺栓绝对尺寸 江津及增立提出:有时也需要标 wjh-2014.6.9
		BOOL bDimRibPlatePartNo;	//标注角钢肋板编号
		BOOL bDimRibPlateSetUpPos;	//标注角钢肋板安装位置
		//切角标注样式一
		//切角标注样式二 B:端头尺寸 L:肢边尺寸 C:筋边尺寸 
		//BXL 切角  CXL 切肢 BXC 切筋  切大角=切筋+切肢
		int	 iCutAngleDimType;		//切角标注样式 0.样式一  1.样式二 wht 10-11-01
		//
		BOOL bDimKaiHe;				//标注角钢开合角
		BOOL bDimKaiheAngleMap;		//标注角钢开合角示意图
		double fKaiHeJiaoThreshold; //开合角标注阈值(°) wht 11-05-06
		//新增开合角标注开关 wht 12-03-13
		BOOL bDimKaiheSumLen;		//标注开合区域总长
		BOOL bDimKaiheAngle;		//标注开合度数	
		BOOL bDimKaiheSegLen;		//标注开合区域分段长
		BOOL bDimKaiheScopeMap;		//标注开合区域标识符
		//
		CString sAngleCardPath;
	}jgDrawing;
	struct FONT{
		double  fTextHeight;		//普通文字字体高度
		double	fDimTextSize;		//长度尺寸标注文本高
		double	fPartNoTextSize;	//构件编号文字高
		double  fDxfTextSize;		//DXF文件中文字字体高度
	}font;
	BOOL Read(CString file_path);	//读配置文件
	BOOL Write(CString file_path);	//写配置文件
	void WriteSysParaToReg(LPCTSTR lpszEntry);	//保存共用参数至注册表
	void ReadSysParaFromReg(LPCTSTR lpszEntry);	//提取共用参数从注册表
	void UpdateHoleIncrement(double fHoleInc);
	DECLARE_PROP_FUNC(CSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100);//通过属性Id获取属性值
	//
	virtual double GetCutInLineLen(double fThick,BYTE cType=-1);
	virtual double GetCutOutLineLen(double fThick,BYTE cType=-1);
	virtual int GetCutEnlargedSpaceLen(BYTE cType=-1);
	virtual BOOL GetCutInitPosFarOrg(BYTE cType=-1);
	virtual BOOL GetCutPosInInitPos(BYTE cType=-1);
	virtual BOOL IsCutSpecialHole(BYTE cType=-1);
	void AngleDrawingParaToBuffer(CBuffer &buffer);
	//钢板NC模式设置
	DWORD AddNcFlag(BYTE ciNcFlag) {
		nc.m_iNcMode |= ciNcFlag;
		return nc.m_iNcMode;
	}
	bool IsValidNcFlag(BYTE ciNcFlag) {
		if ((ciNcFlag&nc.m_iNcMode) > 0)
			return true;
		else
			return false;
	}
};
extern CSysPara g_sysPara;
