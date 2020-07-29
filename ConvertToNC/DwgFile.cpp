#include "StdAfx.h"
#include "BomModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "DimStyle.h"
#include "ArrayList.h"
#include "PNCCmd.h"
#include "PNCSysPara.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
CAngleProcessInfo::CAngleProcessInfo()
{
	keyId=NULL;
	partNumId=NULL;
}
CAngleProcessInfo::~CAngleProcessInfo()
{

}
//���ɽǸֹ��տ�����
void CAngleProcessInfo::CreateRgn()
{
	f3dPoint pt;
	ARRAY_LIST<f3dPoint> profileVertexList;
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	region.CreatePolygonRgn(profileVertexList.m_pData,profileVertexList.GetSize());
}
//�ж�������Ƿ��ڽǸֹ��տ���
bool CAngleProcessInfo::PtInAngleRgn(const double* poscoord)
{
	if(region.GetAxisZ().IsZero())
		return false;
	int nRetCode=region.PtInRgn(poscoord);
	return nRetCode==1;
}
//�������ݵ����ͻ�ȡ������������
f2dRect CAngleProcessInfo::GetAngleDataRect(BYTE data_type)
{
	f2dRect rect;
	if (data_type == ITEM_TYPE_PART_NO)
		rect = g_pncSysPara.part_no_rect;
	else if (data_type == ITEM_TYPE_DES_MAT)
		rect = g_pncSysPara.mat_rect;
	else if (data_type == ITEM_TYPE_DES_GUIGE)
		rect = g_pncSysPara.guige_rect;
	else if (data_type == ITEM_TYPE_LENGTH)
		rect = g_pncSysPara.length_rect;
	else if (data_type == ITEM_TYPE_PIECE_WEIGHT)
		rect = g_pncSysPara.piece_weight_rect;
	else if (data_type == ITEM_TYPE_SUM_PART_NUM)
		rect = g_pncSysPara.jiagong_num_rect;
	else if (data_type == ITEM_TYPE_PART_NUM)
		rect = g_pncSysPara.danji_num_rect;
	else if (data_type == ITEM_TYPE_PART_NOTES)
		rect = g_pncSysPara.note_rect;
	else if (data_type == ITEM_TYPE_SUM_WEIGHT)
		rect = g_pncSysPara.sum_weight_rect;
	else if (data_type == ITEM_TYPE_PART_NOTES)
		rect = g_pncSysPara.note_rect;
	else if (data_type == ITEM_TYPE_CUT_ANGLE_S_X)
		rect = g_pncSysPara.cut_angle_SX_rect;
	else if (data_type == ITEM_TYPE_CUT_ANGLE_S_Y)
		rect = g_pncSysPara.cut_angle_SY_rect;
	else if (data_type == ITEM_TYPE_CUT_ANGLE_E_X)
		rect = g_pncSysPara.cut_angle_EX_rect;
	else if (data_type == ITEM_TYPE_CUT_ANGLE_E_Y)
		rect = g_pncSysPara.cut_angle_EY_rect;
	else if (data_type == ITEM_TYPE_HUOQU_FST)
		rect = g_pncSysPara.huoqu_fst_rect;
	else if (data_type == ITEM_TYPE_HUOQU_SEC)
		rect = g_pncSysPara.huoqu_sec_rect;
	else if (data_type == ITEM_TYPE_CUT_ROOT)
		rect = g_pncSysPara.cut_root_rect;
	else if (data_type == ITEM_TYPE_CUT_BER)
		rect = g_pncSysPara.cut_ber_rect;
	else if (data_type == ITEM_TYPE_PUSH_FLAT)
		rect = g_pncSysPara.push_flat_rect;
	else if (data_type == ITEM_TYPE_WELD)
		rect = g_pncSysPara.weld_rect;
	else if (data_type == ITEM_TYPE_KAIJIAO)
		rect = g_pncSysPara.kai_jiao_rect;
	else if (data_type == ITEM_TYPE_HEJIAO)
		rect = g_pncSysPara.he_jiao_rect;
	rect.topLeft.x+=orig_pt.x;
	rect.topLeft.y+=orig_pt.y;
	rect.bottomRight.x+=orig_pt.x;
	rect.bottomRight.y+=orig_pt.y;
	return rect;
}
//�ж�������Ƿ���ָ�����͵����ݿ���
bool CAngleProcessInfo::PtInDataRect(BYTE data_type,f3dPoint pos)
{
	f2dRect rect=GetAngleDataRect(data_type);
	f2dPoint pt(pos.x,pos.y);
	if(rect.PtInRect(pt))
		return true;
	else
		return false;
}
//�ж�������Ƿ��ڲ�ͼ����
bool CAngleProcessInfo::PtInDrawRect(f3dPoint pos)
{
	f2dRect rect = g_pncSysPara.draw_rect;
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	f2dPoint pt(pos.x, pos.y);
	if (rect.PtInRect(pt))
		return true;
	else
		return false;
}
//��ʼ���Ǹ���Ϣ
BYTE CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos,const char* sValue)
{
	BYTE cType = 0;
	if (PtInDataRect(ITEM_TYPE_PART_NO, data_pos))	//����
	{
		m_xAngle.sPartNo.Copy(sValue);
		cType = ITEM_TYPE_PART_NO;
	}
	else if (PtInDataRect(ITEM_TYPE_DES_MAT, data_pos))	//����
	{
		m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sValue);
		m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sValue);
		cType = ITEM_TYPE_DES_MAT;
	}
	else if(PtInDataRect(ITEM_TYPE_DES_GUIGE,data_pos))	//���
	{	
		CXhChar50 sSpec(sValue);
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","L");
		if (strstr(sSpec, "��"))
			sSpec.Replace("��", "*");
		if (strstr(sSpec, "x"))
			sSpec.Replace("x", "*");
		if (strstr(sSpec, "X"))
			sSpec.Replace("X", "*");
		m_xAngle.sSpec.Copy(sSpec);
		//���ݹ��ʶ�𹹼�����
		if (strstr(sSpec, "��") || strstr(sSpec, "L"))	//�Ǹ�
			m_ciType = TYPE_JG;
		else if (strstr(sSpec, "-"))
			m_ciType = TYPE_FLAT;
		else if (strstr(sSpec, "%c") || strstr(sSpec, "%C") || strstr(sSpec, "/"))
		{	//�ֹ�/Բ��
			sSpec.Replace("%c", "��");
			if (strstr(sSpec, "/"))
				m_ciType = TYPE_TUBE;
			else
				m_ciType = TYPE_YG;
		}
		else if (strstr(sSpec, "C"))
			m_ciType = TYPE_JIG;
		else if (strstr(sSpec, "G"))
			m_ciType = TYPE_GGS;
		else
		{	//Ĭ�ϰ��Ǹִ���(40*3)
			m_ciType = TYPE_JG;
			sprintf(m_xAngle.sSpec, "L%s", (char*)sSpec);
			m_xAngle.sSpec.Replace(" ", "");
			sSpec.Copy(m_xAngle.sSpec);
		}
		//
		int nWidth=0,nThick=0;
		//�ӹ������ȡ���� wht 19-08-05
		CXhChar16 sMaterial;
		CProcessPart::RestoreSpec(sSpec,&nWidth,&nThick,sMaterial);
		if (sMaterial.GetLength() > 0)
		{
			m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
		}
		m_xAngle.wide=(float)nWidth;
		m_xAngle.thick=(float)nThick;
		cType = ITEM_TYPE_DES_GUIGE;
	}
	else if (PtInDataRect(ITEM_TYPE_LENGTH, data_pos))	//����
	{
		m_xAngle.length = atof(sValue);
		cType = ITEM_TYPE_LENGTH;
	}
	else if (PtInDataRect(ITEM_TYPE_PIECE_WEIGHT, data_pos))	//����
	{
		m_xAngle.fMapSumWeight = (float)atof(sValue);
		cType = ITEM_TYPE_PIECE_WEIGHT;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NUM, data_pos))	//������
	{
		m_xAngle.SetPartNum(atoi(sValue));
		cType = ITEM_TYPE_PART_NUM;
	}
	else if(PtInDataRect(ITEM_TYPE_SUM_PART_NUM,data_pos))	//�ӹ���
	{
		m_xAngle.feature1=atoi(sValue);
		cType = ITEM_TYPE_SUM_PART_NUM;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))	//��ע
	{
		strcpy(m_xAngle.sNotes,sValue);
		if (strstr(m_xAngle.sNotes, "���Ŷ�"))
			m_xAngle.bHasFootNail = TRUE;
		else if (strstr(m_xAngle.sNotes, "�н�"))
			m_xAngle.bCutAngle = TRUE;
		else if (strstr(m_xAngle.sNotes, "���") || strstr(m_xAngle.sNotes, "�ٸ�") ||
				 strstr(m_xAngle.sNotes, "��о") || strstr(m_xAngle.sNotes, "����"))
			m_xAngle.bCutRoot = TRUE;
		else if (strstr(m_xAngle.sNotes, "����"))
			m_xAngle.bCutBer = TRUE;
		else if (strstr(m_xAngle.sNotes, "����"))
			m_xAngle.bKaiJiao = TRUE;
		else if (strstr(m_xAngle.sNotes, "�Ͻ�"))
			m_xAngle.bHeJiao = TRUE;
		else if (strstr(m_xAngle.sNotes, "ѹ��") || strstr(m_xAngle.sNotes, "���"))
			m_xAngle.nPushFlat = 0x01;
		//else if (strstr(m_xAngle.sNotes, "����") || strstr(m_xAngle.sNotes, "����"))
		//	m_xAngle.bHeJiao = TRUE;
		else if (strstr(m_xAngle.sNotes, "������") || strstr(m_xAngle.sNotes, "����") ||
				strstr(m_xAngle.sNotes, "����"))
			m_xAngle.bWeldPart = TRUE;
		cType = ITEM_TYPE_PART_NOTES;
	}
	else if (PtInDataRect(ITEM_TYPE_SUM_WEIGHT, data_pos))	//����
	{
		m_xAngle.fSumWeight = (float)atof(sValue);
		cType = ITEM_TYPE_SUM_WEIGHT;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_X, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_Y, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_X, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_Y, data_pos))
	{
		if(!m_xAngle.bCutAngle)
			m_xAngle.bCutAngle = strlen(sValue) > 0;
		cType = ITEM_TYPE_CUT_ANGLE_S_X;
	}
	else if (PtInDataRect(ITEM_TYPE_HUOQU_FST, data_pos) ||
			PtInDataRect(ITEM_TYPE_HUOQU_FST, data_pos))
	{
		if (m_xAngle.siZhiWan == 0)
			m_xAngle.siZhiWan = (strlen(sValue) > 0) ? 1 : 0;
		cType = ITEM_TYPE_HUOQU_FST;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ROOT, data_pos))
	{
		m_xAngle.bCutRoot = strlen(sValue) > 0;
		cType = ITEM_TYPE_CUT_ROOT;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_BER, data_pos))
	{
		m_xAngle.bCutBer = strlen(sValue) > 0;
		cType = ITEM_TYPE_CUT_BER;
	}
	else if (PtInDataRect(ITEM_TYPE_PUSH_FLAT, data_pos))
	{
		if (m_xAngle.nPushFlat == 0)
			m_xAngle.nPushFlat = (strlen(sValue) > 0) ? 1 : 0;
		cType = ITEM_TYPE_PUSH_FLAT;
	}
	else if (PtInDataRect(ITEM_TYPE_WELD, data_pos))
	{
		m_xAngle.bWeldPart = strlen(sValue) > 0;
		cType = ITEM_TYPE_WELD;
	}
	else if (PtInDataRect(ITEM_TYPE_KAIJIAO, data_pos))
	{
		int nLen = strlen(sValue);
		if (nLen > 1)
		{
			CString sKaiJiao = sValue;
			sKaiJiao.Replace("��", "");
			sKaiJiao.Replace("��", "");
			double fAngle = atof(sKaiJiao);
			if (fAngle > 90)
				m_xAngle.bKaiJiao = TRUE;
			else
				m_xAngle.bKaiJiao = FALSE;
		}
		else
			m_xAngle.bKaiJiao = strlen(sValue) > 0;
		cType = ITEM_TYPE_KAIJIAO;
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if(m_xAngle.bKaiJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_HEJIAO, data_pos))
	{
		int nLen = strlen(sValue);
		if (nLen > 1)
		{
			CString sHeiao = sValue;
			sHeiao.Replace("��", "");
			sHeiao.Replace("��", "");
			double fAngle = atof(sHeiao);
			if (fAngle < 90)
				m_xAngle.bHeJiao = TRUE;
			else
				m_xAngle.bHeJiao = FALSE;
		}
		else
			m_xAngle.bHeJiao = strlen(sValue) > 0;
		cType = ITEM_TYPE_HEJIAO;
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if (m_xAngle.bHeJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if(PtInDrawRect(data_pos))
	{	//�����ͼ�������������˵��
		if (strstr(sValue, "���Ŷ�"))
		{
			m_xAngle.bHasFootNail = TRUE;
			cType = ITEM_TYPE_PART_NOTES;
		}
		else if (strstr(sValue, "����"))
		{
			m_xAngle.bWeldPart = TRUE;
			cType = ITEM_TYPE_PART_NOTES;
		}
		else if (strstr(sValue, "����")|| strstr(sValue, "����"))
		{
			m_xAngle.siZhiWan = 1;
			cType = ITEM_TYPE_PART_NOTES;
		}
		else if (strstr(sValue, "�м�ѹ��"))
		{
			m_xAngle.nPushFlat = 1;
			cType = ITEM_TYPE_PART_NOTES;
		}
	}
	return cType;
}
//��ȡ�Ǹ����ݵ�����
f3dPoint CAngleProcessInfo::GetAngleDataPos(BYTE data_type)
{
	f2dRect rect=GetAngleDataRect(data_type);
	double fx=(rect.topLeft.x+rect.bottomRight.x)*0.5;
	double fy=(rect.topLeft.y+rect.bottomRight.y)*0.5;
	return f3dPoint(fx,fy,0);
}
//���½Ǹֵļӹ�����
void CAngleProcessInfo::RefreshAngleNum()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt=GetAngleDataPos(ITEM_TYPE_SUM_PART_NUM);
	CXhChar16 sPartNum("%d",m_xAngle.feature1);
	if(partNumId==NULL)
	{	//��ӽǸּӹ���
		AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
		if(pBlockTableRecord==NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord,data_pt,sPartNum,TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹּӹ���
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,partNumId,AcDb::kForWrite);
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sPartNum));
#else
			pText->setTextString(sPartNum);
