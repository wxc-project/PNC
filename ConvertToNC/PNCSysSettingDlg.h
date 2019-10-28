#pragma once

#include "resource.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "CADCallBackDlg.h"
#include "SuperGridCtrl.h"
class CPNCSysSettingDlg : public CCADCallBackDlg
{
	DECLARE_DYNAMIC(CPNCSysSettingDlg)
	void FinishSelectObjOper();				//完成选择对象的后续操作
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
	void SelectEntObj(int nResultEnt=1);				//选择对象节点或线
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateSave();
	void OnPNCSysDel();
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
