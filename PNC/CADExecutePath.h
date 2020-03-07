#pragma once
#include "XhCharString.h"
#include "ArrayList.h"

struct CAD_PATH
{
	DWORD dwKey;
	BOOL bUsed;
	CXhChar500 sCadPath;
	CXhChar500 sCadName;
	CXhChar16 sMinCadVersion;
	int iSubCadVersion;
	CXhChar16 sCurVersion;
	CAD_PATH(const char* name = NULL, const char* version = NULL, int sub_version=-1, const char*path = NULL, BOOL used = FALSE) {
		iSubCadVersion = sub_version;
		bUsed = used;
		dwKey = 0;
		if (name)
			sCadName.Copy(name);
		if (version)
			sMinCadVersion.Copy(version);
		if (path)
			sCadPath.Copy(path);
	}
	void SetKey(DWORD key) { dwKey = key; }
};
LONG GetRegKey(HKEY key, const char* subkey, const char* sEntry, char* retdata);
LONG SetRegKey(HKEY key, const char* subkey, const char* sEntry, const char* sValue);
void WriteMenuPathToReg(CAD_PATH &cad_path, const char* sValue, int sonVersion = -1);
BOOL ReadCadPath(CAD_PATH &cad_path, bool bAutoFindPath);
void InitCadPathList(ARRAY_LIST<CAD_PATH> &cadPathArr);
CString GetProductVersion(const char* exe_path);
BOOL StartCadAndLoadArx(const char* productName, const char* APP_PATH, 
						char* cad_path, CString &rxFilePath, HWND hWnd,BOOL bEnableDockWnd=FALSE);
