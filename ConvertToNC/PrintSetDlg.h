#pragma once

#include ".\resource.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "BatchPrint.h"
// CPrintSetDlg �Ի���

class CPrintSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CPrintSetDlg)
public:
	BYTE m_ciPrintType;
	PLOT_CFG m_xPlotCfg;
public:
	CPrintSetDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPrintSetDlg();
	//
	DECLARE_PROP_FUNC(CPrintSetDlg);
	int GetPropValueStr(long id, char* valueStr, UINT nMaxStrBufLen = 100);
	void DisplayProperty();
// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PRINT_SET_DLG };
#endif
	CPropertyList m_xPropList;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
};
