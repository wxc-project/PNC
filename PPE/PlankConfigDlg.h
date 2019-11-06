#pragma once


// CPlankConfigDlg 对话框

class CPlankConfigDlg : public CDialog
{
	DECLARE_DYNAMIC(CPlankConfigDlg)
public:
	CPlankConfigDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPlankConfigDlg();
	void UpdateCheckStateByName(const char *sName, BOOL bValue);
	void UpdateCheckBoxEnableState();
	//
	static BOOL SaveDimStyleToFile(const char* file_path);
	static BOOL LoadDimStyleFromFile(const char* file_path);
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PLANK_CONFIG_DLG };
#endif
	CString	m_sEditLine;
	BOOL m_bCompName;	//设计单位
	BOOL m_bPrjCode;	//工程编号
	BOOL m_bPrjName;	//工程名称
	BOOL m_bTaType;		//塔型
	BOOL m_bTaAlias;	//代号
	BOOL m_bStamyNo;	//钢印号
	BOOL m_bOperator;	//操作员
	BOOL m_bAuditor;	//审核人
	BOOL m_bCritic;		//评审人
	BOOL m_bPartNo;		//件号
	BOOL m_bBriefMat;	//简化材质
	BOOL m_bMaterial;	//材质
	BOOL m_bThick;		//厚度
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnAddEditLine();
	afx_msg void OnBnDelCurrentLine();
	afx_msg void OnBnClearEditLine();
	afx_msg void OnChkPrjName();
	afx_msg void OnChkCompName();
	afx_msg void OnChkPrjCode();
	afx_msg void OnChkTaType();
	afx_msg void OnChkTaAlias();
	afx_msg void OnChkStamyNo();
	afx_msg void OnChkOperator();
	afx_msg void OnChkAuditor();
	afx_msg void OnChkCritic();
	afx_msg void OnChkPartNo();
	afx_msg void OnChkMaterial();
	afx_msg void OnChkThick();
	afx_msg void OnChkBriefMat();
};
