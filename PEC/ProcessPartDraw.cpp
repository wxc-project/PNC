#include "StdAfx.h"
#include "ProcessPartDraw.h"
#include "I_DrawSolid.h"
#include "DxfFile.h"
#include "PartLib.h"
#include "JgDrawing.h"
#include "PEC.h"
#ifndef _LEGACY_LICENSE
#include "XhLdsLm.h"
#else
#include "Lic.h"
#endif
#include "IXeroCad.h"
#include "DrawingToolKits.h"
#include "DXFDef.h"
#include "DrawingDef.h"
#include "CadLib.h"
#include "DefCard.h"
#include "NcPlate.h"
#include "Expression.h"
#include "LocalDrawing.h"

//////////////////////////////////////////////////////////////////////////
// CProcessPartDraw
BOOL CProcessPartDraw::m_bDispBoltOrder=FALSE;
char CProcessPlateDraw::m_sPlateProcessCardPath[MAX_PATH]={0};
char CProcessAngleDraw::m_sAngleProcessCardPath[MAX_PATH]={0};

CXhChar16 LocalQuerySteelMark(char cMat)
{
	CXhChar16 sMat;
	QuerySteelMatMark(cMat, sMat);
	if (cMat == 'h'&&sMat.GetLength()<=0)
		sMat.Copy("Q355");	//用小写h表示Q355,输出简化材质字符时需要转大写 wht 19-11-05
	return sMat;
}

CProcessPartDraw::CProcessPartDraw(void)
{
	m_pPart=NULL;
	m_pBelongEditor=NULL;
	m_bMainPartDraw=FALSE;
	m_bOverturn=FALSE;
}
CProcessPartDraw::CProcessPartDraw(IPEC* pEditor,CProcessPart* pPart)
{
	m_pPart=pPart;
	m_pBelongEditor=pEditor;
}
CProcessPartDraw::~CProcessPartDraw(void)
{
}
void CProcessPartDraw::InitSolidDrawUCS(ISolidSet *pSolidSet)
{
	UCS_STRU std_ucs;
	LoadDefaultUCS(&std_ucs);
	pSolidSet->SetObjectUcs(std_ucs);
	pSolidSet->SetPen(0,PS_SOLID,1);
}
void CProcessPartDraw::Draw(I2dDrawing *p2dDraw,ISolidSet *pSolidSet)
{
	if (m_pBelongEditor == NULL)
	{
		//logerr.Log("Editor=NULL");
		return;
	}
	InitSolidDrawUCS(pSolidSet);
	IDrawingAssembly *pDrawAssembly=p2dDraw->GetDrawingAssembly();
	WORD wDrawMode=m_pBelongEditor->GetDrawMode();
	IDrawing *pDrawing=pDrawAssembly->EnumFirstDrawing();
	if(wDrawMode==IPEC::DRAW_MODE_NC)
		pDrawing=pDrawAssembly->EnumNextDrawing();
	if(wDrawMode==IPEC::DRAW_MODE_PROCESSCARD)
		pDrawing=pDrawAssembly->EnumNextDrawing();
	if(pDrawing==NULL)
	{
		//logerr.Log("未找到绘制模式对应的图纸！");
		return;
	}
	p2dDraw->SetActiveDrawing(pDrawing->GetId());
	if(wDrawMode==IPEC::DRAW_MODE_NC)
		NcModelDraw(pDrawing,pSolidSet);
	else if(wDrawMode==IPEC::DRAW_MODE_EDIT)
		EditorModelDraw(pDrawing,pSolidSet);
	else if(wDrawMode==IPEC::DRAW_MODE_PROCESSCARD)
		ProcessCardModelDraw(pDrawing,pSolidSet);
	//else
	//	logerr.Log("DrawMode=%d",wDrawMode);
	XeroDimStyleTable::InitDimStyle2(pDrawing->Database());
}
void CProcessPartDraw::DimSize(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap)
{
	IDrawing* pDrawing=p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	int n=pSolidSnap->GetLastSelectEnts(id_arr);
	if(n!=2)
		return;
	IDbEntity* pEnt1=pDrawing->Database()->GetDbEntity(id_arr[0]);
	IDbEntity* pEnt2=pDrawing->Database()->GetDbEntity(id_arr[1]);
	if(pEnt1==NULL || pEnt2==NULL)
		return;
	f3dPoint dimStartPos,dimEndPos,dimPos;
	if(pEnt1->GetDbEntType()==IDbEntity::DbPoint || pEnt1->GetDbEntType()==IDbEntity::DbCircle)
	{
		if(pEnt1->GetDbEntType()==IDbEntity::DbPoint)
			dimStartPos=((IDbPoint*)pEnt1)->Position();
		else if(pEnt1->GetDbEntType()==IDbEntity::DbCircle)
			dimStartPos=((IDbCircle*)pEnt1)->Center();
		//
		if(pEnt2->GetDbEntType()==IDbEntity::DbPoint)
			dimEndPos=((IDbPoint*)pEnt2)->Position();
		else if(pEnt2->GetDbEntType()==IDbEntity::DbCircle)
			dimEndPos=((IDbCircle*)pEnt2)->Center();
		else if(pEnt2->GetDbEntType()==IDbEntity::DbLine)
		{
			IDbLine* pLine=(IDbLine*)pEnt2;
			SnapPerp(&dimEndPos,pLine->StartPosition(),pLine->EndPosition(),dimStartPos);
		}
	}
	else if(pEnt1->GetDbEntType()==IDbEntity::DbLine)
	{
		IDbLine* pLine=(IDbLine*)pEnt1;
		if(pEnt2->GetDbEntType()==IDbEntity::DbPoint)
			dimStartPos=((IDbPoint*)pEnt2)->Position();
		else if(pEnt2->GetDbEntType()==IDbEntity::DbCircle)
			dimStartPos=((IDbCircle*)pEnt2)->Center();
		SnapPerp(&dimEndPos,pLine->StartPosition(),pLine->EndPosition(),dimStartPos);
	}
	if(dimStartPos.IsEqual(dimEndPos,EPS2))
		return;
	f3dPoint offet_vec,vec=dimEndPos-dimStartPos;
	normalize(vec);
	offet_vec.Set(-vec.y,vec.x,0);
	if(offet_vec.y<0)
		offet_vec*=-1;
	dimPos=(dimStartPos+dimEndPos)*0.5+offet_vec*10;
	GEPOINT dimVec1=dimPos-dimStartPos,dimVec2=dimPos-dimEndPos;
	GEPOINT normal=dimVec1^dimVec2;
	if(normal.z<0)
	{
		f3dPoint tempPt=dimEndPos;
		dimEndPos=dimStartPos;
		dimStartPos=tempPt;
	}
	CXhChar16 sDimText("%.1f",DISTANCE(dimStartPos,dimEndPos));
	SimplifiedNumString(sDimText);
	long dimStyleId=XeroDimStyleTable::dimStyleHoleSpace.m_idDimStyle;
	AppendDbAlignedDim(pDrawing,dimStartPos,dimEndPos,dimPos,sDimText,dimStyleId,3,GEPOINT(0,0,-1),0,PS_SOLID,RGB(255,0,0));
}
//////////////////////////////////////////////////////////////////////////
// CProcessAngleDraw
CProcessAngleDraw::CProcessAngleDraw(void)
{
}

CProcessAngleDraw::~CProcessAngleDraw(void)
{
}

