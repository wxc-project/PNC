// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0D7B17BE_07CD_4FA3_893E_F4E48450DA91__INCLUDED_)
#define AFX_STDAFX_H__0D7B17BE_07CD_4FA3_893E_F4E48450DA91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _AFX_SECURE_NO_WARNINGS     // MFC
#define _ATL_SECURE_NO_WARNINGS     // ATL
#define _CRT_SECURE_NO_WARNINGS     // C
#define _CRT_NONSTDC_NO_WARNINGS    // CPOSIX
#define _SCL_SECURE_NO_WARNINGS     // STL
#define _SCL_SECURE_NO_DEPRECATE

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxcontrolbars.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxcview.h>
#include <afxtempl.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include "f_ent.h"
#include "f_alg_fun.h"
#include "f_ent_list.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0D7B17BE_07CD_4FA3_893E_F4E48450DA91__INCLUDED_)
