// PPEView.cpp : implementation of the CPPEView class
//

#include "stdafx.h"
#include "PPE.h"
#include "PPEDoc.h"
#include "PPEView.h"
#include "ProcessPart.h"
#include "MainFrm.h"
#include "UserDefMsg.h"
#include "PartLib.h"
#include "SysPara.h"
#include "InputAnValDlg.h"
#include "PPEModel.h"
#include "PromptDlg.h"
#include "NcPart.h"
#include "AdjustOrderDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
ISolidDraw *g_pSolidDraw;
ISolidSet *g_pSolidSet;
ISolidSnap *g_pSolidSnap;
ISolidOper *g_pSolidOper;
I2dDrawing *g_p2dDraw;
IPEC* g_pPartEditor;
//////////////////////////////////////////////////////////////////////////
//CSelectEntity
void CSelectEntity::Append(IDbEntity* pEntity)
{
	if(pEntity==NULL)
		return;
	m_xSelectEnts.append(pEntity);
}
BOOL CSelectEntity::IsSelectMK(CProcessPart* pProcessPart)
{
	if(pProcessPart==NULL)
		return FALSE;
	if(m_xSelectEnts.GetNodeNum()!=1)
		return FALSE;
	IDbEntity* pEnt=m_xSelectEnts.GetFirst();
	if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
		return FALSE;
	if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
	{
		BOLT_INFO *pBolt=pProcessPart->FromHoleHiberId(pEnt->GetHiberId());
		if(pBolt)
			return FALSE;
	}
	else if(pEnt->GetDbEntType()==IDbEntity::DbRect)
		return TRUE;
	return TRUE;
}
void CSelectEntity::UpdateSelectEnt(HIBERARCHY entHibId)
{
	m_xSelectEnts.Empty();
	IDrawing *pDrawing=g_p2dDraw->GetActiveDrawing();
	for(IDbEntity *pEnt=pDrawing->EnumFirstDbEntity();pEnt;pEnt=pDrawing->EnumNextDbEntity())
	{
		if(entHibId.IsEqual(pEnt->GetHiberId()))
			m_xSelectEnts.append(pEnt);
	}
}
BOOL CSelectEntity::IsSelectBolt(CProcessPart* pProcessPart)
{
	if(pProcessPart==NULL)
		return FALSE;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		//if(pEnt->GetDbEntType()!=IDbEntity::DbCircle)
		//	return FALSE;
		BOLT_INFO *pBolt=pProcessPart->FromHoleHiberId(pEnt->GetHiberId());
		if(pBolt==NULL)
			return FALSE;
	}
	return TRUE;
}
BOOL CSelectEntity::IsSelectVertex(CProcessPart* pProcessPart)
{
	if(pProcessPart==NULL || pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return FALSE;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		if(pEnt->GetDbEntType()!=IDbEntity::DbPoint)
			return FALSE;
		PROFILE_VER* pVer=pProcessPart->FromVertexHiberId(pEnt->GetHiberId());
		if(pVer==NULL)
			return FALSE;
	}
	return TRUE;
}
BOOL CSelectEntity::IsSelectEdge(CProcessPart* pProcessPart)
{
	if(pProcessPart==NULL || pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return FALSE;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		if(pEnt->GetDbEntType()!=IDbEntity::DbLine && pEnt->GetDbEntType()!=IDbEntity::DbArcline)
			return FALSE;
		PROFILE_VER* pVer=pProcessPart->FromVertexHiberId(pEnt->GetHiberId());
		if(pVer==NULL)
			return FALSE;
	}
	return TRUE;
}
BOOL CSelectEntity::IsSelectCutPt(CProcessPart* pProcessPart)
{
	if(pProcessPart==NULL||pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return FALSE;
	if(m_xSelectEnts.GetNodeNum()!=1)
		return FALSE;
	IDbEntity *pEnt=m_xSelectEnts.GetFirst();
	if(pEnt->GetDbEntType()!=IDbEntity::DbCircle)
		return FALSE;
	return ((CProcessPlate*)pProcessPart)->FromCutPtHiberId(pEnt->GetHiberId())!=NULL;
}
void CSelectEntity::GetSelectCutPts(CXhPtrSet<CUT_POINT>& selectCutPts,CProcessPart* pProcessPart)
{
	selectCutPts.Empty();
	if(pProcessPart==NULL||pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	if(m_xSelectEnts.GetNodeNum()!=1)
		return;
	IDbEntity *pEnt=m_xSelectEnts.GetFirst();
	if(pEnt->GetDbEntType()!=IDbEntity::DbCircle)
		return;
	CUT_POINT *pCutPt=((CProcessPlate*)pProcessPart)->FromCutPtHiberId(pEnt->GetHiberId());
	if(pCutPt)
		selectCutPts.append(pCutPt);
}
void CSelectEntity::GetSelectBolts(CXhPtrSet<BOLT_INFO>& selectBolts,CProcessPart* pProcessPart)
{
	if(!IsSelectBolt(pProcessPart) || pProcessPart==NULL)
		return;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		BOLT_INFO *pBolt=pProcessPart->FromHoleHiberId(pEnt->GetHiberId());
		selectBolts.append(pBolt);
	}
}
void CSelectEntity::GetSelectVertexs(CXhPtrSet<PROFILE_VER>& selectVertexs,CProcessPart* pProcessPart)
{
	if(!IsSelectVertex(pProcessPart))
		return;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		PROFILE_VER* pVertex=pProcessPart->FromVertexHiberId(pEnt->GetHiberId());
		selectVertexs.append(pVertex);
	}
}
void CSelectEntity::GetSelectEdges(CXhPtrSet<PROFILE_VER>& selectVertexs,CProcessPart* pProcessPart)
{
	if(!IsSelectEdge(pProcessPart))
		return;
	for(IDbEntity* pEnt=m_xSelectEnts.GetFirst();pEnt;pEnt=m_xSelectEnts.GetNext())
	{
		if(pEnt->GetHiberId().masterId!=pProcessPart->GetKey())
			continue;	//不是当前构件对应的实体
		PROFILE_VER* pVertex=pProcessPart->FromVertexHiberId(pEnt->GetHiberId());
		selectVertexs.append(pVertex);
	}
}
/////////////////////////////////////////////////////////////////////////////
// CPPEView
UCS_STRU ucs;
int CPPEView::m_nScrWide = 0;
int CPPEView::m_nScrHigh = 0;
static bool FirePartModify(WORD cmdType)
{
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
		pView->SyncPartInfo(false);
	return TRUE;
}
IMPLEMENT_DYNCREATE(CPPEView, CView)

BEGIN_MESSAGE_MAP(CPPEView, CView)
	//{{AFX_MSG_MAP(CPPEView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CPPEView::OnFilePrintPreview)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_OBJECT_SNAPED, ObjectSnappedProcess)
	ON_MESSAGE(WM_OBJECT_SELECTED, ObjectSelectProcess)
	ON_COMMAND(ID_OPEN_WND_SEL, OnOpenWndSel)
	ON_COMMAND(ID_HIDE_PART, OnHidePart)
	ON_COMMAND(ID_REDRAW_ALL, OnRedrawAll)
	ON_COMMAND(ID_DESIGN_GUAXIAN_HOLE,OnDesignGuaXianHole)
	ON_COMMAND(ID_BEND_PLANK,OnBendPlank)
	ON_COMMAND(ID_DISPLAY_BOLT_SORT,OnDisplayBoltSort)
	ON_UPDATE_COMMAND_UI(ID_DISPLAY_BOLT_SORT,OnUpdateDisplayBoltSort)
	ON_COMMAND(ID_DEF_CUT_IN_PT,OnDefPlateCutInPt)
	ON_UPDATE_COMMAND_UI(ID_DEF_CUT_IN_PT,OnUpdateDefPlateCutInPt)
	ON_COMMAND(ID_PREVIEW_CUTTING_TRACK,OnPreviewCuttingTrack)
	ON_UPDATE_COMMAND_UI(ID_PREVIEW_CUTTING_TRACK,OnUpdatePreviewCuttingTrack)
	ON_COMMAND(ID_SMART_SORT_BOLT,OnSmartSortBolts1)
	ON_UPDATE_COMMAND_UI(ID_SMART_SORT_BOLT,OnUpdateSmartSortBolts1)
	ON_COMMAND(ID_SMART_SORT_BOLT_2, OnSmartSortBolts2)
	ON_UPDATE_COMMAND_UI(ID_SMART_SORT_BOLT_2, OnUpdateSmartSortBolts2)
	ON_COMMAND(ID_SMART_SORT_BOLT_3, OnSmartSortBolts3)
	ON_UPDATE_COMMAND_UI(ID_SMART_SORT_BOLT_3, OnUpdateSmartSortBolts3)
	ON_COMMAND(ID_ADJUST_HOLE_ORDER,OnAdjustHoleOrder)
	ON_UPDATE_COMMAND_UI(ID_ADJUST_HOLE_ORDER,OnUpdateAdjustHoleOrder)
	ON_COMMAND(ID_BATCH_SORT_HOLE,OnBatchSortHole)
	ON_UPDATE_COMMAND_UI(ID_BATCH_SORT_HOLE,OnUpdateBatchSortHole)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPEView construction/destruction

