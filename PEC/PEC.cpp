// PartsEditorCord.cpp : 定义 DLL 的初始化例程。
//
#include "stdafx.h"
#include <afxdllx.h>
#include "ProcessPart.h"
#include "ProcessPartDraw.h"
#include "PEC.h"
#include "NcJg.h"
#include "UserDefMsg.h"
#include "2DPtDlg.h"
#include "PlankVertexDlg.h"
#include "LineFeatDlg.h"
#include "BoltHolePropDlg.h"
#include "Query.h"
#include "DefProfileVertexDlg.h"
#include "PartLib.h"
#include "SortFunc.h"
#include "direct.h"
#include "JgDrawing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE PECDll = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("PEC.DLL Initializing!\n");

		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(PECDll, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(PECDll);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("PEC.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(PECDll);
	}
	return 1;   // ok
}

//////////////////////////////////////////////////////////////////////////
// CPEC
static CProcessPart *CreatePart(int idClsType,DWORD key,void* pContext)
{
	CProcessPart *pPart = NULL;
	switch (idClsType)
	{
	case CProcessPart::TYPE_PLATE:
		pPart=new CProcessPlate();
		pPart->SetKey(key);
		break;
	case CProcessPart::TYPE_LINEANGLE:
		pPart=new CProcessAngle();
		pPart->SetKey(key);
		break;
	default:
		break;
	}
	return pPart;
}
static BOOL DeletePart(CProcessPart *pPart)
{
	switch (pPart->m_cPartType)
	{
	case CProcessPart::TYPE_PLATE:
		delete (CProcessPlate*)pPart;
		break;
	case CProcessPart::TYPE_LINEANGLE:
		delete (CProcessAngle*)pPart;
		break;
	default:
		break;
	}
	return TRUE;
}