//标注构件尺寸 
//iOffsetType -1.X轴负方向 -2.Y轴负方向 1.X轴正方向 2.Y轴正方向
/*static void DimText(ISolidDraw* pSolidDraw,f3dPoint start,f3dPoint end,
	char* sDimText,int iOffsetType,int nOffsetDist=0,COLORREF color=RGB(0,0,0))
{
	SimplifiedNumString(sDimText);
	double fTextHeight=1.5,fTextLen=strlen(sDimText)*fTextHeight*0.7;
	int nSpaceDist=5;	//标注文字与轮廓边之间的间隙值
	f3dPoint dim_pos,dim_start,dim_end,mid_start,mid_end,ext_start,ext_end;
	dim_start=start; 
	dim_end=end;
	if(iOffsetType==0)
	{
		if(fabs(dim_start.x-dim_end.x)<EPS)
		{
			if(dim_start.y>0)
				iOffsetType=2;
			else 
				iOffsetType=-2;
		}
		else if(fabs(dim_end.y-dim_start.y)<EPS)
		{
			if(dim_start.x>0)
				iOffsetType=1;
			else 
				iOffsetType=-1;
		}
	}
	if(abs(iOffsetType)==1)
	{	//垂直标注
		if(nOffsetDist==0)	//标注位置相对于标注点的偏移距离
			nOffsetDist=ftoi(fTextLen*0.5)+4;
		dim_start.x+=(iOffsetType*nOffsetDist);
		dim_end.x+=(iOffsetType*nOffsetDist);
		//
		ext_start=dim_start;
		ext_end=dim_end;
		ext_start.x+=(iOffsetType*fTextHeight*0.25);
		ext_end.x+=(iOffsetType*fTextHeight*0.25);
		dim_pos.Set(dim_start.x,0.5*(dim_start.y+dim_end.y));
		//增加两个标注点空出文字空间
		int nFlag=1;
		if(dim_start.y<dim_end.y)
			nFlag=-1;
		mid_start.Set(dim_start.x,dim_pos.y+0.6*fTextHeight*nFlag);
		mid_end.Set(dim_start.x,dim_pos.y-0.6*fTextHeight*nFlag);
	}
	else if(abs(iOffsetType)==2)
	{	//水平标注
		iOffsetType/=2;	//除以2
		if(nOffsetDist==0)	//标注位置相对于标注点的偏移距离
			nOffsetDist=ftoi(fTextHeight*0.6);
		dim_start.y+=(iOffsetType*nOffsetDist);
		dim_end.y+=(iOffsetType*nOffsetDist);
		dim_pos.Set(0.5*(dim_start.x+dim_end.x),dim_start.y);
		//
		ext_start=dim_start;
		ext_end=dim_end;
		ext_start.y+=(iOffsetType*fTextHeight*0.25);
		ext_end.y+=(iOffsetType*fTextHeight*0.25);
		//增加两个标注点空出文字空间
		int nFlag=1;
		if(dim_start.x<dim_end.x)
			nFlag=-1;
		mid_start.Set(dim_pos.x+0.6*fTextLen*nFlag,dim_start.y);
		mid_end.Set(dim_pos.x-0.6*fTextLen*nFlag,dim_start.y);
		iOffsetType*=2;	//乘以2
	}
	//绘制标注线
	f3dLine line;
	line.pen.crColor=color;
	line.startPt=start;
	line.endPt=ext_start;
	pSolidDraw->NewLine(line);
	line.startPt=end;
	line.endPt=ext_end;
	pSolidDraw->NewLine(line);
	//
	f3dPoint dimPos(dim_pos.x-0.5*fTextLen,dim_pos.y-fTextHeight*0.5);
	int nDimSpace=(int)DISTANCE(f3dPoint(dim_start.x,dim_start.y,0),f3dPoint(dim_end.x,dim_end.y,0));
	if(abs(iOffsetType)==2&&nDimSpace<fTextLen)
	{
		line.startPt=dim_start;
		line.endPt=dim_end;
		pSolidDraw->NewLine(line);
		dimPos.x+=fTextLen;
	}
	else
	{
		line.startPt=dim_start;
		line.endPt=mid_start;
		pSolidDraw->NewLine(line);
		pSolidDraw->NewLine(dim_start,mid_start);
		line.startPt=mid_end;
		line.endPt=dim_end;
		pSolidDraw->NewLine(line);
	}
	//绘制标注内容	
	//pSolidDraw->NewText(sLen,dimPos,fTextHeight);
	pSolidDraw->NewText(sDimText,dimPos,fTextHeight,f3dPoint(1,0,0),f3dPoint(0,0,1),ISolidDraw::TEXT_ALIGN_BTMLEFT,0.01);
}
*/
void CProcessAngleDraw::NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	CProcessAngle *pAngle=(CProcessAngle*)m_pPart;
	f3dPoint dimPos(0,pAngle->m_fWidth*1.5,0);
	dimPos.pen.crColor=RGB(0,255,0);
	CXhChar100 sValue;
	double fTextHeight=30;
	if(CPEC::GetSysParaFromReg("TextHeight",sValue))
		fTextHeight=atof(sValue);	//普通文字字高
	if(m_bOverturn)
	{
		dimPos.Set(pAngle->GetLength(),pAngle->m_fWidth*1.5,0);
		AppendDbText(pDrawing,dimPos,"X肢",0,fTextHeight,IDbText::AlignMiddleLeft,HIBERID(pAngle->GetKey()),PS_SOLID,RGB(255,0,0));
		dimPos.Set(pAngle->GetLength(),-pAngle->m_fWidth*1.5,0);
		AppendDbText(pDrawing,dimPos,"Y肢",0,fTextHeight,IDbText::AlignMiddleLeft,HIBERID(pAngle->GetKey()),PS_SOLID,RGB(0,255,0));
	}
	else 
	{
		AppendDbText(pDrawing,dimPos,"Y肢",0,fTextHeight,IDbText::AlignMiddleLeft,HIBERID(pAngle->GetKey()),PS_SOLID,RGB(0,255,0));
		dimPos.y*=-1;
		AppendDbText(pDrawing,dimPos,"X肢",0,fTextHeight,IDbText::AlignMiddleLeft,HIBERID(pAngle->GetKey()),PS_SOLID,RGB(255,0,0));
	}
	if(IsMainPartDraw())
		EditorModelDraw(pDrawing,pSolidSet);
	else
		EditorModelDraw(pDrawing,pSolidSet,RGB(0,255,255),RGB(0,255,255));
}
void CProcessAngleDraw::EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet,
										COLORREF color/*=RGB(0,0,0)*/,COLORREF boltColor/*=RGB(255,0,0)*/)
{
	CProcessAngle *pAngle=(CProcessAngle*)m_pPart;
	HIBERID angleId(pAngle->GetKey());
	double len=pAngle->m_wLength;
	//基本轮廓线
	f3dLine edge_line0(GEPOINT(0,pAngle->m_fWidth,0),GEPOINT(len,pAngle->m_fWidth,0));
	f3dLine edge_line1(GEPOINT(0,pAngle->m_fThick,0),GEPOINT(len,pAngle->m_fThick,0));
	f3dLine edge_line2(GEPOINT(0,0,0),GEPOINT(len,0,0));
	f3dLine edge_line3(GEPOINT(0,-pAngle->m_fThick,0),GEPOINT(len,-pAngle->m_fThick,0));
	f3dLine edge_line4(GEPOINT(0,-pAngle->m_fWidth,0),GEPOINT(len,-pAngle->m_fWidth,0));
	//统计始端轮廓点集合
	ATOM_LIST<GEPOINT> xStartPtList;
	if(pAngle->cut_wing[0]>0)
	{	//始端切肢
		if(pAngle->cut_wing[0]==1)
		{	//X肢为切肢主肢
			edge_line4.startPt.x=pAngle->cut_wing_para[0][2];
			edge_line2.startPt.x=pAngle->cut_wing_para[0][0];
			f3dPoint cut_wing_pt(0,pAngle->cut_wing_para[0][1],0);
			Int3dpl(f3dLine(edge_line2.startPt,edge_line4.startPt),edge_line3,edge_line3.startPt);
			Int3dpl(f3dLine(edge_line2.startPt,cut_wing_pt),edge_line1,edge_line1.startPt);
			//
			xStartPtList.append(edge_line4.startPt);
			xStartPtList.append(edge_line3.startPt);
			xStartPtList.append(edge_line2.startPt);
			if(pAngle->cut_wing_para[0][1]>pAngle->m_fThick)
			{	//切另一肢宽度大于厚度
				xStartPtList.append(edge_line1.startPt);
				xStartPtList.append(cut_wing_pt);
			}
			else
			{	//切整肢
				xStartPtList.append(cut_wing_pt);
				xStartPtList.append(edge_line1.startPt);
			}
			if(pAngle->cut_angle[1][0]>0)
			{	//Y肢切角
				edge_line0.startPt.x=pAngle->cut_angle[1][0];
				xStartPtList.append(f3dPoint(0,pAngle->m_fWidth-pAngle->cut_angle[1][1],0));	
			}
			xStartPtList.append(edge_line0.startPt);
		}
		else							
		{	//Y肢为切肢主肢
			edge_line0.startPt.x=pAngle->cut_wing_para[0][2];
			edge_line2.startPt.x=pAngle->cut_wing_para[0][0];
			f3dPoint cut_wing_pt(0,-pAngle->cut_wing_para[0][1],0);
			Int3dpl(f3dLine(edge_line2.startPt,edge_line0.startPt),edge_line1,edge_line1.startPt);
			Int3dpl(f3dLine(edge_line2.startPt,cut_wing_pt),edge_line3,edge_line3.startPt);
			//
			xStartPtList.append(edge_line0.startPt);
			xStartPtList.append(edge_line1.startPt);
			xStartPtList.append(edge_line2.startPt);
			if(pAngle->cut_wing_para[0][1]>pAngle->m_fThick)
			{	//切另一肢宽度大于厚度
				xStartPtList.append(edge_line3.startPt);
				xStartPtList.append(cut_wing_pt);
			}
			else
			{	//切整肢
				xStartPtList.append(cut_wing_pt);
				xStartPtList.append(edge_line3.startPt);
			}
			if(pAngle->cut_angle[0][0]>0)
			{	//X肢切角
				edge_line4.startPt.x=pAngle->cut_angle[0][0];
				xStartPtList.append(f3dPoint(0,pAngle->cut_angle[0][1]-pAngle->m_fWidth,0));
			}
			xStartPtList.append(edge_line4.startPt);
		}
	}
	else if(pAngle->cut_angle[0][0]>EPS2 || pAngle->cut_angle[1][0]>EPS2)
	{	//处理起始端只有切角
		f3dPoint cut_angle_pt1,cut_angle_pt2;
		if(pAngle->cut_angle[1][0]>0)
		{	//Y肢切角
			edge_line0.startPt.x=pAngle->cut_angle[1][0];
			cut_angle_pt1.Set(0,pAngle->m_fWidth-pAngle->cut_angle[1][1],0);
		}
		xStartPtList.append(edge_line0.startPt);
		if(!cut_angle_pt1.IsZero())
			xStartPtList.append(cut_angle_pt1);
		xStartPtList.append(edge_line1.startPt);
		xStartPtList.append(edge_line2.startPt);
		xStartPtList.append(edge_line3.startPt);
		if(pAngle->cut_angle[0][0]>0)
		{	//X肢切角
			edge_line4.startPt.x=pAngle->cut_angle[0][0];
			cut_angle_pt2.Set(0,pAngle->cut_angle[0][1]-pAngle->m_fWidth,0);
		}
		if(!cut_angle_pt2.IsZero())
			xStartPtList.append(cut_angle_pt2);
		xStartPtList.append(edge_line4.startPt);
	}
	else
	{	//无任何切除操作
		xStartPtList.append(edge_line0.startPt);
		xStartPtList.append(edge_line1.startPt);
		xStartPtList.append(edge_line2.startPt);
		xStartPtList.append(edge_line3.startPt);
		xStartPtList.append(edge_line4.startPt);
	}
	//统计终端轮廓点集合
	ATOM_LIST<GEPOINT> xEndPtList;
	if(pAngle->cut_wing[1]>0)
	{	//终端切肢
		if(pAngle->cut_wing[1]==1)
		{	//X肢为切肢主肢
			edge_line4.endPt.x=len-pAngle->cut_wing_para[1][2];
			edge_line2.endPt.x=len-pAngle->cut_wing_para[1][0];
			f3dPoint cut_wing_pt(len,pAngle->cut_wing_para[1][1],0);
			Int3dpl(f3dLine(edge_line2.endPt,edge_line4.endPt),edge_line3,edge_line3.endPt);
			Int3dpl(f3dLine(edge_line2.endPt,cut_wing_pt),edge_line1,edge_line1.endPt);
			//
			xEndPtList.append(edge_line4.endPt);
			xEndPtList.append(edge_line3.endPt);
			xEndPtList.append(edge_line2.endPt);
			if(pAngle->cut_wing_para[1][1]>pAngle->m_fThick)
			{	//切另一肢宽度大于厚度
				xEndPtList.append(edge_line1.endPt);
				xEndPtList.append(cut_wing_pt);
			}
			else
			{	//切整肢
				xEndPtList.append(cut_wing_pt);
				xEndPtList.append(edge_line1.endPt);
			}
			if(pAngle->cut_angle[3][1]>0)
			{	//Y肢切角
				edge_line0.endPt.x=len-pAngle->cut_angle[3][0];
				xEndPtList.append(f3dPoint(len,pAngle->m_fWidth-pAngle->cut_angle[3][1],0));
			}
			xEndPtList.append(edge_line0.endPt);
		}
		else
		{	//Y肢为切肢主肢
			edge_line0.endPt.x=len-pAngle->cut_wing_para[1][2];
			edge_line2.endPt.x=len-pAngle->cut_wing_para[1][0];
			f3dPoint cut_wing_pt(len,-pAngle->cut_wing_para[1][1],0);
			Int3dpl(f3dLine(edge_line2.endPt,edge_line0.endPt),edge_line1,edge_line1.endPt);
			Int3dpl(f3dLine(edge_line2.endPt,cut_wing_pt),edge_line3,edge_line3.endPt);
			//
			xEndPtList.append(edge_line0.endPt);
			xEndPtList.append(edge_line1.endPt);
			xEndPtList.append(edge_line2.endPt);
			if(pAngle->cut_wing_para[1][1]>pAngle->m_fThick)
			{	//切另一肢宽度大于厚度
				xEndPtList.append(edge_line3.endPt);
				xEndPtList.append(cut_wing_pt);
			}
			else
			{	//切整肢
				xEndPtList.append(cut_wing_pt);
				xEndPtList.append(edge_line3.endPt);
			}
			if(pAngle->cut_angle[2][0]>0)
			{	//X肢切角
				edge_line4.endPt.x=len-pAngle->cut_angle[2][0];
				xEndPtList.append(f3dPoint(len,pAngle->cut_angle[2][1]-pAngle->m_fWidth,0));
			}
			xEndPtList.append(edge_line4.endPt);
		}
	}
	else if(pAngle->cut_angle[2][0]>EPS2 || pAngle->cut_angle[3][0]>EPS2)
	{	//处理终端只有切角
		f3dPoint cut_angle_pt1,cut_angle_pt2;
		if(pAngle->cut_angle[3][0]>0)
		{	//Y肢切角
			edge_line0.endPt.x=len-pAngle->cut_angle[3][0];
			cut_angle_pt1.Set(len,pAngle->m_fWidth-pAngle->cut_angle[3][1],0);
		}
		xEndPtList.append(edge_line0.endPt);
		if(!cut_angle_pt1.IsZero())
			xEndPtList.append(cut_angle_pt1);
		xEndPtList.append(edge_line1.endPt);
		xEndPtList.append(edge_line2.endPt);
		xEndPtList.append(edge_line3.endPt);
		if(pAngle->cut_angle[2][0]>0)
		{	//X肢切角
			edge_line4.endPt.x=len-pAngle->cut_angle[2][0];
			cut_angle_pt2.Set(len,pAngle->cut_angle[2][1]-pAngle->m_fWidth,0);
		}
		if(!cut_angle_pt2.IsZero())
			xEndPtList.append(cut_angle_pt2);
		xEndPtList.append(edge_line4.endPt);
	}
	else
	{	//无任何切除操作
		xEndPtList.append(edge_line0.endPt);
		xEndPtList.append(edge_line1.endPt);
		xEndPtList.append(edge_line2.endPt);
		xEndPtList.append(edge_line3.endPt);
		xEndPtList.append(edge_line4.endPt);
	}
	//添加轮廓点
	for(int i=0;i<xStartPtList.GetNodeNum();i++)
		AppendDbPoint(pDrawing,xStartPtList[i],angleId,PS_SOLID,color);
	for(int i=0;i<xEndPtList.GetNodeNum();i++)
		AppendDbPoint(pDrawing,xEndPtList[i],angleId,PS_SOLID,color);
	//添加轮廓边
	AppendDbLine(pDrawing,edge_line0.startPt,edge_line0.endPt,angleId,PS_SOLID,color);
	AppendDbLine(pDrawing,edge_line1.startPt,edge_line1.endPt,angleId,PS_DASH,color);
	AppendDbLine(pDrawing,edge_line2.startPt,edge_line2.endPt,angleId,PS_SOLID,color);
	AppendDbLine(pDrawing,edge_line3.startPt,edge_line3.endPt,angleId,PS_DASH,color);
	AppendDbLine(pDrawing,edge_line4.startPt,edge_line4.endPt,angleId,PS_SOLID,color);
	for(int i=0;i<xStartPtList.GetNodeNum()-1;i++)
		AppendDbLine(pDrawing,xStartPtList[i],xStartPtList[i+1],angleId,PS_SOLID,color);
	for(int i=0;i<xEndPtList.GetNodeNum()-1;i++)
		AppendDbLine(pDrawing,xEndPtList[i],xEndPtList[i+1],angleId,PS_SOLID,color);
	//添加螺栓孔
	BOLT_INFO *pBolt=NULL;
	WORD wLen=pAngle->GetLength();
	for(pBolt=pAngle->m_xBoltInfoList.GetFirst();pBolt;pBolt=pAngle->m_xBoltInfoList.GetNext())
	{
		pBolt->hiberId.masterId=pAngle->GetKey();
		f3dPoint ls_pos(pBolt->posX,pBolt->posY);
		if(m_bOverturn)
			ls_pos.Set(wLen-pBolt->posX,-pBolt->posY);
		AppendDbCircle(pDrawing,ls_pos,f3dPoint(0,0,1),pBolt->bolt_d*0.5,pBolt->hiberId,PS_SOLID,boltColor);
	}
}
static bool PtInLineLeft(f3dPoint startPt,f3dPoint endPt,f3dPoint pickPt)
{
	f3dPoint v1=endPt-startPt;
	f3dPoint v2=pickPt-startPt;
	if(v1.x*v2.y-v1.y*v2.x<=0) 
		return false;	//点在直线上或直线右侧
	return true;
}
//Dxf对齐方式装换为XeroCad对齐方式
int DxfAlign2XeroCadAlign(short dxfAlign)
{
	//dxfAlign：Horizontal低位字节表示，Vertical高为字节表示
	/*enum TextHorzMode      { kTextLeft    = 0,   // TH_LEFT,
		kTextCenter  = 1,   // TH_CENT,
		kTextRight   = 2,   // TH_RIGHT,
		kTextAlign   = 3,   // THV_ALIGN,
		kTextMid     = 4,   // THV_MID,
		kTextFit     = 5 }; // THV_FIT
	enum TextVertMode      { kTextBase    = 0,   // TV_BASE,
		kTextBottom  = 1,   // TV_BOT,
		kTextVertMid = 2,   // TV_MID,
		kTextTop     = 3 }; // TV_TOP
	//XeroCadAlign
	static const int AlignDefault;		//= 1;
	static const int AlignTopLeft;		//= 2;
	static const int AlignTopCenter;	//= 3;
	static const int AlignTopRight;		//= 4;
	static const int AlignMiddleLeft;	//= 5;
	static const int AlignMiddleCenter;	//= 6;
	static const int AlignMiddleRight;	//= 7;
	static const int AlignBottomLeft;	//= 8;
	static const int AlignBottomCenter;	//= 9;
	static const int AlignBottomRight;	//= 10	
	*/
	union DXF_ALIGN{
		short align;
		struct{BYTE horzMode,vertMode;};
	};
	DXF_ALIGN align;
	align.align=dxfAlign;
	int xeroCadAlign=0;
	switch(align.horzMode){
	case 0:
		if(align.vertMode==0)
			xeroCadAlign=IDbText::AlignDefault;
		else if(align.vertMode==1)
			xeroCadAlign=IDbText::AlignBottomLeft;
		else if(align.vertMode==2)
			xeroCadAlign=IDbText::AlignMiddleLeft;
		else if(align.vertMode==3)
			xeroCadAlign=IDbText::AlignTopLeft;
		break;
	case 1:
		if(align.vertMode==0)
			xeroCadAlign=IDbText::AlignDefault;
		else if(align.vertMode==1)
			xeroCadAlign=IDbText::AlignBottomCenter;
		else if(align.vertMode==2)
			xeroCadAlign=IDbText::AlignMiddleCenter;
		else if(align.vertMode==3)
			xeroCadAlign=IDbText::AlignTopCenter;
		break;
	case 2:
		if(align.vertMode==0)
			xeroCadAlign=IDbText::AlignDefault;
		else if(align.vertMode==1)
			xeroCadAlign=IDbText::AlignBottomRight;
		else if(align.vertMode==2)
			xeroCadAlign=IDbText::AlignMiddleRight;
		else if(align.vertMode==3)
			xeroCadAlign=IDbText::AlignTopRight;
		break;
	case 3:
		xeroCadAlign=IDbText::AlignMiddleCenter;
		break;
	case 4:
		xeroCadAlign=IDbText::AlignMiddleCenter;
		break;
	case 5:
		xeroCadAlign=IDbText::AlignMiddleCenter;
		break;
	}
	return xeroCadAlign;
}

#define	TRANS_POS(pt) GEPOINT(pt.x*xScale+x,pt.y*yScale+y)
void DrawDxfBlockRef(IDrawing *pDrawing,PENTINSERT pBlockRef,CDrawing *pDxfDrawing,HIBERID hiberId);
static void DrawDxfEnt( IDrawing *pDrawing,PENTITYHEADER pEntityHeader,LPVOID pEntityData,CDrawing *pDxfDrawing,
						HIBERID hiberId=0,double x=0, double y=0, double xScale=1, double yScale=1, double Rotation=0)
{
	PENTLINE pLine=NULL;
	PENTPOINT pPoint=NULL;
	PENTCIRCLE pCir=NULL;
	PENTTEXT pText=NULL;
	PENTMTEXT pMText=NULL;
	PENTARC pArc=NULL;

	switch(pEntityHeader->EntityType)
	{
	case ENT_LINE:
		pLine=(PENTLINE)pEntityData;
		AppendDbLine(pDrawing,TRANS_POS(pLine->Point0),TRANS_POS(pLine->Point1),hiberId);
		break;
	case ENT_POINT:
		pPoint=(PENTPOINT)pEntityData;
		AppendDbPoint(pDrawing,TRANS_POS(pPoint->Point0),hiberId);
		break;
	case ENT_CIRCLE:
		pCir=(PENTCIRCLE)pEntityData;
		AppendDbCircle(pDrawing,TRANS_POS(pCir->Point0),f3dPoint(0,0,1),pCir->Radius*xScale,hiberId);
		break;
	case ENT_TEXT:
		pText=(PENTTEXT)pEntityData;
		AppendDbText(pDrawing,TRANS_POS(pText->Point0),pText->strText,
			pText->TextData.RotationAngle,pText->TextData.Height,DxfAlign2XeroCadAlign(pText->TextData.Justification),hiberId);
		break;
	case ENT_MTEXT:
		pMText=(PENTMTEXT)pEntityData;
		//多行文本对齐方式与IXeroCad对齐方式差值为1
		/*71	Attachment point:
		1 = Top left; 2 = Top center; 3 = Top right; 
		4 = Middle left; 5 = Middle center; 6 = Middle right;
		7 = Bottom left; 8 = Bottom center; 9 = Bottom right*/
		AppendDbMText(pDrawing,TRANS_POS(pMText->Point0),pMText->strText,
			pMText->TextData.RotationAngle,pMText->TextData.Height,pMText->TextData.Width,
			pMText->TextData.Justification-1,pMText->TextData.FlowDirection,hiberId);	
		break;
	case ENT_ARC:
		pArc=(PENTARC)pEntityData;
		AppendDbArcLine(pDrawing,TRANS_POS(pArc->Point0),pArc->Radius,pArc->StartAngle,pArc->EndAngle,f3dPoint(0,0,1),hiberId);
		break;
	case ENT_INSERT:
		DrawDxfBlockRef(pDrawing,(PENTINSERT)pEntityData,pDxfDrawing,hiberId);
		break;
	/*
	case ENT_ELLIPSE:
		dxfEllipse(
			hDxf, 
			((PENTELLIPSE)pEntityData)->CenterPoint.x,
			((PENTELLIPSE)pEntityData)->CenterPoint.y,
			((PENTELLIPSE)pEntityData)->MajorAxisEndPoint.x,
			((PENTELLIPSE)pEntityData)->MajorAxisEndPoint.y,
			((PENTELLIPSE)pEntityData)->MinorToMajorRatio,
			((PENTELLIPSE)pEntityData)->StartParam,
			((PENTELLIPSE)pEntityData)->EndParam
		);
		break;

	case ENT_SOLID:
		dxfSolid( 
			hDxf,
			((PENTSOLID)pEntityData)->Point0,
			((PENTSOLID)pEntityData)->Point1,
			((PENTSOLID)pEntityData)->Point2,
			((PENTSOLID)pEntityData)->Point3
		);
		break;

	case ENT_INSERT:
		BLOCKHEADER	BlockHeader;
		BlockHeader.Objhandle = ((PENTINSERT)pEntityData)->BlockHeaderObjhandle;
		if(drwFindBlock(hDrawing, FIND_BYHANDLE, &BlockHeader)>0)
			dxfInsertBlock(
				hDxf,
				BlockHeader.Name,
				((PENTINSERT)pEntityData)->Point0.x,
				((PENTINSERT)pEntityData)->Point0.y,
				((PENTINSERT)pEntityData)->XScale,
				((PENTINSERT)pEntityData)->YScale,
				((PENTINSERT)pEntityData)->RotationAngle
			);
		break;

	case ENT_POLYLINE:
		dxfPolyLine(
			hDxf, 
			(PDXFENTVERTEX)((PENTPOLYLINE)pEntityData)->pVertex,
			((PENTPOLYLINE)pEntityData)->nVertex,
			((PENTPOLYLINE)pEntityData)->Flag
		);
		break;

	case ENT_DIMENSION:
		pStyleName = NULL;
		pDimStyleName = NULL;
		// Find dimstyle name
		DimStyle.Objhandle = ((PENTDIMENSION)pEntityData)->DimStyleObjhandle;
		if(drwFindTableType(hDrawing, TAB_DIMSTYLE, FIND_BYHANDLE, &DimStyle)<=0)
			drwFindTableType(hDrawing, TAB_DIMSTYLE, FIND_FIRST, &DimStyle);
		if(DimStyle.dimtxstyObjhandle>0)
		{
			pDimStyleName = DimStyle.Name;
			// Find textstyle name of dimension
			Style.Objhandle = DimStyle.dimtxstyObjhandle;
			if(drwFindTableType(hDrawing, TAB_STYLE, FIND_BYHANDLE, &Style)>0)
				pStyleName = Style.Name;
		}

		dxfSetCurrentDimStyle(hDxf, pDimStyleName);
		dxfSetCurrentTextStyle(hDxf, pStyleName);

		dxfDimLinear(
			hDxf,
			((PENTDIMENSION)pEntityData)->DefPoint3.x,
			((PENTDIMENSION)pEntityData)->DefPoint3.y,
			((PENTDIMENSION)pEntityData)->DefPoint4.x,
			((PENTDIMENSION)pEntityData)->DefPoint4.y,
			((PENTDIMENSION)pEntityData)->DimLineDefPoint.x,
			((PENTDIMENSION)pEntityData)->DimLineDefPoint.y,
			((PENTDIMENSION)pEntityData)->DimRotationAngle,
			((PENTDIMENSION)pEntityData)->DimText
		);
		break;*/
	}
}
static void DrawDxfBlockRef(IDrawing *pDrawing,PENTINSERT pBlockRef,CDrawing *pDxfDrawing,HIBERID hiberId)
{
	BLOCKHEADER		BlockHeader;
	ENTITYHEADER	BlockEntityHeader;
	char			BlockEntityData[4096];
	BlockHeader.Objhandle = pBlockRef->BlockHeaderObjhandle;
	if(pDxfDrawing->FindBlock(FIND_BYHANDLE,&BlockHeader)>0)
	{
		//LAYER	BlockLayer;
		//Layer.Objhandle = BlockHeader.LayerObjhandle;
		//drwFindTableType(hDrawing, TAB_LAYER, FIND_BYHANDLE, &Layer);
		if(pDxfDrawing->FindEntity(FIND_FIRST,&BlockEntityHeader, &BlockEntityData,BlockHeader.Name)>0)
		{
			do{
				DrawDxfEnt(pDrawing,&BlockEntityHeader,&BlockEntityData,pDxfDrawing,hiberId,
					pBlockRef->Point0.x,pBlockRef->Point0.y,pBlockRef->XScale,pBlockRef->YScale,pBlockRef->RotationAngle);
			} while(pDxfDrawing->FindEntity(FIND_NEXT,&BlockEntityHeader, &BlockEntityData,BlockHeader.Name)>0);
		}
	}
}

