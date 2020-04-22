#pragma once

#include "HashTable.h"
#include "f_ent_list.h"
#include "XhCharString.h"
#include "resource.h"
#include "ServerTowerType.h"

// CSelectTowerTypeDlg 对话框

class CSelectTowerTypeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSelectTowerTypeDlg)
	class CServerProject
	{
	public:
		int id;
		CXhChar200 m_sProjectName;
		ATOM_LIST<CServerTowerType*> towerTypeList;
	};
	CHashList<CServerProject> hashProjectById;
	void InitProjectTowerTypes();
public:
	CSelectTowerTypeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSelectTowerTypeDlg();
	CServerTowerType *m_pTowerType;
// 对话框数据
	enum { IDD = IDD_SELECT_TOWER_TYPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CComboBox m_cmbProject;
	CComboBox m_cmbTowerType;
	afx_msg void OnCbnSelchangeCmbProjectName();
	virtual void OnOK();
};
