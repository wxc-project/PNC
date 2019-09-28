#pragma once

#include "Buffer.h"
#include "ProcessPart.h"
#include "I_DrawSolid.h"
#include "IPEC.h"

class CProcessPartsHolder
{
private:
	int m_iCurFileIndex;						//当前正在编辑的文件索引
	BYTE m_cPartType;							//当前正在编辑的构件类型
	CString m_sFolderPath;						//主文件夹
	CProcessPlate m_xPlate;
	CProcessAngle m_xAngle;
	CStringArray m_arrPlateFileName;			//钢板构件数组
	CStringArray m_arrAngleFileName;			//角钢构件数组
	CString GetFilePathByIndex(int curIndex);
	CString GetCurFileNameByIndex(int index);
	BOOL UpdateCurPartFromFile(int iCurFileIndex);
public:
	CProcessPartsHolder();
	~CProcessPartsHolder();
	//共用操作
	void Init(CString sFolderPath);
	void JumpToFirstPart();
	void JumpToPrePart();
	void JumpToNextPart();
	void JumpToLastPart();
	void JumpToSpecPart(char *sPartFileName);
	CString GetCurFileName();
	BYTE GetCurPartType() { return m_cPartType; }
	void SaveCurPartInfoToFile();
	CXhChar200 AddPart(CProcessPart *pPart);
	//钢板操作
	void CreateAllPlateDxfFile();
	void CreateAllPlateTtpFile();
	//角钢操作
	void CreateAllAngleNcFile(char* drv_path,char* sPartNoPrefix);
	//
	CStringArray& GetPlateFileNameArr() { return m_arrPlateFileName; }
	CStringArray& GetAngleFileNameArr() { return m_arrAngleFileName; }
	//
	CProcessPart* GetCurProcessPart();
	bool UpdatePartInfoToEditor();
	void EscapeCurEditPart();
};

extern CProcessPartsHolder g_partInfoHodler;
extern ISolidDraw *g_pSolidDraw;
extern ISolidSet *g_pSolidSet;
extern ISolidSnap *g_pSolidSnap;
extern ISolidOper *g_pSolidOper;
extern I2dDrawing *g_p2dDraw;
extern IPEC* g_pPartEditor;