#include "StdAfx.h"
#include "ProcessPartDraw.h"
#include "I_DrawSolid.h"
#include "DxfFile.h"
#include "PartLib.h"
#include "JgDrawing.h"
#include "PEC.h"
#include "Lic.h"
#include "IXeroCad.h"

//////////////////////////////////////////////////////////////////////////
// CProcessPartDraw
double CProcessPartDraw::m_fMKHoleD=8;
double CProcessPartDraw::m_fHoleIncrement=1.5;
int CProcessPlateDraw::nShieldHeight=160;

static IDbPoint *AppendDbPoint(IDrawing *pDrawing,GEPOINT pos,
							   HIBERID hiberId=0,short style=PS_SOLID,
							   COLORREF crColor=RGB(0,0,0),BYTE width=5)
{
	IDbPoint *pPoint=(IDbPoint*)pDrawing->AppendDbEntity(IDbEntity::DbPoint);
	pPoint->SetPosition(pos);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pPoint->SetPen(pen);
	pPoint->SetHiberId(hiberId);
	return pPoint;
}

static IDbLine *AppendDbLine(IDrawing *pDrawing,GEPOINT start,GEPOINT end,
							 HIBERID hiberId=0,short style=PS_SOLID,
							 COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbLine *pLine=(IDbLine*)pDrawing->AppendDbEntity(IDbEntity::DbLine);
	pLine->SetStartPosition(start);
	pLine->SetEndPosition(end);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pLine->SetPen(pen);
	pLine->SetHiberId(hiberId);
	return pLine;
}
static IDbArcline *AppendDbArcLine(IDrawing *pDrawing,
								   HIBERID hiberId=0,short style=PS_SOLID,
								   COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbArcline *pArcLine=(IDbArcline*)pDrawing->AppendDbEntity(IDbEntity::DbArcline);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pArcLine->SetPen(pen);
	pArcLine->SetHiberId(hiberId);
	return pArcLine;
}
static IDbCircle *AppendDbCircle(IDrawing *pDrawing,f3dPoint center,f3dPoint norm,
								 double radius,HIBERID hiberId=0,short style=PS_SOLID,
								 COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbCircle *pCir=(IDbCircle*)pDrawing->AppendDbEntity(IDbEntity::DbCircle);
	pCir->SetCenter(center);
	pCir->SetNormal(norm);
	pCir->SetRadius(radius);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pCir->SetPen(pen);
	pCir->SetHiberId(hiberId);
	return pCir;
}
static IDbText *AppendDbText(IDrawing *pDrawing,f3dPoint pos,const char* text,
							 double rotation,double height,int aligment=IDbText::AlignMiddleLeft,
							 HIBERID hiberId=0,short style=PS_SOLID,
							 COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbText *pText=(IDbText*)pDrawing->AppendDbEntity(IDbEntity::DbText);
	pText->SetHeight(height);
	pText->SetAlignment(aligment);
	pText->SetPosition(pos);
	pText->SetRotation(rotation);
	pText->SetTextString(text);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pText->SetPen(pen);
	pText->SetHiberId(hiberId);
	return pText;
}
static IDbRect *AppendDbRect(IDrawing *pDrawing,f3dPoint topLeft,f3dPoint btmRight,
							 HIBERID hiberId=0,short style=PS_SOLID,
							 COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbRect *pRect=(IDbRect*)pDrawing->AppendDbEntity(IDbEntity::DbRect);
	pRect->SetTopLeft(topLeft);
	pRect->SetBottomRight(btmRight);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pRect->SetPen(pen);
	pRect->SetHiberId(hiberId);
	return pRect;
}
IDbRotatedDimension* AppendDbRotatedDim(IDrawing *pDrawing,f3dPoint start,f3dPoint end,
										f3dPoint dimPos,const char* dimText,double rotation,
										HIBERID hiberId=0,short style=PS_SOLID,
										COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbRotatedDimension *pDim=(IDbRotatedDimension*)pDrawing->AppendDbEntity(IDbEntity::DbRotatedDimension);
	pDim->SetLine1Point(start);
	pDim->SetLine2Point(end);
	pDim->SetDimLinePoint(dimPos);
	pDim->SetOblique(rotation);
	pDim->SetDimText(dimText);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pDim->SetPen(pen);
	pDim->SetHiberId(hiberId);
	return pDim;
}
IDbArrow* AppendDbArrow(IDrawing *pDrawing,f3dPoint origin,
						f3dPoint direct,double len,double hederLen,
						HIBERID hiberId=0,short style=PS_SOLID,
						COLORREF crColor=RGB(0,0,0),BYTE width=1)
{
	IDbArrow *pArrow=(IDbArrow*)(IDbArrow*)pDrawing->AppendDbEntity(IDbEntity::DbArrow);
	pArrow->SetArrowOrigin(origin);
	pArrow->SetArrowDirect(direct);
	pArrow->SetArrowLength(len);
	pArrow->SetArrowHeaderLength(hederLen);
	PEN_STRU pen;
	pen.crColor=crColor;
	pen.style=style;
	pen.width=width;
	pArrow->SetPen(pen);
	pArrow->SetHiberId(hiberId);
	return pArrow;
}

