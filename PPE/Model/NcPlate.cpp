#include "stdafx.h"
#include "NcPlate.h"
#include "LicFuncDef.h"

#define	 COORD_EPS	0.002
//////////////////////////////////////////////////////////////////////////
// CNCPlate
#ifdef __PNC_
CXhChar100 PointToString(const double* coord,bool bInsertSpace=false,bool bIsIJ=false)
{
	GEPOINT vertex(coord);
	CXhChar100 sCoord;
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
//////////////////////////////////////////////////////////////////////////
//CNCPlate
CNCPlate::CNCPlate(CProcessPlate *pPlate,int iNo/*=0*/)
{
	m_pPlate = pPlate;
	m_iNo = iNo;
	m_cCSMode = 0;
	m_bClockwise = FALSE;
	m_ciStartId = pPlate ? (BYTE)pPlate->m_xCutPt.hEntId : 1;
	m_nInLineLen = pPlate ? pPlate->m_xCutPt.cInLineLen : 4;
	m_nOutLineLen = pPlate ? pPlate->m_xCutPt.cOutLineLen : 4;
	m_nExtraInLen = m_nExtraOutLen = 0;
	m_nEnlargedSpace = 0;
	m_bCutSpecialHole = FALSE;
	m_bCutFullCircle = FALSE;
	m_bGrindingArc = FALSE;
}
//
GEPOINT CNCPlate::ProcessPoint(const double* coord)
{
	GEPOINT vertex(coord);
	if (fabs(vertex.x) < COORD_EPS)
		vertex.x = 0;
	if (fabs(vertex.y) < COORD_EPS)
		vertex.y = 0;
	if (fabs(vertex.z) < COORD_EPS)
		vertex.z = 0;
	if (m_cCSMode == 1)
	{
		double temp = vertex.x;
		vertex.x = -vertex.y;
		vertex.y = temp;
	}
	return vertex;
}
void CNCPlate::AppendCutPt(GEPOINT& prePt, GEPOINT curPt)
{
	CUT_PT *pPt = m_xCutEdge.cutPtArr.append();
	pPt->cByte = CUT_PT::EDGE_LINE;
	pPt->vertex = ProcessPoint(curPt - prePt);
	//
	prePt = curPt;
}
void CNCPlate::AppendCutPt(GEPOINT& prePt, GEPOINT curPt, GEPOINT center,GEPOINT norm)
{
	CUT_PT *pPt = m_xCutEdge.cutPtArr.append();
	pPt->cByte = CUT_PT::EDGE_ARC;
	pPt->vertex = ProcessPoint(curPt - prePt);
	pPt->center = ProcessPoint(center - prePt);
	pPt->bClockwise = (norm.z < 0) ? TRUE : FALSE;	//˳ʱ��Բ��
	//
	prePt = curPt;
}
//
GECS CNCPlate::GetMCS(CProcessPlate& tempPlate, double fMinDistance, BOOL bGrindingArc)
{
	GECS mcs;
	tempPlate.GetMCS(mcs);
	//����Ⱦ�����������
	ATOM_LIST<PROFILE_VER> xDestList;
	tempPlate.CalEquidistantShape(fMinDistance, &xDestList);
	tempPlate.vertex_list.Empty();
	//���¸ְ�������
	f3dArcLine arcLine;
	int nNum = xDestList.GetNodeNum();
	for (int i = 0; i < nNum; i++)
	{
		PROFILE_VER* pPrevVer = xDestList.GetByIndex((i - 1 + nNum) % nNum);
		PROFILE_VER* pCurrVer = xDestList.GetByIndex(i);
		PROFILE_VER* pNextVer = xDestList.GetByIndex((i + 1) % nNum);
		if (pPrevVer->type == 1 && pCurrVer->type == 1)
		{
			GEPOINT prev_vec = (pCurrVer->vertex - pPrevVer->vertex).normalized();
			GEPOINT next_vec = (pCurrVer->vertex - pNextVer->vertex).normalized();
			GEPOINT verify_vec = next_vec ^ prev_vec;
			double angle = cal_angle_of_2vec(prev_vec, next_vec);
			if (fabs(prev_vec*next_vec) > 0.9999)	//���㹲��
				tempPlate.vertex_list.Append(*pCurrVer);
			else if (verify_vec.z < 0)				//����
				tempPlate.vertex_list.Append(*pCurrVer);
			else if (angle < 0.3*Pi)				//С�Ƕ�
				tempPlate.vertex_list.Append(*pCurrVer);
			else if (!bGrindingArc)				//����ĥ
				tempPlate.vertex_list.Append(*pCurrVer);
			else
			{	//
				GEPOINT norm1(prev_vec.y, -prev_vec.x);	//˳ʱ����ת90��
				GEPOINT norm2(-next_vec.y, next_vec.x);	//��ʱ����ת90��
				double offset = 2;
				if (fMinDistance > 0 && tan(angle / 2)>0)
					offset = fMinDistance / tan(angle / 2);
				GEPOINT ptS = pCurrVer->vertex - prev_vec * offset;
				GEPOINT ptE = pCurrVer->vertex - next_vec * offset;
				GEPOINT center;
				int iRet = Int3dll(ptS - norm1 * 100, ptS + norm1 * 100, ptE - norm2 * 100, ptE + norm2 * 100, center);
				if (iRet == 1)
				{
					double fR = DISTANCE(ptS, center);
					arcLine.CreateMethod3(ptS, ptE, f3dPoint(0, 0, 1), fR, center);
					PROFILE_VER temVertex;
					temVertex.type = 2;
					temVertex.vertex = ptS;
					temVertex.work_norm = arcLine.WorkNorm();
					temVertex.sector_angle = arcLine.SectorAngle();
					temVertex.radius = arcLine.Radius();
					tempPlate.vertex_list.Append(temVertex);
					//
					temVertex.type = 1;
					temVertex.vertex = ptE;
					tempPlate.vertex_list.Append(temVertex);
				}
				else
					tempPlate.vertex_list.Append(*pCurrVer);
			}
		}
		else if (pCurrVer->type == 3)
		{
			arcLine.CreateEllipse(pCurrVer->center, pCurrVer->vertex, pNextVer->vertex,
				pCurrVer->column_norm, pCurrVer->work_norm, pCurrVer->radius);
			double sample_len = 3;
			int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, sample_len, 10);
			double slice_angle = arcLine.SectorAngle() / nSlices;
			PROFILE_VER temVertex;
			for (int j = 0; j < nSlices; j++)
			{
				temVertex.vertex = arcLine.PositionInAngle(j*slice_angle);
				tempPlate.vertex_list.Append(temVertex);
			}
		}
		else
			tempPlate.vertex_list.Append(*pCurrVer);
	}
	//��������ϵԭ��
	SCOPE_STRU scope = tempPlate.GetVertexsScope(&mcs);
	mcs.origin += scope.fMinX*mcs.axis_x;
	mcs.origin += scope.fMinY*mcs.axis_y;
	return mcs;
}
//
void CNCPlate::InitCutHoleInfo(GEPOINT& prevPt, CProcessPlate& tempPlate)
{
	if (!m_bCutSpecialHole)
		return;
	CXhChar100 sValue;
	double fSpecialD = (GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
	GEPOINT curPt;
	for (BOLT_INFO *pBoltInfo = tempPlate.m_xBoltInfoList.GetFirst(); pBoltInfo; pBoltInfo = tempPlate.m_xBoltInfoList.GetNext())
	{
		double hole_d = pBoltInfo->bolt_d + pBoltInfo->hole_d_increment;
		if (pBoltInfo->cFuncType == 0)
			continue;
		if (fSpecialD > 0 && hole_d < fSpecialD)
			continue;
		double dfHoleR = hole_d * 0.5;
		CUT_HOLE_PATH* pCutHole = m_xCutHole.append();
		pCutHole->fHoleR = dfHoleR;
		pCutHole->orgPt.Set(pBoltInfo->posX, pBoltInfo->posY, 0);
		//������λ��
		curPt.Set(pBoltInfo->posX - dfHoleR + m_nInLineLen, pBoltInfo->posY);
		pCutHole->ignitionPt = ProcessPoint(curPt - prevPt);
		prevPt = curPt;
		//�����и�·��
		if (m_bCutFullCircle)
		{	//����Բ
			curPt.Set(pBoltInfo->posX - dfHoleR, pBoltInfo->posY, 0);
			CUT_PT *pCutPt = pCutHole->cutPtArr.append();
			pCutPt->cByte = CUT_PT::HOLE_CIR;
			pCutPt->bClockwise = FALSE;
			pCutPt->radius = pCutHole->fHoleR;
			pCutPt->vertex = ProcessPoint(curPt - prevPt);
			pCutPt->center = ProcessPoint(pCutHole->orgPt - prevPt);
			prevPt = curPt;
		}
		else
		{	//�ֶ���
			int slices = 8;
			double fSliceAngle = 2 * Pi / slices;
			//��ʼ�㣨ֱ�����룩
			curPt.Set(pBoltInfo->posX - dfHoleR, pBoltInfo->posY, 0);
			CUT_PT *pCutPt = pCutHole->cutPtArr.append();
			pCutPt->cByte = CUT_PT::EDGE_LINE;
			pCutPt->vertex = ProcessPoint(curPt - prevPt);
			prevPt = curPt;
			//Բ���ϵ��и��
			for (int ii = 1; ii <= slices; ii++)
			{	
				curPt = prevPt;
				rotate_point_around_axis(curPt, fSliceAngle, pCutHole->orgPt, f3dPoint(pBoltInfo->posX, pBoltInfo->posY, 1));
				CUT_PT *pCutPt = pCutHole->cutPtArr.append();
				pCutPt->cByte = CUT_PT::EDGE_ARC;
				pCutPt->bClockwise = FALSE;
				pCutPt->vertex = ProcessPoint(curPt - prevPt);
				pCutPt->center = ProcessPoint(pCutHole->orgPt - prevPt);
				prevPt = curPt;
			}
			//���������㣨Բ��������
			curPt.Set(pBoltInfo->posX - dfHoleR + m_nOutLineLen, pBoltInfo->posY - m_nOutLineLen);
			f3dArcLine arc_line;
			if (arc_line.CreateMethod3(prevPt, curPt, f3dPoint(0, 0, 1), m_nOutLineLen, GEPOINT(curPt.x, pBoltInfo->posY, 0)))
			{
				CUT_PT *pCutPt = pCutHole->cutPtArr.append();
				pCutPt->cByte = CUT_PT::EDGE_ARC;
				pCutPt->vertex = ProcessPoint(curPt - prevPt);
				pCutPt->center = ProcessPoint(arc_line.Center() - prevPt);
				prevPt = curPt;
			}
		}
	}
}
void CNCPlate::InitCutEdgePtInfo(GEPOINT& prevPt, CProcessPlate& tempPlate)
{
	BOOL bInvertedTraverse = m_bClockwise ? TRUE : FALSE;
	if (m_pPlate->mcsFlg.ciOverturn)
		bInvertedTraverse = !bInvertedTraverse;
	//
	int nCount = m_pPlate->vertex_list.GetNodeNum();
	int iCurVertex = m_ciStartId;
	int iPrevVertex = (m_ciStartId - 1) <= 0 ? nCount : m_ciStartId - 1;
	int iNextVertex = (m_ciStartId + 1) > nCount ? 1 : m_ciStartId + 1;
	PROFILE_VER *pCurVertex = tempPlate.vertex_list.GetValue(iCurVertex);
	PROFILE_VER *pPrevVertex = tempPlate.vertex_list.GetValue(iPrevVertex);
	PROFILE_VER *pNextVertex = tempPlate.vertex_list.GetValue(iNextVertex);
	if (pCurVertex == NULL || pPrevVertex == NULL || pNextVertex == NULL)
		return;
	if (bInvertedTraverse)
	{
		PROFILE_VER *pTempVertex = pPrevVertex;
		pPrevVertex = pNextVertex;
		pNextVertex = pTempVertex;
	}
	GEPOINT prev_vec = (pCurVertex->vertex - pPrevVertex->vertex).normalized();
	GEPOINT next_vec = (pCurVertex->vertex - pNextVertex->vertex).normalized();
	GEPOINT prev_norm(prev_vec.y, -prev_vec.x);	//˳ʱ����ת90��
	GEPOINT next_norm(-next_vec.y, next_vec.x);	//��ʱ����ת90��
	if (m_bClockwise)
	{	//˳ʱ����ת��Ҫ��ת���߷��� wht 17-05-23
		prev_norm *= -1.0;
		next_norm *= -1.0;
	}
	//3.1 �������������
	//3.1.1 �ȼ����һ������������
	GEPOINT inters_pt, curPt;
	f3dLine prev_line, next_line;
	curPt = pCurVertex->vertex;
	if (m_nEnlargedSpace > 0)	//��������������ֵʱ����ǰ�������ط��߷���ƫ������ֵ
	{
		GEPOINT prev_perp = curPt + (prev_norm*m_nEnlargedSpace);
		GEPOINT next_perp = curPt + (next_norm*m_nEnlargedSpace);
		prev_line.startPt = pPrevVertex->vertex + prev_norm * m_nEnlargedSpace;
		prev_line.endPt = pCurVertex->vertex + prev_norm * m_nEnlargedSpace;
		next_line.startPt = pCurVertex->vertex + next_norm * m_nEnlargedSpace;
		next_line.endPt = pNextVertex->vertex + next_norm * m_nEnlargedSpace;
		int nRetCode = Int3dpl(prev_line, next_line, inters_pt);
		if (nRetCode == 1)
			curPt = inters_pt;
		else
			curPt = (prev_perp + next_perp)*0.5;	//ȡ�ױ��е�
	}
	GEPOINT firstVertex = curPt;
	//3.1.2 �������������Ϊ��׼�������������
	if (m_nExtraInLen > 0)
	{
		curPt = firstVertex + next_vec * (m_nInLineLen + m_nExtraInLen);
		m_xCutEdge.extraInVertex = ProcessPoint(curPt - prevPt);
		prevPt = curPt;
	}
	//3.1.3 �����
	curPt = firstVertex + next_vec * m_nInLineLen;
	m_xCutEdge.cutInVertex = ProcessPoint(curPt - prevPt);
	prevPt = curPt;
	//
	iCurVertex = 0;
	int initIndex = m_ciStartId;
	while (initIndex > 0)
	{
		BOOL bFinished = (iCurVertex == initIndex);
		BOOL bFirstVertex = (iCurVertex == 0);
		if (iCurVertex == 0)
			iCurVertex = initIndex;
		if (bInvertedTraverse)
		{
			iPrevVertex = (iCurVertex + 1) > (int)nCount ? 1 : iCurVertex + 1;
			iNextVertex = (iCurVertex - 1) == 0 ? nCount : iCurVertex - 1;
		}
		else
		{
			iPrevVertex = (iCurVertex - 1) == 0 ? nCount : iCurVertex - 1;
			iNextVertex = (iCurVertex + 1) > (int)nCount ? 1 : iCurVertex + 1;
		}
		pPrevVertex = tempPlate.vertex_list.GetValue(iPrevVertex);
		pCurVertex = tempPlate.vertex_list.GetValue(iCurVertex);
		pNextVertex = tempPlate.vertex_list.GetValue(iNextVertex);
		if (!bFirstVertex)
		{
			PROFILE_VER *pNextFeatureVertex = bInvertedTraverse ? pNextVertex : pCurVertex;
			PROFILE_VER *pPrevFeatureVertex = bInvertedTraverse ? pCurVertex : pPrevVertex;
			if (pNextFeatureVertex->type > 1)
			{
				if (pPrevFeatureVertex->type > 1)
				{
					f3dArcLine arcLine;
					PROFILE_VER *pOtherVertex = bInvertedTraverse ? pCurVertex : pNextVertex;
					pNextFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex);
					if ((bInvertedTraverse&&pNextFeatureVertex->work_norm.z > 0) ||
						(!bInvertedTraverse&&pNextFeatureVertex->work_norm.z < 0))
						next_norm = f3dPoint(arcLine.Center()) - pCurVertex->vertex;
					else
						next_norm = pCurVertex->vertex - f3dPoint(arcLine.Center());
					normalize(next_norm);
				}
				prev_norm = next_norm;	//��һ�߶�ΪԲ��ʱnext_norm��prev_normһ��
			}
			else
			{
				prev_norm = next_norm;
				GEPOINT next_vec = pCurVertex->vertex - pNextVertex->vertex;
				normalize(next_vec);
				next_norm.Set(-next_vec.y, next_vec.x);
				if (m_bClockwise)
					next_norm *= -1.0;
				if (pPrevFeatureVertex->type > 1)
					prev_norm = next_norm;	//ǰһ�߶�ΪԲ��ʱnext_norm��prev_normһ��
			}
		}
		PROFILE_VER *pFeatureVertex = bInvertedTraverse ? pCurVertex : pPrevVertex;
		PROFILE_VER *pOtherVertex = bInvertedTraverse ? pPrevVertex : pCurVertex;
		curPt = pCurVertex->vertex;
		if (m_nEnlargedSpace > 0)
		{
			if (bFirstVertex || bFinished)
				curPt = firstVertex;	//curPt += next_vec * m_nEnlargedSpace + next_norm * m_nEnlargedSpace;
			else
				curPt += prev_norm * m_nEnlargedSpace;
		}
		if (pFeatureVertex->type == 1 || bFirstVertex)
		{	//ֱ��
			CUT_PT *pPt = m_xCutEdge.cutPtArr.append();
			pPt->cByte = CUT_PT::EDGE_LINE;
			pPt->vertex = ProcessPoint(curPt - prevPt);
			prev_line.startPt = prevPt;	//��ǰ�ڵ�֮ǰ��һ��������
			prev_line.endPt = curPt;
			f3dPoint oldPrevPt = prevPt;
			prevPt = curPt;
			if (!bFirstVertex && !bFinished&&m_nEnlargedSpace > 0)
			{
				curPt = pCurVertex->vertex + next_norm * m_nEnlargedSpace;
				next_line.startPt = curPt;	//��ǰ�ڵ����ڵ���һ��������
				next_line.endPt = pNextVertex->vertex + next_norm * m_nEnlargedSpace;
				int nRetCode = Int3dll(prev_line, next_line, inters_pt);
				if (nRetCode == 1 || nRetCode == 2)
				{	//�����Ϊ�߶ζ˵���ڲ��(��������ߵ�) wht-17.06.08
					pPt->vertex = ProcessPoint(inters_pt - oldPrevPt);
					prevPt = inters_pt;
				}
				else if (nRetCode == -2)
				{	//����������߶���Σ���ӹ����߶�
					f3dArcLine arcLine;
					PROFILE_VER featureVertex;
					featureVertex.type = 4;
					featureVertex.radius = m_nEnlargedSpace;	//�˴�����Ϊ��ʱ�뻡 wht 17-05-23
					featureVertex.vertex = prevPt;
					featureVertex.center = pCurVertex->vertex;
					featureVertex.RetrieveArcLine(arcLine, curPt);
					if (prev_norm != next_norm)
					{
						pPt = m_xCutEdge.cutPtArr.append();
						pPt->vertex = ProcessPoint(curPt - prevPt);
						if (fabs(arcLine.SectorAngle()) > 0.3*Pi)
						{
							pPt->cByte = CUT_PT::EDGE_ARC;
							pPt->bClockwise = FALSE;
							pPt->center = ProcessPoint(pCurVertex->vertex - prevPt);
						}
					}
					prevPt = curPt;
				}
			}
		}
		else if (pFeatureVertex->type == 2)
		{	//Բ��
			f3dArcLine arcLine;
			pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex);
			if (!bFirstVertex && !bFinished&&m_nEnlargedSpace > 0)
			{
				PROFILE_VER featureVertex;
				featureVertex.type = 4;
				featureVertex.radius = arcLine.Radius() + m_nEnlargedSpace;
				featureVertex.center = arcLine.Center();
				if ((bInvertedTraverse&&pFeatureVertex->work_norm.z > 0) ||
					(!bInvertedTraverse&&pFeatureVertex->work_norm.z < 0))
					featureVertex.radius *= -1.0;
				if ((!bInvertedTraverse&&pFeatureVertex == pPrevVertex) ||
					(bInvertedTraverse&&pFeatureVertex == pCurVertex))
				{
					featureVertex.vertex = prevPt;
					featureVertex.RetrieveArcLine(arcLine, curPt);
				}
				else
				{
					featureVertex.vertex = curPt;
					featureVertex.RetrieveArcLine(arcLine, prevPt);
				}
			}
			double  fSectorAngle = arcLine.SectorAngle();
			if ((bInvertedTraverse&&pFeatureVertex->work_norm.z > 0) ||
				(!bInvertedTraverse&&pFeatureVertex->work_norm.z < 0))
				fSectorAngle *= -1;
			//
			CUT_PT *pPt = m_xCutEdge.cutPtArr.append();
			pPt->cByte = CUT_PT::EDGE_ARC;
			pPt->vertex = ProcessPoint(curPt - prevPt);
			pPt->center = ProcessPoint(arcLine.Center() - GEPOINT(prevPt));
			pPt->bClockwise = (fSectorAngle < 0) ? TRUE : FALSE;	//˳ʱ��Բ��
			prevPt = curPt;
		}
		else if (pFeatureVertex->type == 3)
		{	//��Բ��
			f3dArcLine arcLine;
			pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex);
			if ((!bFirstVertex && !bFinished&&m_nEnlargedSpace > 0) || bInvertedTraverse)
			{
				PROFILE_VER featureVertex;
				featureVertex.type = 3;
				featureVertex.radius = arcLine.Radius() + m_nEnlargedSpace;
				featureVertex.center = arcLine.Center();
				if (bInvertedTraverse)
					featureVertex.column_norm = arcLine.ColumnNorm()*-1.0;
				else
					featureVertex.column_norm = arcLine.ColumnNorm();
				if ((bInvertedTraverse&&pFeatureVertex->work_norm.z > 0) ||
					(!bInvertedTraverse&&pFeatureVertex->work_norm.z < 0))
					featureVertex.radius *= -1.0;
				if ((!bInvertedTraverse&&pFeatureVertex == pPrevVertex) ||
					(bInvertedTraverse&&pFeatureVertex == pCurVertex))
				{
					featureVertex.vertex = prevPt;
					featureVertex.RetrieveArcLine(arcLine, curPt);
				}
				else
				{
					featureVertex.vertex = curPt;
					featureVertex.RetrieveArcLine(arcLine, prevPt);
				}
			}
			int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 3.0, 10);
			double slice_angle = arcLine.SectorAngle() / nSlices;
			prevPt = pPrevVertex->vertex;
			ATOM_LIST<f3dPoint> ptList;
			for (int i = 1; i <= nSlices; i++)
			{
				f3dPoint pt = arcLine.PositionInAngle(i*slice_angle);
				ptList.append(pt);
			}
			int nPtCount = ptList.GetNodeNum();
			for (int i = 0; i < nPtCount; i++)
			{
				curPt = ptList[i];
				//ֱ��
				CUT_PT *pPt = m_xCutEdge.cutPtArr.append();
				pPt->cByte = CUT_PT::EDGE_LINE;
				pPt->vertex = ProcessPoint(curPt - prevPt);
				prev_line.startPt = prevPt;	//��ǰ�ڵ�֮ǰ��һ��������
				prev_line.endPt = curPt;
				f3dPoint oldPrevPt = prevPt;
				prevPt = curPt;
				if (!bFirstVertex && !bFinished&&m_nEnlargedSpace > 0)
				{
					curPt = pCurVertex->vertex + next_norm * m_nEnlargedSpace;
					next_line.startPt = curPt;	//��ǰ�ڵ����ڵ���һ��������
					next_line.endPt = pNextVertex->vertex + next_norm * m_nEnlargedSpace;
					f3dPoint inters_pt;
					int nRetCode = Int3dll(prev_line, next_line, inters_pt);
					if (nRetCode == 1 || nRetCode == 2)
					{	//�����Ϊ�߶ζ˵���ڲ��(��������ߵ�) wht-17.06.08
						pPt->vertex = ProcessPoint(inters_pt - oldPrevPt);
						prevPt = inters_pt;
					}
					else if (nRetCode == -2)
					{	//����������߶���Σ���ӹ����߶�
						f3dArcLine arcLine;
						PROFILE_VER featureVertex;
						featureVertex.type = 4;
						featureVertex.radius = m_nEnlargedSpace;	//�˴�����Ϊ��ʱ�뻡 wht 17-05-23
						featureVertex.vertex = prevPt;
						featureVertex.center = pCurVertex->vertex;
						featureVertex.RetrieveArcLine(arcLine, curPt);
						if (prev_norm != next_norm)
						{
							pPt = m_xCutEdge.cutPtArr.append();
							pPt->vertex = ProcessPoint(curPt - prevPt);
							if (fabs(arcLine.SectorAngle()) > 0.3*Pi)
							{
								pPt->cByte = CUT_PT::EDGE_ARC;
								pPt->bClockwise = FALSE;
								pPt->center = ProcessPoint(pCurVertex->vertex - prevPt);
							}
						}
						prevPt = curPt;
					}
				}
			}
		}
		if (bFinished)
			break;
		if (bInvertedTraverse)
			iCurVertex = (iCurVertex - 1) == 0 ? nCount : iCurVertex - 1;
		else
			iCurVertex = (iCurVertex + 1) > (int)nCount ? 1 : iCurVertex + 1;
	}
	//����������
	curPt += prev_vec * m_nOutLineLen;
	m_xCutEdge.cutOutVertex = ProcessPoint(curPt - prevPt);
	prevPt = curPt;
	if (m_nExtraOutLen > 0)
	{
		PROFILE_VER* pVertex = tempPlate.vertex_list.GetValue(m_pPlate->m_xCutPt.hEntId);
		curPt = pVertex->vertex + prev_vec * (m_nOutLineLen + m_nExtraOutLen);
		m_xCutEdge.extraOutVertex = ProcessPoint(curPt - prevPt);
		prevPt = curPt;
	}
	//
	m_xCutEdge.cutStartVertex = ProcessPoint(m_xStartPt - prevPt);
}
void CNCPlate::InitCutEdgePtInfoEx(GEPOINT& prevPt, CProcessPlate& destPlate)
{
	if (m_ciStartId <= 0)
		return;
	BOOL bInvertedTraverse = m_bClockwise ? TRUE : FALSE;
	if (m_pPlate->mcsFlg.ciOverturn)
		bInvertedTraverse = !bInvertedTraverse;
	//�Ⱦ�����������
	ATOM_LIST<PROFILE_VER> xDestList;
	destPlate.CalEquidistantShape(m_nEnlargedSpace, &xDestList);
	//���������
	int nNum = xDestList.GetNodeNum();
	int iCutStart = m_ciStartId - 1;
	PROFILE_VER* pPrevVer = xDestList.GetByIndex((iCutStart - 1 + nNum) % nNum);
	PROFILE_VER* pCurrVer = xDestList.GetByIndex(iCutStart);
	PROFILE_VER* pNextVer = xDestList.GetByIndex((iCutStart + 1) % nNum);
	GEPOINT prev_vec = (pCurrVer->vertex - pPrevVer->vertex).normalized();
	GEPOINT next_vec = (pCurrVer->vertex - pNextVer->vertex).normalized();
	if (bInvertedTraverse)
	{
		GEPOINT vec = prev_vec;
		prev_vec = next_vec;
		next_vec = vec;
	}
	//���������
	GEPOINT cutStartPt = pCurrVer->vertex, curPt = cutStartPt;
	if (m_nExtraInLen > 0)
	{
		curPt = cutStartPt + next_vec * (m_nInLineLen + m_nExtraInLen);
		m_xCutEdge.extraInVertex = ProcessPoint(curPt - prevPt);
		prevPt = curPt;
	}
	//�����
	curPt = cutStartPt + next_vec * m_nInLineLen;
	m_xCutEdge.cutInVertex = ProcessPoint(curPt - prevPt);
	prevPt = curPt;
	//���������и��־�
	f3dArcLine arcLine;
	int iCurVertex = -1;
	while (true)
	{
		BOOL bStart = (iCurVertex == -1);
		BOOL bFinish = (iCurVertex == iCutStart);
		if (iCurVertex == -1)
			iCurVertex = iCutStart;
		pPrevVer = xDestList.GetByIndex((iCurVertex - 1 + nNum) % nNum);
		pCurrVer = xDestList.GetByIndex(iCurVertex);
		pNextVer = xDestList.GetByIndex((iCurVertex + 1) % nNum);
		curPt = pCurrVer->vertex;
		if (pPrevVer->type == 1 && pCurrVer->type == 1)
		{
			GEPOINT prevVec = (pCurrVer->vertex - pPrevVer->vertex).normalized();
			GEPOINT nextVec = (pCurrVer->vertex - pNextVer->vertex).normalized();
			GEPOINT verify_vec = nextVec ^ prevVec;
			double angle = cal_angle_of_2vec(prevVec, nextVec);
			if (fabs(prevVec*nextVec) > 0.9999)	//���㹲��
				AppendCutPt(prevPt, curPt);
			else if (verify_vec.z < 0)	//����
				AppendCutPt(prevPt, curPt);
			else if (angle < 0.3*Pi)	//С�Ƕ�
				AppendCutPt(prevPt, curPt);
			else if (!m_bGrindingArc)	//����ĥ
				AppendCutPt(prevPt, curPt);
			else
			{	//����Բ����ĥ
				GEPOINT norm1(prevVec.y, -prevVec.x);	//˳ʱ����ת90��
				GEPOINT norm2(-nextVec.y, nextVec.x);	//��ʱ����ת90��
				GEPOINT ptS, ptE, ptC;
				double offset = 2;
				if (m_nEnlargedSpace > 0 && tan(angle / 2) > 0)
					offset = m_nEnlargedSpace * tan(angle / 2);
				ptS = pCurrVer->vertex - prevVec * offset;
				ptE = pCurrVer->vertex - nextVec * offset;
				int iRet = Int3dll(ptS - norm1 * 100, ptS + norm1 * 100, ptE - norm2 * 100, ptE + norm2 * 100, ptC);
				if (iRet == 1)
				{
					double fR = DISTANCE(ptS, ptC);
					arcLine.CreateMethod3(ptS, ptE, f3dPoint(0, 0, 1), fR, ptC);
					GEPOINT work_norm = arcLine.WorkNorm();
					if (bInvertedTraverse)
					{
						AppendCutPt(prevPt, arcLine.End());
						if(!bFinish)
							AppendCutPt(prevPt, arcLine.Start(), arcLine.Center(), work_norm*-1);
					}
					else
					{
						AppendCutPt(prevPt, arcLine.Start());
						if(!bFinish)
							AppendCutPt(prevPt, arcLine.End(), arcLine.Center(), work_norm);
					}
				}
				else
					AppendCutPt(prevPt, curPt);
			}
		}
		else if(!bStart)
		{
			PROFILE_VER *pFeatureVertex = bInvertedTraverse ? pCurrVer : pPrevVer;
			PROFILE_VER *pOtherVertex = bInvertedTraverse ? pNextVer : pCurrVer;
			if (pFeatureVertex->type == 2)
			{
				pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex);
				GEPOINT work_norm = arcLine.WorkNorm();
				if (bInvertedTraverse)
					work_norm *= -1;
				AppendCutPt(prevPt, curPt, arcLine.Center(), work_norm);
			}
			else if (pFeatureVertex->type == 3)
			{
				pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex);
				double sample_len = 4;
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, sample_len, 10);
				double slice_angle = arcLine.SectorAngle() / nSlices;
				if (bInvertedTraverse)
				{
					for (int i = nSlices-1; i >= 0; i--)
					{
						curPt = arcLine.PositionInAngle(i*slice_angle);
						AppendCutPt(prevPt, curPt);
					}
				}
				else
				{
					for (int i = 1; i <= nSlices; i++)
					{
						curPt = arcLine.PositionInAngle(i*slice_angle);
						AppendCutPt(prevPt, curPt);
					}
				}
			}
			else
				AppendCutPt(prevPt, curPt);
		}
		else
			AppendCutPt(prevPt, curPt);
		//
		if (bFinish)
			break;	//�ص���ʼ��
		if (bInvertedTraverse)
			iCurVertex = ((iCurVertex - 1) + nNum) % nNum;
		else
			iCurVertex = (iCurVertex + 1) % nNum;
	}
	//������
	curPt = cutStartPt + prev_vec * m_nOutLineLen;
	m_xCutEdge.cutOutVertex = ProcessPoint(curPt - prevPt);
	prevPt = curPt;
	//����������
	if (m_nExtraOutLen > 0)
	{
		curPt = cutStartPt + prev_vec * (m_nOutLineLen + m_nExtraOutLen);
		m_xCutEdge.extraOutVertex = ProcessPoint(curPt - prevPt);
		prevPt = curPt;
	}
	//
	m_xCutEdge.cutStartVertex = ProcessPoint(m_xStartPt - prevPt);
}
//
void CNCPlate::InitPlateNcInfo()
{
	if (m_pPlate == NULL || !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	m_xCutHole.Empty();
	m_xCutEdge.cutPtArr.Empty();
	//��ʼ���ְ�ӹ�����ϵ
	CProcessPlate tempPlate;
	m_pPlate->ClonePart(&tempPlate);
	GECS mcs = GetMCS(tempPlate, m_nEnlargedSpace, m_bGrindingArc);
	tempPlate.vertex_list.Empty();
	for (PROFILE_VER* pVertex = m_pPlate->vertex_list.GetFirst(); pVertex; pVertex = m_pPlate->vertex_list.GetNext())
		tempPlate.vertex_list.Append(*pVertex, m_pPlate->vertex_list.GetCursorKey());
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	//��ʼ����˨���и�·��
	GEPOINT prevPt = m_xStartPt;
	if (m_bCutSpecialHole)
		InitCutHoleInfo(prevPt, tempPlate);
	//��ʼ���ְ������и��־�
	//InitCutEdgePtInfo(prevPt, tempPlate);
	InitCutEdgePtInfoEx(prevPt, tempPlate);
}
bool CNCPlate::CreatePlateTxtFile(const char* file_path)
{	//д���ļ�
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return false;
	FILE* fp=fopen(file_path,"wt");
	if(fp==NULL)
		return false;
	fprintf(fp,"G21\n");			//����(��λmm)
	fprintf(fp,"G91\n");			//�����ߴ�	G90���Գߴ�
	//���Բ���и�·��
	for (CNCPlate::CUT_HOLE_PATH* pHolePath = m_xCutHole.GetFirst(); pHolePath; pHolePath = m_xCutHole.GetNext())
	{
		if (m_bCutFullCircle)
		{
			CUT_PT* pCutPt = pHolePath->cutPtArr.GetFirst();
			if (pCutPt == NULL)
				continue;
			fprintf(fp, "G00 %s\n", (char*)PointToString(pCutPt->vertex, true));//�ƶ���ͷ��Բ������е�
			fprintf(fp, "G41\n");						//����������
			fprintf(fp, "M04\n");						//���ᷴת  
			fprintf(fp, "G03 I-%.1f\n", pCutPt->radius);	//���ư뾶Ϊ35����Բ,����������Ҳ࣬Բ�������
			fprintf(fp, "M03\n");						//������ת
			fprintf(fp, "G40\n");
		}
		else
		{
			fprintf(fp, "G00 %s\n", (char*)PointToString(pHolePath->ignitionPt, true));	//���λ��
			fprintf(fp, "G41\n");			//G41���첹��
			fprintf(fp, "M07\n");			//��ʼ�и�
			for (CUT_PT* pCutPt = pHolePath->cutPtArr.GetFirst(); pCutPt; pCutPt = pHolePath->cutPtArr.GetNext())
			{
				if (pCutPt->cByte == CUT_PT::EDGE_LINE)
					fprintf(fp, "G01 %s\n", (char*)PointToString(pCutPt->vertex, true));
				else
					fprintf(fp, "G03 %s %s\n", (char*)PointToString(pCutPt->vertex, true), (char*)PointToString(pCutPt->center, true, true));
			}
			fprintf(fp, "M08\n");			//ֹͣ�и�
			fprintf(fp, "G40\n");			//�رղ���
		}
	}
	//��������и�·��
	if(m_nExtraInLen>0)
		fprintf(fp,"G00 %s\n",(char*)PointToString(m_xCutEdge.extraInVertex,true));	//����������
	else
		fprintf(fp,"G00 %s\n",(char*)PointToString(m_xCutEdge.cutInVertex,true));			//�����
	fprintf(fp,"G42\n");			//G42���첹�� G41�Ҹ�첹��
	fprintf(fp,"M07\n");			//��ʼ�и�
	if(m_nExtraInLen > 0)
		fprintf(fp,"G01 %s\n",(char*)PointToString(m_xCutEdge.cutInVertex,true));
	for(CUT_PT *pPt= m_xCutEdge.cutPtArr.GetFirst();pPt;pPt= m_xCutEdge.cutPtArr.GetNext())
	{
		if(pPt->cByte==CUT_PT::EDGE_LINE)
			fprintf(fp,"G01 %s\n",(char*)PointToString(pPt->vertex,true));
		else if(pPt->cByte==CUT_PT::EDGE_ARC)
			fprintf(fp,"G0%d %s %s\n",pPt->bClockwise?2:3,(char*)PointToString(pPt->vertex,true),(char*)PointToString(pPt->center,true,true));
	}
	fprintf(fp,"G01 %s\n",(char*)PointToString(m_xCutEdge.cutOutVertex,true));
	if(m_nExtraOutLen>0)
		fprintf(fp,"G01 %s\n",(char*)PointToString(m_xCutEdge.extraOutVertex,true));
	fprintf(fp,"M08\n");			//ֹͣ�и�
	fprintf(fp,"G40\n");			//�رղ���
	fprintf(fp,"G00 %s\n",(char*)PointToString(m_xCutEdge.cutStartVertex,true));
	fprintf(fp,"M02\n");			//ֹͣ+�������
	fclose(fp);
	return true;
}
bool CNCPlate::CreatePlateNcFile(const char* file_path)
{	//д���ļ�
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return false;
	FILE* fp=fopen(file_path,"wt");
	if(fp==NULL)
		return false;
	fprintf(fp,"O%d\n",m_iNo);		//�����
	fprintf(fp,"G86\n");			//ADָ��
	fprintf(fp,"G54\n");			//0�и�ָ��
	fprintf(fp,"G65P9303A%.1fB0C0D4E3\n",m_pPlate->GetThick());	//�ְ���Ϣ A���
	fprintf(fp,"G65P9013A130000B40000\n");	//������ת
	fprintf(fp,"M31\n");			//�Ƕ�ԭ�㸴��
	fprintf(fp,"M27\n");			//��תԭ�㸴��
	fprintf(fp,"G91\n");			//����ָ��
	fprintf(fp,"G92X0.Y0.C0.A0\n");	//����ϵ�趨
	fprintf(fp,"M7000\n");			//ͼ�θ����
	if(m_nExtraInLen>0)
		fprintf(fp,"G00%s\n",(char*)PointToString(m_xCutEdge.extraInVertex));	//����������
	else
		fprintf(fp,"G00%s\n",(char*)PointToString(m_xCutEdge.cutInVertex));			//�����ƶ��������
	fprintf(fp,"N1\n");				//��һ�����׵�
	fprintf(fp,"M54\n");			//0-CUTָ��
	fprintf(fp,"M45\n");			//��ʼ�߶��趨
	fprintf(fp,"G01C180.F21000\n");	//��ת��180�ȣ�F�ٶ�
	fprintf(fp,"G01A12.5.F21000\n");//�Ƕȣ�F�ٶ�
	fprintf(fp,"G41G01Y-0.1F4000D12\n");//G41���� D��������
	fprintf(fp,"G42.1G01Y-0.9\n");		//G42.1����
	fprintf(fp,"M17\n");			//���׿�ʼ
	if(m_nExtraInLen>0)
		fprintf(fp,"G01%s\n",(char*)PointToString(m_xCutEdge.cutInVertex));
	for(CUT_PT *pPt= m_xCutEdge.cutPtArr.GetFirst();pPt;pPt= m_xCutEdge.cutPtArr.GetNext())
	{
		if(pPt->cByte==CUT_PT::EDGE_LINE)
			fprintf(fp,"G01%s\n",(char*)PointToString(pPt->vertex));
		else if(pPt->cByte==CUT_PT::EDGE_ARC)
			fprintf(fp,"G0%d%s%s\n",pPt->bClockwise?2:3,(char*)PointToString(pPt->vertex),(char*)PointToString(pPt->center,false,true));
	}
	fprintf(fp,"G01%s\n",(char*)PointToString(m_xCutEdge.cutOutVertex));
	if(m_nExtraOutLen>0)
		fprintf(fp,"G40.1G40G01%sM16\n",(char*)PointToString(m_xCutEdge.extraOutVertex));//�����ӹ�
	else
		fprintf(fp,"G40.1G40M16\n");//�����ӹ�
	fprintf(fp,"G01A4.F6000\n");
	fprintf(fp,"M88\n");			//�м�����
	fprintf(fp,"G01%s\n",(char*)PointToString(m_xCutEdge.cutStartVertex));
	fprintf(fp,"M31\n");
	fprintf(fp,"M27\n");
	fprintf(fp,"M14\n");			//������
	fprintf(fp,"N9999\n");			//���׳���
	fprintf(fp,"M02\n");			//�������
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
			GEPOINT center(i, j), norm(0, 0, 1);
			if (bG02)
				norm.Set(0, 0, -1);
			double fRadius= DISTANCE(pVertex->vertex, pPrevVertex->center);
			f3dArcLine arcLine;
			if (arcLine.CreateMethod3(pPrevVertex->vertex, pVertex->vertex, norm, fRadius, center))
			{
				pPrevVertex->type = 2;
				pPrevVertex->sector_angle = arcLine.SectorAngle();
				pPrevVertex->work_norm = arcLine.WorkNorm();
			}
		}
		pPrevVertex=pVertex;
	}
	fclose(fp);
	return true;
}
#endif