#endif
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
		}
		pEnt->close();
	}
}
//���½Ǹֵĵ�����
void CAngleProcessInfo::RefreshAngleSingleNum()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_PART_NUM);
	CXhChar16 sPartNum("%d", m_xAngle.GetPartNum());
	if (singleNumId == NULL)
	{	//��ӽǸֵ�����
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sPartNum, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹֵ�����
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, singleNumId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sPartNum));
#else
			pText->setTextString(sPartNum);
#endif
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
		}
		pEnt->close();
	}
}
//
void CAngleProcessInfo::RefreshAngleSumWeight()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_SUM_WEIGHT);
	CXhChar16 sSumWeight("%.f", m_xAngle.fSumWeight);
	if (sumWeightId == NULL)
	{	//��ӽǸ�����
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sSumWeight, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹּӹ���
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, sumWeightId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sSumWeight));
#else
			pText->setTextString(sSumWeight);
#endif
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sSumWeight));
#else
			pMText->setContents(sSumWeight);
#endif
		}
		pEnt->close();
	}
}
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
CDwgFileInfo::CDwgFileInfo()
{
	m_bJgDwgFile=FALSE;
}
CDwgFileInfo::~CDwgFileInfo()
{

}
//��ʼ��DWG�ļ���Ϣ
BOOL CDwgFileInfo::ExtractDwgInfo(const char* sFileName,BOOL bJgDxf)
{
	if(strlen(sFileName)<=0)
		return FALSE;
	CXhChar100 sName;
	_splitpath(sFileName, NULL, NULL, sName, NULL);
	m_bJgDwgFile=bJgDxf;
	m_sFileName.Copy(sFileName);
	m_sDwgName.Copy(sName);
	if (m_bJgDwgFile)
		return RetrieveAngles();
	else
		return RetrievePlates();
}
BOOL CDwgFileInfo::ExtractThePlate()
{
	ManualExtractPlate(&m_xPncMode);
	return TRUE;
}
BOOL CDwgFileInfo::ImportPrintBomExcelFile(const char* sFileName)
{
	m_xPrintBomFile.Empty();
	m_xPrintBomFile.m_sFileName.Copy(sFileName);
	return m_xPrintBomFile.ImportExcelFile(&g_xUbomModel.m_xBomImoprtCfg.m_xPrintTblCfg);
}
//////////////////////////////////////////////////////////////////////////
//�ְ�DWG����
//////////////////////////////////////////////////////////////////////////
//�������ݵ�������Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPt(f3dPoint text_pos)
{
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=m_xPncMode.EnumFirstPlate(FALSE);pPlateInfo;pPlateInfo=m_xPncMode.EnumNextPlate(FALSE))
	{
		if(pPlateInfo->IsInPlate(text_pos))
			break;
	}
	return pPlateInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPartNo(const char* sPartNo)
{
	return m_xPncMode.PartFromPartNo(sPartNo);
}
//���¸ְ�ӹ�����
void CDwgFileInfo::ModifyPlateDwgPartNum()
{
	if(m_xPncMode.GetPlateNum()<=0)
		return;
	BOOL bFinish=TRUE;
	for(CPlateProcessInfo* pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate())
	{
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pLoftBom ==NULL)
		{
			bFinish=FALSE;
			logerr.Log("TMA�������ϱ���û��%s�ְ�",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			bFinish=FALSE;
			logerr.Log("%s�ְ�����޸�ʧ��!",(char*)sPartNo);
			continue;
		}
		if(pInfo->xBomPlate.feature1!= pLoftBom->feature1)
		{	//�ӹ�����ͬ�����޸�
			pInfo->xBomPlate.feature1 = pLoftBom->feature1;	//�ӹ���
			pInfo->RefreshPlateNum();
		}
	}
	if(bFinish)
		AfxMessageBox("�ְ�ӹ����޸����!");
}
//�õ���ʾͼԪ����
int CDwgFileInfo::GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet)
{
	entSet.Empty();
	long ll=0;
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId;
	ads_name ent_sel_set,entname;
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	//��ʼ��ʵ�弯��
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		entSet.SetValue(entId.handle(),entId);
		pEnt->close();
	}
	acedSSFree(ent_sel_set);
	return entSet.GetNodeNum();
}

