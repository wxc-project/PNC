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
	else if(data_type== ITEM_TYPE_CUT_ANGLE)
		rect = g_pncSysPara.cut_angle_rect;
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
static BYTE InitAnglePropByText(PART_ANGLE *pAngle, const char* sValue, 
								CAngleProcessInfo *pAngleInfo=NULL, f3dPoint *pTextPos=NULL)
{
	if (sValue == NULL)
		return 0;
	const int DIST_TILE_2_PROP = 30;
	BYTE cType = ITEM_TYPE_PART_NOTES;
	CXhChar200 sTemp(sValue);
	if (strstr(sValue, "���Ŷ�"))
	{
		pAngle->bHasFootNail = TRUE;
	}
	else if (strstr(sValue, "�Ŷ�"))
	{
		for (char *skey = strtok((char*)sTemp, " ,����"); skey; skey = strtok(NULL, " ,����"))
		{
			if (stricmp(skey, "�Ŷ�") == 0)
			{
				pAngle->bHasFootNail = TRUE;
				break;
			}
		}
	}
	if (strstr(sValue, "�н�") || strstr(sValue, "��֫"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_CUT_ANGLE);
			if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
				pAngle->bCutAngle = TRUE;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->bCutAngle = TRUE;
	}
	if (strstr(sValue, "���") || strstr(sValue, "�ٸ�") ||
		strstr(sValue, "��о") || strstr(sValue, "����") ||
		strstr(sValue, "�ٽ�"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_CUT_ROOT);
			if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
				pAngle->bCutRoot = TRUE;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->bCutRoot = TRUE;
	}
	if (strstr(sValue, "����"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_CUT_BER);
			if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
				pAngle->bCutBer = TRUE;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->bCutBer = TRUE;
	}
	if (strstr(sValue, "����"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_KAIJIAO);
			if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
				pAngle->bKaiJiao = TRUE;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->bKaiJiao = TRUE;
	}
	if (strstr(sValue, "�Ͻ�"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_HEJIAO);
			if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
				pAngle->bHeJiao = TRUE;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->bHeJiao = TRUE;
	}
	if (strstr(sValue, "ѹ��") || strstr(sValue, "���") || strstr(sValue, "�ı�"))
	{
		if (pAngleInfo && pTextPos)
		{
			f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_PUSH_FLAT);
			if(DISTANCE(*pTextPos,data_pt)> DIST_TILE_2_PROP)
				pAngle->nPushFlat = 0x01;
			//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
		}
		else
			pAngle->nPushFlat = 0x01;
	}
	if (strstr(sValue, "����") || strstr(sValue, "����") ||
		strstr(sValue, "����") || strstr(sValue, "����") ||
		strstr(sValue, "����") || strstr(sValue, "����")||
		strstr(sValue, "���߼н�")|| strstr(sValue, "֫��н�"))
	{
		pAngle->siZhiWan = 1;
	}
	if (strstr(sValue, "������") || strstr(sValue, "����") ||
		strstr(sValue, "����") || strstr(sValue, "#)") || 
		(strstr(sValue, "(") && strstr(sValue, ")") && strstr(sValue, "%d")==NULL)||
		(strstr(sValue, "��") && strstr(sValue, "��") && strstr(sValue, "%d") == NULL))	//"#)","("&&")","��"&&"��"��ʾ�к������� ��(245#) wht 20-07-29
	{
		const int MAX_PART_NO_LEN = 16;	//�˴�ֻ��������ַ��������ȹ���ʱ������Ϊ�߰���ű�ע wht 20-09-29
		if (strlen(sValue) < MAX_PART_NO_LEN)
		{
			if (pAngleInfo && pTextPos)
			{
				f3dPoint data_pt = pAngleInfo->GetAngleDataPos(ITEM_TYPE_WELD);
				if (DISTANCE(*pTextPos, data_pt) > DIST_TILE_2_PROP)
					pAngle->bWeldPart = TRUE;
				//else Ϊ�����ı���ĳЩ���Ա����ŵ���ͼ������ wht 20-07-30
			}
			else
				pAngle->bWeldPart = TRUE;
		}
	}
	return cType;
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
	{	//��%ʱʹ��CXhCharTempl������ wht 20-08-05	
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
		if (strstr(sValue, "x") || strstr(sValue, "X") || strstr(sValue, "*"))
		{	//֧����ȡ 1x4��ʽ�ļӹ��� wht 20-07-29
			if (strstr(sValue, "�ӹ���") || strstr(sValue, "����"))
				cType = 0;	//�к��ֵ����ֲ�����Ϊ�ӹ��������޸� wht 20-07-29
			else
				cType = ITEM_TYPE_SUM_PART_NUM;
			char *skey = strtok((char*)sValue, "x,X,*");
			if (skey != NULL)
			{
				int num1 = atoi(skey);
				skey = strtok(NULL, "x, X, *");
				int num2 = 1;
				if (skey)
					num2 = atoi(skey);
				if (num1 <= 0 && num2 > 0)
				{
					m_xAngle.feature2 = num2;	//��¼�ӹ�ϵ����ͨ���޸ļӹ���Ϊ"�ӹ���        x3"�޸ļӹ��� wht 20-07-29
					if (m_xAngle.feature1 > 0)
						m_xAngle.feature1 = m_xAngle.feature1*m_xAngle.feature2;
				}
				else 
					m_xAngle.feature1 = num1 * num2;
			}
		}
		else
		{
			m_xAngle.feature1 = atoi(sValue);
			if (m_xAngle.feature1 > 0 && m_xAngle.feature2 > 0)
				m_xAngle.feature1 = m_xAngle.feature1*m_xAngle.feature2;
			cType = ITEM_TYPE_SUM_PART_NUM;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))	//��ע
	{
		strcpy(m_xAngle.sNotes,sValue);
		InitAnglePropByText(&m_xAngle, sValue);
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
		cType = ITEM_TYPE_KAIJIAO;
		if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if(m_xAngle.bKaiJiao || m_xAngle.bHeJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_HEJIAO, data_pos))
	{
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
		cType = ITEM_TYPE_HEJIAO;
		if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
		{	//�е罨�ɶ���������Ҫ��:���Ͻ�Ҳ������������
			if (m_xAngle.bHeJiao || m_xAngle.bKaiJiao)
				m_xAngle.siZhiWan = 1;
		}
	}
	else if (PtInDataRect(ITEM_TYPE_CUT_ANGLE, data_pos))
	{
		m_xAngle.bCutAngle = strlen(sValue) > 0;
		cType = ITEM_TYPE_CUT_ANGLE;
	}
	else if(PtInDrawRect(data_pos))
	{	//�����ͼ�������������˵��
		InitAnglePropByText(&m_xAngle, sValue, this, &data_pos);
		cType = ITEM_TYPE_PART_NOTES;
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
	int index = 1, nNum = m_hashJgInfo.GetNodeNum();
	DisplayCadProgress(0, "���½Ǹּӹ���.....");
	CAngleProcessInfo* pJgInfo=NULL;
	BOMPART* pBomJg=NULL;
	BOOL bFinish=TRUE;
	for(pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pJgInfo->m_xAngle.sPartNo;
		pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pBomJg ==NULL)
		{	
			bFinish=FALSE;
			logerr.Log("TMA���ϱ���û��%s�Ǹ�",(char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.feature1 != pBomJg->feature1)
		{
			pJgInfo->m_xAngle.feature1 = pBomJg->feature1;	//�ӹ���
			pJgInfo->RefreshAngleNum();
		}
	}
	DisplayCadProgress(100);
	if(bFinish)
		AfxMessageBox("�Ǹּӹ����޸����!");
}
//���½Ǹֵ�����
void CDwgFileInfo::ModifyAngleDwgSingleNum()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum();
	DisplayCadProgress(0, "���½Ǹֵ�����.....");
	CAngleProcessInfo* pJgInfo = NULL;
	BOMPART* pBomJg = NULL;
	BOOL bFinish = TRUE;
	for (pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
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
	DisplayCadProgress(100);
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
		pJgInfo->CreateRgn();
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
			pJgInfo->CreateRgn();
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