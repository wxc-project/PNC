// PartListDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PartListDlg.h"
#include "PNCModel.h"
#include "ArrayList.h"
#include "CadToolFunc.h"
#include "CadHighlightEntManager.h"
#include "PNCCmd.h"
#include "acdocman.h"
#include "DrawDamBoard.h"
#include "AdjustPlateMCS.h"
#include "AcUiDialogPanel.h"
#include "PNCSysPara.h"
#include "PNCSysSettingDlg.h"
#include "PNCCryptCoreCode.h"

#ifndef __UBOM_ONLY_
// CPartListDlg 对话框
int CPartListDlg::m_nDlgWidth = 220;
#ifdef __SUPPORT_DOCK_UI_
IMPLEMENT_DYNCREATE(CPartListDlg, CAcUiDialog)
CPartListDlg::CPartListDlg(CWnd* pParent /*=NULL*/)
	: CAcUiDialog(CPartListDlg::IDD, pParent)
	, m_bEditMK(FALSE)
{
#else
IMPLEMENT_DYNCREATE(CPartListDlg, CDialog)
CPartListDlg::CPartListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPartListDlg::IDD, pParent)
	, m_bEditMK(FALSE)
	, m_sNote(_T(""))
{
#endif

}

CPartListDlg::~CPartListDlg()
{
}

void CPartListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PART_LIST, m_partList);
	DDX_Check(pDX, IDC_CHK_EDIT_MK, m_bEditMK);
	DDX_Text(pDX, IDC_E_NOTE, m_sNote);
}


BEGIN_MESSAGE_MAP(CPartListDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_NOTIFY(NM_CLICK, IDC_PART_LIST, &CPartListDlg::OnNMClickPartList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_PART_LIST, &CPartListDlg::OnKeydownListPart)
	ON_BN_CLICKED(IDC_BTN_EXTRACT, &CPartListDlg::OnBnClickedBtnExtract)
	ON_BN_CLICKED(IDC_BTN_SETTINGS, &CPartListDlg::OnBnClickedSettings)
	ON_BN_CLICKED(IDC_BTN_SEND_TO_PPE, &CPartListDlg::OnBnClickedBtnSendToPpe)
	ON_BN_CLICKED(IDC_BTN_EXPORT_DXF, &CPartListDlg::OnBnClickedBtnExportDxf)
	ON_BN_CLICKED(IDC_BTN_ANTICLOCKWISE_ROTATION, &CPartListDlg::OnBnClickedBtnAnticlockwiseRotation)
	ON_BN_CLICKED(IDC_BTN_CLOCKWISE_ROTATION, &CPartListDlg::OnBnClickedBtnClockwiseRotation)
	ON_BN_CLICKED(IDC_BTN_MIRROR, &CPartListDlg::OnBnClickedBtnMirror)
	ON_BN_CLICKED(IDC_BTN_MOVE, &CPartListDlg::OnBnClickedBtnMove)
	ON_BN_CLICKED(IDC_BTN_MOVE_MK_RECT, &CPartListDlg::OnBnClickedBtnMoveMkRect)
	ON_BN_CLICKED(IDC_CHK_EDIT_MK, &CPartListDlg::OnBnClickedChkEditMk)
	ON_NOTIFY(NM_DBLCLK, IDC_PART_LIST, &CPartListDlg::OnNMDblclkPartList)
	ON_NOTIFY(NM_RCLICK, IDC_PART_LIST, &CPartListDlg::OnNMRClickPartList)
	ON_COMMAND(ID_REVISE_TEH_PLATE, &CPartListDlg::OnRetrievedPlate)
	ON_COMMAND(ID_DELETE_ITEM, &CPartListDlg::OnDeleteItem)
END_MESSAGE_MAP()


// CPartListDlg 消息处理程序
BOOL CPartListDlg::OnInitDialog()
{
#ifdef __SUPPORT_DOCK_UI_
	CAcUiDialog::OnInitDialog();
#else
	CDialog::OnInitDialog();
#endif
	m_partList.m_arrHeader.RemoveAll();
	m_partList.AddColumnHeader("件号", 70);
	m_partList.AddColumnHeader("规格", 40);
	m_partList.AddColumnHeader("材质", 40);
	m_partList.AddColumnHeader("数量", 40);
	m_partList.InitListCtrl();
	//
	if (m_bmpLeftRotate.LoadBitmap(IDB_LEFT_ROTATE))
		((CButton*)GetDlgItem(IDC_BTN_ANTICLOCKWISE_ROTATION))->SetBitmap(m_bmpLeftRotate);
	if (m_bmpRightRotate.LoadBitmap(IDB_RIGHT_ROTATE))
		((CButton*)GetDlgItem(IDC_BTN_CLOCKWISE_ROTATION))->SetBitmap(m_bmpRightRotate);
	if (m_bmpMirror.LoadBitmap(IDB_MIRROR))
		((CButton*)GetDlgItem(IDC_BTN_MIRROR))->SetBitmap(m_bmpMirror);
	if (m_bmpMoveMk.LoadBitmapA(IDB_MOVE_MK))
		((CButton*)GetDlgItem(IDC_BTN_MOVE_MK_RECT))->SetBitmap(m_bmpMoveMk);
	if (m_bmpMove.LoadBitmap(IDB_MOVE))
		((CButton*)GetDlgItem(IDC_BTN_MOVE))->SetBitmap(m_bmpMove);
	//初始化控件显示状态
	RefreshCtrlState();
	return TRUE;
}
void CPartListDlg::RefreshCtrlState()
{
	int nShowCode = (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS) ? SW_SHOW : SW_HIDE;
	//GetDlgItem(IDC_BTN_SEND_TO_PPE)->ShowWindow(nShowCode);
	//GetDlgItem(IDC_BTN_EXPORT_DXF)->ShowWindow(nShowCode);
	GetDlgItem(IDC_BTN_ANTICLOCKWISE_ROTATION)->ShowWindow(nShowCode);
	GetDlgItem(IDC_BTN_CLOCKWISE_ROTATION)->ShowWindow(nShowCode);
	GetDlgItem(IDC_BTN_MIRROR)->ShowWindow(nShowCode);
	GetDlgItem(IDC_BTN_MOVE_MK_RECT)->ShowWindow(nShowCode);
	GetDlgItem(IDC_BTN_MOVE)->ShowWindow(nShowCode);
	GetDlgItem(IDC_CHK_EDIT_MK)->ShowWindow(nShowCode);
}

BOOL CPartListDlg::IsValidDoc(CString &sFileName)
{
	CString file_name;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc)
		file_name = pDoc->fileName();
	if (file_name.GetLength() <= 0 || file_name.CompareNoCase("Drawing1.dwg") == 0)
		return FALSE;
	sFileName = file_name;
	return TRUE;
}

