// PPE.h : main header file for the PPE application
//

#if !defined(AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_)
#define AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxwinappex.h"

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#ifndef __PNC_
	#ifndef __PROCESS_PLATE_
		#define __PROCESS_PLATE_
	#endif
	#ifndef __PROCESS_ANGLE_
		#define __PROCESS_ANGLE_
	#endif
#endif

#include "resource.h"       // main symbols
#include "PPEDoc.h"
#include "PPEView.h"

/////////////////////////////////////////////////////////////////////////////
// CPPEApp:
// See PPE.cpp for the implementation of this class
//

void GetSysPath(char* StartPath);
class CPPEApp : public CWinAppEx
{
	WORD m_wDogModule;
public:
	CPPEApp();
	struct STARTER{
		bool m_bChildProcess;		//子进程模式
		/*	PPE启动模式：
		  0.单纯作为子进程启动;
		  1.单构件无返回;
		  2.单构件返回模式;
		  3.多构件无返回;
		  4.多构件返回模式;
		  5.两构件外形比对模式;
		  6.指定文件路径模式
		  7.多构件返回模式包括构件工件工艺信息模式属性(用于TMA工艺信息管理)
		*/
		BYTE mode;
		BYTE m_cProductType;	//高位为1表示启动分享加速模式导入证书　wjh-2016.12.07
		bool IsMultiPartsMode(){return (mode==0||mode==3||mode==4||mode==5||mode==6||mode==7);}
		bool IsDuplexMode(){return m_bChildProcess&&(mode==2||mode==4||mode==7);}
		bool IsCompareMode(){return m_bChildProcess&&mode==5;}
		bool IsOfferFilePathMode(){return m_bChildProcess && mode==6;}
		bool IsSupportPlateEditing();
		bool IsIncPartPattern(){return m_bChildProcess&&mode==7;}
	};
	STARTER starter;
	CBuffer m_xPPEModelBuffer;	//构件信息缓冲
	CPPEDoc* GetDocument();
	CPPEView* GetView();
	void InitPPEModel();
	void GetProductVersion(CString &product_version);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPPEApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CPPEApp)
	UINT  m_nMainMenuID;
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern CPPEApp theApp;
extern char APP_PATH[MAX_PATH];
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_)
