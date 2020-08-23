#pragma once

#ifdef __UBOM_ONLY_
//from LoadLibrary("BomExport.dll")
class CProjectTowerType;
class CBomExport
{
	HMODULE m_hBomExport;
public:
	typedef DWORD(*DefGetSupportBOMType)();
	typedef DWORD(*DefGetSupportDataBufferVersion)();
	typedef int(*DefCreateExcelBomFile)(char* data_buf, int buf_len, const char* file_name, DWORD dwFlag);
	typedef int(*DefRecogniseReport)(char* data_buf, int buf_len, const char* file_name, DWORD dwFlag);
	typedef int(*DefGetExcelFormat)(int* colIndexArr, int *startRowIndex);
	typedef int(*DefGetExcelFormatEx)(int* colIndexArr, int *startRowIndex, char* titleStr);

	
	DefGetSupportBOMType GetSupportBOMType;
	DefGetSupportDataBufferVersion GetSupportDataBufferVersion;
	DefCreateExcelBomFile CreateExcelBomFile;
	DefRecogniseReport RecogniseReport;
	DefGetExcelFormat GetBomExcelFormat;
	DefGetExcelFormatEx GetBomExcelFormatEx;
public:
	CBomExport();
	~CBomExport();
	void Init();
	void ExportExcelFile(CProjectTowerType *pPrjTowerType);
	bool IsVaild();
};

extern CBomExport g_xBomExport;
#endif