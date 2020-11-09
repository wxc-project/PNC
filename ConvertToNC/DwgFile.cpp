#include "StdAfx.h"
#include "BomModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "DimStyle.h"
#include "ArrayList.h"
#include "PNCCmd.h"
#include "PNCSysPara.h"

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
CAngleProcessInfo::CAngleProcessInfo()
{
	keyId=NULL;
	partNumId=NULL;
	keyId = NULL;
	partNumId = NULL;
	singleNumId = NULL;
	sumWeightId = NULL;
	m_ciModifyState = 0;
	m_ciType = TYPE_JG;
}
CAngleProcessInfo::~CAngleProcessInfo()
{

}
//
SCOPE_STRU CAngleProcessInfo::GetCADEntScope()
{
	f2dRect rect = g_pncSysPara.frame_rect;
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	SCOPE_STRU scope;
	scope.VerifyVertex(rect.topLeft);
	scope.VerifyVertex(rect.bottomRight);
	return scope;
}
//�ж�������Ƿ��ڽǸֹ��տ���
bool CAngleProcessInfo::PtInAngleRgn(const double* poscoord)
{
	f2dRect rect = g_pncSysPara.frame_rect;
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	f2dPoint pt(poscoord);
	if (rect.PtInRect(pt))
		return true;
	else
		return false;
}
//�ж�������Ƿ���ָ�����͵����ݿ���
bool CAngleProcessInfo::PtInDataRect(BYTE data_type, const double* poscoord)
{
	f2dRect rect = g_pncSysPara.mapJgCardRect[data_type];
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	f2dPoint pt(poscoord);
	if(rect.PtInRect(pt))
		return true;
	else
		return false;
}
//�ж�������Ƿ��ڲ�ͼ����
bool CAngleProcessInfo::PtInDrawRect(const double* poscoord)
{
	f2dRect rect = g_pncSysPara.draw_rect;
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	f2dPoint pt(poscoord);
	if (rect.PtInRect(pt))
		return true;
	else
		return false;
}
//��ʼ���Ǹֹ�����Ϣ
void CAngleProcessInfo::InitProcessInfo(const char* sValue)
{
	//const int DIST_TILE_2_PROP = 30;
	if (g_pncSysPara.IsHasFootNailTag(sValue))
		m_xAngle.bHasFootNail = TRUE;
	if (g_pncSysPara.IsHasAngleBendTag(sValue))
		m_xAngle.siZhiWan = 1;
	if (g_pncSysPara.IsHasAngleWeldTag(sValue))
		m_xAngle.bWeldPart = TRUE;
	if (g_pncSysPara.IsHasCutAngleTag(sValue))
		m_xAngle.bCutAngle = TRUE;
	if (g_pncSysPara.IsHasCutRootTag(sValue))
		m_xAngle.bCutRoot = TRUE;
	if (g_pncSysPara.IsHasCutBerTag(sValue))
		m_xAngle.bCutBer = TRUE;
	if (g_pncSysPara.IsHasPushFlatTag(sValue))
		m_xAngle.nPushFlat = 0X01;
	if (g_pncSysPara.IsHasKaiJiaoTag(sValue))
		m_xAngle.bKaiJiao = TRUE;
	if (g_pncSysPara.IsHasHeJiaoTag(sValue))
		m_xAngle.bHeJiao = TRUE;
}
//��ʼ���Ǹ���Ϣ
BYTE CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos,const char* sValue)
{
	BYTE cType = 0;
	if (PtInDataRect(ITEM_TYPE_PART_NO, data_pos))	
	{	//����
		cType = ITEM_TYPE_PART_NO;
		m_xAngle.sPartNo.Copy(sValue);
		//����Ǹּ����еĲ����ַ�
		if (CProcessPart::QuerySteelMatIndex(m_xAngle.cMaterial) > 0)
		{
			if (g_xUbomModel.m_uiJgCadPartLabelMat == 1)	 //ǰ
				m_xAngle.sPartNo.InsertBefore(m_xAngle.cMaterial, 0);
			else if (g_xUbomModel.m_uiJgCadPartLabelMat == 2)//��
				m_xAngle.sPartNo.Append(m_xAngle.cMaterial);
		}
	}
	else if (PtInDataRect(ITEM_TYPE_DES_MAT, data_pos))	
	{	//��Ʋ���
		cType = ITEM_TYPE_DES_MAT;
		m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sValue);
		m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_DES_MAT_BRIEF, data_pos))
	{	//��Ƽ򻯲���
		cType = ITEM_TYPE_DES_MAT_BRIEF;
		CString sMatBrief(sValue);
		sMatBrief.Remove(' ');
		if (sMatBrief.GetLength() > 0)
			m_xAngle.cMaterial = sMatBrief[0];
		//����Ǹּ����еĲ����ַ�
		if (CProcessPart::QuerySteelMatIndex(m_xAngle.cMaterial) > 0)
		{
			if (g_xUbomModel.m_uiJgCadPartLabelMat == 1)	 //ǰ
				m_xAngle.sPartNo.InsertBefore(m_xAngle.cMaterial, 0);
			else if (g_xUbomModel.m_uiJgCadPartLabelMat == 2)//��
				m_xAngle.sPartNo.Append(m_xAngle.cMaterial);
		}
	}
	else if(PtInDataRect(ITEM_TYPE_DES_GUIGE,data_pos))
	{	//��ƹ�񣬴�%ʱʹ��CXhCharTempl������ wht 20-08-05	
		cType = ITEM_TYPE_DES_GUIGE;
		BOOL bHasMultipleSign = FALSE;
		CString sSpec(sValue);	//CXhChar50 sSpec(sValue);
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","L");
		if (strstr(sSpec, "��"))
		{
			sSpec.Replace("��", "*");
			bHasMultipleSign = TRUE;
		}
		if (strstr(sSpec, "x"))
		{
			sSpec.Replace("x", "*");
			bHasMultipleSign = TRUE;
		}
		if (strstr(sSpec, "X"))
		{
			sSpec.Replace("X", "*");
			bHasMultipleSign = TRUE;
		}
		m_xAngle.sSpec.Copy(sSpec);
		//���ݹ��ʶ�𹹼�����
		if (strstr(sSpec, "��") || strstr(sSpec, "L"))	//�Ǹ�
			m_ciType = TYPE_JG;
		else if (strstr(sSpec, "-"))
		{
			m_ciType = TYPE_PLATE; //m_ciType = TYPE_FLAT;
			m_xAngle.siSubType = BOMPART::SUB_TYPE_COMMON_PLATE;
		}
		else if (strstr(sSpec, "%c") || strstr(sSpec, "%C") || strstr(sSpec, "/"))
		{	//�ֹ�/Բ��
			sSpec.Replace("%%%%C", "��");
			sSpec.Replace("%%%%c", "��");
			sSpec.Replace("%%C", "��");
			sSpec.Replace("%%c", "��");
			sSpec.Replace("%C", "��");
			sSpec.Replace("%c", "��");
			if (strstr(sSpec, "/"))
			{
				m_ciType = TYPE_WIRE_TUBE;	//�׹�
				m_xAngle.siSubType = BOMPART::SUB_TYPE_TUBE_WIRE;
			}
			else if (bHasMultipleSign)
			{
				m_ciType = TYPE_TUBE;		//�ֹ�
				m_xAngle.siSubType = BOMPART::SUB_TYPE_TUBE_MAIN;
			}
			else
				m_ciType = TYPE_YG;
			m_xAngle.sSpec.Copy(sSpec);
		}
		else if (strstr(sSpec, "C"))
			m_ciType = TYPE_JIG;
		else if (strstr(sSpec, "G"))
			m_ciType = TYPE_GGS;
		else
		{	//Ĭ�ϰ��Ǹִ���(40*3)
			m_ciType = TYPE_JG;
			if (sSpec.GetLength() > 0 && sSpec[0] == 'Q')
			{	//Q420B 250x20
				char sText[MAX_PATH];
				strcpy(sText, sSpec);
				char *sKey = strtok(sText, " \t");
				if (sKey != NULL)
				{	//����
					CXhChar16 sMat(sKey);
					m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMat);
					m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sMat);
				}
				sKey = strtok(NULL, " \t");
				if(sKey)
					sSpec = sKey;
			}
			sprintf(m_xAngle.sSpec, "L%s", sSpec);
			m_xAngle.sSpec.Replace(" ", "");
			sSpec = m_xAngle.sSpec;
		}
		//
		double fWidth=0,fThick=0;
		//�ӹ������ȡ���� wht 19-08-05
		CXhChar16 sMaterial;
		CProcessPart::RestoreSpec(sSpec,&fWidth,&fThick,sMaterial);
		if (sMaterial.GetLength() > 0)
		{
			m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
			m_xAngle.sSpec.Replace(sMaterial, "");
		}
		m_xAngle.wide=(float)fWidth;
		m_xAngle.thick=(float)fThick;
	}
	else if (PtInDataRect(ITEM_TYPE_LENGTH, data_pos))	
	{	//����
		cType = ITEM_TYPE_LENGTH;
		m_xAngle.length = atof(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_PIECE_WEIGHT, data_pos))	
	{	//����
		cType = ITEM_TYPE_PIECE_WEIGHT;
		m_xAngle.fMapSumWeight = (float)atof(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_SUM_WEIGHT, data_pos))
	{	//����
		cType = ITEM_TYPE_SUM_WEIGHT;
		m_xAngle.fSumWeight = (float)atof(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_TA_NUM, data_pos))
	{	//����
		cType = ITEM_TYPE_TA_NUM;
		m_xAngle.nTaNum = atoi(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NUM, data_pos))	
	{	//������
		cType = ITEM_TYPE_PART_NUM;
		m_xAngle.SetPartNum(atoi(sValue));
	}
	else if(PtInDataRect(ITEM_TYPE_SUM_PART_NUM,data_pos))
	{	//�ӹ���
		if (strstr(sValue, "�ӹ���") || strstr(sValue, "����"))
			return 0;	//�к��ֵ����ֲ�����Ϊ�ӹ��������޸� wht 20-07-29
		cType = ITEM_TYPE_SUM_PART_NUM;
		if (strstr(sValue, "x") || strstr(sValue, "X") || strstr(sValue, "*"))
		{	//֧����ȡ 1x4��ʽ�ļӹ��� wht 20-07-29s
			char *skey = strtok((char*)sValue, "x,X,*");
			int num1 = atoi(skey);
			skey = strtok(NULL, "x, X, *");
			int num2 = (skey) ? atoi(skey) : 1;
			m_xAngle.nSumPart = num1 * num2;
		}
		else
			m_xAngle.nSumPart = atoi(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))
	{	//��ע
		cType = ITEM_TYPE_PART_NOTES;
		strcpy(m_xAngle.sNotes,sValue);
		InitProcessInfo(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_MAKE_BEND,data_pos))
	{	//����
		cType = ITEM_TYPE_MAKE_BEND;
		if (g_pncSysPara.IsHasAngleBendTag(sValue))
			m_xAngle.siZhiWan = 1;
	}
	else if (PtInDataRect(ITEM_TYPE_WELD, data_pos))
	{	//����
		cType = ITEM_TYPE_WELD;
		if (g_pncSysPara.IsHasAngleWeldTag(sValue))
			m_xAngle.bWeldPart = TRUE;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_X, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_Y, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_X, data_pos)||
			PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_Y, data_pos))
	{	//�н�|��֫
		cType = ITEM_TYPE_CUT_ANGLE_S_X;
		if(!m_xAngle.bCutAngle)
			m_xAngle.bCutAngle = strlen(sValue) > 0;
	}
	else if (PtInDataRect(ITEM_TYPE_HUOQU_FST, data_pos) ||
			PtInDataRect(ITEM_TYPE_HUOQU_FST, data_pos))
	{	//һ�λ���
		cType = ITEM_TYPE_HUOQU_FST;
		if (m_xAngle.siZhiWan == 0)
			m_xAngle.siZhiWan = (strlen(sValue) > 0) ? 1 : 0;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ROOT, data_pos))
	{	//�ٸ�
		cType = ITEM_TYPE_CUT_ROOT;
		m_xAngle.bCutRoot = strlen(sValue) > 0;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_BER, data_pos))
	{	//����
		cType = ITEM_TYPE_CUT_BER;
		m_xAngle.bCutBer = strlen(sValue) > 0;
	}
	else if (PtInDataRect(ITEM_TYPE_PUSH_FLAT, data_pos))
	{	//ѹ��
		cType = ITEM_TYPE_PUSH_FLAT;
		if (m_xAngle.nPushFlat == 0)
			m_xAngle.nPushFlat = (strlen(sValue) > 0) ? 1 : 0;
	}
	else if (PtInDataRect(ITEM_TYPE_WELD, data_pos))
	{	//����
		cType = ITEM_TYPE_WELD;
		m_xAngle.bWeldPart = strlen(sValue) > 0;
	}
	else if (PtInDataRect(ITEM_TYPE_KAIJIAO, data_pos))
	{	//����
		cType = ITEM_TYPE_KAIHE_JIAO;
		int nLen = strlen(sValue);
		if (nLen > 2)
		{
			CString sKaiJiao = sValue;
			BOOL bHasPlus = sKaiJiao.Find("+")>=0;
			BOOL bHasMinus = sKaiJiao.Find("-")>=0;
			sKaiJiao.Replace("��", "");
			sKaiJiao.Replace("��", "");
			sKaiJiao.Replace("+", "");
			sKaiJiao.Replace("-", "");
			double fAngle = atof(sKaiJiao);
			//���������ֱ�ע��ʽ��
			//1. 4.5��	����0С��45�ȷ�Χ�ڣ�ʶ��Ϊ���Ƕ���
			//2.94.5��  ����90��С��180�ȣ�ʶ��Ϊ�ڶ��ֱ�ע��ʽ wht 20-08-20
			BOOL bContinueRecoge = TRUE;
			if (bHasPlus || bHasMinus)
			{	//��һ�ֱ�ע��ʽ����ע +4.5��-4.5
				if (fAngle > 0 && fAngle < 45)
				{
					if (bHasPlus)
						m_xAngle.bKaiJiao = TRUE;
					else
						m_xAngle.bHeJiao = TRUE;
					bContinueRecoge = FALSE;
				}
			}
			if(bContinueRecoge)
			{
				if ((fAngle > 0 && fAngle < 45) || (fAngle > 90 && fAngle < 180))
					m_xAngle.bKaiJiao = TRUE;
				else if (fAngle > 45 && fAngle < 90)
					m_xAngle.bHeJiao = TRUE;
			}
		}
		else
			m_xAngle.bKaiJiao = strlen(sValue) > 0;
		if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if(m_xAngle.bKaiJiao || m_xAngle.bHeJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_HEJIAO, data_pos))
	{	//�Ͻ�
		cType = ITEM_TYPE_HEJIAO;
		int nLen = strlen(sValue);
		if (nLen > 2)
		{
			CString sHeiao = sValue;
			BOOL bHasPlus = sHeiao.Find("+") >= 0;
			BOOL bHasMinus = sHeiao.Find("-") >= 0;
			sHeiao.Replace("��", "");
			sHeiao.Replace("��", "");
			sHeiao.Replace("+", "");
			sHeiao.Replace("-", "");
			//���������ֱ�ע��ʽ��
			//1. 4.5��	����0С��45�ȷ�Χ�ڣ�ʶ��Ϊ���Ƕ���
			//2.84.5��  ����45��С��90��
			BOOL bContinueRecoge = TRUE;
			double fAngle = atof(sHeiao);
			if (bHasPlus || bHasMinus)
			{	//��һ�ֱ�ע��ʽ����ע +4.5��-4.5
				if (fAngle > 0 && fAngle < 45)
				{
					if (bHasPlus)
						m_xAngle.bKaiJiao = TRUE;
					else
						m_xAngle.bHeJiao = TRUE;
					bContinueRecoge = FALSE;
				}
			}
			if (bContinueRecoge)
			{
				if (fAngle > 0 && fAngle < 90)
					m_xAngle.bHeJiao = TRUE;
				else if (fAngle > 90 && fAngle < 180)
					m_xAngle.bKaiJiao = TRUE;
			}
		}
		else
			m_xAngle.bHeJiao = strlen(sValue) > 0;
		if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if (m_xAngle.bHeJiao || m_xAngle.bKaiJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ANGLE, data_pos))
	{	//�н�
		cType = ITEM_TYPE_CUT_ANGLE;
		m_xAngle.bCutAngle = strlen(sValue) > 0;
	}
	else if(PtInDrawRect(data_pos))
	{	//�����ͼ�������������˵��
		cType = ITEM_TYPE_PART_NOTES;
		InitProcessInfo(sValue);
	}
	return cType;
}
GEPOINT CAngleProcessInfo::GetJgCardPosL_B()
{
	f2dRect rect = g_pncSysPara.frame_rect;
	GEPOINT pt_left_btm;
	pt_left_btm.x = rect.topLeft.x + orig_pt.x;
	pt_left_btm.y = rect.bottomRight.y + orig_pt.y;
	return pt_left_btm;
}
CXhChar100 CAngleProcessInfo::GetJgProcessInfo()
{
	CXhChar100 sNote;
	if (m_xAngle.siZhiWan > 0)
		sNote.Append("����", ' ');
	if (m_xAngle.bWeldPart)
		sNote.Append("�纸", ' ');
	if (m_xAngle.bCutAngle)
		sNote.Append("�н�", ' ');
	if (m_xAngle.bCutRoot)
		sNote.Append("���", ' ');
	if (m_xAngle.bCutBer)
		sNote.Append("����", ' ');
	return sNote;
}
//��ȡ�Ǹ����ݵ�����
GEPOINT CAngleProcessInfo::GetAngleDataPos(BYTE data_type)
{
	f2dRect rect = g_pncSysPara.mapJgCardRect[data_type];
	rect.topLeft.x += orig_pt.x;
	rect.topLeft.y += orig_pt.y;
	rect.bottomRight.x += orig_pt.x;
	rect.bottomRight.y += orig_pt.y;
	//
	double fx = (rect.topLeft.x + rect.bottomRight.x)*0.5;
	double fy = (rect.topLeft.y + rect.bottomRight.y)*0.5;
	return GEPOINT(fx, fy, 0);
}
//���½Ǹֵļӹ�����
void CAngleProcessInfo::RefreshAngleNum()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt=GetAngleDataPos(ITEM_TYPE_SUM_PART_NUM);
	CXhChar16 sPartNum("%d",m_xAngle.nSumPart);
	if(partNumId==NULL)
	{	//��ӽǸּӹ���
		AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
		if(pBlockTableRecord==NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord,data_pt,sPartNum,TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh,0,AcDb::kTextCenter,AcDb::kTextVertMid,AcDbObjectId::kNull,
			RGB(255,0,0));
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
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pText->setColorIndex(color_index);
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pMText->setColorIndex(color_index);
		}
		pEnt->close();
	}
	m_ciModifyState |= MODIFY_MANU_NUM;
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
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid,AcDbObjectId::kNull,
			RGB(255,0,0));
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
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pText->setColorIndex(color_index);
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pMText->setColorIndex(color_index);
		}
		pEnt->close();
	}
	m_ciModifyState |= MODIFY_SINGLE_NUM;
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
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid,AcDbObjectId::kNull,
			RGB(255,0,0));
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
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pText->setColorIndex(color_index);
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sSumWeight));
#else
			pMText->setContents(sSumWeight);
