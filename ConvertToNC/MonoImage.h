#pragma once
#include "list.h"
#include "ArrayList.h"
#include "MapArray.h"

/////////////////////////////////////////////////////////////////////////
static const BYTE DETECT_LEFT	= 0x01;	//X减小方向
static const BYTE DETECT_RIGHT	= 0x02;	//X增加方向
static const BYTE DETECT_UP		= 0x04;	//Y减小方向
static const BYTE DETECT_DOWN	= 0x08;	//Y增加方向
static const BYTE DETECT_X0		= 0x10;	//X保持不变
static const BYTE DETECT_Y0		= 0x20;	//Y保持不变
struct PRESET_OBJS1600{static const int COUNT=1600;};
template <class TYPE> class PRESET_ARRAY1600 : public PRESET_ARRAY<TYPE,PRESET_OBJS1600>
{
};
struct PIXEL_RELAOBJ{
	union{
		long hRod;
		long hNode;
	};
	char ciObjType;			//0.节点；1.杆件
	bool bTipPixel;			//是否为端头点，false表示为杆件中间的像素点
	bool bReverseDirection;	//绘制走向是否与杆件始->终方向相反
	float zDepth;			//zDepth越小越靠前，越大表示越靠后
	PIXEL_RELAOBJ(long hObj=0,float _zDepth=0.0,bool tippoint=false){hRod=hObj;zDepth=_zDepth;bTipPixel=tippoint;bReverseDirection=false;ciObjType=1;}
};
class CMonoImage{
protected:
	bool m_bBlackPixelIsTrue;	//像素比特位为True表示黑点
	bool m_bExternalData;
	long m_nWidth,m_nHeight,m_nEffWidth;
	BYTE* m_lpBitMap;
	BYTE m_ciBitCount;
	PIXEL_RELAOBJ* m_parrRelaObjs;
public:
	CMonoImage(BYTE* lpBitMap=NULL,int width=0,int height=0,bool black_is_true=true,BYTE ciBitCount=1);
	virtual ~CMonoImage();
	virtual void SetRelaObjMap(PIXEL_RELAOBJ* pxRelaObjMap){m_parrRelaObjs=pxRelaObjMap;}
	virtual BYTE BitCount(){return m_ciBitCount;}
	virtual void InitBitImage(BYTE* lpBitMap=NULL,int width=0,int height=0,bool black_is_true=true);
	virtual void MirrorByAxisY();
	//virtual void MirrorRelaObjsByAxisY();
	virtual bool BlackIsTrue(){return m_bBlackPixelIsTrue;}
	virtual bool BlackBitIsTrue(){return m_bBlackPixelIsTrue;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual bool SetPixelRelaObj(int i,int j,PIXEL_RELAOBJ& relaObj);
	virtual PIXEL_RELAOBJ* GetPixelRelaObj(int i,int j);
	virtual PIXEL_RELAOBJ* GetPixelRelaObjEx(int& i,int& j,BYTE ciRadiusOfSearchNearFrontObj=1,const int* piExcludeXI=NULL,const int* pjExcludeYJ=NULL);
	virtual BYTE* GetImageMap(){return m_lpBitMap;}
	virtual int GetImageWidth(){return m_nWidth;}		//获取图像每行的有效象素数
	virtual int GetImageHeight(){return m_nHeight;}		//获取图像每列的有效象素数
	__declspec(property(get=GetImageWidth)) int Width;
	__declspec(property(get=GetImageHeight)) int Height;
	void DrawLine(const double* start,const double* end,long hRelaRod=0);
	void DrawPoint(const double* point,long hRelaNode=0);
	virtual int ReadMonoBmpFile(const char* fileName);
	virtual int WriteMonoBmpFile(const char* fileName);
	static int WriteMonoBmpFile(const char* fileName, unsigned int width,unsigned effic_width, 
							   unsigned int height, unsigned char* image);
};
class CByteMonoImage : public CMonoImage
{	//增加探测内部孤岛的代码
public:
	CByteMonoImage(BYTE* lpBitMap=NULL,int width=0,int height=0,bool black_is_true=true);
	virtual ~CByteMonoImage();
	virtual BYTE BitCount(){return 8;}
	struct ISLAND{
		//bool boundary;
		WORD maxy,miny;
		double x,y,count;
		ISLAND(){Clear();}
		void Clear(){x=y=count=maxy=miny=0;}//boundary=false;}
	};	//内部空白孤岛特征区域
	struct PIXEL{
		BYTE *pcbPixel;
		short xI,yJ;	//不能为WORD，否则无法区分负值越界问题
		PIXEL(BYTE* _pcbImgPixel=0,int _xI=0,int _yJ=0);
		bool get_Black();
		bool set_Black(bool black);
		char set_Connected(char connected);		//该像素是否为连通区域的像素
		char get_Connected();		//该像素是否为连通区域的像素
		const static char NONEVISIT	=0;	//还未访问到该像素
		const static char CONNECTED	=1;	//与边界连通的像素
		const static char UNDECIDED	=2;	//已检测到该像素，但状态未判定
		__declspec(property(put=set_Black,get=get_Black)) bool Black;
		__declspec(property(put=set_Connected,get=get_Connected)) char Connected;
	};
	virtual int DetectIslands(CXhSimpleList<ISLAND>* listIslands,bool blCallNewAlgor=false);
protected:	//检测白色孤岛区域新算法 wjh-2019.5.5
	struct VISITSTATE{
		BYTE cbBoundaryState;
		VISITSTATE(){cbBoundaryState=0;}
		//周边像素点占位序号分布
		//1 2 3
		//4   5
		//6 7 8
		static const BYTE TopLeftVisit1 = 0x01;
		static const BYTE TopMidVisit2  = 0x02;
		static const BYTE TopRightVisit3= 0x04;
		static const BYTE LeftVisit4	= 0x08;
		static const BYTE RightVisit5	= 0x10;
		static const BYTE BtmLeftVisit6	= 0x20;
		static const BYTE BtmMidVisit7	= 0x40;
		static const BYTE BtmRightVisit8= 0x80;
		bool TestVisitState(BYTE cbStateValue);
		BYTE SetVisitState(BYTE cbPlaceMaskFlag,bool blVisited=true);
	};
	MAP_ARRAY<VISITSTATE> xStateMap;
	int UpdateBoundaryVisitState(int xI,int yJ);
	virtual bool VisitWhitePixel(int i,int j,ISLAND* pIsland,ARRAY_LIST<PIXEL>* listStatPixels,bool* pblVerifyConnState=NULL);
	//返回true:表检测到孤岛形白点区域；false:表示检测到的区域与边界连通 wjh-2019.5.5
	virtual bool StatIslandWhitePixels(int i,int j,ISLAND* pIsland,ARRAY_LIST<PIXEL>* pxarrStatPixels);
	virtual bool StatNearWhitePixels(int i,int j,ISLAND* pIsland,ARRAY_LIST<PIXEL>* pxarrStatPixels,BYTE cDetectFlag=0x0f,bool blCallNewAlgor=false);
protected:
	virtual bool IsDetect(int i,int j);
	virtual bool SetDetect(int xi,int yi,bool detect=true);
public:
	virtual void ResetDetectStates();
};
