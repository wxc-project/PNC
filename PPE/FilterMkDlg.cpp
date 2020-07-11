// FilterMkDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "FilterMkDlg.h"
#include "afxdialogex.h"

//////////////////////////////////////////////////////////////////////////
//
static BOOL FireModifyValue(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pSelItem, int iSubItem, CString& sTextValue)
{
	CFilterMkDlg *pDlg = (CFilterMkDlg*)pListCtrl->GetParent();
	if (pDlg == NULL)
		return FALSE;
	FILTER_MK_PARA* pPara = (FILTER_MK_PARA*)pSelItem->m_idProp;
	if (pPara == NULL)
		return FALSE;
	CString sOldValue = _T("");
	if (pSelItem&&pSelItem->m_lpNodeInfo)
		sOldValue = pSelItem->m_lpNodeInfo->GetSubItemText(iSubItem);
	//当前单元格内容已修改，更改构件为编辑状态
	if (sOldValue.CompareNoCase(sTextValue) == 0)
		return FALSE;
	if (iSubItem == 1)
		pPara->m_sThickRange = sTextValue;
	return TRUE;
}

static BOOL FireLButtonDblclk(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem, int iSubItem)
{
	CFilterMkDlg *pDlg = (CFilterMkDlg*)pListCtrl->GetParent();
	if (pDlg == NULL)
		return FALSE;
	int iCurSel = -1;
	POSITION pos = pListCtrl->GetFirstSelectedItemPosition();
	if (pos != NULL)
		iCurSel = pListCtrl->GetNextSelectedItem(pos);
	if (iCurSel < 0)
		return FALSE;
	FILTER_MK_PARA* pSelPara = (FILTER_MK_PARA*)pItem->m_idProp;
	if (pSelPara == NULL)
		return FALSE;
	if (iSubItem == 2)
	{	//Q235
		pSelPara->m_bFileterS = !pSelPara->m_bFileterS;
		CString ss = pSelPara->m_bFileterS ? "√" : "×";
		pListCtrl->SetItemText(iCurSel, 2, ss);
	}
	else if (iSubItem == 3)
	{	//Q345
		pSelPara->m_bFileterH = !pSelPara->m_bFileterH;
		CString ss = pSelPara->m_bFileterH ? "√" : "×";
		pListCtrl->SetItemText(iCurSel, 3, ss);
	}
	else if (iSubItem == 4)
	{	//Q355
		pSelPara->m_bFileterh = !pSelPara->m_bFileterh;
		CString ss = pSelPara->m_bFileterh ? "√" : "×";
		pListCtrl->SetItemText(iCurSel, 4, ss);
	}
	else if (iSubItem == 5)
	{	//Q420
		pSelPara->m_bFileterP = !pSelPara->m_bFileterP;
		CString ss = pSelPara->m_bFileterP ? "√" : "×";
		pListCtrl->SetItemText(iCurSel, 5, ss);
	}
	return TRUE;
}
static BOOL FireDeleteItem(CSuperGridCtrl* pListCtrl, CSuperGridCtrl::CTreeItem* pItem)
{
	CFilterMkDlg *pDlg = (CFilterMkDlg*)pListCtrl->GetParent();
	if (pDlg == NULL)
		return FALSE;
	FILTER_MK_PARA* pSelPara = (FILTER_MK_PARA*)pItem->m_idProp;
	if (pSelPara == NULL)
		return FALSE;
	for (FILTER_MK_PARA* pPara = pDlg->m_arrParaItem.GetFirst(); pPara; pPara = pDlg->m_arrParaItem.GetNext())
	{
		if (pPara == pSelPara)
		{
			pDlg->m_arrParaItem.DeleteCursor();
			break;
		}
	}
	pDlg->m_arrParaItem.Clean();
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
// CFilterMkDlg 对话框
IMPLEMENT_DYNAMIC(CFilterMkDlg, CDialogEx)

CFilterMkDlg::CFilterMkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILTER_MK_DLG, pParent)
{

}

CFilterMkDlg::~CFilterMkDlg()
{
}

void CFilterMkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILTER_LIST, m_listCtrl);
}


BEGIN_MESSAGE_MAP(CFilterMkDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ADD_ITEM, &CFilterMkDlg::OnBnAddItem)
END_MESSAGE_MAP()