#endif
			//�޸ļӹ���������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pMText->setColorIndex(color_index);
		}
		pEnt->close();
	}
	m_ciModifyState |= MODIFY_SUM_WEIGHT;
}
//
void CAngleProcessInfo::RefreshAngleSpec()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_DES_GUIGE);
	char cMark1 = '\0', cMark2 = '\0';
	CXhChar100 sValueG;
	if (specId == NULL)
	{	//��ӽǸֹ��
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		CXhChar16 sGuiGe("L%lfX%lf", m_xAngle.wide, m_xAngle.thick);
		DimText(pBlockTableRecord, data_pt, sGuiGe, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull,
			RGB(255, 0, 0));
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹֹ��
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, specId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			sValueG.Copy(_bstr_t(pText->textString()));
#else
			sValueG.Copy(pText->textString());
#endif
			double fWidth = 0, fThick = 0;
			if (strstr(sValueG, "L") || strstr(sValueG, "��"))	//L45*3	//L45��3	//L45 3
				sscanf(sValueG, "%c%lf%c%lf",  &cMark1, &fWidth, &cMark2, &fThick);
			else if(sValueG.At(0) >='0' && sValueG.At(0) <='9')	//45*3	//45��3	//45 3 
				sscanf(sValueG, "%lf%c%lf", &fWidth, &cMark2, &fThick);
			
			CXhChar100 sFormat;
			if(cMark1 != '\0')
				sFormat.Append(cMark1);
			sFormat.Append("%.0f");
			sFormat.Append(cMark2);
			sFormat.Append("%.0f");
			CXhChar16 sGuiGe((char*)sFormat, m_xAngle.wide, m_xAngle.thick);
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sGuiGe));
#else
			pText->setTextString(sGuiGe);
