// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//- ConvertToNC.cpp : Initialization functions
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include <afxdllx.h>

#include "PNCCmd.h"
#include "XhLicAgent.h"
#include "XhLdsLm.h"
#include "CadToolFunc.h"
#include "LicFuncDef.h"
#include "PNCSysPara.h"
#include "MsgBox.h"
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"
#include "SteelSealReactor.h"
#include "RevisionDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
/////////////////////////////////////////////////////////////////////////////
// ObjectARX EntryPoint

char* SearchChar(char* srcStr, char ch, bool reverseOrder/*=false*/)
{
	if (!reverseOrder)
		return strchr(srcStr, ch);
	else
	{
		int len = strlen(srcStr);
		for (int i = len - 1; i >= 0; i--)
		{
			if (srcStr[i] == ch)
				return &srcStr[i];
		}
	}
	return NULL;
}

void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// 智能提取板信息
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"SmartExtractPlate",             // Global function name
		L"SmartExtractPlate",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&SmartExtractPlate);            // Function pointer
	//手动提取钢板信息
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"ReviseThePlate",        // Global function name
		L"ReviseThePlate",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&ManualExtractPlate);            // Function pointer
#ifndef __UBOM_ONLY_
	//钢板排版
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"LP",        // Global function name
		L"LP",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&LayoutPlates);      
	//编辑钢板信息
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"SendPartEdit",        // Global function name
		L"SendPartEdit",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SendPartEditor);            // Function pointer
	//插入钢印区
	acedRegCmds->addCommand( L"PNC-MENU",         // Group name 
		L"MK",
		L"MK",
		ACRX_CMD_MODAL,
		&InsertMKRect);
#endif
	//系统设置
	acedRegCmds->addCommand( L"PNC-MENU",         // Group name 
		L"EnvGeneralSet",          // Global function name
		L"EnvGeneralSet",          // Local function name
		ACRX_CMD_MODAL,      // Type
		&EnvGeneralSet);
	// 智能提取板信息
	acedRegCmds->addCommand(L"REVISION-MENU",   // Group name
		L"RPP",					// Global function name
		L"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer

#else
	// 智能提取板信息
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"SmartExtractPlate",        // Global function name
		"SmartExtractPlate",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SmartExtractPlate);            // Function pointer
	//手动提取钢板信息
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"ReviseThePlate",        // Global function name
		"ReviseThePlate",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&ManualExtractPlate);            // Function pointer
#ifndef __UBOM_ONLY_
	//钢板排版
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"LP",        // Global function name
		"LP",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&LayoutPlates);      
	//编辑钢板信息
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"SendPartEdit",        // Global function name
		"SendPartEdit",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SendPartEditor);            // Function pointer
	//插入钢印区
	acedRegCmds->addCommand( "PNC-MENU",         // Group name 
		"MK",
		"MK",
		ACRX_CMD_MODAL,
		&InsertMKRect);
