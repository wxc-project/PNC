#include "stdafx.h"
#include "XeroNcPart.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"
#include "DefCard.h"
#include "DimStyle.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
CAngleProcessInfo::CAngleProcessInfo()
{
	keyId = NULL;
	partNumId = NULL;
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
	if (rect.PtInRect(pt))
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
BYTE CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos, const char* sValue)
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
	else if (PtInDataRect(ITEM_TYPE_DES_GUIGE, data_pos))
	{	//��ƹ�񣬴�%ʱʹ��CXhCharTempl������ wht 20-08-05	
		cType = ITEM_TYPE_DES_GUIGE;
		BOOL bHasMultipleSign = FALSE;
		CString sSpec(sValue);	//CXhChar50 sSpec(sValue);
		if (strstr(sSpec, "��"))
			sSpec.Replace("��", "L");
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
				if (sKey)
					sSpec = sKey;
			}
			sprintf(m_xAngle.sSpec, "L%s", sSpec);
			m_xAngle.sSpec.Replace(" ", "");
			sSpec = m_xAngle.sSpec;
		}
		//�ӹ������ȡ���� wht 19-08-05
		double fWidth = 0, fThick = 0;
		CXhChar16 sMaterial;
		CProcessPart::RestoreSpec(sSpec, &fWidth, &fThick, sMaterial);
		if (sMaterial.GetLength() > 0)
		{
			sMaterial.Remove(' ');
			m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			m_xAngle.cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
			m_xAngle.sSpec.Replace(sMaterial, "");
			m_xAngle.sSpec.Remove(' ');
		}
		m_xAngle.wide = (float)fWidth;
		m_xAngle.thick = (float)fThick;
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
	else if (PtInDataRect(ITEM_TYPE_SUM_PART_NUM, data_pos))
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
	else if (PtInDataRect(ITEM_TYPE_LSSUM_NUM, data_pos))
	{	//��˨����
		cType = ITEM_TYPE_LSSUM_NUM;
		m_xAngle.nMSumLs = atoi(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))
	{	//��ע
		cType = ITEM_TYPE_PART_NOTES;
		strcpy(m_xAngle.sNotes, sValue);
		InitProcessInfo(sValue);
	}
	else if (PtInDataRect(ITEM_TYPE_MAKE_BEND, data_pos))
	{	//����
		cType = ITEM_TYPE_MAKE_BEND;
		m_xAngle.siZhiWan = (strlen(sValue) > 0)?1:0;
	}
	else if (PtInDataRect(ITEM_TYPE_WELD, data_pos))
	{	//����
		cType = ITEM_TYPE_WELD;
		m_xAngle.bWeldPart = strlen(sValue) > 0;
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_X, data_pos) ||
		PtInDataRect(ITEM_TYPE_CUT_ANGLE_S_Y, data_pos) ||
		PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_X, data_pos) ||
		PtInDataRect(ITEM_TYPE_CUT_ANGLE_E_Y, data_pos))
	{	//�н�|��֫
		cType = ITEM_TYPE_CUT_ANGLE_S_X;
		if (!m_xAngle.bCutAngle)
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
		if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:ѹ��Ҳ������������
			if (m_xAngle.nPushFlat > 0)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_KAIJIAO, data_pos))
	{	//����
		cType = ITEM_TYPE_KAIHE_JIAO;
		int nLen = strlen(sValue);
		if (nLen > 2)
		{
			CString sKaiJiao = sValue;
			BOOL bHasPlus = sKaiJiao.Find("+") >= 0;
			BOOL bHasMinus = sKaiJiao.Find("-") >= 0;
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
			if (bContinueRecoge)
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
			if (m_xAngle.bKaiJiao || m_xAngle.bHeJiao)
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
	else if (PtInDrawRect(data_pos))
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
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_SUM_PART_NUM);
	CXhChar16 sPartNum("%d", m_xAngle.nSumPart);
	if (partNumId == NULL)
	{	//��ӽǸּӹ���
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sPartNum, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull,
			RGB(255, 0, 0));
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹּӹ���
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, partNumId, AcDb::kForWrite);
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
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull,
			RGB(255, 0, 0));
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
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid, AcDbObjectId::kNull,
			RGB(255, 0, 0));
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
				sscanf(sValueG, "%c%lf%c%lf", &cMark1, &fWidth, &cMark2, &fThick);
			else if (sValueG.At(0) >= '0' && sValueG.At(0) <= '9')	//45*3	//45��3	//45 3 
				sscanf(sValueG, "%lf%c%lf", &fWidth, &cMark2, &fThick);

			CXhChar100 sFormat;
			if (cMark1 != '\0')
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

#endif