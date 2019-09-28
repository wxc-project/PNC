// FilterLayerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FilterLayerDlg.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"

// CFilterLayerDlg 对话框

IMPLEMENT_DYNAMIC(CFilterLayerDlg, CDialog)

CFilterLayerDlg::CFilterLayerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterLayerDlg::IDD, pParent)
{
	m_xListCtrl.AddColumnHeader("状态");
	m_xListCtrl.AddColumnHeader("图层名");
}

CFilterLayerDlg::~CFilterLayerDlg()
{
}

void CFilterLayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CTRL, m_xListCtrl);
}


BEGIN_MESSAGE_MAP(CFilterLayerDlg, CDialog)
	ON_COMMAND(ID_MARK_FILTER,OnMarkFilter)
	ON_COMMAND(ID_CANCEL_FILTER,OnCancelFilter)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_CTRL, OnNMRClickListCtrl)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CTRL, OnNMDblclkListCtrl)
END_MESSAGE_MAP()


// CFilterLayerDlg 消息处理程序
BOOL CFilterLayerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	long col_wide_arr[2]={50,130};
	m_xListCtrl.InitListCtrl(col_wide_arr);
	RefreshListCtrl();
	return TRUE;
}
void CFilterLayerDlg::RefreshListCtrl()
{
	ATOM_LIST<CXhChar50> layer_list;
	GetCurDwgLayers(layer_list);
	//
	m_xListCtrl.DeleteAllItems();
	CStringArray str_arr;
	str_arr.SetSize(2);
	for(int i=0;i<layer_list.GetNodeNum();i++)
	{
		str_arr[0]="";
		str_arr[1]=layer_list[i];
		CPNCSysPara::LAYER_ITEM* pItem=g_pncSysPara.GetEdgeLayerItem(str_arr[1]);
		if(pItem && pItem->m_bMark)
			str_arr[0]="√";
		m_xListCtrl.InsertItemRecord(-1,str_arr);
	}
}
void CFilterLayerDlg::ContextMenu(CWnd* pWnd, CPoint point) 
{
	CPoint scr_point = point;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0,MF_BYPOSITION);
	pMenu->AppendMenu(MF_STRING,ID_MARK_FILTER,"标记");
	pMenu->AppendMenu(MF_STRING,ID_CANCEL_FILTER,"取消");
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,scr_point.x,scr_point.y,this);
}
//
void CFilterLayerDlg::OnNMRClickListCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int iCurSel = m_xListCtrl.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED); 
	if(iCurSel<0)
		return;
	CPoint point;
	GetCursorPos(&point);
	ContextMenu(this,point);
	*pResult = 0;
}
void CFilterLayerDlg::OnNMDblclkListCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int iCurSel = m_xListCtrl.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
	if(iCurSel<0)
		return;
	char key[4]="";
	m_xListCtrl.GetItemText(iCurSel,0,key,4);
	if(stricmp(key,"√")==0)
		m_xListCtrl.SetItemText(iCurSel,0,"");
	else
		m_xListCtrl.SetItemText(iCurSel,0,"√");
	*pResult = 0;
}
void CFilterLayerDlg::OnMarkFilter()
{
	POSITION pos = m_xListCtrl.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int iCurSel = m_xListCtrl.GetNextSelectedItem(pos);
		m_xListCtrl.SetItemText(iCurSel,0,"√");
	}
}
void CFilterLayerDlg::OnCancelFilter()
{
	POSITION pos = m_xListCtrl.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int iCurSel = m_xListCtrl.GetNextSelectedItem(pos);
		m_xListCtrl.SetItemText(iCurSel,0,"");
	}
}
void CFilterLayerDlg::OnOK()
{
	g_pncSysPara.EmptyEdgeLayerHash();
	BOOL bHasMarkLayer=FALSE;
	for(int i=0;i<m_xListCtrl.GetItemCount();i++)
	{
		char sTag[50],sLayer[50];
		m_xListCtrl.GetItemText(i,0,sTag,50);
		m_xListCtrl.GetItemText(i,1,sLayer,50);
		CPNCSysPara::LAYER_ITEM* pItem=g_pncSysPara.AppendSpecItem(sLayer);
		pItem->m_sLayer.Copy(sLayer);
		if(stricmp(sTag,"√")==0)
		{
			pItem->m_bMark=TRUE;
			bHasMarkLayer=TRUE;
		}
	}
	if(!bHasMarkLayer)	//未选中任何图层时应情况图层列表 wht 19-01-04
		g_pncSysPara.EmptyEdgeLayerHash();
	return CDialog::OnOK();
}
void CFilterLayerDlg::OnClose()
{
	return CDialog::OnCancel();
}