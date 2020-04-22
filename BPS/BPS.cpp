// BPS.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>
#include "XhLicAgent.h"
#include "XhLdsLm.h"
#include "LicFuncDef.h"
#include "BPSMenu.h"
#include "BPSModel.h"
#include "CadToolFunc.h"
#include "SelectJgCardDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// 智能提取角钢
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"BP",             // Global function name
		L"BP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&BatchPrintPartProcessCards);            // Function pointer
	// 上传角钢工艺卡
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"RP",             // Global function name
		L"RP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&RetrievedProcessCards);            // Function pointer
	// 上传角钢工艺卡
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"AP",             // Global function name
		L"AP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&AppendOrUpdateProcessCards);            // Function pointer
	//优化排序设置
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"OptimalSortSet",        // Global function name
		L"OptimalSortSet",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&OptimalSortSet);            // Function pointer
#else
	// 智能提取角钢
	acedRegCmds->addCommand( "BPS-MENU",           // Group name
		"BP",        // Global function name
		"BP",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&BatchPrintPartProcessCards);            // Function pointer
	//优化排序设置
	acedRegCmds->addCommand( "BPS-MENU",           // Group name
		"OptimalSortSet",        // Global function name
		"OptimalSortSet",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&OptimalSortSet);            // Function pointer
#endif
}
void InitApplication()
{
	//进行加密处理
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=4;
	pByteVer[1]=1;
	pByteVer[2]=0;
	pByteVer[3]=5;
	char lic_file[MAX_PATH];
	if(GetLicFile2(lic_file)==FALSE)
	{
		AfxMessageBox("证书文件路径读取失败!");
		return;
	}
	ULONG retCode=ImportLicFile(lic_file,PRODUCT_TMA,version);
	if(retCode!=0)
	{
		CXhChar500 sInfo;
		if(retCode==-2)
			sInfo.Copy("首次使用，还未指定过证书文件！");
		else if(retCode==-1)
			sInfo.Copy("加密锁初始化失败");
		else if(retCode==1)
			sInfo.Copy("1#无法打开证书文件");
		else if(retCode==2)
			sInfo.Copy("2#证书文件遭到破坏");
		else if(retCode==3)
			sInfo.Copy("3#证书与加密狗不匹配");
		else if(retCode==4)
			sInfo.Copy("4#授权证书的加密版本不对");
		else if(retCode==5)
			sInfo.Copy("5#证书与加密狗产品授权版本不匹配");
		else if(retCode==6)
			sInfo.Copy("6#超出版本使用授权范围");
		else if(retCode==7)
			sInfo.Copy("7#超出免费版本升级授权范围");
		else if(retCode==8)
			sInfo.Copy("8#证书序号与当前加密狗不一致");
		sInfo.Append("证书路径:");
		sInfo.Append(lic_file);
		AfxMessageBox(sInfo);
		ExitCurrentDogSession();
		exit(0);
	}
	if(!VerifyValidFunction(TMA_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("软件缺少合法使用授权!");
		exit(0);
	}
	HWND hWnd = adsw_acadMainWnd();
	::SetWindowText(hWnd,"BPS");
	RegisterServerComponents();
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//退出加密狗
	char sGroupName[100]="BPS-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
}

static AFX_EXTENSION_MODULE BPSDLL = { NULL, NULL };
extern "C" int APIENTRY
	DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// 如果使用 lpReserved，请将此移除
	UNREFERENCED_PARAMETER(lpReserved);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("BPS.DLL 正在初始化!\n");

		// 扩展 DLL 一次性初始化
		if (!AfxInitExtensionModule(BPSDLL, hInstance))
			return 0;

		new CDynLinkLibrary(BPSDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("BPS.DLL 正在终止!\n");

		// 在调用析构函数之前终止该库
		AfxTermExtensionModule(BPSDLL);
	}
	return 1;   // 确定
}
extern "C" 
AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
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