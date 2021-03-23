#include "stdafx.h"
#include "NcPart.h"
#include "ArrayList.h"
#include "NcJg.h"
#include "SortFunc.h"
#include "LicFuncDef.h"
#include "ParseAdaptNo.h"
#include "XhMath.h"
#include "CadColorTbl.h"
#include "Expression.h"
//////////////////////////////////////////////////////////////////////////
//
//����ͬ����˨�������׾���С���бȽ�
int compare_boltInfo(const DRILL_BOLT_INFO& pBoltInfo1, const DRILL_BOLT_INFO& pBoltInfo2)
{
	return compare_double(pBoltInfo1.fHoleD, pBoltInfo2.fHoleD);
}
//����ͬ�׾�����˨�䵽ԭ�����̾�����бȽ�
int compare_boltInfo2(const DRILL_BOLT_INFO& pBoltInfo1, const DRILL_BOLT_INFO& pBoltInfo2)
{
	return compare_double(pBoltInfo1.fMinDist, pBoltInfo2.fMinDist);
}
static void GetPlateScope(CProcessPlate& tempPlate, SCOPE_STRU& scope)
{
	for (PROFILE_VER* pVertex = tempPlate.vertex_list.GetFirst(); pVertex; pVertex = tempPlate.vertex_list.GetNext())
		scope.VerifyVertex(f3dPoint(pVertex->vertex.x, pVertex->vertex.y));
	for (BOLT_INFO *pHole = tempPlate.m_xBoltInfoList.GetFirst(); pHole; pHole = tempPlate.m_xBoltInfoList.GetNext())
	{
		double radius = 0.5*pHole->bolt_d;
		scope.VerifyVertex(f3dPoint(pHole->posX - radius, pHole->posY - radius));
		scope.VerifyVertex(f3dPoint(pHole->posX + radius, pHole->posY + radius));
	}
}
BOOL GetSysParaFromReg(const char* sEntry,char* sValue)
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
int GetNearestACI(COLORREF color)
{
	if (color == 0xCFFFFFFF)
		return 0;	//δ������ɫ wht 20-02-11
	long min_dist = 2147483647L;
	long dist = 0;
	int min_index = 0;
	long red = GetRValue(color);
	long green = GetGValue(color);
	long blue = GetBValue(color);
	for (int i = 1; i < 256; i++)
	{
		dist = abs(AcadRGB[i].R - red) + abs(AcadRGB[i].G - green) + abs(AcadRGB[i].B - blue);
		if (dist < min_dist)
		{
			min_index = i;
			min_dist = dist;
		}
	}
	return min_index;
}
//����ְ��ϵĹ��ߵ�
static void RemoveCollinearPoint(CProcessPlate *pPlate)
{
	ATOM_LIST<PROFILE_VER> xProfileVerArr;
	for (PROFILE_VER *pVertex = pPlate->vertex_list.GetFirst(); pVertex; pVertex = pPlate->vertex_list.GetNext())
	{
		PROFILE_VER *pVertexPre = pPlate->vertex_list.GetPrev();
		if (pVertexPre == NULL)
		{
			pVertexPre = pPlate->vertex_list.GetTail();
			pPlate->vertex_list.GetFirst();
		}
		else
			pPlate->vertex_list.GetNext();
		PROFILE_VER *pVertexNext = pPlate->vertex_list.GetNext();
		if (pVertexNext == NULL)
		{
			pVertexNext = pPlate->vertex_list.GetFirst();
			pPlate->vertex_list.GetTail();
		}
		else
			pPlate->vertex_list.GetPrev();
		//
		f3dPoint pre_vec = (pVertex->vertex - pVertexPre->vertex).normalized();
		f3dPoint cur_vec = (pVertex->vertex - pVertexNext->vertex).normalized();
		if (pVertexPre->type == 1 && pVertex->type == 1 && fabs(pre_vec*cur_vec) > 0.9999)
			continue;	//���㹲��
		xProfileVerArr.append(*pVertex);
	}
	//
	pPlate->vertex_list.Empty();
	for (PROFILE_VER *pVertex = xProfileVerArr.GetFirst(); pVertex; pVertex = xProfileVerArr.GetNext())
		pPlate->vertex_list.SetValue(pVertex->keyId, *pVertex);
}
//////////////////////////////////////////////////////////////////////////
//CNCPart
//////////////////////////////////////////////////////////////////////////
BOOL CNCPart::m_bDeformedProfile = FALSE;
CString CNCPart::m_sExportPartInfoKeyStr;
CString CNCPart::m_sSpecialSeparator="";
//ttp��ʽ
//13�ֽ��ļ�����
//1�ֽڱ�ʶΪ��0x80����ʶ�ְ�
//2�ֽڣ�������+��˨��+1��+1Ϊ��ӡ��λ�ã�
//����������ֹ����ܶ�ȡ�����⣬ԭ�����ļ����а���2����ĸ wht 19-04-11
bool CNCPart::CreatePlateTtpFile(CProcessPlate *pPlate,const char* file_path)
{
	if(pPlate==NULL)
		return false;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//
	char drive[8], dir[MAX_PATH], fname[MAX_PATH], ext[10], file_name[MAX_PATH] = {0};
	_splitpath(file_path,drive,dir,fname,ext);
	FILE *fp = fopen(file_path,"wb");	//,ccs=UTF-8
	if(fp==NULL)
		return false;
	//
	CXhChar200 sFileName(fname);
	CXhChar50 sPartLabel = tempPlate.GetPartNo();
	sPartLabel.ToLower();
	sFileName.ToLower();
	sprintf(file_name,"%s.ttp",(char*)sFileName);
	fwrite(file_name,1,13,fp);
	BYTE flag=0x80;	//��ʾ�����ͼ
	fwrite(&flag,1,1,fp);
	short x,y;
	short n = (short)(tempPlate.vertex_list.GetNodeNum() + tempPlate.m_xBoltInfoList.GetNodeNum()) + 1;
	fwrite(&n,2,1,fp);
	fwrite(sPartLabel,1,12,fp);
	for(PROFILE_VER *pVertex=tempPlate.vertex_list.GetFirst();pVertex;pVertex=tempPlate.vertex_list.GetNext())
	{
		f3dPoint pt=pVertex->vertex;
		x = ftoi(pt.x*10);
		y = ftoi(pt.y*10);
		flag = 0x0;	//�߿�������
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
	coord_trans(centre,mcs,FALSE);
	x = ftoi(centre.x*10);
	y = ftoi(centre.y*10);
	flag = 0x07;	//���
	fwrite(&x,2,1,fp);
	fwrite(&y,2,1,fp);
	fwrite(&flag,1,1,fp);
	fclose(fp);
	return true;
}

bool CNCPart::CreatePlateWkfFile(CProcessPlate *pPlate, const char* file_path)
{
	if (pPlate == NULL)
		return false;
	BOOL bHasArcEdge = FALSE;
	for (PROFILE_VER *pVertex = pPlate->vertex_list.GetFirst(); pVertex; pVertex = pPlate->vertex_list.GetNext())
	{
		if (pVertex->type != 1)
		{
			bHasArcEdge = TRUE;
			break;
		}
	}
	if (bHasArcEdge)
		return FALSE;	//WKF��ʽ�ݲ�֧���������Բ���ߵĸְ� wht 19-06-11
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	//
	FILE *fp = fopen(file_path, "wt");
	if (fp == NULL)
		return false;
	//��һ��Ϊ����
	fprintf(fp, "\n");
	//�����������Ϣ
	PROFILE_VER *pPrevVertex = tempPlate.vertex_list.GetTail();
	for (PROFILE_VER *pVertex = tempPlate.vertex_list.GetFirst(); pVertex; pVertex = tempPlate.vertex_list.GetNext())
	{
		double x0 = fabs(pPrevVertex->vertex.x) < EPS ? 0 : pPrevVertex->vertex.x;
		double y0 = fabs(pPrevVertex->vertex.y) < EPS ? 0 : pPrevVertex->vertex.y;
		double x1 = fabs(pVertex->vertex.x) < EPS ? 0 : pVertex->vertex.x;
		double y1 = fabs(pVertex->vertex.y) < EPS ? 0 : pVertex->vertex.y;
		fprintf(fp, "L %.2f %.2f %.2f %.2f\n", x0, y0, x1, y1);
		pPrevVertex = pVertex;
	}
	//�����˨��Ϣ
	for (BOLT_INFO *pHole = tempPlate.m_xBoltInfoList.GetFirst(); pHole; pHole = tempPlate.m_xBoltInfoList.GetNext())
	{
		double x = fabs(pHole->posX) < EPS ? 0 : pHole->posX;
		double y = fabs(pHole->posY) < EPS ? 0 : pHole->posY;
		fprintf(fp, "C %.2f %.2f %.2f\n", x, y, ((pHole->bolt_d + pHole->hole_d_increment)*0.5));
	}
	//�����ı���Ϣ
	f3dPoint centre(tempPlate.mkpos.x, tempPlate.mkpos.y);
	coord_trans(centre, mcs, FALSE);
	double fTextHeight = 7.5;
	double fPosY = centre.y;
	fprintf(fp, "T %.2f %.2f ����:%s\n", centre.x, fPosY, (char*)tempPlate.GetPartNo());
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ����:%d\n", centre.x, fPosY, tempPlate.m_nSingleNum);
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ���:%d\n", centre.x, fPosY, (int)tempPlate.m_fThick);
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ����:%s\n", centre.x, fPosY, (char*)model.m_sTaType);
	fclose(fp);
	return true;
}
void CNCPart::DeformedPlateProfile(CProcessPlate *pPlate)
{
	CProcessPlate xUnDeformedPlate;
	pPlate->ClonePart(&xUnDeformedPlate);
	for(BOLT_INFO *pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
	{
		f3dPoint ls_pos(pBolt->posX,pBolt->posY,0);
		ls_pos.feature=pBolt->cFaceNo;
		ls_pos=xUnDeformedPlate.GetDeformedVertex(ls_pos);
		pBolt->posX=(float)ls_pos.x;
		pBolt->posY=(float)ls_pos.y;
	}
	for(PROFILE_VER* pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		pVertex->vertex=xUnDeformedPlate.GetDeformedVertex(pVertex->vertex);
}

void CNCPart::InitScopeToDxf(CDxfFile& dxf_file, SCOPE_STRU& scope)
{
	dxf_file.extmin.Set(scope.fMinX, scope.fMaxY);
	dxf_file.extmax.Set(scope.fMaxX, scope.fMinY);
}
void CNCPart::AddLineToDxf(CDxfFile& dxf_file, GEPOINT ptS, GEPOINT ptE, int clrIndex /*= -1*/)
{
	dxf_file.NewLine(ptS, ptE, clrIndex);
}
void CNCPart::AddArcLineToDxf(CDxfFile& dxf_file, GEPOINT ptS, GEPOINT ptE, GEPOINT ptC, double radius,int clrIndex /*= -1*/)
{
	double fAngleS = Cal2dLineAng(ptC.x, ptC.y, ptS.x, ptS.y);
	double fAngleE = Cal2dLineAng(ptC.x, ptC.y, ptE.x, ptE.y);
	dxf_file.NewArc(ptC.x, ptC.y, radius, fAngleS*DEGTORAD_COEF, fAngleE*DEGTORAD_COEF, clrIndex);
}
void CNCPart::AddEllipseToDxf(CDxfFile& dxf_file, f3dArcLine ellipse, int clrIndex /*= -1*/)
{
	if (ellipse.SectorAngle() <= 0 || ellipse.Radius() <= 0)
		return;
	int nSlices = CalArcResolution(ellipse.Radius(), ellipse.SectorAngle(), 1.0, 3.0, 10);
	if (nSlices > 0)
	{
		double slice_angle = ellipse.SectorAngle() / nSlices;
		GEPOINT pre_pt = ellipse.Start(), cut_pt;
		for (int i = 1; i <= nSlices; i++)
		{
			cut_pt = ellipse.PositionInAngle(i*slice_angle);
			AddLineToDxf(dxf_file, pre_pt, cut_pt, clrIndex);
			pre_pt = cut_pt;
		}
	}
	else
		AddLineToDxf(dxf_file, ellipse.Start(), ellipse.End(), clrIndex);
}
void CNCPart::AddCircleToDxf(CDxfFile& dxf_file, GEPOINT ptC, double radius, int clrIndex /*= -1*/)
{
	dxf_file.NewCircle(ptC, radius, clrIndex);
}
void CNCPart::AddTextToDxf(CDxfFile& dxf_file, const char* sText, GEPOINT dimPos,
	double fFontH, double fRotAngle/*=0*/, int clrIndex /*= -1*/)
{

	dxf_file.NewText(sText, dimPos, fFontH, 0, clrIndex);
}
void CNCPart::AddEdgeToDxf(CDxfFile& dxf_file, PROFILE_VER* pPrevVertex, PROFILE_VER* pCurVertex,
							GEPOINT axis_z, BOOL bOverturn)
{
	if (pPrevVertex == NULL || pCurVertex == NULL)
		return;
	GEPOINT ptS = pPrevVertex->vertex, ptE = pCurVertex->vertex, ptC = pPrevVertex->center;
	if (pPrevVertex->type == 1)
		AddLineToDxf(dxf_file, ptS, ptE);
	else if (pPrevVertex->type == 2)
	{
		f3dArcLine arcLine;
		arcLine.CreateMethod2(ptS, ptE, pPrevVertex->work_norm, pPrevVertex->sector_angle);
		ptS = arcLine.Start();
		ptE = arcLine.End();
		if (arcLine.WorkNorm()*axis_z < 0)
		{	//���߷���ͬ������Բ����ʼ�ն�
			ptS = arcLine.End();
			ptE = arcLine.Start();
		}
		if (bOverturn)
		{	//��ת�����Բ����ʼ�ն�
			GEPOINT tmePt = ptS;
			ptS = ptE;
			ptE = tmePt;
		}
		AddArcLineToDxf(dxf_file, ptS, ptE, arcLine.Center(), arcLine.Radius());
	}
	else //if(pPrevVertex->type==3)
	{
		f3dArcLine arcLine;
		if(arcLine.CreateEllipse(ptC, ptS, ptE, pPrevVertex->column_norm, pPrevVertex->work_norm, pPrevVertex->radius))
			AddEllipseToDxf(dxf_file, arcLine);
		else
			AddLineToDxf(dxf_file, ptS, ptE);
	}
}

void CNCPart::AddHoleToDxf(CDxfFile& dxf_file, BOLT_INFO* pHole)
{
	GEPOINT ptC(pHole->posX, pHole->posY);
	double fHoleR = (pHole->bolt_d + pHole->hole_d_increment)*0.5;
	if (pHole->bWaistBolt)
	{	//������Բ��
		GEPOINT vecH = pHole->waistVec, vecN(-vecH.y, vecH.x, 0);
		GEPOINT ptUpS, ptUpE, ptDwS, ptDwE;
		ptUpS = ptC - vecH * pHole->waistLen*0.5 + vecN * fHoleR;
		ptUpE = ptC + vecH * pHole->waistLen*0.5 + vecN * fHoleR;
		ptDwS = ptC - vecH * pHole->waistLen*0.5 - vecN * fHoleR;
		ptDwE = ptC + vecH * pHole->waistLen*0.5 - vecN * fHoleR;
		dxf_file.NewLine(ptUpS, ptUpE);
		dxf_file.NewLine(ptDwS, ptDwE);
		f3dArcLine arcline;
		if (arcline.CreateMethod3(ptUpS, ptDwS, GEPOINT(0, 0, 1), fHoleR, ptC))
		{
			GEPOINT ptS = arcline.Start(), ptE = arcline.End(), pt = arcline.Center();
			double fAngleS = Cal2dLineAng(pt.x, pt.y, ptS.x, ptS.y);
			double fAngleE = Cal2dLineAng(pt.x, pt.y, ptE.x, ptE.y);
			dxf_file.NewArc(pt.x, pt.y, arcline.Radius(), fAngleS*DEGTORAD_COEF, fAngleE*DEGTORAD_COEF);
		}
		if (arcline.CreateMethod3(ptDwE, ptUpE, GEPOINT(0, 0, 1), fHoleR, ptC))
		{
			GEPOINT ptS = arcline.Start(), ptE = arcline.End(), pt = arcline.Center();
			double fAngleS = Cal2dLineAng(pt.x, pt.y, ptS.x, ptS.y);
			double fAngleE = Cal2dLineAng(pt.x, pt.y, ptE.x, ptE.y);
			dxf_file.NewArc(pt.x, pt.y, arcline.Radius(), fAngleS*DEGTORAD_COEF, fAngleE*DEGTORAD_COEF);
		}
	}
	else
	{	//����Բ��
		dxf_file.NewCircle(ptC, fHoleR);
	}
}
void CNCPart::ParseNoteInfo(const char* sPartInfoKeyStr, CStringArray& sNoteArr, CProcessPlate* pPlate)
{
	if (pPlate == NULL)
		return;
	CStringArray str_arr;
	CXhChar200 key_str(sPartInfoKeyStr);
	for (char *token = strtok(key_str, "\n"); token; token = strtok(NULL, "\n"))
		str_arr.Add(token);
	for (int i = 0; i < str_arr.GetSize(); i++)
	{
		CXhChar100 sNotes, sTemp;
		key_str.Copy(str_arr[i]);
		CXhChar100 sPrevProp;
		for (char *token = strtok(key_str, "&"); token; token = strtok(NULL, "&"))
		{
			if (stricmp(token, "��Ƶ�λ") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sCompanyName);
			}
			else if (stricmp(token, "���̱��") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sPrjCode);
			}
			else if (stricmp(token, "��������") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sPrjName);
			}
			else if (stricmp(token, "����") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sTaType);
			}
			else if (stricmp(token, "����") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sTaAlias);
			}
			else if (stricmp(token, "��ӡ��") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sTaStampNo);
			}
			else if (stricmp(token, "����Ա") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sOperator);
			}
			else if (stricmp(token, "�����") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sAuditor);
			}
			else if (stricmp(token, "������") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(model.m_sCritic);
			}
			else if (stricmp(token, "����") == 0)
			{	//�򻯲����ַ��ڼ���֮ǰ����Ҫ�ڼ���ǰ�ӿո� wht 19-11-05
				if (!sPrevProp.EqualNoCase("�򻯲����ַ�") && sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(pPlate->GetPartNo());
			}
			else if (stricmp(token, "����") == 0)
			{
				char steelmark[20] = "";
				CProcessPart::QuerySteelMatMark(pPlate->cMaterial, steelmark);
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sNotes.Append(steelmark);
			}
			else if (stricmp(token, "�򻯲����ַ�") == 0)
			{	//�򻯲����ַ��ڼ���֮����Ҫ�ڼ��ַ�ǰ�ӿո� wht 19-11-05
				char steelmark[20] = "";
				CProcessPart::QuerySteelMatMark(pPlate->cMaterial, steelmark);
				if (stricmp(steelmark, "Q235") != 0)
				{
					if (!sPrevProp.EqualNoCase("����") && sNotes.GetLength() > 0)
						sNotes.Append(' ');
					sNotes.Append(toupper(pPlate->cMaterial));
				}
			}
			else if (stricmp(token, "���") == 0)
			{
				if (sNotes.GetLength() > 0)
					sNotes.Append(' ');
				sTemp.Printf("%.0f", pPlate->GetThick());
				sNotes.Append(sTemp);
			}
			else if (stricmp(token, "�ָ���") == 0)
			{
				if(m_sSpecialSeparator.GetLength()>0)
					sNotes.Append(m_sSpecialSeparator);
			}
			sPrevProp.Copy(token);
		}
		sNoteArr.Add(sNotes);
	}
}
bool CNCPart::CreatePlateDxfFile(CProcessPlate *pPlate,const char* file_path,int dxf_mode)
{
	if(pPlate==NULL)
		return false;
	CLogErrorLife logErrLife;
	if(!m_bDeformedProfile && pPlate->m_bIncDeformed)
	{
		logerr.Log("%s�ְ��е��������Ѿ����������δ���",(char*)pPlate->GetPartNo());
		return false;
	}
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	//���о�߹��յĸְ�����ȡ���������
	if(tempPlate.IsRollEdge())
		tempPlate.ProcessRollEdgeVertex();
	//������������ֵʱ���������������չ
	CXhChar100 sValue;
	double fShapeAddDist = 0;
	if (dxf_mode == CNCPart::FLAME_MODE && GetSysParaFromReg("flameCut.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	else if (dxf_mode == CNCPart::PLASMA_MODE && GetSysParaFromReg("plasmaCut.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	else if (dxf_mode == CNCPart::LASER_MODE && GetSysParaFromReg("laserPara.m_wEnlargedSpace", sValue))
		fShapeAddDist = atof(sValue);
	ATOM_LIST<PROFILE_VER> xDestList;
	tempPlate.CalEquidistantShape(fShapeAddDist, &xDestList);
	if (fShapeAddDist > 0)
	{
		tempPlate.vertex_list.Empty();
		for (PROFILE_VER* pVertex = xDestList.GetFirst(); pVertex; pVertex = xDestList.GetNext())
			tempPlate.vertex_list.Append(*pVertex);
	}
	//���ɸְ�NC�����迼�ǻ������Σ���Ըְ��еĶ������˨���л������δ���
	if(m_bDeformedProfile && !tempPlate.m_bIncDeformed)
		DeformedPlateProfile(&tempPlate);
	//ת�����ӹ�����ϵ��
	GECS mcs;
	tempPlate.GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//
	int nEdgeClrIndex = -1, nTextClrIndex = -1;
	if (GetSysParaFromReg("EdgeColor", sValue))
	{
		char tem_str[100] = "";
		COLORREF crEdge;
		sprintf(tem_str, "%s", (char*)sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crEdge);
		nEdgeClrIndex = GetNearestACI(crEdge);
	}
	if (GetSysParaFromReg("TextColor", sValue))
	{
		char tem_str[100] = "";
		COLORREF crText;
		sprintf(tem_str, "%s", (char*)sValue);
		memmove(tem_str, tem_str + 3, 97);
		sscanf(tem_str, "%X", &crText);
		nTextClrIndex = GetNearestACI(crText);
	}
	//�Ƴ����ߵ�֮ǰ��ȡ������λ�� wht 19-09-26
	f3dLine huoquLine[2] = {};
	if (dxf_mode == CNCPart::LASER_MODE)
	{
		for (int i = 2; i <= tempPlate.m_cFaceN; i++)
		{
			if (tempPlate.GetBendLineAt(i - 2, &huoquLine[i - 2]) == 0)
			{
				huoquLine[i - 2].startPt.Set();
				huoquLine[i - 2].endPt.Set();
				logerr.Log("��%d������ʼĩ�λ��ƶ������ʧ��", i - 1);
			}
		}
	}
	if(tempPlate.m_cFaceN!=3)	//����岻��ִ�й��ߵ�����
		RemoveCollinearPoint(&tempPlate);
	//����DXF�ļ�
	CDxfFile file;
	InitScopeToDxf(file, tempPlate.GetVertexsScope());
	if(file.OpenFile(file_path))
	{
		//����������
		PROFILE_VER* pPrevVertex= tempPlate.vertex_list.GetTail();
		for(PROFILE_VER* pVertex= tempPlate.vertex_list.GetFirst();pVertex;pVertex= tempPlate.vertex_list.GetNext())
		{
			if (pPlate->m_cFaceN < 3 || pPrevVertex->vertex.feature != 3 || pVertex->vertex.feature != 2)
				AddEdgeToDxf(file, pPrevVertex, pVertex, mcs.axis_z, tempPlate.mcsFlg.ciOverturn == 1);
			else
			{
				AddLineToDxf(file, pPrevVertex->vertex, tempPlate.top_point);
				AddLineToDxf(file, tempPlate.top_point, pVertex->vertex);
			}
			pPrevVertex = pVertex;
		}
		//������Բ
		if (tempPlate.m_fInnerRadius > 0)
		{
			if (!tempPlate.m_tInnerColumnNorm.IsZero() && fabs(tempPlate.m_tInnerColumnNorm*f3dPoint(0, 0, 1)) < EPS_COS)
			{	//��Բ
				f3dPoint workNorm(0, 0, 1);
				f3dPoint center = tempPlate.m_tInnerOrigin, columnNorm = tempPlate.m_tInnerColumnNorm;
				f3dPoint minorAxis = columnNorm ^ workNorm;
				normalize(minorAxis);	//��Բ���᷽��
				f3dPoint majorAxis(-minorAxis.y, minorAxis.x, minorAxis.z);
				normalize(majorAxis);	//��Բ���᷽��
				double radiusRatio = columnNorm * workNorm;
				workNorm *= (radiusRatio < EPS) ? -1 : 1;
				double minorRadius = tempPlate.m_fInnerRadius;				//��Բ�̰��᳤��
				double majorRadius = minorRadius / fabs(radiusRatio);		//��Բ�����᳤��
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
					f3dArcLine arcLine;
					arcLine.CreateEllipse(center, ptS, ptE, columnNorm, workNorm, minorRadius);
					AddEllipseToDxf(file, arcLine);
				}
			}
			else
				AddCircleToDxf(file, tempPlate.m_tInnerOrigin, tempPlate.m_fInnerRadius);
		}
		//������˨��
		double fSpecialD = 0;
		if (GetSysParaFromReg("LimitSH", sValue))
			fSpecialD = atof(sValue);
		for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
		{
			if (dxf_mode == CNCPart::FLAME_MODE || dxf_mode==CNCPart::PLASMA_MODE)
			{	//�и�����ģʽ�£���������׽��мӹ�
				if (pHole->bolt_d >= fSpecialD || pHole->bWaistBolt)
				{	//�ӹ�����Բ�׻�����Բ��
					AddHoleToDxf(file, pHole);
				}
			}
			else if (dxf_mode == CNCPart::PUNCH_MODE || dxf_mode==CNCPart::DRILL_MODE)
			{	//�崲���ģʽ��
				if(pHole->bWaistBolt)
					continue;	//�崲ģʽ�£��ݲ�������Բ��
				BOOL bNeedSH = FALSE;
				if (GetSysParaFromReg("DrillNeedSH", sValue) && dxf_mode == CNCPart::DRILL_MODE)
					bNeedSH = atoi(sValue);	//��׿����Ƿ���������
				if (GetSysParaFromReg("PunchNeedSH", sValue) && dxf_mode == CNCPart::PUNCH_MODE)
					bNeedSH = atoi(sValue);	//��׿����Ƿ���������
				if (pHole->cFuncType == 0)
					AddHoleToDxf(file, pHole);
				else if (pHole->bolt_d < fSpecialD)	//��С������׽��мӹ�
					AddHoleToDxf(file, pHole);
				else if(pHole->bolt_d >= fSpecialD && bNeedSH)
					AddHoleToDxf(file, pHole);
			}
			else if (dxf_mode == CNCPart::LASER_MODE)
			{	//����ӹ�ģʽ�£��������п�
				AddHoleToDxf(file, pHole);
			}
		}
		if (tempPlate.IsDisplayMK() && 
			(dxf_mode == CNCPart::DRILL_MODE|| dxf_mode == CNCPart::PUNCH_MODE))
		{	//���ƺ��Ͽ�(�崲���ʱ��Ҫ���Ƹ�ӡ��)
			f3dPoint centre(tempPlate.mkpos.x, tempPlate.mkpos.y);
			coord_trans(centre, mcs, FALSE);
			double fMkHoldD = 5;
			if(GetSysParaFromReg("MKHoleD",sValue))
				fMkHoldD=atof(sValue);
			if (fMkHoldD > 0)
				AddCircleToDxf(file, centre, fMkHoldD*0.5);
		}
		if (dxf_mode == CNCPart::LASER_MODE)
		{	//��������߲���+��-��ʶ������ wht 19-09-26
			double fFontH = 10;
			BOOL bOutputBendLine = FALSE, bOutputBendType = FALSE, bExplodeText = FALSE;
			if (GetSysParaFromReg("laserPara.m_bOutputBendLine", sValue))
				bOutputBendLine = atoi(sValue);
			if (GetSysParaFromReg("laserPara.m_bOutputBendType", sValue))
				bOutputBendType = atoi(sValue);
			if (GetSysParaFromReg("laserPara.m_bExplodeText", sValue))
				bExplodeText = atoi(sValue);
			if (GetSysParaFromReg("DxfTextSize", sValue))
				fFontH = atof(sValue);
			for (int i = 0; i < tempPlate.m_cFaceN-1; i++)
			{
				if(huoquLine[i].startPt!=huoquLine[i].endPt)
				{
					GEPOINT ptS = huoquLine[i].startPt, ptE = huoquLine[i].endPt;
					if (bOutputBendLine)
						AddLineToDxf(file, ptS, ptE, 1);//���û�����Ϊ��ɫ
					if (bOutputBendType)
					{
						double fHuoquAngle = (i == 0) ? tempPlate.m_fHuoQuAngle1 : tempPlate.m_fHuoQuAngle2;
						if (fHuoquAngle == 0)
						{
							logerr.Log("�ְ�%s�ĵ�%d�����淨�߲���ȷ�������Ƕȼ�������!", (char*)tempPlate.GetPartNo(), i + 1);
							continue;
						}
						CXhChar16 sText;
						if (fHuoquAngle > 0)
							sText.Printf("+%.1f", fabs(fHuoquAngle)* DEGTORAD_COEF);
						else
							sText.Printf("-%.1f", fabs(fHuoquAngle)* DEGTORAD_COEF);
						if(!bExplodeText)
							sText.Append("%%d");	//DXF�ļ��нǶȵ���д
						double fTextW = strlen(sText)*fFontH*0.7;
						GEPOINT vec = f3dPoint(ptE - ptS).normalized();
						double fRotAngle = Cal2dLineAng(0, 0, vec.x, vec.y);
						if (fRotAngle > Pi / 2 && fRotAngle < 3 * Pi / 2)
							fRotAngle -= Pi;
						GEPOINT dimVec(cos(fRotAngle), sin(fRotAngle));
						GEPOINT offVec(-sin(fRotAngle), cos(fRotAngle));
						GEPOINT dimPos = 0.5*(ptS + ptE);
						dimPos = dimPos - dimVec * fTextW*0.5 + offVec * 2;
						if (bExplodeText && model.ExplodeText)
						{
							ATOM_LIST<GELINE> lineArr;
							model.ExplodeText(sText, dimPos, fFontH, fRotAngle, lineArr);
							for (GELINE* pLine = lineArr.GetFirst(); pLine; pLine = lineArr.GetNext())
								AddLineToDxf(file, pLine->start, pLine->end, nTextClrIndex);
						}
						else
							AddTextToDxf(file, sText, dimPos, fFontH, fRotAngle*DEGTORAD_COEF, nTextClrIndex);
					}
				}
			}
			//����ӹ�ģʽ�£���ʾ�û�ָ������Ϣ
			CStringArray sNoteArr;
			ParseNoteInfo(m_sExportPartInfoKeyStr, sNoteArr, &tempPlate);
			if (sNoteArr.GetSize() > 0)
			{
				double fTextW = 0, fTextH = 0;
				for (int i = 0; i < sNoteArr.GetSize(); i++)
				{
					double len = strlen(sNoteArr[i])*fFontH*0.7;
					fTextW = max(fTextW, len);
					fTextH += fFontH * 1.4;
				}
				double fLineH = fTextH / sNoteArr.GetSize();
				//
				f3dPoint dimVec(1, 0, 0), offsetVec, dimPt;
				dimPt.Set(tempPlate.mkpos.x, tempPlate.mkpos.y);
				coord_trans(dimPt, mcs, FALSE);
				offsetVec.Set(-dimVec.y, dimVec.x, 0);
				for (int i = 0; i < sNoteArr.GetSize(); i++)
				{
					f3dPoint dimPos = dimPt + offsetVec * fLineH*i - dimVec * fTextW*0.5;
					if (bExplodeText && model.ExplodeText)
					{
						ATOM_LIST<GELINE> lineArr;
						model.ExplodeText(sNoteArr[i], dimPos, fFontH, 0, lineArr);
						for (GELINE* pLine = lineArr.GetFirst(); pLine; pLine = lineArr.GetNext())
							AddLineToDxf(file, pLine->start, pLine->end, nTextClrIndex);
					}
					else
						AddTextToDxf(file, sNoteArr[i], dimPos, fFontH, 0, nTextClrIndex);
				}
			}
		}
		file.CloseFile();
		return true;
	}
	else 
		return false;
}
#ifdef __PNC_
void CNCPart::OptimizeBolt( CProcessPlate *pPlate,CHashList<CDrillBolt>& hashDrillBoltByD, BYTE ciNcMode, BYTE ciAlgType /*= 0*/)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	CXhChar100 sValue;
	double fSpecialD = (GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
	BOOL bNeedSH = FALSE;
	if (ciNcMode == CNCPart::PUNCH_MODE)
	{	//��׿����Ƿ��������ף�����Ҳ�������
		if(GetSysParaFromReg("PunchNeedSH", sValue))
			bNeedSH = atoi(sValue);	
		if (bNeedSH && GetSysParaFromReg("PunchSortHasBigSH", sValue))
			bNeedSH = atoi(sValue);
	}
	if (ciNcMode == CNCPart::DRILL_MODE)
	{	//��׿����Ƿ��������ף�����Ҳ�������
		if(GetSysParaFromReg("DrillNeedSH", sValue))
			bNeedSH = atoi(sValue);	
		if (bNeedSH && GetSysParaFromReg("DrillSortHasBigSH", sValue))
			bNeedSH = atoi(sValue);
	}
	int iSortType = 0;
	if (ciNcMode == CNCPart::PUNCH_MODE && GetSysParaFromReg("PunchHoldSortType", sValue))
		iSortType = atoi(sValue);
	if (ciNcMode == CNCPart::DRILL_MODE && GetSysParaFromReg("DrillHoldSortType", sValue))
		iSortType = atoi(sValue);
	if (iSortType>0)
	{	//���ݿ׾����з���
		f3dPoint startPt(0, 0, 0), ls_pos;
		ARRAY_LIST<DRILL_BOLT_INFO> drillInfoArr;
		for (BOLT_INFO* pBolt = pPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pPlate->m_xBoltInfoList.GetNext())
		{
			if (pBolt->bolt_d <= 0)
				continue;
			if (pBolt->bolt_d >= fSpecialD && !bNeedSH)
				continue;	//��������ſ�
			double fHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
			DRILL_BOLT_INFO* pInfo = NULL;
			ls_pos.Set(pBolt->posX, pBolt->posY, 0);
			double fDist = DISTANCE(startPt, ls_pos);
			for (pInfo = drillInfoArr.GetFirst(); pInfo; pInfo = drillInfoArr.GetNext())
			{
				if (pInfo->fHoleD == fHoleD)
				{
					pInfo->nBoltNum++;
					if (pInfo->fMinDist > fDist)
						pInfo->fMinDist = fDist;
					break;
				}
			}
			if (pInfo == NULL)
			{
				pInfo = drillInfoArr.append();
				pInfo->fHoleD = fHoleD;
				pInfo->nBoltNum++;
				pInfo->fMinDist = fDist;
			}
		}
		if (iSortType == 2)
			CQuickSort<DRILL_BOLT_INFO>::QuickSort(drillInfoArr.m_pData, drillInfoArr.GetSize(), compare_boltInfo);
		else
			CQuickSort<DRILL_BOLT_INFO>::QuickSort(drillInfoArr.m_pData, drillInfoArr.GetSize(), compare_boltInfo2);
		//���hashDrillBoltByD
		for (int i = 0; i < drillInfoArr.GetSize(); i++)
		{
			CDrillBolt* pDrillBolt = hashDrillBoltByD.Add(ftoi(drillInfoArr[i].fHoleD * 10));
			pDrillBolt->fHoleD = drillInfoArr[i].fHoleD;
			for (BOLT_INFO* pBolt = pPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pPlate->m_xBoltInfoList.GetNext())
			{
				double fCurHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
				if (fCurHoleD != pDrillBolt->fHoleD)
					continue;
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pBolt);
				pDrillBolt->boltList.append(newBolt);
			}
		}
	}
	else
	{	//�����ݿ׾����з���
		CDrillBolt* pDrillBolt=hashDrillBoltByD.Add(0);
		for(BOLT_INFO* pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
		{
			if (pBolt->bolt_d <= 0)
				continue;
			if(pBolt->bolt_d >= fSpecialD && !bNeedSH)
				continue;	//��������ſ�
			BOLT_INFO newBolt;
			newBolt.CloneBolt(pBolt);
			pDrillBolt->fHoleD=pBolt->bolt_d+pBolt->hole_d_increment;
			pDrillBolt->boltList.append(newBolt);
		}
	}
	//3���Բ�ͬ�׾���˨����·���Ż�
	f3dPoint startPos;
	for(CDrillBolt* pDrillBolt=hashDrillBoltByD.GetFirst();pDrillBolt;pDrillBolt=hashDrillBoltByD.GetNext())
		pDrillBolt->OptimizeBoltOrder(startPos, ciAlgType);
}
void CNCPart::InitStoreMode(CHashList<CDrillBolt>& hashDrillBoltByD, ARRAY_LIST<double> &holeDList, BOOL bIncSH /*= TRUE*/)
{
	if (hashDrillBoltByD.GetNodeNum() <= 0)
		return;
	CXhChar100 sValue;
	double fSpecialD = 0;
	if (GetSysParaFromReg("LimitSH", sValue))
		fSpecialD = atof(sValue);
	//1.ͳ����˨ֱ����������С��������
	holeDList.Empty();
	CDrillBolt* pDrillBolt = NULL;
	for (pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
	{
		BOOL bSpecialBolt = TRUE;
		if (fSpecialD > 0 && pDrillBolt->fHoleD < fSpecialD)
			bSpecialBolt = FALSE;
		if (!bIncSH && bSpecialBolt)
			continue;	//�û��趨�����������
		holeDList.append(pDrillBolt->fHoleD);
	}
	CHeapSort<double>::HeapSort(holeDList.m_pData, holeDList.GetSize(), compare_double);
	//2.������˨������
	if (holeDList.GetSize() == 1 && holeDList[0] >= 17.5 && holeDList[0] < 20)
	{	//ֻ��M16����˨(�׾�����ֵ��Χ1.5-4)ʱ������ʹ��T3��ͷ
		pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[0] * 10));
		pDrillBolt->biMode = CDrillBolt::DRILL_T3;
	}
	else
	{	//�ж��п׾�ʱ��Ĭ��T2Ϊ��ף�T3ΪС��
		for (int i = 0; i < holeDList.GetSize(); i += 2)
		{
			if (i + 1 < holeDList.GetSize())
			{
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T3;
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i + 1] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T2;
			}
			else
			{
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T2;
			}
		}
	}
}
//���¸��¸ְ����˨����Ϣ
void CNCPart::RefreshPlateHoles(CProcessPlate *pPlate, BYTE ciNcMode, BYTE ciAlgType /*= 0*/)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	if(pPlate==NULL)
		return;
	GECS mcs;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//������
	CHashList<CDrillBolt> hashDrillBoltByD;
	OptimizeBolt(&tempPlate, hashDrillBoltByD, ciNcMode, ciAlgType);
	//����д����˨
	pPlate->ClonePart(&tempPlate);
	pPlate->m_xBoltInfoList.Empty();
	for(CDrillBolt* pDrillBolt=hashDrillBoltByD.GetFirst();pDrillBolt;pDrillBolt=hashDrillBoltByD.GetNext())
	{
		for(BOLT_INFO* pBolt=pDrillBolt->boltList.GetFirst();pBolt;pBolt=pDrillBolt->boltList.GetNext())
		{
			BOLT_INFO* pNewBolt=pPlate->m_xBoltInfoList.Add(0);
			f3dPoint ls_pos(pBolt->posX,pBolt->posY,0);
			coord_trans(ls_pos,mcs,TRUE);
			pNewBolt->CloneBolt(pBolt);
			pNewBolt->posX=(float)ls_pos.x;
			pNewBolt->posY=(float)ls_pos.y;
		}
	}
	//����û�н�������������
	if (pPlate->m_xBoltInfoList.GetNodeNum() < tempPlate.m_xBoltInfoList.GetNodeNum())
	{
		for (BOLT_INFO* pSrcBolt = tempPlate.m_xBoltInfoList.GetFirst(); pSrcBolt; pSrcBolt = tempPlate.m_xBoltInfoList.GetNext())
		{
			GEPOINT pos(pSrcBolt->posX, pSrcBolt->posY, 0);
			BOLT_INFO* pNewBolt = NULL;
			for (pNewBolt = pPlate->m_xBoltInfoList.GetFirst(); pNewBolt; pNewBolt = pPlate->m_xBoltInfoList.GetNext())
			{
				if (pos.IsEqual(GEPOINT(pNewBolt->posX, pNewBolt->posY), EPS2))
					break;
			}
			if (pNewBolt ==NULL)
			{
				pNewBolt = pPlate->m_xBoltInfoList.Add(0);
				pNewBolt->CloneBolt(pSrcBolt);
			}
		}
	}
}
bool CNCPart::CreatePlatePbjFile(CProcessPlate *pPlate, const char* file_path)
{
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return false;
	if (pPlate == NULL)
		return false;
	CXhChar16 sValue;
	BOOL bIncVertex = FALSE, bAutoSplitFile = FALSE, bIncSH = FALSE;
	if (GetSysParaFromReg("PunchNeedSH", sValue))
		bIncSH = atoi(sValue); 
	if (GetSysParaFromReg("PbjIncVertex", sValue))
		bIncVertex = atoi(sValue);
	if (GetSysParaFromReg("PbjAutoSplitFile", sValue))
		bAutoSplitFile = atoi(sValue);
	//
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	CHashList<CDrillBolt> hashDrillBoltByD;
	for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
	{
		double fHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
		int nKeyD = ftoi(fHoleD * 10);
		CDrillBolt* pDrillBolt = hashDrillBoltByD.GetValue(nKeyD);
		if (pDrillBolt == NULL)
			pDrillBolt = hashDrillBoltByD.Add(nKeyD);
		BOLT_INFO newBolt;
		newBolt.CloneBolt(pBolt);
		pDrillBolt->fHoleD = fHoleD;
		pDrillBolt->boltList.append(newBolt);
	}
	//
	ARRAY_LIST<double> holeDList;
	InitStoreMode(hashDrillBoltByD, holeDList, bIncSH);
	if (!bAutoSplitFile&&holeDList.GetSize() > 2)
	{	//���������˨ֱ���������2ʱ�������� wht 19-07-20
		if (holeDList.GetSize() > 2)
			logerr.Log("%s�ְ��ϲ�ͬ�׾�����2�֣�", (char*)tempPlate.GetPartNo());
		//д���ļ�
		FILE* fp = fopen(file_path, "wt");
		if (fp)
		{
			fprintf(fp, "P:%g\n", tempPlate.GetThick());      //д��P:���
			fprintf(fp, "T1:MK\n");                          //д��T1:MK
			//д����������ֱ��
			int iStart = 2; //��T2��ʼ����
			int n = holeDList.GetSize();
			for (int i = 0; i < n; i++)
			{
				int nKey0 = ftoi(holeDList[i] * 10);
				int nKey1 = ((i + 1) < n) ? ftoi(holeDList[i + 1] * 10) : 0;
				CDrillBolt *pDrillBolt0 = hashDrillBoltByD.GetValue(nKey0);
				CDrillBolt *pDrillBolt1 = nKey1 > 0 ? hashDrillBoltByD.GetValue(nKey1) : NULL;
				if (pDrillBolt0&&pDrillBolt1)
				{
					fprintf(fp, "T%d:%.1f\n", iStart + i, holeDList[i + 1]);
					fprintf(fp, "T%d:%.1f\n", iStart + i + 1, holeDList[i]);
					pDrillBolt1->biMode = i + 1;
					pDrillBolt0->biMode = i;
				}
				else if (pDrillBolt0)
				{
					fprintf(fp, "T%d:%.1f\n", iStart + i, holeDList[i]);
					pDrillBolt0->biMode = i;
				}
				i++;
			}
			for (int i = 0; i < holeDList.GetSize(); i++)
			{	//д�벻ֱͬ������˨����
				double fHoleD = holeDList[i];
				int nKey = ftoi(fHoleD * 10);
				CXhChar50 sValueX, sValueY;
				CDrillBolt* pDrillBolt = hashDrillBoltByD.GetValue(nKey);
				if (pDrillBolt)
				{
					fprintf(fp, "T%d\n", iStart + pDrillBolt->biMode);
					for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
						fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					//fprintf(fp, "X%g Y%g\n", pBolt->posX, pBolt->posY);
				}
				else
				{
					logerr.Log("%s�ְ�PBJ�ļ�����ʧ�ܣ�", (char*)tempPlate.GetPartNo());
					return false;
				}
			}
			//
			if (tempPlate.IsDisplayMK())
			{
				fprintf(fp, "MK\n");		//д��MK
				coord_trans(tempPlate.mkpos, mcs, FALSE);
				fprintf(fp, "X%.1f Y%.1f\n", tempPlate.mkpos.x, tempPlate.mkpos.y);//д��MK����
			}
			fprintf(fp, "M30\n");
			if (bIncVertex)
			{
				for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
				{
					double x = pVer->vertex.x < EPS ? 0 : pVer->vertex.x;
					double y = pVer->vertex.y < EPS ? 0 : pVer->vertex.y;
					fprintf(fp, "X%.1f Y%.1f\n", x, y);
				}
			}
			fprintf(fp, "END");//д�������ʾ
			fclose(fp);
			return true;
		}
		else
			return false;
	}
	else
	{
		int hits = 0;
		for (int i = 0; i < holeDList.GetSize(); i += 2)
		{
			if (i > 0 && holeDList.GetSize() > 2)
			{
				if (!bAutoSplitFile)
				{
					logerr.Log("%s�ְ��ϲ�ͬ�׾�����2�֣�", (char*)tempPlate.GetPartNo());
					break;
				}
			}
			int iStart = i;
			CString sPbjFilePath = file_path;
			if (holeDList.GetSize() > 2)
			{	//��Ҫ��ֵ�PBJ�ļ�����������ӿ׾� wht 19-06-18
				CXhChar50 sHoleStr;
				if (i + 1 < holeDList.GetSize())
					sHoleStr.Printf("%.1f,%.1f", holeDList[i + 1], holeDList[i]);
				else
					sHoleStr.Printf("%.1f", holeDList[i]);
				sPbjFilePath.Replace(".pbj", CXhChar16("-%s.pbj", (char*)sHoleStr));
			}
			//д���ļ�
			CDrillBolt* pDrillBolt = NULL;
			FILE* fp = fopen(sPbjFilePath, "wt");
			if (fp)
			{
				fprintf(fp, "P:%g\n", tempPlate.GetThick());      //д��P:���
				fprintf(fp, "T1:MK\n");                          //д��T1:MK
				//д����������ֱ��
				if (i + 1 < holeDList.GetSize())
				{
					fprintf(fp, "T2:%.1f\n", holeDList[i + 1]);
					fprintf(fp, "T3:%.1f\n", holeDList[i]);
					//T2��˨��
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i + 1] * 10));
					if (pDrillBolt)
					{
						fprintf(fp, "T2\n");
						for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
							fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					}
					//T3��˨��
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
					if (pDrillBolt)
					{
						fprintf(fp, "T3\n");
						for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
							fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					}
				}
				else
				{	//ֻ��һ�ֿ׾�ʱ
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
					if (pDrillBolt->biMode == CDrillBolt::DRILL_T2)
					{
						fprintf(fp, "T2:%.1f\n", holeDList[i]);
						fprintf(fp, "T3:\n");
						fprintf(fp, "T2\n");
					}
					else if (pDrillBolt->biMode == CDrillBolt::DRILL_T3)
					{
						fprintf(fp, "T2:\n");
						fprintf(fp, "T3:%.1f\n", holeDList[i]);
						fprintf(fp, "T3\n");
					}
					for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
						fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
				}
				//
				if (tempPlate.IsDisplayMK())
				{
					fprintf(fp, "MK\n");		//д��MK
					GEPOINT mk_pt = tempPlate.mkpos;
					coord_trans(mk_pt, mcs, FALSE);
					fprintf(fp, "X%.1f Y%.1f\n", mk_pt.x, mk_pt.y);//д��MK����
				}
				fprintf(fp, "M30\n");
				if (bIncVertex)
				{
					for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
					{
						double x = pVer->vertex.x < EPS ? 0 : pVer->vertex.x;
						double y = pVer->vertex.y < EPS ? 0 : pVer->vertex.y;
						fprintf(fp, "X%.1f Y%.1f\n", x, y);
					}
				}
				fprintf(fp, "END");//д�������ʾ
				fclose(fp);
				hits++;
			}
			else
				continue;
		}
		return (hits > 0);
	}
}
extern char* SearchChar(char* srcStr,char ch,bool reverseOrder/*=false*/);
bool CNCPart::CreatePlatePmzFile(CProcessPlate *pPlate,const char* file_path)
{
	if(pPlate==NULL)
		return false;
	CXhChar100 sValue;
	int iPmzMode = 0;
	BOOL bIncVertex = FALSE;
	if (GetSysParaFromReg("PmzIncVertex", sValue))
		bIncVertex = atoi(sValue);
	if (GetSysParaFromReg("PmzMode", sValue))
		iPmzMode = atoi(sValue);
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//���ݿ׾���ʼ����˨��Ϣ
	CHashList<CDrillBolt> hashDrillBoltByD;
	CDrillBolt* pDrillBolt=NULL;
	for(BOLT_INFO* pBolt=tempPlate.m_xBoltInfoList.GetFirst();pBolt;pBolt=tempPlate.m_xBoltInfoList.GetNext())
	{
		double fHoleD=pBolt->bolt_d+pBolt->hole_d_increment;
		int nKeyD=ftoi(fHoleD*10);
		pDrillBolt=hashDrillBoltByD.GetValue(nKeyD);
		if(pDrillBolt==NULL)
			pDrillBolt=hashDrillBoltByD.Add(nKeyD);
		BOLT_INFO newBolt;
		newBolt.CloneBolt(pBolt);
		pDrillBolt->fHoleD=fHoleD;
		pDrillBolt->boltList.append(newBolt);
	}
	CXhChar500 sFile(file_path);
	double fSpecialD=(GetSysParaFromReg("LimitSH",sValue))?atof(sValue):0;
	int bNeedSH=(GetSysParaFromReg("DrillNeedSH",sValue))?atoi(sValue):0;
	char* szExt=SearchChar(sFile,'.',true);
	if(szExt)	//�����к��о��
		*szExt=0;
	if (iPmzMode == 1)
	{	//һ�ֿ׾���Ӧһ��pmz�ļ�
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
				continue;
			CXhChar100 sFilePath("%s-D%s.pmz", (char*)sFile, (char*)CXhChar16(pDrillBolt->fHoleD));
			FILE* fp = fopen(sFilePath, "wt");
			if (fp == NULL)
				continue;
			fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
			fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
			fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
			fprintf(fp, "D%g\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
				//fprintf(fp, "X%g Y%g\n", pBolt->posX, pBolt->posY);
			fprintf(fp, "M30\n");
			//��ʱ����Ҫ��ʾ����������
			if (bIncVertex)
			{
				for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
					fprintf(fp,"X%.1f Y%.1f\n",pVer->vertex.x,pVer->vertex.y);
			}
			fprintf(fp, "END");//д�������ʾ
			fclose(fp);
		}
	}
	else
	{
		CXhChar100 sFilePath("%s.pmz", (char*)sFile);
		FILE* fp = fopen(sFilePath, "wt");
		if (fp == NULL)
			return false;
		fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
		fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
		fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
				continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
				//fprintf(fp, "  X%g     Y%g\n", pBolt->posX, pBolt->posY);
		}
		fprintf(fp, "M30\n");
		//����������
		if (bIncVertex)
		{
			for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
				fprintf(fp, "  X%.1f     Y%.1f\n", pVer->vertex.x, pVer->vertex.y);
		}
		fprintf(fp, "END");//д�������ʾ
		fclose(fp);
	}
	return true;
}
bool CNCPart::CreatePlatePmzCheckFile(CProcessPlate *pPlate, const char* file_path)
{
	if (pPlate == NULL)
		return false;
	CXhChar100 sValue;
	BOOL bIncVertex = FALSE;
	if (GetSysParaFromReg("PmzIncVertex", sValue))
		bIncVertex = atoi(sValue);
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	CHashList<CDrillBolt> hashDrillBoltByD;
	CXhChar500 sFile(file_path);
	double fSpecialD = (GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
	int bNeedSH = (GetSysParaFromReg("DrillNeedSH", sValue)) ? atoi(sValue) : 0;
	char* szExt = SearchChar(sFile, '.', true);
	if (szExt)	//�����к��о��
		*szExt = 0;
	//���Ԥ��pmz�ļ�
	CXhChar500 sFilePath("%s.pmz", (char*)sFile);
	FILE* fp = fopen(sFilePath, "wt");
	if (fp == NULL)
		return false;
	fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
	fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
	fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
	if (tempPlate.m_xBoltInfoList.Count < 4)
	{	//��˨����С��4ʱ��ֱ��������п�
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			//if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
			//	continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
		}
	}
	else
	{	//��ʼ��Ԥ�����ף�������˨���������Σ������ĸ����������̵��Ŀ���˨�����ܴ����ص������
		hashDrillBoltByD.Empty();
		CDrillBolt *pDrillBolt = hashDrillBoltByD.Add(0);
		SCOPE_STRU scope;
		//1.������˨����������
		CMaxDouble maxX,maxY;
		CMaxDouble minX,minY;
		for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
		{
			scope.VerifyVertex(GEPOINT(pBolt->posX, pBolt->posY));
			minX.Update(pBolt->posX, pBolt);
			minY.Update(pBolt->posY, pBolt);
			maxX.Update(pBolt->posX, pBolt);
			maxY.Update(pBolt->posY, pBolt);
		}
		BOOL bRetCode = FALSE;
		if (scope.wide() > 0 && scope.high() > 0)
		{
			CMinDouble minDistArr[4];
			GEPOINT cornerPtArr[4] = { GEPOINT(scope.fMinX, scope.fMaxY), GEPOINT(scope.fMinX, scope.fMinY),
									   GEPOINT(scope.fMaxX, scope.fMaxY), GEPOINT(scope.fMaxX, scope.fMinY) };
			for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
			{
				GEPOINT boltPos(pBolt->posX, pBolt->posY);
				for(int i=0;i<4;i++)
					minDistArr[i].Update(DISTANCE(boltPos,cornerPtArr[i]), pBolt);
			}
			CMinDouble minD;
			CHashSet<BOLT_INFO*> hashBoltSet;
			for (int i = 0; i < 4; i++)
			{
				BOLT_INFO *pTempBolt = minDistArr[i].IsInited() ? (BOLT_INFO*)minDistArr[i].m_pRelaObj : NULL;
				if (pTempBolt == NULL || hashBoltSet.GetValue((DWORD)pTempBolt) != NULL)
					continue;
				hashBoltSet.SetValue((DWORD)pTempBolt, pTempBolt);
				minD.Update(pTempBolt->bolt_d + pTempBolt->hole_d_increment, pTempBolt);
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pTempBolt);
				pDrillBolt->boltList.append(newBolt);
			}
			if (hashBoltSet.GetNodeNum() >= 2&&minD.IsInited())
			{
				pDrillBolt->fHoleD = minD.number;
				bRetCode = TRUE;
			}
			else
			{
				pDrillBolt->boltList.Empty();
				bRetCode = FALSE;
			}
		}
		else if (scope.wide() > 0)
		{	//
			BOLT_INFO *pMinXBolt = minX.IsInited() ? (BOLT_INFO*)minX.m_pRelaObj : NULL;
			BOLT_INFO *pMaxXBolt = maxX.IsInited() ? (BOLT_INFO*)maxX.m_pRelaObj : NULL;
			if (pMinXBolt&&pMaxXBolt&&pMinXBolt != pMaxXBolt)
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pMinXBolt);
				pDrillBolt->boltList.append(newBolt);
				newBolt.CloneBolt(pMaxXBolt);
				pDrillBolt->boltList.append(newBolt);
				pDrillBolt->fHoleD = min(pMinXBolt->hole_d_increment + pMinXBolt->bolt_d, pMaxXBolt->hole_d_increment + pMaxXBolt->bolt_d);
				bRetCode = TRUE;
			}
		}
		else if (scope.high() > 0)
		{
			BOLT_INFO *pMinYBolt = minY.IsInited() ? (BOLT_INFO*)minY.m_pRelaObj : NULL;
			BOLT_INFO *pMaxYBolt = maxY.IsInited() ? (BOLT_INFO*)maxY.m_pRelaObj : NULL;
			if (pMinYBolt&&pMaxYBolt&&pMinYBolt != pMaxYBolt)
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pMinYBolt);
				pDrillBolt->boltList.append(newBolt);
				newBolt.CloneBolt(pMaxYBolt);
				pDrillBolt->boltList.append(newBolt);
				pDrillBolt->fHoleD = min(pMinYBolt->hole_d_increment + pMinYBolt->bolt_d, pMaxYBolt->hole_d_increment + pMaxYBolt->bolt_d);
				bRetCode = TRUE;
			}
		}
		if(!bRetCode) //�쳣��������������˨
		{
			pDrillBolt->boltList.Empty();
			CMinDouble minD;
			for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pBolt);
				pDrillBolt->boltList.append(newBolt);
				double fHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
				minD.Update(fHoleD);
			}
			pDrillBolt->fHoleD = minD.number;
		}
		//2.��˨����
		//OptimizeBolt(&tempPlate, hashDrillBoltByD, FALSE);	//����˨����˳���Ż�
		f3dPoint startPos;
		pDrillBolt->OptimizeBoltOrder(startPos);
		//3.�����˨����
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			//if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
			//	continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
		}
	}
	fprintf(fp, "M30\n");
	//����������
	if (bIncVertex)
	{
		for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
			fprintf(fp, "  X%.1f     Y%.1f\n", pVer->vertex.x, pVer->vertex.y);
	}
	fprintf(fp, "END");//д�������ʾ
	fclose(fp);
	return true;
}
int CNCPart::GetLineLenFromExpression(double fThick, const char* sValue)
{
	CExpression expression;
	EXPRESSION_VAR* pVar = expression.varList.Append();
	pVar->fValue = fThick;
	strcpy(pVar->variableStr, "T");
	if (sValue == NULL || strlen(sValue) <= 0)
		return -1;
	else
		return (int)expression.SolveExpression(sValue);
}
bool CNCPart::CreatePlateTxtFile(CProcessPlate *pPlate,const char* file_path)
{
	CNCPlate ncPlate(pPlate);
	ncPlate.m_nInLineLen = -1;
	ncPlate.m_nOutLineLen = -1;
	CXhChar100 sValue;
	if (GetSysParaFromReg("flameCut.m_sOutLineLen", sValue))
		ncPlate.m_nOutLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("flameCut.m_sIntoLineLen", sValue))
		ncPlate.m_nInLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("flameCut.m_wEnlargedSpace", sValue))
		ncPlate.m_nEnlargedSpace = atoi(sValue);
	if (GetSysParaFromReg("flameCut.m_bCutSpecialHole", sValue))
		ncPlate.m_bCutSpecialHole = atoi(sValue);
	if (GetSysParaFromReg("flameCut.m_bGrindingArc", sValue))
		ncPlate.m_bGrindingArc = atoi(sValue);
	ncPlate.InitPlateNcInfo();
	return ncPlate.CreatePlateTxtFile(file_path);
}