//��ȡ���������,ȷ���պ�����
BOOL CDwgFileInfo::RetrievePlates()
{
	CPNCModel::m_bSendCommand = TRUE;
	SmartExtractPlate(&m_xPncMode);
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//�Ǹ�DWG�ļ�����
//////////////////////////////////////////////////////////////////////////
//�������ݵ������������Ӧ�Ǹ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPt(f3dPoint data_pos)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(pJgInfo->PtInAngleRgn(data_pos))
			break;
	}
	return pJgInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĽǸ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPartNo(const char* sPartNo)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(stricmp(pJgInfo->m_xAngle.sPartNo,sPartNo)==0)
			break;
	}
	return pJgInfo;
}
//���½Ǹּӹ���
void CDwgFileInfo::ModifyAngleDwgPartNum()
{
	if(m_hashJgInfo.GetNodeNum()<=0)
		return;
	CAngleProcessInfo* pJgInfo=NULL;
	BOMPART* pBomJg=NULL;
	BOOL bFinish=TRUE;
	for(pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.sPartNo;
		pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pBomJg ==NULL)
		{	
			bFinish=FALSE;
			logerr.Log("TMA���ϱ���û��%s�Ǹ�",(char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.feature1= pBomJg->feature1;	//�ӹ���
		pJgInfo->RefreshAngleNum();
	}
	if(bFinish)
		AfxMessageBox("�Ǹּӹ����޸����!");
}
//���½Ǹֵ�����
void CDwgFileInfo::ModifyAngleDwgSingleNum()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	CAngleProcessInfo* pJgInfo = NULL;
	BOMPART* pBomJg = NULL;
	BOOL bFinish = TRUE;
	for (pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg())
	{
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			bFinish = FALSE;
			logerr.Log("TMA���ϱ���û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.SetPartNum(pBomJg->GetPartNum());	//������
		pJgInfo->RefreshAngleSingleNum();
	}
	if (bFinish)
		AfxMessageBox("�Ǹֵ������޸����!");
}
//���½Ǹ�����
void CDwgFileInfo::ModifyAngleDwgSumWeight()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	CAngleProcessInfo* pJgInfo = NULL;
	BOOL bFinish = TRUE;
	for (pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg())
	{
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			bFinish = FALSE;
			logerr.Log("TMA���ϱ���û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.fSumWeight = pBomJg->fSumWeight;
		pJgInfo->RefreshAngleSumWeight();
	}
	if (bFinish)
		AfxMessageBox("�Ǹ������޸����!");
}
//��ȡ�Ǹֲ���
BOOL CDwgFileInfo::RetrieveAngles()
{
	CAcModuleResourceOverride resOverride;
	//ѡ������ʵ��ͼԪ
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, TRUE);
	//���ݽǸֹ��տ���ʶ��Ǹ�
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		//���ݽǸֹ��տ�����ȡ�Ǹ���Ϣ
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
		AcDbObjectId blockId = pReference->blockTableRecord();
		acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
		if (pTempBlockTableRecord == NULL)
			continue;
		pTempBlockTableRecord->close();
		CXhChar50 sName;
#ifdef _ARX_2007
		ACHAR* sValue = new ACHAR[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy((char*)_bstr_t(sValue));
		delete[] sValue;
#else
		char *sValue = new char[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy(sValue);
		delete[] sValue;
#endif
		if (!g_pncSysPara.IsJgCardBlockName(sName))
			continue;
		//��ӽǸּ�¼
		CAngleProcessInfo* pJgInfo = m_hashJgInfo.Add(entId.handle());
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
		pJgInfo->CreateRgn();
	}
	//����Ǹֹ��տ����������������"����"������ȡ�Ǹ���Ϣ
	CHashSet<AcDbObjectId> textIdHash;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext())
	{
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if(!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()))
			continue;
		textIdHash.SetValue(entId.handle(), entId);	//��¼�Ǹֹ��տ���ʵʱ�ı�
		//���ݼ��Źؼ���ʶ��Ǹ�
		CXhChar100 sText = GetCadTextContent(pEnt);
		if (!g_pncSysPara.IsPartLabelTitle(sText))
			continue;	//��Ч�ļ��ű���
		GEPOINT testPt = GetCadTextDimPos(pEnt);
		CAngleProcessInfo* pJgInfo = NULL;
		for (pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
		{
			if (pJgInfo->PtInAngleRgn(testPt))
				break;
		}
		if (pJgInfo == NULL)
		{	//��ӽǸּ�¼
			//���ݹ��տ�ģ���м��ű�ǵ����ýǸֹ��տ���ԭ��λ��
			GEPOINT orig_pt = g_pncSysPara.GetJgCardOrigin(testPt);
			//
			pJgInfo = m_hashJgInfo.Add(entId.handle());
			pJgInfo->keyId = entId;
			pJgInfo->SetOrig(orig_pt);
			pJgInfo->CreateRgn();
		}
	}
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//���ݽǸ�����λ�û�ȡ�Ǹ���Ϣ
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		CXhChar50 sValue=GetCadTextContent(pEnt);
		GEPOINT text_pos = GetCadTextDimPos(pEnt);
		if(strlen(sValue)<=0)	//���˿��ַ�
			continue;
		CAngleProcessInfo* pJgInfo=FindAngleByPt(text_pos);
		if (pJgInfo)
		{
			BYTE cType = pJgInfo->InitAngleInfo(text_pos, sValue);
			if (cType == ITEM_TYPE_SUM_PART_NUM)
				pJgInfo->partNumId = objId;
			else if (cType == ITEM_TYPE_SUM_WEIGHT)
				pJgInfo->sumWeightId = objId;
			else if (cType == ITEM_TYPE_PART_NUM)
				pJgInfo->singleNumId = objId;
		}
	}
	//����ȡ�ĽǸ���Ϣ���к����Լ��
	CHashStrList<BOOL> hashJgByPartNo;
	for (CAngleProcessInfo* pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
	{
		if (pJgInfo->m_xAngle.sPartNo.GetLength() <= 0)
			m_hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		else
		{
			if (hashJgByPartNo.GetValue(pJgInfo->m_xAngle.sPartNo))
				logerr.Log("����(%s)�ظ�", (char*)pJgInfo->m_xAngle.sPartNo);
			else
				hashJgByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
	}
	m_hashJgInfo.Clean();
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	return TRUE;
}
#endif