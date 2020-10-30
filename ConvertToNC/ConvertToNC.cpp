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
#include "SteelSealReactor.h"
#include "PNCDockBarManager.h"
#include "BomExport.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
/////////////////////////////////////////////////////////////////////////////
// ObjectARX EntryPoint
void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// ������ȡ����Ϣ
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"SmartExtractPlate",             // Global function name
		L"SmartExtractPlate",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&SmartExtractPlate);            // Function pointer
	//ϵͳ����
	acedRegCmds->addCommand(L"PNC-MENU",         // Group name 
		L"EnvGeneralSet",          // Global function name
		L"EnvGeneralSet",          // Local function name
		ACRX_CMD_MODAL,      // Type
		&EnvGeneralSet);
#ifndef __UBOM_ONLY_   
	//�༭�ְ���Ϣ
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"SendPartEdit",        // Global function name
		L"SendPartEdit",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SendPartEditor);            // Function pointer
	//��ʾ�ְ���Ϣ
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"PNC",        // Global function name
		L"PNC",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&ShowPartList);
	//���Ƹְ�
	acedRegCmds->addCommand(L"PNC-MENU",           // Group name
		L"draw",        // Global function name
		L"draw",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&DrawPlates);
	//�����ӡ��
	acedRegCmds->addCommand( L"PNC-MENU",         // Group name 
		L"MK",
		L"MK",
		ACRX_CMD_MODAL,
		&InsertMKRect);
	//ͨ����ȡTxt�ļ���������
	acedRegCmds->addCommand( L"PNC-MENU",   // Group name
		L"DrawByTxtFile",					// Global function name
		L"DrawByTxtFile",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&DrawProfileByTxtFile);				// Function pointer
	////�����ı�
	//acedRegCmds->addCommand(L"PNC-MENU",   // Group name
	//	L"ET",					// Global function name
	//	L"ET",					// Local function name
	//	ACRX_CMD_MODAL,							// Type
	//	&ExplodeText);				// Function pointer	
#else
//У�󹹼�������Ϣ
	acedRegCmds->addCommand(L"PNC-MENU",   // Group name
		L"RPP",					// Global function name
		L"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#endif
#ifdef __ALFA_TEST_
//����
	acedRegCmds->addCommand(L"PNC-MENU",   // Group name
		L"TEST",					// Global function name
		L"TEST",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&InternalTest);					// Function pointer
#endif
#else
	// ������ȡ����Ϣ
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"SmartExtractPlate",        // Global function name
		"SmartExtractPlate",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SmartExtractPlate);            // Function pointer
	//ϵͳ����
	acedRegCmds->addCommand("PNC-MENU",         // Group name 
		"EnvGeneralSet",          // Global function name
		"EnvGeneralSet",          // Local function name
		ACRX_CMD_MODAL,      // Type
		&EnvGeneralSet);
#ifndef __UBOM_ONLY_  
	//�༭�ְ���Ϣ
	acedRegCmds->addCommand( "PNC-MENU",           // Group name
		"SendPartEdit",        // Global function name
		"SendPartEdit",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&SendPartEditor);            // Function pointer
	//��ʾ�ְ���Ϣ
	acedRegCmds->addCommand("PNC-MENU",           // Group name
		"PNC",        // Global function name
		"PNC",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&ShowPartList);
	//���Ƹְ�
	acedRegCmds->addCommand("PNC-MENU",           // Group name
		"draw",        // Global function name
		"draw",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&DrawPlates);
	//�����ӡ��
	acedRegCmds->addCommand( "PNC-MENU",         // Group name 
		"MK",
		"MK",
		ACRX_CMD_MODAL,
		&InsertMKRect);
	//ͨ����ȡTxt�ļ���������
	acedRegCmds->addCommand("PNC-MENU",   // Group name
		"TXT",					// Global function name
		"TXT",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&DrawProfileByTxtFile);				// Function pointer
	////�����ı�
	//acedRegCmds->addCommand("PNC-MENU",   // Group name
	//	"ET",					// Global function name
	//	"ET",					// Local function name
	//	ACRX_CMD_MODAL,							// Type
	//	&ExplodeText);				// Function pointer
