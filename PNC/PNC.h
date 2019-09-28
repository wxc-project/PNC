
// PNC.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include "XhCharString.h"
#include "ArrayList.h"
#include "XhLicAgent.h"
#include "LicFuncDef.h"
//#include "XhLdsLm.h"

// CPNCApp:
// 有关此类的实现，请参阅 PNC.cpp
//
void GetAppPath(char* StartPath);
int ImportLicFile(char* licfile,BYTE cProductType,DWORD version[2]);
class CPNCApp : public CWinApp
{
public:
	DWORD m_version[2];
	char execute_path[MAX_PATH];//获取执行文件的启动目录
	CPNCApp();

// 重写
public:
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CPNCApp theApp;
extern char APP_PATH[MAX_PATH];