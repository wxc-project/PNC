// PromptDlg.cpp : implementation file
//<LOCALE_TRANSLATE BY wbt />

#include "stdafx.h"
#include "PromptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg dialog

CPromptDlg *g_pPromptMsg;
CPromptDlg::CPromptDlg(CWnd* pWnd)
{
	//{{AFX_DATA_INIT(CPromptDlg)
	m_sPromptMsg = _T("");
	//}}AFX_DATA_INIT
	m_pWnd = pWnd;
	m_iType=0;
	m_bForceExitCommand=FALSE;
	m_bPickingObjects=FALSE;
}


void CPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPromptDlg)
	DDX_Text(pDX, IDC_MSG, m_sPromptMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPromptDlg, CDialog)
	//{{AFX_MSG_MAP(CPromptDlg)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_WM_ERASEBKGND()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg message handlers
BOOL CPromptDlg::Create()
{
	return CDialog::Create(CPromptDlg::IDD);
}
BOOL CPromptDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CSize sizeMaxScr;
	sizeMaxScr.cx = GetSystemMetrics(SM_CXSCREEN);
	sizeMaxScr.cy = GetSystemMetrics(SM_CYSCREEN);
	CRect rect,client_rect;
	GetWindowRect(rect);
	GetClientRect(client_rect);
	long fWidth=rect.Width()-client_rect.Width();
	long fHight=rect.Height()-client_rect.Height();
	CEdit* pEdit=(CEdit*)GetDlgItem(IDC_MSG);
	BITMAP bmInfo;
	if(m_iType==1)
	{
		pEdit->ShowWindow(SW_HIDE);
		m_bBitMap.GetBitmap(&bmInfo);
	}
	else
	{	
		pEdit->ShowWindow(SW_SHOW);
		bmInfo.bmWidth=client_rect.Width();
		bmInfo.bmHeight=client_rect.Height();
	}
	rect.right=sizeMaxScr.cx-2;
	rect.left =rect.right-bmInfo.bmWidth-fWidth;
	rect.top=0;
	rect.bottom =bmInfo.bmHeight+fHight;

	MoveWindow(rect, TRUE);
	m_bBrush.CreateSolidBrush(RGB(253,247,198));
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

HBRUSH CPromptDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if(nCtlColor==CTLCOLOR_STATIC)
	{
		pDC->SetBkColor(RGB(253,247,199));
		return m_bBrush;
	}
	else
		return hbr;
}

void CPromptDlg::SetMsg(const char* msg)
{
	m_iType=0;
	if(GetSafeHwnd()==0)
		Create();
	m_sPromptMsg = msg;
	m_bForceExitCommand=FALSE;
	UpdateData(FALSE);
}
void CPromptDlg::SetMsg(long bitMapId)
{
	m_iType=1;
	if(!m_bBitMap.LoadBitmap(bitMapId))
	{
#ifdef AFX_TARG_ENU_ENGLISH
		AfxMessageBox("Please make sure the bitmap resources exist!");
#else 
		AfxMessageBox("请确定位图资源存在!");
#endif
		return;
	}
	if(GetSafeHwnd()==0)
		Create();
	m_bForceExitCommand=FALSE;
}
BOOL CPromptDlg::Destroy()
{
	m_bForceExitCommand=TRUE;
	m_bBrush.DeleteObject();
	return DestroyWindow();	
}

BOOL CPromptDlg::OnEraseBkgnd(CDC* pDC)
{
	if(m_iType==1)
	{
		BITMAP bmInfo;
		m_bBitMap.GetBitmap(&bmInfo);
		CDC MemDc;
		MemDc.CreateCompatibleDC(pDC);
		CBitmap* pOldMap=MemDc.SelectObject(&m_bBitMap);
		pDC->BitBlt(0,0,bmInfo.bmWidth,bmInfo.bmHeight,&MemDc,0,0,SRCCOPY);
		MemDc.SelectObject(pOldMap);
	}
	return TRUE;
}

void CPromptDlg::OnClose()
{
	Destroy();
	CDialog::OnClose();
}

BOOL CPromptDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN)   
	{   
		if(!(GetKeyState(VK_CONTROL)&0x8000))
		{
			if(m_pWnd)
			{
				m_pWnd->SetFocus();
				pMsg->hwnd = m_pWnd->GetSafeHwnd();
			}
		}
		if(pMsg->wParam==VK_SPACE)	//使空格等同回车 wht 10-03-25
			pMsg->wParam=VK_RETURN;
		return CDialog::PreTranslateMessage(pMsg);
	}   
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CPromptDlg::StartPickObjects(BOOL bCaptureLButtonUpMsg/*=FALSE*/)//默认不响应鼠标左键抬起的消息
{
	BOOL bRet=TRUE;
	for(;;)
	{
		m_bPickingObjects = TRUE;
		if(m_bForceExitCommand)
		{
			m_bPickingObjects = m_bForceExitCommand = FALSE;
			return FALSE;
		}
		MSG message;
		if(PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			if(bCaptureLButtonUpMsg&&message.message==WM_LBUTTONUP)
			{	//点击鼠标左键和按回车或按空格效果相同
				m_bPickingObjects = m_bForceExitCommand = FALSE;
				return TRUE;
			}
			else if(message.message==WM_KEYDOWN)
			{
				if(message.wParam==VK_RETURN||message.wParam==VK_SPACE)
				{	//按回车或按空格效果相同
					m_bPickingObjects = m_bForceExitCommand = FALSE;
					return TRUE;
				}
				else if(message.wParam==VK_ESCAPE)
				{
					m_bPickingObjects = m_bForceExitCommand = FALSE;
					return FALSE;
				}
				CWnd *pWnd = CWnd::FromHandle(message.hwnd);
				CWnd *pParentWnd = NULL;
				HWND hParent = NULL;
				if(pWnd)
				{
					pParentWnd = pWnd->GetParent();
					if(pParentWnd)
						hParent = pParentWnd->GetSafeHwnd();
				}
				if((message.hwnd==GetParent()->GetSafeHwnd()||message.hwnd==GetSafeHwnd()||hParent==GetSafeHwnd())&&
					message.wParam!=VK_SHIFT&&message.wParam!=VK_CONTROL&&message.wParam!=VK_CAPITAL)
				{
					if(pWnd)
					{
						pWnd->SetFocus();
						message.hwnd = pWnd->GetSafeHwnd();
					}
				}
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	return FALSE;
}