// CFilterMkDlg 消息处理程序
BOOL CFilterMkDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//
	//m_listCtrl.EnableSortItems(false, true);
	//m_listCtrl.SetGridLineColor(RGB(220, 220, 220));
	//m_listCtrl.SetEvenRowBackColor(RGB(224, 237, 236));
	m_listCtrl.EmptyColumnHeader();
	m_listCtrl.AddColumnHeader("..", 20);
	m_listCtrl.AddColumnHeader("厚度范围", 80);
	m_listCtrl.AddColumnHeader("Q235", 50);
	m_listCtrl.AddColumnHeader("Q345", 50);
	m_listCtrl.AddColumnHeader("Q355", 50);
	m_listCtrl.AddColumnHeader("Q420", 50);
	m_listCtrl.InitListCtrl();
	m_listCtrl.SetImageWidth(0);	//无缩进
	m_listCtrl.SetModifyValueFunc(FireModifyValue);
	m_listCtrl.SetDeleteItemFunc(FireDeleteItem);
	m_listCtrl.SetLButtonDblclkFunc(FireLButtonDblclk);
	UpdateSysParaItemToUI();
	//
	UpdateData(FALSE);
	return TRUE;
}
void CFilterMkDlg::OnOK()
{
	g_sysPara.filterMKParaList.Empty();
	for (FILTER_MK_PARA* pItem = m_arrParaItem.GetFirst(); pItem; pItem = m_arrParaItem.GetNext())
	{
		FILTER_MK_PARA* pCurItem = g_sysPara.filterMKParaList.append();
		pCurItem->m_sThickRange = pItem->m_sThickRange;
		pCurItem->m_bFileterS = pItem->m_bFileterS;	//Q235
		pCurItem->m_bFileterH = pItem->m_bFileterH;	//Q345
		pCurItem->m_bFileterh = pItem->m_bFileterh;	//Q355
		pCurItem->m_bFileterG = pItem->m_bFileterG;	//Q390
		pCurItem->m_bFileterP = pItem->m_bFileterP;	//Q420
		pCurItem->m_bFileterT = pItem->m_bFileterT;	//Q460
	}
	return CDialogEx::OnOK();
}

void CFilterMkDlg::OnBnAddItem()
{
	FILTER_MK_PARA* pCurItem = m_arrParaItem.append();
	pCurItem->m_sThickRange = "*";
	//
	RefreshListCtrl();
}

void CFilterMkDlg::UpdateSysParaItemToUI()
{
	m_arrParaItem.Empty();
	for (FILTER_MK_PARA* pItem = g_sysPara.filterMKParaList.GetFirst(); pItem; pItem = g_sysPara.filterMKParaList.GetNext())
	{
		FILTER_MK_PARA* pCurItem = m_arrParaItem.append();
		pCurItem->m_sThickRange = pItem->m_sThickRange;
		pCurItem->m_bFileterS = pItem->m_bFileterS;	//Q235
		pCurItem->m_bFileterH = pItem->m_bFileterH;	//Q345
		pCurItem->m_bFileterh = pItem->m_bFileterh;	//Q355
		pCurItem->m_bFileterG = pItem->m_bFileterG;	//Q390
		pCurItem->m_bFileterP = pItem->m_bFileterP;	//Q420
		pCurItem->m_bFileterT = pItem->m_bFileterT;	//Q460
	}
	RefreshListCtrl();
}

void CFilterMkDlg::RefreshListCtrl()
{
	m_listCtrl.DeleteAllItems();
	for (FILTER_MK_PARA* pPara = m_arrParaItem.GetFirst(); pPara; pPara = m_arrParaItem.GetNext())
	{
		CListCtrlItemInfo* lpInfo = new CListCtrlItemInfo();
		lpInfo->AddSubItemText("", TRUE);
		//板厚范围
		lpInfo->AddSubItemText(pPara->m_sThickRange);
		lpInfo->SetControlType(1, GCT_EDIT);
		//Q235	
		lpInfo->AddSubItemText(pPara->m_bFileterS ? "√" : "×", TRUE);
		//Q345
		lpInfo->AddSubItemText(pPara->m_bFileterH ? "√" : "×", TRUE);
		//Q355
		lpInfo->AddSubItemText(pPara->m_bFileterh ? "√" : "×",TRUE);
		//Q420
		lpInfo->AddSubItemText(pPara->m_bFileterP ? "√" : "×",TRUE);
		//
		CSuperGridCtrl::CTreeItem* pItem = m_listCtrl.InsertRootItem(lpInfo);
		if (pItem)
			pItem->m_idProp = (DWORD)pPara;
	}
	m_listCtrl.Redraw();
}