#endif
			//�޸Ĺ�������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pText->setColorIndex(color_index);
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
			
			CXhChar16 sGuiGe("L%lfX%lf", m_xAngle.wide, m_xAngle.thick);
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sGuiGe));
#else
			pMText->setContents(sGuiGe);
#endif
			//�޸Ĺ�������Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pMText->setColorIndex(color_index);
		}
		pEnt->close();
	}
	m_ciModifyState |= MODIFY_DES_GUIGE;
}
//
void CAngleProcessInfo::RefreshAngleMaterial()
{
	CLockDocumentLife lockCurDocLife;
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_DES_MAT);
	CXhChar16 sMat("%s", (char*)m_xAngle.sMaterial);
	if (materialId == NULL)
	{	//��ӽǸֲ���
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sMat, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull,
			RGB(255, 0, 0));
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹֲ���
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, materialId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sMat));
#else
			pText->setTextString(sMat);
#endif
			//�޸Ĳ��ʺ�����Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pText->setColorIndex(color_index);
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sMat));
#else
			pMText->setContents(sMat);
#endif
			//�޸Ĳ��ʺ�����Ϊ��ɫ wht 20-07-29
			int color_index = GetNearestACI(RGB(255, 0, 0));
			pMText->setColorIndex(color_index);
		}
		pEnt->close();
	}
	m_ciModifyState |= MODIFY_DES_MAT;
}

