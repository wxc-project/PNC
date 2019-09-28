#include "StdAfx.h"
#include "DrawUnit.h"
#include "SortFunc.h"
#include "PNCModel.h"
#include "XhMath.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//
CDrawingRect::CDrawingRect(const CDrawingRect &srcRect)
{
	Clone(srcRect);
}
CDrawingRect& CDrawingRect::operator =(const CDrawingRect &srcRect)
{
	Clone(srcRect);
	return *this;
}
void CDrawingRect::Clone(const CDrawingRect &srcRect)
{
	m_bException = srcRect.m_bException;
	m_bLayout = srcRect.m_bLayout;
	topLeft = srcRect.topLeft;
	initTopLeft = srcRect.initTopLeft;
	width = srcRect.width;
	height = srcRect.height;
	m_pDrawing = srcRect.m_pDrawing;
	m_vertexArr.Empty();
	for (f3dPoint *pPt = (const_cast<CDrawingRect*>(&srcRect))->m_vertexArr.GetFirst(); pPt;
		pPt = (const_cast<CDrawingRect*>(&srcRect))->m_vertexArr.GetNext())
		m_vertexArr.append(*pPt);
}

BOOL CDrawingRect::GetPolygon(POLYGON &rgn)
{
	if (m_vertexArr.GetSize() <= 0)
		return FALSE;
	return rgn.CreatePolygonRgn(m_vertexArr.Data(),m_vertexArr.GetSize());
}

//
int compareDrawingRect(const CDrawingRect &drawRect1,const CDrawingRect &drawRect2)
{
	if(drawRect1.width>drawRect2.width)
		return -1;
	else if(drawRect1.width<drawRect2.width)
		return 1;
	else if(drawRect1.height>drawRect2.height)
		return -1;
	else if(drawRect1.height<drawRect2.height)
		return 1;
	else
		return 0;
}
double IsOverlap(double start, double end, double start1,double end1)
{
	double fOverlapLen = 0;
	if (start1<end&&start1>start)
	{
		if (end1 > end)
			fOverlapLen = end - start1;
		else
			fOverlapLen = end1 - start1;
	}
	else if (end1<end&&end1>start)
	{
		if (start1 < start)
			fOverlapLen = end1 - start;
		else
			fOverlapLen = end1 - start1;
	}
	else if (start<end1&&start>start1)
	{
		if (end > end1)
			fOverlapLen = end1 - start;
		else
			fOverlapLen = end - start;
	}
	else if (end<end1&&end>start1)
	{
		if (start < start1)
			fOverlapLen = end - start1;
		else
			fOverlapLen = end - start;
	}
	return fOverlapLen;
}
BOOL IsOrthoRectInters(f3dPoint topLeft1,double width1,double height1,f3dPoint topLeft2,double width2,double height2,
					   double *pfLeftDist=NULL,double *pfRightDist=NULL,double *pfTopDist=NULL,double *pfBottomDist=NULL)
{
	if((width1==0&&height1==0)||(width2==0&&height2==0))
		return false;
	if(topLeft2.x>topLeft1.x+width1)
		return FALSE;	//第二个矩形在第一个矩形的右侧
	else if(topLeft2.y<topLeft1.y-height1)
		return FALSE;	//第二个矩形在第一个矩形的下侧
	else if(topLeft2.x+width2<topLeft1.x)
		return FALSE;	//第二个矩形在第一个矩形的左侧
	else if(topLeft2.y-height2>topLeft1.y)
		return FALSE;	//第二个矩形在第一个矩形的上侧
	else
	{
		f3dPoint center1(topLeft1.x + 0.5*width1, topLeft1.y - 0.5*height1);
		f3dPoint center2(topLeft2.x + 0.5*width2, topLeft2.y - 0.5*height2);
		double fOverlapX = IsOverlap(topLeft1.x, topLeft1.x + width1, topLeft2.x, topLeft2.x + width2);
		double fOverlapY = IsOverlap(topLeft1.y - height1, topLeft1.y, topLeft2.y - height2, topLeft2.y);
		if (fOverlapX > 0 || fOverlapY > 0)
		{
			if (pfLeftDist)
			{
				if (fOverlapY > fOverlapX && center1.x < center2.x)	//第二个矩形在第一个矩形右侧，相交时返回左侧交叉长度
					*pfLeftDist = (topLeft1.x + width1) - topLeft2.x;
				else
					*pfLeftDist = 0;
			}
			if (pfRightDist)
			{
				if (fOverlapY > fOverlapX && center1.x > center2.x)	//第二个矩形在第一个矩形左侧，相交时返回右侧交叉长度
					*pfRightDist = (topLeft2.x + width1) - topLeft1.x;
				else
					*pfRightDist = 0;
			}
			if (pfTopDist)
			{
				if (fOverlapX > fOverlapY && center1.y > center2.y)	//第二个矩形在第一个矩形下侧，相交时返回上侧交叉区域长度
					*pfTopDist = topLeft2.y - (topLeft1.y - height1);
				else
					*pfTopDist = 0;
			}
			if (pfBottomDist)
			{
				if (fOverlapX > fOverlapY && center1.y < center2.y)	//第二个矩形在第一个矩形上侧，相交时返回上侧交叉区域长度
					*pfBottomDist = topLeft1.y - (topLeft2.y - height1);
				else
					*pfBottomDist = 0;
			}
		}
		return TRUE;
	}
}
//////////////////////////////////////////////////////////////////////////
//
CDrawingRectLayout::CDrawingRectLayout()
{
	m_fFrameWidth=m_fFrameHeight=0;
}