void CPartListDlg::ClearPartList()
{
	model.Empty();
	//
	m_partList.DeleteAllItems();
	m_sNote = "总数{0},成功{0},失败{0}";
	UpdateData(FALSE);
}

BOOL CPartListDlg::UpdatePartList()
{
	m_partList.DeleteAllItems();
	CString file_name;
	if (!IsValidDoc(file_name) || model.m_sCurWorkFile.CompareNoCase(file_name) != 0)
		return FALSE;
	CStringArray str_arr;
	str_arr.SetSize(4);
	CSortedModel sortedModel(&model);
	int nSum = 0, nValid = 0;
	for(CPlateProcessInfo *pPlate=sortedModel.EnumFirstPlate();pPlate;pPlate=sortedModel.EnumNextPlate())
	{
		str_arr[0]=pPlate->xPlate.GetPartNo();
		str_arr[1].Format("-%.f",pPlate->xPlate.m_fThick);
		str_arr[2]=CProcessPart::QuerySteelMatMark(pPlate->xPlate.cMaterial);
		str_arr[3].Format("%d", pPlate->xPlate.m_nProcessNum);
		int iItem=m_partList.InsertItemRecord(-1,str_arr);
		m_partList.SetItemData(iItem,(DWORD)pPlate);
		//设置颜色
		if (!pPlate->IsValid())
			m_partList.SetItemTextColor(iItem, 0, RGB(255, 0, 0));
		else if(!pPlate->IsClose())
			m_partList.SetItemTextColor(iItem, 0, RGB(255, 191, 0));
		else
			m_partList.SetItemTextColor(iItem, 0, RGB(0, 0, 0));
		//统计数量
		if(pPlate->IsValid())
			nValid++;
		nSum++;
	}
	if(CDrawDamBoard::m_bDrawAllBamBoard&&g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
	{
		m_xDamBoardManager.DrawAllDamBoard(&model);
		m_xDamBoardManager.DrawAllSteelSealRect(&model);
	}
	//
	m_sNote.Format("总数{%d},成功{%d},失败{%d}", nSum, nValid, nSum - nValid);
	UpdateData(FALSE);
	return TRUE;
}

void CPartListDlg::SelectPart(int iCurSel, BOOL bClone /*= TRUE*/)
{
	CPlateProcessInfo *pPlate=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlate==NULL)
		return;
	if (bClone)
	{	//定位到新绘制的图形范围
		if (pPlate->IsValid())
		{	//钢板提取成功，显示
			if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
			{	//下料预审时处理挡板
				SCOPE_STRU scope = pPlate->GetCADEntScope(TRUE);
				f2dPoint leftBtm(scope.fMinX, scope.fMinY);
				f2dRect rect;
				rect.topLeft.Set(leftBtm.x, leftBtm.y + CDrawDamBoard::BOARD_HEIGHT);
				rect.bottomRight.Set(leftBtm.x + CDrawDamBoard::BOARD_HEIGHT, leftBtm.y);
				ZoomAcadView(rect, 100);
				//
				CLockDocumentLife lock;
				if (!CDrawDamBoard::m_bDrawAllBamBoard)
					m_xDamBoardManager.EraseAllDamBoard();
				m_xDamBoardManager.DrawDamBoard(pPlate);
			}
			else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PRINT)
			{	//自动排版时处理块引用
				SCOPE_STRU scope = pPlate->GetCADEntScope(TRUE);
				CAcDbObjLife objLife(pPlate->m_layoutBlockId);
				AcDbEntity *pEnt = objLife.GetEnt();
				if (pEnt && pEnt->isKindOf(AcDbBlockReference::desc()))
				{
					AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
					AcGePoint3d pos = pBlockRef->position();
					f2dRect rect;
					rect.topLeft.Set(scope.fMinX + pos.x, scope.fMaxY + pos.y);
					rect.bottomRight.Set(scope.fMaxX + pos.x, scope.fMinY + pos.y);
					ZoomAcadView(rect, 50);
				}
				else
					ZoomAcadView(scope, 50);
			}
			else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_COMPARE)
			{	//钢板对比时处理提取生成的钢板轮廓点
				SCOPE_STRU scope = pPlate->GetCADEntScope(TRUE);
				f2dRect rect = GetCadEntRect(pPlate->m_newAddEntIdList);
				scope.VerifyVertex(rect.topLeft);
				scope.VerifyVertex(rect.bottomRight);
				ZoomAcadView(scope, 50);
			}
			else
			{
				SCOPE_STRU scope = pPlate->GetCADEntScope(TRUE);
				ZoomAcadView(scope, 50);
			}
		}
		else
		{	//钢板提取失败，显示
			f2dRect rect = pPlate->GetPnDimRect(15, 30);
			ZoomAcadView(rect, 30);
		}
	}
	else
	{	//定位到原始图形
		SCOPE_STRU scope = pPlate->GetCADEntScope(FALSE);
		ZoomAcadView(scope, 50);
	}
	//修改视图位置后需要及时更新界面,否则acedSSGet()可能不能获取正确的实体集 wht 11-06-25
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}
void CPartListDlg::PreSubclassWindow()
{
#ifdef __SUPPORT_DOCK_UI_
	ModifyStyle(DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION, DS_SETFONT | WS_CHILD);
	CAcUiDialog::PreSubclassWindow();
#else
	//ModifyStyle(WS_CHILD, DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU);
	CDialog::PreSubclassWindow();
#endif
}

