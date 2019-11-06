// PlankConfigDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "PlankConfigDlg.h"
#include "afxdialogex.h"
#include "SysPara.h"
#include "NcPart.h"

// CPlankConfigDlg 对话框
IMPLEMENT_DYNAMIC(CPlankConfigDlg, CDialog)

CPlankConfigDlg::CPlankConfigDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PLANK_CONFIG_DLG, pParent)
{
	m_bCompName = FALSE;
	m_bPrjCode = FALSE;
	m_bPrjName = FALSE;
	m_bTaType = FALSE;	
	m_bTaAlias = FALSE;
	m_bStamyNo = FALSE;
	m_bOperator = FALSE;
	m_bAuditor = FALSE;
	m_bCritic = FALSE;
	m_bPartNo = FALSE;
	m_bMaterial = FALSE;
	m_bThick = FALSE;
	m_bBriefMat = FALSE;
	m_sEditLine = "";
}

CPlankConfigDlg::~CPlankConfigDlg()
{
}

void CPlankConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_EDIT_LINE, m_sEditLine);
	DDX_Check(pDX, IDC_CHK_COMPANY_NAME, m_bCompName);
	DDX_Check(pDX, IDC_CHK_PRJ_CODE, m_bPrjCode);
	DDX_Check(pDX, IDC_CHK_PRJ_NAME, m_bPrjName);
	DDX_Check(pDX, IDC_CHK_TA_TYPE, m_bTaType);
	DDX_Check(pDX, IDC_CHK_TA_ALIAS, m_bTaAlias);
	DDX_Check(pDX, IDC_CHK_STAMP_NO, m_bStamyNo);
	DDX_Check(pDX, IDC_CHK_OPERATOR, m_bOperator);
	DDX_Check(pDX, IDC_CHK_AUDITOR, m_bAuditor);
	DDX_Check(pDX, IDC_CHK_CRITIC, m_bCritic);
	DDX_Check(pDX, IDC_CHK_PARTNO, m_bPartNo);
	DDX_Check(pDX, IDC_CHK_BRIEF_MAT, m_bBriefMat);
	DDX_Check(pDX, IDC_CHK_MAT, m_bMaterial);
	DDX_Check(pDX, IDC_CHK_THICK, m_bThick);
}


BEGIN_MESSAGE_MAP(CPlankConfigDlg, CDialog)
	ON_BN_CLICKED(IDC_BN_ADD_EDIT_LINE, OnBnAddEditLine)
	ON_BN_CLICKED(IDC_BN_DEL_CURRENT_LINE, OnBnDelCurrentLine)
	ON_BN_CLICKED(IDC_BN_CLEAR_EDIT_LINE, OnBnClearEditLine)
	ON_BN_CLICKED(IDC_CHK_COMPANY_NAME, OnChkCompName)
	ON_BN_CLICKED(IDC_CHK_PRJ_CODE, OnChkPrjCode)
	ON_BN_CLICKED(IDC_CHK_PRJ_NAME, OnChkPrjName)
	ON_BN_CLICKED(IDC_CHK_TA_TYPE, OnChkTaType)
	ON_BN_CLICKED(IDC_CHK_TA_ALIAS, OnChkTaAlias)
	ON_BN_CLICKED(IDC_CHK_STAMP_NO, OnChkStamyNo)
	ON_BN_CLICKED(IDC_CHK_OPERATOR, OnChkOperator)
	ON_BN_CLICKED(IDC_CHK_AUDITOR, OnChkAuditor)
	ON_BN_CLICKED(IDC_CHK_CRITIC, OnChkCritic)
	ON_BN_CLICKED(IDC_CHK_PARTNO, OnChkPartNo)
	ON_BN_CLICKED(IDC_CHK_MAT, OnChkMaterial)
	ON_BN_CLICKED(IDC_CHK_THICK, OnChkThick)
	ON_BN_CLICKED(IDC_CHK_BRIEF_MAT, &CPlankConfigDlg::OnChkBriefMat)
END_MESSAGE_MAP()


// CPlankConfigDlg 消息处理程序
BOOL CPlankConfigDlg::OnInitDialog()
{
	LoadDimStyleFromFile(g_sysPara.nc.m_sPlateConfigFilePath);
	char *token, key_str[100];
	sprintf(key_str, "%s", CNCPart::m_sExportPartInfoKeyStr);
	CListBox *pListBox = (CListBox*)GetDlgItem(IDC_LIST_PART_INFO);
	for (token = strtok(key_str, "\n"); token; token = strtok(NULL, "\n"))
		pListBox->AddString(token);
	for (int i = 0; i < pListBox->GetCount(); i++)
	{
		pListBox->GetText(i, key_str);
		for (token = strtok(key_str, "&"); token; token = strtok(NULL, "&"))
			UpdateCheckStateByName(token, TRUE);
	}
	UpdateCheckBoxEnableState();
	UpdateData(FALSE);
	return TRUE;
}

