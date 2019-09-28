#pragma once


// CBendPlankStyleDlg 对话框

class CBendPlankStyleDlg : public CDialog
{
	DECLARE_DYNAMIC(CBendPlankStyleDlg)

public:
	CBendPlankStyleDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CBendPlankStyleDlg();

// 对话框数据
	enum { IDD = IDD_BEND_PLANK_STYLE_DLG };
	double	m_fBendAngle;
	double	m_fNormX;
	double	m_fNormY;
	double	m_fNormZ;
	int		m_iBendFaceStyle;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg void OnBnPasteNorm();
	virtual BOOL OnInitDialog();
	afx_msg void OnRdoBendFaceStyle();
	DECLARE_MESSAGE_MAP()
};