void CPartListDlg::OnOK()
{
#ifndef __SUPPORT_DOCK_UI_
	return CDialog::OnOK();
#endif
}
void CPartListDlg::OnCancel()
{
#ifndef __SUPPORT_DOCK_UI_
	CDialog::OnCancel();
#endif
}

void CPartListDlg::RelayoutWnd()
{
	if (m_partList.GetSafeHwnd() == NULL)
		return;
	CRect rectWnd, rectList, rectBtm, rectPic;
	GetClientRect(&rectWnd);
	GetDlgItem(IDC_BTN_ANTICLOCKWISE_ROTATION)->GetWindowRect(&rectPic);
	ScreenToClient(&rectPic);
	int nPicRectHight = (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS) ? rectPic.Height()+1 : 0;
	//
	GetDlgItem(IDC_E_NOTE)->GetWindowRect(&rectBtm);
	ScreenToClient(&rectBtm);
	rectBtm.left = rectWnd.left;
	rectBtm.right = rectWnd.right;
	rectBtm.top = rectWnd.bottom - rectBtm.Height();
	rectBtm.bottom = rectWnd.bottom;
	GetDlgItem(IDC_E_NOTE)->MoveWindow(&rectBtm);
	//
	m_partList.GetWindowRect(&rectList);
	ScreenToClient(&rectList);
	rectList.left = rectWnd.left;
	rectList.right = rectWnd.right;
	rectList.top = rectPic.top + nPicRectHight;
	rectList.bottom = rectBtm.top - 1;
	m_partList.MoveWindow(rectList);
}