CPPEView::CPPEView()
{
	m_nScrWide = 0;
	m_nScrHigh = 0;
	m_bStartOpenWndSelTest=FALSE;
	m_bDisplayLsOrder = FALSE;
	g_pSolidDraw=NULL;
	g_pSolidSet=NULL;
	g_pSolidSnap=NULL;
	g_pSolidOper=NULL;
	g_p2dDraw=NULL;
	g_pPartEditor=NULL;
	m_pProcessPart=NULL;
	g_pPromptMsg = new CPromptDlg(this);
}

CPPEView::~CPPEView()
{
	if(m_pDrawSolid)
	{
		CDrawSolidFactory::Destroy(m_pDrawSolid->GetSerial());
		g_pSolidDraw=NULL;
		g_pSolidOper=NULL;
		g_pSolidSet=NULL;
		g_pSolidSnap=NULL;
		m_pDrawSolid=NULL;
		g_p2dDraw=NULL;
	}
	if(g_pPartEditor)
	{
		CPartsEditorFactory::Destroy(g_pPartEditor->GetSerial());
		g_pPartEditor=NULL;
	}
	m_pProcessPart=NULL;
	if(g_pPromptMsg)
		delete g_pPromptMsg;
}

BOOL CPPEView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPPEView drawing
void GetSysPath(char* startPath);
int CPPEView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	//
	m_pDrawSolid=CDrawSolidFactory::CreateDrawEngine();
	g_pSolidDraw=m_pDrawSolid->SolidDraw();
	g_pSolidSet=m_pDrawSolid->SolidSet();
	g_pSolidSnap=m_pDrawSolid->SolidSnap();
	g_pSolidOper=m_pDrawSolid->SolidOper();
	g_pSolidSet->Init(m_hWnd);
	char sFontFile[MAX_PATH],sBigFontFile[MAX_PATH],sAppPath[MAX_PATH];
	GetSysPath(sAppPath);
	sprintf(sFontFile,"%s\\sys\\simplex.shx",sAppPath);
	bool bRetCode=g_pSolidSet->SetShxFontFile(sFontFile);
	sprintf(sBigFontFile,"%s\\sys\\GBHZFS.shx",sAppPath);
	bRetCode=g_pSolidSet->SetBigFontFile(sBigFontFile);
	char sPartLibPath[MAX_PATH];
	if(theApp.starter.m_cProductType==PRODUCT_TMA)
		sprintf(sPartLibPath,"%s\\TMA.plb",sAppPath);
	else if(theApp.starter.m_cProductType==PRODUCT_LMA)
		sprintf(sPartLibPath,"%s\\LMA.plb",sAppPath);
	//else if(theApp.starter.m_cProductType==PRODUCT_TDA)
	//	sprintf(sPartLibPath,"%s\\TDA.plb",sAppPath);
	else if(theApp.starter.m_cProductType==PRODUCT_LDS)
		sprintf(sPartLibPath,"%s\\LDS.plb",sAppPath);
	else if(theApp.starter.m_cProductType==PRODUCT_TAP)
		sprintf(sPartLibPath,"%s\\TAP.plb",sAppPath);
	else
		sprintf(sPartLibPath,"%s\\ppe.plb",sAppPath);
	InitPartLibrary(sPartLibPath);
	//
	g_p2dDraw=m_pDrawSolid->Drawing2d();
	IDrawingAssembly* pDrawingAssembly=g_p2dDraw->InitDrawingAssembly();
 	IDrawing *pEditorDrawing=pDrawingAssembly->AppendDrawing(1,"构件编辑器");
	pEditorDrawing->Database()->AddTextStyle("standard",sFontFile,sBigFontFile);
	IDrawing *pNcDrawing=pDrawingAssembly->AppendDrawing(1,"NC");
	pNcDrawing->Database()->AddTextStyle("standard",sFontFile,sBigFontFile);
	IDrawing *pCardDrawing=pDrawingAssembly->AppendDrawing(1,"工艺卡");
	pCardDrawing->Database()->AddTextStyle("standard",sFontFile,sBigFontFile);
	//构件编辑器
	g_pPartEditor=CPartsEditorFactory::CreatePartsEditorInstance();
	g_pPartEditor->InitDrawEnviornment(m_hWnd,g_p2dDraw,g_pSolidDraw,g_pSolidSet,g_pSolidSnap,g_pSolidOper);
	g_pPartEditor->SetDrawMode(IPEC::DRAW_MODE_NC);
	g_pPartEditor->SetCallBackPartModifiedFunc(FirePartModify);
	g_pPartEditor->SetProcessCardPath(g_sysPara.jgDrawing.sAngleCardPath);
	CBuffer buffer;
	g_sysPara.AngleDrawingParaToBuffer(buffer);
	g_pPartEditor->UpdateJgDrawingPara(buffer.GetBufferPtr(),buffer.GetLength());
	if (theApp.starter.m_bChildProcess && theApp.starter.mode > 0)
	{
		ReceiveFromParentProcess();
		//打开文件夹之后，根据配置初始化孔径增大值 wht 19-04-09
		AmendHoleIncrement();
	}
	return 0;
}

void CPPEView::OnDraw(CDC* pDC)
{
	CPPEDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	g_pSolidDraw->Draw();

}

/////////////////////////////////////////////////////////////////////////////
// CPPEView printing

void CPPEView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

void CPPEView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{	//Step 3.
	if(pInfo!=NULL&&pDC->IsPrinting())
		pDC->SetMapMode(MM_LOMETRIC);
	else
		pDC->SetMapMode(MM_TEXT);
	CView::OnPrepareDC(pDC,pInfo);
}
BOOL CPPEView::OnPreparePrinting(CPrintInfo* pInfo)
{	//Step 1.
	// default preparation
	pInfo->SetMaxPage(1);

	return DoPreparePrinting(pInfo);
}

void CPPEView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{	//Step 2.
	// TODO: add extra initialization before printing
}
void ParseDimension(IDbDimension* pDimension,CXhSimpleList<GELINE> *lpLineList);
void CPPEView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{	//Step 4.
	PRINTPAGE page;
	int iMapMode=pDC->GetMapMode();
	if(iMapMode==MM_LOMETRIC)
		page.scaleDevicetoModel=0;//10.0;
	else if(iMapMode==MM_HIMETRIC)
		page.scaleDevicetoModel=0;//100;
	LPDEVMODE lpDevMode=(LPDEVMODE)GlobalLock(pInfo->m_pPD->m_pd.hDevMode); 
	page.sizePaper.cx=2100;
	page.sizePaper.cy=2970;
	page.margin.ciLeft=15;
	page.margin.ciTop=page.margin.ciRight=page.margin.ciBottom=15;	//1厘米
	if(lpDevMode)
	{
		page.ciLayout=(lpDevMode->dmOrientation==DMORIENT_LANDSCAPE)?2:1;	//DMORIENT_PORTRAIT(纵向)\DMORIENT_LANDSCAPE(横向)
		if(page.ciLayout==2)
		{	//指定横向打印时，掉换边距值
			BYTE margin=page.margin.ciTop;
			page.margin.ciTop=page.margin.ciRight;
			page.margin.ciRight=page.margin.ciBottom;
			page.margin.ciBottom=page.margin.ciLeft;
			page.margin.ciLeft=margin;
		}
		//if(lpDevMode->dmPaperSize==DMPAPER_A4) //将打印纸设置为A4 
		page.sizePaper.cx=pDC->GetDeviceCaps(PHYSICALWIDTH);
		page.sizePaper.cy=pDC->GetDeviceCaps(PHYSICALHEIGHT);
		page.iValidOffsetX=pDC->GetDeviceCaps(PHYSICALOFFSETX);
		page.iValidOffsetY=pDC->GetDeviceCaps(PHYSICALOFFSETY);
		CPoint org(page.iValidOffsetX,page.iValidOffsetY);
		pDC->DPtoLP(&page.sizePaper);
		pDC->DPtoLP(&org);
		page.iValidOffsetX=org.x;
		page.iValidOffsetY=org.y;
		page.printStartPoint.x=  page.margin.ciLeft*10-page.iValidOffsetX;
		page.printStartPoint.y=-(page.margin.ciTop*10-abs(page.iValidOffsetY));
	}
	page.wWidth=(WORD)pInfo->m_rectDraw.Width();
	page.wHeight=(WORD)abs(pInfo->m_rectDraw.Height());
	page.wActualWidth=(WORD)(page.sizePaper.cx-page.margin.ciLeft*10-page.margin.ciRight*10);
	page.wActualHeight=(WORD)(page.sizePaper.cy-page.margin.ciTop*10-page.margin.ciBottom*10);
	//设定打印线宽与屏幕的映射比例
	page.ciPenWidthPerPixel=2;
	//int printer_ppi=pDC->GetDeviceCaps(LOGPIXELSX);
	//int screen_ppi =::GetDeviceCaps(CClientDC(this),LOGPIXELSX);

	g_pSolidDraw->Print(pDC,page);
	/*测试代码
	CPen pen(PS_SOLID,4,RGB(0,0,0));
	CPen* pOldPen=pDC->SelectObject(&pen);
	pDC->MoveTo(0,0);
	pDC->LineTo(1000,-1000);
	pDC->MoveTo(0,0);
	pDC->LineTo(page.wWidth,0);
	CPen pen2(PS_SOLID,2,RGB(0,0,0));
	pDC->SelectObject(&pen2);
	pDC->MoveTo(0,-50);
	pDC->LineTo(1000,-1050);
	pDC->MoveTo(0,-50);
	pDC->LineTo(page.wWidth,-50);
	pDC->SelectObject(pOldPen);
	*/
}

