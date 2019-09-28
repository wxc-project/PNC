#if !defined(AFX_CADCALLBACKDLG_H__F07AFCF4_8860_4C20_A03A_E723AB08E4B6__INCLUDED_)
#define AFX_CADCALLBACKDLG_H__F07AFCF4_8860_4C20_A03A_E723AB08E4B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CADCallBackDlg.h : header file
//

#include "XhCharString.h"
//#include "Tower.h"
#include "f_ent.h"
#include "f_ent_list.h"

/////////////////////////////////////////////////////////////////////////////
// CCADCallBackDlg dialog
class CLDSObject;
typedef struct tagCAD_SCREEN_ENT
{
	CLDSObject *m_pObj;			//保存选择对象对应的铁塔对象
	AcDbObjectId m_idEnt;		//保存选择的实体
	int m_iTagParam;			//附加的选择项数据
	GEPOINT m_ptPick;			//拾取点坐标
	tagCAD_SCREEN_ENT(){memset(this,0,sizeof(tagCAD_SCREEN_ENT));}
}CAD_SCREEN_ENT;
class CCADCallBackDlg : public CDialog
{
// Construction
protected:
	BOOL (*FireCallBackFunc)(CDialog *pDlg);
public:
	CCADCallBackDlg(UINT nIDTemplate,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCADCallBackDlg)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCADCallBackDlg)
	public:
#if defined(_WIN64)
	virtual INT_PTR DoModal();
#else
	virtual int DoModal();
#endif
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCADCallBackDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	long m_iBreakType;			//中断类型 1.拾取点 2.拾取直线 3.执行命令
	BOOL m_bInernalStart;		//内部重新启动对话框
	BOOL m_bPauseBreakExit;		//内部拾取对象时临时中断退出对话框
public:
	BOOL m_bFireListItemChange;	//是否响应ListCtrl中ItemChange事件,一般在OnInitDialog中由于Insert触发的该事件不响应
	int m_nPickEntType;			//0.点 1.数据点对应的节点 2.杆件心线 3.其余实体 wht 11-06-29
	int m_nResultEnt;			//需要选择的实体个数
	f3dPoint m_ptPick;			//保存拾取点坐标
	ATOM_LIST<CAD_SCREEN_ENT> resultList;	//选择集
	CStringArray m_arrCmdPickPrompt;//拾取对象时命令行的提示字符串
	void SetCallBackFunc(BOOL (*func)(CDialog *pDlg)){FireCallBackFunc = func;}
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CADCALLBACKDLG_H__F07AFCF4_8860_4C20_A03A_E723AB08E4B6__INCLUDED_)
