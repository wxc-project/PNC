#include "StdAfx.h"
#include "CADExecutePath.h"

LONG SetRegKey(HKEY key, const char* subkey, const char* sEntry, const char* sValue)
{
	HKEY hkey;
	LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_WRITE, &hkey);
	if (retval == ERROR_SUCCESS)
	{
		DWORD dwLength = strlen(sValue);
		retval = RegSetValueEx(hkey, _T(sEntry), NULL, REG_SZ, (BYTE*)&sValue[0], dwLength);
		RegCloseKey(hkey);
	}
	return retval;
}
LONG GetRegKey(HKEY key, const char* subkey, const char* sEntry, char* retdata)
{
	HKEY hkey;
	LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_READ, &hkey);
	if (retval == ERROR_SUCCESS)
	{
		char data[MAX_PATH] = "";
		DWORD dwType, dwCount = MAX_PATH;
		retval = RegQueryValueEx(hkey, _T(sEntry), NULL, &dwType, (BYTE*)&data[0], &dwCount);
		if (retval == ERROR_SUCCESS)
			strcpy(retdata, data);
		RegCloseKey(hkey);
	}
	return retval;
}

BOOL ReadCadPath(CAD_PATH &cad_path, bool bAutoFindPath)
{
	//优先从CAD_PATH中读取用户设定的CAD路径
	char sCurVer[MAX_PATH] = "", sSubKey[MAX_PATH] = "";
#ifdef __CNC_
	strcpy(sSubKey, "Software\\Xerofox\\CNC\\Settings");
#elif __PNC_
#ifdef __UBOM_ONLY_
	strcpy(sSubKey, "Software\\Xerofox\\UBOM\\Settings");
#else
	strcpy(sSubKey, "Software\\Xerofox\\PNC\\Settings");
#endif
#endif
	GetRegKey(HKEY_CURRENT_USER, sSubKey, cad_path.sCadName, cad_path.sCadPath);
	if (strlen(cad_path.sCadPath) <= 0 && bAutoFindPath)
	{	//从注册表中自动查询CAD的安装路径
		if (cad_path.sCadName.StartWith('Z'))
		{
			cad_path.sCurVersion.Copy(cad_path.sMinCadVersion);
			sprintf(sSubKey, "Software\\ZWSOFT\\ZWCAD\\%s", (char*)cad_path.sCurVersion);
			if (GetRegKey(HKEY_CURRENT_USER, sSubKey, "CurVer", sCurVer) != ERROR_SUCCESS)
				return FALSE;
			sprintf(sSubKey, "SOFTWARE\\ZWSOFT\\ZWCAD\\%s\\%s", (char*)cad_path.sCurVersion, sCurVer);
			if (GetRegKey(HKEY_LOCAL_MACHINE, sSubKey, "Location", cad_path.sCadPath) != ERROR_SUCCESS)
				return FALSE;
		}
		else
		{
			int i = 0;
			for (i = 0; i <= 9; i++)
			{
				if (cad_path.iSubCadVersion >= 0 && i != cad_path.iSubCadVersion)
					continue;
				cad_path.sCurVersion.Printf("%s.%d", (char*)cad_path.sMinCadVersion, i);
				sprintf(sSubKey, "Software\\Autodesk\\AutoCAD\\%s", (char*)cad_path.sCurVersion);
				if (GetRegKey(HKEY_CURRENT_USER, sSubKey, "CurVer", sCurVer) == ERROR_SUCCESS)
					break;
			}
			if (i == 9)
				return FALSE;
			sprintf(sSubKey, "SOFTWARE\\Autodesk\\AutoCAD\\%s\\%s", (char*)cad_path.sCurVersion, sCurVer);
			if (GetRegKey(HKEY_LOCAL_MACHINE, sSubKey, "AcadLocation", cad_path.sCadPath) != ERROR_SUCCESS)
				return FALSE;
		}
		if (strlen(cad_path.sCadPath) > 1)
		{
			strcat(cad_path.sCadPath, "\\");
			return TRUE;
		}
	}
	return TRUE;
}
void WriteMenuPathToReg(CAD_PATH &cad_path, const char* sValue,int sonVersion/*=-1*/)
{
	char sCurVer[MAX_PATH] = "", sSubKey[MAX_PATH] = "";
	int i = 0;
	if (cad_path.sCadName.StartWith('Z'))
	{
		cad_path.sCurVersion.Copy(cad_path.sMinCadVersion);
		sprintf(sSubKey, "Software\\ZWSOFT\\ZWCAD\\%s", (char*)cad_path.sCurVersion);
		if (GetRegKey(HKEY_CURRENT_USER, sSubKey, "CurVer", sCurVer) != ERROR_SUCCESS)
			return;
		sprintf(sSubKey, "Software\\ZWSOFT\\ZWCAD\\%s\\%s\\Profiles\\Default\\CommonSettings", (char*)cad_path.sCurVersion, sCurVer);
		if (SetRegKey(HKEY_CURRENT_USER, sSubKey, "MainCuiFile", sValue) != ERROR_SUCCESS)
			return;
	}
	else
	{
		for (i = 0; i <= 9; i++)
		{
			if (sonVersion >= 0 && sonVersion != i)
				continue;	//外部指定子版本号
			cad_path.sCurVersion.Printf("%s.%d", (char*)cad_path.sMinCadVersion, i);
			sprintf(sSubKey, "Software\\Autodesk\\AutoCAD\\%s", (char*)cad_path.sCurVersion);
			if (GetRegKey(HKEY_CURRENT_USER, sSubKey, "CurVer", sCurVer) == ERROR_SUCCESS)
				break;
		}
		if (i == 9)
			return;
		sprintf(sSubKey, "Software\\Autodesk\\AutoCAD\\%s\\%s\\Profiles\\<<未命名配置>>\\General Configuration", (char*)cad_path.sCurVersion, sCurVer);
		if (SetRegKey(HKEY_CURRENT_USER, sSubKey, "MenuFile", sValue) != ERROR_SUCCESS)
			return;
	}
}