CProcessPartDraw::CProcessPartDraw(void)
{
	m_pPart=NULL;
	m_pBelongEditor=NULL;
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
	if(m_pBelongEditor==NULL)
		return;
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
	pDrawing->EmptyDbEntities();
	p2dDraw->SetActiveDrawing(pDrawing->GetId());
	if(wDrawMode==IPEC::DRAW_MODE_NC)
		NcModelDraw(pDrawing,pSolidSet);
	else if(wDrawMode==IPEC::DRAW_MODE_EDIT)
		EditorModelDraw(pDrawing,pSolidSet);
	else if(wDrawMode==IPEC::DRAW_MODE_PROCESSCARD)
		ProcessCardModelDraw(pDrawing,pSolidSet);
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
	double fTextHeight=30;
	AppendDbText(pDrawing,dimPos,"Y肢",0,fTextHeight,IDbText::AlignMiddleLeft,0,PS_SOLID,RGB(0,255,0));
	dimPos.y*=-1;
	AppendDbText(pDrawing,dimPos,"X肢",0,fTextHeight,IDbText::AlignMiddleLeft,0,PS_SOLID,RGB(255,0,0));
	EditorModelDraw(pDrawing,pSolidSet);
}
void CProcessAngleDraw::EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	CProcessAngle *pAngle=(CProcessAngle*)m_pPart;
	double len=pAngle->m_wLength;
	AppendDbPoint(pDrawing,GEPOINT(0,0,0));
	AppendDbPoint(pDrawing,GEPOINT(0,pAngle->m_fThick,0));
	AppendDbPoint(pDrawing,GEPOINT(0,pAngle->m_fWidth,0));
	AppendDbPoint(pDrawing,GEPOINT(0,-pAngle->m_fThick,0));
	AppendDbPoint(pDrawing,GEPOINT(0,-pAngle->m_fWidth,0));

	AppendDbPoint(pDrawing,GEPOINT(len,0,0));
	AppendDbPoint(pDrawing,GEPOINT(len,pAngle->m_fThick,0));
	AppendDbPoint(pDrawing,GEPOINT(len,pAngle->m_fWidth,0));
	AppendDbPoint(pDrawing,GEPOINT(len,-pAngle->m_fThick,0));
	AppendDbPoint(pDrawing,GEPOINT(len,-pAngle->m_fWidth,0));
	//轮廓线
	AppendDbLine(pDrawing,GEPOINT(0,-pAngle->m_fWidth),GEPOINT(0,pAngle->m_fWidth));
	AppendDbLine(pDrawing,GEPOINT(0,-pAngle->m_fWidth),GEPOINT(len,-pAngle->m_fWidth));
	AppendDbLine(pDrawing,GEPOINT(0,pAngle->m_fWidth),GEPOINT(len,pAngle->m_fWidth));
	AppendDbLine(pDrawing,GEPOINT(len,-pAngle->m_fWidth),GEPOINT(len,pAngle->m_fWidth));
	AppendDbLine(pDrawing,GEPOINT(0,0),GEPOINT(len,0));
	AppendDbLine(pDrawing,GEPOINT(0,-pAngle->m_fThick),GEPOINT(len,-pAngle->m_fThick),0,PS_DASH);
	AppendDbLine(pDrawing,GEPOINT(0,pAngle->m_fThick),GEPOINT(len,pAngle->m_fThick),0,PS_DASH);
	//螺栓孔
	BOLT_INFO *pBolt=NULL;
	for(pBolt=pAngle->m_xBoltInfoList.GetFirst();pBolt;pBolt=pAngle->m_xBoltInfoList.GetNext())
		AppendDbCircle(pDrawing,f3dPoint(pBolt->posX,pBolt->posY),f3dPoint(0,0,1),pBolt->bolt_d*0.5,pBolt->hiberId);
}
void CProcessAngleDraw::ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	CProcessAngle *pAngle=(CProcessAngle*)m_pPart;
	CLineAngleDrawing xAngleDraw;
	double fMaxDrawingLen=240,fMaxDrawingWidth=130;
	xAngleDraw.CreateLineAngleDrawing(pAngle,fMaxDrawingLen,fMaxDrawingWidth);
	//
	AppendDbRect(pDrawing,f3dPoint(0,0.5*fMaxDrawingWidth),f3dPoint(fMaxDrawingLen,-0.5*fMaxDrawingWidth));
	int i=0,n=0;
	n = xAngleDraw.GetLineCount();
	f3dLine line;
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetLineAt(line,i);
		AppendDbLine(pDrawing,line.startPt,line.endPt);
	}
	for(i=0;i<5;i++)
	{
		xAngleDraw.GetZEdge(line,i);
		if(line.feature==0)
			AppendDbLine(pDrawing,line.startPt,line.endPt);
		else
			AppendDbLine(pDrawing,line.startPt,line.endPt,0,PS_DASH);
	}
	f3dCircle cir;
	n = xAngleDraw.GetXLsCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetXLsCircle(cir,i);
		AppendDbCircle(pDrawing,cir.centre,f3dPoint(0,0,1),0.5);
	}
	n = xAngleDraw.GetYLsCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetYLsCircle(cir,i);
		AppendDbCircle(pDrawing,cir.centre,f3dPoint(0,0,1),0.5);
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
		AppendDbRotatedDim(pDrawing,jg_dim.dimStartPos,jg_dim.dimEndPos,jg_dim.dimPos,jg_dim.DimText(),jg_dim.angle);
	}
	CZhunXianTextDim zhun_dim;
	n = xAngleDraw.GetZhunDimCount();
	for(i=0;i<n;i++)
	{
		xAngleDraw.GetZhunDimAt(i,&zhun_dim);
		zhun_dim.dimStartPos.x=zhun_dim.dimEndPos.x=zhun_dim.dimPos.x;
		if(zhun_dim.dimStartPos!=zhun_dim.dimEndPos)
			AppendDbRotatedDim(pDrawing,zhun_dim.dimStartPos,zhun_dim.dimEndPos,zhun_dim.dimPos,zhun_dim.DimText(),zhun_dim.angle);
		AppendDbLine(pDrawing,zhun_dim.lineStart,zhun_dim.lineEnd,0,PS_DASH,RGB(255,0,0));
	}
	CTextOnlyDrawing pure_txt;
	n = xAngleDraw.GetPureTxtCount();
	//绝对尺寸或者竖直标注的相对尺寸或其他文字标注
	for(i=0;i<n;i++)
	{
		if(!xAngleDraw.GetPureTxtDimAt(i,&pure_txt))
			continue;
		pure_txt.dimPos+=pure_txt.origin;
		AppendDbText(pDrawing,pure_txt.dimPos,pure_txt.dimText,0,2.5);
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
		kaihe_dim.dimPos.x=(kaihe_dim.dimStartPos.x+kaihe_dim.dimEndPos.x)/2.0;
		AppendDbText(pDrawing,kaihe_dim.dimPos,kaihe_dim.dimText,0,2.5);
	}
}
void CProcessAngleDraw::ReDraw(I2dDrawing* p2dDraw,ISolidSnap* pSolidSnap)
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
		BOLT_INFO *pBolt=m_pPart->FromHoleId(pEnt->GetHiberId());
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
static SCOPE_STRU GetPlateVertexScope(CProcessPlate *pPlate,GECS *pCS=NULL)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		f3dPoint pt=pVertex->vertex;
		if(pCS)
			coord_trans(pt,*pCS,FALSE);
		scope.VerifyVertex(pt);
	}
	return scope;
}
void TransPlateToMCS(CProcessPlate *pPlate,GECS &mcs)
{
	if(pPlate==NULL)
		return;
	//1.火曲信息（钢板固接坐标系下）
	for(int i=0;i<pPlate->m_cFaceN-1;i++)
	{
		vector_trans(pPlate->HuoQuFaceNorm[i],mcs,FALSE);
		coord_trans(pPlate->HuoQuLine[i].startPt,mcs,FALSE);
		coord_trans(pPlate->HuoQuLine[i].endPt,mcs,FALSE);
	}
	//2.螺栓信息
	for(BOLT_INFO *pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
	{
		f3dPoint ls_pos(pBolt->posX,pBolt->posY,0);
		coord_trans(ls_pos,mcs,FALSE);
		pBolt->posX=(float)ls_pos.x;
		pBolt->posY=(float)ls_pos.y;
	}
	//3.板外形
	if(pPlate->m_cFaceN==3)
		coord_trans(pPlate->top_point,mcs,FALSE);
	PROFILE_VER *pVertex=NULL;
	for(pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{	
		coord_trans(pVertex->vertex,mcs,FALSE);
		if(pVertex->type>0)
		{
			vector_trans(pVertex->work_norm,mcs,FALSE);
			if(pVertex->type==3)
			{
				coord_trans(pVertex->center,mcs,FALSE);
				vector_trans(pVertex->column_norm,mcs,FALSE);	
			}
		}
	}
}
CProcessPlateDraw::CProcessPlateDraw(void)
{
}

CProcessPlateDraw::~CProcessPlateDraw(void)
{
}
static void DrawPlate(CProcessPlate *pPlate,IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	if(pPlate==NULL||pSolidSet==NULL||pDrawing==NULL)
		return;
	f3dPoint tube_len_vec;	//焊件父杆件延伸方向
	if(pPlate->m_cFaceN==3)
		AppendDbPoint(pDrawing,pPlate->top_point,0);
	CHashListEx<PROFILE_VER> vertex_list;
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext()）
		vertex_list.Append(*pVertex,)
	int i=0;
	f3dLine line;
	PROFILE_VER *pPrevPnt=pPlate->vertex_list.GetTail();
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext(),i++)
	{
		AppendDbPoint(pDrawing,pVertex->vertex,pVertex->hiberId);
		//轮廓线
		line.pen.style=PS_SOLID;
		if(pPlate->m_cFaceN<3||pPrevPnt->vertex.feature!=3||pVertex->vertex.feature!=2)
		{
			f3dArcLine arcLine;
			arcLine.pen=line.pen;
			int iFace=pPrevPnt->vertex.feature;
			if(pPrevPnt->type==2)
			{	//圆弧
				IDbArcline *pArcLine=AppendDbArcLine(pDrawing,pPrevPnt->hiberId);
				pArcLine->CreateMethod2(pPrevPnt->vertex,pVertex->vertex,pPrevPnt->work_norm,fabs(pPrevPnt->sector_angle));	
			}
			else if(pPrevPnt->type==3)	//椭圆弧
			{
				IDbArcline *pArcLine=AppendDbArcLine(pDrawing,pPrevPnt->hiberId);
				pArcLine->CreateEllipse(pPrevPnt->center,pPrevPnt->vertex,pVertex->vertex,pPrevPnt->column_norm,
					pPrevPnt->work_norm*-1,fabs(pPrevPnt->radius));
			}
			else	//直线
				AppendDbLine(pDrawing,pPrevPnt->vertex,pVertex->vertex,pPrevPnt->hiberId);
		}
		else
		{
			AppendDbLine(pDrawing,pPrevPnt->vertex,pPlate->top_point,pPrevPnt->hiberId);
			AppendDbLine(pDrawing,pPlate->top_point,pVertex->vertex,pVertex->hiberId);
		}
		pPrevPnt = pVertex;
	}
	line.pen.style=PS_DASHDOTDOT;
	line.pen.crColor=RGB(255,0,0);
	line.pen.width=1;
	line.feature=2;	//火曲线火曲
	for(int i=2;i<=pPlate->m_cFaceN;i++)
	{
		BOOL bFindStart=FALSE,bFindEnd=FALSE;
		line.startPt=pPlate->HuoQuLine[i-2].startPt;
		line.endPt  =pPlate->HuoQuLine[i-2].endPt;
		for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		{
			if(pVertex->vertex.feature!=10+i)
				continue;	//非火曲点 wht 11-02-22
			if(DistOf3dPtLine(pVertex->vertex,pPlate->HuoQuLine[i-2].startPt,pPlate->HuoQuLine[i-2].endPt)<EPS)
			{
				if(!bFindStart)
				{
					line.startPt=pVertex->vertex;
					bFindStart=TRUE;
				}
				else
				{
					line.endPt = pVertex->vertex;
					bFindEnd=TRUE;
					break;
				}
			}
		}
		if(pPlate->m_cFaceN==3&&DistOf3dPtLine(pPlate->top_point,pPlate->HuoQuLine[i-2].startPt,pPlate->HuoQuLine[i-2].endPt)<EPS)
		{
			if(!bFindStart)
				line.startPt=pPlate->top_point;
			else if(!bFindEnd)
				line.endPt=pPlate->top_point;
		}
		line.ID=i-1;	//第一条火曲线
		AppendDbLine(pDrawing,line.startPt,line.endPt,line.ID,PS_DASHDOTDOT,RGB(255,0,0));
	}
	
	line.pen.style=PS_SOLID;
	line.pen.crColor=0;
	line.pen.width=1;
	line.feature=0;//普通直线
	//螺栓孔
	f3dPoint ls_pos;
	for(BOLT_INFO *pBoltInfo=pPlate->m_xBoltInfoList.GetFirst();pBoltInfo;pBoltInfo=pPlate->m_xBoltInfoList.GetNext())
	{
		f3dCircle circle;
		circle.norm.Set(0,0,1);
		circle.centre.Set(pBoltInfo->posX,pBoltInfo->posY,0);
		circle.radius = (pBoltInfo->bolt_d+pBoltInfo->hole_d_increment)/2;
		circle.ID	  = pBoltInfo->hiberId.HiberUpId(1);
		AppendDbCircle(pDrawing,circle.centre,circle.norm,circle.radius,pBoltInfo->hiberId);
	}
}
static void DrawNcBackGraphic(CProcessPlate *pPlate,IDrawing *pDrawing,ISolidSet *pSolidSet,int shieldHeight)
{
	//1.绘制坐标系
	SCOPE_STRU scope=GetPlateVertexScope(pPlate);
	double axis_len=max(scope.wide(),scope.high())+20;
	f3dPoint original;
	AppendDbArrow(pDrawing,original,f3dPoint(1,0,0),axis_len,10,0,PS_SOLID,RGB(255,0,0),3);
	if(axis_len<shieldHeight)
		axis_len=shieldHeight+20;
	AppendDbArrow(pDrawing,original,f3dPoint(0,1,0),axis_len,10,0,PS_SOLID,RGB(0,255,0),3);
	//2.绘制挡板
	double shieldThick=10;
	AppendDbRect(pDrawing,f3dPoint(-shieldThick,shieldHeight,0),f3dPoint(),0,PS_SOLID,RGB(0,0,0),3);
	//3.绘制构件明细
	if(pPlate->vertex_list.GetNodeNum()>=3)
	{
		char matStr[20]="";
		QuerySteelMatMark(pPlate->cMaterial,matStr);
		CXhChar200 sPartInfo("编号:%s  板厚:%dmm 材质%s",(char*)pPlate->GetPartNo(),(int)pPlate->m_fThick,matStr);
		double fTextHeight=30;
		f3dPoint dim_pos(0,-1*fTextHeight,0);
		AppendDbText(pDrawing,dim_pos,sPartInfo,0,fTextHeight);
	}
}
void CProcessPlateDraw::NcModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	//0.初始化加工坐标系
	InitNcDrawMode();
	//1.将构件转换到加工坐标系下
	CProcessPlate tempPlate;
	m_pPart->ClonePart(&tempPlate);
	TransPlateToMCS(&tempPlate,mcs);
	//2.绘制坐标系及挡板高度
	DrawNcBackGraphic(&tempPlate,pDrawing,pSolidSet,nShieldHeight);
	//3.绘制钢板外形
	DrawPlate(&tempPlate,pDrawing,pSolidSet);
	//4.绘制打号位置
	AppendDbCircle(pDrawing,m_pPart->mkpos,f3dPoint(0,0,1),m_fMKHoleD*0.5,0,PS_SOLID,RGB(255,0,0),2);
}
void CProcessPlateDraw::ProcessCardModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	EditorModelDraw(pDrawing,pSolidSet);
}
void CProcessPlateDraw::EditorModelDraw(IDrawing *pDrawing,ISolidSet *pSolidSet)
{
	DrawPlate((CProcessPlate*)m_pPart,pDrawing,pSolidSet);
}
//得到板相对坐标系
static void  GetMachineUcs(CHashListEx<PROFILE_VER> &vertex_list,GECS &machine_ucs,WORD &bottom_edge)
{
	PROFILE_VER *pVertex=NULL;
	int i=0,n=vertex_list.GetNodeNum();
	if(n<3)
		return ;
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	for(pVertex=vertex_list.GetFirst();pVertex;pVertex=vertex_list.GetNext(),i++)
		vertex_arr[i]=pVertex->vertex;
	machine_ucs.axis_z.Set(0,0,1);
	if(bottom_edge==-1||bottom_edge>=vertex_list.GetNodeNum())
	{	//初始化加工坐标系
		GEPOINT vertice,prev_vec,edge_vec;
		double prev_edge_dist=0, edge_dist = 0, max_edge = 0;
		bottom_edge=0;
		for(i=0;i<n;i++)
		{
			vertice = vertex_arr[i];
			edge_vec=vertex_arr[(i+1)%n]-vertice;
			edge_dist = edge_vec.mod();
			edge_vec/=edge_dist;	//单位化边矢量
			if(i>0&&prev_vec*edge_vec>EPS_COS)	//连续共线边轮廓
				edge_dist+=edge_dist+prev_edge_dist;
			if(edge_dist>max_edge)
			{
				max_edge = edge_dist;
				machine_ucs.axis_x = edge_vec;
				machine_ucs.origin = vertice;
				bottom_edge=i;
			}
			prev_edge_dist=edge_dist;
			prev_vec=edge_vec;
		}
	}
	else 
	{	//根据bottom_edge_i计算加工坐标系
		machine_ucs.axis_x = vertex_arr[(bottom_edge+1)%n]-vertex_arr[bottom_edge];
		machine_ucs.origin = vertex_arr[bottom_edge];
		machine_ucs.axis_x.z=0;
		normalize(machine_ucs.axis_x);
	}
	machine_ucs.axis_y.Set(-machine_ucs.axis_x.y,machine_ucs.axis_x.x);
	double min_coord_x = 0;
	//确定板的最左边位置
	for(i=0;i<n;i++)
	{
		coord_trans(vertex_arr[i],machine_ucs,FALSE);
		if(i==0)
			min_coord_x=vertex_arr[i].x;
		else if(vertex_arr[i].x<min_coord_x)
			min_coord_x = vertex_arr[i].x;
	}
	machine_ucs.origin += machine_ucs.axis_x*min_coord_x;
}
//NC模式需要的函数
void CProcessPlateDraw::InitNcDrawMode()
{	//初始化加工坐标系
	if(m_pBelongEditor==NULL||m_pBelongEditor->GetDrawMode()!=IPEC::DRAW_MODE_NC)
		return;
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	int iOldBottomEdge=pPlate->m_wBottomEdge;
	//1.初始化加工坐标系并调整构件信息至加工坐标系
	if(((CPEC*)m_pBelongEditor)->GetCurCmdType()!=IPEC::CMD_OVERTURN_PLATE)
		GetMachineUcs(pPlate->vertex_list,mcs,pPlate->m_wBottomEdge);
	//2.初始化打号位置
	if(pPlate->m_wBottomEdge!=iOldBottomEdge)	
		pPlate->mkpos.Set(20,20);
}

