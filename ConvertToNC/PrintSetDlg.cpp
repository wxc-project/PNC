// PrintSetDlg.cpp: ʵ���ļ�
//

#include "stdafx.h"
#include "PrintSetDlg.h"
#include "PropertyListOper.h"

BOOL ModifyProperty(CPropertyList *pPropList, CPropTreeItem* pItem, CString &valueStr)
{
	CPrintSetDlg* pDlg = (CPrintSetDlg*)pPropList->GetParent();
	if (pItem->m_idProp == CPrintSetDlg::GetPropID("m_sDeviceName"))
	{
		pDlg->m_xPlotCfg.m_sDeviceName.Copy(valueStr);
		//
		CPropTreeItem* pFindItem = pPropList->FindItemByPropId(CPrintSetDlg::GetPropID("m_sPaperSize"),NULL);
		if (pFindItem)
			pFindItem->m_lpNodeInfo->m_cmbItems = CPrintSet::GetPaperSizeCmbItemStr(valueStr);
	}	
	else if (pItem->m_idProp == CPrintSetDlg::GetPropID("m_sPaperSize"))
	{
		pDlg->m_xPlotCfg.m_sPaperSize.Copy(valueStr);
	}
	else if (pItem->m_idProp == CPrintSetDlg::GetPropID("m_ciPaperRot"))
	{
		if (valueStr.CompareNoCase("����") == 0)
			pDlg->m_xPlotCfg.m_ciPaperRot = 'L';
		else
			pDlg->m_xPlotCfg.m_ciPaperRot = 'P';
	}
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
// CPrintSetDlg �Ի���

IMPLEMENT_DYNAMIC(CPrintSetDlg, CDialog)

CPrintSetDlg::CPrintSetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_PRINT_SET_DLG, pParent)
{
	m_ciPrintType = 0;
}

CPrintSetDlg::~CPrintSetDlg()
{
}

void CPrintSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BOX, m_xPropList);
}


BEGIN_MESSAGE_MAP(CPrintSetDlg, CDialog)
END_MESSAGE_MAP()


// CPrintSetDlg ��Ϣ�������
BOOL CPrintSetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PAPER)
		m_xPlotCfg = CBatchPrint::m_xPaperPlotCfg;
	else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PDF)
		m_xPlotCfg = CBatchPrint::m_xPdfPlotCfg;
	else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PNG)
		m_xPlotCfg = CBatchPrint::m_xPngPlotCfg;
	//
	m_xPropList.SetDividerScale(0.4);
	m_xPropList.m_hPromptWnd = GetDlgItem(IDC_E_PROP_HELP_STR)->GetSafeHwnd();
	m_xPropList.SetModifyValueFunc(ModifyProperty);
	InitPropHashtable();
	DisplayProperty();
	UpdateData(FALSE);
	return TRUE;
}

IMPLEMENT_PROP_FUNC(CPrintSetDlg);
int CPrintSetDlg::InitPropHashtable()
{
	int id = 1;
	propHashtable.SetHashTableGrowSize(50);
	propStatusHashtable.CreateHashTable(50);
	AddPropItem("basicInfo", PROPLIST_ITEM(id++, "��ӡ����", "",""));
	AddPropItem("m_ciPrintType", PROPLIST_ITEM(id++, "��ӡ����", "", "��ӡֽ��|��ӡPDF|��ӡPNG"));
	AddPropItem("m_sDeviceName", PROPLIST_ITEM(id++, "�豸����", "",""));
	AddPropItem("m_sPaperSize", PROPLIST_ITEM(id++, "ͼֽ�ߴ�", "",""));
	AddPropItem("m_ciPaperRot", PROPLIST_ITEM(id++, "ͼ�η���", "","����|����"));
	return id;
}

int CPrintSetDlg::GetPropValueStr(long id, char* valueStr, UINT nMaxStrBufLen/*=100*/)
{
	CXhChar100 sText;
	if (id == GetPropID("m_ciPrintType"))
	{
		if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PAPER)
			sText.Copy("��ӡֽ��");
		else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PDF)
			sText.Copy("��ӡPDF");
		else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PNG)
			sText.Copy("��ӡPNG");
	}
	else if (id == GetPropID("m_sDeviceName"))
		sText.Copy(m_xPlotCfg.m_sDeviceName);
	else if (id == GetPropID("m_sPaperSize"))
		sText.Copy(m_xPlotCfg.m_sPaperSize);
	else if (id == GetPropID("m_ciPaperRot"))
	{
		if (m_xPlotCfg.m_ciPaperRot == 'L')
			sText.Copy("����");
		else
			sText.Copy("����");
	}
	if (valueStr)
		StrCopy(valueStr, sText, nMaxStrBufLen);
	return strlen(sText);
}

void CPrintSetDlg::DisplayProperty()
{
	m_xPropList.CleanTree();
	//
	CPropertyListOper<CPrintSetDlg> oper(&m_xPropList, this);
	CPropTreeItem *pGroupItem = NULL, *pPropItem = NULL, *pRootItem = m_xPropList.GetRootItem();
	pGroupItem = oper.InsertPropItem(pRootItem, "basicInfo");
	pPropItem = oper.InsertEditPropItem(pGroupItem, "m_ciPrintType");
	pPropItem->SetReadOnly();
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_sDeviceName");
	pPropItem->m_lpNodeInfo->m_cmbItems = CPrintSet::GetPrintDeviceCmbItemStr();
	pPropItem = oper.InsertCmbListPropItem(pGroupItem, "m_sPaperSize");
	pPropItem->m_lpNodeInfo->m_cmbItems = CPrintSet::GetPaperSizeCmbItemStr(m_xPlotCfg.m_sDeviceName);
	oper.InsertCmbListPropItem(pGroupItem, "m_ciPaperRot");
	m_xPropList.Redraw();
}
void CPrintSetDlg::OnOK()
{
	if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PAPER)
		CBatchPrint::m_xPaperPlotCfg = m_xPlotCfg;
	else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PDF)
		CBatchPrint::m_xPdfPlotCfg = m_xPlotCfg;
	else if (m_ciPrintType == CBatchPrint::PRINT_TYPE_PNG)
		CBatchPrint::m_xPngPlotCfg = m_xPlotCfg;
	return CDialog::OnOK();
}
void CPrintSetDlg::OnCancel()
{
	return CDialog::OnCancel();
}
