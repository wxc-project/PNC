#pragma once
#include "f_ent.h"
#include "f_ent_list.h"
#include "ArrayList.h"
#include "HashTable.h"

const static char NONEVISIT = 0;	//还未访问到该像素
const static char CONNECTED = 1;	//与边界连通的像素
const static char BOUNDARY  = 2;	//轮廓像素
DWORD MkDW(UINT uiX, UINT uiY);
//////////////////////////////////////////////////////////////////////////
//像素
struct PROFILE_PIXEL{
	union{
		DWORD idLocation;
		struct{WORD wiX,wiY;};
	};
	long idRelaObj;	//关联的原始边界线图元Id，如ACAD的直线、圆弧、椭圆弧等
	BYTE ciFlag;	//外轮廓像素标记
public:
	PROFILE_PIXEL() { memset(this, 0, sizeof(PROFILE_PIXEL)); }
	//
	DWORD get_idHash() { return wiX; }
	__declspec(property(get = get_idHash)) DWORD idHash;
	void SetKey(DWORD keyid) { idLocation = keyid; }
};
//空白条幅带区，固死带宽16个像素
struct VERTSTRIP16{	
	char cbPlaceHolder;		//占位字符
	char ciDetectState;		//竖条状态
	WORD wiyTop;			//竖条顶部坐标
	WORD wiyBtm;			//竖条底部坐标
	WORD wixLeft;			//竖条左侧起始坐标
public:
	VERTSTRIP16() { memset(this, 0, sizeof(VERTSTRIP16)); }
	//
	int  get_niHeight(){return wiyBtm>=wiyTop?wiyBtm-wiyTop+1:0;}
	__declspec(property(get= get_niHeight)) int niHeight;
};
//格栅像素行，固死带宽16个像素
struct STRIP16_ROW{
	char cbPlaceHolder[2];	//占位字符
	short yRowJ;		//当前行的Y坐标
	WORD wbPixels;		//当前行格栅像素数据
	WORD wDetectState;	//当前行格栅像素状态
public:
	STRIP16_ROW() { memset(this, 0, sizeof(STRIP16_ROW)); }
	//
	void SetBlckPixel(int xI);
	BOOL IsBlckPixel(int xI);
	void SetConnState(int xI);
	BOOL IsConnState(int xI);
};
//
struct VERTSTRIP_HEADER{
	char cbHeadByte;	//高位为1表示当前8Bytes代表的是VERTSTRIP16;否则代表STRIP16_ROW
	char cbDataBytes[7];
public:
	VERTSTRIP_HEADER(char _cbHeadByte = 1) { 
		memset(this, 0, sizeof(VERTSTRIP_HEADER));
		cbHeadByte = _cbHeadByte;
	}
};
//像素带，带宽固定16个像素
struct STRIP_SCROLL{
	ARRAY_LIST<VERTSTRIP_HEADER> xarrEntries;
public:
	STRIP_SCROLL() { }
	bool CheckProfilePixel(int xI, int yJ);
};
//
struct OBJECT_ITEM
{
	long m_idRelaObj;
	UINT m_nPixelNum;
	OBJECT_ITEM() {
		memset(this, 0, sizeof(OBJECT_ITEM));
	}
};
//////////////////////////////////////////////////////////////////////////
//CVectorMonoImage
class CVectorMonoImage{
protected:
	GECS ocs;
	double m_fScaleOfView2Model;
	long m_nWidth, m_nHeight;
	GEPOINT2D arr8NearPos[8];
	GEPOINT2D arr4NearPos[4];
	CHashListEx<PROFILE_PIXEL> hashStrokePixels;
	ARRAY_LIST<STRIP_SCROLL> xarrVertStrips;
public:
	CHashList<OBJECT_ITEM> m_hashRelaObj;	//构成闭合区域的关联ID集合
protected:
	void SetStrokePixel(long xI,long yJ,long idRelaObj=0);
	//绘制轮廓点像素
	void DrawPoint(const double* point, long idRelaObj = 0);
	void DrawLine(const double* pxStart, const double* pxEnd, long idRelaObj = 0);
	//连通区域标记--种子填充法，用于识别相互连通区域
	bool LayoutImage();
	VERTSTRIP_HEADER* GetStripHead(int xI, int yJ);
	BYTE StateAt(int xI, int yJ);
	void SetConnStateAt(int xI, int yJ);
	void VisitBlankConnPixels();
	//连通区域标记--轮廓跟踪法，用于识别图形外轮廓
	BOOL IsStartPixel(PROFILE_PIXEL* pPixel, CHashList<BYTE>* pHashMarkBlank=NULL);
	PROFILE_PIXEL* CheckPixelAt(int xI, int yJ, PROFILE_PIXEL *pTagPixel, CHashList<BYTE>& hashMarkBlank);
	BOOL TrackProfilePixels(PROFILE_PIXEL* pStartPixel, ARRAY_LIST<long>& listPixel);
public:
	CVectorMonoImage();
	//
	void(*DisplayProcess)(int percent, const char *sTitle);	//进度显示回调函数
	void SetOCS(GECS& _ocs,double scaleOfView2Model=0.01);
	void DrawLine(f3dLine line, long idRelaObj = 0);
	void DrawArcLine(f3dArcLine arc_line, long idRelaObj = 0);
	//
	void DetectProfilePixelsByVisit();
	void DetectProfilePixelsByTrack();
	//导出BMP文件
	int WriteBmpFileByPixelHash(const char* fileName, BOOL bValidProfile = FALSE);
	int WriteBmpFileByVertStrip(const char* fileName, BOOL bConnPixel = FALSE);
};
