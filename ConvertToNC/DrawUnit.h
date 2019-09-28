#pragma once
#ifndef __DRAW_UNIT_H_
#define __DRAW_UNIT_H_

#include "CadToolFunc.h"
//////////////////////////////////////////////////////////////////////////
//排版布局
class CDrawingRect
{
public:	
	BOOL m_bException;		//异常板
	BOOL m_bLayout;			//内部优化布局时用于标识是否已经布置位置
	f3dPoint topLeft;		//包容矩形重新布局后的左上角坐标位置
	f3dPoint initTopLeft;	//在重新布局前视图包容矩形的初始左上角坐标，用于与重新布局后topLeft最终值一起确定包容矩形的偏移量
	double width,height;	//草图绘制矩形的宽度和高度
	ARRAY_LIST<f3dPoint> m_vertexArr;
	long feature;
	void *m_pDrawing;
	CDrawingRect() { m_pDrawing = NULL; m_bLayout = FALSE; feature = 0; width = height = 0; }
	BOOL GetPolygon(POLYGON &rgn);
	CDrawingRect(const CDrawingRect &srcRect);
	CDrawingRect& operator=(const CDrawingRect &srcRect);
	void Clone(const CDrawingRect &srcRect);
};
class CDrawingRectLayout
{
	double m_fFrameWidth;	//边框宽度
	double m_fFrameHeight;	//边框高度
	CArray<double,double&>m_arrRowHeight;	//
	ARRAY_LIST<GEPOINT> m_arrSideLinePt;	//已布置区域边界线 wht 19-07-27
	void UpdateSideLinePosX(CDrawingRect &rect);
public:
	CArray<CDrawingRect,CDrawingRect&> drawRectArr;
public:
	CDrawingRectLayout();
	//
	BOOL CloseToToLeftTopCorner(CDrawingRect &rect);
	BOOL GetBlankRectTopLeft(CArray<double,double&>&startXArray,CDrawingRect &rect);
	BOOL Relayout(double hight=1000,double width=10000000000000000);
};
#endif