void InitCadPathList(ARRAY_LIST<CAD_PATH> &cadPathArr)
{
	cadPathArr.Empty();
	cadPathArr.SetSize(8);
	cadPathArr[0] = CAD_PATH("CAD2004", "R16", -1, "", false);
	cadPathArr[1] = CAD_PATH("CAD2005", "R16",  1, "", false);
	cadPathArr[2] = CAD_PATH("CAD2006", "R16",  2, "", false);
	cadPathArr[3] = CAD_PATH("CAD2007", "R17", -1, "", false);
	cadPathArr[4] = CAD_PATH("CAD2008", "R17",  1, "", false);
	cadPathArr[5] = CAD_PATH("CAD2009", "R17",  2, "", false);
	cadPathArr[6] = CAD_PATH("CAD2010", "R18", -1, "", false);
	cadPathArr[7] = CAD_PATH("ZWCAD2019", "2019", -1, "", false);
	//cadPathArr[7] = CAD_PATH("CAD2011", "R18",  1, "", false);
	//cadPathArr[8] = CAD_PATH("CAD2012", "R18",  2, "", false);

	CXhChar500 sActivedCad;
	CXhChar100 sSubKey;
#ifdef __CNC_
	sSubKey.Copy("Software\\Xerofox\\CNC\\Settings");
#elif __PNC_
#ifdef __UBOM_ONLY_
	sSubKey.Copy("Software\\Xerofox\\UBOM\\Settings");
#else
	sSubKey.Copy("Software\\Xerofox\\PNC\\Settings");
#endif
#endif
	GetRegKey(HKEY_CURRENT_USER, sSubKey, "ActivedCad", sActivedCad);
	BOOL bUsed = FALSE;
	for (int i = 0; i < cadPathArr.GetSize(); i++)
	{
		ReadCadPath(cadPathArr[i], true);
		if (!bUsed)
		{
			if (cadPathArr[i].sCadName.EqualNoCase(sActivedCad) &&
				cadPathArr[i].sCadPath.GetLength() > 0)
			{
				cadPathArr[i].bUsed = TRUE;
				bUsed = TRUE;
			}
			else
				cadPathArr[i].bUsed = FALSE;
			//修改CAD注册表中菜单路径
			/*if (cadPathArr[i].bUsed)
			{
				bUsed = TRUE;
				char sMenuPath[MAX_PATH] = "";
				sprintf(sMenuPath, "%sCNC.mnu", APP_PATH);
				WriteMenuPathToReg(cadPathArr[i], sMenuPath);
			}*/
		}
	}
	/*for(int i=0;i<m_xCadPathArr.GetSize();i++)
	{
		if(m_xCadPathArr[i].sCadPath.GetLength()>0)
		{
			m_xCadPathArr[i].bUsed=TRUE;
			char sMenuPath[MAX_PATH]="";
			sprintf(sMenuPath,"%sCNC.mnu",APP_PATH);
			WriteMenuPathToReg(m_xCadPathArr[i],sMenuPath);
			SetRegKey(HKEY_CURRENT_USER,sSubKey,"ActivedCad",m_xCadPathArr[i].sCadName);
		}
	}*/
}
CString GetProductVersion(const char* exe_path)
{
	CString product_version;
	DWORD dwLen = GetFileVersionInfoSize(exe_path, 0);
	BYTE *data = new BYTE[dwLen];
	WORD product_ver[4];
	if (GetFileVersionInfo(exe_path, 0, dwLen, data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data, "\\", (void**)&info, &uLen);
		memcpy(product_ver, &info->dwProductVersionMS, 4);
		memcpy(&product_ver[2], &info->dwProductVersionLS, 4);
		product_version.Format("%d.%d.%d.%d", product_ver[1], product_ver[0], product_ver[3], product_ver[2]);
	}
	delete data;
	return product_version;
}

