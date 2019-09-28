// PPEView.h : interface of the CPPEView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PPEVIEW_H__57057E67_7CEF_481E_825D_11AE7B63BEF7__INCLUDED_)
#define AFX_PPEVIEW_H__57057E67_7CEF_481E_825D_11AE7B63BEF7__INCLUDED_

#include "f_ent.h"	// Added by ClassView
#include "PPEDoc.h"
#include "I_DrawSolid.h"
#include "IPEC.h"
#include "ProcessPart.h"
#include "PropertyList.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//管理捕捉图元对象
class CSelectEntity
{
	CXhPtrSet<IDbEntity> m_xSelectEnts;
public:
	CSelectEntity(){;}
	~CSelectEntity(){Empty();}
	int GetSelectEntNum(){return m_xSelectEnts.GetNodeNum();}
	void Append(IDbEntity* pEntity);
	void Empty(){m_xSelectEnts.Empty();}
	IDbEntity* GetFirst(){return m_xSelectEnts.GetFirst();}
	IDbEntity* GetNext(){return m_xSelectEnts.GetNext();}
	//
	void UpdateSelectEnt(HIBERARCHY entHibId);
	BOOL IsSelectMK(CProcessPart* pProcessPart);
	BOOL IsSelectBolt(CProcessPart* pProcessPart);
	BOOL IsSelectVertex(CProcessPart* pProcessPart);
	BOOL IsSelectEdge(CProcessPart* pProcessPart);
	BOOL IsSelectCutPt(CProcessPart* pProcessPart);
	void GetSelectBolts(CXhPtrSet<BOLT_INFO>& selectBolts,CProcessPart* pProcessPart);
	void GetSelectVertexs(CXhPtrSet<PROFILE_VER>& selectVertexs,CProcessPart* pProcessPart);
	void GetSelectEdges(CXhPtrSet<PROFILE_VER>& selectVertexs,CProcessPart* pProcessPart);
	void GetSelectCutPts(CXhPtrSet<CUT_POINT>& selectCutPts,CProcessPart* pProcessPart);
};
class CPPEView : public CView
{
	BYTE m_cCurTask;
	IDrawSolid* m_pDrawSolid;
	CPoint m_ptLBDownPos;
	BOOL m_bStartOpenWndSelTest;
	BOOL m_bEnableOpenWndSelectTest;
	CProcessPart* m_pProcessPart;
	BOOL ReceiveFromParentProcess();
public:
	static const int TASK_OTHER = 0;
	static const int TASK_OPEN_WND_SEL = 1;
	static const int TASK_DIM_SIZE	= 2;	//标注尺寸
	static const int TASK_SPEC_WCS_ORIGIN = 3;
	static const int TASK_SPEC_WCS_AXIS_X = 4;
	static const int TASK_VIEW_PART_FEATURE=5;
	static const int TASK_DEF_PLATE_CUT_IN_PT=6;	//定义钢板切入点
protected: // create from serialization only
	CPPEView();
	DECLARE_DYNCREATE(CPPEView)

// Attributes
public:
	CPPEDoc* GetDocument();
	void Refresh(BOOL bZoomAll=TRUE);
	void UpdateCurWorkPartByPartNo(const char *part_no);
	CProcessPart* GetCurProcessPart(){return m_pProcessPart;}
	void SetCurProcessPart(CProcessPart* pPart) { m_pProcessPart = pPart; }
	bool SyncPartInfo(bool bToPEC,bool bReDraw=true);
	void UpdateSelectEnt();
	void SetCurTast(int iCurTask){m_cCurTask=iCurTask;}
	bool SavePartInfoToFile();
	void AmendHoleIncrement();
// Operations
public:
	static int m_nScrWide,m_nScrHigh;
	CSelectEntity m_xSelectEntity;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPPEView)
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPPEView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void UpdatePropertyPage();
	BOOL DisplayPlateProperty();
	BOOL DisplayLineAngleProperty();
	BOOL DisplaySysSettingProperty();
	BOOL DisplayBoltHoleProperty();
	BOOL DisplayEdgeLineProperty();
	BOOL DisplayVertexProperty();
	BOOL DisplayCutPointProperty();
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPPEView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg LRESULT ObjectSnappedProcess(WPARAM ID, LPARAM ent_type);
	afx_msg LRESULT ObjectSelectProcess(WPARAM nSelect, LPARAM other);
	virtual void OnInitialUpdate();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDisplayBoltSort();
	afx_msg void OnUpdateDisplayBoltSort(CCmdUI *pCmdUI);
	afx_msg void OnRedrawAll();
	afx_msg void OnHidePart();
	afx_msg void OnOpenWndSel(); 
	afx_msg void OnDesignGuaXianHole();
	afx_msg void OnBendPlank();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDefPlateCutInPt();
	afx_msg void OnUpdateDefPlateCutInPt(CCmdUI *pCmdUI);
	afx_msg void OnPreviewCuttingTrack();
	afx_msg void OnUpdatePreviewCuttingTrack(CCmdUI *pCmdUI);
	afx_msg void OnAdjustHoleOrder();
	afx_msg void OnUpdateAdjustHoleOrder(CCmdUI *pCmdUI);
	afx_msg void OnSmartSortBolts1();
	afx_msg void OnUpdateSmartSortBolts1(CCmdUI *pCmdUI);
	afx_msg void OnSmartSortBolts2();
	afx_msg void OnUpdateSmartSortBolts2(CCmdUI *pCmdUI);
	afx_msg void OnSmartSortBolts3();
	afx_msg void OnUpdateSmartSortBolts3(CCmdUI *pCmdUI);
	afx_msg void OnBatchSortHole();
	afx_msg void OnUpdateBatchSortHole(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void OpenWndSel();
	void OperOther();
};

#ifndef _DEBUG  // debug version in PPEView.cpp
inline CPPEDoc* CPPEView::GetDocument()
   { return (CPPEDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPEVIEW_H__57057E67_7CEF_481E_825D_11AE7B63BEF7__INCLUDED_)
//
extern ISolidDraw *g_pSolidDraw;
extern ISolidSet *g_pSolidSet;
extern ISolidSnap *g_pSolidSnap;
extern ISolidOper *g_pSolidOper;
extern I2dDrawing *g_p2dDraw;
extern IPEC* g_pPartEditor;
