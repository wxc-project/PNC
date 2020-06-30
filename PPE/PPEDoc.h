// PPEDoc.h : interface of the CPPEDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PPEDOC_H__5777E2E8_9A48_4730_85F5_74FF670B4D25__INCLUDED_)
#define AFX_PPEDOC_H__5777E2E8_9A48_4730_85F5_74FF670B4D25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HashTable.h"
#include "NcPart.h"

class CPPEDoc : public CDocument
{
protected: // create from serialization only
	CPPEDoc();
	DECLARE_DYNCREATE(CPPEDoc)

// Attributes
public:
// Operations
public:
	void OpenFolder(const char* sFolderPath);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPPEDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPPEDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CView* GetView(const CRuntimeClass *pClass);
	BOOL WriteToParentProcess();
	//
	void InitPlateGroupByThickMat(CHashStrList<PLATE_GROUP> &hashPlateByThickMat);
	bool CreatePlateNcFiles(CHashStrList<PLATE_GROUP> &hashPlateByThickMat, char* thickSetStr,
		const char* mainFolder, int iNcMode, int iNcFileType, BOOL bIsPmzCheck = FALSE);
	void CreatePlateDxfFiles();
	void CreatePlateNcFiles(int iFileType);
	void CreatePlateFiles(int iFileType);
protected:
	CXhChar16 GetSubFolder(int iNcMode);
	CXhChar16 GetFileSuffix(int iFileType);
// Generated message map functions
protected:
	//{{AFX_MSG(CPPEDoc)
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnNewFile();
	afx_msg void OnFileSaveDxf();
	afx_msg void OnFileSaveTtp();
	afx_msg void OnFileSaveWkf();
	afx_msg void OnFileSavePbj();
	afx_msg void OnUpdateFileSavePbj(CCmdUI *pCmdUI);
	afx_msg void OnFileSavePmz();
	afx_msg void OnUpdateFileSavePmz(CCmdUI *pCmdUI);
	afx_msg void OnFileSaveTxt();
	afx_msg void OnUpdateFileSaveTxt(CCmdUI *pCmdUI);
	afx_msg void OnFileSaveNc();
	afx_msg void OnUpdateFileSaveNc(CCmdUI *pCmdUI);
	afx_msg void OnFileSaveCnc();
	afx_msg void OnUpdateFileSaveCnc(CCmdUI *pCmdUI);
	afx_msg void OnGenAngleNcFile();
	afx_msg void OnUpdateGenAngleNcFile(CCmdUI *pCmdUI);
	afx_msg void OnCreateFlameNcData();
	afx_msg void OnCreatePlasmaNcData();
	afx_msg void OnCreatePunchNcData();
	afx_msg void OnCreateDrillNcData();
	afx_msg void OnCreateLaserNcData();
	afx_msg void OnCreatePlateNcData();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPEDOC_H__5777E2E8_9A48_4730_85F5_74FF670B4D25__INCLUDED_)