void CProcessPlateDraw::CreateDxfFile(char* filename)
{
	CProcessPlate tempPlate;
	m_pPart->ClonePart(&tempPlate);
	InitNcDrawMode();
	TransPlateToMCS(&tempPlate,mcs);
	//
	CDxfFile file;
	SCOPE_STRU scope;
	if(file.OpenFile(filename))
	{
		if(tempPlate.vertex_list.GetNodeNum()>0)
		{
			f3dPoint first_pt,prev_pt,next_pt;
			first_pt=tempPlate.vertex_list.GetFirst()->vertex;
			prev_pt=first_pt;
			for(PROFILE_VER *pVertex=tempPlate.vertex_list.GetNext();pVertex;pVertex=tempPlate.vertex_list.GetNext())
			{
				next_pt=pVertex->vertex;
				file.NewLine(prev_pt,next_pt);
				prev_pt=next_pt;
				//
				scope.VerifyVertex(f3dPoint(pVertex->vertex.x,pVertex->vertex.y));
			}
			file.NewLine(next_pt,first_pt);
		}

		f3dPoint centre;
		for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
		{
			centre.Set(pHole->posX,pHole->posY);
			file.NewCircle(centre,(pHole->bolt_d+pHole->hole_d_increment)/2.0);
			//
			double radius=0.5*pHole->bolt_d;
			scope.VerifyVertex(f3dPoint(pHole->posX-radius,pHole->posY-radius));
			scope.VerifyVertex(f3dPoint(pHole->posX+radius,pHole->posY+radius));
		}
		centre.Set(tempPlate.mkpos.x,tempPlate.mkpos.y);
		file.NewCircle(f3dPoint(centre.x,centre.y),m_fMKHoleD/2.0);
		//
		scope.VerifyVertex(f3dPoint(tempPlate.mkpos.x,tempPlate.mkpos.y));
		//
		file.extmin.Set(scope.fMinX,scope.fMaxY);
		file.extmax.Set(scope.fMaxX,scope.fMinY);
		file.CloseFile();
	}
}
void CProcessPlateDraw::CreateTtpFile(char* filename)
{
	CProcessPlate tempPlate;
	m_pPart->ClonePart(&tempPlate);
	InitNcDrawMode();
	TransPlateToMCS(&tempPlate,mcs);
	//
	char drive[8],dir[MAX_PATH],fname[MAX_PATH],ext[10];
	_splitpath(filename,drive,dir,fname,ext);
	FILE *fp = fopen(filename,"wb");
	if(fp!=NULL)
	{
		memset(filename,0,20);
		sprintf(filename,"%s.ttp",fname);
		fwrite(filename,1,13,fp);
		BYTE flag=0x80;	//表示联结板图
		fwrite(&flag,1,1,fp);
		short x,y;
		short n=(short)(tempPlate.vertex_list.GetNodeNum()+tempPlate.m_xBoltInfoList.GetNodeNum())+1;
		fwrite(&n,2,1,fp);
		fwrite(tempPlate.GetPartNo(),1,12,fp);
		for(PROFILE_VER *pVertex=tempPlate.vertex_list.GetFirst();pVertex;pVertex=tempPlate.vertex_list.GetNext())
		{
			f3dPoint pt=pVertex->vertex;
			x = ftoi(pt.x*10);
			y = ftoi(pt.y*10);
			flag = 0x0;	//边框轮廓点
			fwrite(&x,2,1,fp);
			fwrite(&y,2,1,fp);
			fwrite(&flag,1,1,fp);
		}

		f3dPoint centre;
		for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
		{
			centre.Set(pHole->posX,pHole->posY);
			x = ftoi(centre.x*10);
			y = ftoi(centre.y*10);
			flag = 0x0;
			if(pHole->bolt_d==16)
				flag = 0x01;
			else if(pHole->bolt_d==20)
				flag = 0x02;
			else if(pHole->bolt_d==24)
				flag = 0x03;
			else if(pHole->bolt_d==12)
				flag = 0x04;
			else if(pHole->bolt_d==18||(pHole->bolt_d+pHole->hole_d_increment)==19.5)
				flag = 0x05;
			else if(pHole->bolt_d==22||(pHole->bolt_d+pHole->hole_d_increment)==23.5)
				flag = 0x06;
			fwrite(&x,2,1,fp);
			fwrite(&y,2,1,fp);
			fwrite(&flag,1,1,fp);
		}
		centre.Set(tempPlate.mkpos.x,tempPlate.mkpos.y);
		x = ftoi(centre.x*10);
		y = ftoi(centre.y*10);
		flag = 0x07;	//钻孔
		fwrite(&x,2,1,fp);
		fwrite(&y,2,1,fp);
		fwrite(&flag,1,1,fp);
		fclose(fp);
	}
}