//////////////////////////////////////////////////////////////////////////
//
BOOL PART_LABEL_DIM::IsPartLabelDim()
{
	if (m_xCirEnt.idCadEnt > 0 && m_xInnerText.idCadEnt > 0 &&
		m_xInnerText.sText != NULL && strlen(m_xInnerText.sText) > 0 &&
		GetSegI().iSeg > 0)
		return TRUE;
	else
		return FALSE;
}
SEGI PART_LABEL_DIM::GetSegI()
{
	SEGI segI;
	if (!ParsePartNo(m_xInnerText.sText, &segI, NULL))
		segI.iSeg = 0;
	return segI;
}
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
CDwgFileInfo::CDwgFileInfo()
{
	m_pProject = NULL;
}
CDwgFileInfo::~CDwgFileInfo()
{

}
BOOL CDwgFileInfo::ImportPrintBomExcelFile(const char* sFileName)
{
	m_xPrintBomFile.Empty();
	m_xPrintBomFile.m_sFileName.Copy(sFileName);
	return m_xPrintBomFile.ImportExcelFile(&g_xBomCfg.m_xPrintTblCfg);
}
//�ְ�DWG����
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
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ�ӹ�����Ϣ.....");
	for(CPlateProcessInfo* pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pLoftBom ==NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!",(char*)sPartNo);
			continue;
		}
		//�ӹ�����ͬ�����޸�
		if(pInfo->xBomPlate.nSumPart != pLoftBom->nSumPart)
			pInfo->RefreshPlateNum(pLoftBom->nSumPart);
	}
	DisplayCadProgress(100);
}
//���¸ְ���
void CDwgFileInfo::ModifyPlateDwgSpec()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ�����Ϣ.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		if (pInfo->xBomPlate.thick != pLoftBom->thick)
		{	//���ͬ�����޸�
			pInfo->xBomPlate.thick = pLoftBom->thick;	//���
			pInfo->xBomPlate.sSpec = pLoftBom->sSpec;	//���
			pInfo->RefreshPlateSpec();
		}
	}
	DisplayCadProgress(100);
}
//���¸ְ����
void CDwgFileInfo::ModifyPlateDwgMaterial()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ������Ϣ.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		if (toupper(pInfo->xBomPlate.cMaterial) != toupper(pLoftBom->cMaterial) ||
			(pInfo->xBomPlate.cMaterial != pLoftBom->cMaterial && !g_xUbomModel.m_bEqualH_h)||
			(pInfo->xBomPlate.cMaterial == 'A' && !pLoftBom->sMaterial.Equal(pInfo->xBomPlate.sMaterial)) ||
			(g_xUbomModel.m_bCmpQualityLevel && pInfo->xBomPlate.cQualityLevel != pLoftBom->cQualityLevel))
		{	//���ʲ�ͬ�����޸�
			pInfo->xBomPlate.cMaterial = pLoftBom->cMaterial;	//����
			pInfo->xBomPlate.sMaterial = pLoftBom->sMaterial;	//����
			pInfo->xBomPlate.cQualityLevel = pLoftBom->cQualityLevel;
			//pInfo->xPlate.cMaterial = pLoftBom->cMaterial;	//����
			//pInfo->xPlate.cQuality = pLoftBom->cQualityLevel;
			pInfo->RefreshPlateMat();
		}
	}
	DisplayCadProgress(100);
}
//���Ƹְ����ͼ������Ϣ���ɶ����������ƣ�
void CDwgFileInfo::FillPlateDwgData()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���ְ�ӹ�����Ϣ");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			continue;
		}
		if (pLoftBom->nSumPart <= 0)
		{
			logerr.Log("�ϵ������иְ�(%s)�ӹ���Ϊ0", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		//����ְ�ļӹ���
		pInfo->FillPlateNum(pLoftBom->nSumPart);
	}
	DisplayCadProgress(100);
}
//��ȡ���������,ȷ���պ�����
BOOL CDwgFileInfo::RetrievePlates(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CPNCModel::m_bSendCommand = TRUE;
	if (bSupportSelectEnts)
	{	//��ȡ�û�ѡ���ͼԪ
		CPNCModel tempMode;
		SmartExtractPlate(&tempMode, TRUE);
		//���ݿ���
		CPlateProcessInfo* pSrcPlate = NULL, *pDestPlate = NULL;
		for (pSrcPlate = tempMode.EnumFirstPlate(FALSE); pSrcPlate; pSrcPlate = tempMode.EnumNextPlate(FALSE))
		{
			pDestPlate = m_xPncMode.GetPlateInfo(pSrcPlate->GetPartNo());
			if (pDestPlate == NULL)
				pDestPlate = m_xPncMode.AppendPlate(pSrcPlate->GetPartNo());
			pDestPlate->CopyAttributes(pSrcPlate);
		}
	}
	else
	{	//��ȡ����ͼԪ
		m_xPncMode.Empty();
		SmartExtractPlate(&m_xPncMode);
	}
	return TRUE;
}
//�Ǹ�DWG�ļ�����
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
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "���½Ǹּӹ���.....");
	for(CAngleProcessInfo* pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pBomJg ==NULL)
		{	
			logerr.Log("�ϵ�������û��%s�Ǹ�",(char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.nSumPart != pBomJg->nSumPart)
		{
			pJgInfo->m_xAngle.nSumPart = pBomJg->nSumPart;	//�ӹ���
			pJgInfo->RefreshAngleNum();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֵ�����
void CDwgFileInfo::ModifyAngleDwgSingleNum()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "���½Ǹֵ�����.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.SetPartNum(pBomJg->GetPartNum());	//������
		pJgInfo->RefreshAngleSingleNum();
	}
	DisplayCadProgress(100);
}
//���½Ǹ�����
void CDwgFileInfo::ModifyAngleDwgSumWeight()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹ�����.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.fSumWeight != pBomJg->fSumWeight)
		{
			pJgInfo->m_xAngle.fSumWeight = pBomJg->fSumWeight;
			pJgInfo->RefreshAngleSumWeight();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֹ��
void CDwgFileInfo::ModifyAngleDwgSpec()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹֹ����Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (!pJgInfo->m_xAngle.sSpec.EqualNoCase(pBomJg->sSpec))
		{
			pJgInfo->m_xAngle.wide = pBomJg->wide;
			pJgInfo->m_xAngle.wingWideY = pBomJg->wingWideY;
			pJgInfo->m_xAngle.thick = pBomJg->thick;
			pJgInfo->m_xAngle.sSpec = pBomJg->sSpec;
			pJgInfo->RefreshAngleSpec();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֲ���
void CDwgFileInfo::ModifyAngleDwgMaterial()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹֲ�����Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (toupper(pJgInfo->m_xAngle.cMaterial) != toupper(pBomJg->cMaterial) ||
			(pJgInfo->m_xAngle.cMaterial != pBomJg->cMaterial && !g_xUbomModel.m_bEqualH_h) ||
			(pJgInfo->m_xAngle.cMaterial == 'A' && !pBomJg->sMaterial.Equal(pJgInfo->m_xAngle.sMaterial)) ||
			(g_xUbomModel.m_bCmpQualityLevel && pJgInfo->m_xAngle.cQualityLevel != pBomJg->cQualityLevel))
		{
			pJgInfo->m_xAngle.cMaterial = pBomJg->cMaterial;
			pJgInfo->m_xAngle.sMaterial = pBomJg->sMaterial;
			pJgInfo->m_xAngle.cQualityLevel = pBomJg->cQualityLevel;
			pJgInfo->RefreshAngleMaterial();
		}
	}
	DisplayCadProgress(100);
}
//��䵥Ԫ������
void CDwgFileInfo::DimGridData(AcDbBlockTableRecord *pBlockTableRecord, GEPOINT orgPt,
	GRID_DATA_STRU& grid_data, const char* sText)
{
	if (strlen(sText) <= 0 || pBlockTableRecord == NULL)
		return;
	GEPOINT dimPos;
	dimPos.x = (grid_data.max_x + grid_data.min_x)*0.5 + orgPt.x;
	dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
	AcDb::TextHorzMode hMode = AcDb::kTextCenter;
	AcDb::TextVertMode vMode = AcDb::kTextVertMid;
	if (grid_data.align_type > 0 && grid_data.align_type < 10)
	{	//���뷽ʽΪ1-9
		AcDb::TextHorzMode hModeArr[3] = { AcDb::kTextRight,AcDb::kTextLeft,AcDb::kTextCenter };
		AcDb::TextVertMode vModeArr[3] = { AcDb::kTextTop,AcDb::kTextVertMid,AcDb::kTextBottom };
		int iHIndex = grid_data.align_type % 3;
		int iVIndex = 0;
		if (grid_data.align_type == 4 || grid_data.align_type == 5 || grid_data.align_type == 6)
			iVIndex = 1;
		else if (grid_data.align_type == 7 || grid_data.align_type == 8 || grid_data.align_type == 9)
			iVIndex = 2;
		hMode = hModeArr[iHIndex];
		vMode = vModeArr[iVIndex];
		if (hMode == AcDb::kTextRight)
		{
			dimPos.x = grid_data.max_x + orgPt.x;
			dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
		}
		else if (hMode == AcDb::kTextLeft)
		{
			dimPos.x = grid_data.min_x + orgPt.x;
			dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
		}
	}
	DimText(pBlockTableRecord, dimPos, sText, TextStyleTable::hzfs.textStyleId, grid_data.fTextHigh, 0, hMode, vMode);
}
//����Ǹֹ����ӿ�
void CDwgFileInfo::InsertSubJgCard(CAngleProcessInfo* pJgInfo)
{
	CLockDocumentLife lockCurDocLife;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();//�������¼ָ��
	if (pBlockTableRecord == NULL || pJgInfo == NULL)
		return;
	GEPOINT org_pt = pJgInfo->GetJgCardPosL_B();
	char sSubJgCardPath[MAX_PATH] = "", APP_PATH[MAX_PATH] = "";
	GetAppPath(APP_PATH);
	sprintf(sSubJgCardPath, "%s�Ǹֹ��տ�\\SubJgCard.dwg", APP_PATH);
	AcDbDatabase blkDb(Adesk::kFalse);//����յ����ݿ�
#ifdef _ARX_2007
	if (blkDb.readDwgFile((ACHAR*)_bstr_t(sSubJgCardPath), _SH_DENYRW, true) == Acad::eOk)
#else
	if (blkDb.readDwgFile(sSubJgCardPath, _SH_DENYRW, true) == Acad::eOk)
#endif
	{
		AcDbBlockTable *pTempBlockTable = NULL;
		blkDb.getBlockTable(pTempBlockTable, AcDb::kForRead);
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		pTempBlockTable->getAt(ACDB_MODEL_SPACE, pTempBlockTableRecord, AcDb::kForWrite);
		pTempBlockTable->close();//�رտ��
		AcDbBlockTableRecordIterator *pIterator = NULL;
		pTempBlockTableRecord->newIterator(pIterator);
		for (; !pIterator->done(); pIterator->step())
		{
			AcDbEntity *pEnt = NULL;
			pIterator->getEntity(pEnt, AcDb::kForWrite);
			if (pEnt == NULL)
				continue;
			GRID_DATA_STRU grid_data;
			if (!pEnt->isKindOf(AcDbPoint::desc()) ||
				!GetGridKey((AcDbPoint*)pEnt, &grid_data) ||
				grid_data.data_type != 2)
			{
				pEnt->close();
				continue;
			}
			if (grid_data.type_id == ITEM_TYPE_SUM_PART_NUM)
			{	//�ӹ���
				CXhChar50 ss("%d", pJgInfo->m_xAngle.nSumPart);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_SUM_WEIGHT)
			{	//����
				CXhChar50 ss("%g", pJgInfo->m_xAngle.fSumWeight);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_LSSUM_NUM)
			{	//�ܿ���
				CXhChar50 ss("%d", pJgInfo->m_xAngle.nMSumLs);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PROCESS_NAME)
			{	//��������
				CXhChar100 ss=pJgInfo->GetJgProcessInfo();
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_TYPE)
			{	//����
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaType);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TOLERANCE)
			{	//���ϱ�׼
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sMatStandard);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TECH_REQ)
			{	//����Ҫ�������
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaAlias);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TASK_NO)
			{	//���񵥺�
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaskNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PRJ_NAME)
			{	//��������
				CXhChar50 ss;
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sPrjName);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_NUM)
			{	//����
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaNum);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_CODE_NO)
			{	//����(��ͬ��)
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sContractNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_DATE)
			{	//����
				CTime time = CTime::GetCurrentTime();
				CString ss = time.Format("%Y.%m.%d");
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else
				pEnt->close();
		}
		pTempBlockTableRecord->close();
	}
	//
	CXhChar50 sSubJgCard("SubJgCard");
	AcDbObjectId blockId = SearchBlock(sSubJgCard);
	if (blockId.isNull())
	{
		AcDbDatabase *pTempDb = NULL;
		blkDb.wblock(pTempDb);
#ifdef _ARX_2007
		GetCurDwg()->insert(blockId, _bstr_t(sSubJgCard), pTempDb);
#else
		GetCurDwg()->insert(blockId, sSubJgCard, pTempDb);
#endif
		delete pTempDb;
	}
	//�����տ������һ��ͼ��
	AcGeScale3d scaleXYZ(1.0, 1.0, 1.0);
	AcDbBlockReference *pBlkRef = new AcDbBlockReference;
	pBlkRef->setBlockTableRecord(blockId);
	pBlkRef->setPosition(AcGePoint3d(org_pt.x, org_pt.y, 0));
	pBlkRef->setRotation(0);
	pBlkRef->setScaleFactors(scaleXYZ);
	//��ͼ����ӵ������
	pBlockTableRecord->appendAcDbEntity(blockId, pBlkRef);
	pBlkRef->close();
	pBlockTableRecord->close();
}
//���ƽǸֹ��տ���Ϣ���ɶ����������ƣ�
void CDwgFileInfo::FillAngleDwgData()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���Ǹֹ��տ���Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		//
		InsertSubJgCard(pJgInfo);
	}
	DisplayCadProgress(100);
}
//��ȡ�Ǹֲ���
BOOL CDwgFileInfo::RetrieveAngles(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CAcModuleResourceOverride resOverride;
	//ѡ������ʵ��ͼԪ
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, bSupportSelectEnts ? FALSE : TRUE);
	//���ݽǸֹ��տ���ʶ��Ǹ�
	int index = 1, nNum = allEntIdSet.GetNodeNum()*2;
	DisplayCadProgress(0, "ʶ��Ǹֹ��տ�.....");
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
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
		if (!g_xUbomModel.IsJgCardBlockName(sName))
			continue;
		//��ӽǸּ�¼
		CAngleProcessInfo* pJgInfo = m_hashJgInfo.Add(entId.handle());
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
	}
	//����Ǹֹ��տ����������������"����"������ȡ�Ǹ���Ϣ
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<PART_LABEL_DIM> labelDimList;
	ATOM_LIST<CAD_ENTITY> cadTextList;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			PART_LABEL_DIM *pLabelDim = labelDimList.append();
			pLabelDim->m_xCirEnt.ciEntType = TYPE_CIRCLE;
			pLabelDim->m_xCirEnt.idCadEnt = pEnt->objectId().asOldId();
			pLabelDim->m_xCirEnt.m_fSize = pCir->radius();
			Cpy_Pnt(pLabelDim->m_xCirEnt.pos, pCir->center());
			continue;
		}
		if(!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()))
			continue;
		textIdHash.SetValue(entId.handle(), entId);	//��¼�Ǹֹ��տ���ʵʱ�ı�
		//���ݼ��Źؼ���ʶ��Ǹ�
		CXhChar100 sText = GetCadTextContent(pEnt);
		GEPOINT testPt = GetCadTextDimPos(pEnt);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText *pText = (AcDbText*)pEnt;
			CAD_ENTITY *pCadText = cadTextList.append();
			pCadText->ciEntType = TYPE_TEXT;
			pCadText->idCadEnt = entId.asOldId();
			pCadText->m_fSize = pText->height();
			pCadText->pos = testPt;
			strncpy(pCadText->sText, sText, 99);
		}
		if (!g_xUbomModel.IsPartLabelTitle(sText))
			continue;	//��Ч�ļ��ű���
		CAngleProcessInfo* pJgInfo = NULL;
		for (pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
		{
			if (pJgInfo->PtInAngleRgn(testPt))
				break;
		}
		if (pJgInfo == NULL)
			pJgInfo = m_hashJgInfo.Add(entId.handle());
		//���½Ǹ���Ϣ wht 20-07-29
		if(pJgInfo)
		{	//��ӽǸּ�¼
			//���ݹ��տ�ģ���м��ű�ǵ����ýǸֹ��տ���ԭ��λ��
			GEPOINT orig_pt = g_pncSysPara.GetJgCardOrigin(testPt);
			pJgInfo->keyId = entId;
			pJgInfo->SetOrig(orig_pt);
		}
	}
	DisplayCadProgress(100);
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//���ݽǸ�����λ�û�ȡ�Ǹ���Ϣ
	index = 1;
	nNum = textIdHash.GetNodeNum();
	DisplayCadProgress(0, "��ʼ���Ǹ���Ϣ.....");
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
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
			else if (cType == ITEM_TYPE_DES_MAT)
				pJgInfo->materialId = objId;
			else if (cType == ITEM_TYPE_DES_GUIGE)
				pJgInfo->specId = objId;
		}
	}
	DisplayCadProgress(100);
	//���ݺ����߰��ʼ���Ǹֺ������� wht 20-09-29
	//��ʼ�����ű�ע��������
	for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
	{
		CAD_ENTITY *pCadText = NULL;
		for (pCadText = cadTextList.GetFirst(); pCadText; pCadText = cadTextList.GetNext())
		{
			if (pLabelDim->m_xCirEnt.IsInScope(pCadText->pos))
			{
				pLabelDim->m_xInnerText.ciEntType = pCadText->ciEntType;
				pLabelDim->m_xInnerText.idCadEnt = pCadText->idCadEnt;
				pLabelDim->m_xInnerText.m_fSize = pCadText->m_fSize;
				pLabelDim->m_xInnerText.pos = pCadText->pos;
				strcpy(pLabelDim->m_xInnerText.sText, pCadText->sText);
				break;
			}
		}
		if (pCadText == NULL)	//δ�ҵ����ֵ�ԲȦ���Ƴ�
			labelDimList.DeleteCursor();
	}
	labelDimList.Clean();
	for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
	{
		SEGI segI = pLabelDim->GetSegI();
		if (segI.iSeg <= 0)
			continue;
		CAngleProcessInfo* pJgInfo = FindAngleByPt(pLabelDim->m_xCirEnt.pos);
		if (pJgInfo && !pJgInfo->m_xAngle.bWeldPart)
			pJgInfo->m_xAngle.bWeldPart = TRUE;
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