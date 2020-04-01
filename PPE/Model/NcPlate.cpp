#include "stdafx.h"
#include "NcPlate.h"
#include "LicFuncDef.h"

//////////////////////////////////////////////////////////////////////////
// CNCPlate
#ifdef __PNC_
CXhChar100 PointToString(const double* coord,bool bInsertSpace=false,bool bIsIJ=false)
{
	GEPOINT vertex(coord);
	CXhChar100 sCoord;
	const double COORD_EPS=0.002;
	CXhChar16 sCoordX("%.3f",vertex.x),sCoordY("%.3f",vertex.y);
	SimplifiedNumString(sCoordX);
	SimplifiedNumString(sCoordY);
	char cX='X',cY='Y';
	if(bIsIJ)
	{
		cX='I';
		cY='J';
	}
	if(fabs(vertex.x)<COORD_EPS)
		sCoord.Printf("%C%s",cY,(char*)sCoordY);
	else if(fabs(vertex.y)<EPS)
		sCoord.Printf("%C%s",cX,(char*)sCoordX);
	else
	{
		if(bInsertSpace)
			sCoord.Printf("%C%s %C%s",cX,(char*)sCoordX,cY,(char*)sCoordY);
		else
			sCoord.Printf("%C%s%C%s",cX,(char*)sCoordX,cY,(char*)sCoordY);
	}
	return sCoord;
}
GEPOINT CNCPlate::ProcessPoint(const double* coord,BYTE cCSMode/*=0*/)
{
	GEPOINT vertex(coord);
	const double COORD_EPS=0.002;
	if(fabs(vertex.x)<COORD_EPS)
		vertex.x=0;
	if(fabs(vertex.y)<COORD_EPS)
		vertex.y=0;
	if(fabs(vertex.z)<COORD_EPS)
		vertex.z=0;
	if(cCSMode==1)
	{
		double temp=vertex.x;
		vertex.x=-vertex.y;
		vertex.y=temp;
	}
	return vertex;
}
/*
 * pPlate:		当前切割钢板
 * iNo:			程序号
 * cutter_pos:	刀头位置
 * cCSMode:		0.横X+纵Y+ 1.横Y+纵X-
 * bClockwise：	TRUE 顺指针，FALSE 逆时针
 * nExtraInLen:	额外的引入长度
 * nExtraOutLen:额外的引出长度
 */
