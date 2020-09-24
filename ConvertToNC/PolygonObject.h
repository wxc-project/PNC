#pragma once
#include "f_ent.h"
#include "f_ent_list.h"
#include "ArrayList.h"
#include "HashTable.h"

const static char NONEVISIT = 0;	//��δ���ʵ�������
const static char CONNECTED = 1;	//��߽���ͨ������
const static char BOUNDARY  = 2;	//��������
DWORD MkDW(UINT uiX, UINT uiY);
//////////////////////////////////////////////////////////////////////////
//����
struct PROFILE_PIXEL{
	union{
		DWORD idLocation;
		struct{WORD wiX,wiY;};
	};
	long idRelaObj;	//������ԭʼ�߽���ͼԪId����ACAD��ֱ�ߡ�Բ������Բ����
	BYTE ciFlag;	//���������ر��
public:
	PROFILE_PIXEL() { memset(this, 0, sizeof(PROFILE_PIXEL)); }
	//
	DWORD get_idHash() { return wiX; }
	__declspec(property(get = get_idHash)) DWORD idHash;
	void SetKey(DWORD keyid) { idLocation = keyid; }
};
//�հ�������������������16������
struct VERTSTRIP16{	
	char cbPlaceHolder;		//ռλ�ַ�
	char ciDetectState;		//����״̬
	WORD wiyTop;			//������������
	WORD wiyBtm;			//�����ײ�����
	WORD wixLeft;			//���������ʼ����
public:
	VERTSTRIP16() { memset(this, 0, sizeof(VERTSTRIP16)); }
	//
	int  get_niHeight(){return wiyBtm>=wiyTop?wiyBtm-wiyTop+1:0;}
	__declspec(property(get= get_niHeight)) int niHeight;
};
//��դ�����У���������16������
struct STRIP16_ROW{
	char cbPlaceHolder[2];	//ռλ�ַ�
	short yRowJ;		//��ǰ�е�Y����
	WORD wbPixels;		//��ǰ�и�դ��������
	WORD wDetectState;	//��ǰ�и�դ����״̬
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
	char cbHeadByte;	//��λΪ1��ʾ��ǰ8Bytes�������VERTSTRIP16;�������STRIP16_ROW
	char cbDataBytes[7];
public:
	VERTSTRIP_HEADER(char _cbHeadByte = 1) { 
		memset(this, 0, sizeof(VERTSTRIP_HEADER));
		cbHeadByte = _cbHeadByte;
	}
};
//���ش�������̶�16������
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
	CHashList<OBJECT_ITEM> m_hashRelaObj;	//���ɱպ�����Ĺ���ID����
protected:
	void SetStrokePixel(long xI,long yJ,long idRelaObj=0);
	//��������������
	void DrawPoint(const double* point, long idRelaObj = 0);
	void DrawLine(const double* pxStart, const double* pxEnd, long idRelaObj = 0);
	//��ͨ������--������䷨������ʶ���໥��ͨ����
	bool LayoutImage();
	VERTSTRIP_HEADER* GetStripHead(int xI, int yJ);
	BYTE StateAt(int xI, int yJ);
	void SetConnStateAt(int xI, int yJ);
	void VisitBlankConnPixels();
	//��ͨ������--�������ٷ�������ʶ��ͼ��������
	BOOL IsStartPixel(PROFILE_PIXEL* pPixel, CHashList<BYTE>* pHashMarkBlank=NULL);
	PROFILE_PIXEL* CheckPixelAt(int xI, int yJ, PROFILE_PIXEL *pTagPixel, CHashList<BYTE>& hashMarkBlank);
	BOOL TrackProfilePixels(PROFILE_PIXEL* pStartPixel, ARRAY_LIST<long>& listPixel);
public:
	CVectorMonoImage();
	//
	void(*DisplayProcess)(int percent, const char *sTitle);	//������ʾ�ص�����
	void SetOCS(GECS& _ocs,double scaleOfView2Model=0.01);
	void DrawLine(f3dLine line, long idRelaObj = 0);
	void DrawArcLine(f3dArcLine arc_line, long idRelaObj = 0);
	//
	void DetectProfilePixelsByVisit();
	void DetectProfilePixelsByTrack();
	//����BMP�ļ�
	int WriteBmpFileByPixelHash(const char* fileName, BOOL bValidProfile = FALSE);
	int WriteBmpFileByVertStrip(const char* fileName, BOOL bConnPixel = FALSE);
};
