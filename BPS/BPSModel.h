#pragma once
#include "f_ent.h"
#include "f_ent_list.h"
#include "HashTable.h"
#include "Hash64Table.h"
#include "XhCharString.h"
#include "Variant.h"
#include "LogFile.h"
#include "ArrayList.h"
//#include "StdArx.h"

//工艺卡的识别机制
class CIdentifyManager
{
private:
	static double fMaxX,fMaxY,fMinX,fMinY;
	static f3dPoint partNoPt;
	static CXhChar500 sJgCardPath;
	static CHashList<f2dRect> hashRectByItemType;
public:
	CIdentifyManager();
	~CIdentifyManager();
	static bool InitJgCardInfo(const char* sFileName);
	static CXhChar500 GetJgCardPath(){return sJgCardPath;}
	static f2dRect* GetItemRect(long itemType){return hashRectByItemType.GetValue(itemType+1);}
	static f3dPoint GetLeftBtmPt(){return f3dPoint(fMinX,fMinY,0);}
	static f3dPoint GetLeftTopPt(){return f3dPoint(fMinX,fMaxY,0);}
	static f3dPoint GetRightBtmPt(){return f3dPoint(fMaxX,fMinY,0);}
	static f3dPoint GetRightTopPt(){return f3dPoint(fMaxX,fMaxY);}
	static f3dPoint GetPartnoPt(){return partNoPt;}
};
//
class CAngleProcessInfo
{
private:
	POLYGON region;
	SCOPE_STRU scope;
public:
	static const BYTE TYPE_JG	 = 1;	//角钢
	static const BYTE TYPE_YG	 = 2;	//圆钢
	static const BYTE TYPE_TUBE	 = 3;	//钢管
	static const BYTE TYPE_FLAT	 = 4;	//扁铁
	static const BYTE TYPE_JIG	 = 5;	//夹具
	static const BYTE TYPE_GGS	 = 6;	//钢格栅
	BYTE m_ciType;				//
	CXhChar16 m_sPartNo;		//构件号
	CXhChar50 m_sSpec;			//规格
	char m_cMaterial;			//材质
	int m_nWidth;				//宽度
	int m_nThick;				//厚度
	int m_nLength;				//长度
	int m_nSumNum;				//加工数
	//孔数
	int m_nM12;					//M12孔数
	int m_nM16;					//M16孔数
	int m_nM18;					//M18孔数
	int m_nM20;					//M20孔数
	int m_nM22;					//M22孔数
	int m_nM24;					//M24孔数
	int m_nSumHoleCount;		//总孔数
	//塔型
	CXhChar100 m_sTowerType;
	//备注
	CXhChar200 m_sNotes;		
	//角钢工艺
	BOOL m_bCutRoot;			//刨根
	BOOL m_bCutBer;				//铲背
	BOOL m_bMakBend;			//制弯
	BOOL m_bPushFlat;			//压扁
	BOOL m_bCutEdge;			//铲边
	BOOL m_bRollEdge;			//卷边
	BOOL m_bWeld;				//焊接
	BOOL m_bKaiJiao;			//开角
	BOOL m_bHeJiao;				//合角
	BOOL m_bCutAngle;			//切角
	//
	BOOL m_bInBlockRef;			//是否在工艺卡块中
	AcDbObjectId keyId;
	f3dPoint sign_pt,orig_pt;
	CXhChar100 m_sCardPngFile;
	CXhChar500 m_sCardPngFilePath;	//工艺卡图片路径
	BOOL m_bUploadToServer;
	BOOL m_bUpdatePng;
	int m_iBatchNo;	//记录第几次提取,突出显示当前更新构件
public:
	CAngleProcessInfo();
	~CAngleProcessInfo();
	//
	void InitOrig();
	void CreateRgn(ARRAY_LIST<f3dPoint>& vertexList);
	bool PtInAngleRgn(const double* poscoord);
	void InitAngleInfo(f3dPoint data_pos,const char* sValue);
	f2dRect GetAngleDataRect(BYTE data_type);
	f3dPoint GetAngleDataPos(BYTE data_type);
	bool PtInDataRect(BYTE data_type,f3dPoint pt);
	void CopyProperty(CAngleProcessInfo* pJg);
	SCOPE_STRU GetCADEntScope(){return scope;}
	CXhChar50 GetPartName() const;
};
class CBPSModel
{
	CHash64List<CAngleProcessInfo> m_hashJgInfo;
protected:
	CAngleProcessInfo* FindAngleByPt(f3dPoint data_pos);
	CAngleProcessInfo* FindAngleByPartNo(const char* sPartNo);
public:
	int m_idProject;
	int m_idManuTask;
	int m_idTowerType;
	int m_iRetrieveBatchNo;	//提取批次号 每提取一次批次号+1，重新提取时从1开始
public:
	CBPSModel(void);
	~CBPSModel(void);
	//
	void CorrectAngles();
	void RetrieveJgCardRegion(CHashSet<AcDbObjectId>& selPolyLineIdHash);
	BOOL RetrieveAngleInfo(CHashSet<AcDbObjectId>& selTextIdSet);
	void Empty()
	{
		m_iRetrieveBatchNo=1;
		return m_hashJgInfo.Empty();
	}
	CAngleProcessInfo* AppendJgInfo(UINT_PTR hKey)
	{
		CAngleProcessInfo *pJgInfo=m_hashJgInfo.Add(hKey);
		if(pJgInfo)
			pJgInfo->m_iBatchNo=m_iRetrieveBatchNo;
		return pJgInfo;
	}
	CAngleProcessInfo* EnumFirstJg(){return m_hashJgInfo.GetFirst();}
	CAngleProcessInfo* EnumNextJg(){return m_hashJgInfo.GetNext();}
	BOOL DeleteJgInfo(long hKey){return m_hashJgInfo.DeleteNode(hKey);}
	int GetJgNum(){return m_hashJgInfo.GetNodeNum();}
	//
	static char QueryBriefMatMark(const char* sMatMark);
	static CXhChar16 QuerySteelMatMark(char cMat);
	static int QuerySteelMatIndex(char cMat);
	static void RestoreSpec(const char* spec,int *width,int *thick,char *matStr=NULL);
};
extern CBPSModel BPSModel;