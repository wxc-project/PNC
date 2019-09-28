#pragma once


// CKaiHeLineAngleDlg 对话框

class CKaiHeLineAngleDlg : public CDialog
{
	DECLARE_DYNAMIC(CKaiHeLineAngleDlg)

public:
	CKaiHeLineAngleDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CKaiHeLineAngleDlg();

// 对话框数据
	enum { IDD = IDD_KAIHE_JG_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bHasKaiHeWing;	//判断是否已有开合肢
	float m_fKaiHeAngle;	//开合角度
	int m_iDimPos;			//标注位置（距始端距离）
	int m_iLeftLen;			//左侧开合长度
	int m_iRightLen;		//右侧开合长度
	int m_iKaiHeWing;		//开合肢
	CString m_sKaiHeWing;
};
