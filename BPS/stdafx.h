// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#ifdef _DEBUG
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#define NDEBUG
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#define _AFX_SECURE_NO_WARNINGS     // MFC
#define _ATL_SECURE_NO_WARNINGS     // ATL
#define _CRT_SECURE_NO_WARNINGS     // C
#define _CRT_NONSTDC_NO_WARNINGS    // CPOSIX
#define _SCL_SECURE_NO_WARNINGS     // STL
#define _SCL_SECURE_NO_DEPRECATE

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 类
#include <afxodlgs.h>       // MFC OLE 对话框类
#include <afxdisp.h>        // MFC 自动化类
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 数据库类
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 数据库类
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "comutil.h"
#include "adui.h"
#include "acui.h"
#include "accmd.h"
#include "acuiComboBox.h"
#include "acuiDialog.h"
#include "adsdef.h"
#include "acedads.h"
#include "acdb.h"               // acdb definitions
#include "rxregsvc.h"           // ARX linker
#include "dbapserv.h"           // Host application services
#include "aced.h"               // aced stuff
#include "adslib.h"             // RXADS definitions
#include "acdocman.h"           // MDI document manager
#include "rxmfcapi.h"           // ObjectARX MFC support
#include "actrans.h"			// for dinfine actrTransactionManager
#include "dbents.h"
#include "dbspline.h"
#include "dbelipse.h"
#include "dbpl.h"				// AcDbPolyline
#include "dbhatch.h"			// AcDbHatch
#include "dbxrecrd.h"
#include "dbmain.h"
#include "dbregion.h"
#include "dblayout.h"
#include "acaplmgr.h"
#include "dbplotsetval.h"
#include "dbplotsettings.h"
#include "AcExtensionModule.h"  // Utility class for extension dlls
#include <afxcontrolbars.h>

#include "resource.h"

#ifdef _DEBUG_WAS_DEFINED
#undef NDEBUG
#define _DEBUG
#undef _DEBUG_WAS_DEFINED
#endif