void CPartListDlg::OnClose()
{
	DestroyWindow();
}

void CPartListDlg::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);
	//
	RelayoutWnd();
}

void CPartListDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	//
	RelayoutWnd();
}

BOOL CPartListDlg::CreateDlg()
{
	return CDialog::Create(CPartListDlg::IDD);
}

void CPartListDlg::OnNMClickPartList(NMHDR *pNMHDR, LRESULT *pResult)
{
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if(pos!=NULL)
	{
		int iCurSel = m_partList.GetNextSelectedItem(pos);
		if(iCurSel>=0)
			SelectPart(iCurSel);
	}
	*pResult = 0;
}

void CPartListDlg::OnNMDblclkPartList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		int iCurSel = m_partList.GetNextSelectedItem(pos);
		if (iCurSel >= 0)
			SelectPart(iCurSel, FALSE);
	}
	*pResult = 0;
}

void CPartListDlg::OnNMRClickPartList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CPlateProcessInfo *pSelPlate = NULL;
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		int iCurSel = m_partList.GetNextSelectedItem(pos);
		if (iCurSel >= 0)
			pSelPlate = (CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	}
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos));
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu = popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0, MF_BYPOSITION);
	if (pSelPlate)
	{
		pMenu->AppendMenu(MF_STRING, ID_REVISE_TEH_PLATE, "重新提取");
		pMenu->AppendMenu(MF_SEPARATOR);
	}
	pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "清空");
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = 0;
}

void CPartListDlg::OnRetrievedPlate()
{
	CPlateProcessInfo *pSelPlate = NULL;
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		int iCurSel = m_partList.GetNextSelectedItem(pos);
		if (iCurSel >= 0)
			pSelPlate = (CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	}
	if (pSelPlate == NULL)
		return;
	CXhChar16 sPartNo = pSelPlate->GetPartNo();
	//
	CPNCModel tempMode;
	CPNCModel::m_bSendCommand = TRUE;
	SmartExtractPlate(&tempMode,TRUE);
	CPlateProcessInfo* pFirstPlate = tempMode.EnumFirstPlate(FALSE);
	if (tempMode.GetPlateNum() != 1 || 
		!sPartNo.Equal(pFirstPlate->GetPartNo())||
		!pFirstPlate->IsValid())
	{
		AfxMessageBox(CXhChar50("钢板{%s}重新提取失败!",(char*)sPartNo));
		return;
	}
	pSelPlate->CopyAttributes(pFirstPlate);
	UpdatePartList();
}
void CPartListDlg::OnDeleteItem()
{
	model.Empty();
	UpdatePartList();
}

