// PartTreeDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PPE.h"
#include "MainFrm.h"
#include "PartTreeDlg.h"
#include "ProcessPart.h"
#include "InputAnValDlg.h"
#include "PPEModel.h"
#include "NcPart.h"
#include "AdjustOrderDlg.h"
#include "SysPara.h"
#include "SortFunc.h"
#include "ArrayList.h"
#include "LicFuncDef.h"
#include "PartLib.h"
#include "ComparePartNoString.h"

//////////////////////////////////////////////////////////////////////////
//
typedef CProcessPlate* CProcessPlatePtr;
static int ComparePartNo(const CProcessPlatePtr &plate1, const CProcessPlatePtr &plate2)
{
	CXhChar50 sPartNo1 = plate1->GetPartNo();
	CXhChar50 sPartNo2 = plate2->GetPartNo();
	return ComparePartNoString(sPartNo1, sPartNo2, "SHGPT");
}
static int CompareThick(const CProcessPlatePtr &plate1, const CProcessPlatePtr &plate2)
{
	return compare_double(plate1->GetThick(), plate2->GetThick());
}
static int CompareMaterial(const CProcessPlatePtr &plate1, const CProcessPlatePtr &plate2)
{
	int index1 = QuerySteelMatIndex(plate1->cMaterial);
	int index2 = QuerySteelMatIndex(plate2->cMaterial);
	return compare_int(index1, index2);
}
typedef PLATE_GROUP* PLATE_GROUP_PTR;
static int ComparePlateGroupPtr(const PLATE_GROUP_PTR &platePtr1, const PLATE_GROUP_PTR &platePtr2)
{
	int index1 = QuerySteelMatIndex(platePtr1->cMaterial);
	int index2 = QuerySteelMatIndex(platePtr2->cMaterial);
	if (index1 > index2)
		return 1;
	else if (index1 < index2)
		return -1;
	else if (platePtr1->thick > platePtr2->thick)
		return 1;
	else if (platePtr1->thick < platePtr2->thick)
		return -1;
	else
		return 0;

}
typedef CProcessAngle* CProcessAnglePtr;
static int ComparePartNo(const CProcessAnglePtr &jg1, const CProcessAnglePtr &jg2)
{
	CXhChar50 sPartNo1 = jg1->GetPartNo();
	CXhChar50 sPartNo2 = jg2->GetPartNo();
	return ComparePartNoString(sPartNo1, sPartNo2, "SHGPT");
}
static int CompareSpec(const CProcessAnglePtr &jg1, const CProcessAnglePtr &jg2)
{
	if (jg1->GetWidth() > jg2->GetWidth())
		return 1;
	else if (jg1->GetWidth() < jg2->GetWidth())
		return -1;
	else
		return compare_double(jg1->GetThick(), jg2->GetThick());
}
static int CompareMaterial(const CProcessAnglePtr &jg1, const CProcessAnglePtr &jg2)
{
	return compare_char(jg1->cMaterial, jg2->cMaterial);
}
//////////////////////////////////////////////////////////////////////////
// CPartTreeDlg �Ի���
IMPLEMENT_DYNCREATE(CPartTreeDlg, CDialogEx)

CPartTreeDlg::CPartTreeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPartTreeDlg::IDD, pParent)
{
	m_hAngleParentItem=NULL;
	m_hPlateParentItem=NULL;
}

CPartTreeDlg::~CPartTreeDlg()
{
}

void CPartTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CTRL, m_treeCtrl);
}