int CompareSideLinePoint(const GEPOINT& item1, const GEPOINT& item2)
{	//Y从大到小，X从大到小
	if (item1.y > item2.y)
		return -1;
	else if (item1.y < item2.y)
		return 1;
	else if (item1.x > item2.x)
		return -1;
	else if (item1.x < item2.x)
		return 1;
	else
		return 0;
}
//X坐标从小到大,Y坐标从小到大
typedef f3dPoint* PT_PTR;
int ComparePtPtr(const PT_PTR& item1, const PT_PTR& item2)
{
	if (item1->x > item2->x)
		return 1;
	else if (item1->x < item2->x)
		return -1;
	else if (item1->y > item2->y)
		return 1;
	else if (item1->y < item2->y)
		return -1;
	else
		return 0;
}

void CDrawingRectLayout::UpdateSideLinePosX(CDrawingRect &rect)
{
	if (m_arrSideLinePt.GetSize() == 0)
	{
		m_arrSideLinePt.append(GEPOINT(0,0));
		m_arrSideLinePt.append(GEPOINT(0,-m_fFrameHeight));
	}
	//1. 查找最大Y、最小Y取右侧轮廓点
	int iMinYIndex = 0,iMaxYIndex = 0;
	double fMinY = 1000000, fMaxY = -100000;
	f3dPoint *pMaxVertexY = NULL, *pMinVertexY = NULL;
	for (int i = 0; i < rect.m_vertexArr.GetSize(); i++)
	{
		if (pMaxVertexY == NULL && pMinVertexY == NULL)
		{	//初始化
			pMaxVertexY = &rect.m_vertexArr[i];
			pMinVertexY = &rect.m_vertexArr[i];
			fMinY = fMaxY = rect.m_vertexArr[i].y;
			iMinYIndex = iMaxYIndex = i;
			continue;
		}
		if ((rect.m_vertexArr[i].y > fMaxY)||
			(rect.m_vertexArr[i].y == fMaxY && rect.m_vertexArr[i].x > pMaxVertexY->x))
		{
			pMaxVertexY = &rect.m_vertexArr[i];
			fMaxY = pMaxVertexY->y;
			iMaxYIndex = i;
		}
		if ((rect.m_vertexArr[i].y < fMinY) ||
			(rect.m_vertexArr[i].y == fMinY && rect.m_vertexArr[i].x > pMinVertexY->x))
		{
			pMinVertexY = &rect.m_vertexArr[i];
			fMinY = pMinVertexY->y;
			iMinYIndex = i;
		}
	}
	if (iMinYIndex == 0 && iMaxYIndex == 0)
		return;
	//2. 根据上下关键点（Y坐标最大、最小）将轮廓点分为两组，取右侧轮廓点
	int iMaxIndex = max(iMinYIndex, iMaxYIndex);
	int iMinIndex = min(iMinYIndex, iMaxYIndex);
	ARRAY_LIST<f3dPoint*> leftVertexArr, rightVertexArr;
	double fSumVertexX1 = 0, fSumVertexX2 = 0;
	for (int i = iMinIndex; i <= iMaxIndex; i++)
	{
		leftVertexArr.append(&rect.m_vertexArr[i]);
		fSumVertexX1 += rect.m_vertexArr[i].x;
	}
	int n = rect.m_vertexArr.GetSize();
	for (int i = iMaxIndex; i <= iMinIndex + n; i++)
	{
		rightVertexArr.append(&rect.m_vertexArr[i%n]);
		fSumVertexX2 += rect.m_vertexArr[i%n].x;
	}
	double fAverageX1 = leftVertexArr.GetSize() > 0 ? fSumVertexX1 / leftVertexArr.GetSize() : 0;
	double fAverageX2 = rightVertexArr.GetSize() > 0 ? fSumVertexX2 / rightVertexArr.GetSize() : 0;
	if (fAverageX1 > fAverageX2)
	{
		rightVertexArr.Empty();
		for (int i = 0; i < leftVertexArr.GetSize(); i++)
			rightVertexArr.append(leftVertexArr[i]);
	}
	//3. 将右侧轮廓点添加切割线中
	ARRAY_LIST<GEPOINT> tempPtList;
	for (int i = 0; i < m_arrSideLinePt.GetSize(); i++)
		tempPtList.append(m_arrSideLinePt[i]);
	//
	m_arrSideLinePt.Empty();
	BOOL bAddNewPtArr = TRUE;
	double fRealMinY = rect.topLeft.y + fMinY;
	double fRealMaxY = rect.topLeft.y + fMaxY;
	//找到临界最大Y坐标临界点、最小Y坐标临界点
	f3dLine minYLine, maxYLine;
	BOOL bFoundMinYLine = FALSE, bFoundMaxYLine = FALSE;
	for (int i = 0; i < tempPtList.GetSize(); i++)
	{
		GEPOINT pt = tempPtList[i];
		if (pt.y > fRealMinY && !bFoundMinYLine)
		{
			if (i > 0)
			{
				minYLine.startPt.Set(tempPtList[i - 1].x, tempPtList[i - 1].y);
				minYLine.endPt.Set(pt.x,pt.y);
				bFoundMinYLine = true;
			}
		}
		if (pt.y > fRealMaxY && !bFoundMaxYLine)
		{
			if (i > 0)
			{
				maxYLine.startPt.Set(tempPtList[i - 1].x, tempPtList[i - 1].y);
				minYLine.endPt.Set(pt.x,pt.y);
				bFoundMaxYLine = TRUE;
			}
		}
	}
	for (int i = 0; i < tempPtList.GetSize(); i++)
	{
		GEPOINT pt = tempPtList[i];
		if (pt.y < fRealMinY)
			m_arrSideLinePt.append(pt);
		else if (pt.y > fRealMaxY)
			m_arrSideLinePt.append(pt);
		else if(bAddNewPtArr && rightVertexArr.Count>0)
		{
			int nRighVertexCount = rightVertexArr.Count;
			f3dPoint *pFirstPt = rightVertexArr[0];
			f3dPoint *pTailPt = rightVertexArr[nRighVertexCount-1];
			f3dPoint firstPt = *pFirstPt;
			firstPt.y -= rect.height;
			f3dPoint tailPt = *pTailPt;
			tailPt.y -= rect.height;

			f3dLine line;
			f3dPoint inter_pt;
			if (bFoundMaxYLine)
			{
				line.startPt.Set(firstPt.x, firstPt.y);
				line.endPt.Set(firstPt.x - 10000, firstPt.y);
				int nRetCode = Int3dll(maxYLine, line, inter_pt);
				if (nRetCode == 1 || nRetCode == 2)
					m_arrSideLinePt.append(GEPOINT(inter_pt));
			}
			for (int i = 0; i < rightVertexArr.GetSize(); i++)
			{
				f3dPoint vertex = *rightVertexArr[i];
				vertex.y -= rect.height;	//原轮廓点坐标以左下角为基准，需调整为以左上角为准 wht 19-08-01
				f3dPoint pt = rect.topLeft + vertex;
				m_arrSideLinePt.append(pt);
			}
			if (bFoundMinYLine && pFirstPt)
			{
				line.startPt.Set(tailPt.x, tailPt.y);
				line.endPt.Set(tailPt.x - 10000, tailPt.y);
				int nRetCode = Int3dll(maxYLine, line, inter_pt);
				if (nRetCode == 1 || nRetCode == 2)
					m_arrSideLinePt.append(GEPOINT(inter_pt));
			}
			bAddNewPtArr = FALSE;
		}
	}
	//4. 排序并合并合并距离过近的内容
	CHeapSort<GEPOINT>::HeapSort(m_arrSideLinePt.Data(), m_arrSideLinePt.GetSize(), CompareSideLinePoint);
	const int MIN_SPACE = 5;
	for (int i = 1; i < m_arrSideLinePt.GetSize()-1; i++)
	{
		if ((m_arrSideLinePt[i].y - m_arrSideLinePt[i - 1].y) < MIN_SPACE &&
			(m_arrSideLinePt[i + 1].y - m_arrSideLinePt[i].y) < MIN_SPACE)
			m_arrSideLinePt.RemoveAt(i);
	}
}
BOOL CDrawingRectLayout::CloseToToLeftTopCorner(CDrawingRect &rect)
{	//找到最左侧的两个轮廓点
	ARRAY_LIST<f3dPoint*> ptPtrArr;
	for (int i = 0; i < rect.m_vertexArr.GetSize(); i++)
		ptPtrArr.append(&rect.m_vertexArr[i]);
	CHeapSort<f3dPoint*>::HeapSort(ptPtrArr.Data(), ptPtrArr.GetSize(), ComparePtPtr);
	f3dPoint ptArr[2] = { *ptPtrArr[0],*ptPtrArr[1] };
	ptArr[0].y -= rect.height;	//将已左下角为基准的坐标转为以左上角为基准 wht 19-08-01
	ptArr[1].y -= rect.height;
	ptArr[0] += rect.topLeft;
	ptArr[1] += rect.topLeft;
	double fLeftMoveDist=-1;
	for (int i = 0; i < 2; i++)
	{
		f3dPoint pt = ptArr[i];
		for (int j = 0; j < m_arrSideLinePt.GetSize()-1; j++)
		{
			f3dPoint start = m_arrSideLinePt[j];
			f3dPoint end = m_arrSideLinePt[j + 1];
			if (start.y > end.y)
			{
				start = m_arrSideLinePt[j + 1];
				end = m_arrSideLinePt[j];
			}
			if (pt.y<start.y || pt.y>end.y)
				continue;
			f2dLine line1,line2;
			line1.startPt.Set(start.x, start.y);
			line1.endPt.Set(end.x, end.y);
			line2.startPt.Set(pt.x, pt.y);
			if (start.x < end.x)
				line2.endPt.Set(start.x, pt.y);
			else
				line2.endPt.Set(end.x, pt.y);
			GEPOINT inters_pt;
			int nRetCode = Int2dll(line1, line2, inters_pt.x, inters_pt.y);
			if (nRetCode == 1 || nRetCode == 2)
			{
				double dist = pt.x - inters_pt.x;
				if (dist > 0 && (fLeftMoveDist<0 || dist < fLeftMoveDist))
					fLeftMoveDist = dist;
			}
		}
	}
	if (fLeftMoveDist > 0)
	{
		rect.topLeft.x -= fLeftMoveDist;
		return TRUE;
	}
	else
		return FALSE;
	/*
	//1.扩大rect范围，搜索左侧、上侧干涉矩形
	CDrawingRect enlargedRect;
	const int EXTEND_LEN = 20;
	enlargedRect.topLeft = rect.topLeft;
	enlargedRect.width = rect.width;
	enlargedRect.height = rect.height;
	enlargedRect.topLeft.x -= EXTEND_LEN;
	enlargedRect.topLeft.y += EXTEND_LEN;
	enlargedRect.width += 2 * EXTEND_LEN;
	enlargedRect.height += 2 * EXTEND_LEN;

	CHashList<POLYGON> hashPlateRgnByRectPtr;
	ARRAY_LIST<CDrawingRect*> leftRectPtrArr, topRectPtrArr;
	for (int k = 0; k < drawRectArr.GetSize(); k++)
	{
		if (!drawRectArr[k].m_bLayout)
			continue;	//未布置
		if (drawRectArr[k].topLeft.x + drawRectArr[k].width < enlargedRect.topLeft.x)
			continue;
		if (enlargedRect.topLeft.y - rect.height > drawRectArr[k].topLeft.y)
			continue;
		double fLeftDist = 0, fRightDist = 0, fTopDist = 0, fBottomDist = 0;
		if (IsOrthoRectInters(drawRectArr[k].topLeft, drawRectArr[k].width, drawRectArr[k].height, enlargedRect.topLeft, enlargedRect.width, enlargedRect.height,
			&fLeftDist, &fRightDist, &fTopDist, &fBottomDist))
		{
			if (fRightDist == 0 && fTopDist == 0 && fBottomDist == 0 && fLeftDist > 0)
			{
				drawRectArr[k].feature = (long)floor(fLeftDist);
				leftRectPtrArr.append(&drawRectArr[k]);
				POLYGON *pRgn = hashPlateRgnByRectPtr.Add((DWORD)&drawRectArr[k]);
				if (pRgn && !drawRectArr[k].GetPolygon(*pRgn))
					hashPlateRgnByRectPtr.DeleteNode((DWORD)&drawRectArr[k]);
			}
			else if (fLeftDist == 0 && fRightDist == 0 && fTopDist > 0 && fBottomDist == 0)
			{
				drawRectArr[k].feature = (long)floor(fTopDist);
				topRectPtrArr.append(&drawRectArr[k]);
				POLYGON *pRgn = hashPlateRgnByRectPtr.Add((DWORD)&drawRectArr[k]);
				if (pRgn && !drawRectArr[k].GetPolygon(*pRgn))
					hashPlateRgnByRectPtr.DeleteNode((DWORD)&drawRectArr[k]);
			}
		}
	}
	//2.根据矩形真实轮廓点，向左、向上靠拢
	if (hashPlateRgnByRectPtr.GetNodeNum() == leftRectPtrArr.GetSize() + topRectPtrArr.GetSize())
	{	//  向左靠拢
		int iLoopCount = 1;
		int nLeftOffset = 0, nTopOffset = 0;
		while (iLoopCount < 1000 && leftRectPtrArr.GetSize()>0)
		{
			int i = 0, j = 0;
			for (i = 0; i < rect.m_vertexArr.GetSize(); i++)
			{
				f3dPoint pt = rect.m_vertexArr[i];
				pt.x -= iLoopCount;
				for (j = 0; j < leftRectPtrArr.GetSize(); j++)
				{
					POLYGON *pRgn = hashPlateRgnByRectPtr.GetValue((DWORD)leftRectPtrArr[j]);
					int iRetCode = pRgn?pRgn->PtInRgn(pt):0;
					if (iRetCode != 2)
						break;
				}
				if (j < leftRectPtrArr.GetSize())
					break;
			}
			if (i < rect.m_vertexArr.GetSize())
			{
				nLeftOffset = max(0, iLoopCount - 1);
				break;
			}
			iLoopCount++;
		}
		// 向上靠拢
		while (iLoopCount < 1000 && topRectPtrArr.GetSize()>0)
		{
			int i = 0, j = 0;
			for (i = 0; i < rect.m_vertexArr.GetSize(); i++)
			{
				f3dPoint pt = rect.m_vertexArr[i];
				pt.y += iLoopCount;
				for (j = 0; j < topRectPtrArr.GetSize(); j++)
				{
					POLYGON *pRgn = hashPlateRgnByRectPtr.GetValue((DWORD)topRectPtrArr[j]);
					int iRetCode = pRgn ? pRgn->PtInRgn(pt) : 0;
					if (iRetCode != 2)
						break;
				}
				if (j < topRectPtrArr.GetSize())
					break;
			}
			if (i < rect.m_vertexArr.GetSize())
			{
				nTopOffset = max(0, iLoopCount - 1);
				break;
			}
			iLoopCount++;
		}
		if (nLeftOffset > 0 && nTopOffset > 0)
		{
			rect.topLeft.x -= nLeftOffset;
			rect.topLeft.y += nTopOffset;
			return TRUE;
		}
		else
			return FALSE;
	}
	else
		return FALSE;*/
}