f2dRect CProcessAngleDraw::InsertDataByGridData(IDrawing *pDarwing,CProcessAngle *pAngle,
												f2dPoint insert_pos,GRID_DATA_STRU &grid_data)
{
	CXhChar50 sGridText;
	f2dRect draw_rect;
	if(grid_data.data_type==1||grid_data.data_type==2)	//实时文本||实时数据
	{
		f3dPoint text_pos=f3dPoint((grid_data.max_x+grid_data.min_x)/2.0,(grid_data.max_y+grid_data.min_y)/2.0,0)+f3dPoint(insert_pos.x,insert_pos.y);
		if(grid_data.type_id==ITEM_TYPE_CODE_NO)		//代号
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_TA_TYPE)	//塔型
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_LENGTH)	//长度
			sGridText.Printf("%d",pAngle->GetLength());
		else if(grid_data.type_id==ITEM_TYPE_PART_NO)	//件号
			sGridText.Copy(pAngle->GetPartNo());
		else if(grid_data.type_id==ITEM_TYPE_MAP_NO)	//钢印号
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_TA_NUM)	//基数
			sGridText.Copy("1");
		else if(grid_data.type_id==ITEM_TYPE_CUT_ROOT)	//刨根
		{	//默认显示刨根标记，如果刨根长度已设置且标记类型为实时数据则显示长度  wht 12-03-10
			if(pAngle->m_bCutRoot)
				sGridText.Copy("√");
		}
		else if(grid_data.type_id==ITEM_TYPE_CUT_BER)	//铲背
		{	//默认显示铲背标记，如果铲背长度已设置且标记类型为实时数据则显示长度  wht 12-03-10
			if(pAngle->m_bCutBer)
				sGridText.Copy("√");
		}
		else if(grid_data.type_id==ITEM_TYPE_HUOQU_FST)	//一次火曲
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_HUOQU_SEC)	//二次火曲
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_CUT_ANGLE)	//切角
		{
			if(pAngle->IsEndCutAngleOrWing()||pAngle->IsStartCutAngleOrWing())
				sGridText.Copy("√");
		}
		else if(grid_data.type_id==ITEM_TYPE_CUT_ANGLE_S_X)	//始端X肢切角
			pAngle->GetCutAngleAngWingStr(true,sGridText,NULL,50);
		else if(grid_data.type_id==ITEM_TYPE_CUT_ANGLE_S_Y)	//始端Y肢切角
			pAngle->GetCutAngleAngWingStr(true,NULL,sGridText,50);
		else if(grid_data.type_id==ITEM_TYPE_CUT_ANGLE_E_X)	//终端X肢切角
			pAngle->GetCutAngleAngWingStr(false,sGridText,NULL,50);
		else if(grid_data.type_id==ITEM_TYPE_CUT_ANGLE_E_Y)	//终端Y肢切角
			pAngle->GetCutAngleAngWingStr(false,NULL,sGridText,50);
		else if(grid_data.type_id==ITEM_TYPE_PUSH_FLAT)	//压扁
		{
			if((pAngle->start_push_pos==0&&pAngle->end_push_pos>0)||pAngle->end_push_pos>=pAngle->GetLength())
				sGridText.Printf("%.0f",(float)(pAngle->end_push_pos-pAngle->start_push_pos));
		}
		else if(grid_data.type_id==ITEM_TYPE_PUSH_FLAT_S_X)	//始端X肢压扁
		{
			if((pAngle->start_push_pos==0&&pAngle->end_push_pos>0)&&pAngle->wing_push_X1_Y2==1)
				sGridText.Printf("始-X/%.0f",(float)(pAngle->end_push_pos-pAngle->start_push_pos));
		}
		else if(grid_data.type_id==ITEM_TYPE_PUSH_FLAT_S_Y)	//始端Y肢压扁
		{
			if((pAngle->start_push_pos==0&&pAngle->end_push_pos>0)&&pAngle->wing_push_X1_Y2==2)
				sGridText.Printf("始-Y/%.0f",(float)(pAngle->end_push_pos-pAngle->start_push_pos));
		}
		else if(grid_data.type_id==ITEM_TYPE_PUSH_FLAT_E_X)	//终端X肢压扁
		{
			if(pAngle->end_push_pos>=pAngle->GetLength()&&pAngle->wing_push_X1_Y2==1)
				sGridText.Printf("终-X/%.0f",(float)(pAngle->end_push_pos-pAngle->start_push_pos));
		}
		else if(grid_data.type_id==ITEM_TYPE_PUSH_FLAT_E_Y)	//终端Y肢压扁
		{
			if(pAngle->end_push_pos>=pAngle->GetLength()&&pAngle->wing_push_X1_Y2==2)
				sGridText.Printf("终-Y/%.0f",(float)(pAngle->end_push_pos-pAngle->start_push_pos));
		}
		else if(grid_data.type_id==ITEM_TYPE_WELD)	//焊接
		{
			if(pAngle->m_bWeld)
				sGridText.Copy("√");
		}
		else if(grid_data.type_id==ITEM_TYPE_KAIJIAO)	//开角
		{
			double wing_angle=pAngle->GetDecWingAngle();
			double fKaiHeJiaoThreshold=2;
			if(pAngle->IsWingKaiHe()&&wing_angle-90>fKaiHeJiaoThreshold)
				sGridText.Printf("%.0f",wing_angle-90);
		}
		else if(grid_data.type_id==ITEM_TYPE_HEJIAO)	//合角
		{
			double wing_angle=pAngle->GetDecWingAngle();
			double fKaiHeJiaoThreshold=2;
			if(pAngle->IsWingKaiHe()&&90-wing_angle>fKaiHeJiaoThreshold)
				sGridText.Printf("%.0f",90-wing_angle);
		}
		else if(grid_data.type_id==ITEM_TYPE_WING_ANGLE)	//两肢夹角
		{
			if(pAngle->IsWingKaiHe())
				sGridText.Printf("%.0f",pAngle->GetDecWingAngle());
		}
		else if(grid_data.type_id==ITEM_TYPE_PART_NUM)		//单基数
			sGridText.Copy("1");
		else if(grid_data.type_id==ITEM_TYPE_PIECE_WEIGHT)	//单重 
			sGridText.Printf("%.1f",pAngle->GetWeight());
		else if(grid_data.type_id==ITEM_TYPE_SUM_PART_NUM)	//加工数
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_SUM_WEIGHT)	//总重
			sGridText.Copy("");
		else if (grid_data.type_id == ITEM_TYPE_DES_MAT)		//设计材质
			sGridText.Copy(LocalQuerySteelMark(pAngle->cMaterial));
		else if(grid_data.type_id==ITEM_TYPE_REPL_GUIGE)	//代用规格
		{
			if(pAngle->IsReplacePart())
				sGridText.Copy(pAngle->GetSpec());
		}
		else if(grid_data.type_id==ITEM_TYPE_DES_GUIGE)	//设计规格
		{
			if(pAngle->IsReplacePart())
			{
				QuerySteelMatMark(pAngle->m_cOrgMaterial,sGridText);
				sGridText.Printf("L%dx%d-%s",pAngle->m_fOrgWidth,pAngle->m_fOrgThick,(char*)sGridText);
			}
			else
				sGridText.Copy(pAngle->GetSpec(FALSE));
		}
		else if(grid_data.type_id==ITEM_TYPE_PRJ_NAME)	//工程名称
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_LSM12_NUM)	//M12孔数
			sGridText.Printf("%d",pAngle->GetLsCount(12));
		else if(grid_data.type_id==ITEM_TYPE_LSM16_NUM)	//M16孔数
			sGridText.Printf("%d",pAngle->GetLsCount(16));
		else if(grid_data.type_id==ITEM_TYPE_LSM18_NUM)	//M18孔数
			sGridText.Printf("%d",pAngle->GetLsCount(18));
		else if(grid_data.type_id==ITEM_TYPE_LSM20_NUM)	//M20孔数
			sGridText.Printf("%d",pAngle->GetLsCount(20));
		else if(grid_data.type_id==ITEM_TYPE_LSM22_NUM)	//M22孔数
			sGridText.Printf("%d",pAngle->GetLsCount(22));
		else if(grid_data.type_id==ITEM_TYPE_LSM24_NUM)	//M24孔数
			sGridText.Printf("%d",pAngle->GetLsCount(24));
		else if(grid_data.type_id==ITEM_TYPE_LSSUM_NUM)	//总孔数
			sGridText.Printf("%d",pAngle->GetLsCount(0));
		else if(grid_data.type_id==ITEM_TYPE_TAB_MAKER)	//制表人
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_CRITIC)	//评审人
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_AUDITOR)	//审核人
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_PART_NOTES)//备注
			pAngle->GetProcessStr(sGridText,50);
		else if(grid_data.type_id==ITEM_TYPE_DATE)		//日期
		{
			CTime t=CTime::GetCurrentTime();
			CString s=t.Format("%Y.%m.%d");
			sprintf(sGridText,"%s",s);
		}
		else if(grid_data.type_id==ITEM_TYPE_PAGENUM)	//共 页
			sGridText.Copy("");
		else if(grid_data.type_id==ITEM_TYPE_PAGEINDEX)	//第 页
			sGridText.Copy("");
		AppendDbText(pDarwing,text_pos,sGridText,0,grid_data.fTextHigh,IDbText::AlignMiddleCenter,0,PS_SOLID,RGB(255,0,0));
	}
	else if(grid_data.data_type==3)	//草图区域
	{
		draw_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
		draw_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
	}
	return draw_rect;
}

BOOL InsertBlock(IDrawing *pDrawing,const char *blkname,f3dPoint insert_pos,
				 double scale,double angle,HIBERID hiberid)
{
	CDrawing dxfDrawing;
	if(!dxfDrawing.Create())
		return FALSE;
	char drive[8],dir[MAX_PATH],fname[MAX_PATH],ext[10];
	_splitpath(CProcessAngleDraw::m_sAngleProcessCardPath,drive,dir,fname,ext);
	CXhChar500 sFilePath("%s%s\\%s.dxf",drive,dir,blkname);
	if(!dxfDrawing.LoadDXFFile(sFilePath))
		return FALSE;
	ENTITYHEADER	EntityHeader;
	char			EntityData[4096];
	if(dxfDrawing.FindEntity(FIND_FIRST,&EntityHeader,&EntityData)>0)
	{
		do{
			if(!EntityHeader.Deleted)
				DrawDxfEnt(pDrawing,&EntityHeader,&EntityData,&dxfDrawing,hiberid,insert_pos.x,insert_pos.y,scale,scale);
		} while(dxfDrawing.FindEntity(FIND_NEXT,&EntityHeader,&EntityData)>0);
	}
	return TRUE;
}
	
