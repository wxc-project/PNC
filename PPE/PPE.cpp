// PPE.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "PPE.h"
#include "MainFrm.h"
#include "PPEDoc.h"
#include "PPEView.h"
#include "direct.h"
#ifndef _LEGACY_LICENSE
#include "XhLdsLm.h"
#include "LicFuncDef.h"
#else
#include "Lic.h"
#endif
#include "SysPara.h"
#include "PPEModel.h"
#include "ProcBarDlg.h"
#include "PlankConfigDlg.h"
#include "LocalFeatureDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPPEApp

BEGIN_MESSAGE_MAP(CPPEApp, CWinAppEx)
	//{{AFX_MSG_MAP(CPPEApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPEApp construction

CPPEApp::CPPEApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	starter.m_bChildProcess=false;
	starter.m_cProductType=0;
	m_bHiColorIcons = TRUE;
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPPEApp object

CPPEApp theApp;
/////////////////////////////////////////////////////////////////////////////
// CPPEApp initialization
char APP_PATH[MAX_PATH];
void GetSysPath(char* startPath)
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

CProcBarDlg *pProcDlg=NULL;
static int prevPercent=-1;
void DisplayProcess(int percent,char *sTitle)
{
	if(percent>=100)
	{
		if(pProcDlg!=NULL)
		{
			pProcDlg->DestroyWindow();
			delete pProcDlg;
			pProcDlg=NULL;
		}
		return;
	}
	else if(pProcDlg==NULL)
		pProcDlg=new CProcBarDlg(NULL);
	if(pProcDlg->GetSafeHwnd()==NULL)
		pProcDlg->Create();
	if(percent==prevPercent)
		return;	//跟上次进度一致不需要更新
	else
		prevPercent=percent;
	if(sTitle)
		pProcDlg->SetTitle(CString(sTitle));
	else
		pProcDlg->SetTitle("进度");
	pProcDlg->Refresh(percent);
}
void CPPEApp::InitPPEModel()
{
	model.DisplayProcess=DisplayProcess;
	CPPEModel::log2file=&logerr;
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
		if (fgets(line_txt, line_txt.LengthMax(), fp) == NULL)
			break;
		line_txt.Replace("＝", "=");
		char* final = SearchChar(line_txt, ';');
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
#include <DbgHelp.h>
#pragma comment(lib,"DbgHelp.Lib")  //MiniDumpWriteDump链接时用到
LONG WINAPI  PpeExceptionFilter(EXCEPTION_POINTERS *pExptInfo)
{
	// 程序崩溃时，将写入程序目录下的ExceptionDump.dmp文件  
	CXhChar500 sAppPath;
	GetSysPath(sAppPath);
	CXhChar500 sExceptionFilePath("%s\\PpeException.dmp", (char*)sAppPath);
	size_t size = sExceptionFilePath.GetLength();
	if (size > 0)
	{
		HANDLE hFile = ::CreateFile(sExceptionFilePath, GENERIC_WRITE,
			FILE_SHARE_WRITE, NULL, CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION exptInfo;
			exptInfo.ThreadId = ::GetCurrentThreadId();
			exptInfo.ExceptionPointers = pExptInfo;
			//将内存堆栈dump到文件中
			//MiniDumpWriteDump需引入dbghelp头文件
			BOOL bOK = ::MiniDumpWriteDump(::GetCurrentProcess(),
				::GetCurrentProcessId(), hFile, MiniDumpNormal,
				&exptInfo, NULL, NULL);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return 0;
}
BOOL CPPEApp::InitInstance()
{
	::SetUnhandledExceptionFilter(PpeExceptionFilter);	//设置未处理异常过滤器生成dmp文件 wht 19-04-09

	CWinAppEx::InitInstance();
	AfxEnableControlContainer();
	EnableTaskbarInteraction(FALSE);
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

/*#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif*/

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Xerofox"));
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager();
	//Sleep(5000);
	//判断PPE是否以子进程方式进行工作
	if(__argc>1)
	{
		char sTemp[MAX_PATH]="";
		strcpy(sTemp,__argv[1]);
		char *key=strtok(sTemp,"-/");
		if(key&&_stricmp(key,"child")==0)//以网络协同的方式进行工作
			starter.m_bChildProcess=true;
		else					//以单机方式进行工作
			starter.m_bChildProcess=false;
	}
	BYTE cProductType;
	DWORD version[2];
	char lic_file[MAX_PATH],lic_func[MAX_PATH];
	if(starter.m_bChildProcess)
	{
		HANDLE hPipeRead=NULL;
		hPipeRead= GetStdHandle( STD_INPUT_HANDLE );
		if( hPipeRead == INVALID_HANDLE_VALUE)
		{
			AfxMessageBox("获取管道句柄无效\n");
			return FALSE;
		}
		DWORD buffer_size;
		CBuffer buffer(1000000);	//1Mb
		//从匿名管道中读取数据，显示，并返回给服务器
		buffer.ReadFromPipe(hPipeRead,1024);
		buffer.SeekToBegin();
		//1、读取加密狗证书信息
		buffer.ReadByte(&cProductType);
		buffer.ReadDword(&version[0]);
		buffer.ReadDword(&version[1]);
		buffer.ReadString(APP_PATH,MAX_PATH);
		char* pchSpliter = strchr(APP_PATH, '|');
		UINT uiParentProcessLicenseSerialNumber=0;
		if (pchSpliter)
		{	//快速加载父进程证书授权 wjh-2019.5.16
			*pchSpliter = 0;
			uiParentProcessLicenseSerialNumber = atoi(pchSpliter + 1);
			InitializeParentProcessSerialNumber(uiParentProcessLicenseSerialNumber);
		}
		//2、读取子程序启动模式
		buffer.ReadByte(&starter.mode);	
		//3、读取工艺管理缓存
		buffer.ReadDword(&buffer_size);
		m_xPPEModelBuffer.Write(buffer.GetCursorBuffer(),buffer_size);
		m_xPPEModelBuffer.SeekToBegin();
		//
		if(cProductType==PRODUCT_TMA)
		{
			sprintf(lic_file,"%sTMA.lic",APP_PATH);
			strcpy(lic_func,"965BCAC3EC47E7423FA3A5721962D3D1");	//TMAP_LICFUNC::FUNC_IDENTITY_PPE_EDITOR
		}
		else if(cProductType==PRODUCT_LMA)
		{	
			sprintf(lic_file,"%sLMA.lic",APP_PATH);
			strcpy(lic_func,"D2FE7B2AC2CF7D7D418438CBDACBCA70");	//LMA_LICFUNC::FUNC_IDENTITY_PPE_EDITOR
		}
		else if(cProductType==PRODUCT_TSA)
		{	
			sprintf(lic_file,"%sTSA.lic",APP_PATH);
			strcpy(lic_func,"C164E9BC6B42E6352BCFA73B74FA9D20");	//TSA_LICFUNC::FUNC_IDENTITY_BASIC
		}
		else if(cProductType==PRODUCT_LDS)
		{	
			sprintf(lic_file,"%sLDS.lic",APP_PATH);
			strcpy(lic_func,"BC7B509D398E6546D032C7A832629D6B");	//LDS_LICFUNC::FUNC_IDENTITY_BASIC
		}
		else if(cProductType==PRODUCT_TDA)
		{	
			sprintf(lic_file,"%sTDA.lic",APP_PATH);
			strcpy(lic_func,"4178568EB88BA82649D78F40FF5BEAE3");	//TDA_LICFUNC::FUNC_IDENTITY_BASIC
		}
		else if(cProductType==PRODUCT_TAP)
		{
			sprintf(lic_file,"%sTAP.lic",APP_PATH);
			strcpy(lic_func,"521F11746DD1BE876E9811D6649C4CAD");	//TAP_LICFUNC::FUNC_IDENTITY_BASIC
		}
		else if(cProductType==PRODUCT_PNC)
		{
			sprintf(lic_file,"%sPNC.lic",APP_PATH);
			strcpy(lic_func,"E4083E129359BCE0246F1C428FB3A7EA");	//PNC_LICFUNC::FUNC_IDENTITY_PPE_EDITOR
		}
		starter.m_cProductType=cProductType;
	}
	else
	{
#ifndef __PNC_
		BYTE* byteVer=(BYTE*)version;
		version[1]=20130801;	//版本发布日期
		byteVer[0]=5;
		byteVer[1]=1;
		byteVer[2]=6;
		byteVer[3]=0;
		cProductType=PRODUCT_TMA;
		starter.m_cProductType=0;
		//对于重庆江津，由于新老TMA证书存在，证书文件名暂由新TMA证书改为PPE.lic,wjh-2015.1.16
		GetSysPath(APP_PATH);
		sprintf(lic_file,"%sPPE.lic",APP_PATH);
		strcpy(lic_func,"7F601F34C0F884947770541562990587");
#else
		BYTE* byteVer=(BYTE*)version;
		WORD wModule=0;
		version[1]=20170615;	//版本发布日期
		byteVer[0]=1;
		byteVer[1]=2;
		byteVer[2]=0;
		byteVer[3]=0;
		cProductType=PRODUCT_PNC;
		starter.m_cProductType=PRODUCT_PNC;
		GetSysPath(APP_PATH);
		sprintf(lic_file,"%sPNC.lic",APP_PATH);
		strcpy(lic_func,"E4083E129359BCE0246F1C428FB3A7EA");
#endif
	}
	DWORD retCode=0;
	DWORD dogSerial=0;
#ifndef _LEGACY_LICENSE
	char* separator = SearchChar(lic_file, '.', true);
	strcpy(separator, ".key");
	DetectSpecifiedHaspKeyFile(lic_file);
	strcpy(separator, ".lic");
	retCode=ImportLicFile(lic_file,cProductType,version);
	//将来应实现动态区分产品类别, 目前针对重庆江津暂时固化为TMA软件 wjh-2015.1.13
	//retCode=ImportLicFile(lic_file,cProductType,version);
#else
#ifdef _NET_DOG
	m_wDogModule=NetDogModule(lic_file);
	if(!InitNetLicEnv(m_wDogModule))
	{
		AfxMessageBox("0#网络狗初始化失败!");
		return FALSE;
	}
#endif
	else
		retCode=ImportLicFile(lic_file,version);
#endif

	if(retCode==0)
		WriteProfileString("Settings","lic_file",lic_file);
	else
	{
		if(retCode==1)
			AfxMessageBox(CXhChar50("1#无法打开证书文件[%s]",lic_file));
		else if(retCode==2)
			AfxMessageBox("2#证书文件遭到破坏");
		else if(retCode==3)
			AfxMessageBox("3#证书与加密狗不匹配");
		else if(retCode==4)
			AfxMessageBox("4#授权证书的加密版本不对");
		else if(retCode==5)
			AfxMessageBox("5#证书与加密狗产品授权版本不匹配");
		else if(retCode==6)
			AfxMessageBox("6#超出版本使用授权范围");
		else if(retCode==7)
			AfxMessageBox("7#超出免费版本升级授权范围");
		else if(retCode==8)
			AfxMessageBox("8#加密锁与证书序号不匹配");
#ifdef _NET_DOG
		ExitNetLicEnv(m_wDogModule);
#endif
		ExitCurrentDogSession();
		return FALSE;
	}
#ifndef _LEGACY_LICENSE
	dogSerial=DogSerialNo();
	if(!VerifyValidFunction(lic_func))
#else
	if((GetProductFuncFlag()&0x02)==0)
#endif
	{
		AfxMessageBox("证书中无构件工艺编辑器功能授权!");
		return FALSE;
	}
	//
	CString sys_path;
	sys_path = APP_PATH;
	sys_path += "Sys\\";
	WriteProfileString("Settings","SYS_PATH", sys_path);
	//读取本地化特征功能配置文件
	separator=SearchChar(lic_file,'.',true);
	strcpy(separator,CXhChar16("%d.fac",DogSerialNo()));
	int retcode=ImportLocalFeatureAccessControlFile(lic_file);
	//初始化企业定制配置信息
	UINT uiCustomizeSerial = ValidateLocalizeFeature(FEATURE::LOCALIZE_CUSTOMIZE_CLIENT);
	gxLocalizer.InitCustomerSerial(uiCustomizeSerial);

	//读取配置文件
	sprintf(lic_file,"%sPPE.cfg",APP_PATH);
	g_sysPara.Read(lic_file);
	InitPPEModel();
#ifdef __PNC_
	//读取钢板的配置明细
	CString sPlateCfgFile;
	sPlateCfgFile = APP_PATH;
	sPlateCfgFile += "Plate.pds";
	g_sysPara.nc.m_sPlateConfigFilePath = sPlateCfgFile;
	CPlankConfigDlg::LoadDimStyleFromFile(sPlateCfgFile);
#endif
	//
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	//初始化工作空间布局路径
	CString versionStr;
	GetProductVersion(versionStr);
#ifdef __PNC_
	m_strRegSection = "V" + versionStr + "\\WorkspacePNC";
#else
	m_strRegSection = "V" + versionStr + "\\WorkspaceTMA";
#endif
#ifdef __PNC_
	m_nMainMenuID=IDR_MAINFRAME_PNC;
#else
	m_nMainMenuID=IDR_MAINFRAME;
#endif
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		m_nMainMenuID,
		RUNTIME_CLASS(CPPEDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CPPEView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}
int CPPEApp::ExitInstance() 
{
#ifndef _LEGACY_LICENSE
	ExitCurrentDogSession();
#elif _NET_DOG
	ExitNetLicEnv(m_wDogModule);
#endif
	return CWinAppEx::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CString m_sVersion;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
	, m_sVersion(_T(""))
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_S_VERSION, m_sVersion);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CStatic *pIcon=(CStatic*)GetDlgItem(IDC_S_ICON);
	if(pIcon)
	{
#if defined(__PNC_)
		pIcon->SetIcon(theApp.LoadIcon(IDR_MAINFRAME_PNC));
#else
		pIcon->SetIcon(theApp.LoadIcon(IDR_MAINFRAME));
#endif
	}
	
	CString sSerialStr,sVersion;
	sSerialStr.Format(" 授权号:%d",DogSerialNo());
	theApp.GetProductVersion(sVersion);
#if defined(__PNC_)
	SetWindowText("关于 PNC");
	m_sVersion="PNC "+sVersion+sSerialStr;
#else
	SetWindowText("关于 PPE");
	m_sVersion="PPE "+sVersion+sSerialStr;
#endif
	return CDialog::OnInitDialog();
}

// App command to run the dialog
void CPPEApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CPPEApp message handlers
CPPEDoc* CPPEApp::GetDocument()
{
	POSITION pos,docpos;
	pos=AfxGetApp()->m_pDocManager->GetFirstDocTemplatePosition();
	for(CDocTemplate *pDocTemp=AfxGetApp()->m_pDocManager->GetNextDocTemplate(pos);
		pDocTemp;pDocTemp=AfxGetApp()->m_pDocManager->GetNextDocTemplate(pos))
	{
		docpos=pDocTemp->GetFirstDocPosition();
		for(CDocument *pDoc=pDocTemp->GetNextDoc(docpos);pDoc;pDoc=pDocTemp->GetNextDoc(docpos))
		{
			if(pDoc->IsKindOf(RUNTIME_CLASS(CPPEDoc)))
				return (CPPEDoc*)pDoc;
		}
	}
	return NULL;
}
CPPEView* CPPEApp::GetView()
{
	CPPEDoc *pDoc=GetDocument();
	if(pDoc==NULL)
		return NULL;
	CPPEView *pView = (CPPEView*)pDoc->GetView(RUNTIME_CLASS(CPPEView));
	return pView;
}
BYTE GetCurPartType();	//From MainFrm.cpp
bool CPPEApp::STARTER::IsSupportPlateEditing()
{
	if(theApp.starter.IsCompareMode())
		return true;
	else if(theApp.starter.IsMultiPartsMode()||GetCurPartType()==CProcessPart::TYPE_PLATE)
		return true;
	else
		return false;
}

void CPPEApp::GetProductVersion(CString &product_version)
{
	DWORD dwLen=GetFileVersionInfoSize(__argv[0],0);
	BYTE *data=new BYTE[dwLen];
	WORD product_ver[4];
	if(GetFileVersionInfo(__argv[0],0,dwLen,data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data,"\\",(void**)&info,&uLen);
		memcpy(product_ver,&info->dwProductVersionMS,4);
		memcpy(&product_ver[2],&info->dwProductVersionLS,4);
		product_version.Format("%d.%d.%d.%d",product_ver[1],product_ver[0],product_ver[3],product_ver[2]);
	}
	delete data;
}
// CPPEApp 自定义加载/保存方法
void CPPEApp::PreLoadState()
{
	/*BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);*/
}

void CPPEApp::LoadCustomState()
{
}

void CPPEApp::SaveCustomState()
{
}