BOOL CDrawingRectLayout::GetBlankRectTopLeft(CArray<double,double&>&startXArray,CDrawingRect &rect)
{
	int i,k,nLayoutNum=0;
	int maxY=0;
	f3dPoint origin;
	for(i=0;;i++)		//列优先搜索
	{
		if (i>0)
		{
			if(i>startXArray.GetSize())
				break;
			origin.x = startXArray[i-1]+0.1;
		}
		else
			origin.x = 0;
		if(origin.x+ rect.width>m_fFrameWidth)
			break;
		origin.y=0;
		while(origin.y- rect.height>-m_fFrameHeight)
		{
			BOOL bHasInters=FALSE;
			int num = nLayoutNum;
			nLayoutNum = 0;
			for(k=0;k<drawRectArr.GetSize();k++)
			{
				if(!drawRectArr[k].m_bLayout)
					continue;	//未布置
				nLayoutNum++;	//已布置过的矩形个数
				if (drawRectArr[k].topLeft.x+drawRectArr[k].width<origin.x)
					continue;
				if (origin.y- rect.height>drawRectArr[k].topLeft.y)
					continue;
				double fLeftDist = 0, fRightDist = 0, fTopDist = 0, fBottomDist = 0;
				if(IsOrthoRectInters(drawRectArr[k].topLeft,drawRectArr[k].width,drawRectArr[k].height,origin, rect.width, rect.height,
									&fLeftDist,&fRightDist,&fTopDist,&fBottomDist))
				{
					if (fRightDist == 0 && fTopDist == 0 && fBottomDist == 0 && fLeftDist > 0 &&
						fLeftDist < 50 && (origin.x+fLeftDist+rect.width)<m_fFrameWidth)
					{
						origin.x += (fLeftDist + 0.1);
						continue;
					}
					else
					{
						bHasInters = TRUE;
						origin.y = (drawRectArr[k].topLeft.y - drawRectArr[k].height) - 0.1;
						//origin.y -= drawRectArr[k].height + 0.1;
						break;
					}
				}
			}
			if(!bHasInters)
			{
				rect.topLeft=origin;
				//向左上角靠紧 wht 19-07-25
				//CloseToToLeftTopCorner(rect);
				double widthTemp = rect.topLeft.x+ rect.width;
				if (startXArray.GetSize()==0)
					startXArray.Add(widthTemp);
				else if(widthTemp>startXArray[startXArray.GetSize()-1])
					startXArray.Add(widthTemp);
				else if (startXArray.GetSize()==1)
					startXArray.InsertAt(0,widthTemp);
				else if(fabs(widthTemp-startXArray[startXArray.GetSize()-1])>EPS)
				{
					int j=0;
					for (j=startXArray.GetSize()-1;j>0;j--)
					{
						if (widthTemp<startXArray[j]&&widthTemp>startXArray[j-1]&&j>=1)
						{
							startXArray.InsertAt(j,widthTemp);
							break;
						}
					}
					if (j==0)
						startXArray.InsertAt(0,widthTemp);
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL CDrawingRectLayout::Relayout(double hight,double width/*=10000000000000000*/)
{
	int nNum = drawRectArr.GetSize();
	if(nNum<=0)
		return FALSE;
	//将草图绘制矩形按照长/宽由大到小依次排序，以便下一步自动优化布置
	CHeapSort<CDrawingRect>::HeapSort(drawRectArr.GetData(),drawRectArr.GetSize(),compareDrawingRect);
	m_fFrameWidth=width;	//绘制边框初始宽
	m_fFrameHeight=hight;	//绘制边框初始高
	//对草图绘制矩形的布置方案进行优化(初始状态是按一列多行布置)
	m_arrRowHeight.RemoveAll();
	int i = 0;
	for(i=0;i<drawRectArr.GetSize();i++)
	{
		if(drawRectArr[i].m_pDrawing)
		{
			drawRectArr[i].m_bLayout=FALSE;
			drawRectArr[i].m_bException=FALSE;
		}
	}
	CLogErrorLife logLife;
	CArray<double,double&> startXArray;
	for(i=0;i<drawRectArr.GetSize();i++)
	{
		if(drawRectArr[i].height>hight)
		{	//钢板最小包络矩形宽度大于指定出图宽度
			drawRectArr[i].m_bException=TRUE;
			continue;
		}
		if(drawRectArr[i].m_bLayout)
			continue;
		int j=0;
		for(j=0;j<drawRectArr.GetSize();j++)
		{
			if(drawRectArr[j].m_bLayout)
				continue;
			if(GetBlankRectTopLeft(startXArray,drawRectArr[j]))
			{
				drawRectArr[j].m_bLayout=TRUE;
				for (int k = 0; k < m_arrSideLinePt.Count; k++)
				{
					//logerr.Log("%s %s")
				}
				UpdateSideLinePosX(drawRectArr[j]);
				break;
			}
		}
		if(j==drawRectArr.GetSize())
			break;	//空间已经不够用,回滚到上一布局方案
	}
	BOOL bLayout = FALSE;
	for (i = 0; i < drawRectArr.GetSize(); i++)
	{
		if (drawRectArr[i].m_bLayout)
			bLayout = TRUE;
	}
	return bLayout;
}