// PPE.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PPE.h"
#include "MainFrm.h"
#include "PPEDoc.h"
#include "PPEView.h"
#include "direct.h"
#ifdef __HASP_DOG_
#include "XhLdsLm.h"
#else
#include "Lic.h"
#endif
#include "SysPara.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPPEApp

BEGIN_MESSAGE_MAP(CPPEApp, CWinApp)
	//{{AFX_MSG_MAP(CPPEApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPEApp construction

CPPEApp::CPPEApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_bChildProcess=FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPPEApp object

CPPEApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPPEApp initialization
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

BOOL CPPEApp::InitInstance()
{
	AfxEnableControlContainer();

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
	//判断PPE是否以子进程方式进行工作
	if(__argc>1)
	{
		char sTemp[MAX_PATH]="";
		strcpy(sTemp,__argv[1]);
		char *key=strtok(sTemp,"-/");
		if(key&&_stricmp(key,"child")==0)//以网络协同的方式进行工作
			m_bChildProcess=TRUE;
		else					//以单机方式进行工作
			m_bChildProcess=FALSE;
	}
	char lic_file[MAX_PATH],APP_PATH[MAX_PATH];
	GetSysPath(APP_PATH);
	DWORD version[2];
	BYTE* byteVer=(BYTE*)version;
	WORD wModule=0;
	version[1]=20130801;	//版本发布日期
	sprintf(lic_file,"%sLDS.lic",APP_PATH);
	byteVer[0]=1;
	byteVer[1]=1;
	byteVer[2]=1;
	byteVer[3]=2;

	DWORD retCode=0;
#ifdef __HASP_DOG_
	retCode=ImportLicFile(lic_file,PRODUCT_LDS,version);
#else
#ifdef _NET_DOG
	m_wDogModule=NetDogModule(lic_file);
	if(!InitNetLicEnv(m_wDogModule))
		retCode=-1;
#endif
	retCode=ImportLicFile(lic_file,version);
#endif
	if(retCode==0)
		WriteProfileString("Settings","lic_file",lic_file);
	else
	{
#ifdef __HASP_DOG_
		if(GetLicVersion()<1000003)
			AfxMessageBox("该证书文件已过时，当前软件版本必须使用新证书！");
		else 
#endif
		if(retCode==-1)
			AfxMessageBox("0#加密狗初始化失败!");
		else if(retCode==1)
			AfxMessageBox("1#无法打开证书文件");
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
#ifdef __HASP_DOG_
		ExitCurrentDogSession();
#elif defined(_NET_DOG)
		ExitNetLicEnv(m_wDogModule);
#endif
		return FALSE;
	}
	sprintf(lic_file,"%sPPE.cfg",APP_PATH);
	g_sysPara.Read(lic_file);
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
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
#ifdef __HASP_DOG_
	ExitCurrentDogSession();
#elif defined(_NET_DOG)
	ExitNetLicEnv(m_wDogModule);
#endif
	return CWinApp::ExitInstance();
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
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

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
