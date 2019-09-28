// AdjustOrderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "AdjustOrderDlg.h"
#include "afxdialogex.h"


// CAdjustOrderDlg 对话框

IMPLEMENT_DYNAMIC(CAdjustOrderDlg, CDialog)

CAdjustOrderDlg::CAdjustOrderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdjustOrderDlg::IDD, pParent)
{
	pPlate=NULL;
	m_sPartNo = _T("");
	m_nSubListCtrlBtmMargin=0;
	m_nBtBtmMargin=0;
}

CAdjustOrderDlg::~CAdjustOrderDlg()
{
}

void CAdjustOrderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_PART_NO, m_sPartNo);
}


BEGIN_MESSAGE_MAP(CAdjustOrderDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BN_ADD_LS_INDEX, OnBnAddLsIndex)
	ON_BN_CLICKED(IDC_BN_DEL_LS_INDEX, OnBnDelLsIndex)
	ON_LBN_DBLCLK(IDC_LIST_LS_INDEX1, OnBnAddLsIndex)
	ON_LBN_DBLCLK(IDC_LIST_LS_INDEX2, OnBnDelLsIndex)
END_MESSAGE_MAP()


// CAdjustOrderDlg 消息处理程序
BOOL CAdjustOrderDlg::OnInitDialog() 
{
	if(pPlate==NULL)
		return TRUE;
	m_sPartNo.Format("%s",(char*)pPlate->GetPartNo());
	CListBox* pListBox1=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX1);
	BOLT_INFO* pHole=NULL;
	int i=0;
	for(pHole=pPlate->m_xBoltInfoList.GetFirst();pHole;pHole=pPlate->m_xBoltInfoList.GetNext(),i++)
	{
		CString sText;
		sText.Format("%3d",pHole->keyId);
		pListBox1->AddString(sText);
	}
	//计算列表框相对于对话框底边的边距
	RECT rect,clientRect;
	GetClientRect(&clientRect);
	GetDlgItem(IDC_LIST_LS_INDEX1)->GetWindowRect(&rect);	//屏幕坐标系
	ScreenToClient(&rect);
	m_nSubListCtrlBtmMargin=clientRect.bottom-rect.bottom;
	GetDlgItem(IDOK)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_nBtBtmMargin=clientRect.bottom-rect.bottom;
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAdjustOrderDlg::OnBnAddLsIndex() 
{
	CListBox* pListBox1=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX1);
	CListBox* pListBox2=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX2);
	int nSel=pListBox1->GetSelCount();
	if(nSel==LB_ERR)	//无被选中项
		return;
	int *rgIndex=new int[nSel];
	nSel=pListBox1->GetSelItems(nSel,rgIndex);
	CString ss;
	CStringArray ss_arr;
	for(int i=0;i<nSel;i++)
	{
		pListBox1->GetText(rgIndex[i],ss);
		int iCur=pListBox2->AddString(ss);
		ss_arr.Add(ss);
	}
	delete []rgIndex;
	for(i=0;i<nSel;i++)
	{
		int ii=pListBox1->FindString(0,ss_arr[i]);
		if(ii!=LB_ERR)
			pListBox1->DeleteString(ii);
	}
}

void CAdjustOrderDlg::OnBnDelLsIndex() 
{
	CListBox* pListBox1=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX1);
	CListBox* pListBox2=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX2);
	int nSel=pListBox2->GetSelCount();
	if(nSel==LB_ERR)	//无被选中项
		return;
	int *rgIndex=new int[nSel];
	nSel=pListBox2->GetSelItems(nSel,rgIndex);
	CString ss;
	CStringArray ss_arr;
	for(int i=0;i<nSel;i++)
	{
		pListBox2->GetText(rgIndex[i],ss);
		int iSel=pListBox1->AddString(ss);
		ss_arr.Add(ss);
	}
	delete []rgIndex;
	for(i=0;i<nSel;i++)
	{
		int ii=pListBox2->FindString(0,ss_arr[i]);
		if(ii!=LB_ERR)
			pListBox2->DeleteString(ii);
	}
}
void CAdjustOrderDlg::OnOK() 
{
	if(!UpdateData())
		return;
	CListBox* pListBox2=(CListBox*)GetDlgItem(IDC_LIST_LS_INDEX2);
	int nNum=pListBox2->GetCount();
	if(nNum!=pPlate->m_xBoltInfoList.GetNodeNum())
	{
		MessageBox("调整螺栓孔顺序有误!");
		return;
	}
	CString ss;
	for(int i=0;i<nNum;i++)
	{
		pListBox2->GetText(i,ss);
		int iValue=atoi(ss);
		keyList.append(iValue);
	}
	CDialog::OnOK();
}
void CAdjustOrderDlg::OnSize(UINT nType, int cx, int cy)
{
	RECT rect;
	CWnd* pWnd=GetDlgItem(IDC_LIST_LS_INDEX1);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.bottom=cy-m_nSubListCtrlBtmMargin;
		pWnd->MoveWindow(&rect);
	}
	pWnd=GetDlgItem(IDC_LIST_LS_INDEX2);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.bottom=cy-m_nSubListCtrlBtmMargin;
		pWnd->MoveWindow(&rect);
	}
	pWnd=GetDlgItem(IDOK);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		int high=rect.bottom-rect.top;
		rect.bottom=cy-m_nBtBtmMargin;
		rect.top=rect.bottom-high;
		pWnd->MoveWindow(&rect);
	}
	pWnd=GetDlgItem(IDCANCEL);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		int high=rect.bottom-rect.top;
		rect.bottom=cy-m_nBtBtmMargin;
		rect.top=rect.bottom-high;
		pWnd->MoveWindow(&rect);
	}
	CDialog::OnSize(nType, cx, cy);
}