#else
	//У�󹹼�������Ϣ
	acedRegCmds->addCommand("PNC-MENU",   // Group name
		"RPP",					// Global function name
		"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#endif
#ifdef __ALFA_TEST_
	//����
	acedRegCmds->addCommand("PNC-MENU",   // Group name
		"TEST",					// Global function name
		"TEST",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&InternalTest);					// Function pointer
#endif
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
		line_txt.Replace("��", "=");
		char* final = SearchChar(line_txt, ';',true);
		if (final != NULL)
			*final = 0;
		char *skey = strtok(line_txt, " =,");
		//��������
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
BOOL IsValidAuthorization()
{
#ifdef __UBOM_ONLY_
	#ifdef __PNC_
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_UBOM))
		return FALSE;
	#else
	if (!VerifyValidFunction(TMA_LICFUNC::FUNC_IDENTITY_UBOM))
		return FALSE;
	#endif
#else
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_BASIC))
		return FALSE;
#endif
	return TRUE;
}
void InitApplication()
{
	//���м��ܴ���
	DWORD version[2]={0,20170615};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=2;
	pByteVer[2]=0;
	pByteVer[3]=0;
	char lic_file[MAX_PATH]="";
	char lic_file2[MAX_PATH] = "";
	BYTE cProductType = PRODUCT_PNC;
	GetLicFile(lic_file);
#ifdef __UBOM_ONLY_
	//UBOMģ����Ȩ������װʱʹ��PNC.lic��������������ʹ��ʱʹ��TMA.lic
#ifndef __PNC_
	cProductType = PRODUCT_TMA;
	GetLicFile(lic_file);
	GetLicFile2(lic_file2);
	if (strlen(lic_file)==0)
		strcpy(lic_file, lic_file2);
#endif
#endif
	char key_file[MAX_PATH];
	strcpy(key_file, lic_file);
	//�����Ƿ����ָ���������ŵ��ļ� wht-2017.06.07
	char* separator = SearchChar(key_file, '.', true);
	strcpy(separator, ".key");
	DetectSpecifiedHaspKeyFile(key_file);
	ULONG retCode = ImportLicFile(lic_file, cProductType, version);
	if (retCode != 0 && strlen(lic_file2) > 0 && stricmp(lic_file, lic_file2) != 0)
	{	//����֤��lic_fileʧ��ʱ�����Ե���lic_file2
		//lic_fileΪTMAV4.1֤��·����lic_file2ΪTMAV5.1֤��·�� wht 19-12-27
		strcpy(key_file, lic_file2);
		separator = SearchChar(key_file, '.', true);
		strcpy(separator, ".key");
		DetectSpecifiedHaspKeyFile(key_file);
		retCode = ImportLicFile(lic_file2, cProductType, version);
		strcpy(lic_file, lic_file2);
	}
	if(retCode!=0)
	{
		CXhChar500 errormsgstr;
		if(retCode==-2)
			errormsgstr.Copy("�״�ʹ�ã���δָ����֤���ļ���");
		else if(retCode==-1)
			errormsgstr.Copy("��������ʼ��ʧ��");
		else if(retCode==1)
			errormsgstr.Copy("1#�޷���֤���ļ�");
		else if(retCode==2)
			errormsgstr.Copy("2#֤���ļ��⵽�ƻ�");
		else if(retCode==3)
			errormsgstr.Copy("3#֤������ܹ���ƥ��");
		else if(retCode==4)
			errormsgstr.Copy("4#��Ȩ֤��ļ��ܰ汾����");
		else if(retCode==5)
			errormsgstr.Copy("5#֤������ܹ���Ʒ��Ȩ�汾��ƥ��");
		else if(retCode==6)
			errormsgstr.Copy("6#�����汾ʹ����Ȩ��Χ");
		else if(retCode==7)
			errormsgstr.Copy("7#������Ѱ汾������Ȩ��Χ");
		else if(retCode==8)
			errormsgstr.Copy("8#֤������뵱ǰ���ܹ���һ��");
		else if(retCode==9)
			errormsgstr.Copy("9#��Ȩ���ڣ���������Ȩ");
		else if(retCode==10)
			errormsgstr.Copy("10#����ȱ����Ӧִ��Ȩ�ޣ����Թ���ԱȨ�����г���");
		else if (retCode == 11)
			errormsgstr.Copy("11#��Ȩ�쳣����ʹ�ù���ԱȨ����������֤��");
		else
			errormsgstr.Printf("δ֪���󣬴������%d#", retCode);
		errormsgstr.Append(CXhChar500(".֤��·����%s",lic_file));
		strcpy(BTN_ID_YES_TEXT,"���ļ���");
		strcpy(BTN_ID_NO_TEXT,"�˳�CAD");
		strcpy(BTN_ID_CANCEL_TEXT,"�ر�");
		int nRetCode=MsgBox(adsw_acadMainWnd(),errormsgstr,"������ʾ",MB_YESNOCANCEL|MB_ICONERROR);
		if(nRetCode==IDYES)
		{
			CXhChar500 sPath(lic_file);
			sPath.Replace("PNC.lic","");	//�Ƴ�PNC.lic
			sPath.Replace("TMA.lic", "");
			ShellExecute(NULL,"open",NULL,NULL,sPath,SW_SHOW);
			exit(0);
		}
		else if(nRetCode==IDNO)
			exit(0);
		else
		{	//ж��PNC�����˳�cad
			//kLoadDwgMsg�в��ܵ���sendStringToExecute��acedCommand,acDocManager->curDocument()==NULL,�޷�ִ�� wht 18-12-25
#ifdef __UBOM_ONLY_
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
	if (!IsValidAuthorization())
	{
		AfxMessageBox("���ȱ�ٺϷ�ʹ����Ȩ������ϵ��Ӧ�̣�");
		exit(0);
	}
	//���ز˵���
	RegisterServerComponents();
	//��ȡ�����ļ�
	PNCSysSetImportDefault();
	//��ȡ���ػ��������������ļ�
	separator = SearchChar(lic_file, '.', true);
	strcpy(separator, CXhChar16("%d.fac", DogSerialNo()));
	int nFacFileRetcode = ImportLocalFeatureAccessControlFile(lic_file);
	if (nFacFileRetcode != -1 && nFacFileRetcode < 0)
	{
#ifdef AFX_TARG_ENU_ENGLISH
		AfxMessageBox(CXhChar100("FAC file init fail!%d# %s", nFacFileRetcode, lic_file));
#else
		AfxMessageBox(CXhChar100("FAC�ļ���ʼ��ʧ��!%d# %s", nFacFileRetcode, lic_file));
#endif
	}
	//��ʾ�Ի���
#ifndef __UBOM_ONLY_
	::SetWindowText(adsw_acadMainWnd(), "PNC");
	g_xPNCDockBarManager.DisplayPartListDockBar(CPartListDlg::m_nDlgWidth);
#else
	TraversalUbomConfigFiles();
	ImportUbomConfigFile();	//��ȡ�����ļ�
	CXhChar100 sWndText("UBOM");
	if(g_xUbomModel.m_sCustomizeName.GetLength()>0)
		sWndText.Append(g_xUbomModel.m_sCustomizeName, '-');
	::SetWindowText(adsw_acadMainWnd(), sWndText);
	g_xBomExport.Init();
	if(g_xUbomModel.m_bExeRppWhenArxLoad)
		RevisionPartProcess	();
#endif
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//�˳����ܹ�
	char sGroupName[100]="PNC-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
	//ɾ��acad.rx
	char cad_path[MAX_PATH]="";
	GetModuleFileName(NULL,cad_path,MAX_PATH);
	if(strlen(cad_path)>0)
	{
		char *separator=SearchChar(cad_path,'.',true);
		strcpy(separator,".rx");
		::DeleteFile(cad_path);
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
#ifdef __ZRX_
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
		g_pSteelSealReactor = new CSteelSealReactor();
		g_pPNCDocReactor = new CPNCDocReactor();
		acDocManager->addReactor(g_pPNCDocReactor);
		break;
	case AcRx::kUnloadAppMsg:
		if (g_pSteelSealReactor != NULL)
		{
			delete g_pSteelSealReactor;
			g_pSteelSealReactor = NULL;
		}
		if (g_pPNCDocReactor != NULL)
		{
			delete g_pPNCDocReactor;
			g_pPNCDocReactor = NULL;
		}
		UnloadApplication();
		break;
	case AcRx::kLoadDwgMsg:
		//InitDrawingEnv();
		break;
	}
	return AcRx::kRetOK;
}