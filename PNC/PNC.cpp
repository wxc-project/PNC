
// PNC.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "PNC.h"
#include "PNCDlg.h"
#include "direct.h"
#include "XhLmd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPNCApp

BEGIN_MESSAGE_MAP(CPNCApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPNCApp 构造

CPNCApp::CPNCApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CPNCApp 对象

CPNCApp theApp;


// CPNCApp 初始化
char APP_PATH[MAX_PATH];
void GetAppPath(char* startPath)
{
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];

	_splitpath(__argv[0],drive,dir,fname,ext);
	strcpy(startPath,drive);
	strcat(startPath,dir);
	_chdir(startPath);
}
int ImportLicFile(char* licfile,BYTE cProductType,DWORD version[2])
{
	FILE* fp=fopen(licfile,"rb");
	if(fp==NULL)
		return 1;	//无法打开证书文件
	fseek(fp,0,SEEK_END);
	UINT uBufLen=ftell(fp);	//获取认证文件长度
	CHAR_ARRAY buffer_s(uBufLen);
	fseek(fp,0,SEEK_SET);
	fread(buffer_s,uBufLen,1,fp);
	fclose(fp);
	return ImportLicBuffer(buffer_s,uBufLen,cProductType,version);
}
char* SearchChar(char* srcStr,char ch,bool reverseOrder=false)
{
	if(!reverseOrder)
		return strchr(srcStr,ch);
	else
	{
		int len=strlen(srcStr);
		for(int i=len-1;i>=0;i--)
		{
			if(srcStr[i]==ch)
				return &srcStr[i];
		}
	}
	return NULL;
}
bool DetectSpecifiedHaspKeyFile(const char* default_file)
{
	FILE* fp=fopen(default_file,"rt");
	if(fp==NULL)
		return false;
	bool detected=false;
	CXhChar200 line_txt;//[MAX_PATH];
	CXhChar200 scope_xmlstr;
	scope_xmlstr.Append(
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
		"<haspscope>");
	while(!feof(fp))
	{
		if(fgets(line_txt,line_txt.GetLengthMax(),fp)==NULL)
			break;
		line_txt.Replace("＝","=");
		char* final=SearchChar(line_txt,';');
		if(final!=NULL)
			*final=0;
		char *skey=strtok(line_txt," =,");
		//常规设置
		if(_stricmp(skey,"Key")==0)
		{
			if(skey=strtok(NULL,"=,"))
			{
				scope_xmlstr.Append("<hasp id=\"");
				scope_xmlstr.Append(skey);
				scope_xmlstr.Append("\" />");
				detected=true;
			}
		}
	}
	fclose(fp);
	scope_xmlstr.Append("</haspscope>");
	if(detected)
		SetHaspLoginScope(scope_xmlstr);
	return detected;
}
BOOL CPNCApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
#ifdef __UBOM_ONLY_
	free((void*)m_pszAppName);
	m_pszAppName = _tcsdup(_T("UBOM"));
#endif

	SetRegistryKey(_T("Xerofox"));
	GetAppPath(APP_PATH);
	WriteProfileString("SETUP","APP_PATH",APP_PATH);
	strcpy(execute_path,APP_PATH);
	char lic_file[MAX_PATH],sys_file[MAX_PATH];
	sprintf(sys_file,"%ssys",APP_PATH);
	WriteProfileString("SETUP","SYS_PATH",sys_file);
	sprintf(lic_file,"%sPNC.lic",APP_PATH);
	WriteProfileString("Settings","lic_file",lic_file);

	DWORD version[2];
	BYTE* byteVer=(BYTE*)version;
	WORD wModule=0;
	version[1]=20170615;	//版本发布日期
	byteVer[0]=1;
	byteVer[1]=2;
	byteVer[2]=0;
	byteVer[3]=0;

	m_version[0]=version[0];
	m_version[1]=version[1];
	char default_file[MAX_PATH];
	strcpy(default_file,lic_file);
	//查找是否存在指定加密锁号的文件 wht-2017.06.07
	char* separator=SearchChar(default_file,'.',true);
	strcpy(separator,".key");
	DetectSpecifiedHaspKeyFile(default_file);
	DWORD retCode=0;
	bool passed_liccheck=true;
	CXhChar50 errormsgstr("未指证书读取错误");
	for(int i=0;true;i++){
		retCode=ImportLicFile(lic_file,PRODUCT_PNC,version);
		if(retCode==0)
		{
			passed_liccheck=false;
			if(GetLicVersion()<1000005)
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("The certificate file has been out of date, the current software's version must use the new certificate file！");
#else 
				errormsgstr.Copy("该证书文件已过时，当前软件版本必须使用新证书！");
#endif
			else if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("Software Lacks of legitimate use authorized!");
#else 
				errormsgstr.Copy("软件缺少合法使用授权!");
#endif
			else
			{
				passed_liccheck=true;
				WriteProfileString("Settings","lic_file",lic_file);
			}
			if(!passed_liccheck)
			{
#ifndef _LEGACY_LICENSE
				ExitCurrentDogSession();
#elif defined(_NET_DOG)
				ExitNetLicEnv(m_wDogModule);
#endif
			}
			else
				break;
		}
		else
		{
#ifdef AFX_TARG_ENU_ENGLISH
			if(retCode==-1)
				errormsgstr.Copy("0# Hard dog failed to initialize!");
			else if(retCode==1)
				errormsgstr.Copy("1# Unable to open the license file!");
			else if(retCode==2)
				errormsgstr.Copy("2# License file was damaged!");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3# License file not found or does'nt match the hard lock!");
			else if(retCode==5)
				errormsgstr.Copy("5# License file doesn't match the authorized products in hard lock!");
			else if(retCode==6)
				errormsgstr.Copy("6# Beyond the scope of authorized version !");
			else if(retCode==7)
				errormsgstr.Copy("7# Beyond the scope of free authorize version !");
			else if(retCode==8)
				errormsgstr.Copy("8# The serial number of license file does'nt match the hard lock!");
#else 
			if(retCode==-1)
				errormsgstr.Copy("0#加密狗初始化失败!");
			else if(retCode==1)
				errormsgstr.Copy("1#无法打开证书文件");
			else if(retCode==2)
				errormsgstr.Copy("2#证书文件遭到破坏");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3#执行目录下不存在证书文件或证书文件与加密狗不匹配");
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
#endif
#ifndef _LEGACY_LICENSE
			ExitCurrentDogSession();
#elif defined(_NET_DOG)
			ExitNetLicEnv(m_wDogModule);
#endif
			passed_liccheck=false;
			//return FALSE;
		}
		if(!passed_liccheck)
		{
			DWORD dwRet=ProcessLicenseAuthorize(PRODUCT_PNC,m_version,theApp.execute_path,false,errormsgstr);
			if(dwRet==0)
				return FALSE;
			if(passed_liccheck=(dwRet==1))
				break;	//内部已成功导入证书文件
		}
	}
	if(!passed_liccheck)
	{
#ifndef _LEGACY_LICENSE
		ExitCurrentDogSession();
#elif defined(_NET_DOG)
		ExitNetLicEnv(m_wDogModule);
#endif
		return FALSE;
	}
	if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("证书授权不完整!");
		return FALSE;
	}
	//
	CPNCDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}
	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

int CPNCApp::ExitInstance() 
{
#ifndef _LEGACY_LICENSE
	ExitCurrentDogSession();
#elif defined(_NET_DOG)
	ExitNetLicEnv(m_wDogModule);
#endif
	return CWinApp::ExitInstance();
}