void CPPEView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CPPEView diagnostics

#ifdef _DEBUG
void CPPEView::AssertValid() const
{
	CView::AssertValid();
}

void CPPEView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPPEDoc* CPPEView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPPEDoc)));
	return (CPPEDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPPEView message handlers
void CPPEView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	m_nScrWide = cx;
	m_nScrHigh = cy;
	if(g_pSolidOper)
		g_pSolidOper->ReSize();
	Refresh();
}

void CPPEView::Refresh(BOOL bZoomAll/*=TRUE*/)
{
	Invalidate();
	m_xSelectEntity.Empty();
	g_pSolidDraw->BuildDisplayList(this);
	if(bZoomAll)
		g_pSolidOper->ZoomAll(0.9);
}

void DrawProcessPart(void* pContext)
{
	if(g_pPartEditor==NULL)
		return;
	g_pPartEditor->Draw();
	g_p2dDraw->RenderDrawing();
}

BOOL CPPEView::ReceiveFromParentProcess()
{
	theApp.m_xPPEModelBuffer.SeekToBegin();
	model.Empty();
	if(theApp.m_xPPEModelBuffer.GetLength()<=0)
	{
		AfxMessageBox("读取构件工艺信息失败");
		return FALSE;
	}
	CBuffer buffer;
	if(theApp.starter.IsOfferFilePathMode())
	{
		CXhChar500 folder_path;
		theApp.m_xPPEModelBuffer.ReadString(folder_path);
		model.InitModelByFolderPath(folder_path);
		CXhChar500 cfg_path(folder_path);
		cfg_path.Append("\\config.ini");
		model.ReadPrjTowerInfoFromCfgFile(cfg_path);
	}
	else
	{
		m_pProcessPart=NULL;
		model.FromBuffer(theApp.m_xPPEModelBuffer);
		m_pProcessPart=model.EnumPartFirst();
		if(m_pProcessPart)
		{
			m_pProcessPart->ToPPIBuffer(buffer);
			buffer.SeekToBegin();
			g_pPartEditor->AddProcessPart(buffer,m_pProcessPart->GetKey());
		}
	}
	return TRUE;
}

void CPPEView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	UCS_STRU ucs;								//设置坐标系
	LoadDefaultUCS(&ucs);
	g_pSolidSet->SetObjectUcs(ucs);
	g_pSolidSet->SetBkColor(RGB(255,255,255));	//设置背景颜色
	PEN_STRU pen;								//设置PEN
	pen.crColor = RGB(255,255,0);
	pen.width = 2;
	pen.style = 0;
	g_pSolidSet->SetPen(pen);
	g_pSolidSet->SetDisplayType(DISP_DRAWING);		//设置显示类型
	g_pSolidSet->SetDisplayFunc(DrawProcessPart);
	g_pSolidSet->SetSolidAndLineStatus(FALSE);
	g_pSolidSet->SetArcSamplingLength(5);
	g_pSolidSet->SetSmoothness(36);
	g_pSolidDraw->InitialUpdate();
	g_pSolidSet->SetDisplayLineVecMark(FALSE);
	g_pSolidSet->SetDispObjectUcs(FALSE);
	g_pSolidSet->SetRotOrg(f3dPoint(0,0,0));
	g_pSolidSet->SetZoomStyle(ROT_CENTER);
	//
	CProcessAngle::InitPropHashtable();
	CProcessPlate::InitPropHashtable();
	CSysPara::InitPropHashtable();
	PROFILE_VER::InitPropHashtable();
	BOLT_INFO::InitPropHashtable();
	CUT_POINT::InitPropHashtable();
	DisplaySysSettingProperty();
	if(theApp.starter.IsMultiPartsMode())	
	{	//多构件工作模式，刷新构件树列表(ReceiveFromParentProcess时构件树还未创建)
		CPartTreeDlg *pPartTreeDlg=((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
		if(pPartTreeDlg)
		{
			if(m_pProcessPart)
				pPartTreeDlg->InitTreeView(m_pProcessPart->GetPartNo());
			else
				pPartTreeDlg->InitTreeView();
		}
	}
	Refresh();
	UpdatePropertyPage();
}

void CPPEView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CView::OnLButtonDblClk(nFlags, point);
	g_pSolidOper->LMouseDoubleClick(point);

	if( m_pProcessPart&&m_pProcessPart->IsPlate()&&
		g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_NC)
	{
		f3dPoint user_pt,port_pt(point.x,point.y);
		g_pSolidOper->ScreenToUser(&user_pt,port_pt);
		UCS_STRU object_ucs;
		g_pSolidSet->GetObjectUcs(object_ucs);
		coord_trans(user_pt,object_ucs,FALSE);
		GECS cs;
		g_pPartEditor->GetPlateNcUcs(cs);
		coord_trans(user_pt,cs,TRUE);
		g_pPartEditor->SetMarkPos(user_pt.x,user_pt.y);
		g_pPartEditor->InitMkRect();
		g_pSolidDraw->BuildDisplayList(this);
		g_p2dDraw->RenderDrawing();
		SyncPartInfo(false,false);
	}
}

void CPPEView::OnLButtonDown(UINT nFlags, CPoint point)
{
	BOOL bAltKeyDown=GetKeyState(VK_MENU)&0x8000;
	if(nFlags&MK_SHIFT)
	{
		m_xSelectEntity.Empty();
		g_pSolidDraw->ReleaseSnapStatus();
	}
	SetCapture();	//锁定光标在当前窗口
	if(g_pSolidSet->GetOperType()==OPER_OTHER&&nFlags==MK_LBUTTON&&!bAltKeyDown)	//空闲，且未按住Ctrl或Shift键
	{
		m_ptLBDownPos=point;
		m_bStartOpenWndSelTest = TRUE;	//只有空闲时才启动开窗选择检测
	}
	g_pSolidOper->LMouseDown(point);
	CView::OnLButtonDown(nFlags, point);
}

void CPPEView::OnLButtonUp(UINT nFlags, CPoint point)
{
	g_pSolidOper->LMouseUp(point);
	if(GetCapture()==this)
		ReleaseCapture();
	CView::OnLButtonUp(nFlags, point);
}

void CPPEView::OnMouseMove(UINT nFlags, CPoint point)
{
	CSize offset=point-m_ptLBDownPos;
	if(m_bStartOpenWndSelTest&&nFlags&MK_LBUTTON&&(abs(offset.cx)>2||abs(offset.cy)>2))
	{
		OpenWndSel();
		g_pSolidOper->LMouseDown(m_ptLBDownPos);
		m_bStartOpenWndSelTest=FALSE;
	}
	g_pSolidOper->MouseMove(point,nFlags);
	f3dPoint user_pt,port_pt(point.x,point.y);
	CString sMousePos;
	if(!g_pSolidOper->IsHasHighlightPoint(&user_pt))
		g_pSolidOper->ScreenToUser(&user_pt,port_pt);
	UCS_STRU object_ucs;
	g_pSolidSet->GetObjectUcs(object_ucs);
	coord_trans(user_pt,object_ucs,FALSE);
	sMousePos.Format("X=%.2f,Y=%.2f,Z=%.2f",user_pt.x,user_pt.y,user_pt.z);
	CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	pMainWnd->SetPaneText(1,sMousePos);
	CView::OnMouseMove(nFlags, point);
}