f2dRect CProcessAngleDraw::InsertJgProcessCardTbl(IDrawing *pDrawing,CProcessAngle *pAngle,f2dPoint insert_pos/*=f2dPoint(0,0)*/)
{
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];
	_splitpath(CProcessAngleDraw::m_sAngleProcessCardPath,drive,dir,fname,ext);
	if(stricmp(".pcd",ext)==0)
		return InsertJgProcessCardTbl2(pDrawing,pAngle,insert_pos);
	
	f2dRect rect,draw_rect;
	CDrawing dxfDrawing;
	if(!dxfDrawing.Create())
		return draw_rect;
	if(!dxfDrawing.LoadDXFFile(CProcessAngleDraw::m_sAngleProcessCardPath))
		return draw_rect;
	ENTITYHEADER	EntityHeader;
	char			EntityData[4096];
	OBJECTHEADER	ObjectHeader;
	char			ObjectData[4096];
	GRID_DATA_STRU	grid_data;
	PENTPOINT		pPoint=NULL;
	POBJDICTIONARY	pDict=NULL;
	POBJXRECORD		pRec=NULL;
	
	if(dxfDrawing.FindEntity(FIND_FIRST,&EntityHeader,&EntityData)>0)
	{
		do{
			if(!EntityHeader.Deleted)
			{
				if(EntityHeader.EntityType==ENT_POINT)
				{
					pDict=NULL;
					pRec=NULL;
					pPoint=(PENTPOINT)&EntityData;
					ObjectHeader.ObjType=OBJ_DICTIONARY;
					ObjectHeader.DxfObjHandle=pPoint->hRelatedObj;
					if(dxfDrawing.FindObject(FIND_BYDXFHANDLE,&ObjectHeader,&ObjectData)>0)
					{
						pDict=(POBJDICTIONARY)&ObjectData;
						ObjectHeader.ObjType=OBJ_XRECORD;
						for(int i=0;i<pDict->nRecCount;i++)
						{
							if(strcmp(pDict->recArr[i].sName,"TOWER_XREC")==0)
							{
								ObjectHeader.DxfObjHandle=pDict->recArr[i].hRec;
								break;
							}
						}
						if(dxfDrawing.FindObject(FIND_BYDXFHANDLE,&ObjectHeader,&ObjectData)>0)
							pRec=(POBJXRECORD)&ObjectData;
					}
					if(pRec)
					{
						for(int i=0;i<pRec->nValCount;i++)
						{
							if(i==0)
								grid_data.data_type=pRec->resValArr[i].resval.rlong;
							else if(i==1)
								grid_data.type_id=pRec->resValArr[i].resval.rlong;
							else if(i==2)
								grid_data.fTextHigh=pRec->resValArr[i].resval.rreal;
							else if(i==3)
								grid_data.fHighToWide=pRec->resValArr[i].resval.rreal;
							else if(i==4)
								grid_data.min_x=pRec->resValArr[i].resval.rreal;
							else if(i==5)
								grid_data.min_y=pRec->resValArr[i].resval.rreal;
							else if(i==6)
								grid_data.max_x=pRec->resValArr[i].resval.rreal;
							else if(i==7)
								grid_data.max_y=pRec->resValArr[i].resval.rreal;
						}
						rect=InsertDataByGridData(pDrawing,pAngle,insert_pos,grid_data);//根据数据点附加数据提取信息
						if(grid_data.data_type==3)
							draw_rect=rect;
					}
				}
				else
					DrawDxfEnt(pDrawing,&EntityHeader,&EntityData,&dxfDrawing);
			}
		} while(dxfDrawing.FindEntity(FIND_NEXT,&EntityHeader,&EntityData)>0);
	}
	return draw_rect;
}
bool InitXeroDrawingAndDataFromFile(IDrawing *pDrawing,CHashList<GRID_DATA_STRU>& hashAttachDataByPointId,const char* sFileName);
f2dRect CProcessAngleDraw::InsertJgProcessCardTbl2(IDrawing *pDrawing,CProcessAngle *pAngle,f2dPoint insert_pos/*=f2dPoint(0,0)*/)
{
	f2dRect draw_rect,rect;
	CHashList<GRID_DATA_STRU> hashAttachDataByPointId;
	InitXeroDrawingAndDataFromFile(pDrawing,hashAttachDataByPointId,CProcessAngleDraw::m_sAngleProcessCardPath);
	for (GRID_DATA_STRU* pDataStru=hashAttachDataByPointId.GetFirst();pDataStru;pDataStru=hashAttachDataByPointId.GetNext())
	{
		rect=InsertDataByGridData(pDrawing,pAngle,insert_pos,*pDataStru);//根据数据点附加数据提取信息
		if(pDataStru->data_type==3)
			draw_rect=rect;
	}
	return draw_rect;
}
int InitDrawingFromBuffer( IDrawing *pDrawing,CBuffer& buffer);
BOOL InsertPcdBlock(IDrawing *pDrawing,f3dPoint insert_pos,char *pcd_name)
{
	char filename[MAX_PATH],temstr[MAX_PATH],sys_path[MAX_PATH];
	CPEC::GetSysParaFromReg("SYS_PATH",sys_path);
	strcpy(filename,sys_path);
	sprintf(temstr,"%s.pcd",pcd_name);
	strcat(filename,temstr);
	FILE* fp=fopen(filename,"rb");
	if(fp==NULL)
		return false;
	int nFileLen=fseek(fp,0,SEEK_END);
	nFileLen=ftell(fp);
	fseek(fp,0,SEEK_SET);
	CBuffer buffer;
	buffer.WriteAt(0,NULL,nFileLen);
	fread(buffer.GetBufferPtr(),nFileLen,1,fp);
	fclose(fp);
	//
	CXeroDrawing xDrawing;
	InitDrawingFromBuffer(&xDrawing,buffer);
	for(IDbEntity* pEnt=xDrawing.EnumFirstDbEntity();pEnt;pEnt=xDrawing.EnumNextDbEntity())
	{
		if(pEnt->GetDbEntType()==IDbEntity::DbLine)
		{
			IDbLine* pDbLine=(IDbLine*)pEnt;
			f3dPoint ptS=f3dPoint(pDbLine->StartPosition())+insert_pos;
			f3dPoint ptE=f3dPoint(pDbLine->EndPosition())+insert_pos;
			AppendDbLine(pDrawing,ptS,ptE);
		}
		else if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
		{
			IDbCircle* pCircle=(IDbCircle*)pEnt;
			f3dPoint center=f3dPoint(pCircle->Center())+insert_pos;
			AppendDbCircle(pDrawing,center,pCircle->Normal(),pCircle->Radius());
		}
		else if (pEnt->GetDbEntType()==IDbEntity::DbPolyline)
		{	//目前DrawSolid.dll不支持多段线的绘制，通过绘制子线段实现
			IDbPolyline* pPolyline=(IDbPolyline*)pEnt;
			ARRAY_LIST<GEPOINT> ptArr;
			for(int i=0;i<pPolyline->numVerts();i++)
			{
				GEPOINT pt;
				pPolyline->GetVertexAt(i,pt);
				ptArr.append(f3dPoint(pt)+insert_pos);
			}
			for(int i=0;i<ptArr.GetSize()-1;i++)
				AppendDbLine(pDrawing,ptArr[i],ptArr[(i+1)%ptArr.GetSize()]);
			//AppendDbPolyline(pDrawing,ptArr.m_pData,ptArr.GetSize());
		}
	}
	return TRUE;
}
void CProcessAngleDraw::InsertJgProcessSketch(IDrawing *pDrawing,CProcessAngle *pJg,f2dRect draw_rect)
{
	//查找切角、压扁图块的文件夹路径
	char sys_path[MAX_PATH];
	CPEC::GetSysParaFromReg("SYS_PATH",sys_path);
	if(strlen(sys_path)<=0)
		return;
	//初始化工艺卡区域特征位置
	f3dPoint leftTopPt(draw_rect.topLeft.x,draw_rect.topLeft.y,0);
	f3dPoint leftBtmPt(draw_rect.topLeft.x,draw_rect.bottomRight.y);
	f3dPoint rightTopPt(draw_rect.bottomRight.x,draw_rect.topLeft.y,0);
	f3dPoint rightBtmPt(draw_rect.bottomRight.x,draw_rect.bottomRight.y,0);
	f3dPoint midTopPT=(leftTopPt+rightTopPt)*0.5;
	f3dPoint midBtmPt=(leftBtmPt+rightBtmPt)*0.5;
	//
	double fTextSize=CRodDrawing::drawPara.fDimTextSize;
	double fLeverMargin=fTextSize+3,fVertMargin=fTextSize+3;
	HIBERID angleId(pJg->GetKey());
	CXhChar100 sDimText;
	f3dPoint base_pos,dim_pos;
	if(CRodDrawing::drawPara.bDimCutAngleMap)
	{
		double fBlockL=18,fBlockW=10;	//插入块的长和高
		//起始端切肢/切角标注
		double fDimOffX=5;	//从块原点(左下)到数据标注的水平偏移距
		if(pJg->cut_wing[0]>0)
		{
			if(pJg->cut_wing[0]==1)	//X肢为切肢主肢
			{	//表示切角/切肢的图标 长=18,宽=10	(左下角)
				base_pos.Set(leftBtmPt.x+fLeverMargin,leftBtmPt.y+fVertMargin,0);
				InsertPcdBlock(pDrawing,base_pos,"CutWingSX");
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);		//块上侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[0][0]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);	//块下侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[0][2]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				if(pJg->cut_wing_para[0][1]>0)	//Y肢切角
				{	//(左上角)
					base_pos.Set(leftTopPt.x+fLeverMargin,leftTopPt.y-fVertMargin-fBlockW,0);
					InsertPcdBlock(pDrawing,base_pos,"CutAngleSY1");
					dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[0][0]);
					AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
					dim_pos.Set(base_pos.x-1,base_pos.y+fBlockW/2);			 //块左侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[0][1]);
					AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
				}
			}
			else	//Y肢为切肢主肢
			{	//(左上角)
				base_pos.Set(leftTopPt.x+fLeverMargin,leftTopPt.y-fVertMargin-fBlockW,0);
				InsertPcdBlock(pDrawing,base_pos,"CutWingSY");
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);	//块下侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[0][0]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);		//块上侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[0][2]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				if(pJg->cut_wing_para[0][1]>0)	//X肢切角
				{	//(左下角)
					base_pos.Set(leftBtmPt.x+fLeverMargin,leftBtmPt.y+fVertMargin,0);
					InsertPcdBlock(pDrawing,base_pos,"CutAngleSX1");
					dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);	//块上侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[0][0]);
					AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
					dim_pos.Set(base_pos.x-1,base_pos.y+fBlockW/2);			//块左侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[0][1]);
					AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
				}
			} 
		}
		if(pJg->cut_angle[0][0]>0)	//起始端X肢切角
		{	//(左下角)
			base_pos.Set(leftBtmPt.x+fLeverMargin,leftBtmPt.y+fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"CutAngleSX");
			dim_pos.Set(base_pos.x-1,base_pos.y+fBlockW/2);//块左侧
			sDimText.Printf("%.0f",pJg->cut_angle[0][1]);
			AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
			sDimText.Printf("%.0f",pJg->cut_angle[0][0]);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		if(pJg->cut_angle[1][0]>0)	//起始端Y肢切角
		{	//(左上角)
			base_pos.Set(leftTopPt.x+fLeverMargin,draw_rect.topLeft.y-fVertMargin-fBlockW,0);
			InsertPcdBlock(pDrawing,base_pos,"CutAngleSY");
			dim_pos.Set(base_pos.x-1,base_pos.y+fBlockW/2);//块左侧
			sDimText.Printf("%.0f",pJg->cut_angle[1][1]);
			AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);//块上侧
			sDimText.Printf("%.0f",pJg->cut_angle[1][0]);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		//终止端切肢/切角标注
		fDimOffX=13;	//从块原点(左下)到数据标注的水平偏移距
		if(pJg->cut_wing[1]>0)
		{
			if(pJg->cut_wing[1]==1)	//X肢为切肢主肢
			{	//(右下角)
				base_pos.Set(rightBtmPt.x-fLeverMargin-fBlockL,rightBtmPt.y+fVertMargin,0);
				InsertPcdBlock(pDrawing,base_pos,"CutWingEX");
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);//块上侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[1][0]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[1][2]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				if(pJg->cut_wing_para[1][1]>0)	//Y肢切角
				{	//(右上角)
					base_pos.Set(rightTopPt.x-fLeverMargin-fBlockL,rightTopPt.y-fBlockW-fVertMargin,0);
					InsertPcdBlock(pDrawing,base_pos,"CutAngleEY1");
					dim_pos.Set(base_pos.x+fBlockL+fTextSize+1,base_pos.y+fBlockW/2);//块右侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[1][1]);
					AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
					dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[1][0]);
					AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				}
			}
			else							//Y肢为切肢主肢
			{	//(右上角)
				base_pos.Set(rightTopPt.x-fLeverMargin-fBlockL,rightTopPt.y-fBlockW-fVertMargin,0);
				InsertPcdBlock(pDrawing,base_pos,"CutWingEY");
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[1][0]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);//块上侧
				sDimText.Printf("%.0f",pJg->cut_wing_para[1][2]);
				AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				if(pJg->cut_wing_para[1][1]>0)	//X肢切角
				{	//(右下角)
					base_pos.Set(rightBtmPt.x-fLeverMargin-fBlockL,rightBtmPt.y+fVertMargin,0);
					InsertPcdBlock(pDrawing,base_pos,"CutAngleEX1");
					dim_pos.Set(base_pos.x+fBlockL+fTextSize+1,base_pos.y+fBlockW/2);//块右侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[1][1]);
					AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
					dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);//块上侧
					sDimText.Printf("%.0f",pJg->cut_wing_para[1][0]);
					AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
				}
			}
		}
		if(pJg->cut_angle[2][0]>0)	//终止端X肢切角
		{	//(右下角)
			base_pos.Set(rightBtmPt.x-fLeverMargin-fBlockL,rightBtmPt.y+fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"CutAngleEX");
			dim_pos.Set(base_pos.x+fBlockL+fTextSize+1,base_pos.y+fBlockW/2);//块右侧
			sDimText.Printf("%.0f",pJg->cut_angle[2][1]);
			AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y-fTextSize-1);//块下侧
			sDimText.Printf("%.0f",pJg->cut_angle[2][0]);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		if(pJg->cut_angle[3][0]>0)	//终止端Y肢切角
		{	//(右上角)
			base_pos.Set(rightTopPt.x-fLeverMargin-fBlockL,rightTopPt.y-fBlockW-fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"CutAngleEY");
			dim_pos.Set(base_pos.x+fBlockL+fTextSize+1,base_pos.y+fBlockW/2);//块右侧
			sDimText.Printf("%.0f",pJg->cut_angle[3][1]);
			AppendDbText(pDrawing,dim_pos,sDimText,Pi*0.5,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW+1);//块上侧
			sDimText.Printf("%.0f",pJg->cut_angle[3][0]);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
	}
	if(CRodDrawing::drawPara.bDimPushFlatMap)
	{	 
		double fBlockL=17,fBlockW=8;		//始终端压扁块 长=17,宽=8
		double fDimOffX=2.5,fDimOffY=2;		//始终端压扁 压扁长度标注的X偏移值，Y偏移值 
		double fBlockMidL=32,fBlockMidW=9;	//中间压扁块 长=32,宽=9
		double fMidXDimX1=15,fMidXDimY1=11,fMidXDimX2=11,fMidXDimY2=4;	//X肢中间压扁长度标注和向上偏移标注的X偏移值，Y偏移值
		double fMidYDimX1=15,fMidYDimY1=2,fMidYDimX2=11,fMidYDimY2=5;	//Y肢中间压扁长度标注和向上偏移标注的X偏移值，Y偏移值
		if(pJg->wing_push_X1_Y2==1&&pJg->GetPushFlatType()==1)
		{	//始端X肢压扁
			base_pos.Set(leftBtmPt.x+fLeverMargin,leftBtmPt.y+fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"PushFlatSX");
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fBlockW-fDimOffY-fTextSize-1);//块下侧
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		else if(pJg->wing_push_X1_Y2==2&&pJg->GetPushFlatType()==1)
		{	//始端Y肢压扁
			base_pos.Set(leftTopPt.x+fLeverMargin,leftTopPt.y-fVertMargin-fBlockW,0);
			InsertPcdBlock(pDrawing,base_pos,"PushFlatSY");
			dim_pos.Set(base_pos.x+fDimOffX,base_pos.y+fDimOffY+1);//块上侧
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		if(pJg->wing_push_X1_Y2==1&&pJg->GetPushFlatType()==2)
		{	//终端X肢压扁
			base_pos.Set(rightBtmPt.x-fBlockL-fLeverMargin,rightBtmPt.y+fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"PushFlatEX");
			dim_pos.Set(base_pos.x+fBlockL-fDimOffX,base_pos.y+fBlockW-fDimOffY-fTextSize-1);//块下侧
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		else if(pJg->wing_push_X1_Y2==2&&pJg->GetPushFlatType()==2)
		{	//终端Y肢压扁
			base_pos.Set(rightTopPt.x-fBlockL-fLeverMargin,rightTopPt.y-fVertMargin-fBlockW,0);
			InsertPcdBlock(pDrawing,base_pos,"PushFlatEY");
			dim_pos.Set(base_pos.x+fBlockL-fDimOffX,base_pos.y+fDimOffY+1);//块上侧
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		if(pJg->wing_push_X1_Y2==1&&pJg->GetPushFlatType()==3)
		{	//中间X肢压扁
			base_pos.Set(midBtmPt.x-fBlockMidL/2,midBtmPt.y+fVertMargin,0);
			InsertPcdBlock(pDrawing,base_pos,"MidPushFlatX");
			dim_pos.Set(base_pos.x+fMidXDimX2,base_pos.y+fMidXDimY2+1);//块上侧（上偏移距离）
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fMidXDimX1,base_pos.y+fMidXDimY1-fTextSize-1);//块下侧（压扁长度）
			sDimText.Printf("%.0f",(float)pJg->start_push_pos);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
		else if(pJg->wing_push_X1_Y2==2&&pJg->GetPushFlatType()==3)
		{	//中间Y肢压扁
			base_pos.Set(midTopPT.x-fBlockMidL/2,midTopPT.y-fVertMargin-fBlockMidW,0);
			InsertPcdBlock(pDrawing,base_pos,"MidPushFlatY");
			dim_pos.Set(base_pos.x+fMidYDimX1,base_pos.y-fMidYDimY1-fTextSize);//块下侧（压扁长度）
			sDimText.Printf("%.0f",(float)(pJg->end_push_pos-pJg->start_push_pos));
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
			dim_pos.Set(base_pos.x+fMidYDimX2,base_pos.y+fMidYDimY2+1);//块上侧（上偏移距离）
			sDimText.Printf("%.0f",(float)pJg->start_push_pos);
			AppendDbText(pDrawing,dim_pos,sDimText,0,fTextSize,IDbText::AlignMiddleCenter,angleId);
		}
	}
}
void CProcessAngleDraw::ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	CProcessAngle *pAngle=(CProcessAngle*)m_pPart;
	HIBERID angleId(pAngle->GetKey());
	CLineAngleDrawing xAngleDraw;
	f2dRect draw_rect=InsertJgProcessCardTbl(pDrawing,pAngle);
	double fMaxDrawingLen=draw_rect.Width(),fMaxDrawingWidth=draw_rect.Height();
	if(fMaxDrawingLen==0||fMaxDrawingLen==0)
	{	//插入工艺卡图框失败，初始化矩形绘图区域并绘制矩形框
		fMaxDrawingLen=240;
		fMaxDrawingWidth=130;
		draw_rect.topLeft.Set(0,0.5*fMaxDrawingWidth);
		draw_rect.bottomRight.Set(fMaxDrawingLen,-0.5*fMaxDrawingWidth);
		AppendDbRect(pDrawing,f3dPoint(0,0.5*fMaxDrawingWidth),f3dPoint(fMaxDrawingLen,-0.5*fMaxDrawingWidth),angleId);
	}
	InsertJgProcessSketch(pDrawing,pAngle,draw_rect);
	xAngleDraw.CreateLineAngleDrawing(pAngle,fMaxDrawingLen,fMaxDrawingWidth);
	f3dPoint origin(draw_rect.topLeft.x,(draw_rect.topLeft.y+draw_rect.bottomRight.y)/2);
	//
	int i=0,n=0;
	n = xAngleDraw.GetLineCount();
	f3dLine line;
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetLineAt(line,i);
		line.startPt+=origin;
		line.endPt+=origin;
		AppendDbLine(pDrawing,line.startPt,line.endPt,angleId);
	}
	for(i=0;i<5;i++)
	{
		xAngleDraw.GetZEdge(line,i);
		line.startPt+=origin;
		line.endPt+=origin;
		if(line.feature==0)
			AppendDbLine(pDrawing,line.startPt,line.endPt,angleId);
		else
			AppendDbLine(pDrawing,line.startPt,line.endPt,angleId,PS_DASH,RGB(255,0,0));
	}
	f3dCircle cir;
	n = xAngleDraw.GetXLsCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetXLsCircle(cir,i);
		cir.centre+=origin;
		CXhChar50 sBlockName("M%.f_Jg",cir.radius*2);
		HIBERID hiberId(angleId.masterId,HIBERARCHY(0,0,2,cir.ID));
		BOOL bRetCode=InsertBlock(pDrawing,sBlockName,cir.centre,1,0,hiberId);
		if(!bRetCode)
			AppendDbCircle(pDrawing,cir.centre,f3dPoint(0,0,1),1,hiberId,PS_SOLID,RGB(255,0,0));
	}
	n = xAngleDraw.GetYLsCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetYLsCircle(cir,i);
		cir.centre+=origin;
		CXhChar50 sBlockName("M%.f_Jg",cir.radius*2);
		HIBERID hiberId(angleId.masterId,HIBERARCHY(0,0,2,cir.ID));
		BOOL bRetCode=InsertBlock(pDrawing,sBlockName,cir.centre,1,0,hiberId);
		if(!bRetCode)
			AppendDbCircle(pDrawing,cir.centre,f3dPoint(0,0,1),1,hiberId,PS_SOLID,RGB(255,0,0));
	}
	//标注螺栓相对距离
	CSizeTextDim jg_dim;
	n = xAngleDraw.GetDimCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetDimAt(i,&jg_dim);
		if(jg_dim.dist==0)
			continue;
		jg_dim.dimPos+=jg_dim.origin;
		jg_dim.dimStartPos+=jg_dim.origin;
		jg_dim.dimEndPos+=jg_dim.origin;
		jg_dim.dimPos+=origin;
		jg_dim.dimStartPos+=origin;
		jg_dim.dimEndPos+=origin;
		if(jg_dim.angle==0)						//水平标注,标注位置居中
			jg_dim.dimPos.x=(jg_dim.dimStartPos.x+jg_dim.dimEndPos.x)*0.5;
		else if(fabs(jg_dim.angle-0.5*Pi)<EPS)	//垂直标注
			jg_dim.dimPos.y=(jg_dim.dimStartPos.y+jg_dim.dimEndPos.y)*0.5;
		//TODO: 需改进ParseDimension函数
		if(PtInLineLeft(jg_dim.dimStartPos,jg_dim.dimEndPos,jg_dim.dimPos))
		{
			f3dPoint tempPt=jg_dim.dimEndPos;
			jg_dim.dimEndPos=jg_dim.dimStartPos;
			jg_dim.dimStartPos=tempPt;
		}
		CXhChar100 sDimText(jg_dim.DimText());
		if(CAngleDrawing::drawPara.iLsSpaceDimStyle==4&&jg_dim.nHits>0)	//不标注内容
			sDimText.Copy(" ");
		else
			sDimText.Copy(jg_dim.DimText());
		if((CAngleDrawing::drawPara.iLsSpaceDimStyle>=0&&jg_dim.nHits>0)
			||(CAngleDrawing::drawPara.bDimCutAngle&&jg_dim.nHits<=0))
		{
			long dimStyleId=XeroDimStyleTable::dimStyleHoleSpace.m_idDimStyle;
			if(jg_dim.iTextDimSylte==1)	//下侧螺栓间距标注时,文字应置于尺寸标注线外侧(即下方) wjh-2017.12.1
				dimStyleId=XeroDimStyleTable::dimStyleHoleSpace2.m_idDimStyle;
			AppendDbRotatedDim(pDrawing,jg_dim.dimStartPos,jg_dim.dimEndPos,jg_dim.dimPos,
			sDimText,jg_dim.angle,dimStyleId,GEPOINT(0,0,-1),angleId);
		}
	}
	CZhunXianTextDim zhun_dim;
	n = xAngleDraw.GetZhunDimCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetZhunDimAt(i,&zhun_dim);
		zhun_dim.dimStartPos+=zhun_dim.origin;
		zhun_dim.dimEndPos+=zhun_dim.origin;
		zhun_dim.dimPos+=zhun_dim.origin;
		zhun_dim.dimStartPos+=origin;
		zhun_dim.dimEndPos+=origin;
		zhun_dim.dimPos+=origin;
		zhun_dim.lineStart+=origin;
		zhun_dim.lineEnd+=origin;
		if(zhun_dim.angle==0)						//水平标注,标注位置居中
			zhun_dim.dimPos.x=(zhun_dim.dimStartPos.x+zhun_dim.dimEndPos.x)*0.5;
		else if(fabs(jg_dim.angle-0.5*Pi)<EPS)	//垂直标注
			zhun_dim.dimPos.y=(zhun_dim.dimStartPos.y+zhun_dim.dimEndPos.y)*0.5;
		//TODO: 需改进ParseDimension函数
		if(PtInLineLeft(zhun_dim.dimStartPos,zhun_dim.dimEndPos,zhun_dim.dimPos))
		{
			f3dPoint tempPt=zhun_dim.dimEndPos;
			zhun_dim.dimEndPos=zhun_dim.dimStartPos;
			zhun_dim.dimStartPos=tempPt;
		}
		if(zhun_dim.dimStartPos!=zhun_dim.dimEndPos)
		{
			f3dPoint dimPos(zhun_dim.dimStartPos.x-5,zhun_dim.lineStart.y);
			AppendDbText(pDrawing,dimPos,zhun_dim.DimText(),0,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleRight,angleId);
		}
		AppendDbLine(pDrawing,zhun_dim.lineStart,zhun_dim.lineEnd,angleId,PS_DASHDOT,RGB(128,0,255));
	}
	CTextOnlyDrawing pure_txt;
	n = xAngleDraw.GetPureTxtCount();
	//绝对尺寸或者竖直标注的相对尺寸或其他文字标注
	for(i=0;i<n;i++)
	{
		if(!xAngleDraw.GetPureTxtDimAt(i,&pure_txt))
			continue;
		if(!CRodDrawing::drawPara.bDimLsAbsoluteDist&&abs(pure_txt.iOrder)==3)
			continue;	//不标注绝对尺寸
		if(CRodDrawing::drawPara.iLsSpaceDimStyle==3&&(abs(pure_txt.iOrder)==1||abs(pure_txt.iOrder)==2))
			continue;	//不标注竖直相对尺寸(1.第一排螺栓相对尺寸 2.第二排螺栓相对尺寸) wht 11-05-07
		pure_txt.dimPos+=pure_txt.origin;
		pure_txt.dimPos+=origin;
		if(pure_txt.iOrder==0)
			AppendDbText(pDrawing,pure_txt.dimPos,pure_txt.GetPrecisionDimText(),0,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleRight,angleId,PS_SOLID,RGB(64,128,128));
		else if(pure_txt.iOrder==4)
		{	//特殊螺栓标记说明
			AppendDbText(pDrawing,pure_txt.dimPos,pure_txt.dimText,0,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleRight,angleId,PS_SOLID,RGB(64,128,128));
		}
		if(pure_txt.bInXWing)
			AppendDbText(pDrawing,pure_txt.dimPos,pure_txt.dimText,0.5*Pi,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleRight,angleId,PS_SOLID,RGB(64,128,128));
		else
			AppendDbText(pDrawing,pure_txt.dimPos,pure_txt.dimText,0.5*Pi,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleLeft,angleId,PS_SOLID,RGB(64,128,128));
	}
	//标注开合角
	CKaiHeAngleDim kaihe_dim;
	n = xAngleDraw.GetKaiHeDimCount();
	for(i=0;i<n;i++)
	{
		if(xAngleDraw.GetKaiHeDimAt(i,&kaihe_dim))
			continue;
		kaihe_dim.dimPos+=kaihe_dim.origin;
		kaihe_dim.dimStartPos+=kaihe_dim.origin;
		kaihe_dim.dimEndPos+=kaihe_dim.origin;
		kaihe_dim.dimPos+=origin;
		kaihe_dim.dimStartPos+=origin;
		kaihe_dim.dimEndPos+=origin;
		kaihe_dim.dimPos.x=(kaihe_dim.dimStartPos.x+kaihe_dim.dimEndPos.x)/2.0;
		AppendDbText(pDrawing,kaihe_dim.dimPos,kaihe_dim.dimText,0,CRodDrawing::drawPara.fDimTextSize,IDbText::AlignMiddleLeft,angleId);
	}
}
void CProcessAngleDraw::ReDraw(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap,ISolidDraw* pSolidDraw)
{
	IDrawing* pDrawing=p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	int n=pSolidSnap->GetLastSelectEnts(id_arr);
	for(int i=0;i<n;i++)
	{
		IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(id_arr[i]);
		if(pEnt==NULL)
			continue;
		if(pEnt->GetDbEntType()!=IDbEntity::DbCircle)
			continue;
		BOLT_INFO *pBolt=m_pPart->FromHoleHiberId(pEnt->GetHiberId());
		if(pBolt)
		{
			IDbCircle* pCircle=(IDbCircle*)pEnt;
			pCircle->SetCenter(f3dPoint(pBolt->posX,pBolt->posY,0));
			pCircle->SetRadius(pBolt->bolt_d*0.5);
			pCircle->SetNormal(f3dPoint(0,0,1));
		}
	}
}
//////////////////////////////////////////////////////////////////////////
// CProcessPlateDraw
CProcessPlateDraw::CProcessPlateDraw(void)
{
}