static CProcessPartDraw *CreatePartDraw(int idClsType,DWORD key,void* pContext)
{
	CProcessPartDraw *pPartDraw = NULL;
	switch (idClsType)
	{
	case CProcessPart::TYPE_PLATE:
		pPartDraw=new CProcessPlateDraw();
		break;
	case CProcessPart::TYPE_LINEANGLE:
		pPartDraw=new CProcessAngleDraw();
		break;
	default:
		break;
	}
	return pPartDraw;
}
static BOOL DeletePartDraw(CProcessPartDraw *pPartDraw)
{
	if(pPartDraw==NULL)
		return FALSE;
	CProcessPart *pPart=pPartDraw->GetPart();
	if(pPart==NULL)
		return FALSE;
	switch ((int)pPart->m_cPartType)
	{
	case CProcessPart::TYPE_PLATE:
		delete (CProcessPlateDraw*)pPartDraw;
		break;
	case CProcessPart::TYPE_LINEANGLE:
		delete (CProcessAngleDraw*)pPartDraw;
		break;
	default:
		break;
	}
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//
CPEC::CPEC(int iNo)
{
	m_iNo=0;
	m_wDrawMode=DRAW_MODE_EDIT;	
	m_cPartType=0;
	m_hWnd=NULL;
	m_p2dDraw=NULL;
	m_pSolidDraw=NULL;
	m_pSolidSet=NULL;
	m_pSolidSnap=NULL;
	m_pSolidOper=NULL;
	m_hashPartDrawBySerial.CreateNewAtom = CreatePartDraw;
	m_hashPartDrawBySerial.DeleteAtom = DeletePartDraw;
	m_hashProcessPartBySerial.CreateNewAtom = CreatePart;
	m_hashProcessPartBySerial.DeleteAtom = DeletePart;
	m_workCS.InitStdCS();
	FirePartModify=NULL;
}
CPEC::~CPEC()
{
	m_hashPartDrawBySerial.Empty();
	m_hashProcessPartBySerial.Empty();
}

bool CPEC::InitDrawEnviornment(HWND hWnd,I2dDrawing* p2dDraw,ISolidDraw* pSolidDraw,
	ISolidSet* pSolidSet,ISolidSnap* pSolidSnap,ISolidOper* pSolidOper)
{
	m_hWnd=hWnd;
	m_p2dDraw=p2dDraw;
	m_pSolidDraw=pSolidDraw;
	m_pSolidSet=pSolidSet;
	m_pSolidSnap=pSolidSnap;
	m_pSolidOper=pSolidOper;
	m_wCurCmdType=CMD_OTHER;
	m_pSolidDraw->BatchClearCS(ISolidDraw::WORK_CS);
	return true;
}
//1.编辑模式，2.NC模式，3.工艺草图模式
int CPEC::SetDrawMode(WORD mode)
{
	m_wDrawMode=mode;
	m_pSolidDraw->BatchClearCS(ISolidDraw::WORK_CS);
	if(m_wDrawMode==DRAW_MODE_EDIT)
		m_pSolidDraw->AddCS(ISolidDraw::WORK_CS,m_workCS);
	return m_wDrawMode;
}
//向编辑器中增加当前绘制显示的工艺构件
bool CPEC::AddProcessPart(CBuffer &processPartBuffer,DWORD key)
{
	if(processPartBuffer.GetLength()<=0)
		return false;
	//初始化绘图环境坐标系
	m_workCS.InitStdCS();	//向构件编辑器中添加构件时需要重置工作坐标系
	m_pSolidDraw->BatchClearCS(ISolidDraw::WORK_CS);
	if(m_wDrawMode==DRAW_MODE_EDIT)
		m_pSolidDraw->AddCS(ISolidDraw::WORK_CS,m_workCS);
	//初始化构件信息
	processPartBuffer.SeekToBegin();
	BYTE cPartType=CProcessPart::RetrievedPartTypeAndLabelFromBuffer(processPartBuffer);
	if(cPartType!=CProcessPart::TYPE_LINEANGLE&&cPartType!=CProcessPart::TYPE_PLATE)
		return false;
	if(m_cPartType==0)
		m_cPartType=cPartType;
	else if(m_cPartType!=cPartType)
		return false;	//构件编辑器中编辑多个构件时只允许编辑同一种构件
	CProcessPart *pPart=m_hashProcessPartBySerial.Add(key,cPartType);
	processPartBuffer.SeekToBegin();
	pPart->FromPPIBuffer(processPartBuffer);
	//初始化构件的绘制环境
	CProcessPartDraw *pPartDraw=m_hashPartDrawBySerial.Add(pPart->GetKey(),cPartType);
	pPartDraw->SetPart(pPart);
	pPartDraw->SetBelongEditor(this);
	if(m_hashPartDrawBySerial.GetNodeNum()==1)
		pPartDraw->SetMainPartDraw(TRUE);
	return true;
}
//From ProcessPartDraw.cpp
void TransPlateToMCS(CProcessPlate *pPlate,GECS &mcs);
bool CPEC::GetProcessPart(CBuffer &partBuffer,int serial/*=0*/)
{
	int i=0;
	CProcessPart *pPart=NULL;
	for(pPart=m_hashProcessPartBySerial.GetFirst();pPart;pPart=m_hashProcessPartBySerial.GetNext())
	{
		if(i==serial)
			break;
		i++;
	}
	if(pPart==NULL)
		return false;
	partBuffer.ClearContents();
	pPart->ToPPIBuffer(partBuffer);
	return true;
}
char CPEC::GetCurPartType()
{
	return m_cPartType;
}
bool CPEC::UpdateProcessPart(CBuffer &partBuffer,int serial/*=0*/)
{
	int i=0;
	CProcessPart *pPart=NULL;
	for(pPart=m_hashProcessPartBySerial.GetFirst();pPart;pPart=m_hashProcessPartBySerial.GetNext())
	{
		if(i==serial)
			break;
		i++;
	}
	if(pPart==NULL)
		return false;
	partBuffer.SeekToBegin();
	BYTE cPartType=CProcessPart::RetrievedPartTypeAndLabelFromBuffer(partBuffer);
	if(cPartType!=pPart->m_cPartType)
		return false;
	partBuffer.SeekToBegin();
	pPart->FromPPIBuffer(partBuffer);
	return true;
}
//清空编辑器中当前工艺构件集
void CPEC::ClearProcessParts()
{
	m_cPartType=0;
	m_wCurCmdType=CMD_OTHER;
	m_hashPartDrawBySerial.Empty();
	m_hashProcessPartBySerial.Empty();
}

void CPEC::Draw()
{
	//CLogErrorLife logLife;
	if(m_hashPartDrawBySerial.GetNodeNum()<=0)
	{
		IDrawing *pDrawing=m_p2dDraw->GetActiveDrawing();
		if(pDrawing)
			pDrawing->EmptyDbEntities();
		//logerr.Log("Count=0");
		return;
	}
	DISPLAY_TYPE displayType;
	m_pSolidSet->GetDisplayType(&displayType);
	IDrawingAssembly *pDrawAssembly=m_p2dDraw->GetDrawingAssembly();
	for(IDrawing *pDrawing=pDrawAssembly->EnumFirstDrawing();pDrawing;pDrawing=pDrawAssembly->EnumNextDrawing())
		pDrawing->EmptyDbEntities();
	if(displayType==DISP_DRAWING)
	{
		for(CProcessPartDraw *pPartDraw=m_hashPartDrawBySerial.GetFirst();pPartDraw;pPartDraw=m_hashPartDrawBySerial.GetNext())
			pPartDraw->Draw(m_p2dDraw,m_pSolidSet);
	}
}
void CPEC::ReDraw(int serial/*=0*/)
{
	int i=0;
	CProcessPart *pPart=NULL;
	for(pPart=m_hashProcessPartBySerial.GetFirst();pPart;pPart=m_hashProcessPartBySerial.GetNext())
	{
		if(i==serial)
			break;
		i++;
	}
	if(pPart==NULL)
		return;
	i=0;
	CProcessPartDraw *pPartDraw=NULL;
	for(pPartDraw=m_hashPartDrawBySerial.GetFirst();pPartDraw;pPartDraw=m_hashPartDrawBySerial.GetNext())
	{
		if(i==serial)
			break;
		i++;
	}
	if(pPartDraw==NULL)
		return;
	pPartDraw->SetPart(pPart);
	pPartDraw->ReDraw(m_p2dDraw,m_pSolidSnap,m_pSolidDraw);
}
bool CPEC::SetWorkCS(GECS cs)
{
	m_workCS=cs;
	return false;
}
bool CPEC::SetCallBackProcessPartModified()
{
	return false;
}
bool CPEC::SetCallBackOperationFunc()
{
	return false;
}
void CPEC::RotateAntiClockwisePlate()
{
	if(m_cPartType!=CProcessPart::TYPE_PLATE)//||m_hashPartDrawBySerial.GetNodeNum()!=1)
		return;
	CProcessPlateDraw *pPlateDraw=(CProcessPlateDraw*)m_hashPartDrawBySerial.GetFirst();
	if(pPlateDraw==NULL)
		return;
	pPlateDraw->RotateAntiClockwise();
}
void CPEC::RotateClockwisePlate()
{
	if(m_cPartType!=CProcessPart::TYPE_PLATE)//||m_hashPartDrawBySerial.GetNodeNum()!=1)
		return;
	CProcessPlateDraw *pPlateDraw=(CProcessPlateDraw*)m_hashPartDrawBySerial.GetFirst();
	if(pPlateDraw==NULL)
		return;
	pPlateDraw->RotateClockwise();
}
void CPEC::OverturnPart()
{	
	CProcessPartDraw *pPartDraw=m_hashPartDrawBySerial.GetFirst();
	if(pPartDraw)
		pPartDraw->OverturnPart();
}
void CPEC::GetPlateNcUcs(GECS& cs)
{
	if(m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlateDraw *pPlatePartDraw=(CProcessPlateDraw*)m_hashPartDrawBySerial.GetFirst();
	if(pPlatePartDraw)
		cs=pPlatePartDraw->GetMCS();
}
void CPEC::NewBoltHole()
{
	if(m_wDrawMode!=DRAW_MODE_EDIT||m_hashProcessPartBySerial.GetNodeNum()<=0)
		return;
	CBoltHolePropDlg boltHoleDlg;
	boltHoleDlg.m_pWorkPart=m_hashProcessPartBySerial.GetFirst();
	boltHoleDlg.work_ucs=m_workCS;
	if(boltHoleDlg.DoModal()==IDOK)
	{
		BOLT_INFO boltHole;
		boltHole.waistLen=boltHoleDlg.m_nWaistLen;
		f3dPoint pos(boltHoleDlg.m_fPosX,boltHoleDlg.m_fPosY);
		coord_trans(pos,m_workCS,TRUE);
		boltHole.posX=(float)pos.x;
		boltHole.posY=(float)pos.y;
		if(!restore_Ls_guige(boltHoleDlg.m_sLsGuiGe,boltHole))
		{
			AfxMessageBox("输入螺栓规格非法,螺栓生成失败!");
			return ;
		}
		boltHole.hole_d_increment = (float)(boltHoleDlg.m_fHoleD-boltHole.bolt_d);
		boltHole.waistVec=boltHoleDlg.waist_vec;
		m_pSolidDraw->BuildDisplayList();
		if(boltHoleDlg.m_pWorkPart->m_cPartType==CProcessAngle::TYPE_LINEANGLE)
		{
			CProcessAngle *pAngle=(CProcessAngle*)boltHoleDlg.m_pWorkPart;
			pAngle->m_xBoltInfoList.SetValue(0,boltHole,true);
		}
		else if(boltHoleDlg.m_pWorkPart->m_cPartType==CProcessAngle::TYPE_PLATE)
			((CProcessPlate*)boltHoleDlg.m_pWorkPart)->m_xBoltInfoList.SetValue(0,boltHole,true);
		m_pSolidDraw->BuildDisplayList();
	}
}

void CPEC::CalPlateWrapProfile()
{
	if(m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_hashProcessPartBySerial.GetFirst();
	if(pPlate==NULL)
		return;
	CalPlateWrapProfile(pPlate);
	m_pSolidDraw->BuildDisplayList();
}
//用于返回外包切线的夹角和切线 WJH-2004.9.22
double WrapLine2dcc(double start_x,double start_y,double start_radius,double end_x,double end_y,double end_radius,f2dLine &wrap_tan_line)
{
	f2dCircle cir1,cir2;
	f2dPoint pick1,pick2,axis_norm;
	cir1.centre.Set(start_x,start_y);
	cir2.centre.Set(end_x,end_y);
	cir1.radius=start_radius;
	cir2.radius=end_radius;
	axis_norm.Set(end_y-start_y,start_x-end_x);//顺时针转90度,(x,y)-->(y,-x)
	pick1.Set(start_x+axis_norm.x,start_y+axis_norm.y);
	pick2.Set(end_x+axis_norm.x,end_y+axis_norm.y);
	if(TangLine2dcc(cir1,pick1,cir2,pick2,wrap_tan_line)!=1)
		return -1;	//没找到合法的外包切线
	return Cal2dLineAng(wrap_tan_line.startPt,wrap_tan_line.endPt);
}

double WrapLine2dcc(f2dCircle cir1,f2dCircle cir2,f2dLine &wrap_tan_line)
{
	return WrapLine2dcc(cir1.centre.x,cir1.centre.y,cir1.radius,cir2.centre.x,cir2.centre.y,cir2.radius,wrap_tan_line);
}
void CPEC::CalPlateWrapProfile(CProcessPlate *pPlate,double angle_lim/*=60.0*/)
{
	if(pPlate->m_cFaceN>=3)
		return;
	ATOM_LIST<f2dCircle> ls_cir_list,out_ls_list;
	f3dPoint ls_pos;
	for(BOLT_INFO *pHole=pPlate->m_xBoltInfoList.GetFirst();pHole;pHole=pPlate->m_xBoltInfoList.GetNext())
	{
		f2dCircle ls_cir;
		ls_cir.centre.Set(pHole->posX,pHole->posY);
		LSSPACE_STRU LsSpace;
		//只有特殊孔才可能是小数，特殊孔也查不到合适的螺栓边距，此处可强制转为整数 wht 19-09-12
		GetLsSpace(LsSpace,(long)pHole->bolt_d);
		ls_cir.radius = LsSpace.PlateEdgeSpace;
		ls_cir_list.append(ls_cir);
	}
	//查找外轮廓特征螺栓
	//通过最简便的方式找出其中一颗外轮廓特征螺栓,作为外轮廓首螺栓
	BOOL bHasTwoLs=FALSE,bOneAxisLineLs=TRUE;	//所有螺栓都在一条心线上的情况
	int start_space,end_space;
	f3dLine axis_line;
	f2dCircle *pLsCir=NULL,*pStartLsCir=NULL,*pEndLsCir=NULL,*pFirstStartLsCir=NULL;
	for(pLsCir=ls_cir_list.GetFirst();pLsCir;pLsCir=ls_cir_list.GetNext())
	{
		if(pStartLsCir==NULL)
		{
			pStartLsCir=pLsCir;
			axis_line.startPt.Set(pLsCir->centre.x,pLsCir->centre.y);
			axis_line.startPt.feature=pLsCir->feature;
			axis_line.endPt=axis_line.startPt;	//添加用于处理单螺栓情况板外形的创建
			axis_line.endPt.feature=pLsCir->feature;
			end_space=start_space=ftoi(pLsCir->radius);
		}
		else
		{
			if(bHasTwoLs)
			{
				if(bOneAxisLineLs)
				{
					f3dPoint centre;
					centre.Set(pLsCir->centre.x,pLsCir->centre.y);
					int ret=axis_line.PtInLine(centre);	//螺栓不在螺栓射线上
					if(ret==-2)			//起点外侧
					{
						axis_line.startPt=centre;
						axis_line.startPt.feature=pLsCir->feature;
						start_space=ftoi(pLsCir->radius);
					}
					else if(ret==-1)	//终点外侧
					{
						axis_line.endPt=centre;
						axis_line.endPt.feature=pLsCir->feature;
						end_space=ftoi(pLsCir->radius);
					}
					if(ret==0)
						bOneAxisLineLs=FALSE;
				}
			}
			else
			{
				bHasTwoLs=TRUE;
				axis_line.endPt.Set(pLsCir->centre.x,pLsCir->centre.y);
				axis_line.endPt.feature=pLsCir->feature;
				end_space=ftoi(pLsCir->radius);
			}
			if(pStartLsCir->centre.y>pLsCir->centre.y)
				pStartLsCir=pLsCir;		//查获最低处的螺栓
		}
	}
	if(bOneAxisLineLs)
	{	//所有螺栓都在同一直线上
		f3dPoint axis_vec=axis_line.endPt-axis_line.startPt;
		if(normalize(axis_vec)==0)
			axis_vec.Set(1);
		f3dPoint axis_norm=f3dPoint(0,0,1)^axis_vec;
		normalize(axis_norm);
		pPlate->vertex_list.Empty();
		f3dPoint pt;
		pt=axis_line.startPt-start_space*axis_vec+axis_norm*start_space;
		pt.feature=axis_line.startPt.feature;
		pPlate->vertex_list.Append(PROFILE_VER(pt));
		pt=axis_line.startPt-start_space*axis_vec-axis_norm*start_space;
		pt.feature=axis_line.startPt.feature;
		pPlate->vertex_list.Append(PROFILE_VER(pt));
		pt=axis_line.endPt+end_space*axis_vec-axis_norm*end_space;
		pt.feature=axis_line.endPt.feature;
		pPlate->vertex_list.Append(PROFILE_VER(pt));
		pt=axis_line.endPt+end_space*axis_vec+axis_norm*end_space;
		pt.feature=axis_line.endPt.feature;
		pPlate->vertex_list.Append(PROFILE_VER(pt));
	}
	else
	{	//至少有三个非共线螺栓
		out_ls_list.append(*pStartLsCir);
		pFirstStartLsCir=pStartLsCir;
		//逆时针依次查找其余外轮廓特征螺栓
		int nLoops=0;
		f2dLine tan_line;
		for(;;)
		{
			if(nLoops==ls_cir_list.GetNodeNum())
				break;
			nLoops++;
			for(pEndLsCir=ls_cir_list.GetFirst();pEndLsCir;pEndLsCir=ls_cir_list.GetNext())
			{
				if(pEndLsCir==pStartLsCir)
					continue;
				BOOL bOutLs=TRUE;
				ls_cir_list.push_stack();
				//由点在线左侧算法改为直接求包络公切线夹角算法,这样
				//可以避免圆直径不同的影响 WJH-2004.09.22
				WrapLine2dcc(*pStartLsCir,*pEndLsCir,tan_line);
				for(pLsCir=ls_cir_list.GetFirst();pLsCir;pLsCir=ls_cir_list.GetNext())
				{
					if(pLsCir==pStartLsCir||pLsCir==pEndLsCir)
						continue;
					//由点在线左侧算法改为直接求包络公切线夹角算法,这样
					//可以避免圆直径不同的影响 WJH-2004.09.22
					//double dd=DistOf2dPtLine(pLsCir->centre,pStartLsCir->centre,pEndLsCir->centre);
					//if(dd<-eps)	//终端螺栓不是边缘螺栓
					double dd=DistOf2dPtLine(pLsCir->centre,tan_line.startPt,tan_line.endPt);
					if(dd<pLsCir->radius-eps)	//终端螺栓不是边缘螺栓
					{
						bOutLs=FALSE;
						break;
					}
					else if(dd>pLsCir->radius-eps&&dd<pLsCir->radius+eps)
					{	//清除掉共线螺栓
						f2dPoint perp;
						SnapPerp(&perp,tan_line,pLsCir->centre);
						f3dPoint line_vec(tan_line.endPt.x-tan_line.startPt.x,tan_line.endPt.y-tan_line.startPt.y);
						f3dPoint vec1(pLsCir->centre.x-tan_line.startPt.x,pLsCir->centre.y-tan_line.startPt.y);
						f3dPoint vec2(pLsCir->centre.x-tan_line.endPt.x,pLsCir->centre.y-tan_line.endPt.y);
						if(line_vec*vec1>0&&line_vec*vec2>0)
						{	//pLsCir为pEndLsCir的下一个共线螺栓,所以pEndLsCir不为外轮廓特征螺栓
							bOutLs=FALSE;
							break;
						}
					}
				}
				ls_cir_list.pop_stack();
				if(bOutLs)		//终端螺栓为边缘螺栓
					break;
			}
			if(pEndLsCir==pFirstStartLsCir)	//末螺栓为第一颗外轮廓螺栓时终止(证明已找了一周)
				break;
			else if(pEndLsCir)				//找到下一颗外轮廓螺栓
			{
				out_ls_list.append(*pEndLsCir);
				pStartLsCir=pEndLsCir;
			}
			else							//当前螺栓不是下一颗外轮廓螺栓
				continue;
		}

		//根据圆公切线,自动设计板外形
		//清空已有设计
		pPlate->vertex_list.Empty();
		f3dPoint start,end,vec,out_vec,pick_start,pick_end;
		//计算公切线组成的切线边列表
		ATOM_LIST<f2dLine>edge_line_list;
		for(pStartLsCir=out_ls_list.GetFirst();pStartLsCir;pStartLsCir=out_ls_list.GetNext())
		{
			pEndLsCir=out_ls_list.GetNext();
			if(pEndLsCir==NULL)
			{
				pEndLsCir=out_ls_list.GetFirst();
				out_ls_list.GetTail();
			}
			else
				out_ls_list.GetPrev();	//复原位置
			start.Set(pStartLsCir->centre.x,pStartLsCir->centre.y);
			end.Set(pEndLsCir->centre.x,pEndLsCir->centre.y);
			vec=end-start;
			out_vec=vec^f3dPoint(0,0,1);
			normalize(out_vec);
			pick_start=start+out_vec*20;
			pick_end=end+out_vec*20;
			if(TangLine2dcc(*pStartLsCir,pick_start,*pEndLsCir,pick_end,tan_line)>0)	//切线计算成功
			{
				tan_line.startPt.feature=ftoi(pStartLsCir->radius);
				tan_line.startPt.pen.style=pStartLsCir->feature;	//利用图层名的第一个字母表示所在的面
				tan_line.endPt.feature=ftoi(pEndLsCir->radius);
				tan_line.endPt.pen.style=pEndLsCir->feature;		//利用图层名的第一个字母表示所在的面
				edge_line_list.append(tan_line);
			}
		}
		//根据公切线计算板外形
		f2dLine *pLine=NULL,*pNextLine=NULL;
		for(pLine=edge_line_list.GetFirst();pLine;pLine=edge_line_list.GetNext())
		{
			pNextLine=edge_line_list.GetNext();
			if(pNextLine==NULL)
			{
				pNextLine=edge_line_list.GetFirst();
				edge_line_list.GetTail();
			}
			else
				edge_line_list.GetPrev();	//复原位置
			f3dPoint vec_prev(pLine->startPt.x-pLine->endPt.x,pLine->startPt.y-pLine->endPt.y);
			f3dPoint vec_next(pNextLine->endPt.x-pNextLine->startPt.x,pNextLine->endPt.y-pNextLine->startPt.y);
			normalize(vec_prev);
			normalize(vec_next);
			double fai=cal_angle_of_2vec(vec_prev,vec_next);
			f3dPoint inters;
			if(Int2dpl(*pLine,*pNextLine,inters.x,inters.y)>0)
			{
				if(fai>=Pi*angle_lim/180)
				{
					inters.feature=pLine->endPt.pen.style;
					pPlate->vertex_list.Append(PROFILE_VER(inters));
				}
				else
				{
					f3dPoint prev,next;
					double radius=pLine->endPt.feature;
					double cut_len=radius*2*(1-sin(fai/2))/sin(fai);
					prev=inters+vec_prev*cut_len;
					next=inters+vec_next*cut_len;
					prev.feature=next.feature=pLine->endPt.pen.style;
					pPlate->vertex_list.Append(PROFILE_VER(prev));
					pPlate->vertex_list.Append(PROFILE_VER(next));
				}
			}
		}
	}
	f3dLine line;
	f3dPoint last,now,next,inters;
	int n=pPlate->vertex_list.GetNodeNum();
	for(int j=0;j<n;j++)
	{
		last = pPlate->vertex_list[(n+j-1)%n].vertex;
		now  = pPlate->vertex_list[j].vertex;
		next = pPlate->vertex_list[(j+1)%n].vertex;
		line.startPt = now;
		line.endPt = next;
		line.startPt.z = line.endPt.z = 0;
		if(now.feature+next.feature==3)
		{
			if(Int3dpl(line,pPlate->HuoQuLine[0],inters)<=0)
				continue;	//无交点
			else
			{
				int ret=line.PtInLine(inters);
				if(ret==2)
				{	//火曲点在中间，需要新生成火曲点
					inters.feature = 12;
					pPlate->vertex_list.insert(PROFILE_VER(inters),j+1);
					j++;
					n++;
				}
				else// if(ret==1)
				{	//端点即为火曲点
					if(line.startPt==inters)
						pPlate->vertex_list[j].vertex.feature=12;
					else //if(line.endPt==inters)
						pPlate->vertex_list[(j+1)%n].vertex.feature=12;
				}
			}
		}
	}
}
void CPEC::ExecuteCommand(WORD taskType)
{
	m_wCurCmdType=taskType;
	switch (m_wCurCmdType)
	{
	case CMD_OVERTURN_PART:
		OverturnPart();
		break;
	case CMD_ROTATEANTICLOCKWISE_PLATE:
		RotateAntiClockwisePlate();
		break;
	case CMD_ROTATECLOCKWISE_PLATE:
		RotateClockwisePlate();
		break;
	case CMD_NWE_BOLT_HOLE:
		NewBoltHole();
		break;
	case CMD_CAL_PLATE_PROFILE:
		CalPlateWrapProfile();
		break;
	case CMD_SPEC_WCS_ORIGIN:
	case CMD_SPEC_WCS_AXIS_X:
		SetSpecWCS();
		break;
	case CMD_VIEW_PART_FEATURE:
		ViewPartFeature();
		break;
	case CMD_DIM_SIZE:
		DimSize();
		break;
	case CMD_DEF_PLATE_CUT_IN_PT:
		DefPlateCutInPt();
		break;
	case CMD_DRAW_CUTTING_TRACK:
		DrawCuttingTrack();
		break;
	default:
		break;
	}
}
void CPEC::DimSize()
{
	CProcessPartDraw *pPartDraw=m_hashPartDrawBySerial.GetFirst();
	if(pPartDraw)
		pPartDraw->DimSize(m_p2dDraw,m_pSolidSnap);
}
void CPEC::SetProcessCardPath(const char* sCardDxfFilePath,BYTE angle0_plate1/* =0 */)
{
	if(sCardDxfFilePath==NULL)
		return;
	if(angle0_plate1==0)
		strcpy(CProcessAngleDraw::m_sAngleProcessCardPath,sCardDxfFilePath);
	else if(angle0_plate1==1)
		strcpy(CProcessPlateDraw::m_sPlateProcessCardPath,sCardDxfFilePath);
}
void CPEC::UpdateJgDrawingPara(const char* para_buf,int buf_len)
{
	CBuffer buffer;
	buffer.Write(para_buf,buf_len);
	buffer.SeekToBegin();
	buffer.ReadDouble(&CRodDrawing::drawPara.fDimTextSize);			//长度尺寸标注文本高
	buffer.ReadInteger(&CRodDrawing::drawPara.iDimPrecision);		//尺寸精确度
	buffer.ReadDouble(&CRodDrawing::drawPara.fRealToDraw);			//基础绘图比例尺＝实际尺寸/绘图尺寸，如1:20时，fRealToDraw=20
	buffer.ReadDouble(&CRodDrawing::drawPara.fDimArrowSize);		//尺寸标注箭头长
	buffer.ReadDouble(&CRodDrawing::drawPara.fTextXFactor);
	buffer.ReadDouble(&CRodDrawing::drawPara.fPartNoTextSize);		//构件编号文字高
	buffer.ReadInteger(&CRodDrawing::drawPara.iPartNoFrameStyle);	//编号框类型 //0.圆圈 1.带圆弧的矩形框 2.矩形框	3.自动判断
	buffer.ReadDouble(&CRodDrawing::drawPara.fPartNoMargin);		//构件编号与编号框之间的间隙值 
	buffer.ReadDouble(&CRodDrawing::drawPara.fPartNoCirD);			//构件编号圈直径
	buffer.ReadDouble(&CRodDrawing::drawPara.fPartGuigeTextSize);	//构件规格文字高
	buffer.ReadInteger(&CRodDrawing::drawPara.iMatCharPosType);
	buffer.ReadInteger(&CRodDrawing::drawPara.bModulateLongJg);		//调整角钢长度 暂未使用，由iJgZoomSchema代替该变量 wht 11-05-07
	buffer.ReadInteger(&CRodDrawing::drawPara.iJgZoomSchema);		//角钢绘制方案，0.1:1绘制 1.使用构件图比例 2.长宽同比缩放 3.长宽分别缩放
	buffer.ReadInteger(&CRodDrawing::drawPara.bMaxExtendAngleLength);//最大限度延伸角钢绘制长度
	//buffer.ReadDouble(&CRodDrawing::drawPara fLsDistThreshold);		//角钢长度自动调整螺栓间距阈值(大于此间距时就要进行调整);
	//buffer.ReadDouble(&CRodDrawing::drawPara.fLsDistZoomCoef);		//螺栓间距缩放系数
	buffer.ReadInteger(&CRodDrawing::drawPara.bOneCardMultiPart);	//角钢允许一卡多件情况
	buffer.ReadInteger(&CRodDrawing::drawPara.iJgGDimStyle);		//0.始端标注  1.中间标注 2.自动判断
	buffer.ReadInteger(&CRodDrawing::drawPara.nMaxBoltNumStartDimG);//集中在始端标注准距支持的最大螺栓数
	buffer.ReadInteger(&CRodDrawing::drawPara.iLsSpaceDimStyle);	//0.X轴方向	  1.Y轴方向  2.自动判断 3.不标注  4.无标注内容(X轴方向)  4.仅标尺寸线，无标注内容(X轴方向)主要用于江津(北)塔厂质检是盲检填数
	buffer.ReadInteger(&CRodDrawing::drawPara.nMaxBoltNumAlongX);	//沿X轴方向标注支持的最大螺栓个数
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimCutAngle);		//标注角钢切角
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimCutAngleMap);		//标注角钢切角示意图
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimPushFlatMap);		//标注压扁示意图
	buffer.ReadInteger(&CRodDrawing::drawPara.bJgUseSimpleLsMap);	//角钢使用简化螺栓图符
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimLsAbsoluteDist);	//标注螺栓绝对尺寸
	buffer.ReadInteger(&CRodDrawing::drawPara.bMergeLsAbsoluteDist);//合并相邻等距螺栓绝对尺寸 江津及增立提出:有时也需要标 wjh-2014.6.9
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimRibPlatePartNo);	//标注角钢肋板编号
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimRibPlateSetUpPos);//标注角钢肋板安装位置
	buffer.ReadInteger(&CRodDrawing::drawPara.iCutAngleDimType);	//切角标注样式 0.样式一  1.样式二 wht 10-11-01
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiHe);			//标注角钢开合角
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiheAngleMap);	//标注角钢开合角示意图
	buffer.ReadDouble(&CRodDrawing::drawPara.fKaiHeJiaoThreshold);	//开合角标注阈值(°) wht 11-05-06
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiheSumLen);		//标注开合区域总长
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiheAngle);		//标注开合度数	
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiheSegLen);		//标注开合区域分段长
	buffer.ReadInteger(&CRodDrawing::drawPara.bDimKaiheScopeMap);	//标注开合区域标识符
}
void CPEC::SetSpecWCS()
{	//编辑模式支持设置工作坐标系
	WORD wDrawMode=GetDrawMode();
	if(wDrawMode!=IPEC::DRAW_MODE_EDIT)
		return;
	IDrawing* pDrawing=m_p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	m_pSolidSnap->GetLastSelectEnts(id_arr);
	IDbEntity* pEnt=pDrawing->Database()->GetDbEntity(id_arr[0]);
	if(pEnt==NULL)
		return;
	GEPOINT spec_point;
	if(pEnt->GetDbEntType()==IDbEntity::DbPoint)
		spec_point=((IDbPoint*)pEnt)->Position();
	else if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
		spec_point=((IDbCircle*)pEnt)->Center();
	else
		return;
	if(m_wCurCmdType==CMD_SPEC_WCS_ORIGIN)
		m_workCS.origin=spec_point;
	else if(m_wCurCmdType==CMD_SPEC_WCS_AXIS_X)
	{
		m_workCS.axis_x = spec_point-m_workCS.origin;
		normalize(m_workCS.axis_x);
		m_workCS.axis_z.Set(0,0,1);
		m_workCS.axis_y=m_workCS.axis_z^m_workCS.axis_x;
	}
	m_pSolidDraw->AddCS(ISolidDraw::WORK_CS,m_workCS);
	m_pSolidDraw->BuildDisplayList();
}
//构件属性(点、线、圆、螺栓孔)
void CPEC::ViewPartFeature()
{	//构件模式下查看构件特性
	WORD wDrawMode=GetDrawMode();
	if(wDrawMode!=IPEC::DRAW_MODE_EDIT)
		return;
	IDrawing* pDrawing=m_p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	m_pSolidSnap->GetLastSelectEnts(id_arr);
	IDbEntity* pEnt=pDrawing->Database()->GetDbEntity(id_arr[0]);
	if(pEnt==NULL)
		return;
	if(pEnt->GetDbEntType()==IDbEntity::DbPoint)
		return FinishViewVertex(pEnt);
	else if(pEnt->GetDbEntType()==IDbEntity::DbLine||
		pEnt->GetDbEntType()==IDbEntity::DbArcline)
		return FinishViewLine(pEnt);
	else if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
		return FinishViewCir(pEnt);
}
void CPEC::FinishViewVertex(IDbEntity* pEnt)
{
	GEPOINT vertex=((IDbPoint*)pEnt)->Position();
	f3dPoint old_vertex(vertex);
	coord_trans(vertex,m_workCS,FALSE);
	if(m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		C2DPtDlg dlg;
		dlg.m_bCanModify=FALSE;
		dlg.m_fPtX = vertex.x;
		dlg.m_fPtY = vertex.y;
		dlg.DoModal();
	}
	else if(m_cPartType==CProcessPart::TYPE_PLATE)
	{
		CProcessPlate *pPlate=(CProcessPlate*)m_hashProcessPartBySerial.GetFirst();
		CPlankVertexDlg dlg;
		dlg.m_fPosX = vertex.x;
		dlg.m_fPosY = vertex.y;
		PROFILE_VER *pVertex=NULL;
		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		{
			f3dPoint curVertex=pVertex->vertex;
			if(curVertex.x==old_vertex.x&&curVertex.y==old_vertex.y)
			{
				if(pVertex->vertex.feature==2)
					dlg.m_iVertexType = 1;
				else if(pVertex->vertex.feature==12)
					dlg.m_iVertexType = 2;
				else if(pVertex->vertex.feature==3)
					dlg.m_iVertexType = 3;
				else if(pVertex->vertex.feature ==13)
					dlg.m_iVertexType = 4;
				else
					dlg.m_iVertexType = 0;
				break;
			}
		}
		if(dlg.DoModal()!=IDOK)
			return;
		vertex.Set(dlg.m_fPosX,dlg.m_fPosY);
		coord_trans(vertex,m_workCS,TRUE);
		pVertex->vertex=vertex;
		if(dlg.m_iVertexType == 1)
			pVertex->vertex.feature=2;
		else if(dlg.m_iVertexType == 2)
			pVertex->vertex.feature=12;
		else if(dlg.m_iVertexType == 3)
			pVertex->vertex.feature=3;
		else if(dlg.m_iVertexType == 4)
			pVertex->vertex.feature =13;
		else
			pVertex->vertex.feature = 1;
		m_pSolidDraw->BuildDisplayList();
	}
}
void CPEC::FinishViewLine(IDbEntity* pEnt)
{
	
}
void CPEC::FinishViewCir(IDbEntity* pEnt)
{
	CProcessPart *pPart=m_hashProcessPartBySerial.GetFirst();
	if(pPart==NULL)
		return;
	BOLT_INFO *pHole=pPart->FromHoleHiberId(pEnt->GetHiberId());
	if(pHole)
		return FinishViewLsCir(pEnt);
	else 
		return FinishViewLine(pEnt);
}
void CPEC::FinishViewLsCir(IDbEntity* pEnt)
{
	CProcessPart* pProcessPart=m_hashProcessPartBySerial.GetFirst();
	if(pProcessPart==NULL)
		return;
	BOLT_INFO* pBoltInfo=pProcessPart->FromHoleHiberId(pEnt->GetHiberId());
	if(pBoltInfo==NULL)
		return;
	CBoltHolePropDlg dlg;
	dlg.m_pWorkPart=pProcessPart;
	dlg.m_pLs=pBoltInfo;
	dlg.work_ucs=m_workCS;
	f3dPoint ls_init_pos(pBoltInfo->posX,pBoltInfo->posY);
	coord_trans(ls_init_pos,m_workCS,FALSE);
	if(m_wDrawMode==DRAW_MODE_EDIT)
		dlg.m_bCanModifyPos=TRUE;
	dlg.m_fPosX = ls_init_pos.x;
	dlg.m_fPosY = ls_init_pos.y;
	dlg.m_fPosZ = ls_init_pos.z;
	dlg.m_fHoleD =pBoltInfo->bolt_d+pBoltInfo->hole_d_increment;
	dlg.m_hLs.Format("0X%X",dlg.m_pLs->hiberId.HiberUpId(1));
	dlg.m_sLsGuiGe.Format("M%d",dlg.m_pLs->bolt_d);
	dlg.m_nWaistLen = dlg.m_pLs->waistLen;
	dlg.m_dwRayNo	= dlg.m_pLs->dwRayNo;
	dlg.m_fNormX=0;
	dlg.m_fNormY=0;
	dlg.m_fNormZ=1;
	//腰圆孔方向
	dlg.waist_vec=pBoltInfo->waistVec;
	if(dlg.DoModal()!=IDOK)
		return;
	if(m_wDrawMode==DRAW_MODE_EDIT)
	{
		dlg.m_pLs->waistLen = dlg.m_nWaistLen;
		if(dlg.m_pLs->waistLen>0)	//更新腰圆孔方向
			dlg.m_pLs->waistVec=dlg.waist_vec;
		if(dlg.m_bCanModifyPos)
		{
			f3dPoint pos(dlg.m_fPosX,dlg.m_fPosY);
			coord_trans(pos,m_workCS,TRUE);
			dlg.m_pLs->posX=(float)pos.x;
			dlg.m_pLs->posY=(float)pos.y;
		}
		restore_Ls_guige(dlg.m_sLsGuiGe,*dlg.m_pLs);
		dlg.m_pLs->hole_d_increment = (float)(dlg.m_fHoleD-dlg.m_pLs->bolt_d);
		dlg.m_pLs->dwRayNo			= dlg.m_dwRayNo;
		m_pSolidDraw->BuildDisplayList(this);
	}
}
////插入、删除轮廓点,新建、删除螺栓
//BOOL CPEC::FinishInsertPlankVertex(fAtom *pAtom)
//{
//	static CDefProfileVertexDlg vertexdlg;
//	f3dPoint insert_vertex;
//	CProcessPart *pPart=m_hashProcessPartBySerial.GetFirst();
//	if(pPart==NULL||pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
//	{
//		AfxMessageBox("只能在编辑板时添加轮廓顶点!");
//		return FALSE;
//	}
//	if(pAtom->feature==2)
//	{
//		AfxMessageBox("当前所选直线为火曲线,火曲线上不能添加轮廓点!");
//		return FALSE;
//	}
//	
//	f3dLine *pLine=NULL;
//	f3dArcLine *pArcLine=NULL;
//	f3dPoint lineStart,lineEnd;
//	if(pAtom->atom_type==AtomType::prLine)
//	{
//		pLine = (f3dLine*)pAtom;
//		//SetSectorAngle(0)必须在SetStart之前，否则如果datum_line为弧时会出错 wjh-2013.11.04
//		vertexdlg.datum_line.SetSectorAngle(0);
//		vertexdlg.datum_line.SetStart(pLine->startPt);
//		vertexdlg.datum_line.SetEnd(pLine->endPt);
//		lineStart = pLine->startPt;
//		lineEnd = pLine->endPt;
//	}
//	else if(pAtom->atom_type==AtomType::prArc)
//	{
//		pArcLine = (f3dArcLine*)pAtom;
//		vertexdlg.datum_line=*pArcLine;
//		lineStart = pArcLine->Start();
//		lineEnd = pArcLine->End();
//	}
//	f3dPoint start = vertexdlg.datum_line.Start();
//	f3dPoint end = vertexdlg.datum_line.End();
//	f3dPoint norm = vertexdlg.datum_line.WorkNorm();
//	double angle = vertexdlg.datum_line.SectorAngle();
//	coord_trans(start,m_workCS,FALSE);
//	coord_trans(end,m_workCS,FALSE);
//	vector_trans(norm,m_workCS,FALSE);
//	vertexdlg.datum_line.CreateMethod2(start,end,norm,angle);
//	CPoint point;
//	GetCursorPos(&point);
//	ScreenToClient(m_hWnd,&point);
//	f3dPoint port_pt(point.x,point.y);
//	m_pSolidOper->ScreenToUser(&vertexdlg.pickPoint,port_pt);
//	coord_trans(vertexdlg.pickPoint,m_workCS,FALSE);
//	if(vertexdlg.DoModal()==IDOK)
//	{
//		insert_vertex.Set(vertexdlg.m_fPosX,vertexdlg.m_fPosY);
//		coord_trans(insert_vertex,m_workCS,TRUE);
//	}
//	else
//		return FALSE;
//	CProcessPlate *pPlate=(CProcessPlate*)pPart;
//	int i=0;
//	PROFILE_VER *pVertex=NULL;
//	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
//	{
//		if(pVertex->vertex.x==lineStart.x&&pVertex->vertex.y==lineStart.y)
//		{
//			lineStart.feature = pVertex->vertex.feature;
//			break;
//		}
//	}
//		
//	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
//	{
//		if(pPlate->m_cFaceN==3
//			&&((pPlate->top_point.x==lineEnd.x&&pPlate->top_point.x==lineEnd.x)
//			&&(pVertex->vertex.x==lineStart.x&&pVertex->vertex.y==lineStart.y)))
//		{
//			PROFILE_VER *pTmVertex=pPlate->vertex_list.insert(PROFILE_VER(insert_vertex.x,insert_vertex.y,pVertex->vertex.z),i+1);
//			if(pVertex->vertex.feature<10)
//				pTmVertex->vertex.feature = pVertex->vertex.feature;
//			else
//				pTmVertex->vertex.feature = 1;
//			break;
//		}
//		else
//		{
//			if(pArcLine&&fabs(pVertex->vertex.x-lineStart.x)+fabs(pVertex->vertex.y-lineStart.y)<EPS)
//			{
//				f3dArcLine arc = *pArcLine;
//				arc.SetEnd(insert_vertex);
//				if(arc.WorkNorm().z>0)
//					pVertex->sector_angle=arc.SectorAngle();
//				else
//					pVertex->sector_angle=-arc.SectorAngle();
//			}
//			if(fabs(pVertex->vertex.x-lineEnd.x)+fabs(pVertex->vertex.y-lineEnd.y)<EPS)
//			{
//				PROFILE_VER *pTmVertex=pPlate->vertex_list.insert(PROFILE_VER(insert_vertex.x,insert_vertex.y,pVertex->vertex.z),i);
//				
//				//附加顶点所在面号
//				if(pVertex->vertex.feature<10)
//					pTmVertex->vertex.feature = pVertex->vertex.feature;
//				else if(lineStart.feature<10)
//					pTmVertex->vertex.feature = lineStart.feature;
//				else
//					pTmVertex->vertex.feature = 1;
//				if(pArcLine)
//				{
//					f3dArcLine arc = *pArcLine;
//					arc.SetStart(insert_vertex);
//					double sector_angle=arc.SectorAngle();
//					pTmVertex->type=1;
//					//pPoint->layer[1]='0';
//					if(arc.WorkNorm().z>0)
//						pTmVertex->sector_angle=sector_angle;
//					else
//						pTmVertex->sector_angle=-sector_angle;
//				}
//				break;
//			}
//		}
//		i++;
//	}
//	if(pPlate->vertex_list.GetNodeNum()<3)
//	{
//		PROFILE_VER *pNewVertex=pPlate->vertex_list.Add(PROFILE_VER(insert_vertex.x,insert_vertex.y,0));
//		pNewVertex->vertex.feature=1;
//	}
//	m_pSolidDraw->BuildDisplayList();
//	if(FirePartModify)
//		FirePartModify(m_wCurCmdType);
//	return TRUE;
//}
//BOOL CPEC::FinishDelPartFeature(fAtom *pFeatAtom)
//{
//	CProcessPart *pPart=m_hashProcessPartBySerial.GetFirst();
//	if(pPart==NULL)
//		return FALSE;
//	BOOL bRetCode=FALSE;
//	if(pFeatAtom->atom_type==AtomType::prPoint)
//	{
//		if(pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
//		{
//			AfxMessageBox("仅支持删除钢板上的轮廓点!");
//			return FALSE;
//		}
//		CProcessPlate* pPlate=(CProcessPlate*)pPart;
//		f3dPoint vertex = *((f3dPoint*)pFeatAtom);
//		PROFILE_VER *pVertex=NULL;
//		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
//		{
//			f3dPoint curVertex=pVertex->vertex;
//			if((vertex.ID>0&&vertex.ID==curVertex.ID)||curVertex.IsEqual(vertex))
//			{
//				if(curVertex.feature>10)
//				{
//					AfxMessageBox("火曲点不能删除!");
//					return FALSE;
//				}
//				pPlate->vertex_list.DeleteCursor(TRUE);
//				bRetCode=TRUE;
//				break;
//			}
//		}
//		if(!bRetCode)
//			return FALSE;
//	}
//	else if(pFeatAtom->atom_type==AtomType::prCircle)
//	{
//		f3dCircle *pCircle=(f3dCircle*)pFeatAtom;
//		bRetCode=pPart->DeleteBoltHoleById(pCircle->ID);
//	}
//	else if(pFeatAtom->atom_type==AtomType::prLine&&pPart->m_cPartType==CProcessPart::TYPE_PLATE)
//	{
//		f3dLine *pLine=(f3dLine*)pFeatAtom;
//		if(pLine->feature!=2)	//不是火曲线
//			return FALSE;
//		PROFILE_VER *pVertex=NULL;
//		CProcessPlate *pPlate=(CProcessPlate*)pPart;
//		int del_huoqu_face_i=0;
//		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
//		{
//			if(pVertex->vertex==pLine->startPt||pVertex->vertex==pLine->endPt)
//			{
//				if(pVertex->vertex.feature>10)
//				{
//					del_huoqu_face_i=__max(pVertex->vertex.feature/10,pVertex->vertex.feature%10);
//					break;
//				}
//			}
//		}
//		pPlate->m_cFaceN--;
//		for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
//		{
//			int face_i=__max(pVertex->vertex.feature/10,pVertex->vertex.feature%10);
//			if(face_i==del_huoqu_face_i)
//				pVertex->vertex.feature=1;
//			else if(pVertex->vertex.feature>10&&face_i>1)	//另一火曲线
//				pVertex->vertex.feature=12;
//			else if(face_i>1)	//另一面板
//				pVertex->vertex.feature=2;
//		}
//		if(del_huoqu_face_i==2)
//		{
//			pPlate->HuoQuFaceNorm[0]=pPlate->HuoQuFaceNorm[1];
//			pPlate->HuoQuFaceNorm[1].Set();
//			pPlate->HuoQuLine[0]=pPlate->HuoQuLine[1];
//		}
//		m_pSolidDraw->BuildDisplayList();
//		MessageBox(m_hWnd,"成功删除火曲面!","提示",MB_OK);
//		bRetCode=TRUE;
//	}
//	else
//		return FALSE;
//	if(bRetCode&&FirePartModify)
//		FirePartModify(m_wCurCmdType);
//	return bRetCode;
//}
BOOL CPEC::GetSysParaFromReg(const char* sEntry,char* sValue)
{
	char sStr[MAX_PATH];
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	DWORD dwDataType,dwLength=MAX_PATH;
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_READ,&hKey)==ERROR_SUCCESS&&hKey)
	{
		if(RegQueryValueEx(hKey,sEntry,NULL,&dwDataType,(BYTE*)&sStr[0],&dwLength)!= ERROR_SUCCESS)
			return FALSE;
		RegCloseKey(hKey);
	}
	else 
		return FALSE;
	if(sValue && strlen(sStr)>0)
		strcpy(sValue,sStr);
	return TRUE;
}
void CPEC::DefPlateCutInPt()
{
	IDrawing* pDrawing=m_p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	CProcessPart *pPart=m_hashProcessPartBySerial.GetFirst();
	if(pPart==NULL||!pPart->IsPlate())
		return;
	CProcessPlate *pPlate=(CProcessPlate*)pPart;
	long* id_arr=NULL;
	m_pSolidSnap->GetLastSelectEnts(id_arr);
	IDbEntity* pEnt=pDrawing->Database()->GetDbEntity(id_arr[0]);
	if(pEnt==NULL)
		return;
	if(pEnt->GetDbEntType()==IDbEntity::DbPoint)
	{
		PROFILE_VER *pVertex=pPlate->FromVertexHiberId(pEnt->GetHiberId());
		pPlate->m_xCutPt.hEntId=pVertex->keyId;
		pPlate->m_xCutPt.fInAngle=pPlate->m_xCutPt.fOutAngle=0;
	}
	else if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
	{

	}
	Draw();
}
void CPEC::DrawCuttingTrack()
{
	if(m_cPartType!=CProcessPart::TYPE_PLATE&&GetDrawMode()==IPEC::DRAW_MODE_NC)
		return;
	CProcessPlateDraw *pPlateDraw=(CProcessPlateDraw*)m_hashPartDrawBySerial.GetFirst();
	if(pPlateDraw)
		pPlateDraw->DrawCuttingTrack(m_p2dDraw,m_pSolidSet);
}