BOOL StartCadAndLoadArx(const char* productName, const char* APP_PATH, 
						char* cad_path, CString &rxFilePath, HWND hWnd)
{
	//启动CAD，并自动加载PNC.arx
	char cadPath[MAX_PATH] = "", cadName[MAX_PATH] = "";
	strcpy(cadPath, cad_path);
	if (strlen(cadPath) <= 0)
	{
		CAD_PATH xCadPath;
		if (ReadCadPath(xCadPath, true))
			strcpy(cadPath, xCadPath.sCadPath);
	}
	if (cad_path != NULL)	//输出cad路径
		strcpy(cad_path, cadPath);
	BOOL bZWCad = FALSE;
	CString sCadPath(cadPath);
	rxFilePath.Format("%s", cadPath);
	if (sCadPath.FindOneOf("ZW") > 0)
	{	//中望CAD
		bZWCad = TRUE;
		rxFilePath += "Zwcad.rx";
		sprintf(cadName, "%sZWCAD.exe", cadPath);
	}
	else
	{
		rxFilePath += "Acad.rx";
		sprintf(cadName, "%sacad.exe", cadPath);
	}
	FILE* fp;
	if ((fp = fopen(rxFilePath, "wt")) == NULL)
	{
		AfxMessageBox("绘图启动文件创建失败,不能启动绘图模块!");
		return FALSE;
	}
	else
	{
		CString sProductVersion = GetProductVersion(cadName);
		if (sProductVersion.Find("16.") == 0)
			fprintf(fp, "%s%s05.arx\n", APP_PATH, productName);
		else if (sProductVersion.Find("17.") == 0)
			fprintf(fp, "%s%s07.arx\n", APP_PATH, productName);
		else if (sProductVersion.Find("18.") == 0)
			fprintf(fp, "%s%s10.arx\n", APP_PATH, productName);
		else if(bZWCad)
			fprintf(fp, "%s%s19.zrx\n", APP_PATH, productName);
		fclose(fp);
		//
		//修改CAD注册表中菜单路径
		CAD_PATH cad_path;
		cad_path.sCadPath.Copy(cadPath);
		int sonVersion = -1;
		char sMenuPath[MAX_PATH] = "";
		if (sProductVersion.Find("16.") == 0)
		{
			cad_path.sMinCadVersion.Copy("R16");
			sprintf(sMenuPath, "%s%s.mnu", APP_PATH, productName);
		}
		else if (sProductVersion.Find("17.") == 0)
		{
			cad_path.sMinCadVersion.Copy("R17");
			if (sProductVersion.Find("17.1") == 0)
			{
				sonVersion = 1;
				sprintf(sMenuPath, "%s%s08.cui", APP_PATH, productName);
			}
			else
				sprintf(sMenuPath, "%s%s07.cui", APP_PATH, productName);
		}
		else if (sProductVersion.Find("18.") == 0)
		{
			cad_path.sMinCadVersion.Copy("R18");
			sprintf(sMenuPath, "%s%s10.cui", APP_PATH, productName);
		}
		else if (bZWCad)
		{
			cad_path.sCadName.Copy("ZWCAD 2019");
			cad_path.sMinCadVersion.Copy("2019");
			sprintf(sMenuPath, "%s%s-ZRX.cuix", APP_PATH, productName);
		}
		WriteMenuPathToReg(cad_path, sMenuPath, sonVersion);
	}
	//创建进程，启动CAD
	STARTUPINFO startInfo;
	memset(&startInfo, 0, sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.dwFlags = STARTF_USESHOWWINDOW;
	startInfo.wShowWindow = SW_SHOWMAXIMIZED;

	PROCESS_INFORMATION processInfo;
	memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));
	BOOL bRet = CreateProcess(cadName, "", NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &processInfo);
	if (bRet)
	{
		WaitForInputIdle(processInfo.hProcess, INFINITE);
		ShowWindow(hWnd,SW_HIDE);
		Sleep(30000);
		return TRUE;
	}
	else
		MessageBox(hWnd,("启动cad失败"),productName, MB_OK);
	return FALSE;
}