void CPartListDlg::OnBnClickedBtnExtract()
{
	CString file_name;
	if (!IsValidDoc(file_name))
		return;
	CPNCModel::m_bSendCommand = TRUE;
	if (model.m_sCurWorkFile.CompareNoCase(file_name) == 0)
	{	//同一文档（分次处理）
		CPNCModel tempMode;
		SmartExtractPlate(&tempMode, TRUE);
		//数据拷贝
		CPlateProcessInfo* pSrcPlate = NULL, *pDestPlate = NULL;
		for (pSrcPlate = tempMode.EnumFirstPlate(FALSE); pSrcPlate; pSrcPlate = tempMode.EnumNextPlate(FALSE))
		{
			pDestPlate = model.GetPlateInfo(pSrcPlate->GetPartNo());
			if (pDestPlate == NULL)
				pDestPlate = model.AppendPlate(pSrcPlate->GetPartNo());
			pDestPlate->CopyAttributes(pSrcPlate);
		}
	}
	else
	{	//不同文档(重新处理)
		model.Empty();
		model.m_sCurWorkFile = file_name;
		SmartExtractPlate(&model, TRUE);
	}
	UpdatePartList();
}
void CPartListDlg::OnBnClickedSettings()
{
	CAcModuleResourceOverride resOverride;
	CLogErrorLife logErrLife;
	CPNCSysSettingDlg dlg;
	dlg.DoModal();
}
BOOL CPartListDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		int iCurSel = -1;
		POSITION pos = m_partList.GetFirstSelectedItemPosition();
		if (pos != NULL)
			iCurSel = m_partList.GetNextSelectedItem(pos);
		if (iCurSel >= 0)
		{
			if (pMsg->wParam == VK_LEFT)
			{
				OnBnClickedBtnAnticlockwiseRotation();
				return TRUE;
			}	
			if (pMsg->wParam == VK_RIGHT)
			{
				OnBnClickedBtnClockwiseRotation();
				return TRUE;
			}	
			if (pMsg->wParam == VK_NEXT)
			{
				OnBnClickedBtnMirror();
				return TRUE;
			}
		}
	}
#ifdef __SUPPORT_DOCK_UI_
	return CAcUiDialog::PreTranslateMessage(pMsg);
#else
	return CDialog::PreTranslateMessage(pMsg);
#endif
}
void CPartListDlg::OnKeydownListPart(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	int iCurSel = -1;
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if (pos != NULL)
		iCurSel = m_partList.GetNextSelectedItem(pos);
	if (iCurSel >= 0)
	{
		if (pLVKeyDow->wVKey == VK_UP)
		{
			if (iCurSel >= 1)
				iCurSel--;
			if (iCurSel >= 0)
				SelectPart(iCurSel);
		}
		else if (pLVKeyDow->wVKey == VK_DOWN)
		{
			if (iCurSel < m_partList.GetItemCount())
				iCurSel++;
			if (iCurSel >= 0)
				SelectPart(iCurSel);
		}
	}
	*pResult = 0;
}

void CPartListDlg::OnBnClickedBtnSendToPpe()
{
#ifndef __UBOM_ONLY_
	SendPartEditor();
#endif
}

void CPartListDlg::OnBnClickedBtnExportDxf()
{
	CLockDocumentLife lock;
	CString sDxfFolderPath,sFilePath;
	GetCurWorkPath(sDxfFolderPath,TRUE,"DXF");
	for(CPlateProcessInfo *pPlate=model.EnumFirstPlate(FALSE);pPlate;pPlate=model.EnumNextPlate(FALSE))
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for(ULONG *pId=pPlate->m_cloneEntIdList.GetFirst();pId;pId=pPlate->m_cloneEntIdList.GetNext())
			entIdList.append(MkCadObjId(*pId));
		sFilePath.Format("%s%s.dxf",sDxfFolderPath,(char*)pPlate->GetPartNo());
		SaveAsDxf(sFilePath,entIdList,true);
	}
	ShellExecute(NULL,"open",NULL,NULL,sDxfFolderPath,SW_SHOW);
}


void CPartListDlg::OnBnClickedBtnAnticlockwiseRotation()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo, FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	if (pPlateInfo->xPlate.mcsFlg.ciOverturn == TRUE)
		mcs.ClockwiseRotation();
	else
		mcs.AnticlockwiseRotation();
	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	//更新PPI文件
	if (CPlateProcessInfo::m_bCreatePPIFile)
	{
		CString file_path;
		GetCurWorkPath(file_path);
		pPlateInfo->CreatePPiFile(file_path);
	}
	//刷新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}


