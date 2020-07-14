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

// 对话框数据
	enum { IDD = IDD_SYSTEM_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	CSuperGridCtrl m_listCtrlSysSetting;
	CHashStrList<CSuperGridCtrl::CTreeItem*> hashGroupByItemName;
	long m_idEventProp;		//记录触发中断的属性ID,恢复窗口时使用
	int m_iSelTabGroup;		//记录选中的分组序号
public:
	void DisplaySystemSetting();
	void UpdatePncSettingProp();
	void UpdateUbomSettingProp();
	void UpdateLayoutProperty(CPropTreeItem* pParentItem);
	void RefreshCtrlState();
	void RefreshListItem();
#ifdef __PNC_
	void SelectEntObj(int nResultEnt=1);	//选择对象节点或线
	void FinishSelectObjOper();				//完成选择对象的后续操作
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedBtnDefault();
	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPNCSysDel();
	afx_msg void OnPNCSysAdd();
};
