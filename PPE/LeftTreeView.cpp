// LeftTreeView.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "LeftTreeView.h"
#include "ProcessPart.h"
#include "ProcessPartsHolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLeftTreeView

IMPLEMENT_DYNCREATE(CLeftTreeView, CTreeView)

CLeftTreeView::CLeftTreeView()
{
	m_hAngleParentItem=m_hPlateParentItem=NULL;
}

CLeftTreeView::~CLeftTreeView()
{
}

BEGIN_MESSAGE_MAP(CLeftTreeView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnTvnSelchanged)
END_MESSAGE_MAP()


// CLeftTreeView 诊断

#ifdef _DEBUG
void CLeftTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG


// CLeftTreeView 消息处理程序
void CLeftTreeView::InitTreeView(CStringArray &arrAngleFile,CStringArray &arrPlateFile)
{
	CTreeCtrl &treeCtrl=GetTreeCtrl();
	treeCtrl.ModifyStyle(0,TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_FULLROWSELECT);
	if(treeCtrl.GetSafeHwnd()==NULL)
		return;
	treeCtrl.DeleteAllItems();
	m_hAngleParentItem=treeCtrl.InsertItem("角钢",TVI_ROOT);
	for(int i=0;i<arrAngleFile.GetCount();i++)
		treeCtrl.InsertItem(arrAngleFile[i],m_hAngleParentItem);
	treeCtrl.Expand(m_hAngleParentItem,TVE_EXPAND);
#ifdef __SUPPORT_PLATE_
	m_hPlateParentItem=treeCtrl.InsertItem("钢板",TVI_ROOT);
	for(int i=0;i<arrPlateFile.GetCount();i++)
		treeCtrl.InsertItem(arrPlateFile[i],m_hPlateParentItem);
	treeCtrl.Expand(m_hPlateParentItem,TVE_EXPAND);
#endif
}

void CLeftTreeView::InsertTreeItem(char* sText,BYTE cPartType)
{
	CTreeCtrl &treeCtrl=GetTreeCtrl();
	HTREEITEM hParentItem=NULL;
	if(cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		if(m_hAngleParentItem==NULL)
			m_hAngleParentItem=treeCtrl.InsertItem("角钢",TVI_ROOT);
		hParentItem=m_hAngleParentItem;
	}
	else if(cPartType==CProcessPart::TYPE_PLATE)
	{
		if(m_hPlateParentItem==NULL)
			m_hPlateParentItem=treeCtrl.InsertItem("钢板",TVI_ROOT);
		hParentItem=m_hPlateParentItem;
	}
	HTREEITEM hItem=treeCtrl.InsertItem(sText,hParentItem);
	treeCtrl.SelectItem(hItem);
}

void CLeftTreeView::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CTreeCtrl &treeCtrl=GetTreeCtrl();
	HTREEITEM hItem = treeCtrl.GetSelectedItem();
	if(hItem!=NULL)
	{
		CString sText = treeCtrl.GetItemText(hItem);
		g_partInfoHodler.JumpToSpecPart(sText.GetBuffer());
		CPPEView *pView = theApp.GetView();
		if(pView)
			pView->Refresh();
	}
	*pResult = 0;
}

void CLeftTreeView::UpdateSelectItem()
{
	CTreeCtrl &treeCtrl=GetTreeCtrl();
	CString sFileName=g_partInfoHodler.GetCurFileName();
	if(sFileName.GetLength()<=0)
		return;
	for(int i=0;i<2;i++)
	{
		HTREEITEM hTempItem=treeCtrl.GetChildItem(m_hPlateParentItem);
		if(i==1)
			hTempItem=treeCtrl.GetChildItem(m_hAngleParentItem);
		while(hTempItem)
		{
			CString sText=treeCtrl.GetItemText(hTempItem);
			if(sText.CompareNoCase(sFileName)==0)
			{
				treeCtrl.SelectItem(hTempItem);
				return;
			}
			hTempItem=treeCtrl.GetNextItem(hTempItem,TVGN_NEXT);
		}
	}
}
