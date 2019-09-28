// SyncPropertyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "SyncPropertyDlg.h"
#include "afxdialogex.h"

//////////////////////////////////////////////////////////////////////////
static BOOL DblclkListSyncDetails(CXhListCtrl* pListCtrl,int iItem,int iSubItem)
{
	CSyncPropertyDlg *pDlg=(CSyncPropertyDlg*)pListCtrl->GetParent();
	if(pDlg==NULL)
		return FALSE;
	char tem_str[101]="";
	pListCtrl->GetItemText(iItem,1,tem_str,100);
	if(stricmp(tem_str,"√")==0)
		pListCtrl->SetItemText(iItem,1,"");
	else
		pListCtrl->SetItemText(iItem,1,"√");
	pDlg->RefeshSyncFlag();
	return TRUE;
}
// CSyncPropertyDlg 对话框

IMPLEMENT_DYNAMIC(CSyncPropertyDlg, CDialog)

CSyncPropertyDlg::CSyncPropertyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSyncPropertyDlg::IDD, pParent)
{
	m_pProcessPart=NULL;
	m_xPropertyList.AddColumnHeader("属性名称",130);
	m_xPropertyList.AddColumnHeader("状态",60);
}

CSyncPropertyDlg::~CSyncPropertyDlg()
{
}

void CSyncPropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_PROPERTY_LIST, m_xPropertyList);
}


BEGIN_MESSAGE_MAP(CSyncPropertyDlg, CDialog)
	ON_NOTIFY(NM_RCLICK, IDC_PROPERTY_LIST, &CSyncPropertyDlg::OnNMRClickListSyncProp)
	ON_COMMAND(ID_NEW_ITEM, OnSetSyncItem)
	ON_COMMAND(ID_DELETE_ITEM, OnRevokeSyncItem)
END_MESSAGE_MAP()


// CSyncPropertyDlg 消息处理程序
BOOL CSyncPropertyDlg::OnInitDialog()
{
	if(m_pProcessPart==NULL)
		return FALSE;
	if(!CDialog::OnInitDialog())
		return FALSE;
	m_xPropertyList.InitListCtrl();
	m_xPropertyList.SetDblclkFunc(DblclkListSyncDetails);
	PROPLIST_TYPE* pListProps=m_pProcessPart->GetSyncPropList();
	if(pListProps!=NULL)
	{
		CStringArray str_arr;
		str_arr.SetSize(2);
		for(PROPLIST_ITEM* pItem=pListProps->EnumFirst();pItem;pItem=pListProps->EnumNext())
		{
			str_arr[0]=(char*)pItem->name;
			str_arr[1]="";
			if(m_pProcessPart->m_dwInheritPropFlag&pItem->id)
				str_arr[1]="√";	
			int iCurr=m_xPropertyList.InsertItemRecord(-1,str_arr);
			if(iCurr>=0)
				m_xPropertyList.SetItemData(iCurr,(DWORD)pItem);
		}
	}
	
	return TRUE;
}
void CSyncPropertyDlg::RefeshSyncFlag()
{
	char tem_str[101]="";
	m_pProcessPart->m_dwInheritPropFlag=0;
	int nNum=m_xPropertyList.GetItemCount();
	for(int i=0;i<nNum;i++)
	{
		PROPLIST_ITEM* pItem=(PROPLIST_ITEM*)m_xPropertyList.GetItemData(i);
		m_xPropertyList.GetItemText(i,1,tem_str,100);
		if(stricmp(tem_str,"√")==0 && pItem)
			m_pProcessPart->m_dwInheritPropFlag|=pItem->id;
	}
}
//
void CSyncPropertyDlg::OnNMRClickListSyncProp(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CPoint point;
	GetCursorPos(&point);
	CRect rect;
	m_xPropertyList.GetWindowRect(&rect);
	if(rect.PtInRect(point))
	{
		CMenu popMenu;
		popMenu.LoadMenu(IDR_ITEM_CMD_POPUP); //加载菜单项
		popMenu.GetSubMenu(0)->DeleteMenu(0,MF_BYPOSITION);
		popMenu.GetSubMenu(0)->AppendMenu(MF_STRING,ID_NEW_ITEM,"同步");
		popMenu.GetSubMenu(0)->AppendMenu(MF_STRING,ID_DELETE_ITEM,"取消同步");
		//弹出右键菜单
		popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,
			point.x,point.y,this);
	}
	*pResult = 0;
}
void CSyncPropertyDlg::OnSetSyncItem()
{
	POSITION pos = m_xPropertyList.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int iItem=m_xPropertyList.GetNextSelectedItem(pos);
		m_xPropertyList.SetItemText(iItem,1,"√");
	}
	RefeshSyncFlag();
}
void CSyncPropertyDlg::OnRevokeSyncItem()
{
	POSITION pos = m_xPropertyList.GetFirstSelectedItemPosition();
	while(pos!=NULL)
	{
		int iItem=m_xPropertyList.GetNextSelectedItem(pos);
		m_xPropertyList.SetItemText(iItem,1,"");
	}
	RefeshSyncFlag();
}
