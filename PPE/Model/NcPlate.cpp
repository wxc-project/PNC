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
//
GECS CNCPlate::GetMCS()
{
	GECS mcs;
	CProcessPlate tempPlate;
	m_pPlate->ClonePart(&tempPlate);
	tempPlate.GetMCS(mcs);
	if (m_nEnlargedSpace > 0)
	{
		ATOM_LIST<PROFILE_VER> xDestList;
		tempPlate.CalEquidistantShape(m_nEnlargedSpace, &xDestList, 1);
		tempPlate.vertex_list.Empty();
		for (PROFILE_VER* pVertex = xDestList.GetFirst(); pVertex; pVertex = xDestList.GetNext())
			tempPlate.vertex_list.Append(*pVertex);
		//��������ϵԭ��
		SCOPE_STRU scope = tempPlate.GetVertexsScope(&mcs);
		mcs.origin += scope.fMinX*mcs.axis_x;
		mcs.origin += scope.fMinY*mcs.axis_y;
	}
	return mcs;
}
//
void CNCPlate::InitPlateNcInfo()
{
	if (m_pPlate == NULL || !VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	m_xCutHole.Empty();
	m_xCutEdge.cutPtArr.Empty();
	//��ʼ���ְ�ӹ�����ϵ
	BOOL bInvertedTraverse = m_bClockwise ? TRUE : FALSE;
	if (m_pPlate->mcsFlg.ciOverturn)
		bInvertedTraverse = !bInvertedTraverse;
	GECS mcs = GetMCS();
	CProcessPlate tempPlate;
	m_pPlate->ClonePart(&tempPlate);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	//2.��ʼ����˨���и�·��
	GEPOINT prevPt = m_xStartPt, curPt;
	if (m_bCutSpecialHole)
	{
		CXhChar100 sValue;
		double fSpecialD = (GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
		for (BOLT_INFO *pBoltInfo = tempPlate.m_xBoltInfoList.GetFirst(); pBoltInfo; pBoltInfo = tempPlate.m_xBoltInfoList.GetNext())
		{
			double hole_d = pBoltInfo->bolt_d + pBoltInfo->hole_d_increment;
			if (fSpecialD >0 && hole_d < fSpecialD)
				continue;
			CUT_HOLE_PATH* pCutHole = m_xCutHole.append();
			pCutHole->fHoleR = hole_d * 0.5;
			pCutHole->orgPt.Set(pBoltInfo->posX, pBoltInfo->posY, 0);
			//������λ��
			GEPOINT cenPt;
			curPt.x = pBoltInfo->posX - hole_d * 0.5 + m_nInLineLen;
			curPt.y = pBoltInfo->posY + m_nInLineLen;
			pCutHole->ignitionPt = ProcessPoint(curPt - prevPt);
			prevPt = curPt;
			//�����и�·��
			if (m_bCutFullCircle)
			{	//����Բ
				curPt.Set(pBoltInfo->posX - hole_d * 0.5, pBoltInfo->posY, 0);
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
				double fArcR = 0;
				int slices = 8, nCutPt = slices + 1;
				double fSliceAngle = 2 * Pi / slices;
				for (int ii = 0; ii <= nCutPt; ii++)
				{
					if (ii == 0)
					{	//��ʼ��
						curPt.x = pBoltInfo->posX - hole_d * 0.5;
						curPt.y = pBoltInfo->posY;
						cenPt.Set(curPt.x + m_nInLineLen, curPt.y, 0);
						fArcR = m_nInLineLen;
					}
					else if (ii == nCutPt)
					{	//����������
						curPt.x = pBoltInfo->posX - hole_d * 0.5 + m_nOutLineLen;
						curPt.y = pBoltInfo->posY - m_nOutLineLen;
						cenPt.Set(curPt.x, pBoltInfo->posY, 0);
						fArcR = m_nOutLineLen;
					}
					else
					{	//Բ���ϵ��и��
						curPt = prevPt;
						rotate_point_around_axis(curPt, fSliceAngle, pCutHole->orgPt, f3dPoint(pBoltInfo->posX, pBoltInfo->posY, 1));
						cenPt = pCutHole->orgPt;
						fArcR = pCutHole->fHoleR;
					}
					f3dArcLine arc_line;
					if (arc_line.CreateMethod3(prevPt, curPt, f3dPoint(0, 0, 1),fArcR,cenPt))
					{
						CUT_PT *pCutPt = pCutHole->cutPtArr.append();
						pCutPt->cByte = CUT_PT::EDGE_ARC;
						pCutPt->bClockwise = FALSE;
						pCutPt->vertex = ProcessPoint(curPt - prevPt);
						pCutPt->center = ProcessPoint(arc_line.Center() - f3dPoint(prevPt));
						prevPt = curPt;
					}
				}
			}
		}
	}
	//3.��ʼ���ְ������
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
	f3dPoint inters_pt;
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
	//4.��ʼ����������Ϣ
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
					pNextFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex, NULL);
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
				else
				{
					f3dArcLine arcLine;
					PROFILE_VER featureVertex;
					featureVertex.type = 4;
					featureVertex.radius = m_nEnlargedSpace;	//�˴�����Ϊ��ʱ�뻡 wht 17-05-23
					featureVertex.vertex = prevPt;
					featureVertex.center = pCurVertex->vertex;
					featureVertex.RetrieveArcLine(arcLine, curPt, NULL);
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
			pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex, NULL);
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
					featureVertex.RetrieveArcLine(arcLine, curPt, NULL);
				}
				else
				{
					featureVertex.vertex = curPt;
					featureVertex.RetrieveArcLine(arcLine, prevPt, NULL);
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
			pFeatureVertex->RetrieveArcLine(arcLine, pOtherVertex->vertex, NULL);
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
					featureVertex.RetrieveArcLine(arcLine, curPt, NULL);
				}
				else
				{
					featureVertex.vertex = curPt;
					featureVertex.RetrieveArcLine(arcLine, prevPt, NULL);
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
					else
					{
						f3dArcLine arcLine;
						PROFILE_VER featureVertex;
						featureVertex.type = 4;
						featureVertex.radius = m_nEnlargedSpace;	//�˴�����Ϊ��ʱ�뻡 wht 17-05-23
						featureVertex.vertex = prevPt;
						featureVertex.center = pCurVertex->vertex;
						featureVertex.RetrieveArcLine(arcLine, curPt, NULL);
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
				fprintf(fp, "G03 %s %s\n", (char*)PointToString(pCutPt->vertex, true), (char*)PointToString(pCutPt->center, true, true));
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
			pPrevVertex->type=4;
			pPrevVertex->center.Set(i,j);
			pPrevVertex->radius=DISTANCE(pVertex->vertex,pPrevVertex->center);
			pPrevVertex->work_norm.Set(0,0,1);
			if(bG02)	//˳ʱ�뷽��Բ��
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
		{	//ͳһ��Բ�����巽ʽ�޶�Ϊtype=2ģʽ
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