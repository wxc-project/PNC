// FileFormatSetDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PPE.h"
#include "FileFormatSetDlg.h"
#include "afxdialogex.h"
#include "PPEModel.h"


// CFileFormatSetDlg 对话框

IMPLEMENT_DYNAMIC(CFileFormatSetDlg, CDialogEx)

CFileFormatSetDlg::CFileFormatSetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILE_FORMAT_DLG, pParent)
{
	m_bTaType = FALSE;
	m_bPartNo = FALSE;
	m_bPartMat = FALSE;
	m_bPartThick = FALSE;
	m_bSingleNum = FALSE;
	m_bProcessNum = FALSE;
}

CFileFormatSetDlg::~CFileFormatSetDlg()
{
}

void CFileFormatSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_TA_TYPE, m_bTaType);
	DDX_Check(pDX, IDC_CHK_PART_NO, m_bPartNo);
	DDX_Check(pDX, IDC_CHK_PART_MAT, m_bPartMat);
	DDX_Check(pDX, IDC_CHK_PART_THICK, m_bPartThick);
	DDX_Check(pDX, IDC_CHK_SINGLE_NUM, m_bSingleNum);
	DDX_Check(pDX, IDC_CHK_PROCESS_NUM, m_bProcessNum);
}


BEGIN_MESSAGE_MAP(CFileFormatSetDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHK_TA_TYPE, OnChkTaType)
	ON_BN_CLICKED(IDC_CHK_PART_NO, OnChkPartNo)
	ON_BN_CLICKED(IDC_CHK_PART_MAT, OnChkPartMat)
	ON_BN_CLICKED(IDC_CHK_PART_THICK, OnChkPartThick)
	ON_BN_CLICKED(IDC_CHK_SINGLE_NUM, OnChkSingleNum)
	ON_BN_CLICKED(IDC_CHK_PROCESS_NUM, OnChkProcessNum)
	ON_BN_CLICKED(IDC_BTN_RESET, OnBtnReset)
	ON_EN_CHANGE(IDC_E_FORMAT_TEXT, &CFileFormatSetDlg::OnEnChangeEFormatText)
END_MESSAGE_MAP()


// CFileFormatSetDlg 消息处理程序
BOOL CFileFormatSetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//初始化CheckBox
	m_sKeyArr = model.file_format.m_sKeyMarkArr;
	m_sSplitters = model.file_format.m_sSplitters;
	//
	InitCheckBoxByKeyArr();
	UpdateCheckBoxEnableState();
	UpdateEditLine();
	UpdateData(FALSE);
	return TRUE;
}
void CFileFormatSetDlg::OnOK()
{
	model.file_format.m_sKeyMarkArr = m_sKeyArr;
	model.file_format.m_sSplitters = m_sSplitters;
	return CDialog::OnOK();
}
void CFileFormatSetDlg::UpdateEditLine()
{
	CString sEditLine;
	size_t nSplit = m_sSplitters.size();
	size_t nKeySize = m_sKeyArr.size();
	for (size_t i = 0; i < nKeySize; i++)
	{
		if (sEditLine.GetLength() > 0)
		{
			if (nSplit >= i)
			{	//存在分隔符
				if (m_sSplitters[i - 1].GetLength() > 0)
					sEditLine += m_sSplitters[i - 1];
			}
			else
			{	//不存在分隔符，默认'-'
				sEditLine += "-";
			}
			sEditLine += m_sKeyArr[i];
		}
		else
			sEditLine = m_sKeyArr[i];
	}
	//文件格式末尾带特殊分隔符
	if (nKeySize > 0 && nSplit == nKeySize && m_sSplitters[nSplit - 1].GetLength() > 0)
		sEditLine += m_sSplitters[nSplit - 1];
	GetDlgItem(IDC_E_FORMAT_TEXT)->SetWindowText(sEditLine);
}
void CFileFormatSetDlg::InitCheckBoxByKeyArr()
{
	m_bTaType = m_bPartNo = m_bPartMat = FALSE;
	m_bPartThick = m_bSingleNum = m_bProcessNum = FALSE;
	for (size_t i = 0; i < m_sKeyArr.size(); i++)
	{
		if (m_sKeyArr[i].Equal(CPPEModel::KEY_TA_TYPE))
			m_bTaType = TRUE;
		else if (m_sKeyArr[i].Equal(CPPEModel::KEY_PART_NO))
			m_bPartNo = TRUE;
		else if (m_sKeyArr[i].Equal(CPPEModel::KEY_PART_MAT))
			m_bPartMat = TRUE;
		else if (m_sKeyArr[i].Equal(CPPEModel::KEY_PART_THICK))
			m_bPartThick = TRUE;
		else if (m_sKeyArr[i].Equal(CPPEModel::KEY_SINGLE_NUM))
			m_bSingleNum = TRUE;
		else if (m_sKeyArr[i].Equal(CPPEModel::KEY_PROCESS_NUM))
			m_bProcessNum = TRUE;
	}
}
void CFileFormatSetDlg::UpdateCheckBoxEnableState()
{
	GetDlgItem(IDC_CHK_TA_TYPE)->EnableWindow(!m_bTaType);
	GetDlgItem(IDC_CHK_PART_NO)->EnableWindow(!m_bPartNo);
	GetDlgItem(IDC_CHK_PART_MAT)->EnableWindow(!m_bPartMat);
	GetDlgItem(IDC_CHK_PART_THICK)->EnableWindow(!m_bPartThick);
	GetDlgItem(IDC_CHK_SINGLE_NUM)->EnableWindow(!m_bSingleNum);
	GetDlgItem(IDC_CHK_PROCESS_NUM)->EnableWindow(!m_bProcessNum);
}

