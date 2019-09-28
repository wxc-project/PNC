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
#include "MenuFunc.h"
#include "XhLdsLm.h"
#include "CadToolfunc.h"
#include "LicFuncDef.h"
#include "RevisionDlg.h"
#include "MsgBox.h"

/////////////////////////////////////////////////////////////////////////////
// ObjectARX EntryPoint

void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// 智能提取板信息
	acedRegCmds->addCommand(L"REVISION-MENU",   // Group name
		L"RPP",					// Global function name
		L"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#else
	// 智能提取板信息
	acedRegCmds->addCommand("REVISION-MENU",   // Group name
		"RPP",					// Global function name
		"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#endif
}
WORD m_wDogModule=0;
#if !defined(__AFTER_ARX_2007)
extern "C" void ads_queueexpr(ACHAR *);
#endif
void InitApplication()
{
	//进行加密处理
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=4;
	pByteVer[1]=1;
	pByteVer[2]=0;
	pByteVer[3]=5;
	char lic_file[MAX_PATH],lic_file2[MAX_PATH];
	if(GetLicFile(lic_file)==FALSE&&GetLicFile2(lic_file2)==FALSE)
	{
		AfxMessageBox("证书文件路径读取失败!");
		return;
	}
	CXhChar500 cur_lic_file(lic_file2);
	ULONG retCode=ImportLicFile(lic_file2,PRODUCT_TMA,version);
	if(retCode!=0)
	{
		retCode=ImportLicFile(lic_file,PRODUCT_TMA,version);
		cur_lic_file.Copy(lic_file);
	}
	if(retCode!=0)
	{
		CXhChar500 sError;
		if(retCode==-2)
			sError.Copy("首次使用，还未指定过证书文件！");
		else if(retCode==-1)
			sError.Copy("加密锁初始化失败!");
		else if(retCode==1)
			sError.Copy("1#无法打开证书文件!");
		else if(retCode==2)
			sError.Copy("2#证书文件遭到破坏!");
		else if(retCode==3)
			sError.Copy("3#证书与加密狗不匹配!");
		else if(retCode==4)
			sError.Copy("4#授权证书的加密版本不对!");
		else if(retCode==5)
			sError.Copy("5#证书与加密狗产品授权版本不匹配!");
		else if(retCode==6)
			sError.Copy("6#超出版本使用授权范围!");
		else if (retCode == 7)
			sError.Copy("7#超出免费版本升级授权范围");
		else if (retCode == 8)
			sError.Copy("8#证书序号与当前加密狗不一致");
		else if (retCode == 9)
			sError.Copy("9#授权过期，请续借授权");
		else if (retCode == 10)
			sError.Copy("10#程序缺少相应执行权限，请以管理员权限运行程序");
		else if (retCode == 11)
			sError.Copy("11#授权异常，请使用管理员权限重新申请证书");
		else
			sError.Printf("未知错误，错误代码%d#", retCode);
		sError.Append(CXhChar500(".证书路径：%s", lic_file));
		strcpy(BTN_ID_YES_TEXT, "打开文件夹");
		strcpy(BTN_ID_NO_TEXT, "退出CAD");
		strcpy(BTN_ID_CANCEL_TEXT, "关闭");
		int nRetCode = MsgBox(adsw_acadMainWnd(), sError, "错误提示", MB_YESNOCANCEL | MB_ICONERROR);
		if (nRetCode == IDYES)
		{
			CXhChar500 sPath(lic_file);
			sPath.Replace("UBOM.lic", "");	//移除UBOM.lic
			sPath.Replace("TMA.lic", "");	//移除TMA.lic
			ShellExecute(NULL, "open", NULL, NULL, sPath, SW_SHOW);
			exit(0);
		}
		else if (nRetCode == IDNO)
			exit(0);
		else
		{	//卸载UBOM，不退出cad
			//kLoadDwgMsg中不能调用sendStringToExecute和acedCommand,acDocManager->curDocument()==NULL,无法执行 wht 18-12-25
#ifdef _ARX_2007
			ads_queueexpr(L"(command\"arx\" \"u\" \"UBOM.arx\")");
#else
			ads_queueexpr("(command\"arx\" \"u\" \"UBOM.arx\")");
#endif
			return;
		}
		ExitCurrentDogSession();
		exit(0);
	}
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("软件缺少合法使用授权!");
		exit(0);
	}
	RegisterServerComponents();
	//创建对话框
	g_pRevisionDlg = new CRevisionDlg(); 
}
void UnloadApplication()
{
	delete g_pRevisionDlg;
	//
#ifndef _LEGACY_LICENSE
	ExitCurrentDogSession();
#elif defined(_NET_DOG)
	ExitNetLicEnv(m_wDogModule);
#endif
	char sGroupName[100]="NC-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
}

//-----------------------------------------------------------------------------
//- Define the sole extension module object.
AC_IMPLEMENT_EXTENSION_MODULE(UbomDLL)
//- Please do not remove the 3 following lines. These are here to make .NET MFC Wizards
//- running properly. The object will not compile but is require by .NET to recognize
//- this project as being an MFC project
#ifdef NEVER
AFX_EXTENSION_MODULE UbomDLL ={ NULL, NULL } ;
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
		UbomDLL.AttachInstance (hInstance) ;
		InitAcUiDLL () ;
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		UbomDLL.DetachInstance () ;
	}
	return (TRUE) ;
}

extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
	switch (msg) {
	case AcRx::kInitAppMsg:
		// Comment out the following line if your
		// application should be locked into memory
		acrxDynamicLinker->unlockApplication(pkt);
		acrxDynamicLinker->registerAppMDIAware(pkt);
		InitApplication();
		break;
	case AcRx::kUnloadAppMsg:
		UnloadApplication();
		break;
	case AcRx::kLoadDwgMsg:
		InitDrawingEnv();
		break;
	}
	return AcRx::kRetOK;
}