BOOL CPPEView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	ZOOM_CENTER_STYLE zoom_style;
	g_pSolidSet->GetZoomStyle(&zoom_style);
	g_pSolidSet->SetZoomStyle(MOUSE_CENTER);
	if(zDelta>0)
		g_pSolidOper->Scale(1.4);
	else
		g_pSolidOper->Scale(1/1.4);
	g_pSolidOper->RefreshScope();
	g_pSolidSet->SetZoomStyle(zoom_style);
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CPPEView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	g_pSolidOper->KeyDown(nChar);
	if(nChar==VK_ESCAPE)
	{
		OperOther();
		if (m_xSelectEntity.GetSelectEntNum() > 0)
		{	//取消选中构件的图元，重新刷新构件
			m_xSelectEntity.Empty();
			if (m_pProcessPart)
				g_p2dDraw->RenderDrawing();
		}
		else if (m_pProcessPart)
		{	//取消选中构件
			CPartTreeDlg *pPartTreeDlg = ((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
			pPartTreeDlg->CancelSelTreeItem();
		}
		else
		{
			CPartTreeDlg *pPartTreeDlg=((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
			pPartTreeDlg->GetTreeCtrl()->SelectItem(NULL);
			Refresh();
		}
		if(g_pPromptMsg)
			g_pPromptMsg->Destroy();
		UpdatePropertyPage();
	}
	else if(nChar==VK_DELETE)		//删除操作
	{
		if(m_xSelectEntity.IsSelectBolt(m_pProcessPart))		//删除螺栓孔
		{
			if(AfxMessageBox("确定要删除选中螺栓吗？",MB_YESNO)==IDNO)
				return;
			for(IDbEntity* pEnt=m_xSelectEntity.GetFirst();pEnt;pEnt=m_xSelectEntity.GetNext())
			{
				if(pEnt->GetHiberId().masterId==m_pProcessPart->GetKey())
					m_pProcessPart->DeleteBoltHoleByHiberId(pEnt->GetHiberId());
			}
		}
		else if(m_xSelectEntity.IsSelectVertex(m_pProcessPart))	//删除顶点
		{
			for(IDbEntity* pEnt=m_xSelectEntity.GetFirst();pEnt;pEnt=m_xSelectEntity.GetNext())
			{
				if(pEnt->GetHiberId().masterId==m_pProcessPart->GetKey())
					m_pProcessPart->DeleteVertexByHiberId(pEnt->GetHiberId());
			}
		}
		else if(m_xSelectEntity.IsSelectMK(m_pProcessPart))		//删除号位孔
			m_pProcessPart->mkpos.Set(1000000,0,0);
		g_pSolidDraw->ReleaseSnapStatus();
		m_xSelectEntity.Empty();
		SyncPartInfo(true,false);
		//
		Refresh(FALSE);
		UpdatePropertyPage();
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPPEView::OperOther() 
{
	g_pSolidSnap->SetSnapType(SNAP_ALL);
	g_pSolidSet->SetOperType(OPER_OTHER);
	m_cCurTask = TASK_OTHER;
}

void CPPEView::OpenWndSel() 
{
	//只画橡皮筋不生成图元
	g_pSolidSet->SetCreateEnt(FALSE);
	g_pSolidSet->SetDrawType(DRAW_RECTANGLE);
	m_cCurTask = TASK_OPEN_WND_SEL;
}

void CPPEView::OnOpenWndSel() 
{
	OpenWndSel();
}

void CPPEView::OnHidePart() 
{
	/*long *id_arr=NULL;
	int i,n = g_pSolidSnap->GetLastSelectEnts(id_arr);
	for(i=0;i<n;i++)
	{
		CLDSDbObject* pObj = (CLDSDbObject*)console.FromHandle(id_arr[i],TRUE);
		if(pObj)
			pObj->is_visible = FALSE;
		BOOL bRet=TRUE;
		while(bRet) //删除所有ID为id_arr[i]的实体
			bRet=g_pSolidDraw->DelEnt(id_arr[i]);
	}*/
}

void CPPEView::OnRedrawAll()
{
	Refresh();
}

LRESULT CPPEView::ObjectSnappedProcess(WPARAM ID, LPARAM ent_type)
{
	if(ID==0&&ent_type==0)
		return 0;	//空点鼠标左键不做任何处理
	CLogErrorLife logErrLife;
	DISPLAY_TYPE dispType;
	g_pSolidSet->GetDisplayType(&dispType);
	if(dispType!=DISP_DRAWING)
		return 0;
	IDrawing *pDrawing=g_p2dDraw->GetActiveDrawing();
	IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(ID);
	if(pEnt==NULL)
	{
		logerr.Log("没有捕捉到有效对象!");
		return 0;
	}
	m_xSelectEntity.Append(pEnt);
	switch(m_cCurTask)
	{
	case TASK_DIM_SIZE:
		if(m_xSelectEntity.GetSelectEntNum()==1)
			g_pPromptMsg->SetMsg("请选择第二个对象！");
		if(m_xSelectEntity.GetSelectEntNum()==2)
		{
			if(m_xSelectEntity.IsSelectEdge(GetCurProcessPart()))
				return 0;
			g_pPartEditor->ExecuteCommand(IPEC::CMD_DIM_SIZE);
			m_xSelectEntity.Empty();
			g_pPromptMsg->Destroy();
			g_pSolidDraw->ReleaseSnapStatus();
			OperOther();
		}
		break;
	case TASK_SPEC_WCS_ORIGIN:
		if(m_xSelectEntity.GetSelectEntNum()==1)
		{
			g_pPartEditor->ExecuteCommand(IPEC::CMD_SPEC_WCS_ORIGIN);
			m_xSelectEntity.Empty();
			g_pSolidDraw->ReleaseSnapStatus();
			OperOther();
		}
		break; 
	case TASK_SPEC_WCS_AXIS_X:
		if(m_xSelectEntity.GetSelectEntNum()==1)
		{
			g_pPartEditor->ExecuteCommand(IPEC::CMD_SPEC_WCS_AXIS_X);
			m_xSelectEntity.Empty();
			g_pSolidDraw->ReleaseSnapStatus();
			OperOther();
		}
		break;
	case TASK_VIEW_PART_FEATURE:
		if(m_xSelectEntity.GetSelectEntNum()==1)
		{
			g_pPartEditor->ExecuteCommand(IPEC::CMD_VIEW_PART_FEATURE);
			m_xSelectEntity.Empty();
			g_pSolidDraw->ReleaseSnapStatus();
		}
		break;
	case TASK_DEF_PLATE_CUT_IN_PT:
		if(m_xSelectEntity.GetSelectEntNum()==1&&pEnt->GetDbEntType()==IDbEntity::DbPoint)
		{
			g_pPartEditor->ExecuteCommand(IPEC::CMD_DEF_PLATE_CUT_IN_PT);
			SyncPartInfo(false);
			m_xSelectEntity.Empty();
			g_pSolidDraw->ReleaseSnapStatus();
		}
		break;
	case TASK_OPEN_WND_SEL:
		UpdatePropertyPage();	//更新属性对话框
		OperOther();
		break;
	default:
		UpdatePropertyPage();	//更新属性对话框
		break;
	}
	return 0;
}
LRESULT CPPEView::ObjectSelectProcess(WPARAM nSelect, LPARAM other)
{
	IDrawing *pDrawing=g_p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return 0;
	long *id_arr = NULL;
	int n=g_pSolidSnap->GetLastSelectEnts(id_arr);
	for(int i=0;i<n;i++)
	{
		IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(id_arr[i]);
		if(pEnt==NULL)
			continue;
		m_xSelectEntity.Append(pEnt);
	}
	switch(m_cCurTask)
	{
	case TASK_OPEN_WND_SEL:
		UpdatePropertyPage();	//更新属性对话框
		OperOther();
		break;
	default:
		UpdatePropertyPage();	//更新属性对话框
		break;
	}
	return 0;
}
void CPPEView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if(m_xSelectEntity.GetSelectEntNum()<=0)
		return;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0,MF_BYPOSITION);
	if(m_pProcessPart->m_cPartType==CProcessPart::TYPE_PLATE)
	{
		if(m_xSelectEntity.IsSelectBolt(m_pProcessPart))
			pMenu->AppendMenu(MF_STRING,ID_DESIGN_GUAXIAN_HOLE,"挂线孔设计");
		else if(m_xSelectEntity.IsSelectVertex(m_pProcessPart) && m_xSelectEntity.GetSelectEntNum()==2)
			pMenu->AppendMenu(MF_STRING,ID_BEND_PLANK,"火曲钢板");
	}
	else //if(m_pProcessPart->m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{

	}
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this);
}
//挂线孔处外形设计
#include "ArrayList.h"
#include "SortFunc.h"
struct GUAXIANHOLE_BASE_INFO : public ICompareClass	//挂线孔信息
{
	f3dPoint ls_pos;	
	double fRadius;
	double lengthPosToEdge;		//挂线孔在轮廓边的投影位置
	virtual int Compare(const ICompareClass *pCompareObj) const
	{
		GUAXIANHOLE_BASE_INFO* pCompareNode=(GUAXIANHOLE_BASE_INFO*)pCompareObj;
		return int(lengthPosToEdge-pCompareNode->lengthPosToEdge);
	}
};
struct GUAXIANHOLE_DESIGN_INFO	//挂线孔设计信息
{
	BOOL bWaveShape;
	PROFILE_VER* pStartVertex,*pEndVertex;
	ARRAY_LIST<GUAXIANHOLE_BASE_INFO> guaXianHoleInfoArr;
	GUAXIANHOLE_DESIGN_INFO()
	{
		bWaveShape=TRUE;
		pStartVertex=NULL;
		pEndVertex=NULL;
	}
};
//计算距挂线孔距离最近的轮廓边，记录始终端顶点
static BOOL CalNearestEdgeToHole(CProcessPlate* pPlate,GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo);
//重新检测轮廓边的始终端顶点是否合理，选取合适的轮廓顶点
static void RefreshStartAndEndVertex(CProcessPlate* pPlate,GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo);
//去除不合理的挂线孔
static BOOL RemoveErrorHoles(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo);
//计算轮廓边始终端到始末挂线孔圆弧的公切线
static BOOL CalStartAndEndTangentPt(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo,PROFILE_VER& startTangPt,PROFILE_VER& endTangPt);
//计算多个挂线孔之间的公切点
static BOOL CalMultiHoleTangentPt(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo,ATOM_LIST<PROFILE_VER> &vertexList);
void CPPEView::OnDesignGuaXianHole()
{
	CLogErrorLife logErrLife;
	if(m_pProcessPart==NULL || m_pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate* pPlate=(CProcessPlate*)m_pProcessPart;
	int n=m_xSelectEntity.GetSelectEntNum();
	if(n<=0 || !m_xSelectEntity.IsSelectBolt(m_pProcessPart))
		return;
	//确定挂线孔对应的圆弧半径
	GUAXIANHOLE_DESIGN_INFO designHoleInfo;
	designHoleInfo.guaXianHoleInfoArr.SetSize(0,n);
	CInputAnIntegerValDlg dlg;
	dlg.m_sValTitle="挂孔处圆弧半径:";
	double R=40;
	for(IDbEntity* pEnt=m_xSelectEntity.GetFirst();pEnt;pEnt=m_xSelectEntity.GetNext())
	{
		BOLT_INFO* pBolt=m_pProcessPart->FromHoleHiberId(pEnt->GetHiberId());
		if(pBolt==NULL)
			continue;
		dlg.m_fVal=R;
		g_pSolidDraw->ReleaseSnapStatus();
		g_pSolidDraw->SetEntSnapStatus(pEnt->GetId(),true);
		if(dlg.DoModal()!=IDOK)
			continue;
		GUAXIANHOLE_BASE_INFO holeInfo;
		holeInfo.fRadius=dlg.m_fVal;
		holeInfo.ls_pos.Set(pBolt->posX,pBolt->posY,0);
		designHoleInfo.guaXianHoleInfoArr.append(holeInfo);
	}
	//计算距挂线孔距离最近的轮廓边，根据挂线孔在轮廓边的投影距始端的距离进行排序
	if(!CalNearestEdgeToHole(pPlate,&designHoleInfo))
		return;
	int nNum=designHoleInfo.guaXianHoleInfoArr.GetSize();
	//确定设计模式：外凸圆弧形 或 波浪圆弧形
	if(nNum==1)
		designHoleInfo.bWaveShape=FALSE;
	else if(AfxMessageBox("板外形设计:Y(外凸圆弧形)/N(波浪圆弧形)?",MB_YESNO)==IDYES)
		designHoleInfo.bWaveShape=FALSE;
	else
		designHoleInfo.bWaveShape=TRUE;
	//更新始终端顶点，删除不合理的挂线孔
	RefreshStartAndEndVertex(pPlate,&designHoleInfo);
	if(!RemoveErrorHoles(&designHoleInfo))
		return;
	//计算轮廓边始终顶点与相应挂线孔所在圆的公切点
	PROFILE_VER startTangPt,endTangPt;
	if(!CalStartAndEndTangentPt(&designHoleInfo,startTangPt,endTangPt))
		return ;
	//多个挂线孔的公切点计算
	ATOM_LIST<PROFILE_VER> vertexList;
	if(designHoleInfo.guaXianHoleInfoArr.GetSize()>1)		
	{
		if(!CalMultiHoleTangentPt(&designHoleInfo,vertexList))
			return;
	}
	//检查切点之间的合理性
	vertexList.append(endTangPt);
	vertexList.insert(startTangPt,0);
	f3dPoint vec=designHoleInfo.pEndVertex->vertex-designHoleInfo.pStartVertex->vertex;
	normalize(vec);
	PROFILE_VER curVertex,nextVertex;
	for(int i=0;i<vertexList.GetNodeNum()-1;i++)
	{
		curVertex=vertexList[i];
		nextVertex=vertexList[(i+1)%vertexList.GetNodeNum()];
		f3dPoint offset_vec=nextVertex.vertex-curVertex.vertex;
		normalize(offset_vec);
		if(vec*offset_vec<0)
		{
			vertexList.DeleteAt(i);
			i--;
			vertexList.DeleteAt(i+1);
			continue;
		}
	}
	//计算圆弧的角度
	for(PROFILE_VER* pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		if(pVer->type==1)
			continue;
		f3dPoint startPt,endPt;
		startPt=pVer->vertex;
		BOOL bPush=vertexList.push_stack();
		PROFILE_VER* pNextVer=vertexList.GetNext();
		endPt=pNextVer->vertex;
		f3dArcLine arcLine;
		arcLine.CreateMethod3(startPt,endPt,f3dPoint(0,0,1),pVer->radius,pVer->center);
		pVer->sector_angle=arcLine.SectorAngle();
		pVer->work_norm=arcLine.WorkNorm();
		if(bPush)
			vertexList.pop_stack();
	}
	//插入新增顶点,更新板外形
	PROFILE_VER* pVertex=NULL;
	int index=0;
	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		if(pVertex==designHoleInfo.pEndVertex)
			break;
		index++;
	}
	for(pVertex=vertexList.GetFirst();pVertex;pVertex=vertexList.GetNext())
	{	
		pVertex->vertex.feature=designHoleInfo.pStartVertex->vertex.feature;
		pPlate->vertex_list.insert(*pVertex,index);
		index++;
	}
	//进行绘制
	m_xSelectEntity.Empty();
	SyncPartInfo(true);
	UpdatePropertyPage();
}
//计算距挂线孔距离最近的轮廓边
BOOL CalNearestEdgeToHole(CProcessPlate* pPlate,GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo)
{
	if(pDesignHoleInfo->guaXianHoleInfoArr.GetSize()<=0)
	{
		logerr.Log("没有选中进行设计的挂线孔!");
		return FALSE;
	}
	//计算与挂线孔距离最近的轮廓边
	double fMinDist=10000,fDist=0,fSumDist=0;
	f3dPoint inters;
	PROFILE_VER *pPreVertex=pPlate->vertex_list.GetTail();
	for(PROFILE_VER *pCurVertex=pPlate->vertex_list.GetFirst();pCurVertex;pCurVertex=pPlate->vertex_list.GetNext())
	{
		BOOL bValid=TRUE;
		fSumDist=0;
		for(GUAXIANHOLE_BASE_INFO* pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();pHoleInfo;pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext())
		{	
			SnapPerp(&inters,pPreVertex->vertex,pCurVertex->vertex,pHoleInfo->ls_pos,&fDist);
			fSumDist+=fDist;
			if(f3dPoint(inters-pPreVertex->vertex)*f3dPoint(pCurVertex->vertex-inters)<0)	//挂线孔投影点在此轮廓边外面
			{
				bValid=FALSE;
				break;
			}
		}
		if(bValid&&fSumDist<fMinDist)
		{	//距挂线孔最短距离
			fMinDist=fSumDist;
			pDesignHoleInfo->pStartVertex=pPreVertex;
			pDesignHoleInfo->pEndVertex=pCurVertex;
		}
		pPreVertex=pCurVertex;
	}
	if(pDesignHoleInfo->pStartVertex==NULL || pDesignHoleInfo->pEndVertex==NULL)
	{
		logerr.Log("找不到距挂线孔最近的轮廓边，查看轮廓边是否选择正确!");
		return FALSE;
	}
	//计算挂孔向最近边垂足点到最近边起点距离，并按由近及远排序 wjh-2013.8.15
	for(GUAXIANHOLE_BASE_INFO *pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();pHoleInfo;pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext())
	{
		SnapPerp(&inters,pDesignHoleInfo->pStartVertex->vertex,pDesignHoleInfo->pEndVertex->vertex,pHoleInfo->ls_pos);
		pHoleInfo->lengthPosToEdge=DISTANCE(pDesignHoleInfo->pStartVertex->vertex,inters);
	}
	int nNum=pDesignHoleInfo->guaXianHoleInfoArr.GetSize();
	CHeapSort<GUAXIANHOLE_BASE_INFO>::HeapSortClassic(pDesignHoleInfo->guaXianHoleInfoArr.m_pData,nNum);
	return TRUE;
}
//重新检测轮廓边的始终端顶点是否合理，选取合适的轮廓顶点
void RefreshStartAndEndVertex(CProcessPlate* pPlate,GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo)
{
	GUAXIANHOLE_BASE_INFO* pFirstHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();
	GUAXIANHOLE_BASE_INFO* pEndHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetTail();
	f3dPoint startPt=pDesignHoleInfo->pStartVertex->vertex;
	f3dPoint endPt=pDesignHoleInfo->pEndVertex->vertex;
	if(DISTANCE(pFirstHoleInfo->ls_pos,startPt)<=pFirstHoleInfo->fRadius)
	{	//始端顶点在挂线点圆弧内,删除此顶点，取前一个顶点
		PROFILE_VER *pVertex=NULL,*pPreVertex=pPlate->vertex_list.GetTail();
		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		{
			if(pVertex->hiberId.IsEqual(pDesignHoleInfo->pStartVertex->hiberId))
				break;
			pPreVertex=pVertex;
		}
		pDesignHoleInfo->pStartVertex=pPreVertex;
		pPlate->vertex_list.DeleteNode(pVertex->keyId);
	}
	if(DISTANCE(pEndHoleInfo->ls_pos,endPt)<=pEndHoleInfo->fRadius)
	{	//终端顶点在挂线点圆弧内,删除此顶点，取下一个顶点
		PROFILE_VER* pVertex=NULL,*pNextVertex=NULL;
		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		{
			if(pVertex->hiberId.IsEqual(pDesignHoleInfo->pEndVertex->hiberId))
			{
				pNextVertex=pPlate->vertex_list.GetNext();
					break;
			}
		}
		if(pNextVertex==NULL)
			pNextVertex=pPlate->vertex_list.GetFirst();
		pDesignHoleInfo->pEndVertex=pNextVertex;
		pPlate->vertex_list.DeleteNode(pVertex->keyId);
	}
	pPlate->vertex_list.Clean();
}
//去除不合理的挂线孔
BOOL RemoveErrorHoles(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo)
{
	int nNum=pDesignHoleInfo->guaXianHoleInfoArr.GetSize();
	f3dPoint startPt=pDesignHoleInfo->pStartVertex->vertex;
	f3dPoint endPt=pDesignHoleInfo->pEndVertex->vertex;
	for(GUAXIANHOLE_BASE_INFO* pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();pHoleInfo;pHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext())
	{
		double fDistToLine=-DistOf2dPtLine(pHoleInfo->ls_pos,startPt,endPt);
		if((pHoleInfo==&(pDesignHoleInfo->guaXianHoleInfoArr[0]) ||pHoleInfo==&(pDesignHoleInfo->guaXianHoleInfoArr[nNum-1]))
			&&fDistToLine<=-pHoleInfo->fRadius)
		{	//始端或终端挂线孔所在的圆在轮廓边的左侧（板内）
			logerr.Log("所选挂线孔不合理，设计失败!");
			return FALSE;
		}
	}
	if(nNum<=2)
		return TRUE;
	f3dPoint vec,offset_vec;
	vec=endPt-startPt;
	normalize(vec);
	offset_vec.Set(vec.y,-vec.x,0);
	for(int i=1;i<nNum-1;i++)
	{
		GUAXIANHOLE_BASE_INFO holeInfo=pDesignHoleInfo->guaXianHoleInfoArr[i];
		if(!pDesignHoleInfo->bWaveShape)
		{	//凸弧形要求中间孔距离前后孔弧公切线左侧垂距<圆弧半径
			f2dCircle circle1,circle2;
			f3dPoint pick_pt1=pDesignHoleInfo->guaXianHoleInfoArr[0].ls_pos+offset_vec*10;
			circle1.radius=pDesignHoleInfo->guaXianHoleInfoArr[0].fRadius;
			circle1.centre.Set(pDesignHoleInfo->guaXianHoleInfoArr[0].ls_pos.x,pDesignHoleInfo->guaXianHoleInfoArr[0].ls_pos.y);
			f3dPoint pick_pt2=pDesignHoleInfo->guaXianHoleInfoArr[nNum-1].ls_pos+offset_vec*10;
			circle2.radius=pDesignHoleInfo->guaXianHoleInfoArr[nNum-1].fRadius;
			circle2.centre.Set(pDesignHoleInfo->guaXianHoleInfoArr[nNum-1].ls_pos.x,pDesignHoleInfo->guaXianHoleInfoArr[nNum-1].ls_pos.y);
			f2dLine tan_line;
			if(TangLine2dcc(circle1,pick_pt1,circle2,pick_pt2,tan_line)!=1)	//始末挂线孔的公切线
				return FALSE;
			vec=f3dPoint(tan_line.endPt.x,tan_line.endPt.y,0)-f3dPoint(tan_line.startPt.x,tan_line.startPt.y,0);
			normalize(vec);
			double dist=DistOf2dPtLine(holeInfo.ls_pos,f3dPoint(tan_line.startPt.x,tan_line.startPt.y,0),f3dPoint(tan_line.endPt.x,tan_line.endPt.y,0));
			if(dist>holeInfo.fRadius || fabs(dist-holeInfo.fRadius)<EPS2)
			{
				pDesignHoleInfo->guaXianHoleInfoArr.RemoveAt(i);
				nNum--;
				i--;
			}
		}
		//else{//波浪形要求挂孔间间距<两凸弧半径加一凹弧半径}
	}
	return TRUE;
}
//计算始终端顶点到相应挂线孔圆弧的切点
BOOL CalStartAndEndTangentPt(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo,PROFILE_VER& startTangPt,PROFILE_VER& endTangPt)
{
	f3dPoint startPt,endPt,vec,pick_vec,pick_pos;
	startPt=pDesignHoleInfo->pStartVertex->vertex;
	endPt=pDesignHoleInfo->pEndVertex->vertex;
	vec=endPt-startPt;
	normalize(vec);
	pick_vec.Set(vec.y,-vec.x,0);
	//始端顶点与始端处挂线孔所在圆弧求切点
	f2dPoint tan_pt;
	f2dCircle circle;
	GUAXIANHOLE_BASE_INFO* pStartHoleInfo,*pEndHoleInfo;
	pStartHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();
	circle.radius=pStartHoleInfo->fRadius;
	circle.centre.Set(pStartHoleInfo->ls_pos.x,pStartHoleInfo->ls_pos.y);
	pick_pos=pStartHoleInfo->ls_pos+pick_vec*10;
	if(Tang2dpc(f2dPoint(startPt.x,startPt.y),circle,pick_pos,tan_pt))
	{
		startTangPt.vertex.Set(tan_pt.x,tan_pt.y,0);
		startTangPt.type=2;
		startTangPt.center=pick_pos;
		startTangPt.radius=pStartHoleInfo->fRadius;
	}
	else
	{
		logerr.Log("始端顶点在挂线孔圆弧内部,设计失败！");
		return FALSE;
	}
	//终端顶点与终端处挂线孔所在圆弧求切点
	pEndHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetTail();
	circle.radius=pEndHoleInfo->fRadius;
	circle.centre.Set(pEndHoleInfo->ls_pos.x,pEndHoleInfo->ls_pos.y);
	pick_pos=pEndHoleInfo->ls_pos+pick_vec*10;
	if(Tang2dpc(f2dPoint(endPt.x,endPt.y),circle,pick_pos,tan_pt))
	{
		endTangPt.vertex.Set(tan_pt.x,tan_pt.y,0);
		endTangPt.type=1;
	}
	else
	{	
		logerr.Log("终端顶点在挂线孔圆弧内部,设计失败！");
		return FALSE;
	}
	return TRUE;
}
//计算相邻挂线孔之间的公切点，保存到列表中
BOOL CalMultiHoleTangentPt(GUAXIANHOLE_DESIGN_INFO* pDesignHoleInfo,ATOM_LIST<PROFILE_VER> &vertexList)
{
	f3dPoint vec,offset_vec;
	vec=pDesignHoleInfo->pEndVertex->vertex-pDesignHoleInfo->pStartVertex->vertex;
	normalize(vec);
	if(pDesignHoleInfo->bWaveShape)	
	{	//波浪圆弧形设计
		double angle,cosa,start_radius,end_radius,tan_cir_r,dis;
		f3dPoint axis(0,0,1);
		PROFILE_VER startVertex,endVertex;
		GUAXIANHOLE_BASE_INFO* pPreHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();
		for(GUAXIANHOLE_BASE_INFO* pCurHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext();pCurHoleInfo;pCurHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext())
		{
			dis=DISTANCE(pPreHoleInfo->ls_pos,pCurHoleInfo->ls_pos);
			if(dis<=fabs(pPreHoleInfo->fRadius-pCurHoleInfo->fRadius))
			{
				logerr.Log("两圆相交或相套,合法外公切圆不存在!");
				return FALSE;
			}
			start_radius=pPreHoleInfo->fRadius+dis/2;
			end_radius=pCurHoleInfo->fRadius+dis/2;
			tan_cir_r=(dis/2)*-1;
			cosa=(SQR(start_radius)+SQR(dis)-SQR(end_radius))*0.5/(start_radius*dis);
			angle=ACOS(cosa);
			offset_vec=pCurHoleInfo->ls_pos-pPreHoleInfo->ls_pos;
			normalize(offset_vec);
			RotateVectorAroundVector(offset_vec,angle,-axis);
			startVertex.vertex=pPreHoleInfo->ls_pos+offset_vec*pPreHoleInfo->fRadius;
			startVertex.vertex.z=0;
			startVertex.type=2;
			startVertex.center=pPreHoleInfo->ls_pos+offset_vec*start_radius;
			startVertex.radius=tan_cir_r;
			vertexList.append(startVertex);
			//
			cosa=(SQR(end_radius)+SQR(dis)-SQR(start_radius))*0.5/(end_radius*dis);
			angle=ACOS(cosa);
			offset_vec=pPreHoleInfo->ls_pos-pCurHoleInfo->ls_pos;
			normalize(offset_vec);
			RotateVectorAroundVector(offset_vec,angle,axis);
			endVertex.vertex=pCurHoleInfo->ls_pos+offset_vec*pCurHoleInfo->fRadius;
			endVertex.vertex.z=0;
			endVertex.type=2;
			endVertex.center=pCurHoleInfo->ls_pos;
			endVertex.radius=pCurHoleInfo->fRadius;
			vertexList.append(endVertex);
			pPreHoleInfo=pCurHoleInfo;
		}
	}
	else	
	{	//外凸圆弧形设计,计算相邻挂线孔所在圆的公切点
		f3dPoint pick_pt1,pick_pt2;
		offset_vec.Set(vec.y,-vec.x,0);
		f2dCircle circle1,circle2;
		PROFILE_VER startVertex,endVertex;
		GUAXIANHOLE_BASE_INFO* pPreHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetFirst();
		for(GUAXIANHOLE_BASE_INFO* pCurHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext();pCurHoleInfo;pCurHoleInfo=pDesignHoleInfo->guaXianHoleInfoArr.GetNext())
		{
			pick_pt1=pPreHoleInfo->ls_pos+offset_vec*10;
			circle1.radius=pPreHoleInfo->fRadius;
			circle1.centre.Set(pPreHoleInfo->ls_pos.x,pPreHoleInfo->ls_pos.y);
			pick_pt2=pCurHoleInfo->ls_pos+offset_vec*10;
			circle2.radius=pCurHoleInfo->fRadius;
			circle2.centre.Set(pCurHoleInfo->ls_pos.x,pCurHoleInfo->ls_pos.y);
			f2dLine tan_line;
			if(TangLine2dcc(circle1,pick_pt1,circle2,pick_pt2,tan_line)==1)
			{
				startVertex.vertex.Set(tan_line.startPt.x,tan_line.startPt.y,0);
				startVertex.type=1;
				vertexList.append(startVertex);
				//
				endVertex.vertex.Set(tan_line.endPt.x,tan_line.endPt.y,0);
				endVertex.type=2;
				endVertex.radius=pCurHoleInfo->fRadius;
				endVertex.center=pick_pt2;
				vertexList.append(endVertex);
			}
			else
			{
				logerr.Log("两圆相交或相套，合法公切线不存在!");
				return FALSE;
			}
			pPreHoleInfo=pCurHoleInfo;
		}
	}
	return TRUE;
}
//火曲钢板
#include "BendPlankStyleDlg.h"
void CPPEView::OnBendPlank()
{
	CLogErrorLife logErrLife;
	if(m_pProcessPart==NULL || m_pProcessPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate* pPlate=(CProcessPlate*)m_pProcessPart;
	if(pPlate->m_cFaceN>=3)
	{
		logerr.Log("本系统现不能处理三面以上的火曲板,钢板火曲失败!");
		return;
	}
	CXhPtrSet<PROFILE_VER> selectVertex;
	m_xSelectEntity.GetSelectVertexs(selectVertex,m_pProcessPart);
	PROFILE_VER* pStartVer=NULL,*pEndVer=NULL;
	pStartVer=selectVertex.GetFirst();
	pEndVer=selectVertex.GetTail();
	if(pStartVer->vertex.feature>10 || pEndVer->vertex.feature>10)
	{
		logerr.Log("选择了火曲点为新火曲线端点,钢板火曲失败!");
		return;
	}
	CBendPlankStyleDlg bend_style_dlg;
	f3dPoint bend_face_norm,rot_axis;
	rot_axis=pEndVer->vertex-pStartVer->vertex;
	normalize(rot_axis);
	if(bend_style_dlg.m_iBendFaceStyle>0)	//输入法线值确定火曲面
	{
		bend_style_dlg.m_fNormX=pPlate->HuoQuFaceNorm[pPlate->m_cFaceN-2].x;
		bend_style_dlg.m_fNormY=pPlate->HuoQuFaceNorm[pPlate->m_cFaceN-2].y;
		bend_style_dlg.m_fNormZ=pPlate->HuoQuFaceNorm[pPlate->m_cFaceN-2].z;
	}
	if(bend_style_dlg.DoModal()==IDOK)
	{
		if(bend_style_dlg.m_iBendFaceStyle==0)
		{
			bend_face_norm.Set(0,0,1);
			RotateVectorAroundVector(bend_face_norm,bend_style_dlg.m_fBendAngle*RADTODEG_COEF,rot_axis);
		}
		else if(bend_style_dlg.m_iBendFaceStyle==2)
			bend_face_norm.Set(bend_style_dlg.m_fNormX,bend_style_dlg.m_fNormY,bend_style_dlg.m_fNormZ);
	}
	else
		return;
	SCOPE_STRU scope;//新生成的火曲面顶点组成的区域,用来判断螺栓是否在火曲面上 wht 09-09-04
	pPlate->m_cFaceN++;
	pPlate->HuoQuLine[pPlate->m_cFaceN-2].startPt=pStartVer->vertex;
	pPlate->HuoQuLine[pPlate->m_cFaceN-2].startPt.feature=10+pPlate->m_cFaceN;
	pPlate->HuoQuLine[pPlate->m_cFaceN-2].endPt=pEndVer->vertex;
	pPlate->HuoQuLine[pPlate->m_cFaceN-2].endPt.feature=10+pPlate->m_cFaceN;
	
	scope.VerifyVertex(pStartVer->vertex); 
	scope.VerifyVertex(pEndVer->vertex);
	BOOL bStart=FALSE;
	PROFILE_VER* pPt=pPlate->vertex_list.GetFirst();
	while(1)
	{
		if(bStart)	//开始火曲面
		{
			scope.VerifyVertex(pPt->vertex);//火曲面轮廓区
			if(pPt==pEndVer)
			{
				pPt->vertex.feature=pPlate->m_cFaceN+10;
				break;		//终止火曲面
			}
			if(pPt->vertex.feature==1)
				pPt->vertex.feature=pPlate->m_cFaceN;
		}
		else		//查找火曲面开始点
		{
			if(pPt==pStartVer)
			{
				pPt->vertex.feature=pPlate->m_cFaceN+10;
				bStart=TRUE;
			}
		}
		pPt=pPlate->vertex_list.GetNext();
		if(pPt==NULL&&bStart)
			pPt=pPlate->vertex_list.GetFirst();
		else if(pPt==NULL)	//已经将轮廓点循环了一遍但未找到pStart,此处需要跳出否则会死循环 wht 11-01-15
			break;
	}
	normalize(bend_face_norm);
	pPlate->HuoQuFaceNorm[pPlate->m_cFaceN-2]=bend_face_norm;
	//完成火曲钢板后，更新新生成的火曲面上螺栓的法线 wht 09-09-04
	for(BOLT_INFO* pLs=pPlate->m_xBoltInfoList.GetFirst();pLs;pLs=pPlate->m_xBoltInfoList.GetNext())
	{
		f3dPoint ls_pos(pLs->posX,pLs->posY,0);
		if(scope.IsIncludePoint(ls_pos))
		{	//更新螺栓法线方向
			
			//(*pLsRef)->set_norm(bend_face_norm);
			//(*pLsRef)->des_work_norm.norm_style=0;	//指定法线
			//(*pLsRef)->des_work_norm.vector=bend_face_norm;
		}
	}
	//进行绘制
	m_xSelectEntity.Empty();
	SyncPartInfo(true,true);
	UpdatePropertyPage();	
}

void CPPEView::UpdateCurWorkPartByPartNo(const char *part_no)
{
	if(!theApp.starter.IsMultiPartsMode())
		return;
	if (part_no && strlen(part_no) > 0)
		m_pProcessPart = model.FromPartNo(part_no, g_sysPara.nc.m_ciDisplayType);
	else
		m_pProcessPart=NULL;
	if(g_pPartEditor)
		g_pPartEditor->ClearProcessParts();
	if (m_pProcessPart == NULL)
		return;
#ifdef __PNC_
	if(m_pProcessPart->IsPlate())
	{	//根据系统设置初始化引入线长度 
		CProcessPlate *pPlate=(CProcessPlate*)m_pProcessPart;
		pPlate->m_xCutPt.cOutLineLen=(BYTE)g_sysPara.GetCutOutLineLen(pPlate->GetThick());
		pPlate->m_xCutPt.cInLineLen=(BYTE)g_sysPara.GetCutInLineLen(pPlate->GetThick());
	}
#endif
	CBuffer buffer;
	m_pProcessPart->ToPPIBuffer(buffer);
	buffer.SeekToBegin();
	if(g_pPartEditor)
		g_pPartEditor->AddProcessPart(buffer,m_pProcessPart->GetKey());
	if(theApp.starter.IsCompareMode()&&model.PartCount()==2)
	{	//构件对比模式，添加对比构件
		for(CProcessPart *pPart=model.EnumPartFirst();pPart;pPart=model.EnumPartNext())
		{
			if(pPart->GetPartNo().Equal(part_no))
				continue;
			buffer.ClearContents();
			pPart->ToPPIBuffer(buffer);
			if(g_pPartEditor)
				g_pPartEditor->AddProcessPart(buffer,pPart->GetKey());
		}
	}
}
bool CPPEView::SavePartInfoToFile()
{
	if(m_pProcessPart==NULL)
		return false;
	return model.SavePartToFile(m_pProcessPart);
}
bool CPPEView::SyncPartInfo(bool bToPEC,bool bReDraw/*=true*/)
{
	if(m_pProcessPart==NULL)
		return false;
	CBuffer part_buffer;
	if(bToPEC)
	{	//同步PPE构件信息至PEC并重绘
		m_pProcessPart->ToPPIBuffer(part_buffer);
		g_pPartEditor->UpdateProcessPart(part_buffer);
		if(bReDraw)
		{
			g_pPartEditor->ReDraw();	//更新选中的DbEntity数据
			g_p2dDraw->RenderDrawing();	//将DbEntity绘制到屏幕,此函数功能没有实现
		}
	}
	else
	{	//PEC修改构件信息后同步至PPE
		if(!g_pPartEditor->GetProcessPart(part_buffer))
			return false;
		part_buffer.SeekToBegin();
		m_pProcessPart->FromPPIBuffer(part_buffer);
		if(bReDraw)
			Refresh();
	}
	//存在对应文件时保存PPI文件
	//SavePartInfoToFile();
	return true;
}
void CPPEView::UpdateSelectEnt()
{
	m_xSelectEntity.Empty();
	//重新更新m_xSelectEntity
	IDrawing *pDrawing=g_p2dDraw->GetActiveDrawing();
	long *id_arr = NULL;
	int n=g_pSolidSnap->GetLastSelectEnts(id_arr);
	for(int i=0;i<n;i++)
	{
		IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(id_arr[i]);
		if(pEnt==NULL)
			continue;
		m_xSelectEntity.Append(pEnt);
	}
}
void CPPEView::UpdatePropertyPage()
{
	CLogErrorLife logLife;
	if(!g_sysPara.dock.pageProp.bDisplay)
		return;
	CPartPropertyDlg *pPropDlg=((CMainFrame*)AfxGetMainWnd())->GetPartPropertyPage();
	CPropertyList *pPropList=pPropDlg->GetPropertyList();
	if(m_pProcessPart==NULL)
		DisplaySysSettingProperty();
	else if(m_pProcessPart->m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		if(m_xSelectEntity.GetSelectEntNum()<=0)
			DisplayLineAngleProperty();
		else if(m_xSelectEntity.IsSelectBolt(m_pProcessPart))
			DisplayBoltHoleProperty();
		//else
		//	logerr.Log("工艺角钢选择非螺栓对象没有意义!");
	}
	else if(m_pProcessPart->m_cPartType==CProcessPart::TYPE_PLATE)
	{
		if(m_xSelectEntity.GetSelectEntNum()<=0)
			DisplayPlateProperty();
		else if(m_xSelectEntity.IsSelectCutPt(m_pProcessPart))
			DisplayCutPointProperty();
		else if(m_xSelectEntity.IsSelectBolt(m_pProcessPart))
			DisplayBoltHoleProperty();
		else if(m_xSelectEntity.IsSelectVertex(m_pProcessPart))
			DisplayVertexProperty();
		else if(m_xSelectEntity.IsSelectEdge(m_pProcessPart))
			DisplayEdgeLineProperty();
		//else
		//	logerr.Log("工艺钢板选择非同类型对象没有意义!");
	}	
}
void CPPEView::AmendHoleIncrement()
{
	CXhChar16 sCurPartNo;
	if(m_pProcessPart)
		sCurPartNo=m_pProcessPart->GetPartNo();
	CWaitCursor waitCursor;
	CProcessPart* pProcessPart = NULL;
#ifdef __PNC_
	//修正不同加工工艺下钢板螺栓孔径增大值
	for (int i = 1; i <= 5; i++)
	{
		NC_INFO_PARA* pNCPare = NULL;
		DWORD iNcMode = GetSingleWord(i);
		if (iNcMode == CNCPart::FLAME_MODE)
			pNCPare = &g_sysPara.nc.m_xFlamePara;
		else if (iNcMode == CNCPart::PLASMA_MODE)
			pNCPare = &g_sysPara.nc.m_xPlasmaPara;
		else if (iNcMode == CNCPart::PUNCH_MODE)
			pNCPare = &g_sysPara.nc.m_xPunchPara;
		else if (iNcMode == CNCPart::DRILL_MODE)
			pNCPare = &g_sysPara.nc.m_xDrillPara;
		else if (iNcMode == CNCPart::LASER_MODE)
			pNCPare = &g_sysPara.nc.m_xLaserPara;
		for (pProcessPart = model.EnumPartFirst(); pProcessPart; pProcessPart = model.EnumPartNext())
		{
			if (!pProcessPart->IsPlate()|| pProcessPart->m_xBoltInfoList.GetNodeNum() <= 0)
				continue;
			CProcessPlate* pSrcPlate = (CProcessPlate*)pProcessPart;
			CProcessPlate* pDestPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), iNcMode);
			for (BOLT_INFO* pBolt = pDestPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pDestPlate->m_xBoltInfoList.GetNext())
			{
				if (pBolt->bolt_d == 12 && pBolt->cFuncType == 0)
					pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM12;
				else if (pBolt->bolt_d == 16 && pBolt->cFuncType == 0)
					pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM16;
				else if (pBolt->bolt_d == 20 && pBolt->cFuncType == 0)
					pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM20;
				else if (pBolt->bolt_d == 24 && pBolt->cFuncType == 0)
					pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM24;
				else if (pBolt->cFuncType >= 2)
				{	//特殊孔
					if (pBolt->bolt_d >= g_sysPara.nc.m_fLimitSH)
					{	//大号孔处理
						pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fCutSH;
					}
					else
					{	//小号孔处理
						if ((iNcMode == CNCPart::PUNCH_MODE&&pNCPare->m_bReduceSmallSH) ||
							(iNcMode == CNCPart::DRILL_MODE&&pNCPare->m_bReduceSmallSH))
						{	//板床孔进行降级处理
							if (pBolt->bolt_d > 24)
							{
								pBolt->bolt_d = 24;
								pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM24;
							}
							else if (pBolt->bolt_d > 20)
							{
								pBolt->bolt_d = 20;
								pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM20;
							}
							else if (pBolt->bolt_d > 16)
							{
								pBolt->bolt_d = 16;
								pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM16;
							}
							else
							{
								pBolt->bolt_d = 12;
								pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fM12;
							}
						}
						else
							pBolt->hole_d_increment = (float)pNCPare->m_xHoleIncrement.m_fProSH;
					}
				}
			}
		}
	}
#else
	for (pProcessPart = model.EnumPartFirst(); pProcessPart; pProcessPart = model.EnumPartNext())
	{
		if (!pProcessPart->IsPlate() || pProcessPart->m_xBoltInfoList.GetNodeNum() <= 0)
			continue;
		for(BOLT_INFO* pBolt=pProcessPart->m_xBoltInfoList.GetFirst();pBolt;pBolt=pProcessPart->m_xBoltInfoList.GetNext())
		{
			if(pBolt->bolt_d==12&&pBolt->cFuncType==0)
				pBolt->hole_d_increment=(float)g_sysPara.holeIncrement.m_fM12;
			else if(pBolt->bolt_d==16&&pBolt->cFuncType==0)
				pBolt->hole_d_increment=(float)g_sysPara.holeIncrement.m_fM16;
			else if(pBolt->bolt_d==20&&pBolt->cFuncType==0)
				pBolt->hole_d_increment=(float)g_sysPara.holeIncrement.m_fM20;
			else if(pBolt->bolt_d==24&&pBolt->cFuncType==0)
				pBolt->hole_d_increment=(float)g_sysPara.holeIncrement.m_fM24;
			else if (pBolt->cFuncType >= 2)
			{	//特殊孔
				if (pBolt->bolt_d >= g_sysPara.nc.m_fLimitSH)
					pBolt->hole_d_increment = (float)g_sysPara.holeIncrement.m_fCutSH;
				else
					pBolt->hole_d_increment = (float)g_sysPara.holeIncrement.m_fProSH;
			}
		}

	}
#endif
	UpdateCurWorkPartByPartNo(sCurPartNo);
	Refresh();
}