void CPlankConfigDlg::OnOK()
{
	UpdateData();
	CListBox *pListBox = (CListBox*)GetDlgItem(IDC_LIST_PART_INFO);
	CNCPart::m_sExportPartInfoKeyStr = "";
	for (int i = 0; i < pListBox->GetCount(); i++)
	{
		CString ss;
		pListBox->GetText(i, ss);
		if (i == 0)
			CNCPart::m_sExportPartInfoKeyStr = ss;
		else
			CNCPart::m_sExportPartInfoKeyStr += "\n" + ss;
	}
	//
	SaveDimStyleToFile(g_sysPara.nc.m_sPlateConfigFilePath);
	CDialog::OnOK();
}

void CPlankConfigDlg::UpdateCheckStateByName(const char *sName, BOOL bValue)
{
	if (sName == NULL || strlen(sName) <= 0)
		return;
	if (stricmp(sName, "设计单位") == 0)
		m_bCompName = bValue;
	else if (stricmp(sName, "工程编号") == 0)
		m_bPrjCode = bValue;
	else if (stricmp(sName, "工程名称") == 0)
		m_bPrjName = bValue;
	else if (stricmp(sName, "塔型") == 0)
		m_bTaType = bValue;
	else if (stricmp(sName, "代号") == 0)
		m_bTaAlias = bValue;
	else if (stricmp(sName, "钢印号") == 0)
		m_bStamyNo = bValue;
	else if (stricmp(sName, "操作员") == 0)
		m_bOperator = bValue;
	else if (stricmp(sName, "审核人") == 0)
		m_bAuditor = bValue;
	else if (stricmp(sName, "评审人") == 0)
		m_bCritic = bValue;
	else if (stricmp(sName, "件号") == 0)
		m_bPartNo = bValue;
	else if (stricmp(sName, "材质") == 0)
		m_bMaterial = bValue;
	else if (stricmp(sName, "厚度") == 0)
		m_bThick = bValue;
	else if (stricmp(sName, "简化材质字符") == 0)
		m_bBriefMat = bValue;

	UpdateData(FALSE);
}

void CPlankConfigDlg::UpdateCheckBoxEnableState()
{
	GetDlgItem(IDC_CHK_COMPANY_NAME)->EnableWindow(!m_bCompName);
	GetDlgItem(IDC_CHK_PRJ_CODE)->EnableWindow(!m_bPrjCode);
	GetDlgItem(IDC_CHK_PRJ_NAME)->EnableWindow(!m_bPrjName);
	GetDlgItem(IDC_CHK_TA_TYPE)->EnableWindow(!m_bTaType);
	GetDlgItem(IDC_CHK_TA_ALIAS)->EnableWindow(!m_bTaAlias);
	GetDlgItem(IDC_CHK_STAMP_NO)->EnableWindow(!m_bStamyNo);
	GetDlgItem(IDC_CHK_OPERATOR)->EnableWindow(!m_bOperator);
	GetDlgItem(IDC_CHK_AUDITOR)->EnableWindow(!m_bAuditor);
	GetDlgItem(IDC_CHK_CRITIC)->EnableWindow(!m_bCritic);
	GetDlgItem(IDC_CHK_PARTNO)->EnableWindow(!m_bPartNo);
	GetDlgItem(IDC_CHK_BRIEF_MAT)->EnableWindow(!m_bBriefMat);
	GetDlgItem(IDC_CHK_MAT)->EnableWindow(!m_bMaterial);
	GetDlgItem(IDC_CHK_THICK)->EnableWindow(!m_bThick);
}

void CPlankConfigDlg::OnBnAddEditLine()
{
	UpdateData();
	if (m_sEditLine.GetLength() > 0)
	{
		CListBox *pListBox = (CListBox*)GetDlgItem(IDC_LIST_PART_INFO);
		pListBox->AddString(m_sEditLine);
		m_sEditLine = "";
		UpdateData(FALSE);
	}
}

void CPlankConfigDlg::OnBnDelCurrentLine()
{
	CListBox *pListBox = (CListBox*)GetDlgItem(IDC_LIST_PART_INFO);
	if (pListBox->GetCurSel() >= 0)
	{
		CString ss;
		pListBox->GetText(pListBox->GetCurSel(), ss);
		char key_str[200];
		strcpy(key_str, ss);
		for (char *token = strtok(key_str, "&"); token; token = strtok(NULL, "&"))
		{
			UpdateCheckStateByName(token, FALSE);
		}
		UpdateData(FALSE);
		OnBnClearEditLine();
		pListBox->DeleteString(pListBox->GetCurSel());
	}
}