#ifdef PEC_EXPORTS
BOOL GetSysParaFromReg(const char* sEntry, char* sValue)
{
	char sStr[MAX_PATH];
	char sSubKey[MAX_PATH] = "Software\\Xerofox\\PPE\\Settings";
	DWORD dwDataType, dwLength = MAX_PATH;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS && hKey)
	{
		if (RegQueryValueEx(hKey, sEntry, NULL, &dwDataType, (BYTE*)&sStr[0], &dwLength) != ERROR_SUCCESS)
			return FALSE;
		RegCloseKey(hKey);
	}
	else
		return FALSE;
	if (sValue && strlen(sStr) > 0)
		strcpy(sValue, sStr);
	return TRUE;
}
#else
BOOL GetSysParaFromReg(const char* sEntry, char* sValue);	//From NcPart.cpp
#endif
CNCPlate::CNCPlate(CProcessPlate *pPlate,GEPOINT cutter_pos,int iNo/*=0*/,BYTE cCSMode/*=0*/,bool bClockwise/*=true*/,
				   int nInLineLen/*=-1*/,int nOutLineLen/*=-1*/,int nExtraInLen/*=0*/,int nExtraOutLen/*=0*/,int nEnlargedSpace/*=0*/,
				   BOOL bCutSpecialHole /*= FALSE*/)
{
	if(pPlate==NULL||!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	if(nInLineLen>=0)
		pPlate->m_xCutPt.cInLineLen=nInLineLen;
	if(nOutLineLen>=0)
		pPlate->m_xCutPt.cOutLineLen=nOutLineLen;
	BOOL bInvertedTraverse=FALSE;	//反向遍历
	if(bClockwise)
		bInvertedTraverse=TRUE;
	if(pPlate->mcsFlg.ciOverturn)
		bInvertedTraverse=!bInvertedTraverse;
	m_iNo=iNo;
	m_pPlate=pPlate;
	//1.将钢板转换至加工坐标系下
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	f3dPoint prevPt=cutter_pos,curPt;
	//2.初始化螺栓孔切入点
	for(CUT_POINT *pCutPt=pPlate->m_hashHoleCutPtByKey.GetFirst();pCutPt;pCutPt=pPlate->m_hashHoleCutPtByKey.GetNext())
	{

	}
	//计算圆孔切割点 wht 19-09-24
	//G0Y430.		//移动刀头至圆孔外侧切点
	//G41			//刀径左向补齐
	//M04           //主轴反转  
	//G3I - 35.		//绘制半径为35的整圆
	//M03           //主轴正转
	//G40           //取消刀径补齐
	double fSpecialD = 0;
	int bNeedSH=FALSE;
	CXhChar100 sValue;
	if (GetSysParaFromReg("LimitSH", sValue))
		fSpecialD = atof(sValue);
	if (GetSysParaFromReg("NeedSH", sValue))
		bNeedSH = atoi(sValue);
	//3.初始化钢板切入点
	DWORD nCount=pPlate->vertex_list.GetNodeNum();
	int iCurVertex=pPlate->m_xCutPt.hEntId;
	int iPrevVertex=(pPlate->m_xCutPt.hEntId-1)<=0?nCount:pPlate->m_xCutPt.hEntId-1;
	int iNextVertex=(pPlate->m_xCutPt.hEntId+1)>nCount?1:pPlate->m_xCutPt.hEntId+1;
	PROFILE_VER *pCurVertex=tempPlate.vertex_list.GetValue(iCurVertex);
	PROFILE_VER *pPrevVertex=tempPlate.vertex_list.GetValue(iPrevVertex);
	PROFILE_VER *pNextVertex=tempPlate.vertex_list.GetValue(iNextVertex);
	if(bInvertedTraverse)
	{
		PROFILE_VER *pTempVertex=pPrevVertex;
		pPrevVertex=pNextVertex;
		pNextVertex=pTempVertex;
	}
	PROFILE_VER *pFirstVertex=pCurVertex;
	if(pCurVertex==NULL||pPrevVertex==NULL||pNextVertex==NULL)
		return;
	GEPOINT prev_vec=pCurVertex->vertex-pPrevVertex->vertex;
	GEPOINT next_vec=pCurVertex->vertex-pNextVertex->vertex;
	normalize(prev_vec);
	normalize(next_vec);
	GEPOINT prev_norm(prev_vec.y,-prev_vec.x);	//顺时针旋转90度
	GEPOINT next_norm(-next_vec.y,next_vec.x);	//逆时针旋转90度
	if(bClockwise)
	{	//顺时针旋转需要翻转法线方向 wht 17-05-23
		prev_norm*=-1.0;
		next_norm*=-1.0;
	}
	if(nExtraInLen>0)
	{
		curPt=pCurVertex->vertex+next_vec*(pPlate->m_xCutPt.cInLineLen+nExtraInLen+nEnlargedSpace);
		m_cutPt.bHasExtraInVertex=TRUE;
		m_cutPt.extraInVertex=ProcessPoint(curPt-prevPt,cCSMode);
		prevPt=curPt;
	}
	curPt=pCurVertex->vertex+next_vec*(pPlate->m_xCutPt.cInLineLen+nEnlargedSpace);
	if(nEnlargedSpace>0)
		curPt+=(next_norm*nEnlargedSpace);
	m_cutPt.vertex=ProcessPoint(curPt-prevPt,cCSMode);
	prevPt=curPt;
	if (bCutSpecialHole)
	{	//在切入点之后，初始化切大孔 wht 19-10-22
		for (BOLT_INFO *pBoltInfo = tempPlate.m_xBoltInfoList.GetFirst(); pBoltInfo; pBoltInfo = tempPlate.m_xBoltInfoList.GetNext())
		{
			double hole_d = pBoltInfo->bolt_d + pBoltInfo->hole_d_increment;
			if (hole_d < fSpecialD)
				continue;
			CUT_PT *pCutPt = m_xCutHoleList.append();
			pCutPt->radius = hole_d * 0.5;
			pCutPt->cByte = CUT_PT::HOLE_CIR;
			pCutPt->bClockwise = FALSE;
			f3dPoint cir_center(pBoltInfo->posX, pBoltInfo->posY);
			curPt.Set(cir_center.x + pCutPt->radius, cir_center.y);
			pCutPt->vertex = ProcessPoint(curPt - prevPt, cCSMode);
			pCutPt->centerPt = ProcessPoint(cir_center - f3dPoint(prevPt), cCSMode);
			prevPt = curPt;
		}
	}
	//4.初始化轮廓点信息
	iCurVertex=0;
	int initIndex=pPlate->m_xCutPt.hEntId;
	f3dLine prev_line,next_line;
	while(initIndex>0)
	{
		BOOL bFinished=(iCurVertex==initIndex);
		BOOL bFirstVertex=(iCurVertex==0);
		if(iCurVertex==0)
			iCurVertex=initIndex;
		if(bInvertedTraverse)
		{
			iPrevVertex=(iCurVertex+1)>(int)nCount?1:iCurVertex+1;
			iNextVertex=(iCurVertex-1)==0?nCount:iCurVertex-1;
		}
		else
		{
			iPrevVertex=(iCurVertex-1)==0?nCount:iCurVertex-1;
			iNextVertex=(iCurVertex+1)>(int)nCount?1:iCurVertex+1;
		}
		pPrevVertex=tempPlate.vertex_list.GetValue(iPrevVertex);
		pCurVertex=tempPlate.vertex_list.GetValue(iCurVertex);
		pNextVertex=tempPlate.vertex_list.GetValue(iNextVertex);
		if(!bFirstVertex)
		{
			PROFILE_VER *pNextFeatureVertex=bInvertedTraverse?pNextVertex:pCurVertex;
			PROFILE_VER *pPrevFeatureVertex=bInvertedTraverse?pCurVertex:pPrevVertex;
			if(pNextFeatureVertex->type>1)
			{
				if(pPrevFeatureVertex->type>1)
				{
					f3dArcLine arcLine;
					PROFILE_VER *pOtherVertex=bInvertedTraverse?pCurVertex:pNextVertex;
					pNextFeatureVertex->RetrieveArcLine(arcLine,pOtherVertex->vertex,NULL);
					if( (bInvertedTraverse&&pNextFeatureVertex->work_norm.z>0)||
						(!bInvertedTraverse&&pNextFeatureVertex->work_norm.z<0))
						next_norm=f3dPoint(arcLine.Center())-pCurVertex->vertex;
					else
						next_norm=pCurVertex->vertex-f3dPoint(arcLine.Center());
					normalize(next_norm);
				}
				prev_norm=next_norm;	//后一线段为圆弧时next_norm与prev_norm一致
			}
			else
			{
				prev_norm=next_norm;
				GEPOINT next_vec=pCurVertex->vertex-pNextVertex->vertex;
				normalize(next_vec);
				next_norm.Set(-next_vec.y,next_vec.x);
				if(bClockwise)
					next_norm*=-1.0;
				if(pPrevFeatureVertex->type>1)
					prev_norm=next_norm;	//前一线段为圆弧时next_norm与prev_norm一致
			}
		}
		PROFILE_VER *pFeatureVertex=bInvertedTraverse?pCurVertex:pPrevVertex;
		PROFILE_VER *pOtherVertex=bInvertedTraverse?pPrevVertex:pCurVertex;
		curPt=pCurVertex->vertex;
		if(nEnlargedSpace>0)
		{
			if(bFirstVertex||bFinished)
				curPt+=next_vec*nEnlargedSpace+next_norm*nEnlargedSpace;
			else
				curPt+=prev_norm*nEnlargedSpace;
		}
		if(pFeatureVertex->type==1||bFirstVertex)
		{	//直线
			CUT_PT *pPt=m_xCutPtList.append();
			pPt->cByte=CUT_PT::EDGE_LINE;
			pPt->vertex=ProcessPoint(curPt-prevPt,cCSMode);
			prev_line.startPt=prevPt;	//当前节点之前的一条轮廓标
			prev_line.endPt=curPt;
			f3dPoint oldPrevPt=prevPt;
			prevPt=curPt;
			if(!bFirstVertex&&!bFinished&&nEnlargedSpace>0)
			{
				curPt=pCurVertex->vertex+next_norm*nEnlargedSpace;
				next_line.startPt=curPt;	//当前节点所在的下一条轮廓边
				next_line.endPt=pNextVertex->vertex+next_norm*nEnlargedSpace;
				f3dPoint inters_pt;
				int nRetCode=Int3dll(prev_line,next_line,inters_pt);
				if(nRetCode==1||nRetCode==2)
				{	//交叉点为线段端点或内侧点(处理凹点或共线点) wht-17.06.08
					pPt->vertex=ProcessPoint(inters_pt-oldPrevPt,cCSMode);
					prevPt=inters_pt;
				}
				else
				{
					f3dArcLine arcLine;
					PROFILE_VER featureVertex;
					featureVertex.type=4;
					featureVertex.radius=nEnlargedSpace;	//此处弧均为逆时针弧 wht 17-05-23
					featureVertex.vertex=prevPt;
					featureVertex.center=pCurVertex->vertex;
					featureVertex.RetrieveArcLine(arcLine,curPt,NULL);
					if(prev_norm!=next_norm)
					{
						pPt=m_xCutPtList.append();
						pPt->vertex=ProcessPoint(curPt-prevPt,cCSMode);
						if(fabs(arcLine.SectorAngle())>0.3*Pi)
						{
							pPt->cByte=CUT_PT::EDGE_ARC;
							pPt->bClockwise=FALSE;
							pPt->centerPt=ProcessPoint(pCurVertex->vertex-prevPt,cCSMode);
							pPt->fSectorAngle=arcLine.SectorAngle();
						}
					}
					prevPt=curPt;
				}
			}
		}
		else if(pFeatureVertex->type==2)
		{	//圆弧
			f3dArcLine arcLine;
			pFeatureVertex->RetrieveArcLine(arcLine,pOtherVertex->vertex,NULL);
			if(!bFirstVertex&&!bFinished&&nEnlargedSpace>0)
			{
				PROFILE_VER featureVertex;
				featureVertex.type=4;
				featureVertex.radius=arcLine.Radius()+nEnlargedSpace;
				featureVertex.center=arcLine.Center();
				if( (bInvertedTraverse&&pFeatureVertex->work_norm.z>0)||
					(!bInvertedTraverse&&pFeatureVertex->work_norm.z<0))
					featureVertex.radius*=-1.0;
				if( (!bInvertedTraverse&&pFeatureVertex==pPrevVertex)||
					(bInvertedTraverse&&pFeatureVertex==pCurVertex))
				{
					featureVertex.vertex=prevPt;
					featureVertex.RetrieveArcLine(arcLine,curPt,NULL);
				}
				else
				{
					featureVertex.vertex=curPt;
					featureVertex.RetrieveArcLine(arcLine,prevPt,NULL);
				}
			}
			CUT_PT *pPt=m_xCutPtList.append();
			pPt->cByte=CUT_PT::EDGE_ARC;
			pPt->vertex=ProcessPoint(curPt-prevPt,cCSMode);
			pPt->centerPt=ProcessPoint(arcLine.Center()-GEPOINT(prevPt),cCSMode);
			pPt->fSectorAngle=arcLine.SectorAngle();
			if( (bInvertedTraverse&&pFeatureVertex->work_norm.z>0)||
				(!bInvertedTraverse&&pFeatureVertex->work_norm.z<0))
				pPt->fSectorAngle*=-1;
			pPt->bClockwise=pPt->fSectorAngle<0;	//顺时针圆弧
			prevPt=curPt;
		}
		else if(pFeatureVertex->type==3)
		{	//椭圆弧
			f3dArcLine arcLine;
			pFeatureVertex->RetrieveArcLine(arcLine,pOtherVertex->vertex,NULL);
			if((!bFirstVertex&&!bFinished&&nEnlargedSpace>0)||bInvertedTraverse)
			{
				PROFILE_VER featureVertex;
				featureVertex.type=3;
				featureVertex.radius=arcLine.Radius()+nEnlargedSpace;
				featureVertex.center=arcLine.Center();
				if(bInvertedTraverse)
					featureVertex.column_norm=arcLine.ColumnNorm()*-1.0;
				else
					featureVertex.column_norm=arcLine.ColumnNorm();
				if( (bInvertedTraverse&&pFeatureVertex->work_norm.z>0)||
					(!bInvertedTraverse&&pFeatureVertex->work_norm.z<0))
					featureVertex.radius*=-1.0;
				if( (!bInvertedTraverse&&pFeatureVertex==pPrevVertex)||
					(bInvertedTraverse&&pFeatureVertex==pCurVertex))
				{
					featureVertex.vertex=prevPt;
					featureVertex.RetrieveArcLine(arcLine,curPt,NULL);
				}
				else
				{
					featureVertex.vertex=curPt;
					featureVertex.RetrieveArcLine(arcLine,prevPt,NULL);
				}
			}
			int nSlices= CalArcResolution(arcLine.Radius(),arcLine.SectorAngle(),1.0,3.0,10);
			double slice_angle = arcLine.SectorAngle()/nSlices;
			prevPt=pPrevVertex->vertex;
			ATOM_LIST<f3dPoint> ptList;
			for(int i=1;i<=nSlices;i++)
			{
				f3dPoint pt=arcLine.PositionInAngle(i*slice_angle);
				ptList.append(pt);
			}
			int nPtCount=ptList.GetNodeNum();
			for(int i=0;i<nPtCount;i++)
			{
				curPt=ptList[i];
				//直线
				CUT_PT *pPt=m_xCutPtList.append();
				pPt->cByte=CUT_PT::EDGE_LINE;
				pPt->vertex=ProcessPoint(curPt-prevPt,cCSMode);
				prev_line.startPt=prevPt;	//当前节点之前的一条轮廓标
				prev_line.endPt=curPt;
				f3dPoint oldPrevPt=prevPt;
				prevPt=curPt;
				if(!bFirstVertex&&!bFinished&&nEnlargedSpace>0)
				{
					curPt=pCurVertex->vertex+next_norm*nEnlargedSpace;
					next_line.startPt=curPt;	//当前节点所在的下一条轮廓边
					next_line.endPt=pNextVertex->vertex+next_norm*nEnlargedSpace;
					f3dPoint inters_pt;
					int nRetCode=Int3dll(prev_line,next_line,inters_pt);
					if(nRetCode==1||nRetCode==2)
					{	//交叉点为线段端点或内侧点(处理凹点或共线点) wht-17.06.08
						pPt->vertex=ProcessPoint(inters_pt-oldPrevPt,cCSMode);
						prevPt=inters_pt;
					}
					else
					{
						f3dArcLine arcLine;
						PROFILE_VER featureVertex;
						featureVertex.type=4;
						featureVertex.radius=nEnlargedSpace;	//此处弧均为逆时针弧 wht 17-05-23
						featureVertex.vertex=prevPt;
						featureVertex.center=pCurVertex->vertex;
						featureVertex.RetrieveArcLine(arcLine,curPt,NULL);
						if(prev_norm!=next_norm)
						{
							pPt=m_xCutPtList.append();
							pPt->vertex=ProcessPoint(curPt-prevPt,cCSMode);
							if(fabs(arcLine.SectorAngle())>0.3*Pi)
							{
								pPt->cByte=CUT_PT::EDGE_ARC;
								pPt->bClockwise=FALSE;
								pPt->centerPt=ProcessPoint(pCurVertex->vertex-prevPt,cCSMode);
								pPt->fSectorAngle=arcLine.SectorAngle();
							}
						}
						prevPt=curPt;
					}
				}
			}
		}
		if(bFinished)
			break;
		if(bInvertedTraverse)
			iCurVertex=(iCurVertex-1)==0?nCount:iCurVertex-1;
		else
			iCurVertex=(iCurVertex+1)>(int)nCount?1:iCurVertex+1;
	}
	curPt+=prev_vec*pPlate->m_xCutPt.cOutLineLen;
	m_cutPt.vertex2=ProcessPoint(curPt-prevPt,cCSMode);
	prevPt=curPt;
	if(nExtraOutLen>0)
	{
		m_cutPt.bHasExtraOutVertex=TRUE;
		curPt=pFirstVertex->vertex+prev_vec*(pPlate->m_xCutPt.cOutLineLen+nExtraOutLen);
		m_cutPt.extraOutVertex=ProcessPoint(curPt-prevPt,cCSMode);
		prevPt=curPt;
	}
	//
	m_cutPt.vertex3=ProcessPoint(f3dPoint(cutter_pos)-prevPt,cCSMode);
}
bool CNCPlate::CreatePlateTxtFile(const char* file_path)
{	//写入文件
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return false;
	FILE* fp=fopen(file_path,"wt");
	if(fp==NULL)
		return false;
	fprintf(fp,"G21\n");			//公制(单位mm)
	fprintf(fp,"G91\n");			//增量尺寸	G90绝对尺寸
	//输出圆孔切割点 wht 19-09-24
	//G0Y430.		//移动刀头至圆孔外侧切点
	//G41			//刀径左向补齐
	//M04           //主轴反转  
	//G3I - 35.		//绘制半径为35的整圆
	//M03           //主轴正转
	//G40           //取消刀径补齐
	for (CUT_PT *pCutPt = m_xCutHoleList.GetFirst(); pCutPt; pCutPt = m_xCutHoleList.GetNext())
	{
		fprintf(fp, "G00 %s\n", (char*)PointToString(pCutPt->vertex, true));//移动刀头至圆孔外侧切点
		fprintf(fp, "G41\n");						//刀径左向补齐
		fprintf(fp, "M04\n");						//主轴反转  
		fprintf(fp, "G03 I-%.1f\n",pCutPt->radius);	//绘制半径为35的整圆,因切入点在右侧，圆心在左侧
		fprintf(fp, "M03\n");						//主轴正转
		fprintf(fp, "G40\n");						//取消刀径补齐
	}
	if(m_cutPt.bHasExtraInVertex)
		fprintf(fp,"G00 %s\n",(char*)PointToString(m_cutPt.extraInVertex,true));	//额外的引入点
	else
		fprintf(fp,"G00 %s\n",(char*)PointToString(m_cutPt.vertex,true));			//引入点
	fprintf(fp,"G42\n");			//G42左割缝补偿 G41右割缝补偿
	fprintf(fp,"M07\n");			//开始切割
	if(m_cutPt.bHasExtraInVertex)
		fprintf(fp,"G01 %s\n",(char*)PointToString(m_cutPt.vertex,true));
	for(CUT_PT *pPt=m_xCutPtList.GetFirst();pPt;pPt=m_xCutPtList.GetNext())
	{
		if(pPt->cByte==CUT_PT::EDGE_LINE)
			fprintf(fp,"G01 %s\n",(char*)PointToString(pPt->vertex,true));
		else if(pPt->cByte==CUT_PT::EDGE_ARC)
			fprintf(fp,"G0%d %s %s\n",pPt->bClockwise?2:3,(char*)PointToString(pPt->vertex,true),(char*)PointToString(pPt->centerPt,true,true));
		else if(pPt->cByte==CUT_PT::HOLE_CUT_IN)
		{

		}
		else if(pPt->cByte==CUT_PT::HOLE_CUT_OUT)
		{

		}
	}
	fprintf(fp,"G01 %s\n",(char*)PointToString(m_cutPt.vertex2,true));
	if(m_cutPt.bHasExtraOutVertex)
		fprintf(fp,"G01 %s\n",(char*)PointToString(m_cutPt.extraOutVertex,true));
	fprintf(fp,"M08\n");			//停止切割
	fprintf(fp,"G40\n");			//关闭补偿
	fprintf(fp,"G00 %s\n",(char*)PointToString(m_cutPt.vertex3,true));
	fprintf(fp,"M02\n");			//停止+返回起点
	fclose(fp);
	return true;
}
bool CNCPlate::CreatePlateNcFile(const char* file_path)
{	//写入文件
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return false;
	FILE* fp=fopen(file_path,"wt");
	if(fp==NULL)
		return false;
	fprintf(fp,"O%d\n",m_iNo);		//程序号
	fprintf(fp,"G86\n");			//AD指令
	fprintf(fp,"G54\n");			//0切割指令
	fprintf(fp,"G65P9303A%.1fB0C0D4E3\n",m_pPlate->GetThick());	//钢板信息 A厚度
	fprintf(fp,"G65P9013A130000B40000\n");	//坐标旋转
	fprintf(fp,"M31\n");			//角度原点复归
	fprintf(fp,"M27\n");			//旋转原点复归
	fprintf(fp,"G91\n");			//增量指令
	fprintf(fp,"G92X0.Y0.C0.A0\n");	//坐标系设定
	fprintf(fp,"M7000\n");			//图形跟随点
	if(m_cutPt.bHasExtraInVertex)
		fprintf(fp,"G00%s\n",(char*)PointToString(m_cutPt.extraInVertex));	//额外的引入点
	else
		fprintf(fp,"G00%s\n",(char*)PointToString(m_cutPt.vertex));			//快速移动至引入点
	fprintf(fp,"N1\n");				//第一个穿孔点
	fprintf(fp,"M54\n");			//0-CUT指令
	fprintf(fp,"M45\n");			//初始高度设定
	fprintf(fp,"G01C180.F21000\n");	//旋转轴180度，F速度
	fprintf(fp,"G01A12.5.F21000\n");//角度，F速度
	fprintf(fp,"G41G01Y-0.1F4000D12\n");//G41补偿 D补偿代码
	fprintf(fp,"G42.1G01Y-0.9\n");		//G42.1法线
	fprintf(fp,"M17\n");			//穿孔开始
	if(m_cutPt.bHasExtraInVertex)
		fprintf(fp,"G01%s\n",(char*)PointToString(m_cutPt.vertex));
	for(CUT_PT *pPt=m_xCutPtList.GetFirst();pPt;pPt=m_xCutPtList.GetNext())
	{
		if(pPt->cByte==CUT_PT::EDGE_LINE)
			fprintf(fp,"G01%s\n",(char*)PointToString(pPt->vertex));
		else if(pPt->cByte==CUT_PT::EDGE_ARC)
			fprintf(fp,"G0%d%s%s\n",pPt->bClockwise?2:3,(char*)PointToString(pPt->vertex),(char*)PointToString(pPt->centerPt,false,true));
		else if(pPt->cByte==CUT_PT::HOLE_CUT_IN)
		{

		}
		else if(pPt->cByte==CUT_PT::HOLE_CUT_OUT)
		{

		}
	}
	fprintf(fp,"G01%s\n",(char*)PointToString(m_cutPt.vertex2));
	if(m_cutPt.bHasExtraOutVertex)
		fprintf(fp,"G40.1G40G01%sM16\n",(char*)PointToString(m_cutPt.extraOutVertex));//等离子关
	else
		fprintf(fp,"G40.1G40M16\n");//等离子关
	fprintf(fp,"G01A4.F6000\n");
	fprintf(fp,"M88\n");			//中间上升
	fprintf(fp,"G01%s\n",(char*)PointToString(m_cutPt.vertex3));
	fprintf(fp,"M31\n");
	fprintf(fp,"M27\n");
	fprintf(fp,"M14\n");			//蜂鸣器
	fprintf(fp,"N9999\n");			//简易程序
	fprintf(fp,"M02\n");			//程序结束
	fclose(fp);
	return true;
}
bool CNCPlate::InitVertextListByNcFile(CProcessPlate *pPlate,const char* file_path)
{
	if(file_path==NULL || strlen(file_path)==0)
		return false;
	FILE *fp =fopen(file_path,"rt");
	if(fp==NULL)
		return false;
	char line_txt[200],bak_line_txt[200];
	PROFILE_VER *pPrevVertex=NULL;
	while(!feof(fp))
	{
		if(fgets(line_txt,200,fp)==NULL)
			break;
		line_txt[strlen(line_txt)-1]='\0';
		strcpy(bak_line_txt,line_txt);
		char szTokens[] = " =\n" ;
		char* szToken = strtok(line_txt, szTokens) ; 
		bool bG02=(stricmp(szToken,"G02")==0);
		bool bG03=(stricmp(szToken,"G03")==0);
		if(szToken==NULL||(stricmp(szToken,"G00")!=0&&stricmp(szToken,"G01")!=0&&!bG02&&!bG03))
			continue;
		PROFILE_VER *pVertex=pPlate->vertex_list.Add(0);
		pVertex->type=1;
		double x=0,y=0,i=0,j=0;
		while(szToken)
		{
			if(szToken[0]=='X')
				x=atof((char*)(szToken+1));
			else if(szToken[0]=='Y')
				y=atof((char*)(szToken+1));
			else if(szToken[0]=='I')
				i=atof((char*)(szToken+1));
			else if(szToken[0]=='J')
				j=atof((char*)(szToken+1));
			szToken=strtok(NULL,szTokens);
		}
		if(pPrevVertex)
		{
			x+=pPrevVertex->vertex.x;
			y+=pPrevVertex->vertex.y;
			i+=pPrevVertex->vertex.x;
			j+=pPrevVertex->vertex.y;
		}
		pVertex->vertex.x=x;
		pVertex->vertex.y=y;
		if(bG02||bG03)
		{
			pPrevVertex->type=4;
			pPrevVertex->center.Set(i,j);
			pPrevVertex->radius=DISTANCE(pVertex->vertex,pPrevVertex->center);
			pPrevVertex->work_norm.Set(0,0,1);
			if(bG02)	//顺时针方向圆弧
				pPrevVertex->radius*=-1;
		}
		pPrevVertex=pVertex;
	}
	fclose(fp);

	f3dArcLine arcLine;
	pPrevVertex=pPlate->vertex_list.GetTail();
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		if(pPrevVertex->type==4)
		{	//统一将圆弧定义方式修订为type=2模式
			pPrevVertex->RetrieveArcLine(arcLine,pVertex->vertex,NULL);
			pPrevVertex->type=2;
			pPrevVertex->work_norm=arcLine.WorkNorm();
			pPrevVertex->sector_angle=arcLine.SectorAngle();
		}
		pPrevVertex=pVertex;
	}
	return true;
}
#endif