CProcessPlateDraw::~CProcessPlateDraw(void)
{
}
static BOOL IsPositionOverlap(f3dPoint ls_pt,ATOM_LIST<f3dPoint>& ptList)
{
	int nOverlap=0;
	for(int i=0;i<ptList.GetNodeNum();i++)
	{
		if(ls_pt.IsEqual(ptList[i],0.3))
			nOverlap++;
	}
	if(nOverlap>1)
		return TRUE;
	return FALSE;
}
int GetLineLenFromExpression(double fThick,const char* sValue)
{
	CExpression expression;
	EXPRESSION_VAR* pVar=expression.varList.Append();
	pVar->fValue=fThick;
	strcpy(pVar->variableStr,"T");
	if(sValue==NULL||strlen(sValue)<=0)
		return -1;
	else
		return (int)expression.SolveExpression(sValue);
}

static void GetCutParamFromReg(BYTE cType,double fThick,int *pnIntoLineLen,int *pnOutLineLen,int *pnEnlargedSpace,
							   BOOL *pbInitPosFarOrg = NULL, BOOL *pbCutPosInInitPos = NULL, BOOL *pbCutSpecialHole=NULL)
{
	CXhChar100 sValue;
	if(cType==0xFF&&CPEC::GetSysParaFromReg("m_ciDisplayType",sValue))
		cType=atoi(sValue);
	if(cType==0xFF)
		cType= 0X01;
	if(cType&0X01)
	{	//火焰切割
		if(pnEnlargedSpace&&CPEC::GetSysParaFromReg("flameCut.m_wEnlargedSpace",sValue))
			*pnEnlargedSpace=atoi(sValue);
		if(pnOutLineLen&&CPEC::GetSysParaFromReg("flameCut.m_sOutLineLen",sValue))
			*pnOutLineLen=GetLineLenFromExpression(fThick,sValue);
		if(pnIntoLineLen&&CPEC::GetSysParaFromReg("flameCut.m_sIntoLineLen",sValue))
			*pnIntoLineLen=GetLineLenFromExpression(fThick,sValue);
		if(pbInitPosFarOrg&&CPEC::GetSysParaFromReg("flameCut.m_bInitPosFarOrg",sValue))
			*pbInitPosFarOrg=atoi(sValue);
		if(pbCutPosInInitPos&&CPEC::GetSysParaFromReg("flameCut.m_bCutPosInInitPos",sValue))
			*pbCutPosInInitPos=atoi(sValue);	
		if (pbCutSpecialHole&&CPEC::GetSysParaFromReg("flameCut.m_bCutSpecialHole", sValue))
			*pbCutSpecialHole = atoi(sValue);
	}
	else if(cType&0X02)
	{	//等离子切割
		if(pnEnlargedSpace&&CPEC::GetSysParaFromReg("plasmaCut.m_wEnlargedSpace",sValue))
			*pnEnlargedSpace=atoi(sValue);
		if(pnOutLineLen&&CPEC::GetSysParaFromReg("plasmaCut.m_sOutLineLen",sValue))
			*pnOutLineLen=GetLineLenFromExpression(fThick,sValue);
		if(pnIntoLineLen&&CPEC::GetSysParaFromReg("plasmaCut.m_sIntoLineLen",sValue))
			*pnIntoLineLen=GetLineLenFromExpression(fThick,sValue);
		if(pbInitPosFarOrg&&CPEC::GetSysParaFromReg("plasmaCut.m_bInitPosFarOrg",sValue))
			*pbInitPosFarOrg=atoi(sValue);
		if(pbCutPosInInitPos&&CPEC::GetSysParaFromReg("plasmaCut.m_bCutPosInInitPos",sValue))
			*pbCutPosInInitPos=atoi(sValue);
		if (pbCutSpecialHole&&CPEC::GetSysParaFromReg("plasmaCut.m_bCutSpecialHole", sValue))
			*pbCutSpecialHole = atoi(sValue);
	}
}
static void DrawPlateNc(CProcessPlate *pPlate, IDrawing *pDrawing, ISolidSet *pSolidSet, COLORREF color, BOOL bDrawCutPt = FALSE)
{
	if (pPlate == NULL || pSolidSet == NULL || pDrawing == NULL)
		return;
	char sValue[MAX_PATH] = "", tem_str[100] = "";
	//获取显示工艺模式
	BYTE ciDisplayNcMode = 0;
	if (CPEC::GetSysParaFromReg("m_ciDisplayType", sValue))
		ciDisplayNcMode = atoi(sValue);
	//获取颜色设置
	COLORREF crEdge = color, crText = color, crHuoQu = color;
	if (CPEC::GetSysParaFromReg("EdgeColor", sValue))
	{
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crEdge);
	}
	if (CPEC::GetSysParaFromReg("TextColor", sValue))
	{
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crText);
	}
	if (CPEC::GetSysParaFromReg("HuoQuColor", sValue))
	{
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crHuoQu);
	}
	//绘制钢板
	HIBERID plateId(pPlate->GetKey());
	if (pPlate->m_cFaceN == 3 && !(pPlate->top_point.IsZero()))
		AppendDbPoint(pDrawing, pPlate->top_point, plateId);
	//绘制直线
	int i = 0; 
	PROFILE_VER *pVertex = NULL, *pPrevPnt = pPlate->vertex_list.GetTail(), *pPrevPrevPnt = pPlate->vertex_list.GetPrev();
	for (pVertex = pPlate->vertex_list.GetFirst(); pVertex; pVertex = pPlate->vertex_list.GetNext(), i++)
	{
		//轮廓点
		pVertex->hiberId.masterId = pPlate->GetKey();
		pPrevPnt->hiberId.masterId = pPlate->GetKey();
		if (pPlate->mcsFlg.ciBottomEdge != i)
			AppendDbPoint(pDrawing, pVertex->vertex, pVertex->hiberId, PS_SOLID, crEdge);
		else
			AppendDbPoint(pDrawing, pVertex->vertex, pVertex->hiberId, PS_SOLID, RGB(127, 255, 0), 8);
		//轮廓线
		int nPenWidth = pPrevPnt->m_bWeldEdge ? 2 : 1;
		if (pPlate->m_cFaceN < 3 || pPrevPnt->vertex.feature != 3 || pVertex->vertex.feature != 2)
		{
			if (pPrevPnt->type == 2)
			{	//圆弧
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
				pArcLine->CreateMethod2(pPrevPnt->vertex, pVertex->vertex, pPrevPnt->work_norm, pPrevPnt->sector_angle);
			}
			else if (pPrevPnt->type == 3)
			{	//椭圆弧
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
				pArcLine->CreateEllipse(pPrevPnt->center, pPrevPnt->vertex, pVertex->vertex, pPrevPnt->column_norm,
					pPrevPnt->work_norm, pPrevPnt->radius);
			}
			else	//直线
				AppendDbLine(pDrawing, pPrevPnt->vertex, pVertex->vertex, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
		}
		else
		{
			AppendDbLine(pDrawing, pPrevPnt->vertex, pPlate->top_point, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
			AppendDbLine(pDrawing, pPlate->top_point, pVertex->vertex, pVertex->hiberId, PS_SOLID, crEdge, nPenWidth);
		}
		pPrevPrevPnt = pPrevPnt;
		pPrevPnt = pVertex;
	}
	//绘制内圆
	if (pPlate->m_fInnerRadius > 0)
	{
		if (!pPlate->m_tInnerColumnNorm.IsZero() && fabs(pPlate->m_tInnerColumnNorm*f3dPoint(0, 0, 1)) < EPS_COS)
		{	//椭圆
			f3dPoint center = pPlate->m_tInnerOrigin, workNorm(0, 0, 1), columnNorm = pPlate->m_tInnerColumnNorm;
			f3dPoint minorAxis = columnNorm ^ workNorm;
			normalize(minorAxis);	//椭圆短轴方向
			f3dPoint majorAxis(-minorAxis.y, minorAxis.x, minorAxis.z);
			normalize(majorAxis);	//椭圆长轴方向
			double radiusRatio = fabs(columnNorm*workNorm);
			double minorRadius = pPlate->m_fInnerRadius;				//椭圆短半轴长度
			double majorRadius = pPlate->m_fInnerRadius / radiusRatio;	//椭圆长半轴长度
			workNorm *= (columnNorm*workNorm < EPS) ? -1 : 1;
			for (int i = 0; i < 4; i++)
			{
				f3dPoint ptS, ptE;
				if (i == 0)
				{
					ptS = center + majorAxis * majorRadius;
					ptE = center + minorAxis * minorRadius;
				}
				else if (i == 1)
				{
					ptS = center + minorAxis * minorRadius;
					ptE = center - majorAxis * majorRadius;
				}
				else if (i == 2)
				{
					ptS = center - majorAxis * majorRadius;
					ptE = center - minorAxis * minorRadius;
				}
				else
				{
					ptS = center - minorAxis * minorRadius;
					ptE = center + majorAxis * majorRadius;
				}
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, plateId, PS_SOLID, crEdge);
				pArcLine->CreateEllipse(center, ptS, ptE, columnNorm, workNorm, minorRadius);
			}
		}
		else
			AppendDbCircle(pDrawing, pPlate->m_tInnerOrigin, f3dPoint(0, 0, 1), pPlate->m_fInnerRadius, plateId, PS_SOLID, crEdge);
	}
	//绘制火曲线
	for (int i = 2; i <= pPlate->m_cFaceN; i++)
	{
		f3dLine line;
		if (pPlate->GetBendLineAt(i - 2, &line) != 0)
		{
			int ID = i - 1;	//第一条火曲线
			HIBERID huoquLineId(pPlate->GetKey(), HIBERARCHY(0, 0, 1, line.ID));
			AppendDbLine(pDrawing, line.startPt, line.endPt, huoquLineId, PS_DASHDOT, crHuoQu);
		}
		else
			logerr.Log("第%d火曲线始末段绘制顶点查找失败", i - 1);
	}
	//螺栓孔
	if (pPlate->m_xBoltInfoList.GetNodeNum() > 0)
	{
		double fSpecialD = (CPEC::GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
		ATOM_LIST<f3dPoint> lsPtList;
		for (BOLT_INFO *pBoltInfo = pPlate->m_xBoltInfoList.GetFirst(); pBoltInfo; pBoltInfo = pPlate->m_xBoltInfoList.GetNext())
			lsPtList.append(f3dPoint(pBoltInfo->posX, pBoltInfo->posY, 0));
		BOLT_INFO* pFirstBolt = pPlate->m_xBoltInfoList.GetFirst();
		f3dPoint pre_ls_pt, cur_ls_pt;
		pre_ls_pt.Set(pFirstBolt->posX, pFirstBolt->posY, 0);
		COLORREF ls_color = color;
		for (BOLT_INFO *pBoltInfo = pPlate->m_xBoltInfoList.GetFirst(); pBoltInfo; pBoltInfo = pPlate->m_xBoltInfoList.GetNext())
		{
			f3dCircle circle;
			circle.norm.Set(0, 0, 1);
			circle.centre.Set(pBoltInfo->posX, pBoltInfo->posY, 0);
			circle.radius = (pBoltInfo->bolt_d + pBoltInfo->hole_d_increment) / 2;
			circle.ID = pBoltInfo->hiberId.HiberDownId(2);
			pBoltInfo->hiberId.masterId = pPlate->GetKey();
			CXhChar100 sEnter;
			if (pBoltInfo->bolt_d == 12 && pBoltInfo->cFuncType == 0)
				sEnter = "M12Color";
			else if (pBoltInfo->bolt_d == 16 && pBoltInfo->cFuncType == 0)
				sEnter = "M16Color";
			else if (pBoltInfo->bolt_d == 20 && pBoltInfo->cFuncType == 0)
				sEnter = "M20Color";
			else if (pBoltInfo->bolt_d == 24 && pBoltInfo->cFuncType == 0)
				sEnter = "M24Color";
			else
				sEnter = "OtherColor";
			if (CPEC::GetSysParaFromReg(sEnter, sValue))
			{
				char tem_str[100] = "";
				sprintf(tem_str, "%s", (char*)sValue);
				memmove(tem_str, tem_str + 3, 97);
				sscanf(tem_str, "%X", &ls_color);
			}
			if (ciDisplayNcMode&0X01 || ciDisplayNcMode&0X02)
			{	//切割下料模式下，显示切割孔
				if (pBoltInfo->bolt_d >= fSpecialD)
					AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, pBoltInfo->hiberId, PS_SOLID, ls_color, 2);
			}
			if (ciDisplayNcMode&0x04 || ciDisplayNcMode&0x08)
			{	//板床打孔模式下
				BOOL bNeedSH = FALSE;
				if (ciDisplayNcMode&0x04 && CPEC::GetSysParaFromReg("PunchNeedSH", sValue))
					bNeedSH = atoi(sValue);	//冲孔考虑是否保留特殊大孔
				if (ciDisplayNcMode&0x08 && CPEC::GetSysParaFromReg("DrillNeedSH", sValue))
					bNeedSH = atoi(sValue);	//钻孔考虑是否保留特殊大孔
				if (pBoltInfo->cFuncType == 0)
					AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, pBoltInfo->hiberId, PS_SOLID, ls_color, 2);
				else if (pBoltInfo->bolt_d < fSpecialD)	//对小号特殊孔进行加工
					AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, pBoltInfo->hiberId, PS_SOLID, ls_color, 2);
				else if (pBoltInfo->bolt_d >= fSpecialD && bNeedSH && (ciDisplayNcMode == 0x04 || ciDisplayNcMode == 0x08))
					AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, pBoltInfo->hiberId, PS_SOLID, ls_color, 2);
			}
			if (ciDisplayNcMode == 0x10 || ciDisplayNcMode == 0)
			{	//原始或激光加工模式下，生成所有孔
				AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, pBoltInfo->hiberId, PS_SOLID, ls_color, 2);
			}
			//显示螺栓顺序
			BOOL bNeedDispBoltOrder = FALSE;
			if (ciDisplayNcMode&0x04)
			{	//冲孔是否显示特殊大孔（输出且参与排序）
				BOOL bNeedSH = FALSE;
				if (CPEC::GetSysParaFromReg("PunchNeedSH", sValue))
					bNeedSH = atoi(sValue);
				if (bNeedSH && CPEC::GetSysParaFromReg("PunchSortHasBigSH", sValue))
					bNeedSH = atoi(sValue);
				//
				if (pBoltInfo->bolt_d < fSpecialD)
					bNeedDispBoltOrder = TRUE;
				else if (bNeedSH)
					bNeedDispBoltOrder = TRUE;
			}
			if (ciDisplayNcMode&0x08)
			{	//钻孔是否显示特殊大孔（输出且参与排序）
				BOOL bNeedSH = FALSE;
				if (CPEC::GetSysParaFromReg("DrillNeedSH", sValue))
					bNeedSH = atoi(sValue);
				if (bNeedSH && CPEC::GetSysParaFromReg("DrillSortHasBigSH", sValue))
					bNeedSH = atoi(sValue);
				//
				if (pBoltInfo->bolt_d < fSpecialD)
					bNeedDispBoltOrder = TRUE;
				else if (bNeedSH)
					bNeedDispBoltOrder = TRUE;
			}
			if (CProcessPartDraw::m_bDispBoltOrder && bNeedDispBoltOrder)
			{
				cur_ls_pt = circle.centre;
				double fTextHeight = (CPEC::GetSysParaFromReg("TextHeight", sValue)) ? atof(sValue) : 10;
				CXhChar50 text("%d", pBoltInfo->keyId);
				AppendDbText(pDrawing, cur_ls_pt, text, 0, fTextHeight, IDbText::AlignMiddleCenter, plateId, 0, crText, 2);
				if (pBoltInfo != pFirstBolt)
					AppendDbLine(pDrawing, pre_ls_pt, cur_ls_pt, HIBERID(pPlate->GetKey()), PS_DASH, crText);
			}
			pre_ls_pt = cur_ls_pt;
		}
	}
	if (ciDisplayNcMode&0X01 || ciDisplayNcMode&0X02)
	{	//火焰切割或等离子切割时，绘制切割路径
		pVertex = pPlate->vertex_list.GetValue(pPlate->m_xCutPt.hEntId);
		if (bDrawCutPt&&pVertex)
		{
			HIBERID hiberId = pPlate->m_xCutPt.GetHiberId(pPlate->GetKey());
			HIBERID inLineHiberId = pPlate->m_xCutPt.GetInLineHiberId(pPlate->GetKey());
			HIBERID outLineHiberId = pPlate->m_xCutPt.GetOutLineHiberId(pPlate->GetKey());
			f3dCircle circle;
			circle.norm.Set(0, 0, 1);
			circle.centre.Set(pVertex->vertex.x, pVertex->vertex.y, 0);
			circle.radius = 3;
			AppendDbCircle(pDrawing, circle.centre, circle.norm, circle.radius, hiberId, PS_SOLID, RGB(255, 0, 0), 4);
			int n = pPlate->vertex_list.GetNodeNum();
			long prev_index = (pPlate->m_xCutPt.hEntId == n) ? 1 : pPlate->m_xCutPt.hEntId + 1;
			long next_index = (pPlate->m_xCutPt.hEntId == 1) ? n : pPlate->m_xCutPt.hEntId - 1;
			PROFILE_VER *pPrevVertex = pPlate->vertex_list.GetValue(prev_index), *pNextVertex = pPlate->vertex_list.GetValue(next_index);
			if (pPrevVertex&&pNextVertex)
			{
				GEPOINT prev_vec = pVertex->vertex - pPrevVertex->vertex, next_vec = pVertex->vertex - pNextVertex->vertex;
				normalize(prev_vec);
				normalize(next_vec);
				int nInLineLen = pPlate->m_xCutPt.cInLineLen, nOutLineLen = pPlate->m_xCutPt.cOutLineLen;
				if (nOutLineLen > 0)
					AppendDbLine(pDrawing, pVertex->vertex, pVertex->vertex + nOutLineLen * next_vec, outLineHiberId, PS_SOLID, RGB(255, 0, 0), 4);
				if (nInLineLen > 0)
					AppendDbLine(pDrawing, pVertex->vertex, pVertex->vertex + nInLineLen * prev_vec, inLineHiberId, PS_SOLID, RGB(0, 255, 0), 4);
			}
		}
	}
}
static void DrawPlate(CProcessPlate *pPlate,IDrawing *pDrawing,ISolidSet *pSolidSet,COLORREF color,BOOL bDrawCutPt=FALSE)
{
	if(pPlate==NULL||pSolidSet==NULL||pDrawing==NULL)
		return;
	HIBERID plateId(pPlate->GetKey());
	if(pPlate->m_cFaceN==3 && !(pPlate->top_point.IsZero()))
		AppendDbPoint(pDrawing,pPlate->top_point,plateId);
	//获取颜色设置
	char sValue[MAX_PATH] = "", tem_str[100] = "";
	COLORREF crEdge = color, crText = color, crHuoQu = color;
	if (CPEC::GetSysParaFromReg("EdgeColor", sValue))
	{
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crEdge);
	}
	if (CPEC::GetSysParaFromReg("TextColor", sValue))
	{
		char tem_str[100] = "";
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crText);
	}
	if (CPEC::GetSysParaFromReg("HuoQuColor", sValue))
	{
		sprintf(tem_str, "%s", sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crHuoQu);
	}
	//绘制直线
	int i = 0;
	PROFILE_VER *pVertex=NULL,*pPrevPnt=pPlate->vertex_list.GetTail(),*pPrevPrevPnt=pPlate->vertex_list.GetPrev();
	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext(),i++)
	{
		pVertex->hiberId.masterId=pPlate->GetKey();
		pPrevPnt->hiberId.masterId=pPlate->GetKey();
		if(pPlate->mcsFlg.ciBottomEdge!=i)
			AppendDbPoint(pDrawing,pVertex->vertex,pVertex->hiberId,PS_SOLID,crEdge);
		else
			AppendDbPoint(pDrawing,pVertex->vertex,pVertex->hiberId,PS_SOLID,RGB(127, 255, 0),8);
		//轮廓线
		int nPenWidth = pPrevPnt->m_bWeldEdge ? 2 : 1;	//焊接边加粗显示(江电通过晓仲提出 wjh-2017.1.16)
		if((pPrevPrevPnt&&pPrevPrevPnt->m_bRollEdge&&pPrevPrevPnt->manu_space!=0)||
			(pPrevPnt&&pPrevPnt->m_bRollEdge&&pPrevPnt->manu_space!=0)||pVertex->m_bRollEdge&&pVertex->manu_space!=0)
		{	//有卷边工艺时，此处不绘制与卷边工艺轮廓边相连接的前后两条轮廓边（共三条边）
			pPrevPrevPnt=pPrevPnt;
			pPrevPnt=pVertex;
			continue;
		}
		if(pPlate->m_cFaceN<3||pPrevPnt->vertex.feature!=3||pVertex->vertex.feature!=2)
		{
			if (pPrevPnt->type == 2)
			{	//圆弧
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
				pArcLine->CreateMethod2(pPrevPnt->vertex, pVertex->vertex, pPrevPnt->work_norm, pPrevPnt->sector_angle);
			}
			else if (pPrevPnt->type == 3)	
			{	//椭圆弧
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
				pArcLine->CreateEllipse(pPrevPnt->center, pPrevPnt->vertex, pVertex->vertex, pPrevPnt->column_norm,
					pPrevPnt->work_norm, pPrevPnt->radius);
			}
			else	//直线
				AppendDbLine(pDrawing, pPrevPnt->vertex, pVertex->vertex, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
		}
		else
		{
			AppendDbLine(pDrawing, pPrevPnt->vertex, pPlate->top_point, pPrevPnt->hiberId, PS_SOLID, crEdge, nPenWidth);
			AppendDbLine(pDrawing, pPlate->top_point, pVertex->vertex, pVertex->hiberId, PS_SOLID, crEdge, nPenWidth);
		}
		pPrevPrevPnt=pPrevPnt;
		pPrevPnt = pVertex;
	}
	//绘制内圆
	if (pPlate->m_fInnerRadius > 0)
	{
		if (!pPlate->m_tInnerColumnNorm.IsZero() && fabs(pPlate->m_tInnerColumnNorm*f3dPoint(0, 0, 1)) < EPS_COS)
		{	//椭圆
			f3dPoint center = pPlate->m_tInnerOrigin, workNorm(0, 0, 1), columnNorm = pPlate->m_tInnerColumnNorm;
			f3dPoint minorAxis = columnNorm ^ workNorm;
			normalize(minorAxis);	//椭圆短轴方向
			f3dPoint majorAxis(-minorAxis.y, minorAxis.x, minorAxis.z);
			normalize(majorAxis);	//椭圆长轴方向
			double radiusRatio = fabs(columnNorm*workNorm);
			double minorRadius = pPlate->m_fInnerRadius;				//椭圆短半轴长度
			double majorRadius = pPlate->m_fInnerRadius / radiusRatio;	//椭圆长半轴长度
			workNorm *= (columnNorm*workNorm < EPS) ? -1 : 1;
			for (int i = 0; i < 4; i++)
			{
				f3dPoint ptS, ptE;
				if (i == 0)
				{
					ptS = center + majorAxis * majorRadius;
					ptE = center + minorAxis * minorRadius;
				}
				else if (i == 1)
				{
					ptS = center + minorAxis * minorRadius;
					ptE = center - majorAxis * majorRadius;
				}
				else if (i == 2)
				{
					ptS = center - majorAxis * majorRadius;
					ptE = center - minorAxis * minorRadius;
				}
				else
				{
					ptS = center - minorAxis * minorRadius;
					ptE = center + majorAxis * majorRadius;
				}
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, plateId, PS_SOLID, crEdge);
				pArcLine->CreateEllipse(center, ptS, ptE, columnNorm, workNorm, minorRadius);
			}
		}
		else
			AppendDbCircle(pDrawing, pPlate->m_tInnerOrigin, f3dPoint(0,0,1), pPlate->m_fInnerRadius, plateId, PS_SOLID, crEdge);
	}
	//绘制火曲线
	for(int i=2;i<=pPlate->m_cFaceN;i++)
	{
		f3dLine line;
		if(pPlate->GetBendLineAt(i-2,&line)!=0)
		{
			int ID=i-1;	//第一条火曲线
			HIBERID huoquLineId(pPlate->GetKey(),HIBERARCHY(0,0,1,line.ID));
			AppendDbLine(pDrawing, line.startPt, line.endPt, huoquLineId, PS_DASHDOT, crHuoQu);
		}
		else
			logerr.Log("第%d火曲线始末段绘制顶点查找失败",i-1);
	}
	//螺栓孔
	if(pPlate->m_xBoltInfoList.GetNodeNum()>0)
	{
		ATOM_LIST<f3dPoint> lsPtList;
		for(BOLT_INFO *pBoltInfo=pPlate->m_xBoltInfoList.GetFirst();pBoltInfo;pBoltInfo=pPlate->m_xBoltInfoList.GetNext())
			lsPtList.append(f3dPoint(pBoltInfo->posX,pBoltInfo->posY,0));
		BOLT_INFO* pFirstBolt=pPlate->m_xBoltInfoList.GetFirst();
		f3dPoint pre_ls_pt,cur_ls_pt;
		pre_ls_pt.Set(pFirstBolt->posX,pFirstBolt->posY,0);
		COLORREF ls_color=color;
		for(BOLT_INFO *pBoltInfo=pPlate->m_xBoltInfoList.GetFirst();pBoltInfo;pBoltInfo=pPlate->m_xBoltInfoList.GetNext())
		{
			f3dCircle circle;
			circle.norm.Set(0,0,1);
			cur_ls_pt.Set(pBoltInfo->posX,pBoltInfo->posY,0);
			circle.centre=cur_ls_pt;
			circle.radius = (pBoltInfo->bolt_d+pBoltInfo->hole_d_increment)/2;
			circle.ID	  = pBoltInfo->hiberId.HiberDownId(2);
			pBoltInfo->hiberId.masterId=pPlate->GetKey();
			//只有特殊孔才可能是小数，特殊孔也查不到合适的颜色，此处可强制转为整数 wht 19-09-12
			CXhChar100 sEnter;
			if (pBoltInfo->bolt_d == 12 && pBoltInfo->cFuncType == 0)
				sEnter = "M12Color";
			else if (pBoltInfo->bolt_d == 16 && pBoltInfo->cFuncType == 0)
				sEnter = "M16Color";
			else if (pBoltInfo->bolt_d == 20 && pBoltInfo->cFuncType == 0)
				sEnter = "M20Color";
			else if (pBoltInfo->bolt_d == 24 && pBoltInfo->cFuncType == 0)
				sEnter = "M24Color";
			else
				sEnter = "OtherColor";
			if (CPEC::GetSysParaFromReg(sEnter, sValue))
			{
				char tem_str[100] = "";
				sprintf(tem_str, "%s", (char*)sValue);
				memmove(tem_str, tem_str + 3, 97);
				sscanf(tem_str, "%X", &ls_color);
			}
			if(IsPositionOverlap(cur_ls_pt,lsPtList))	//同一个位置出现多个螺栓孔，特殊标记RGB(123,104,238)
				AppendDbCircle(pDrawing,circle.centre,circle.norm,circle.radius,pBoltInfo->hiberId,PS_SOLID,RGB(127,255,0),3);
			else
				AppendDbCircle(pDrawing,circle.centre,circle.norm,circle.radius,pBoltInfo->hiberId,PS_SOLID,ls_color,2);
			if(CProcessPartDraw::m_bDispBoltOrder)
			{
				CXhChar100 sValue;
				double fTextHeight=10;
				if(CPEC::GetSysParaFromReg("TextHeight",sValue))
					fTextHeight=atof(sValue);
				CXhChar50 text("%d",pBoltInfo->keyId);
				AppendDbText(pDrawing,cur_ls_pt,text,0,fTextHeight,IDbText::AlignMiddleCenter,plateId,0,crText,2);
				if(pBoltInfo!=pFirstBolt)
					AppendDbLine(pDrawing,pre_ls_pt,cur_ls_pt,HIBERID(pPlate->GetKey()),PS_DASH, crText);
			}
			pre_ls_pt=cur_ls_pt;
		}
	}
	//处理卷边
	pPrevPnt=pPlate->vertex_list.GetTail();
	PROFILE_VER* pTailVertex=pPrevPnt;
	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		if(pVertex->m_bRollEdge&&pVertex->manu_space!=0&&pVertex->type==1)
		{
			PROFILE_VER* pRollStart=pVertex,*pRollEnd=NULL,*pNextPnt=NULL;
			int nPush=pPlate->vertex_list.push_stack();
			pRollEnd=(pTailVertex==pVertex)?pPlate->vertex_list.GetFirst():pPlate->vertex_list.GetNext();
			pNextPnt=(pTailVertex==pRollEnd)?pPlate->vertex_list.GetFirst(): pPlate->vertex_list.GetNext();
			pPlate->vertex_list.pop_stack(nPush);
			if(pRollStart==NULL||pRollEnd==NULL||pPrevPnt==NULL||pNextPnt==NULL)
				continue;
			//计算进行卷边工艺的火曲线和卷边线
			f3dLine huoqu_line,roll_edge_line,line;
			pPlate->CalRollLineAndHuoquLine(pRollStart,pRollEnd,pPrevPnt,pNextPnt,huoqu_line,roll_edge_line);
			//绘制卷边特征点
			AppendDbPoint(pDrawing,roll_edge_line.startPt,plateId,PS_SOLID,color);
			AppendDbPoint(pDrawing,roll_edge_line.endPt,plateId,PS_SOLID,color);
			AppendDbPoint(pDrawing,huoqu_line.startPt,plateId,PS_SOLID,color);
			AppendDbPoint(pDrawing,huoqu_line.endPt,plateId,PS_SOLID,color);
			//绘制卷边特征线
			if(pRollStart->vertex.feature>10&&!pRollStart->vertex.IsEqual(huoqu_line.startPt))
			{
				AppendDbLine(pDrawing,pPrevPnt->vertex,pRollStart->vertex,pPrevPnt->hiberId,PS_SOLID,color);
				AppendDbLine(pDrawing,pRollStart->vertex,huoqu_line.startPt,pPrevPnt->hiberId,PS_SOLID,color);
			}
			else
				AppendDbLine(pDrawing,pPrevPnt->vertex,huoqu_line.startPt,pPrevPnt->hiberId,PS_SOLID,color);
			AppendDbLine(pDrawing,huoqu_line.startPt,roll_edge_line.startPt,plateId,PS_SOLID,color);
			AppendDbLine(pDrawing,roll_edge_line.startPt,roll_edge_line.endPt,plateId,PS_SOLID,color);
			AppendDbLine(pDrawing,roll_edge_line.endPt,huoqu_line.endPt,plateId,PS_SOLID,color);
			if(pRollEnd->vertex.feature>10&&!pRollEnd->vertex.IsEqual(huoqu_line.endPt))
			{
				AppendDbLine(pDrawing,huoqu_line.endPt,pRollEnd->vertex,plateId,PS_SOLID,color);
				AppendDbLine(pDrawing,pRollEnd->vertex,pNextPnt->vertex,plateId,PS_SOLID,color);
			}
			else
				AppendDbLine(pDrawing,huoqu_line.endPt,pNextPnt->vertex,plateId,PS_SOLID,color);
			if(pRollStart->vertex.IsEqual(huoqu_line.startPt)&&pRollEnd->vertex.IsEqual(huoqu_line.endPt))
			{	//火曲线与有卷边工艺信息的轮廓边重合，只需绘制火曲线即可
				AppendDbLine(pDrawing,pRollStart->vertex,pRollEnd->vertex,pRollStart->hiberId,PS_DASHDOTDOT,RGB(255,0,0));
			}
			else
			{	//火曲线与有卷边工艺信息的轮廓边不重合，需分别绘制轮廓边线和制弯线
				AppendDbLine(pDrawing,pRollStart->vertex,pRollEnd->vertex,pRollStart->hiberId,PS_DASHDOT,color);
				AppendDbLine(pDrawing,huoqu_line.startPt,huoqu_line.endPt,plateId,PS_DASHDOTDOT,RGB(255,0,0));
			}
		}
		pPrevPnt=pVertex;
	}
}
static void DrawNcBackGraphic(CProcessPlate *pPlate,IDrawing *pDrawing,ISolidSet *pSolidSet,int shieldHeight)
{
	//1.绘制坐标系
	SCOPE_STRU scope=pPlate->GetVertexsScope();
	double axis_len=max(scope.wide(),scope.high())+20;
	f3dPoint original;
	HIBERID plateId(pPlate->GetKey());
	IDbArrow *pArrow=AppendDbArrow(pDrawing,original,f3dPoint(1,0,0),axis_len,10,plateId,PS_SOLID,RGB(255,0,0),1);
	pArrow->SetSelectable(false);
	if(axis_len<shieldHeight)
		axis_len=shieldHeight+20;
	pArrow=AppendDbArrow(pDrawing,original,f3dPoint(0,1,0),axis_len,10,plateId,PS_SOLID,RGB(0,255,0),1);
	pArrow->SetSelectable(false);
	//2.绘制挡板
	double shieldThick=10;
	IDbRect *pRect=AppendDbRect(pDrawing,f3dPoint(-shieldThick,shieldHeight,0),f3dPoint(),plateId,PS_SOLID,RGB(0,0,0),1);
	pRect->SetSelectable(false);
	//3.绘制构件明细
	if(pPlate->vertex_list.GetNodeNum()>=3)
	{
		CXhChar16 matStr = LocalQuerySteelMark(pPlate->cMaterial);
		CXhChar200 sValue,sPartInfo("编号:%s  板厚:%dmm 材质%s",(char*)pPlate->GetPartNo(),(int)pPlate->m_fThick,(char*)matStr);
		double fTextHeight=30;
		if(CPEC::GetSysParaFromReg("TextHeight",sValue))
			fTextHeight=atof(sValue);	//普通文字字高
		f3dPoint dim_pos(0,-1*fTextHeight,0);
		AppendDbText(pDrawing,dim_pos,sPartInfo,0,fTextHeight,IDbText::AlignMiddleLeft,plateId);
	}
}
void CProcessPlateDraw::NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	if (m_pPart == NULL || m_pPart->m_cPartType != CProcessPart::TYPE_PLATE)
		return;
	CXhChar100 sValue;
	//初始化切入点
	CProcessPlate *pPlate = (CProcessPlate*)m_pPart;
	if (pPlate->mkVec.IsZero())
		InitMkRect();
	int nInLineLen = 0, nOutLineLen = 0;
	BOOL bCutPosInInitPos = FALSE, bInitFarOrg = FALSE;
	GetCutParamFromReg(-1, pPlate->m_fThick, &nInLineLen, &nOutLineLen, NULL, &bInitFarOrg, &bCutPosInInitPos);
	if (bCutPosInInitPos)
		pPlate->m_xCutPt.hEntId = 0;
	pPlate->InitCutPt(bInitFarOrg == TRUE);
	pPlate->m_xCutPt.cInLineLen = (BYTE)nInLineLen;
	pPlate->m_xCutPt.cOutLineLen = (BYTE)nOutLineLen;
	//将构件转换到加工坐标系下
	CProcessPlate tempPlate;
	m_pPart->ClonePart(&tempPlate);
	tempPlate.GetMCS(mcs);
	tempPlate.SetKey(m_pPart->GetKey());
