#pragma once

#include "resource.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "CADCallBackDlg.h"
#include "SuperGridCtrl.h"

#ifdef __PNC_
class CPNCSysSettingDlg : public CCADCallBackDlg
#else
class CPNCSysSettingDlg : public CDialog
#endif
{
	DECLARE_DYNAMIC(CPNCSysSettingDlg)
public:
	CPNCSysSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPNCSysSettingDlg();

	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
// 对话框数据
	enum { IDD = IDD_SYSTEM_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	CHashStrList<CSuperGridCtrl::CTreeItem*> hashGroupByItemName;
	long m_idEventProp;		//记录触发中断的属性ID,恢复窗口时使用
public:
	void DisplaySystemSetting();
	void OnPNCSysDel();
#ifdef __PNC_
	void SelectEntObj(int nResultEnt=1);	//选择对象节点或线
	void FinishSelectObjOper();				//完成选择对象的后续操作
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	// DDX/DDV 支持
	void OnPNCSysAdd();
	void OnPNCSysGroupDel();
	void OnPNCSysGroupAdd();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDefault();
	CSuperGridCtrl m_listCtrlSysSetting;
};