BEGIN_MESSAGE_MAP(CPartTreeDlg, CDialogEx)
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CTRL, &CPartTreeDlg::OnTvnSelchangedTreeCtrl)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CTRL, OnRclickTreeCtrl)
	ON_COMMAND(ID_DELETE_ITEM, OnDeleteItem)
	ON_COMMAND(ID_SMART_SORT_BOLT, OnSmartSortBolts1)
	ON_COMMAND(ID_SMART_SORT_BOLT_2, OnSmartSortBolts2)
	ON_COMMAND(ID_SMART_SORT_BOLT_3, OnSmartSortBolts3)
	ON_COMMAND(ID_ADJUST_HOLE_ORDER,OnAdjustHoleOrder)
	ON_COMMAND(ID_SORT_BY_PARTNO,OnSortByPartNo)
	ON_COMMAND(ID_SORT_BY_THICK,OnSortByThick)
	ON_COMMAND(ID_SORT_BY_MATERIAL,OnSortByMaterial)
	ON_COMMAND(ID_GROUP_BY_THICK_MATERIAL, OnGroupByThickMaterial)
	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE_CTRL, &CPartTreeDlg::OnTvnKeydownTreeCtrl)
END_MESSAGE_MAP()


// CPartTreeDlg ��Ϣ�������
BOOL CPartTreeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_xModelImages.Create(IDB_PART_TREE, 16, 1, RGB(192,192,192));
	m_treeCtrl.SetImageList(&m_xModelImages,TVSIL_NORMAL);
	return TRUE;
}
BOOL CPartTreeDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			CPPEView* pView = (CPPEView*)theApp.GetView();
			pView->DisplaySysSettingProperty();
			return TRUE;
		}
		if (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT || pMsg->wParam== VK_NEXT)
		{
			HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
			TREEITEM_INFO *pInfo = (hItem != NULL) ? (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem) : NULL;
			if (pInfo && pInfo->itemType == TREEITEM_INFO::INFO_PLATE)
			{
				CMainFrame* pMainFrame = (CMainFrame*)theApp.m_pMainWnd;
				if (pMsg->wParam == VK_LEFT)
					pMainFrame->OnRotateAntiClockwise();
				else if (pMsg->wParam == VK_RIGHT)
					pMainFrame->OnRotateClockwise();
				else
					pMainFrame->OnOverturnPlate();
				return TRUE;
			}
		}
		if (pMsg->wParam == VK_F3 || pMsg->wParam == VK_F4 || pMsg->wParam == VK_F5)
		{
#ifdef __PNC_
			HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
			TREEITEM_INFO *pInfo = (hItem != NULL) ? (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem) : NULL;
			if (pInfo && pInfo->itemType == TREEITEM_INFO::INFO_PLATE)
			{
				CPPEView* pView = (CPPEView*)theApp.GetView();
				if (pMsg->wParam == VK_F3)
					pView->SmartSortBolts(CNCPart::ALG_GREEDY);
				else if (pMsg->wParam == VK_F4)
					pView->SmartSortBolts(CNCPart::ALG_BACKTRACK);
				else
					pView->SmartSortBolts(CNCPart::ALG_ANNEAL);
				return TRUE;
			}
#endif
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
void CPartTreeDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	CWnd *pWnd = GetTreeCtrl();
	if(pWnd->GetSafeHwnd())
		pWnd->MoveWindow(0,0,cx,cy);
}
void CPartTreeDlg::OnOK() 
{
	//ȷ������
}
void CPartTreeDlg::OnCancel() 
{
}
void CPartTreeDlg::OnRclickTreeCtrl(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	CTreeCtrl *pTreeCtrl = GetTreeCtrl();
	pTreeCtrl->ScreenToClient(&HitTestInfo.pt);
	pTreeCtrl->HitTest(&HitTestInfo);
	pTreeCtrl->Select(HitTestInfo.hItem,TVGN_CARET);

	ContextMenu(this,HitTestInfo.pt);
	*pResult = 0;
}
void CPartTreeDlg::OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	if(hItem!=NULL)
	{
		TREEITEM_INFO *pInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
		CProcessPart *pPart=NULL;
		if (pInfo && (pInfo->itemType == TREEITEM_INFO::INFO_ANGLE || pInfo->itemType == TREEITEM_INFO::INFO_PLATE))
			pPart = (CProcessPart*)pInfo->dwRefData;
		
		CPPEView *pView = theApp.GetView();
		if (pPart)
			pView->UpdateCurWorkPartByPartNo(pPart->GetPartNo());
		else
			pView->UpdateCurWorkPartByPartNo(NULL);
		pView->Refresh();
		pView->UpdatePropertyPage();
	}
	*pResult = 0;
}
void CPartTreeDlg::OnTvnKeydownTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	if (pTVKeyDown->wVKey == VK_DELETE)
		OnDeleteItem();
	*pResult = 0;
}
//
void CPartTreeDlg::CancelSelTreeItem()
{
	GetTreeCtrl()->SelectItem(m_hPlateParentItem);
	//ˢ����ͼ
	CPPEView* pView = (CPPEView*)theApp.GetView();
	pView->UpdateCurWorkPartByPartNo(NULL);
	pView->SyncPartInfo(false, true);
	pView->UpdatePropertyPage();
}
//ɾ�������ļ�
void CPartTreeDlg::OnDeleteItem()
{
	int nSelectCount = m_treeCtrl.GetSelectedCount();
	if (nSelectCount <= 0)
		return; 
	if (AfxMessageBox("ȷ��Ҫɾ��ѡ������", MB_YESNO) == IDNO)
		return;
	HTREEITEM hSelItem = NULL;
	for (int i = 0; i < nSelectCount; i++)
	{
		HTREEITEM hSelItem = m_treeCtrl.GetFirstSelectedItem();
		if (hSelItem == NULL)
			return;
		TREEITEM_INFO *pInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
		if (pInfo == NULL || (pInfo->itemType != TREEITEM_INFO::INFO_ANGLE&&pInfo->itemType != TREEITEM_INFO::INFO_PLATE))
			return;
		CProcessPart *pPart = (CProcessPart*)pInfo->dwRefData;
		if (pPart == NULL)
			return;
		model.DeletePart(pPart->GetPartNo());
		m_treeCtrl.DeleteItem(hSelItem);
	}
	//���½���
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	CProcessPart *pPart = NULL;
	if (hItem)
	{
		TREEITEM_INFO *pInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
		if (pInfo && (pInfo->itemType == TREEITEM_INFO::INFO_ANGLE || pInfo->itemType == TREEITEM_INFO::INFO_PLATE))
			pPart = (CProcessPart*)pInfo->dwRefData;
	}
	CPPEView* pView = (CPPEView*)theApp.GetView();
	if (pPart)
		pView->UpdateCurWorkPartByPartNo(pPart->GetPartNo());
	else
		pView->UpdateCurWorkPartByPartNo(NULL);
	pView->UpdatePropertyPage();
	pView->Refresh();
}
//����˨�׽�����������
void CPartTreeDlg::OnSmartSortBolts1()
{
#ifdef __PNC_ 
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CPPEView* pView = (CPPEView*)theApp.GetView();
	pView->SmartSortBolts(CNCPart::ALG_GREEDY);
#endif
}
void CPartTreeDlg::OnSmartSortBolts2()
{
#ifdef __PNC_ 
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CPPEView* pView = (CPPEView*)theApp.GetView();
	pView->SmartSortBolts(CNCPart::ALG_BACKTRACK);
#endif
}
void CPartTreeDlg::OnSmartSortBolts3()
{
#ifdef __PNC_ 
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CPPEView* pView = (CPPEView*)theApp.GetView();
	pView->SmartSortBolts(CNCPart::ALG_ANNEAL);
#endif
}
//�ֶ�������˨��˳��
void CPartTreeDlg::OnAdjustHoleOrder()
{
#ifdef __PNC_ 
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CPPEView* pView = (CPPEView*)theApp.GetView();
	pView->AdjustHoleOrder();
#endif
}
void CPartTreeDlg::OnSortByPartNo()
{
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	if(hSelItem==NULL)
		return;
	if(hSelItem==m_hPlateParentItem)
		UpdatePlateTreeItem();
	else if(hSelItem==m_hAngleParentItem)
		UpdateAngleTreeItem();
}
void CPartTreeDlg::OnSortByThick()
{
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	if(hSelItem==NULL)
		return;
	if(hSelItem==m_hPlateParentItem)
		UpdatePlateTreeItem(1);
	else if(hSelItem==m_hAngleParentItem)
		UpdateAngleTreeItem(1);
}
void CPartTreeDlg::OnSortByMaterial()
{
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	if(hSelItem==NULL)
		return;
	if(hSelItem==m_hPlateParentItem)
		UpdatePlateTreeItem(2);
	else if(hSelItem==m_hAngleParentItem)
		UpdateAngleTreeItem(3);
}
void CPartTreeDlg::OnGroupByThickMaterial()
{
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	if (hSelItem == NULL)
		return;
	if (hSelItem == m_hPlateParentItem)
		UpdatePlateTreeItem(3);
}
//
HTREEITEM CPartTreeDlg::InsertTreeItem( CTreeCtrl &treeCtrl,const char* sText,int nImage,int nSelectedImage,
										HTREEITEM hParent,int nNodeType,void *pData/*=NULL*/)
{
	HTREEITEM hItem = treeCtrl.InsertItem(sText,nImage,nSelectedImage,hParent);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(nNodeType,(DWORD)pData));
	treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	return hItem;
}
void CPartTreeDlg::InitTreeView(const char* cur_part_no/*=NULL*/)
{
	m_treeCtrl.ModifyStyle(0,TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_FULLROWSELECT);
	if(m_treeCtrl.GetSafeHwnd()==NULL)
		return;
	CXhChar100 sText;
	m_treeCtrl.DeleteAllItems();
	CXhPtrSet<CProcessAngle> angleSet;
	CXhPtrSet<CProcessPlate> plateSet;
	model.GetSortedAngleSetAndPlateSet(angleSet,plateSet);
	HTREEITEM hItem=NULL,hCurItem=NULL;
	//�Ǹ��ļ��б�
	if(angleSet.GetNodeNum()>0)
	{
		int nJgNum=angleSet.GetNodeNum();
		m_hAngleParentItem=InsertTreeItem(m_treeCtrl,CXhChar50("�Ǹ�(%d)",nJgNum),IMG_ANGLE_SET,IMG_ANGLE_SET,TVI_ROOT,TREEITEM_INFO::INFO_ANGLESET);
		for(CProcessAngle *pAngle=angleSet.GetFirst();pAngle;pAngle=angleSet.GetNext())
		{
			sText.Printf("%s#%s",(char*)pAngle->GetPartNo(),(char*)pAngle->GetSpec());
			hItem=InsertTreeItem(m_treeCtrl,sText,IMG_ANGLE,IMG_ANGLE_SEL,m_hAngleParentItem,TREEITEM_INFO::INFO_ANGLE,pAngle);
			if(cur_part_no&&pAngle->GetPartNo().Equal(cur_part_no))
				hCurItem=hItem;
		}
		m_treeCtrl.Expand(m_hAngleParentItem,TVE_EXPAND);
	}
	//�ְ��ļ��б�
	if(plateSet.GetNodeNum()>0)
	{
		int nPlateNum=plateSet.GetNodeNum();
		m_hPlateParentItem=InsertTreeItem(m_treeCtrl,CXhChar50("�ְ�(%d)",nPlateNum),IMG_PLATE_SET,IMG_PLATE_SET,TVI_ROOT,TREEITEM_INFO::INFO_PLATESET);
		for(CProcessPlate *pPlate=plateSet.GetFirst();pPlate;pPlate=plateSet.GetNext())
		{
			if(pPlate->m_sRelatePartNo.Length()>0)
				sText.Printf("%s,%s#%s",(char*)pPlate->GetPartNo(),(char*)pPlate->m_sRelatePartNo,(char*)pPlate->GetSpec());
			else
				sText.Printf("%s#%s",(char*)pPlate->GetPartNo(),(char*)pPlate->GetSpec());
			sText.Replace(","," ");
			hItem=InsertTreeItem(m_treeCtrl,sText,IMG_PLATE,IMG_PLATE_SEL,m_hPlateParentItem,TREEITEM_INFO::INFO_PLATE,pPlate);
			if(cur_part_no&&pPlate->GetPartNo().Equal(cur_part_no))
				hCurItem=hItem;
		}
		m_treeCtrl.Expand(m_hPlateParentItem,TVE_EXPAND);
	}
	if(hCurItem)
		m_treeCtrl.SelectItem(hCurItem);
}
void CPartTreeDlg::InsertTreeItem(char* sText,CProcessPart *pPart)
{
	if(sText==NULL||pPart==NULL)
		return;
	HTREEITEM hItem=NULL;
	HTREEITEM hParentItem=NULL;
	if(pPart->m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		if(m_hAngleParentItem==NULL)
			m_hAngleParentItem = InsertTreeItem(m_treeCtrl, "�Ǹ�", IMG_ANGLE_SET, IMG_ANGLE_SET, TVI_ROOT, TREEITEM_INFO::INFO_ANGLESET);
		hParentItem=m_hAngleParentItem;
		hItem=InsertTreeItem(m_treeCtrl,sText,IMG_ANGLE,IMG_ANGLE_SEL,m_hAngleParentItem,TREEITEM_INFO::INFO_ANGLE,pPart);
	}
	else if(pPart->m_cPartType==CProcessPart::TYPE_PLATE)
	{
		if(m_hPlateParentItem==NULL)
			m_hPlateParentItem = InsertTreeItem(m_treeCtrl, "�ְ�", IMG_PLATE_SET, IMG_PLATE_SET, TVI_ROOT, TREEITEM_INFO::INFO_PLATESET);
		hParentItem=m_hPlateParentItem;
		hItem=InsertTreeItem(m_treeCtrl,sText,IMG_PLATE,IMG_PLATE_SEL,m_hPlateParentItem,TREEITEM_INFO::INFO_PLATE,pPart);
	}
	if(hItem!=NULL)
		m_treeCtrl.SelectItem(hItem);
}
void CPartTreeDlg::ContextMenu(CWnd *pWnd, CPoint point)
{
	CPoint scr_point = point;
	CTreeCtrl *pTreeCtrl=GetTreeCtrl();
	pTreeCtrl->ClientToScreen(&scr_point);
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0,MF_BYPOSITION);
	HTREEITEM hItem=pTreeCtrl->GetSelectedItem();
	if(hItem==NULL)
		return;
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItem);
	if(pItemInfo->itemType==TREEITEM_INFO::INFO_ANGLESET)
	{
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_PARTNO,"����������");
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_THICK,"���������");
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_MATERIAL,"����������");
	}
	else if(pItemInfo->itemType==TREEITEM_INFO::INFO_ANGLE)
	{
		pMenu->AppendMenu(MF_STRING,ID_DELETE_ITEM,"ɾ��");
	}
	else if(pItemInfo->itemType==TREEITEM_INFO::INFO_PLATESET)
	{
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_PARTNO,"����������");
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_THICK,"���������");
		pMenu->AppendMenu(MF_STRING,ID_SORT_BY_MATERIAL,"����������");
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_GROUP_BY_THICK_MATERIAL, "�����ʺ�ȷ���");
	}
	else if(pItemInfo->itemType==TREEITEM_INFO::INFO_PLATE)
	{
		pMenu->AppendMenu(MF_STRING,ID_DELETE_ITEM,"ɾ��");
#ifdef __PNC_ 
		if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		{
			pMenu->AppendMenu(MF_STRING, ID_SMART_SORT_BOLT,"��˨�Ż�����1");
			pMenu->AppendMenu(MF_STRING, ID_SMART_SORT_BOLT_2, "��˨�Ż�����2");
			pMenu->AppendMenu(MF_STRING, ID_SMART_SORT_BOLT_3, "��˨�Ż�����3");
			pMenu->AppendMenu(MF_STRING,ID_ADJUST_HOLE_ORDER,"������˨˳��");
		}
#endif
	}
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,scr_point.x,scr_point.y,this);
}
void CPartTreeDlg::JumpSelectItem(BYTE head0_prev1_next2_tail3)
{
	HTREEITEM hParentItem=NULL,hNewItem=NULL,hCurItem=m_treeCtrl.GetSelectedItem();
	if(hCurItem==NULL)
	{
		if(head0_prev1_next2_tail3==1)
			head0_prev1_next2_tail3=0;
		else if(head0_prev1_next2_tail3==2)
			head0_prev1_next2_tail3=3;
	}
	else
		hParentItem=m_treeCtrl.GetParentItem(hCurItem);
	if(hParentItem==NULL&&m_hAngleParentItem)
		hParentItem=m_hAngleParentItem;
	if(hParentItem==NULL&&m_hPlateParentItem)
		hParentItem=m_hPlateParentItem;
	if(head0_prev1_next2_tail3==0)
	{	//��һ��
		if(hParentItem)
			hNewItem=m_treeCtrl.GetChildItem(hParentItem);
	}
	else if(head0_prev1_next2_tail3==3)
	{	//���һ��
		if(hParentItem)
		{
			HTREEITEM hChildItem=m_treeCtrl.GetChildItem(hParentItem);
			while(hChildItem){
				hNewItem=hChildItem;
				hChildItem=m_treeCtrl.GetNextSiblingItem(hChildItem);
			}
		}
	}
	else if(head0_prev1_next2_tail3==1)	//ǰһ��
	{	//ǰһ������һ��ʱֹͣ��ת��������ܵ��²�����Ƿ��Ѿ�У����� wht 19-04-12
		hNewItem=m_treeCtrl.GetPrevVisibleItem(hCurItem);
		//if(hNewItem==NULL)
		//	hNewItem=m_treeCtrl.GetLastVisibleItem();
	}
	else if(head0_prev1_next2_tail3==2)
	{	//��һ�������һ��ʱֹͣ��ת��������ܵ��²�����Ƿ��Ѿ�У����� wht 19 - 04 - 12
		hNewItem=m_treeCtrl.GetNextVisibleItem(hCurItem);
		//if(hNewItem==NULL)
		//	hNewItem=m_treeCtrl.GetFirstVisibleItem();
	}
	CProcessPart *pPart=NULL;
	if(hNewItem)
	{
		TREEITEM_INFO *pInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hNewItem);
		if(pInfo&&(pInfo->itemType==TREEITEM_INFO::INFO_ANGLE||pInfo->itemType==TREEITEM_INFO::INFO_PLATE))
			pPart=(CProcessPart*)pInfo->dwRefData;
	}
	//����ѡ����
	m_treeCtrl.SelectItem(hNewItem);
	//ˢ����ͼ
	CPPEView *pView=theApp.GetView();
	if(pPart)
		pView->UpdateCurWorkPartByPartNo(pPart->GetPartNo());
	else
		pView->UpdateCurWorkPartByPartNo(NULL);
	pView->Refresh();
	pView->UpdatePropertyPage();
}
void CPartTreeDlg::UpdateTreeItemText(HTREEITEM hItem/*=NULL*/)
{
	if(hItem==NULL)
		hItem=m_treeCtrl.GetSelectedItem();
	if(hItem==NULL)
		return;
	TREEITEM_INFO *pInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if(pInfo==NULL||(pInfo->itemType!=TREEITEM_INFO::INFO_PLATE&&
		pInfo->itemType!=TREEITEM_INFO::INFO_ANGLE))
		return;
	CProcessPart *pPart=(CProcessPart*)pInfo->dwRefData;
	if(pPart==NULL)
		return;
	CXhChar50 sText("%s#%s",(char*)pPart->GetPartNo(),(char*)pPart->GetSpec());
	m_treeCtrl.SetItemText(hItem,sText);
}

