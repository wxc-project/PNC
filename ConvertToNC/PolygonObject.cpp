#include "StdAfx.h"
#include "PolygonObject.h"
#include "XhLdsLm.h"
#include "SortFunc.h"
#include <stack>

using namespace std;
//////////////////////////////////////////////////////////////////////////
//静态函数定义
DWORD MkDW(UINT uiX, UINT uiY) {
	DWORD location = (uiY << 16) | uiX;
	return location;
}
static DWORD _LocalHashFunc(DWORD key, DWORD nHashSize)
{
	return key & 0x0000ffff;
}
static int compare_func(const VERTSTRIP_HEADER& item1, const VERTSTRIP_HEADER& item2)
{
	int iRowY1 = 0, iRowY2 = 0;
	if (item1.cbHeadByte == 0)
	{
		STRIP16_ROW* pRowPixels = (STRIP16_ROW*)&item1;
		iRowY1 = pRowPixels->yRowJ;
	}
	else
	{
		VERTSTRIP16* pStrip = (VERTSTRIP16*)&item1;
		iRowY1 = pStrip->wiyTop;
	}
	if (item2.cbHeadByte == 0)
	{
		STRIP16_ROW* pRowPixels = (STRIP16_ROW*)&item2;
		iRowY2 = pRowPixels->yRowJ;
	}
	else
	{
		VERTSTRIP16* pStrip = (VERTSTRIP16*)&item2;
		iRowY2 = pStrip->wiyTop;
	}
	return compare_int(iRowY1,iRowY2);
}
//////////////////////////////////////////////////////////////////////////
//STRIP16_ROW
void STRIP16_ROW::SetBlckPixel(int xI) 
{
	WORD wFlag = (WORD)GetSingleWord(xI+1);
	wbPixels |= wFlag;
}
BOOL STRIP16_ROW::IsBlckPixel(int xI)
{
	WORD wFlag = (WORD)GetSingleWord(xI + 1);
	return wbPixels & wFlag;
}
void STRIP16_ROW::SetConnState(int xI)
{
	WORD wFlag = (WORD)GetSingleWord(xI + 1);
	wDetectState |= wFlag;
}
BOOL STRIP16_ROW::IsConnState(int xI)
{
	WORD wFlag = (WORD)GetSingleWord(xI + 1);
	return wDetectState & wFlag;
}
//////////////////////////////////////////////////////////////////////////
//STRIP_SCROLL
bool STRIP_SCROLL::CheckProfilePixel(int xI, int yJ)
{
	if (xarrEntries.Count <= 0)
		return false;
	int xiLocal = xI % 16;	//
	for (WORD i = 0; i < xarrEntries.Count; i++)
	{
		if (xarrEntries[i].cbHeadByte == 0)
		{	//格栅像素区
			STRIP16_ROW* pRowPixels = (STRIP16_ROW*)&xarrEntries[i];
			if (pRowPixels->yRowJ == yJ)
			{
				pRowPixels->SetBlckPixel(xiLocal);
				break;
			}
		}
		else
		{	//空白条幅带
			VERTSTRIP16* pStrip = (VERTSTRIP16*)&xarrEntries[i];
			if (yJ<pStrip->wiyTop || yJ > pStrip->wiyBtm)
				continue;	//不在此区域
			if (pStrip->niHeight == 1)
			{	//条幅带只有一行时，直接转换成像素带
				memset(&xarrEntries[i], 0, sizeof(STRIP16_ROW));
				STRIP16_ROW* pRowPixels = (STRIP16_ROW*)&xarrEntries[i];
				pRowPixels->yRowJ = yJ;
				pRowPixels->SetBlckPixel(xiLocal);
			}
			else if (yJ > pStrip->wiyTop && yJ < pStrip->wiyBtm)
			{	//离散像素位于中间部分
				//处理零散像素
				STRIP16_ROW* pRowPixels = (STRIP16_ROW*)xarrEntries.Append(VERTSTRIP_HEADER(0));
				pRowPixels->yRowJ = yJ;
				pRowPixels->SetBlckPixel(xiLocal);
				//处理条幅带
				VERTSTRIP16* pNextStrip = (VERTSTRIP16*)xarrEntries.Append(VERTSTRIP_HEADER(1));
				*pNextStrip = *pStrip;
				pNextStrip->wiyTop = yJ + 1;
				pStrip->wiyBtm = yJ - 1;
			}
			else if (yJ == pStrip->wiyTop)
			{	//离散像素位于顶部
				//处理零散像素
				STRIP16_ROW* pRowPixels = (STRIP16_ROW*)xarrEntries.Append(VERTSTRIP_HEADER(0));
				pRowPixels->yRowJ = yJ;
				pRowPixels->SetBlckPixel(xiLocal);
				//处理条幅带
				pStrip->wiyTop = yJ + 1;
			}
			else if (yJ == pStrip->wiyBtm)
			{	//离散像素位于底部
				//处理零散像素
				STRIP16_ROW* pRowPixels = (STRIP16_ROW*)xarrEntries.Append(VERTSTRIP_HEADER(0));
				pRowPixels->yRowJ = yJ;
				pRowPixels->SetBlckPixel(xiLocal);
				//处理条幅带
				pStrip->wiyBtm = yJ - 1;
			}
			break;
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////
//CVectorMonoImage
CVectorMonoImage::CVectorMonoImage()
{
	m_fScaleOfView2Model = 1;
	m_nWidth = 0;
	m_nHeight = 0;
	DisplayProcess = NULL;
	//像素8邻接点位置
	arr8NearPos[0] = GEPOINT2D(1, 0);
	arr8NearPos[1] = GEPOINT2D(1, 1);
	arr8NearPos[2] = GEPOINT2D(0, 1);
	arr8NearPos[3] = GEPOINT2D(-1, 1);
	arr8NearPos[4] = GEPOINT2D(-1, 0);
	arr8NearPos[5] = GEPOINT2D(-1,-1);
	arr8NearPos[6] = GEPOINT2D(0, -1);
	arr8NearPos[7] = GEPOINT2D(1, -1);
	//像素4邻接点位置
	arr4NearPos[0] = GEPOINT2D(1, 0);
	arr4NearPos[1] = GEPOINT2D(0, 1);
	arr4NearPos[2] = GEPOINT2D(-1, 0);
	arr4NearPos[3] = GEPOINT2D(0, -1);
	//
	hashStrokePixels.SetHashFunc(_LocalHashFunc);
	hashStrokePixels.SetHashTableGrowSize(65535, true);
}
void CVectorMonoImage::SetOCS(GECS& _ocs,double scaleOfView2Model/*=0.01*/)
{
	ocs=_ocs;
	m_fScaleOfView2Model=scaleOfView2Model;
}
//
void CVectorMonoImage::SetStrokePixel(long xI,long yJ,long idRelaObj/*=0*/)
{
	PROFILE_PIXEL* pPixel=hashStrokePixels.Add(MkDW(xI,yJ));	//保证不会出现仅对角连续
	pPixel->idRelaObj=idRelaObj;
	m_nWidth = max(m_nWidth, xI);
	m_nHeight = max(m_nHeight, yJ);
}
//
void CVectorMonoImage::DrawPoint(const double* point,long idRelaObj/*=0*/)
{
	int xI=point[0]>=0?(int)(point[0]+0.5):(int)(point[0]-0.5);
	int yJ=point[1]>=0?(int)(point[1]+0.5):(int)(point[1]-0.5);
	SetStrokePixel(xI, yJ, idRelaObj);
}
void CVectorMonoImage::DrawLine(const double* pxStart,const double* pxEnd,long idRelaObj/*=0*/)
{
	double start[3],end[3];
	bool swapping=false;
	if(pxStart[1]>pxEnd[1]+1)
		swapping=true;	//优先自上向下绘
	else if(fabs(pxStart[1]-pxEnd[1])<1&&pxStart[0]>pxEnd[0])
		swapping=true;	//接近水平时（差别在一个像素）优先从左向右绘
	if(swapping)
	{	//防止同一根线绘制始->终与终->始间因舍入误差带来的差异
		memcpy(start,pxEnd,3*sizeof(double));
		memcpy(end,pxStart,3*sizeof(double));
	}
	else
	{
		memcpy(start,pxStart,3*sizeof(double));
		memcpy(end,pxEnd,3*sizeof(double));
	}
	double sx=start[0];
	double sy=start[1];
	double sz=start[2];
	double ex=end[0];
	double ey=end[1];
	double ez=end[2];
	double dy=end[1]-start[1];
	double dx=end[0]-start[0];
	if(fabs(dy)+fabs(dx)<0.01)
		return;	//跳过垂直于当前绘图平面的直线
	int xPrevI=-1,yPrevJ=-1;
	if(fabs(dy)>fabs(dx))
	{	//直线与Y轴更靠近
		double coefX2Y= dx / dy;
		//必须圆整起点坐标，否则会导致由于投影共线的不同线段绘制时不能完全重叠
		int syi = (int)(0.5 + sy);
		sx += coefX2Y * (syi - sy);
		sy = syi;
		int stepX = 0, stepY = ey > sy ? 1 : -1;
		if (ex > sx + 0.0001)
			stepX = 1;
		else if (ex < sx - 0.0001)
			stepX = -1;
		for(double yfJ=sy;(yfJ<=ey&&ey>sy)||(yfJ>=ey&&ey<sy);yfJ+=stepY)
		{
			int xI=(int)(0.5+sx+coefX2Y*(yfJ-sy));
			int yJ=(int)(0.5+yfJ);
			SetStrokePixel(xI,yJ,idRelaObj);
			//
			if(xPrevI>=0&&yPrevJ>=0&&(abs(yJ-yPrevJ)+abs(xI-xPrevI)==2))
				SetStrokePixel(xI-stepX,yJ,idRelaObj);	//保证不会出现仅对角连续
			xPrevI=xI;
			yPrevJ=yJ;
		}
	}
	else
	{	//直线与X轴更靠近
		double coefY2X = dy / dx;
		//必须圆整起点坐标，否则会导致由于投影共线的不同线段绘制时不能完全重叠
		int sxi = (int)(0.5 + sx);
		sy += coefY2X * (sxi - sx);
		sx = sxi;
		int stepX = ex > sx ? 1 : -1, stepY = 0;
		if (ey > sy + 0.0001)
			stepY = 1;
		else if (ey < sy - 0.0001)
			stepY = -1;
		for(double xfI=sx;(xfI<=ex&&ex>sx)||(xfI>=ex&&ex<sx);xfI+=stepX)
		{
			int xI=(int)(0.5+xfI);
			int yJ=(int)(0.5+sy+coefY2X*(xfI-sx));
			SetStrokePixel(xI,yJ,idRelaObj);
			//
			if(xPrevI>=0&&yPrevJ>=0&&(abs(yJ-yPrevJ)+abs(xI-xPrevI)==2))
				SetStrokePixel(xI,yJ-stepY,idRelaObj);	//保证不会出现仅对角连续
			xPrevI=xI;
			yPrevJ=yJ;
		}
	}
}
//
void CVectorMonoImage::DrawLine(f3dLine line, long idRelaObj /*= 0*/)
{
	GEPOINT ptS = ocs.TransPToCS(line.startPt);
	GEPOINT ptE = ocs.TransPToCS(line.endPt);
	if (m_fScaleOfView2Model>0)
	{
		ptS *= m_fScaleOfView2Model;
		ptE *= m_fScaleOfView2Model;
	}
	return DrawLine(ptS, ptE, idRelaObj);
}
//绘制圆弧段，将圆弧段简化成若干个直线段
void CVectorMonoImage::DrawArcLine(f3dArcLine arc_line, long idRelaObj /*= 0*/)
{
	GEPOINT ptS, ptE;
	if (arc_line.SectorAngle() == 0)
	{
		ptS = arc_line.Start();
		ptE = arc_line.End();
		if (ptS.IsEqual(ptE, EPS))
			return;
		DrawLine(f3dLine(ptS, ptE), idRelaObj);
	}
	else
	{	
		int nSlices = 4;
		double angle = arc_line.SectorAngle() / nSlices;
		ptS = arc_line.Start();
		for (int i = 1; i <= nSlices; i++)
		{
			ptE = arc_line.PositionInAngle(angle*i);
			DrawLine(f3dLine(ptS, ptE), idRelaObj);
			ptS = ptE;
		}
	}
}
//将轮廓像素在画布上进行简化布局
bool CVectorMonoImage::LayoutImage()
{
	if (m_nHeight <= 0 || m_nWidth <= 0)
		return false;	//
	CString sProcess = "初始化钢板的外轮廓区域.....";
	if (DisplayProcess)
		DisplayProcess(0, sProcess.GetBuffer());
	//初始化像素带列
	int nStripScroll = m_nWidth / 16;
	xarrVertStrips.SetSize(nStripScroll + 1);
	VERTSTRIP16* pStrip = NULL;
	int index = 1, nNum = xarrVertStrips.GetSize() * 2 + hashStrokePixels.GetNodeNum();
	for (int i = 0; i < xarrVertStrips.GetSize(); i++)
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * index / nNum), sProcess.GetBuffer());
		xarrVertStrips[i].xarrEntries.SetSize(0, 512);
		pStrip = (VERTSTRIP16*)xarrVertStrips[i].xarrEntries.Append(VERTSTRIP_HEADER(1));
		pStrip->wixLeft = i * 16;
		pStrip->wiyTop = 0;
		pStrip->wiyBtm = WORD(m_nHeight + 1);
		index++;
	}
	//处理轮廓边像素
	for (PROFILE_PIXEL* pPixel = hashStrokePixels.GetFirst(); pPixel; pPixel = hashStrokePixels.GetNext())
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * index / nNum), sProcess.GetBuffer());
		int iStripScroll = pPixel->wiX / 16;
		STRIP_SCROLL* pCurrScroll = xarrVertStrips.GetAt(iStripScroll);
		if (pCurrScroll == NULL)
			return false;
		pCurrScroll->CheckProfilePixel(pPixel->wiX, pPixel->wiY);
		index++;
	}
	//进行排序
	for(STRIP_SCROLL* pScroll=xarrVertStrips.GetFirst();pScroll;pScroll=xarrVertStrips.GetNext())
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * index / nNum), sProcess.GetBuffer());
		int nCount = pScroll->xarrEntries.GetSize();
		VERTSTRIP_HEADER* pHead=pScroll->xarrEntries.m_pData;
		CQuickSort<VERTSTRIP_HEADER>::QuickSort(pHead, nCount, compare_func);
		index++;
	}
	if (DisplayProcess)
		DisplayProcess(100, sProcess.GetBuffer());
	return true;
}
//
VERTSTRIP_HEADER* CVectorMonoImage::GetStripHead(int xI, int yJ)
{
	int nWidth = xarrVertStrips.GetSize()*16;
	int nHight = m_nHeight+1;
	if (xI<0 || xI>nWidth || yJ<0 || yJ>nHight)
		return NULL;
	int iStripScroll = xI / 16;
	STRIP_SCROLL* pCurrScroll = xarrVertStrips.GetAt(iStripScroll);
	if (pCurrScroll == NULL)
		return NULL;
	for (VERTSTRIP_HEADER* pHead = pCurrScroll->xarrEntries.GetFirst(); pHead; pHead = pCurrScroll->xarrEntries.GetNext())
	{
		if (pHead->cbHeadByte == 0)
		{	//格栅像素行
			STRIP16_ROW* pRowPixels = (STRIP16_ROW*)pHead;
			if (pRowPixels->yRowJ == yJ)
				return pHead;
		}
		else
		{	//条幅带区
			VERTSTRIP16* pVertStrip = (VERTSTRIP16*)pHead;
			if (yJ >= pVertStrip->wiyTop && yJ <= pVertStrip->wiyBtm)
				return pHead;
		}
	}
	return NULL;
}
BYTE CVectorMonoImage::StateAt(int xI, int yJ)
{
	VERTSTRIP_HEADER* pHead = GetStripHead(xI,yJ);
	if (pHead == NULL)
		return -1;
	if (pHead->cbHeadByte == 0)
	{	//格栅像素行
		STRIP16_ROW* pRowPixels = (STRIP16_ROW*)pHead;
		int xiLocal = xI % 16;
		if (pRowPixels->IsBlckPixel(xiLocal))
			return BOUNDARY;	//轮廓像素
		else if (pRowPixels->IsConnState(xiLocal))
			return CONNECTED;
		else
			return NONEVISIT;
	}
	else
	{	//条幅带区
		VERTSTRIP16* pVertStrip = (VERTSTRIP16*)pHead;
		if (pVertStrip->ciDetectState > 0)
			return CONNECTED;
		else
			return NONEVISIT;
	}
	return -1;
}
void CVectorMonoImage::SetConnStateAt(int xI, int yJ)
{
	VERTSTRIP_HEADER* pHead = GetStripHead(xI, yJ);
	if (pHead == NULL)
		return;
	if (pHead->cbHeadByte == 0)
	{	//格栅像素行
		STRIP16_ROW* pRowPixels = (STRIP16_ROW*)pHead;
		int xiLocal = xI % 16;
		if (!pRowPixels->IsBlckPixel(xiLocal))
			pRowPixels->SetConnState(xiLocal);
	}
	else
	{	//条幅带区
		VERTSTRIP16* pVertStrip = (VERTSTRIP16*)pHead;
		if (pVertStrip->ciDetectState == 0)
			pVertStrip->ciDetectState = 1;
	}
}
//访问空白连通区域
void CVectorMonoImage::VisitBlankConnPixels()
{
	UINT nNum = m_nWidth * m_nHeight, index = 1;
	if (nNum <= 0)
		return;
	CString sProcess = "智能识别钢板的外轮廓边.....";
	if (DisplayProcess)
		DisplayProcess(0, sProcess.GetBuffer());
	std::stack< std::pair<int, int> > xNearPixels;
	xNearPixels.push(std::pair<int, int>(0, 0));
	SetConnStateAt(0, 0);
	while (!xNearPixels.empty())
	{
		if (DisplayProcess)
			DisplayProcess(int(100 * index / nNum), sProcess.GetBuffer());
		//处理栈顶像素，并出栈
		std::pair<int, int> curPixel = xNearPixels.top();
		int wiCurX = curPixel.first;
		int wiCurY = curPixel.second;
		xNearPixels.pop();	//出栈
		//检查该像素的相邻像素
		BYTE ciState = 0;
		int iX = 0, iY = 0;
		VERTSTRIP_HEADER* pHead = GetStripHead(wiCurX, wiCurY);
		if (pHead->cbHeadByte == 0)
		{	//格栅像素行
			for (int i = 0; i < 4; i++)
			{
				iX = wiCurX + (int)arr4NearPos[i].x;
				iY = wiCurY + (int)arr4NearPos[i].y;
				ciState = StateAt(iX, iY);
				if (ciState == NONEVISIT)
				{
					SetConnStateAt(iX, iY);	//设置连通状态
					xNearPixels.push(std::pair<int, int>(iX, iY));
				}
			}
			index += 4;
		}
		else
		{	//条幅带区
			VERTSTRIP16* pVertStrip = (VERTSTRIP16*)pHead;
			for (int i = 0; i < 16; i++)
			{	//区域上侧
				iX = pVertStrip->wixLeft + i;
				iY = pVertStrip->wiyTop - 1;
				ciState = StateAt(iX, iY);
				if (ciState == NONEVISIT)
				{
					SetConnStateAt(iX, iY);	//设置连通状态
					xNearPixels.push(std::pair<int, int>(iX, iY));
				}
				//区域下侧
				iY = pVertStrip->wiyBtm + 1;
				ciState = StateAt(iX, iY);
				if (ciState == NONEVISIT)
				{
					SetConnStateAt(iX, iY);	//设置连通状态
					xNearPixels.push(std::pair<int, int>(iX, iY));
				}
			}
			for (int j = pVertStrip->wiyTop; j <= pVertStrip->wiyBtm; j++)
			{	//区域左侧
				iX = pVertStrip->wixLeft -1;
				iY = j;
				ciState = StateAt(iX, j);
				if (ciState == NONEVISIT)
				{
					SetConnStateAt(iX, iY);	//设置连通状态
					xNearPixels.push(std::pair<int, int>(iX, iY));
				}
				//区域右侧
				iX = pVertStrip->wixLeft + 16;
				ciState = StateAt(iX, iY);
				if (ciState == NONEVISIT)
				{
					SetConnStateAt(iX, iY);	//设置连通状态
					xNearPixels.push(std::pair<int, int>(iX, iY));
				}
			}
			index += 16 * pVertStrip->niHeight;
		}
	}
	if (DisplayProcess)
		DisplayProcess(100, sProcess.GetBuffer());
}
//检测轮廓边像素
void CVectorMonoImage::DetectProfilePixelsByVisit()
{
	//初始化画布上像素，对像素进行分类布局
	LayoutImage();
	//访问连通区域
	VisitBlankConnPixels();
	//获取外轮廓像素点
	PROFILE_PIXEL* pPixel = NULL;
	for(pPixel=hashStrokePixels.GetFirst();pPixel;pPixel=hashStrokePixels.GetNext())
	{
		for (int index = 0; index < 8; index++)
		{
			int iX = pPixel->wiX + (int)arr8NearPos[index].x;
			int iY = pPixel->wiY + (int)arr8NearPos[index].y;
			if (StateAt(iX, iY) == CONNECTED)
			{
				OBJECT_ITEM* pItem = m_hashRelaObj.GetValue(pPixel->idRelaObj);
				if (pItem == NULL)
					pItem = m_hashRelaObj.Add(pPixel->idRelaObj);
				pItem->m_idRelaObj = pPixel->idRelaObj;
				pItem->m_nPixelNum += 1;
				break;
			}
		}
	}
}
//判断是否为起始轮廓像素
BOOL CVectorMonoImage::IsStartPixel(PROFILE_PIXEL* pPixel,CHashList<BYTE>* pHashMarkBlank /*= NULL*/)
{
	if (pPixel == NULL)
		return FALSE;
	if (pPixel->ciFlag != 0)
		return FALSE;
	//取该像素的顶部像素，如果为空则该像素为起始
	WORD wiX = pPixel->wiX;
	WORD wiY = pPixel->wiY;
	if (pHashMarkBlank && pHashMarkBlank->GetNodeNum()>0)
	{
		if (pHashMarkBlank->GetValue(MkDW(wiX, wiY - 1))!=NULL)
			return TRUE;
	}
	else
	{
		if (hashStrokePixels.GetValue(MkDW(wiX, wiY - 1))==NULL)
			return TRUE;
	}
	return FALSE;
}
//根据指定位置查看像素点
PROFILE_PIXEL* CVectorMonoImage::CheckPixelAt(int xI, int yJ,PROFILE_PIXEL *pTagPixel, CHashList<BYTE>& hashMarkBlank)
{
	PROFILE_PIXEL* pPixel = hashStrokePixels.GetValue(MkDW(xI, yJ));
	if (pPixel == NULL)
	{
		hashMarkBlank.SetValue(MkDW(xI, yJ),1);
		return NULL;
	}
	if (pPixel == pTagPixel)
		return pPixel;	//找到终止像素点
	if (pPixel->ciFlag != 0)
		return NULL;
	return pPixel;
}
//跟踪外轮廓像素点(8邻接关系进行判断)
BOOL CVectorMonoImage::TrackProfilePixels(PROFILE_PIXEL* pStartPixel,ARRAY_LIST<long>& listPixel)
{
	BOOL bValid = FALSE;
	PROFILE_PIXEL *pPixel = NULL, *pTagPixel = pStartPixel;
	PROFILE_PIXEL *pPrePixel = NULL, *pNextPixel = NULL, *pCurPixel = pStartPixel;
	listPixel.Empty();
	listPixel.SetSize(0, 512);
	CHashList<BYTE> hashMarkBlank;
	while (true)
	{
		if (IsStartPixel(pCurPixel,&hashMarkBlank))
		{	//起始像素点，从右上顺时针检测
			for (int i = 0; i < 8; i++)
			{
				int index = (i + 7) % 8;
				int iX = pCurPixel->wiX + (int)arr8NearPos[index].x;
				int iY = pCurPixel->wiY + (int)arr8NearPos[index].y;
				if(pNextPixel = CheckPixelAt(iX, iY, pTagPixel, hashMarkBlank))
					break;
			}
		}
		else
		{	//中间像素点，从上一个像素点做加2取模运算
			int posX = pPrePixel->wiX - pCurPixel->wiX;
			int posY = pPrePixel->wiY - pCurPixel->wiY;
			int iStart = 0;
			for (iStart = 0; iStart < 8; iStart++)
			{
				if (arr8NearPos[iStart].x == posX &&
					arr8NearPos[iStart].y == posY)
					break;
			}
			for (int i = 0; i < 8; i++)
			{
				int index = (i + iStart + 2) % 8;
				int iX = pCurPixel->wiX + (int)arr8NearPos[index].x;
				int iY = pCurPixel->wiY + (int)arr8NearPos[index].y;
				if (pNextPixel = CheckPixelAt(iX, iY, pTagPixel, hashMarkBlank))
					break;
			}
		}
		if (pNextPixel == NULL)
		{	//找不到闭合外轮廓
			bValid = FALSE;
			break;
		}
		else
		{	//记录像素点
			pCurPixel->ciFlag = 1;
			listPixel.append(pCurPixel->idLocation);
		}
		if (listPixel.GetSize()>10 && pNextPixel == pStartPixel)
		{	//找到闭合外轮廓
			bValid = TRUE;
			break;
		}
		//
		pPrePixel = pCurPixel;
		pCurPixel = pNextPixel;
	}
	//识别失败，删除像素标签
	if (!bValid)
	{
		for (int i = 0; i < listPixel.GetSize(); i++)
		{
			pPixel = hashStrokePixels.GetValue(listPixel[i]);
			pPixel->ciFlag = 0;
		}
	}
	return bValid;
}
//查找所有闭合外轮廓像素点
void CVectorMonoImage::DetectProfilePixelsByTrack()
{
	CString sProcess = "智能识别钢板的外轮廓边.....";
	if(DisplayProcess)
		DisplayProcess(0, sProcess.GetBuffer());
	UINT nNum = m_nWidth * m_nHeight, index = 0;
	PROFILE_PIXEL* pPixel = NULL;
	for (int xI = 1; xI < m_nWidth; xI++)
	{
		for (int yJ = 1; yJ < m_nHeight; yJ++)
		{
			index = xI * m_nHeight + yJ;
			if(DisplayProcess)
				DisplayProcess(int(100 * index / nNum), sProcess.GetBuffer());
			pPixel = hashStrokePixels.GetValue(MkDW(xI, yJ));
			if (!IsStartPixel(pPixel))
				continue;	//非起始轮廓点
			ARRAY_LIST<long> listProfilePixel;
			if (!TrackProfilePixels(pPixel, listProfilePixel))
				continue;
			ARRAY_LIST<f3dPoint> ptArr;
			for (int i = 0; i < listProfilePixel.GetSize(); i++)
			{
				pPixel = hashStrokePixels.GetValue(listProfilePixel[i]);
				OBJECT_ITEM* pItem = m_hashRelaObj.GetValue(pPixel->idRelaObj);
				if (pItem==NULL)
					pItem = m_hashRelaObj.Add(pPixel->idRelaObj);
				pItem->m_idRelaObj = pPixel->idRelaObj;
				pItem->m_nPixelNum += 1;
				if(i%10==0)
					ptArr.append(f3dPoint(pPixel->wiX,pPixel->wiY));
			}
			//去除已处理的像素和闭合轮廓内部干扰像素
			POLYGON region;
			region.CreatePolygonRgn(ptArr.m_pData, ptArr.GetSize());
			for (pPixel = hashStrokePixels.GetFirst(); pPixel; pPixel = hashStrokePixels.GetNext())
			{
				if (pPixel->ciFlag != 0)
					hashStrokePixels.DeleteCursor();
				int iRet = region.PtInRgn2(GEPOINT(pPixel->wiX, pPixel->wiY));
				if (iRet == 1)
					hashStrokePixels.DeleteCursor();
			}
			hashStrokePixels.Clean();
		}
	}
	if (DisplayProcess)
		DisplayProcess(100, sProcess.GetBuffer());
}
#ifdef __MONO_IMAGE_
#include "MonoImage.h"
#endif
//通过像素哈希表生成BMP文件
int CVectorMonoImage::WriteBmpFileByPixelHash(const char* fileName,BOOL bValidProfile/*=FALSE*/)
{
#ifdef __MONO_IMAGE_
	CMonoImage xImage(NULL,m_nWidth+2,m_nHeight+2);
	for (PROFILE_PIXEL* pPixel = hashStrokePixels.GetFirst(); pPixel; pPixel = hashStrokePixels.GetNext())
	{
		if(!bValidProfile || (bValidProfile && pPixel->ciFlag > 0))
			xImage.SetPixelState(pPixel->wiX, pPixel->wiY);
	}
	return xImage.WriteMonoBmpFile(fileName);
#else
	return 0;
#endif
}
//通过像素带生成BMP文件
int CVectorMonoImage::WriteBmpFileByVertStrip(const char* fileName, BOOL bConnPixel /*= FALSE*/)
{
#ifdef __MONO_IMAGE_
	CMonoImage xImage(NULL, m_nWidth + 2, m_nHeight + 2);
	for(int i=0;i<xarrVertStrips.GetSize();i++)
	{
		STRIP_SCROLL* pScroll = (STRIP_SCROLL*)&xarrVertStrips[i];
		for (WORD j = 0; j < pScroll->xarrEntries.Count; j++)
		{
			if (pScroll->xarrEntries[j].cbHeadByte == 0)
			{
				STRIP16_ROW* pRowPixels = (STRIP16_ROW*)&(pScroll->xarrEntries[j]);
				for (int nn = 0; nn < 16; nn++)
				{
					if(pRowPixels->IsBlckPixel(nn))
						xImage.SetPixelState(i * 16 + nn, pRowPixels->yRowJ);
					if (bConnPixel && pRowPixels->IsConnState(nn))
						xImage.SetPixelState(i * 16 + nn, pRowPixels->yRowJ);
				}
			}
			else if(bConnPixel)
			{
				VERTSTRIP16* pVertStrip = (VERTSTRIP16*)&(pScroll->xarrEntries[j]);
				if (pVertStrip->ciDetectState == 0)
					continue;	//非连通区
				for (int mm = 0; mm < 16; mm++)
					for (int nn = pVertStrip->wiyTop; nn <= pVertStrip->wiyBtm; nn++)
						xImage.SetPixelState(pVertStrip->wixLeft + mm, nn);
			}
		}
	}
	return xImage.WriteMonoBmpFile(fileName);
#else
	return 0;
#endif
}