void CFileFormatSetDlg::OnBtnReset()
{
	m_sKeyArr.clear();
	InitCheckBoxByKeyArr();
	UpdateCheckBoxEnableState();
	UpdateEditLine();
	UpdateData(FALSE);
}
void CFileFormatSetDlg::OnChkTaType()
{
	GetDlgItem(IDC_CHK_TA_TYPE)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_TA_TYPE);
	UpdateEditLine();
}
void CFileFormatSetDlg::OnChkPartNo()
{
	GetDlgItem(IDC_CHK_PART_NO)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_PART_NO);
	UpdateEditLine();
}
void CFileFormatSetDlg::OnChkPartMat()
{
	GetDlgItem(IDC_CHK_PART_MAT)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_PART_MAT);
	UpdateEditLine();
}
void CFileFormatSetDlg::OnChkPartThick()
{
	GetDlgItem(IDC_CHK_PART_THICK)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_PART_THICK);
	UpdateEditLine();
}
void CFileFormatSetDlg::OnChkSingleNum()
{
	GetDlgItem(IDC_CHK_SINGLE_NUM)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_SINGLE_NUM);
	UpdateEditLine();
}
void CFileFormatSetDlg::OnChkProcessNum()
{
	GetDlgItem(IDC_CHK_PROCESS_NUM)->EnableWindow(FALSE);
	m_sKeyArr.push_back(CPPEModel::KEY_PROCESS_NUM);
	UpdateEditLine();
}

void CFileFormatSetDlg::OnEnChangeEFormatText()
{
	CString sText;
	GetDlgItem(IDC_E_FORMAT_TEXT)->GetWindowText(sText);
	vector<CXhChar16> tmpKeys = m_sKeyArr;
	m_sSplitters.clear();
	m_sKeyArr.clear();
	if (sText.GetLength() > 0)
	{
		char* data = sText.GetBuffer();
		//初始化关键标识
		for (size_t i = 0; i < tmpKeys.size(); i++)
		{
			if (strstr(data, tmpKeys[i]))
				m_sKeyArr.push_back(tmpKeys[i]);
		}
		//解析字符串解析，初始化分隔符
		for (size_t i = 0; i < m_sKeyArr.size(); i++)
		{
			if (i < m_sKeyArr.size() - 1)
			{
				size_t strLen = m_sKeyArr[i].GetLength();
				char* psFind1 = strstr(data, m_sKeyArr[i]);
				char* psFind2 = strstr(data, m_sKeyArr[i + 1]);
				size_t subLen = psFind2 - psFind1 - strLen;
				if (subLen>0)
				{
					char* ss = new char[subLen + 1];
					memset(ss, 0, subLen + 1);
					memcpy(ss, psFind1 + strLen, subLen);
					m_sSplitters.push_back(CXhChar16(ss));
					delete []ss;
				}
				else
					m_sSplitters.push_back(CXhChar16(""));
			}
			else
			{	//处理最后一个位置
				size_t strLen = m_sKeyArr[i].GetLength();
				char* psFind1 = strstr(data, m_sKeyArr[i]);
				size_t subLen = sText.GetLength() - (psFind1 - data) - strLen;
				if (subLen > 0)
				{
					char* ss = new char[subLen + 1];
					memset(ss, 0, subLen + 1);
					memcpy(ss, psFind1 + strLen, subLen);
					m_sSplitters.push_back(CXhChar16(ss));
					delete[]ss;
				}
			}
		}
	}
}