void CPlankConfigDlg::OnBnClearEditLine()
{
	m_sEditLine = "";
	UpdateCheckBoxEnableState();
	UpdateData(FALSE);
}

void CPlankConfigDlg::OnChkPrjName()
{
	GetDlgItem(IDC_CHK_PRJ_NAME)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&工程名称";
	else
		m_sEditLine = "工程名称";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkCompName()
{
	GetDlgItem(IDC_CHK_COMPANY_NAME)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&设计单位";
	else
		m_sEditLine = "设计单位";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkPrjCode()
{
	GetDlgItem(IDC_CHK_PRJ_CODE)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&工程编号";
	else
		m_sEditLine = "工程编号";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkTaType()
{
	GetDlgItem(IDC_CHK_TA_TYPE)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&塔型";
	else
		m_sEditLine = "塔型";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkTaAlias()
{
	GetDlgItem(IDC_CHK_TA_ALIAS)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&代号";
	else
		m_sEditLine = "代号";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkStamyNo()
{
	GetDlgItem(IDC_CHK_STAMP_NO)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&钢印号";
	else
		m_sEditLine = "钢印号";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkOperator()
{
	GetDlgItem(IDC_CHK_OPERATOR)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&操作员";
	else
		m_sEditLine = "操作员";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkAuditor()
{
	GetDlgItem(IDC_CHK_AUDITOR)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&审核人";
	else
		m_sEditLine = "审核人";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkCritic()
{
	GetDlgItem(IDC_CHK_CRITIC)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&评审人";
	else
		m_sEditLine = "评审人";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkPartNo()
{
	GetDlgItem(IDC_CHK_PARTNO)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&件号";
	else
		m_sEditLine = "件号";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkBriefMat()
{
	GetDlgItem(IDC_CHK_BRIEF_MAT)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&简化材质字符";
	else
		m_sEditLine = "简化材质字符";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkMaterial()
{
	GetDlgItem(IDC_CHK_MAT)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&材质";
	else
		m_sEditLine = "材质";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

void CPlankConfigDlg::OnChkThick()
{
	GetDlgItem(IDC_CHK_THICK)->EnableWindow(FALSE);
	if (m_sEditLine.GetLength() > 0)
		m_sEditLine += "&厚度";
	else
		m_sEditLine = "厚度";
	GetDlgItem(IDC_E_EDIT_LINE)->SetWindowText(m_sEditLine);
}

BOOL CPlankConfigDlg::SaveDimStyleToFile(const char* file_path)
{
	CBuffer buffer;
	char *token, key_str[100];
	sprintf(key_str, "%s", CNCPart::m_sExportPartInfoKeyStr);
	BUFFERPOP stack(&buffer, 0);
	buffer.WriteInteger(0);
	for (token = strtok(key_str, "\n"); token; token = strtok(NULL, "\n"))
	{
		buffer.WriteString(token);
		stack.Increment();
	}
	stack.VerifyAndRestore();
	//
	FILE *fp = fopen(file_path, "wb");
	if (fp == NULL)
		return FALSE;
	long version = 2;
	long buf_size = buffer.GetLength();
	fwrite(&version, 4, 1, fp);
	fwrite(&buf_size, 4, 1, fp);
	fwrite(buffer.GetBufferPtr(), buf_size, 1, fp);
	fclose(fp);
	return TRUE;
}

BOOL CPlankConfigDlg::LoadDimStyleFromFile(const char* file_path)
{
	FILE *fp = fopen(file_path, "rb");
	if (fp == NULL)
		return FALSE;
	long version = 0, buf_size = 0;
	fread(&version, 4, 1, fp);
	fread(&buf_size, 4, 1, fp);
	CBuffer buffer;
	buffer.Write(NULL, buf_size);
	fread(buffer.GetBufferPtr(), buf_size, 1, fp);
	fclose(fp);
	//
	CNCPart::m_sExportPartInfoKeyStr.Empty();
	buffer.SeekToBegin();
	CXhChar200 sRow;
	int n = buffer.ReadInteger();
	for (int i = 0; i < n; i++)
	{
		buffer.ReadString(sRow);
		if (CNCPart::m_sExportPartInfoKeyStr.GetLength() > 0)
			CNCPart::m_sExportPartInfoKeyStr += "\n";
		CNCPart::m_sExportPartInfoKeyStr.Append(sRow);
	}
	return TRUE;
}