#ifdef __PNC_
	//PNC模式下，考虑轮廓边增大值
	BYTE ciDisplayNcMode = (CPEC::GetSysParaFromReg("m_ciDisplayType", sValue)) ? atoi(sValue) : 0;
	double fShapeAddDist = 0;
	if ((ciDisplayNcMode&0X01) && CPEC::GetSysParaFromReg("flameCut.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	if ((ciDisplayNcMode&0X02) && CPEC::GetSysParaFromReg("plasmaCut.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	if (ciDisplayNcMode==0X10 && CPEC::GetSysParaFromReg("laserPara.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	BOOL bGrindingArc = FALSE;
	if ((ciDisplayNcMode & 0X01) && CPEC::GetSysParaFromReg("flameCut.m_bGrindingArc", sValue))
		bGrindingArc = atoi(sValue);
	if ((ciDisplayNcMode & 0X02) && CPEC::GetSysParaFromReg("plasmaCut.m_bGrindingArc", sValue))
		bGrindingArc = atoi(sValue);
	mcs = CNCPlate::GetMCS(tempPlate, fShapeAddDist, bGrindingArc);
	//还原轮廓点
	tempPlate.vertex_list.Empty();
	for (PROFILE_VER* pVertex = pPlate->vertex_list.GetFirst(); pVertex; pVertex = pPlate->vertex_list.GetNext())
		tempPlate.vertex_list.Append(*pVertex, pPlate->vertex_list.GetCursorKey());
	ATOM_LIST<PROFILE_VER> xDestList;
	tempPlate.CalEquidistantShape(fShapeAddDist, &xDestList);
	tempPlate.vertex_list.Empty();
	for (PROFILE_VER* pVertex = xDestList.GetFirst(); pVertex; pVertex = xDestList.GetNext())
		tempPlate.vertex_list.Append(*pVertex);
#endif
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//2.绘制坐标系及挡板高度
	COLORREF color=RGB(0,0,0);
	if(IsMainPartDraw())
	{
		int nShieldHeight=160;
		if(CPEC::GetSysParaFromReg("SideBaffleHigh",sValue))
			nShieldHeight=atoi(sValue);
		DrawNcBackGraphic(&tempPlate,pDrawing,pSolidSet,nShieldHeight);
	}
	else
	{	//绘制第2块钢板的标注信息
		color=RGB(0,255,255);
		CXhChar16 matStr=LocalQuerySteelMark(tempPlate.cMaterial);
		CXhChar200 sValue,sPartInfo("编号:%s  板厚:%dmm 材质%s",(char*)tempPlate.GetPartNo(),(int)tempPlate.m_fThick,(char*)matStr);
		double fTextHeight=30;
		if(CPEC::GetSysParaFromReg("TextHeight",sValue))
			fTextHeight=atof(sValue);	//普通文字字高
		f3dPoint dim_pos(0,-2*fTextHeight,0);
		AppendDbText(pDrawing,dim_pos,sPartInfo,0,fTextHeight,IDbText::AlignMiddleLeft,HIBERID(m_pPart->GetKey()),PS_SOLID,color);
	}
	//3.绘制钢板外形
#ifdef __PNC_
	DrawPlateNc(&tempPlate, pDrawing, pSolidSet, color, TRUE);
#else
	DrawPlate(&tempPlate, pDrawing, pSolidSet, color, TRUE);
#endif
	//4.绘制打号位置
	if (((CProcessPlate*)m_pPart)->IsDisplayMK())
	{
		COLORREF mkColRef = RGB(255, 0, 0);
		if (CPEC::GetSysParaFromReg("MarkColor", sValue))
		{
			char tem_str[100] = "";
			sprintf(tem_str, "%s", (char*)sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &mkColRef);
		}
		if (!IsMainPartDraw())
			mkColRef = RGB(0, 128, 128);
		//号料孔
		double fMKHoleD = 8;
		if (CPEC::GetSysParaFromReg("MKHoleD", sValue))
			fMKHoleD = atof(sValue);
		f3dPoint center(m_pPart->mkpos);
		coord_trans(center, mcs, FALSE);
		AppendDbCircle(pDrawing, center, f3dPoint(0, 0, 1), fMKHoleD*0.5, HIBERID(m_pPart->GetKey()), PS_SOLID, mkColRef, 2);
		//字盒
		BOOL bDisplayMkRect = FALSE;
		if (CPEC::GetSysParaFromReg("DispMkRect", sValue))
			bDisplayMkRect = atoi(sValue);
		if (bDisplayMkRect)
		{
			BYTE ciVectType = (CPEC::GetSysParaFromReg("MKVectType", sValue)) ? atoi(sValue) : 0;
			double fLen = (CPEC::GetSysParaFromReg("MKRectL", sValue)) ? atof(sValue) : 60;
			double fWidth = (CPEC::GetSysParaFromReg("MKRectW", sValue)) ? atof(sValue) : 30;
			if (ciVectType == 1)
			{	//保持水平
				f3dPoint vec(1, 0, 0);
				vector_trans(vec, mcs, TRUE);
				tempPlate.mkVec = vec;
			}
			GEPOINT ptArr[4];
			tempPlate.GetMkRect(fLen, fWidth, ptArr);
			for (int i = 0; i < 4; i++)
				coord_trans(ptArr[i], mcs, FALSE);
			AppendDbRect(pDrawing, ptArr[0], ptArr[2], HIBERID(m_pPart->GetKey()), PS_SOLID, mkColRef, 2);
		}
	}
}
void CProcessPlateDraw::ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	EditorModelDraw(pDrawing,pSolidSet);
}
void CProcessPlateDraw::EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet,COLORREF color/*=RGB(0,0,0)*/,COLORREF boltColor/*=RGB(255,0,0)*/)
{
	DrawPlate((CProcessPlate*)m_pPart,pDrawing,pSolidSet,IsMainPartDraw()?RGB(0,0,0):RGB(0,255,255));
}

void CProcessPlateDraw::DrawCuttingTrack(I2dDrawing *p2dDraw,ISolidSet *pSolidSet,double interval/*=0.5*/)
{
#ifdef __PNC_
	if(m_pPart==NULL||!m_pPart->IsPlate())
		return;
	IDrawing *pDrawing=p2dDraw?p2dDraw->GetActiveDrawing():NULL;
	if(pDrawing==NULL)
		return;
	//删除HiberId为0的实体
	for(IDbEntity* pEnt=pDrawing->EnumFirstDbEntity();pEnt;pEnt=pDrawing->EnumNextDbEntity())
	{
		if(pEnt->GetHiberId().IsEqual(HIBERID(0)))
			pDrawing->DeleteDbEntity(pEnt->GetId());
	}
	CNCPlate ncPlate((CProcessPlate*)m_pPart);
	//初始化切割路径
	CXhChar100 sValue;
	BYTE cCutType = (CPEC::GetSysParaFromReg("m_ciDisplayType", sValue)) ? atoi(sValue) : 0;
	if (cCutType & 0X01)
	{	//火焰切割(逆时针)
		ncPlate.m_bClockwise = FALSE;
		if (CPEC::GetSysParaFromReg("flameCut.m_sOutLineLen", sValue))
			ncPlate.m_nOutLineLen = GetLineLenFromExpression(m_pPart->m_fThick, sValue);
		if (CPEC::GetSysParaFromReg("flameCut.m_sIntoLineLen", sValue))
			ncPlate.m_nInLineLen = GetLineLenFromExpression(m_pPart->m_fThick, sValue);
		if (CPEC::GetSysParaFromReg("flameCut.m_wEnlargedSpace", sValue))
			ncPlate.m_nEnlargedSpace = atoi(sValue);
		if (CPEC::GetSysParaFromReg("flameCut.m_bCutSpecialHole", sValue))
			ncPlate.m_bCutSpecialHole = atoi(sValue);
		if (CPEC::GetSysParaFromReg("flameCut.m_bGrindingArc", sValue))
			ncPlate.m_bGrindingArc = atoi(sValue);
	}
	else if (cCutType & 0X02)
	{	//等离子切割（顺时针）,目前生成NC数据时不考虑轮廓边增大值和特殊孔  wxc-20.05.22
		ncPlate.m_bClockwise = TRUE;
		if (CPEC::GetSysParaFromReg("plasmaCut.m_sOutLineLen", sValue))
			ncPlate.m_nOutLineLen = GetLineLenFromExpression(m_pPart->m_fThick, sValue);
		if (CPEC::GetSysParaFromReg("plasmaCut.m_sIntoLineLen", sValue))
			ncPlate.m_nInLineLen = GetLineLenFromExpression(m_pPart->m_fThick, sValue);
		if (CPEC::GetSysParaFromReg("plasmaCut.m_wEnlargedSpace", sValue))
			ncPlate.m_nEnlargedSpace = atoi(sValue);
		if (CPEC::GetSysParaFromReg("plasmaCut.m_bCutSpecialHole", sValue))
			ncPlate.m_bCutSpecialHole = atoi(sValue);
		if (CPEC::GetSysParaFromReg("plasmaCut.m_bGrindingArc", sValue))
			ncPlate.m_bGrindingArc = atoi(sValue);
	}
	else
		return;
	ncPlate.InitPlateNcInfo();
	//绘制特殊孔切割路径
	COLORREF cutLineClr = RGB(255, 0, 0), otherLineClr = RGB(0, 0, 0), ptClr = RGB(181, 0, 181);
	GEPOINT ptS = ncPlate.GetCutStartPt(), ptE;
	for (CNCPlate::CUT_HOLE_PATH* pHolePath = ncPlate.m_xCutHole.GetFirst(); pHolePath; pHolePath = ncPlate.m_xCutHole.GetNext())
	{
		ptE = ptS+pHolePath->ignitionPt;
		AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
		AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
		p2dDraw->RenderDrawing();
		Sleep(ftol(interval * 1000));
		ptS = ptE;
		if (ncPlate.m_bCutFullCircle)
		{
			CUT_PT* pCutPt = pHolePath->cutPtArr.GetFirst();
			if(pCutPt==NULL)
				continue;
			ptE = ptS + pCutPt->vertex;
			GEPOINT center(ptE.x + pCutPt->radius, ptE.y);
			AppendDbCircle(pDrawing, center, GEPOINT(0, 0, 1), pCutPt->radius, 0, PS_SOLID, ptClr, 3);
			p2dDraw->RenderDrawing();
			Sleep(ftol(interval * 1000));
			ptS = ptE;
		}
		else
		{
			for (CUT_PT* pCutPt = pHolePath->cutPtArr.GetFirst(); pCutPt; pCutPt = pHolePath->cutPtArr.GetNext())
			{
				ptE = ptS + pCutPt->vertex;
				GEPOINT center = ptS + pCutPt->center;
				GEPOINT norm = pCutPt->bClockwise ? GEPOINT(0, 0, -1) : GEPOINT(0, 0, 1);
				double radius = (pCutPt->radius > 0) ? pCutPt->radius : DISTANCE(center, ptE);
				IDbArcline *pArcLine = AppendDbArcLine(pDrawing, 0, PS_SOLID, cutLineClr, 3);
				pArcLine->CreateMethod3(ptS, ptE, norm, radius, center);
				p2dDraw->RenderDrawing();
				Sleep(ftol(interval * 1000));
				ptS = ptE;
			}
		}
	}
	//绘制额外引出线
	if (ncPlate.m_nExtraInLen>0)
	{	
		ptE = ptS + ncPlate.m_xCutEdge.extraInVertex;
		AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
		AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
		p2dDraw->RenderDrawing();
		Sleep(ftol(interval * 1000));
		ptS = ptE;
	}
	//绘制引出线
	ptE = ptS + ncPlate.m_xCutEdge.cutInVertex;
	AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
	AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
	p2dDraw->RenderDrawing();
	Sleep(ftol(interval * 1000));
	ptS = ptE;
	//绘制轮廓边
	for(CUT_PT *pPt=ncPlate.m_xCutEdge.cutPtArr.GetFirst();pPt;pPt= ncPlate.m_xCutEdge.cutPtArr.GetNext())
	{
		ptE = ptS + pPt->vertex;
		if(pPt->cByte==CUT_PT::EDGE_LINE)
		{
			AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, cutLineClr, 3);
			AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);	
		}
		else if (pPt->cByte == CUT_PT::EDGE_ARC)
		{
			GEPOINT center = ptS + pPt->center;
			GEPOINT norm = pPt->bClockwise ? GEPOINT(0, 0, -1) : GEPOINT(0, 0, 1);
			double radius = (pPt->radius > 0) ? pPt->radius : DISTANCE(center, ptE);
			IDbArcline *pArcLine = AppendDbArcLine(pDrawing, 0, PS_SOLID, cutLineClr, 3);
			pArcLine->CreateMethod3(ptS, ptE, norm, radius, center);
			AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
		}
		p2dDraw->RenderDrawing();
		Sleep(ftol(interval * 1000));
		ptS = ptE;
	}
	//绘制引出线
	ptE = ptS + ncPlate.m_xCutEdge.cutOutVertex;
	AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
	AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
	p2dDraw->RenderDrawing();
	Sleep(ftol(interval*1000));
	ptS = ptE;
	//绘制额外引出线
	if (ncPlate.m_nExtraOutLen > 0)
	{
		ptE = ptS + ncPlate.m_xCutEdge.extraOutVertex;
		AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
		AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
		p2dDraw->RenderDrawing();
		Sleep(ftol(interval * 1000));
		ptS = ptE;
	}
	//回到起始点
	ptE = ptS + ncPlate.m_xCutEdge.cutStartVertex;
	AppendDbLine(pDrawing, ptS, ptE, 0, PS_SOLID, otherLineClr, 3);
	AppendDbPoint(pDrawing, ptE, 0, PS_SOLID, ptClr, 8);
	p2dDraw->RenderDrawing();
	Sleep(ftol(interval*1000));
