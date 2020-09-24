#pragma once
#include ".\Resource.h"
#include "DockBarTemplate.h"
#include "PartListDlg.h"
#include "RevisionDlg.h"

class CPNCDockBarManager {
#ifndef __UBOM_ONLY_
	struct PartList_DLG_ID { static const int DLG_ID = CPartListDlg::IDD; };
	struct PartList_NAME_ID { static const int NAME_ID = IDS_PART_LIST; };
	CDlgDockBar<CPartListDlg, PartList_DLG_ID, PartList_NAME_ID> m_xPartListDockBar;
	
#endif
//
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	struct UBOM_DLG_ID { static const int DLG_ID = CRevisionDlg::IDD; };
	struct UBOM_NAME_ID { static const int NAME_ID = IDS_REVISION; };
	CDlgDockBar< CRevisionDlg, UBOM_DLG_ID, UBOM_NAME_ID> m_xRevisionDockBar;
#endif
public:
	CPNCDockBarManager(){}
	~CPNCDockBarManager()
	{
#ifndef __UBOM_ONLY_
		m_xPartListDockBar.DestoryDockBar();
#endif
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
		m_xRevisionDockBar.DestoryDockBar();
#endif
	}
	//钢板信息显示对话框
#ifndef __UBOM_ONLY_
	void DisplayPartListDockBar(int width = 200) { m_xPartListDockBar.DisplayDockBar(width); }
	void HidePartListDockBar() { m_xPartListDockBar.HideDockBar(); }
	CPartListDlg* GetPartListDlgPtr() { return m_xPartListDockBar.GetDlgPtr(); }
#endif
	//料单校审信息显示对话框
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	void DisplayRevisionDockBar(int width = 200) { m_xRevisionDockBar.DisplayDockBar(width); }
	CRevisionDlg* GetRevisionDlgPtr() { return m_xRevisionDockBar.GetDlgPtr(); }
#endif
};
extern CPNCDockBarManager g_xPNCDockBarManager;