void CProcessPlateDraw::RotateAntiClockwise() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	int i=0,n=pPlate->vertex_list.GetNodeNum();
	if(n<=0)
		return;
	f3dPoint prev_pt,first_pt,next_pt;
	first_pt=prev_pt=pPlate->vertex_list.GetFirst()->vertex;
	pPlate->m_wBottomEdge=(pPlate->m_wBottomEdge-1+n)%n;
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetNext();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		next_pt=pVertex->vertex;
		if(i==pPlate->m_wBottomEdge)
		{
			mcs.origin=prev_pt;
			mcs.axis_x=next_pt-prev_pt;
			normalize(mcs.axis_x);
			mcs.axis_y.Set(-mcs.axis_x.y,mcs.axis_x.x);
			break;
		}
		prev_pt=next_pt;
		i++;
	}
	if(pPlate->m_wBottomEdge==n-1)
	{
		mcs.origin=prev_pt;
		mcs.axis_x=first_pt-prev_pt;
		normalize(mcs.axis_x);
		mcs.axis_y.Set(-mcs.axis_x.y,mcs.axis_x.x);
	}
	SCOPE_STRU scope=GetPlateVertexScope(pPlate,&mcs);
	mcs.origin+=(scope.fMinX*mcs.axis_x+scope.fMinY*mcs.axis_y);
}