void CPartListDlg::OnBnClickedBtnClockwiseRotation()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo, FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	if (pPlateInfo->xPlate.mcsFlg.ciOverturn == TRUE)
		mcs.AnticlockwiseRotation();
	else
		mcs.ClockwiseRotation();
	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	if (CPlateProcessInfo::m_bCreatePPIFile)
	{	//更新PPI文件
		CString file_path;
		GetCurWorkPath(file_path);
		pPlateInfo->CreatePPiFile(file_path);
	}
	//刷新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}

void CPartListDlg::OnBnClickedBtnMirror()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo,FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	mcs.Mirror();

	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	if (CPlateProcessInfo::m_bCreatePPIFile)
	{	//更新PPI文件
		CString file_path;
		GetCurWorkPath(file_path);
		pPlateInfo->CreatePPiFile(file_path);
	}
	//刷新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}


void CPartListDlg::OnBnClickedBtnMove()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CLockDocumentLife lock;
	DRAGSET.ClearEntSet();
	ARRAY_LIST<AcDbObjectId> entIdList;
	for(ULONG *pId=pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=pPlateInfo->m_cloneEntIdList.GetNext())
	{
		DRAGSET.Add(MkCadObjId(*pId));
		entIdList.append(MkCadObjId(*pId));
	}
	f2dRect rect=GetCadEntRect(entIdList);
	f3dPoint basepnt(rect.topLeft.x,rect.bottomRight.y);
#ifdef AFX_TARG_ENU_ENGLISH
	CXhChar100 sPrompt("please enter the position\n");
#else
	CXhChar100 sPrompt("请输入一个新位置\n");
#endif
	int nRetCode=DragEntSet(basepnt,sPrompt);
	if(nRetCode==RTNORM)
	{	//更新构件规格位置
		m_xDamBoardManager.EraseDamBoard(pPlateInfo);
		m_xDamBoardManager.DrawDamBoard(pPlateInfo);
		for(CAD_LINE *pLineId=pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetFirst();pLineId;pLineId=pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetNext())
			pLineId->UpdatePos();
		//更新字盒位置
		CLockDocumentLife lockLife;
		m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}

void CPartListDlg::OnBnClickedBtnMoveMkRect()
{	//调整钢印号位置
	int iCurSel = m_partList.GetSelectedItem();
	if (iCurSel < 0)
		return;
	CPlateProcessInfo *pPlateInfo = (CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if (pPlateInfo == NULL)
		return;
	CLockDocumentLife lock;
	DRAGSET.ClearEntSet();
	AcDbObjectId pointId = MkCadObjId(pPlateInfo->m_xMkDimPoint.idCadEnt);
	DRAGSET.Add(pointId);
	AcDbObjectId entId=m_xDamBoardManager.GetSteelSealRectId(pPlateInfo);
	DRAGSET.Add(entId);
	AcDbEntity *pEnt = NULL;
	XhAcdbOpenAcDbEntity(pEnt, pointId, AcDb::kForRead);
	if (pEnt == NULL)
		return;
	pEnt->close();
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	f3dPoint basepnt(pPoint->position().x, pPoint->position().y);
#ifdef AFX_TARG_ENU_ENGLISH
	CXhChar100 sPrompt("please enter the position\n");
#else
	CXhChar100 sPrompt("请输入一个新位置\n");
#endif
	int nRetCode = DragEntSet(basepnt, sPrompt);
	if (nRetCode == RTNORM)
	{	//更新字盒位置
		m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
		//更新字盒子位置之后，同步更新PPI文件中钢印号位置
		pPlateInfo->SyncSteelSealPos();
		if (CPlateProcessInfo::m_bCreatePPIFile)
		{	//更新PPI文件
			CString file_path;
			GetCurWorkPath(file_path);
			pPlateInfo->CreatePPiFile(file_path);
		}
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}
void CPartListDlg::OnBnClickedChkEditMk()
{
	/*m_bEditMK = !m_bEditMK;
	if (m_bEditMK)
		SetupEditMkWatch();
	else
		UninstallEditMkWatch();*/
}
#endif