bool CNCPart::CreatePlateNcFile(CProcessPlate *pPlate,const char* file_path)
{	
	CNCPlate ncPlate(pPlate);
	ncPlate.m_cCSMode = 1;
	ncPlate.m_bClockwise = TRUE;
	ncPlate.m_nInLineLen = -1;
	ncPlate.m_nOutLineLen = -1;
	ncPlate.m_nExtraInLen = 5;
	ncPlate.m_nExtraOutLen = 2;
	CXhChar100 sValue;
	if (GetSysParaFromReg("plasmaCut.m_sOutLineLen", sValue))
		ncPlate.m_nOutLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("plasmaCut.m_sIntoLineLen", sValue))
		ncPlate.m_nInLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("plasmaCut.m_wEnlargedSpace", sValue))
		ncPlate.m_nEnlargedSpace = atoi(sValue);
	if (GetSysParaFromReg("plasmaCut.m_bCutSpecialHole", sValue))
		ncPlate.m_bCutSpecialHole = atoi(sValue);
	if (GetSysParaFromReg("plasmaCut.m_bGrindingArc", sValue))
		ncPlate.m_bGrindingArc = atoi(sValue);
	ncPlate.InitPlateNcInfo();
	return ncPlate.CreatePlateNcFile(file_path);
}
bool CNCPart::CreatePlateCncFile(CProcessPlate *pPlate,const char* file_path)
{	
	CNCPlate ncPlate(pPlate);
	ncPlate.m_nInLineLen = -1;
	ncPlate.m_nOutLineLen = -1;
	CXhChar100 sValue;
	if (GetSysParaFromReg("flameCut.m_sOutLineLen", sValue))
		ncPlate.m_nOutLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("flameCut.m_sIntoLineLen", sValue))
		ncPlate.m_nInLineLen = GetLineLenFromExpression(pPlate->m_fThick, sValue);
	if (GetSysParaFromReg("flameCut.m_wEnlargedSpace", sValue))
		ncPlate.m_nEnlargedSpace = atoi(sValue);
	if (GetSysParaFromReg("flameCut.m_bCutSpecialHole", sValue))
		ncPlate.m_bCutSpecialHole = atoi(sValue);
	if (GetSysParaFromReg("flameCut.m_bGrindingArc", sValue))
		ncPlate.m_bGrindingArc = atoi(sValue);
	ncPlate.InitPlateNcInfo();
	return ncPlate.CreatePlateTxtFile(file_path);
}
void CNCPart::ImprotPlateCncOrTextFile(CProcessPlate *pPlate,const char* file_path)
{
	char drive[4];
	char dir[MAX_PATH];
	CXhChar50 file_name;
	CXhChar16 extension;
	_splitpath(file_path,drive,dir,file_name,extension);
	pPlate->SetPartNo(CXhChar100("%s%s",(char*)file_name,(char*)extension));
	CNCPlate::InitVertextListByNcFile(pPlate,file_path);
}
#endif
//�Ǹֲ���
void CNCPart::CreateAngleNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessAngle> &angleSet,
								 const char* drv_path,const char* sPartNoPrefix,const char* sWorkDir)
{
	CXhChar500 sFilePath;
	CNcJg NcJg;
	if(!NcJg.NcManager.InitJgNcDriver(drv_path))
		return;	//��ʼ��ʧ��
	CProcessAngle *pAngle=NULL;
	char fpath[MAX_PATH],bak_fpath[MAX_PATH];
	strcpy(fpath,sWorkDir);
	strcpy(bak_fpath,fpath);
	FILE *dir_fp=NULL;
	for(CNCDirectoryFile *pDirFile=NcJg.NcManager.directory.GetFirst();pDirFile;pDirFile=NcJg.NcManager.directory.GetNext())
	{	//��Ա�����ˮ��ѹ������������,�������ļ��⻹��һ��Ŀ¼�ļ�,�˴������������ɴ���Ŀ¼�ļ�
		if(pDirFile->contents.GetNodeNum()>0)
		{	//��Ҫ����Ŀ¼
			strcat(fpath,pDirFile->DIR_FILE_NAME);
			if(pDirFile->m_bDirectoryPrintByASCII)
				dir_fp=fopen(fpath,"wt");
			else
				dir_fp=fopen(fpath,"wb");
			if(dir_fp)
			{
				BOOL bak_format=NcJg.NcManager.m_bPrintByASCII;
				NcJg.NcManager.m_bPrintByASCII = pDirFile->m_bDirectoryPrintByASCII;
				if(sPartNoPrefix)
					strncpy(NcJg.sPrefix,sPartNoPrefix,19);
				for(pAngle=angleSet.GetFirst();pAngle;pAngle=angleSet.GetNext())
				{
					NcJg.InitProcessAngle(pAngle);
					for(CVariant *pVar=pDirFile->contents.GetFirst();pVar;pVar=pDirFile->contents.GetNext())
						NcJg.PrintLn(dir_fp,pVar->sVal);
				}
				NcJg.NcManager.m_bPrintByASCII = bak_format;
				fclose(dir_fp);
			}
		}
	}
	for(pAngle=angleSet.GetFirst();pAngle;pAngle=angleSet.GetNext())
	{
		strcpy(fpath,bak_fpath);
		if(strlen(sPartNoPrefix)>0)
		{
			strcat(fpath,sPartNoPrefix);
			strcat(fpath,"-");
		}
		strcat(fpath,pAngle->GetPartNo());
		NcJg.InitProcessAngle(pAngle);
		NcJg.GenNCFile(fpath,sPartNoPrefix);
	}
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
}
//����PPI�ļ�
bool CNCPart::CreatePPIFile(CProcessPart *pPart, const char* file_path)
{
	FILE *fp = fopen(file_path, "wb");
	if (fp == NULL)
		return false;
	CBuffer buffer;
	pPart->ToPPIBuffer(buffer);
	long file_len = buffer.GetLength();
	fwrite(&file_len, sizeof(long), 1, fp);
	fwrite(buffer.GetBufferPtr(), buffer.GetLength(), 1, fp);
	fclose(fp);
	return true;
}
//////////////////////////////////////////////////////////////////////////
//CDrillBolt
#include "TSPAlgorithm.h"
struct BOLT_SORT_ITEM : public ICompareClass {
	double m_fDist;
	BOLT_INFO* m_pBolt;
public:
	BOLT_SORT_ITEM() { m_fDist = 0; m_pBolt = NULL; }
	virtual int Compare(const ICompareClass *pCompareObj) const {
		BOLT_SORT_ITEM* pCmpItem = (BOLT_SORT_ITEM*)pCompareObj;
		return compare_double(this->m_fDist, pCmpItem->m_fDist);
	}
};
void CDrillBolt::OptimizeBoltOrder(f3dPoint& startPos, BYTE ciAlgType /*= 0*/)
{
	if (boltList.GetNodeNum() <= 1)
		return;
	//������˨��startPos�ľ���������򣬲��������˨��Ϊ��ʼ��˨
	ARRAY_LIST<BOLT_SORT_ITEM> boltItemArr;
	boltItemArr.SetSize(0, boltList.GetNodeNum());
	BOLT_INFO* pBolt = NULL, *pNewBolt = NULL;
	for (pBolt = boltList.GetFirst(); pBolt; pBolt = boltList.GetNext())
	{
		BOLT_SORT_ITEM* pItem = boltItemArr.append();
		pItem->m_fDist = DISTANCE(startPos, f3dPoint(pBolt->posX, pBolt->posY, 0));
		pItem->m_pBolt = pBolt;
	}
	CQuickSort<BOLT_SORT_ITEM>::QuickSortClassic(boltItemArr.m_pData, boltItemArr.GetSize());
	//��ʼ����˨�����б����˨����б�ʹ��һһ��Ӧ
	ARRAY_LIST<f3dPoint> ptArr;
	ptArr.SetSize(boltItemArr.GetSize()+1);
	ptArr[0] = startPos;
	CHashListEx<BOLT_INFO> hashBoltList;
	for(int i=0;i<boltItemArr.GetSize();i++)
	{
		pNewBolt=hashBoltList.Add(0);
		pNewBolt->CloneBolt(boltItemArr[i].m_pBolt);
		ptArr[i+1].Set(pNewBolt->posX, pNewBolt->posY, 0);
	}
	//�����Ż����򣬲����ǹ��ɻ�
	int n[300] = { 0 };
	CTSPAlgorithm xOptimize;
	xOptimize.InitData(ptArr,ciAlgType);
	xOptimize.CalBestPath(1,n);
	//����д����˨
	boltList.Empty();
	for(int i=1;i<=(int)hashBoltList.GetNodeNum();i++)
	{
		pBolt=hashBoltList.GetValue(n[i]);
		if(pBolt==NULL)
			continue;
		pNewBolt=boltList.append();
		pNewBolt->CloneBolt(pBolt);
	}
	pBolt=boltList.GetTail();
	startPos.Set(pBolt->posX,pBolt->posY);
}