void CProcessPlateDraw::RotateClockwise() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	int i=0,n=pPlate->vertex_list.GetNodeNum();
	if(n<=0)
		return;
	f3dPoint prev_pt,first_pt,next_pt;
	first_pt=prev_pt=pPlate->vertex_list.GetFirst()->vertex;
	pPlate->m_wBottomEdge=(pPlate->m_wBottomEdge+1)%n;
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetNext();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		next_pt=pVertex->vertex;
		if(i==pPlate->m_wBottomEdge)
		{
			mcs.origin=prev_pt;
			mcs.axis_x=next_pt-prev_pt;
			normalize(mcs.axis_x);
			mcs.axis_y.Set(-mcs.axis_x.y,mcs.axis_x.x);
			break;
		}
		prev_pt=next_pt;
		i++;
	}
	if(pPlate->m_wBottomEdge==n-1)
	{
		mcs.origin=prev_pt;
		mcs.axis_x=first_pt-prev_pt;
		normalize(mcs.axis_x);
		mcs.axis_y.Set(-mcs.axis_x.y,mcs.axis_x.x);
	}
	SCOPE_STRU scope=GetPlateVertexScope(pPlate,&mcs);
	mcs.origin+=(scope.fMinX*mcs.axis_x+scope.fMinY*mcs.axis_y);
}