#endif
}
void CProcessPlateDraw::InitMkRect()
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	f3dPoint vec(1,0,0);
	vector_trans(vec,mcs,TRUE);
	pPlate->mkVec=vec;
}
//NC模式需要的函数
void CProcessPlateDraw::RotateAntiClockwise() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	int n=pPlate->vertex_list.GetNodeNum();
	if(n<=3)
		return;
	pPlate->mcsFlg.ciBottomEdge=(pPlate->mcsFlg.ciBottomEdge-1+n)%n;
	if(pPlate->IsConcaveVertex(pPlate->mcsFlg.ciBottomEdge))
		pPlate->mcsFlg.ciBottomEdge=(pPlate->mcsFlg.ciBottomEdge-1+n)% n;
	pPlate->GetMCS(mcs);
#ifdef __PNC_
	BOOL bCutPosInInitPos=FALSE,bInitFarOrg=FALSE;
	GetCutParamFromReg(-1,0,NULL,NULL,NULL,&bInitFarOrg,&bCutPosInInitPos);
	if(bCutPosInInitPos)
		pPlate->m_xCutPt.hEntId=0;
	pPlate->InitCutPt(bInitFarOrg==TRUE);
#endif
}

void CProcessPlateDraw::RotateClockwise() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	int n=pPlate->vertex_list.GetNodeNum();
	if(n<=3)
		return;
	pPlate->mcsFlg.ciBottomEdge=(pPlate->mcsFlg.ciBottomEdge+1)%n;
	if(pPlate->IsConcaveVertex(pPlate->mcsFlg.ciBottomEdge))
		pPlate->mcsFlg.ciBottomEdge = (pPlate->mcsFlg.ciBottomEdge+1)%n;
	pPlate->GetMCS(mcs);
