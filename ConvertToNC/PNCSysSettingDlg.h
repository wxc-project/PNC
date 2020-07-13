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
	CPNCSysSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPNCSysSettingDlg();

// �Ի�������
	enum { IDD = IDD_SYSTEM_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	CSuperGridCtrl m_listCtrlSysSetting;
	CHashStrList<CSuperGridCtrl::CTreeItem*> hashGroupByItemName;
	long m_idEventProp;		//��¼�����жϵ�����ID,�ָ�����ʱʹ��
	int m_iSelTabGroup;		//��¼ѡ�еķ������
public:
	void DisplaySystemSetting();
	void UpdatePncSettingProp();
	void UpdateUbomSettingProp();
	void UpdateLayoutProperty(CPropTreeItem* pParentItem);
	void RefreshCtrlState();
	void RefreshListItem();
#ifdef __PNC_
	void SelectEntObj(int nResultEnt=1);	//ѡ�����ڵ����
	void FinishSelectObjOper();				//���ѡ�����ĺ�������
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