void CPartTreeDlg::UpdatePlateTreeItem(int sortByPN0_TK1_MAT2/*=0*/)
{
	HTREEITEM hItem=m_treeCtrl.GetSelectedItem();
	if(hItem==NULL || hItem!=m_hPlateParentItem)
		return;
	if (sortByPN0_TK1_MAT2 == 3)
	{
		CHashStrList<PLATE_GROUP> hashPlateByThickMat;
		for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
		{
			if (!pPart->IsPlate())
				continue;
			int thick = (int)(pPart->GetThick());
			CXhChar16 sMat;
			QuerySteelMatMark(pPart->cMaterial, sMat);
			CXhChar50 sKey("%s_%d", (char*)sMat, thick);
			PLATE_GROUP* pPlateGroup = hashPlateByThickMat.GetValue(sKey);
			if (pPlateGroup == NULL)
			{
				pPlateGroup = hashPlateByThickMat.Add(sKey);
				pPlateGroup->thick = thick;
				pPlateGroup->cMaterial = pPart->cMaterial;
				pPlateGroup->sKey.Copy(sKey);
			}
			pPlateGroup->plateSet.append((CProcessPlate*)pPart);
		}
		//��������
		CXhChar100 sText;
		ARRAY_LIST<PLATE_GROUP_PTR> groupPtrList;
		for (PLATE_GROUP *pPlateGroup = hashPlateByThickMat.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThickMat.GetNext())
			groupPtrList.append(pPlateGroup);
		CHeapSort<PLATE_GROUP_PTR>::HeapSort(groupPtrList.m_pData, groupPtrList.Size(), ComparePlateGroupPtr);
		m_treeCtrl.DeleteAllSonItems(m_hPlateParentItem);
		for (int i = 0; i < (int)groupPtrList.Size(); i++)
		{
			PLATE_GROUP *pPlateGroup = groupPtrList[i];
			sText.Printf("%s(%d)", (char*)pPlateGroup->sKey, pPlateGroup->plateSet.GetNodeNum());
			HTREEITEM  hGroupItem = InsertTreeItem(m_treeCtrl, sText, IMG_PLATE_SET, IMG_PLATE_SET, m_hPlateParentItem, TREEITEM_INFO::INFO_PLATE_GROUP, NULL);
			ARRAY_LIST<CProcessPlate*> plateSet;
			for (CProcessPlate *pPlate = pPlateGroup->plateSet.GetFirst(); pPlate; pPlate = pPlateGroup->plateSet.GetNext())
				plateSet.append(pPlate);
			CQuickSort<CProcessPlate*>::QuickSort(plateSet.m_pData, plateSet.GetSize(), ComparePartNo);
			for(int j=0;j<plateSet.GetSize();j++)
			{
				CProcessPlate *pPlate = plateSet[j];
				if (pPlate->m_sRelatePartNo.Length() > 0)
					sText.Printf("%s,%s#%s", (char*)pPlate->GetPartNo(), (char*)pPlate->m_sRelatePartNo, (char*)pPlate->GetSpec());
				else
					sText.Printf("%s#%s", (char*)pPlate->GetPartNo(), (char*)pPlate->GetSpec());
				sText.Replace(",", " ");
				InsertTreeItem(m_treeCtrl, sText, IMG_PLATE, IMG_PLATE_SEL, hGroupItem, TREEITEM_INFO::INFO_PLATE, pPlate);
			}
			m_treeCtrl.Expand(hGroupItem, TVE_COLLAPSE);
		}
		m_treeCtrl.Expand(m_hPlateParentItem, TVE_EXPAND);
	}
	else
	{
		//�Ըְ�������ļ���Ҫ���������
		ARRAY_LIST<CProcessPlate*> platePtrArr;
		for (CProcessPart* pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
		{
			if (pPart->IsPlate())
				platePtrArr.append((CProcessPlate*)pPart);
		}
		CBubbleSort<CProcessPlate*>::BubSort(platePtrArr.m_pData, platePtrArr.GetSize(), ComparePartNo);
		if (sortByPN0_TK1_MAT2 == 1)
			CBubbleSort<CProcessPlate*>::BubSort(platePtrArr.m_pData, platePtrArr.GetSize(), CompareThick);
		else if (sortByPN0_TK1_MAT2 == 2)
			CBubbleSort<CProcessPlate*>::BubSort(platePtrArr.m_pData, platePtrArr.GetSize(), CompareMaterial);
		//���¸ְ������б�
		CXhChar100 sText;
		m_treeCtrl.DeleteAllSonItems(m_hPlateParentItem);
		for (CProcessPlate **ppPlate = platePtrArr.GetFirst(); ppPlate; ppPlate = platePtrArr.GetNext())
		{
			if ((*ppPlate)->m_sRelatePartNo.Length() > 0)
				sText.Printf("%s,%s#%s", (char*)(*ppPlate)->GetPartNo(), (char*)(*ppPlate)->m_sRelatePartNo, (char*)(*ppPlate)->GetSpec());
			else
				sText.Printf("%s#%s", (char*)(*ppPlate)->GetPartNo(), (char*)(*ppPlate)->GetSpec());
			sText.Replace(",", " ");
			InsertTreeItem(m_treeCtrl, sText, IMG_PLATE, IMG_PLATE_SEL, m_hPlateParentItem, TREEITEM_INFO::INFO_PLATE, *ppPlate);
		}
		m_treeCtrl.Expand(m_hPlateParentItem, TVE_EXPAND);
	}
}
void CPartTreeDlg::UpdateAngleTreeItem(int sortByPN0_SPEC1_MAT2/*=0*/)
{
	HTREEITEM hItem=m_treeCtrl.GetSelectedItem();
	if(hItem==NULL || hItem!=m_hAngleParentItem)
		return;
	//�ԽǸֵ������ļ���Ҫ���������
	ARRAY_LIST<CProcessAngle*> jgPtrArr;
	for(CProcessPart* pPart=model.EnumPartFirst();pPart;pPart=model.EnumPartNext())
	{
		if(pPart->IsAngle())
			jgPtrArr.append((CProcessAngle*)pPart);
	}
	CBubbleSort<CProcessAngle*>::BubSort(jgPtrArr.m_pData,jgPtrArr.GetSize(),ComparePartNo);
	if(sortByPN0_SPEC1_MAT2==1)
		CBubbleSort<CProcessAngle*>::BubSort(jgPtrArr.m_pData,jgPtrArr.GetSize(),CompareSpec);
	else if(sortByPN0_SPEC1_MAT2==2)
		CBubbleSort<CProcessAngle*>::BubSort(jgPtrArr.m_pData,jgPtrArr.GetSize(),CompareMaterial);
	//���¸ְ������б�
	CXhChar100 sText;
	m_treeCtrl.DeleteAllSonItems(m_hAngleParentItem);
	for(CProcessAngle **ppAngle=jgPtrArr.GetFirst();ppAngle;ppAngle=jgPtrArr.GetNext())
	{
		sText.Printf("%s#%s",(char*)(*ppAngle)->GetPartNo(),(char*)(*ppAngle)->GetSpec());
		InsertTreeItem(m_treeCtrl,sText,IMG_ANGLE,IMG_ANGLE_SEL,m_hAngleParentItem,TREEITEM_INFO::INFO_ANGLE,*ppAngle);
	}
	m_treeCtrl.Expand(m_hAngleParentItem,TVE_EXPAND);
}