void CProcessPlateDraw::OverturnPlate() 
{
	if(m_pPart==NULL||m_pPart->m_cPartType!=CProcessPart::TYPE_PLATE)
		return;
	CProcessPlate *pPlate=(CProcessPlate*)m_pPart;
	SCOPE_STRU scope=GetPlateVertexScope(pPlate,&mcs);
	mcs.origin+=((scope.fMaxX-scope.fMinX)*mcs.axis_x);
	mcs.axis_x*=-1.0;
	mcs.axis_z*=-1.0;
}
static IDbEntity* GetLineByHiberId(I2dDrawing* p2dDraw,HIBERID hiberId)
{
	IDrawing *pDrawing=p2dDraw->GetActiveDrawing();
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
void CProcessPlateDraw::ReDrawEdge(I2dDrawing* p2dDraw,PROFILE_VER* pStartVer,PROFILE_VER* pEndVer)
{
	if(pStartVer==NULL || pEndVer==NULL)
		return;
	WORD wDrawMode=m_pBelongEditor->GetDrawMode();
	f3dPoint startPt,endPt,workNorm,columnNorm,centerPt;
	if(pStartVer->type==1)
	{
		IDbLine* pLine=(IDbLine*)GetLineByHiberId(p2dDraw,pStartVer->hiberId);
		if(pLine==NULL)
			return;
		startPt=pStartVer->vertex;
		endPt=pEndVer->vertex;
		if(wDrawMode==IPEC::DRAW_MODE_NC)
		{
			coord_trans(startPt,mcs,FALSE);
			coord_trans(endPt,mcs,FALSE);
		}
		pLine->SetStartPosition(startPt);
		pLine->SetEndPosition(endPt);
	}
	else if(pStartVer->type==2)	//圆弧
	{
		IDbArcline* pArcLine=(IDbArcline*)GetLineByHiberId(p2dDraw,pStartVer->hiberId);
		if(pArcLine==NULL)
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
		pArcLine->SetStartPosition(startPt);
		pArcLine->SetEndPosition(endPt);
		pArcLine->SetWorkNorm(workNorm);
		pArcLine->SetSectorAngle(fabs(pStartVer->sector_angle));
	}
	else if(pStartVer->type==3)	//椭圆弧
	{
		IDbArcline *pArcLine=(IDbArcline*)GetLineByHiberId(p2dDraw,pStartVer->hiberId);
		if(pArcLine==NULL)
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
		pArcLine->SetCenter(centerPt);
		pArcLine->SetStartPosition(startPt);
		pArcLine->SetEndPosition(endPt);
		pArcLine->SetColumnNorm(columnNorm);
		pArcLine->SetWorkNorm(workNorm*-1);
		pArcLine->SetRadius(fabs(pStartVer->radius));
	}
}
void CProcessPlateDraw::ReDraw(I2dDrawing* p2dDraw,ISolidSnap* pSolidSnap)
{
	IDrawing* pDrawing=p2dDraw->GetActiveDrawing();
	if(pDrawing==NULL)
		return;
	long* id_arr=NULL;
	int n=pSolidSnap->GetLastSelectEnts(id_arr);
	WORD wDrawMode=m_pBelongEditor->GetDrawMode();
	for(int i=0;i<n;i++)
	{
		IDbEntity *pEnt=pDrawing->Database()->GetDbEntity(id_arr[i]);
		if(pEnt==NULL)
			continue;
		if(pEnt->GetDbEntType()==IDbEntity::DbCircle)
		{
			BOLT_INFO *pBolt=m_pPart->FromHoleId(pEnt->GetHiberId());
			if(pBolt==NULL)
				continue;
			IDbCircle* pCircle=(IDbCircle*)pEnt;
			f3dPoint ls_pos=f3dPoint(pBolt->posX,pBolt->posY,0);
			if(wDrawMode==IPEC::DRAW_MODE_NC)	//NC模式
				coord_trans(ls_pos,mcs,FALSE);
			pCircle->SetCenter(ls_pos);
			pCircle->SetRadius(pBolt->bolt_d*0.5);
			pCircle->SetNormal(f3dPoint(0,0,1));
		}
		else if(pEnt->GetDbEntType()==IDbEntity::DbPoint) 
		{
			CProcessPlate* pPlate=(CProcessPlate*)m_pPart;
			PROFILE_VER* pPreVertex=pPlate->vertex_list.GetTail(),*pNextVertex=NULL,*pCurVertex=NULL;
			for(pCurVertex=pPlate->vertex_list.GetFirst();pCurVertex;pCurVertex=pPlate->vertex_list.GetNext(),i++)
			{
				if(pCurVertex->hiberId.IsEqual(pEnt->GetHiberId()))
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
			ReDrawEdge(p2dDraw,pPreVertex,pCurVertex);
			ReDrawEdge(p2dDraw,pCurVertex,pNextVertex);
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
			if(pCurVer==NULL || pNextVer==NULL)
				continue;
			ReDrawEdge(p2dDraw,pCurVer,pNextVer);
		}
	}
}