#pragma once
#include ".\resource.h"
#include "supergridctrl.h"
#include "BomModel.h"
#include "ArrayList.h"
#include "BatchPrint.h"
// COptimalSortDlg 对话框

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
class COptimalSortDlg : public CDialog
{
	DECLARE_DYNAMIC(COptimalSortDlg)
	CHashList<int> m_widthHashTbl;
	CHashList<int> m_thickHashTbl;
	//ARRAY_LIST<CAngleProcessInfo*> m_xJgList;
	//ARRAY_LIST<CPlateProcessInfo*> m_xPlateList;
	ARRAY_LIST<BOMPART*> m_xDisplayPartList;
	CBomFile m_xPrintBomFile;
	CDwgFileInfo *m_pDwgFile;
	int m_nQ235Count, m_nQ345Count, m_nQ355Count, m_nQ390Count, m_nQ420Count, m_nQ460Count;
	int m_nJgCount, m_nPlateCount, m_nYGCount, m_nTubeCount, m_nJiaCount, m_nFlatCount, m_nGgsCount;
	int m_nCutAngle, m_nKaiHe, m_nPushFlat, m_nCutRoot, m_nCutBer, m_nBend, m_nCommonAngle,m_nOtherNotes;
	int m_iPrintType;
protected:
	void InitByProcessAngle(CAngleProcessInfo* pJgInfo, BOMPART *pPart);
	void InitByProcessPlate(CPlateProcessInfo* pPlateInfo, BOMPART *pPart);
	bool IsFillTheFilter(CAngleProcessInfo* pJgInfo, BOMPART *pPart);
	bool IsFillTheFilter(CPlateProcessInfo* pPlateInfo, BOMPART *pPart);
	void UpdateHelpStr();
	BOOL GetPrintParaFromReg(LPCTSTR lpszSection, LPCTSTR lpszEntry);
	void WritePrintParaToReg(LPCTSTR lpszSection, LPCTSTR lpszEntry);
public:
	COptimalSortDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~COptimalSortDlg();
	//
	void SetDwgFile(CDwgFileInfo *pDwgFile) { m_pDwgFile = pDwgFile; }
	void UpdatePartList();
	void RefeshListCtrl();
	void InitCtrlState();
	bool InitPrintBom(const char* sFileName);
	bool IsValidPrintBom();
// 对话框数据
	enum { IDD = IDD_OPTIMAL_SORT_DLG };
	CSuperGridCtrl m_xListCtrl;
	BOOL m_bSelJg;
	BOOL m_bSelPlate;
	BOOL m_bSelYg;
	BOOL m_bSelTube;
	BOOL m_bSelFlat;
	BOOL m_bSelJig;
	BOOL m_bSelGgs;
	BOOL m_bSelQ235;
	BOOL m_bSelQ345;
	BOOL m_bSelQ355;
	BOOL m_bSelQ390;
	BOOL m_bSelQ420;
	BOOL m_bSelQ460;
	BOOL m_bCutAngle;
	BOOL m_bKaiHe;
	BOOL m_bPushFlat;
	BOOL m_bCutRoot;
	BOOL m_bCutBer;
	BOOL m_bBend;
	BOOL m_bOtherNotes;
	BOOL m_bCommonAngle;
	CString m_sWidth;
	CString m_sThick;
	int m_nRecord;
	//
	ATOM_LIST<PRINT_SCOPE> m_xPrintScopyList;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateJgInfo();
	afx_msg void OnEnChangeEWidth();
	afx_msg void OnEnChangeEThick();
	afx_msg void OnBnClickedBtnJgCard();
	afx_msg void OnBnClickedOk();
public:
	// 打印分组
	int m_iCmbPrintGroup;
	int m_iCmbPrintMode;
	int m_iCmbPrintType;
	afx_msg void OnBnClickedBtnPrintSet();
	virtual BOOL DestroyWindow();
};
#endif