#ifdef __PNC_
	BOOL bCutPosInInitPos=FALSE,bInitFarOrg=FALSE;
	GetCutParamFromReg(-1,0,NULL,NULL,NULL,&bInitFarOrg,&bCutPosInInitPos);
	if(bCutPosInInitPos)
		pPlate->m_xCutPt.hEntId=0;
	pPlate->InitCutPt(bInitFarOrg==TRUE);
#endif
}

void CProcessPlateDraw::OverturnPart() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	m_bOverturn=pPlate->mcsFlg.ciOverturn;
	m_bOverturn=!m_bOverturn;
	pPlate->mcsFlg.ciOverturn=m_bOverturn;
	pPlate->GetMCS(mcs);
#ifdef __PNC_
	BOOL bCutPosInInitPos=FALSE,bInitFarOrg=FALSE;
	GetCutParamFromReg(-1,0,NULL,NULL,NULL,&bInitFarOrg,&bCutPosInInitPos);
	if(bCutPosInInitPos)
		pPlate->m_xCutPt.hEntId=0;
	pPlate->InitCutPt(bInitFarOrg==TRUE);
#endif
}
static IDbEntity* GetLineByHiberId(IDrawing* pDrawing,HIBERARCHY hiberId)
{
	if(pDrawing==NULL)
		return NULL;
	IDbEntity* pEnt=NULL;
	for(pEnt=pDrawing->EnumFirstDbEntity();pEnt;pEnt=pDrawing->EnumNextDbEntity())
	{
		if(pEnt->GetDbEntType()==IDbEntity::DbLine && hiberId.IsEqual(pEnt->GetHiberId()))
			break;
		else if(pEnt->GetDbEntType()==IDbEntity::DbArcline && hiberId.IsEqual(pEnt->GetHiberId()))
			break;
	}
	return pEnt;
}
void CProcessPlateDraw::ReDrawEdge(IDrawing* pDrawing,ISolidDraw* pSolidDraw,PROFILE_VER* pStartVer,PROFILE_VER* pEndVer)
{
	WORD wDrawMode=m_pBelongEditor->GetDrawMode();
	f3dPoint startPt,endPt,workNorm,columnNorm,centerPt;
	IDbEntity* pEnt=GetLineByHiberId(pDrawing,pStartVer->hiberId);
	if(pEnt==NULL)
		return;
	if(pStartVer->type==1)	//直线
	{
		IDbLine *pLine=NULL;
		startPt=pStartVer->vertex;
		endPt=pEndVer->vertex;
		if(wDrawMode==IPEC::DRAW_MODE_NC)
		{
			coord_trans(startPt,mcs,FALSE);
			coord_trans(endPt,mcs,FALSE);
		}
		if(pEnt->GetDbEntType()!=IDbEntity::DbLine)
		{
			pLine=AppendDbLine(pDrawing,startPt,endPt,pEnt->GetHiberId());
			pSolidDraw->SetEntSnapStatus(pLine->GetId(),true);
			pSolidDraw->SetEntSnapStatus(pEnt->GetId(),false);
			pDrawing->DeleteDbEntity(pEnt->GetId());	//删除直线图元
		}
		else
		{
			pLine=(IDbLine*)pEnt;
			pLine->SetStartPosition(startPt);
			pLine->SetEndPosition(endPt);
		}
	}
	else if(pStartVer->type==2)	//圆弧
	{
		if(pStartVer->sector_angle==0||pStartVer->work_norm.IsZero())
			return;
		startPt=pStartVer->vertex;
		endPt=pEndVer->vertex;
		workNorm=pStartVer->work_norm;
		if(wDrawMode==IPEC::DRAW_MODE_NC)
		{
			coord_trans(startPt,mcs,FALSE);
			coord_trans(endPt,mcs,FALSE);
			vector_trans(workNorm,mcs,FALSE);
		}
		IDbArcline* pArcLine=NULL;
		if(pEnt->GetDbEntType()!=IDbEntity::DbArcline)
		{
			pArcLine=AppendDbArcLine(pDrawing,pEnt->GetHiberId());
			pArcLine->CreateMethod2(startPt,endPt,workNorm,pStartVer->sector_angle);
			pSolidDraw->SetEntSnapStatus(pArcLine->GetId(),true);	//新增图元设置为选中项
			pSolidDraw->SetEntSnapStatus(pEnt->GetId(),false);		//	
			pDrawing->DeleteDbEntity(pEnt->GetId());	//删除直线图元
		}
		else
		{
			pArcLine=(IDbArcline*)pEnt;
			pArcLine->CreateMethod2(startPt,endPt,workNorm,pStartVer->sector_angle);
		}
	}
	else if(pStartVer->type==3)	//椭圆弧
	{
		if(pStartVer->sector_angle==0||pStartVer->work_norm.IsZero()||pStartVer->radius==0
			&&pStartVer->center.IsZero()&&pStartVer->column_norm.IsZero())
			return;
		startPt=pStartVer->vertex;
		endPt=pEndVer->vertex;
		centerPt=pStartVer->center;
		workNorm=pStartVer->work_norm;
		columnNorm=pStartVer->column_norm;
		if(wDrawMode==IPEC::DRAW_MODE_NC)
		{
			coord_trans(startPt,mcs,FALSE);
			coord_trans(endPt,mcs,FALSE);
			coord_trans(centerPt,mcs,FALSE);
			vector_trans(workNorm,mcs,FALSE);
			vector_trans(columnNorm,mcs,FALSE);
		}
		IDbArcline* pArcLine=NULL;
		if(pEnt->GetDbEntType()!=IDbEntity::DbArcline)
		{
			pArcLine=AppendDbArcLine(pDrawing,pEnt->GetHiberId());
			pArcLine->CreateEllipse(centerPt,startPt,endPt,columnNorm,workNorm,pStartVer->radius);
			pSolidDraw->SetEntSnapStatus(pArcLine->GetId(),true);	//新增图元设置为选中项
			pSolidDraw->SetEntSnapStatus(pEnt->GetId(),false);		//	
			pDrawing->DeleteDbEntity(pEnt->GetId());	//删除直线图元
		}
		else
		{
			pArcLine=(IDbArcline*)pEnt;
			pArcLine->CreateEllipse(centerPt,startPt,endPt,columnNorm,workNorm,pStartVer->radius);
		}
	}
}

static void RedrawCutPtEnt(CProcessPlate *pPlate,IDrawing *pDrawing,IDbEntity *pEnt)
{
	if(pPlate==NULL||pDrawing==NULL||pEnt==NULL)
		return;
	CUT_POINT *pCutPt=NULL;
	IDbLine *pOutLine=NULL,*pInLine=NULL;
	if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
	{
		pCutPt=pPlate->FromCutPtHiberId(pEnt->GetHiberId());
		if(pCutPt)
		{
			HIBERID inLineId=pCutPt->GetInLineHiberId(pPlate->GetKey()),outLineId=pCutPt->GetOutLineHiberId(pPlate->GetKey());
			for(IDbEntity *pEnt=pDrawing->Database()->EnumFirstDbEntity();pEnt;pEnt=pDrawing->Database()->EnumNextDbEntity())
			{
				if(pEnt->GetDbEntType()!=IDbEntity::DbLine)
					continue;
				if(pEnt->GetHiberId().IsEqual(inLineId))
					pInLine=(IDbLine*)pEnt;
				else if(pEnt->GetHiberId().IsEqual(outLineId))
					pOutLine=(IDbLine*)pEnt;
				if(pOutLine&&pInLine)
					break;
			}
		}
	}
	else if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
	{
		IDbLine *pLine=(IDbLine*)pEnt;
		GEPOINT line_vec=pLine->EndPosition()-pLine->StartPosition();
		normalize(line_vec);
		GEPOINT endPt=pLine->StartPosition();
		if(pCutPt=pPlate->FromCutPtHiberId(pEnt->GetHiberId(),CProcessPlate::CUT_IN_LINE))
			pInLine=pLine;
		else if(pCutPt=pPlate->FromCutPtHiberId(pEnt->GetHiberId(),CProcessPlate::CUT_OUT_LINE))
			pOutLine=pLine;
	}
	if(pOutLine)
	{
		GEPOINT line_vec=pOutLine->EndPosition()-pOutLine->StartPosition();
		normalize(line_vec);
		pOutLine->SetEndPosition(pOutLine->StartPosition()+line_vec*pCutPt->cOutLineLen);
	}
	if(pInLine)
	{
		GEPOINT line_vec=pInLine->EndPosition()-pInLine->StartPosition();
		normalize(line_vec);
		pInLine->SetEndPosition(pInLine->StartPosition()+line_vec*pCutPt->cInLineLen);
	}
}
void CProcessPlateDraw::ReDraw(I2dDrawing *p2dDraw,ISolidSnap* pSolidSnap,ISolidDraw* pSolidDraw)
{
	IDrawing* pDrawing=p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	int n=pSolidSnap->GetLastSelectEnts(id_arr);
	WORD wDrawMode=m_pBelongEditor->GetDrawMode();
	CProcessPlate* pPlate=(CProcessPlate*)m_pPart;
	for(int i=0;i<n;i++)
	{
		IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(id_arr[i]);
		if(pEnt==NULL)
			continue;
		if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
		{
			BOLT_INFO *pBolt=m_pPart->FromHoleHiberId(pEnt->GetHiberId());
			if(pBolt)
			{
				IDbCircle* pCircle=(IDbCircle*)pEnt;
				f3dPoint ls_pos=f3dPoint(pBolt->posX,pBolt->posY,0);
				if(wDrawMode==IPEC::DRAW_MODE_NC)	//NC模式
					coord_trans(ls_pos,mcs,FALSE);
				pCircle->SetCenter(ls_pos);
				pCircle->SetRadius(pBolt->bolt_d*0.5);
				pCircle->SetNormal(f3dPoint(0,0,1));
			}
			else
				RedrawCutPtEnt(pPlate,pDrawing,pEnt);
		}
		else if(pEnt->GetDbEntType()==IDbEntity::DbPoint) 
		{
			PROFILE_VER* pVertex=pPlate->FromVertexHiberId(pEnt->GetHiberId());
			if(pVertex==NULL)
				return;
			PROFILE_VER* pPreVertex=pPlate->vertex_list.GetTail(),*pNextVertex=NULL,*pCurVertex=NULL;
			for(pCurVertex=pPlate->vertex_list.GetFirst();pCurVertex;pCurVertex=pPlate->vertex_list.GetNext(),i++)
			{
				if(pCurVertex->vertex.IsEqual(pVertex->vertex,0.1))
				{
					pNextVertex=pPlate->vertex_list.GetNext();
					if(pNextVertex==NULL)
						pNextVertex=pPlate->vertex_list.GetFirst();
					break;
				}
				pPreVertex=pCurVertex;
			}
			if(pPreVertex==NULL || pCurVertex==NULL || pNextVertex==NULL)
				return;
			//更新点坐标
			IDbPoint* pPoint=(IDbPoint*)pEnt;
			f3dPoint vertex=pCurVertex->vertex;
			if(wDrawMode==IPEC::DRAW_MODE_NC)
				coord_trans(vertex,mcs,FALSE);
			pPoint->SetPosition(vertex);
			//更新关联边
			ReDrawEdge(pDrawing,pSolidDraw,pPreVertex,pCurVertex);
			ReDrawEdge(pDrawing,pSolidDraw,pCurVertex,pNextVertex);
		}
		else if(pEnt->GetDbEntType()==IDbEntity::DbLine || pEnt->GetDbEntType()==IDbEntity::DbArcline)
		{
			CProcessPlate* pPlate=(CProcessPlate*)m_pPart;
			PROFILE_VER* pCurVer=NULL,*pNextVer=NULL;
			for(pCurVer=pPlate->vertex_list.GetFirst();pCurVer;pCurVer=pPlate->vertex_list.GetNext(),i++)
			{
				if(pCurVer->hiberId.IsEqual(pEnt->GetHiberId()))
				{
					pNextVer=pPlate->vertex_list.GetNext();
					if(pNextVer==NULL)
						pNextVer=pPlate->vertex_list.GetFirst();
					break;
				}
			}
			if(pCurVer&&pNextVer)
				ReDrawEdge(pDrawing,pSolidDraw,pCurVer,pNextVer);
			else
				RedrawCutPtEnt(pPlate,pDrawing,pEnt);
		}
	}
}
GECS CProcessPlateDraw::GetMCS()
{ 
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return GECS();
	((CProcessPlate*)m_pPart)->GetMCS(mcs);
	return mcs; 
}