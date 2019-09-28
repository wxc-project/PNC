// AboutDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PNC.h"
#include "AboutDlg.h"
#include "afxdialogex.h"
#include "XhLmd.h"

// CAboutDlg 对话框

IMPLEMENT_DYNAMIC(CAboutDlg, CDialog)

CAboutDlg::CAboutDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_ABOUT_DLG, pParent)
{

}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MODULE, m_xListCtrl);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_AUTH, &CAboutDlg::OnBnClickedBtnAuth)
END_MESSAGE_MAP()


static CString GetFileTime(const char* file_path)
{
	CString sFileTime;
	HANDLE hFile = NULL;
	WIN32_FIND_DATA FindFileData;
	if ((hFile = FindFirstFile(file_path, &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		FILETIME ftWrite;
		SYSTEMTIME stUTC1;
		//获取文件时间
		FileTimeToLocalFileTime(&FindFileData.ftLastWriteTime, &ftWrite);
		//FileTime->LocalTime
		FileTimeToSystemTime(&ftWrite, &stUTC1);
		//中国时区的信息
		//TIME_ZONE_INFORMATION zinfo;
		//GetTimeZoneInformation(&zinfo);
		//SystemTimeToTzSpecificLocalTime(&zinfo, &stUTC1, &stLocal1);
		sFileTime.Format("%02d/%02d/%02d  %02d:%02d",
			stUTC1.wYear, stUTC1.wMonth, stUTC1.wDay,
			stUTC1.wHour, stUTC1.wMinute);
		FindClose(hFile);
	}
	return sFileTime;
}

// CAboutDlg 消息处理程序
void GetProductVersion(const char* file_path,CString &product_version,CString &sReleaseDateTime)
{
	DWORD dwLen = GetFileVersionInfoSize(file_path, 0);
	BYTE *data = new BYTE[dwLen];
	WORD product_ver[4];
	if (GetFileVersionInfo(file_path, 0, dwLen, data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data, "\\", (void**)&info, &uLen);
		memcpy(product_ver, &info->dwProductVersionMS, 4);
		memcpy(&product_ver[2], &info->dwProductVersionLS, 4);
		product_version.Format("%d.%d.%d.%d", product_ver[1], product_ver[0], product_ver[3], product_ver[2]);
		sReleaseDateTime=GetFileTime(file_path);
	}
	delete data;
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString sSerialStr, sVersion,sReleaseDateTime;
	sSerialStr.Format("授权号:%d", DogSerialNo());
	GetProductVersion(__argv[0],sVersion, sReleaseDateTime);
#if defined(__PNC_)
	SetWindowText("关于 PNC");
	GetDlgItem(IDC_S_VERSION)->SetWindowText("PNC " + sVersion);
#elif defined(__CNC_)
	SetWindowText("关于 CNC");
	GetDlgItem(IDC_S_VERSION)->SetWindowText("CNC " + sVersion);
#endif
	GetDlgItem(IDC_S_DOG)->SetWindowText(sSerialStr);
	//
	m_xListCtrl.AddColumnHeader("模块",100);
	m_xListCtrl.AddColumnHeader("版本",100);
	m_xListCtrl.AddColumnHeader("时间",160);
	m_xListCtrl.InitListCtrl();
	//
	CXhChar50 sModuleArr[4] = {"PNC05.arx","PNC07.arx","PNC10.arx","PNC19.zrx"};
	CStringArray str_arr;
	str_arr.SetSize(3);
#ifdef __UBOM_ONLY_
	str_arr[0] = "UBOM.exe";
#else
	str_arr[0] = "PNC.exe";
#endif
	str_arr[1] = sVersion;
	str_arr[2] = sReleaseDateTime;
	m_xListCtrl.InsertItemRecord(-1, str_arr);
	CXhChar500 sFilePath;
	for (int i = 0; i < 4; i++)
	{
		sFilePath.Printf("%s%s", APP_PATH, (char*)sModuleArr[i]);
		GetProductVersion(sFilePath, sVersion, sReleaseDateTime);
		str_arr[0] = sModuleArr[i];
		str_arr[1] = sVersion;
		str_arr[2] = sReleaseDateTime;
		m_xListCtrl.InsertItemRecord(-1, str_arr);
	}
	return TRUE; 
}

void CAboutDlg::OnBnClickedBtnAuth()
{
#if defined(__PNC_)
	ProcessLicenseAuthorize(PRODUCT_PNC, theApp.m_version, theApp.execute_path, true);
#elif defined(__CNC_)
	ProcessLicenseAuthorize(PRODUCT_CNC, theApp.m_version, theApp.execute_path, true);
#endif
}