#endif
	//系统设置
	acedRegCmds->addCommand( "PNC-MENU",         // Group name 
		"EnvGeneralSet",          // Global function name
		"EnvGeneralSet",          // Local function name
		ACRX_CMD_MODAL,      // Type
		&EnvGeneralSet);
	// 智能提取板信息
	acedRegCmds->addCommand("REVISION-MENU",   // Group name
		"RPP",					// Global function name
		"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
	acedRegCmds->addCommand("REVISION-MENU",   // Group name
		"DrawByTxtFile",					// Global function name
		"DrawByTxtFile",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&DrawProfileByTxtFile);				// Function pointer
#endif
}
#if !defined(__AFTER_ARX_2007)
extern "C" void ads_queueexpr( ACHAR *);
#endif
bool DetectSpecifiedHaspKeyFile(const char* default_file)
{
	FILE* fp = fopen(default_file, "rt");
	if (fp == NULL)
		return false;
	bool detected = false;
	CXhChar200 line_txt;//[MAX_PATH];
	CXhChar200 scope_xmlstr;
	scope_xmlstr.Append(
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
		"<haspscope>");
	while (!feof(fp))
	{
		if (fgets(line_txt, line_txt.GetLengthMax(), fp) == NULL)
			break;
		line_txt.Replace("＝", "=");
		char* final = SearchChar(line_txt, ';',true);
		if (final != NULL)
			*final = 0;
		char *skey = strtok(line_txt, " =,");
		//常规设置
		if (_stricmp(skey, "Key") == 0)
		{
			if (skey = strtok(NULL, "=,"))
			{
				scope_xmlstr.Append("<hasp id=\"");
				scope_xmlstr.Append(skey);
				scope_xmlstr.Append("\" />");
				detected = true;
			}
		}
	}
	fclose(fp);
	scope_xmlstr.Append("</haspscope>");
	if (detected)
		SetHaspLoginScope(scope_xmlstr);
	return detected;
}
void InitApplication()
{
	//进行加密处理
	DWORD version[2]={0,20170615};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=2;
	pByteVer[2]=0;
	pByteVer[3]=0;
	char lic_file[MAX_PATH];
	GetLicFile(lic_file);
	char key_file[MAX_PATH];
	strcpy(key_file, lic_file);
	//查找是否存在指定加密锁号的文件 wht-2017.06.07
	char* separator = SearchChar(key_file, '.', true);
	strcpy(separator, ".key");
	DetectSpecifiedHaspKeyFile(key_file);
	ULONG retCode=ImportLicFile(lic_file,PRODUCT_PNC,version);
	if(retCode!=0)
	{
		CXhChar500 errormsgstr;
		if(retCode==-2)
			errormsgstr.Copy("首次使用，还未指定过证书文件！");
		else if(retCode==-1)
			errormsgstr.Copy("加密锁初始化失败");
		else if(retCode==1)
			errormsgstr.Copy("1#无法打开证书文件");
		else if(retCode==2)
			errormsgstr.Copy("2#证书文件遭到破坏");
		else if(retCode==3)
			errormsgstr.Copy("3#证书与加密狗不匹配");
		else if(retCode==4)
			errormsgstr.Copy("4#授权证书的加密版本不对");
		else if(retCode==5)
			errormsgstr.Copy("5#证书与加密狗产品授权版本不匹配");
		else if(retCode==6)
			errormsgstr.Copy("6#超出版本使用授权范围");
		else if(retCode==7)
			errormsgstr.Copy("7#超出免费版本升级授权范围");
		else if(retCode==8)
			errormsgstr.Copy("8#证书序号与当前加密狗不一致");
		else if(retCode==9)
			errormsgstr.Copy("9#授权过期，请续借授权");
		else if(retCode==10)
			errormsgstr.Copy("10#程序缺少相应执行权限，请以管理员权限运行程序");
		else if (retCode == 11)
			errormsgstr.Copy("11#授权异常，请使用管理员权限重新申请证书");
		else
			errormsgstr.Printf("未知错误，错误代码%d#", retCode);
		errormsgstr.Append(CXhChar500(".证书路径：%s",lic_file));
		strcpy(BTN_ID_YES_TEXT,"打开文件夹");
		strcpy(BTN_ID_NO_TEXT,"退出CAD");
		strcpy(BTN_ID_CANCEL_TEXT,"关闭");
		int nRetCode=MsgBox(adsw_acadMainWnd(),errormsgstr,"错误提示",MB_YESNOCANCEL|MB_ICONERROR);
		if(nRetCode==IDYES)
		{
			CXhChar500 sPath(lic_file);
			sPath.Replace("PNC.lic","");	//移除PNC.lic
			sPath.Replace("TMA.lic", "");
			ShellExecute(NULL,"open",NULL,NULL,sPath,SW_SHOW);
			exit(0);
		}
		else if(nRetCode==IDNO)
			exit(0);
		else
		{	//卸载PNC，不退出cad
			//kLoadDwgMsg中不能调用sendStringToExecute和acedCommand,acDocManager->curDocument()==NULL,无法执行 wht 18-12-25
#ifdef __UBOM_
#ifdef _ARX_2007
			ads_queueexpr(L"(command\"arx\" \"u\" \"UBOM.arx\")");
#else
			ads_queueexpr("(command\"arx\" \"u\" \"UBOM.arx\")");
#endif
#else
#ifdef _ARX_2007
			ads_queueexpr(L"(command\"arx\" \"u\" \"PNC.arx\")");
#else
			ads_queueexpr("(command\"arx\" \"u\" \"PNC.arx\")");
#endif
#endif
			return;
		}
	}
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("软件缺少合法使用授权!");
		exit(0);
	}
	HWND hWnd = adsw_acadMainWnd();
	::SetWindowText(hWnd,"PNC");
	RegisterServerComponents();
	//
	//创建对话框
	g_pRevisionDlg = new CRevisionDlg();
	PNCSysSetImportDefault();
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//退出加密狗
	char sGroupName[100]="PNC-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
	//删除acad.rx
	char cad_path[MAX_PATH]="";
	GetModuleFileName(NULL,cad_path,MAX_PATH);
	if(strlen(cad_path)>0)
	{
		char *separator=SearchChar(cad_path,'.',true);
		strcpy(separator,".rx");
		::DeleteFile(cad_path);
	}
	if (g_pRevisionDlg)
	{
		delete g_pRevisionDlg;
		g_pRevisionDlg = NULL;
	}
}

//-----------------------------------------------------------------------------
//- Define the sole extension module object.
AC_IMPLEMENT_EXTENSION_MODULE(ConvertToNCDLL)
//- Please do not remove the 3 following lines. These are here to make .NET MFC Wizards
//- running properly. The object will not compile but is require by .NET to recognize
//- this project as being an MFC project
#ifdef NEVER
AFX_EXTENSION_MODULE ConvertToNCExtDLL ={ NULL, NULL } ;
#endif

//- Now you can use the CAcModuleResourceOverride class in
//- your application to switch to the correct resource instance.
//- Please see the ObjectARX Documentation for more details

//-----------------------------------------------------------------------------
//- DLL Entry Point
extern "C"
BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {
	//- Remove this if you use lpReserved
	//UNREFERENCED_PARAMETER(lpReserved) ;

	if ( dwReason == DLL_PROCESS_ATTACH ) 
	{
        //_hdllInstance =hInstance ;
		ConvertToNCDLL.AttachInstance (hInstance) ;
		InitAcUiDLL () ;
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		ConvertToNCDLL.DetachInstance () ;
	}
	return (TRUE) ;
}
extern "C" AcRx::AppRetCode
#ifdef _MAPTMA_ZRX
zcrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
#else
acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
#endif
{
	switch (msg) {
	case AcRx::kInitAppMsg:
		// Comment out the following line if your
		// application should be locked into memory
		acrxDynamicLinker->unlockApplication(pkt);
		acrxDynamicLinker->registerAppMDIAware(pkt);
		InitApplication();
		DisplayPartListDockBar();
		g_pSteelSealReactor = new CSteelSealReactor();
		break;
	case AcRx::kUnloadAppMsg:
		if (g_pSteelSealReactor != NULL)
		{
			delete g_pSteelSealReactor;
			g_pSteelSealReactor = NULL;
		}
		DestoryPartListDockBar();
		UnloadApplication();
		break;
	case AcRx::kLoadDwgMsg:
		//InitDrawingEnv();
		break;
	}
	return AcRx::kRetOK;
}