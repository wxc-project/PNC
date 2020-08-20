#include "stdafx.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"
#include "LayerTable.h"
#include "XeroExtractor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPNCSysPara g_pncSysPara;
CSteelSealReactor *g_pSteelSealReactor = NULL;
const int HASHTABLESIZE=500;
const int STATUSHASHTABLESIZE=500;
//////////////////////////////////////////////////////////////////////////
//CDimRuleManager
CPNCSysPara::CPNCSysPara()
{
	Init();
}
RECOG_SCHEMA* CPNCSysPara::InsertRecogSchema(const char* name,int dimStyle,const char* partNoKey,
										     const char* matKey, const char* thickKey,const char* partCountKey/*=""*/,
											 const char* frontBendKey/*=""*/,const char* reverseBendKey/*=""*/,BOOL bEditable/*=FALSE*/)
{
	RECOG_SCHEMA *pSchema1 = g_pncSysPara.m_recogSchemaList.append();
	pSchema1->m_bEditable = bEditable;
	pSchema1->m_iDimStyle = dimStyle;
	pSchema1->m_bEnable = FALSE;
	if(name!=NULL)
		pSchema1->m_sSchemaName.Copy(name);
	if(partCountKey!=NULL)
		pSchema1->m_sPnKey.Copy(partNoKey);
	if(thickKey!=NULL)
		pSchema1->m_sThickKey.Copy(thickKey);
	if(matKey!=NULL)
		pSchema1->m_sMatKey.Copy(matKey);
	if(partCountKey!=NULL)
		pSchema1->m_sPnNumKey.Copy(partCountKey);
	if(frontBendKey!=NULL)
		pSchema1->m_sFrontBendKey.Copy(frontBendKey);
	if(reverseBendKey!=NULL)
		pSchema1->m_sReverseBendKey.Copy(reverseBendKey);
	return pSchema1;
}
void CPNCSysPara::Init()
{
	CPlateExtractor::Init();
	m_iPPiMode = 1;
	m_bIncDeformed = true;
	m_iAxisXCalType = 0;
	m_bUseMaxEdge = FALSE;
	m_nMaxEdgeLen = 2200;
	m_nMaxHoleD = 100;
	m_sJgCadName.Empty();
	m_bCmpQualityLevel = TRUE;
	m_bEqualH_h = FALSE;
	m_sPartLabelTitle.Empty();
	m_sJgCardBlockName.Empty();
	//�Զ��Ű�����
	m_ciLayoutMode = 1;
	m_ciArrangeType = 1;
	m_nMapWidth = 1500;
	m_nMapLength = 0;
	m_nMinDistance = 0;
	m_ciMKPos = 0;
	m_fMKHoleD = 10;
	m_nMkRectWidth = 30;
	m_nMkRectLen = 60;
	//ͼֽ��������
	m_fMapScale = 1;
	m_iLayerMode = 0;
	m_ciRecogMode = 0;
	m_ciBoltRecogMode = FILTER_PARTNO_CIR;
	m_fPartNoCirD = 0;
	m_ciProfileColorIndex = 1;		//��ɫ
	m_ciBendLineColorIndex = 0;		//����ɫ
	m_sProfileLineType.Copy("CONTINUOUS");
	m_fPixelScale = 0.6;
	m_fMaxLenErr = 0.5;
	//Ĭ����ɫ����
	crMode.crEdge = RGB(255, 0, 0);
	crMode.crLS12 = RGB(0, 255, 255);
	crMode.crLS16 = RGB(0, 255, 0);
	crMode.crLS20 = RGB(255, 0, 255);
	crMode.crLS24 = RGB(255, 255, 0);
	crMode.crOtherLS = RGB(255, 255, 255);
	//
	standard_hole.m_fLS12 = 13.5;
	standard_hole.m_fLS16 = 17.5;
	standard_hole.m_fLS20 = 21.5;
	standard_hole.m_fLS24 = 25.5;
	//Ĭ�ϼ�������ʶ������
	g_pncSysPara.m_recogSchemaList.Empty();
	RECOG_SCHEMA *pSchema = InsertRecogSchema("����1", 0, "#", "Q", "-");
	if (pSchema)
		pSchema->m_bEnable = TRUE;
	InsertRecogSchema("����2", 0, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����1", 1, "#", "Q", "-");
	InsertRecogSchema("����2", 1, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����5", 1, "����:", "����:", "���:");
	InsertRecogSchema("����6", 1, "����:", "����:", "���:", "����");
	InsertRecogSchema("����7", 1, "����:", "����:", "���:", "����", "����", "����");
	InsertRecogSchema("����8", 1, "����:", "����:", "���:", "����", "����", "����");
	
#ifdef __PNC_
	//Ĭ�Ϲ���ͼ����
	m_xHashDefaultFilterLayers.SetValue(LayerTable::UnvisibleProfileLayer.layerName, LAYER_ITEM(LayerTable::UnvisibleProfileLayer.layerName, 1));//���ɼ�������
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltSymbolLayer.layerName, LAYER_ITEM(LayerTable::BoltSymbolLayer.layerName, 1));		//��˨ͼ��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BendLineLayer.layerName, LAYER_ITEM(LayerTable::BendLineLayer.layerName, 1));		//�Ǹֻ������ְ����
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BreakLineLayer.layerName, LAYER_ITEM(LayerTable::BreakLineLayer.layerName, 1));		//�Ͽ�����
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DimTextLayer.layerName, LAYER_ITEM(LayerTable::DimTextLayer.layerName, 1));		//���ֱ�עͼ��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltLineLayer.layerName, LAYER_ITEM(LayerTable::BoltLineLayer.layerName, 1));		//��˨��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DamagedSymbolLine.layerName, LAYER_ITEM(LayerTable::DamagedSymbolLine.layerName, 1));	//�����������
	m_xHashDefaultFilterLayers.SetValue(LayerTable::CommonLayer.layerName, LAYER_ITEM(LayerTable::CommonLayer.layerName, 1));			//����
#endif
}
CPNCSysPara::~CPNCSysPara()
{
	hashBoltDList.Empty();
	m_xHashDefaultFilterLayers.Empty();
	m_xHashEdgeKeepLayers.Empty();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumFirst()
{
	if(m_iLayerMode==1)
		return m_xHashDefaultFilterLayers.GetFirst();
	else
		return m_xHashEdgeKeepLayers.GetFirst();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumNext()
{
	if(m_iLayerMode==1)
		return m_xHashDefaultFilterLayers.GetNext();
	else
		return m_xHashEdgeKeepLayers.GetNext();
}
IMPLEMENT_PROP_FUNC(CPNCSysPara);
void CPNCSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//��������
	AddPropItem("general_set",PROPLIST_ITEM(id++,"��������","��������"));
	AddPropItem("m_sJgCadName", PROPLIST_ITEM(id++,"�Ǹֹ��տ�","��ǰʹ�õĽǸֹ��տ�"));
	AddPropItem("m_fMaxLenErr", PROPLIST_ITEM(id++, "����������ֵ", "����У��ʱ�����ȱȽϵ�������ֵ"));
	AddPropItem("m_bIncDeformed",PROPLIST_ITEM(id++,"�ѿ��ǻ�������","����ȡ�ĸְ�ͼ���ѿ��ǻ���������","��|��"));
	AddPropItem("m_iPPiMode",PROPLIST_ITEM(id++,"�ļ�����ģ��","PPI�ļ�����ģʽ","0.һ��һ��|1.һ����"));
	AddPropItem("m_ciMKPos",PROPLIST_ITEM(id++,"��ӡʶ��ʽ","��ȡ��ӡλ��","0.�������ֱ�ע|1.��ӡ�ֺп�|2.��ӡ��λ��"));
	AddPropItem("m_fMKHoleD", PROPLIST_ITEM(id++, "��λ��ֱ��"));
	AddPropItem("m_nMaxHoleD", PROPLIST_ITEM(id++, "�����˨�׾�", "�����˨�׾�"));
	AddPropItem("AxisXCalType",PROPLIST_ITEM(id++,"X����㷽ʽ","�ӹ�����ϵX��ļ��㷽ʽ","0.�������|1.��˨ƽ�б�����|2.���ӱ�����"));
	AddPropItem("m_bUseMaxEdge", PROPLIST_ITEM(id++, "��������߳�", "���ڴ�����Ƭ��ȡ", "��|��"));
	AddPropItem("m_nMaxEdgeLen", PROPLIST_ITEM(id++, "���߳�", "������Ƭ��ȡ������߳�"));
	//ʶ��ģʽ
	AddPropItem("RecogMode",PROPLIST_ITEM(id++,"ʶ��ģʽ","ʶ��ģʽ"));
	AddPropItem("m_fMapScale", PROPLIST_ITEM(id++, "ʶ�����", "��ͼ���ű���"));
	AddPropItem("m_iRecogMode", PROPLIST_ITEM(id++, "������ʶ��", "�ְ�ʶ��ģʽ", "0.������ʶ��|1.��ͼ��ʶ��|2.����ɫʶ��|3.����ʶ��"));
	AddPropItem("m_iProfileLineTypeName", PROPLIST_ITEM(id++, "����������", "�ְ�������������", "CONTINUOUS|HIDDEN|DASHDOT2X|DIVIDE|ZIGZAG"));
	AddPropItem("m_iProfileColorIndex",PROPLIST_ITEM(id++,"��������ɫ","�ְ�����������ɫ"));
	AddPropItem("m_iBendLineColorIndex",PROPLIST_ITEM(id++,"��������ɫ","�ְ���������ɫ"));
	AddPropItem("layer_mode",PROPLIST_ITEM(id++,"ͼ�㴦��ʽ","������ͼ�㴦��ʽ","0.ָ��������ͼ��|1.����Ĭ��ͼ��"));
	AddPropItem("m_fPixelScale", PROPLIST_ITEM(id++, "�������ر���"));
	AddPropItem("FilterPartNoCir", PROPLIST_ITEM(id++, "����ר��ԲȦ", "", "����|������"));
	AddPropItem("m_fPartNoCirD", PROPLIST_ITEM(id++, "����ԲȦֱ��", ""));
	AddPropItem("RecogHoleDimText", PROPLIST_ITEM(id++, "�׾����ֱ�ע", "����׾���ע(����˵����ֱ����ע)", "����ע����|�����д���"));
	AddPropItem("RecogLsCircle", PROPLIST_ITEM(id++, "Բ��ʽ��˨", "��ͼ�����ԲȦ��ʾ����˨", "ͳһ��ʵ�ʿ׾�����|���ݱ�׼�׽���ɸѡ"));
	AddPropItem("RecogLsBlock", PROPLIST_ITEM(id++, "ͼ��ʽ��˨", "����˨ͼ�����ʾ����˨","��˨ͼ������"));
	AddPropItem("standardM12", PROPLIST_ITEM(id++, "M12��׼�׾�"));
	AddPropItem("standardM16", PROPLIST_ITEM(id++, "M16��׼�׾�"));
	AddPropItem("standardM20", PROPLIST_ITEM(id++, "M20��׼�׾�"));
	AddPropItem("standardM24", PROPLIST_ITEM(id++, "M24��׼�׾�"));
	//��ʾģʽ
	AddPropItem("DisplayMode", PROPLIST_ITEM(id++, "��ʾģʽ"));
	AddPropItem("m_ciLayoutMode", PROPLIST_ITEM(id++, "��ʾ����ģʽ","","ͼԪ��¡|�ְ�Ա�|�Զ��Ű�|����Ԥ��|ͼԪɸѡ"));
	AddPropItem("m_nMapWidth", PROPLIST_ITEM(id++, "ͼֽ���", "ͼֽ���"));
	AddPropItem("m_nMapLength", PROPLIST_ITEM(id++, "ͼֽ����", "ͼֽ����"));
	AddPropItem("m_nMinDistance", PROPLIST_ITEM(id++, "��С���", "ͼ��֮�����С���"));
	AddPropItem("CDrawDamBoard::BOARD_HEIGHT", PROPLIST_ITEM(id++, "����߶�", "����߶�"));
	AddPropItem("CDrawDamBoard::m_bDrawAllBamBoard", PROPLIST_ITEM(id++, "������ʾģʽ", "������ʾģʽ", "0.����ʾѡ�иְ嵵��|1.��ʾ���е���"));
	AddPropItem("m_nMkRectLen", PROPLIST_ITEM(id++, "��ӡ�ֺг���", "��ӡ�ֺп��"));
	AddPropItem("m_nMkRectWidth", PROPLIST_ITEM(id++, "��ӡ�ֺп��", "��ӡ�ֺп��"));
	AddPropItem("m_ciArrangeType", PROPLIST_ITEM(id++, "���ַ���", "","0.����Ϊ��|1.����Ϊ��"));
	AddPropItem("m_ciGroupType", PROPLIST_ITEM(id++, "���鷽��", "", "1.���κ�|2.����&���|3.����|4.���"));
	AddPropItem("crMode", PROPLIST_ITEM(id++, "��ɫ����"));
	AddPropItem("crMode.crEdge", PROPLIST_ITEM(id++, "��������ɫ"));
	AddPropItem("crMode.crLS12", PROPLIST_ITEM(id++, "M12�׾���ɫ"));
	AddPropItem("crMode.crLS16", PROPLIST_ITEM(id++, "M16�׾���ɫ"));
	AddPropItem("crMode.crLS20", PROPLIST_ITEM(id++, "M20�׾���ɫ"));
	AddPropItem("crMode.crLS24", PROPLIST_ITEM(id++, "M24�׾���ɫ"));
	AddPropItem("crMode.crOtherLS", PROPLIST_ITEM(id++, "�����׾���ɫ"));
}
int CPNCSysPara::GetPropValueStr(long id, char* valueStr, UINT nMaxStrBufLen/*=100*/, CPropTreeItem *pItem/*=NULL*/)
{
	CXhChar100 sText;
	if (GetPropID("m_bIncDeformed") == id)
	{
		if (g_pncSysPara.m_bIncDeformed)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_sJgCadName") == id)
		sText = m_sJgCadName;
	else if (GetPropID("m_sPartLabelTitle") == id)
		g_pncSysPara.m_sPartLabelTitle = valueStr;
	else if (GetPropID("m_sJgCardBlockName") == id)
		g_pncSysPara.m_sJgCardBlockName = valueStr;
	else if (GetPropID("m_fMaxLenErr") == id)
		sText.Printf("%.1f", g_pncSysPara.m_fMaxLenErr);
	else if (GetPropID("m_bUseMaxEdge") == id)
	{
		if (g_pncSysPara.m_bUseMaxEdge)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_nMaxEdgeLen") == id)
		sText.Printf("%d", g_pncSysPara.m_nMaxEdgeLen);
	else if (GetPropID("m_iPPiMode") == id)
	{
		if (g_pncSysPara.m_iPPiMode == 0)
			sText.Copy("0.һ��һ��");
		else if (g_pncSysPara.m_iPPiMode == 1)
			sText.Copy("1.һ����");
	}
	else if (GetPropID("AxisXCalType") == id)
	{
		if (g_pncSysPara.m_iAxisXCalType == 0)
			sText.Copy("0.�������");
		else if (g_pncSysPara.m_iAxisXCalType == 1)
			sText.Copy("1.��˨ƽ�б�����");
		else if (g_pncSysPara.m_iAxisXCalType == 2)
			sText.Copy("2.���ӱ�����");
	}
	else if (GetPropID("m_ciMKPos") == id)
	{
		if (g_pncSysPara.m_ciMKPos == 1)
			sText.Copy("1.��ӡ�ֺп�");
		else if (g_pncSysPara.m_ciMKPos == 2)
			sText.Copy("2.��ӡ��λ��");
		else
			sText.Copy("0.�������ֱ�ע");
	}
	else if (GetPropID("m_fMKHoleD") == id)
		sText.Printf("%g", m_fMKHoleD);
	else if (GetPropID("m_nMaxHoleD") == id)
		sText.Printf("%d", g_pncSysPara.m_nMaxHoleD);
	else if (GetPropID("m_fMapScale") == id)
		sText.Printf("%.f", g_pncSysPara.m_fMapScale);
	else if (GetPropID("m_ciLayoutMode") == id)
	{
		if (g_pncSysPara.m_ciLayoutMode == 1)
			sText.Copy("�ְ�Ա�");
		else if (g_pncSysPara.m_ciLayoutMode == 2)
			sText.Copy("�Զ��Ű�");
		else if (g_pncSysPara.m_ciLayoutMode == 3)
			sText.Copy("����Ԥ��");
		else if (g_pncSysPara.m_ciLayoutMode == 4)
			sText.Copy("ͼԪɸѡ");
		else
			sText.Copy("ͼԪ��¡");
	}
	else if (GetPropID("m_ciArrangeType") == id)
	{
		if (g_pncSysPara.m_ciArrangeType == 0)
			sText.Copy("0.����Ϊ��");
		else
			sText.Copy("1.����Ϊ��");
	}
	else if (GetPropID("m_ciGroupType") == id)
	{
		if (g_pncSysPara.m_ciGroupType == 1)
			sText.Copy("1.���κ�");
		else if (g_pncSysPara.m_ciGroupType == 2)
			sText.Copy("2.����&���");
		else if (g_pncSysPara.m_ciGroupType == 3)
			sText.Copy("3.����");
		else if (g_pncSysPara.m_ciGroupType == 4)
			sText.Copy("4.���");
		else
			sText.Copy("0.������");
	}
	else if (GetPropID("m_nMapLength") == id)
		sText.Printf("%d", g_pncSysPara.m_nMapLength);
	else if (GetPropID("m_nMapWidth") == id)
		sText.Printf("%d", g_pncSysPara.m_nMapWidth);
	else if (GetPropID("m_nMinDistance") == id)
		sText.Printf("%d", g_pncSysPara.m_nMinDistance);
#ifndef __UBOM_ONLY_ 
	else if (GetPropID("CDrawDamBoard::m_bDrawAllBamBoard") == id)
	{
		if (CDrawDamBoard::m_bDrawAllBamBoard)
			sText.Copy("1.��ʾ���е���");
		else
			sText.Copy("0.����ʾѡ�иְ嵵��");
	}
	else if (GetPropID("CDrawDamBoard::BOARD_HEIGHT") == id)
		sText.Printf("%d", CDrawDamBoard::BOARD_HEIGHT);
#endif
	else if (GetPropID("m_nMkRectWidth") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectWidth);
	else if (GetPropID("m_nMkRectLen") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectLen);
	else if (GetPropID("layer_mode") == id)
	{
		if (m_iLayerMode == 0)
			sText.Copy("0.ָ��������ͼ��");
		else if (m_iLayerMode == 1)
			sText.Copy("1.����Ĭ��ͼ��");
	}
	else if (GetPropID("m_iRecogMode") == id)
	{
		if (m_ciRecogMode == FILTER_BY_LINETYPE)
			sText.Copy("0.������ʶ��");
		else if (m_ciRecogMode == FILTER_BY_LAYER)
			sText.Copy("1.��ͼ��ʶ��");
		else if (m_ciRecogMode == FILTER_BY_COLOR)
			sText.Copy("2.����ɫʶ��");
		else if (m_ciRecogMode == FILTER_BY_PIXEL)
			sText.Copy("3.����ʶ��");
	}
	else if (GetPropID("m_fPixelScale") == id)
	{
		sText.Printf("%f", g_pncSysPara.m_fPixelScale);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("FilterPartNoCir") == id)
	{
		if (IsFilterPartNoCir())
			sText.Copy("����");
		else
			sText.Copy("������");
	}
	else if (GetPropID("m_fPartNoCirD") == id)
		sText.Printf("%g", m_fPartNoCirD);
	else if (GetPropID("standardM12") == id)
		sText.Printf("%g", standard_hole.m_fLS12);
	else if (GetPropID("standardM16") == id)
		sText.Printf("%g", standard_hole.m_fLS16);
	else if (GetPropID("standardM20") == id)
		sText.Printf("%g", standard_hole.m_fLS20);
	else if (GetPropID("standardM24") == id)
		sText.Printf("%g", standard_hole.m_fLS24);
	else if (GetPropID("RecogLsBlock") == id)
		sText.Copy("����ͼ�����ô���");
	else if (GetPropID("RecogHoleDimText") == id)
	{
		if (IsRecogHoleDimText())
			sText.Copy("����ע����");
		else
			sText.Copy("�����д���");
	}
	else if (GetPropID("RecogLsCircle") == id)
	{
		if (IsRecogCirByBoltD())
			sText.Copy("���ݱ�׼�׽���ɸѡ");
		else
			sText.Copy("ͳһ������״���");
	}
	else if (GetPropID("m_iProfileColorIndex") == id)
		sText.Printf("RGB%X", GetColorFromIndex(m_ciProfileColorIndex));
	else if (GetPropID("m_iBendLineColorIndex") == id)
		sText.Printf("RGB%X", GetColorFromIndex(m_ciBendLineColorIndex));
	else if (GetPropID("m_iProfileLineTypeName") == id)
		sText.Copy(m_sProfileLineType);
	else if (GetPropID("crMode.crLS12") == id)
		sText.Printf("RGB%X", crMode.crLS12);
	else if (GetPropID("crMode.crLS16") == id)
		sText.Printf("RGB%X", crMode.crLS16);
	else if (GetPropID("crMode.crLS20") == id)
		sText.Printf("RGB%X", crMode.crLS20);
	else if (GetPropID("crMode.crLS24") == id)
		sText.Printf("RGB%X", crMode.crLS24);
	else if (GetPropID("crMode.crEdge") == id)
		sText.Printf("RGB%X", crMode.crEdge);
	if (valueStr)
		StrCopy(valueStr, sText, nMaxStrBufLen);
	return strlen(sText);
}

BOOL CPNCSysPara::IsNeedFilterLayer(const char* sLayer)
{
	if(m_ciRecogMode==FILTER_BY_LAYER)
	{
		if(m_iLayerMode==0&&m_xHashEdgeKeepLayers.GetNodeNum()>0)
		{	//�û�ָ������������ͼ��
			LAYER_ITEM* pItem=GetEdgeLayerItem(sLayer);
			if(pItem && pItem->m_bMark)
				return FALSE;
			else
				return TRUE;
		}
		else if(m_iLayerMode==1&&m_xHashDefaultFilterLayers.GetValue(sLayer))
			return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::IsProfileEnt(AcDbEntity* pEnt)
{
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbLine::desc()))
		return TRUE;	//ֱ��
	if (pEnt->isKindOf(AcDbArc::desc()))
		return TRUE;	//Բ��
	if (pEnt->isKindOf(AcDbEllipse::desc()))
		return TRUE;	//��Բ��
	if (pEnt->isKindOf(AcDbPolyline::desc()))
		return TRUE;	//�����а���������
	if (pEnt->isKindOf(AcDbRegion::desc()))
		return TRUE;	//���߶�
	if (pEnt->isKindOf(AcDbCircle::desc()))
	{	//������ȡʱ������ԭ��
		if (m_ciRecogMode != CPNCSysPara::FILTER_BY_PIXEL)
			return FALSE;
		AcDbCircle* pCir = (AcDbCircle*)pEnt;
		if (pCir->radius() * 2 > m_nMaxHoleD)
			return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=CPlateExtractor::IsBendLine(pAcDbLine,pRecognizer);
	if (!bRet && m_ciBendLineColorIndex > 0 && m_ciBendLineColorIndex < 255)
		bRet = (GetEntColorIndex(pAcDbLine) == m_ciBendLineColorIndex);
	return bRet;
}

BOOL CPNCSysPara::RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum)
{
	if(m_ciMKPos==0)
		return FALSE;	//����Ҫ��ȡ��ӡ��
	if (m_ciMKPos == 1)
	{	//��ӡ���ο�
		if (pEnt->isKindOf(AcDbText::desc()))
		{	//��ȡ��ӡ��
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			CXhChar50 sText(_bstr_t(pText->textString()));
#else
			CXhChar50 sText(pText->textString());
#endif
			if (!sText.Equal("��ӡ��"))
				return FALSE;
			double len = DrawTextLength(sText, pText->height(), pText->textStyle());
			f3dPoint dim_vec(cos(pText->rotation()), sin(pText->rotation()));
			f3dPoint origin(pText->position().x, pText->position().y, pText->position().z);
			origin += dim_vec * len*0.5;
			f2dRect rect;
			rect.topLeft.Set(origin.x - 10, origin.y + 10);
			rect.bottomRight.Set(origin.x + 10, origin.y - 10);
			ZoomAcadView(rect, 200);		//�Ը�ӡ�������ʵ�����
			ads_name seqent;
			AcDbObjectId initLastObjId, plineId;
			acdbEntLast(seqent);
			acdbGetObjectId(initLastObjId, seqent);
			ads_point base_pnt;
			base_pnt[X] = origin.x;
			base_pnt[Y] = origin.y;
			base_pnt[Z] = origin.z;
#ifdef _ARX_2007
			int resCode = acedCommand(RTSTR, L"-boundary", RTSTR, L"a", RTSTR, L"i", RTSTR, L"n", RTSTR, L"", RTSTR, L"", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
#else
			int resCode = acedCommand(RTSTR, "-boundary", RTSTR, "a", RTSTR, "i", RTSTR, "n", RTSTR, "", RTSTR, "", RTPOINT, base_pnt, RTSTR, "", RTNONE);
#endif		
			if (resCode != RTNORM)
				return FALSE;
			acdbEntLast(seqent);
			acdbGetObjectId(plineId, seqent);
			if (initLastObjId == plineId)
				return FALSE;
			AcDbEntity *pEnt = NULL;
			acdbOpenAcDbEntity(pEnt, plineId, AcDb::kForWrite);
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			if (pPline == NULL || pPline->numVerts() != nNum)
			{
				if (pPline)
				{
					pPline->erase(Adesk::kTrue);
					pPline->close();
				}
				return FALSE;
			}
			AcGePoint3d location;
			for (int iVertIndex = 0; iVertIndex < nNum; iVertIndex++)
			{
				pPline->getPointAt(iVertIndex, location);
				ptArr[iVertIndex].Set(location.x, location.y, location.z);
			}
			pPline->erase(Adesk::kTrue);	//ɾ��polyline����
			pPline->close();
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//��ӡ��ͼ��
			AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
			AcDbObjectId blockId = pReference->blockTableRecord();
			AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
			acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
			if (pTempBlockTableRecord == NULL)
				return FALSE;
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
			if (!sName.Equal("MK"))
				return FALSE;
			double rot_angle = pReference->rotation();
			f3dPoint orig(pReference->position().x, pReference->position().y, 0);
			AcGeScale3d scaleXYZ = pReference->scaleFactors();
			AcDbBlockTableRecordIterator *pIterator = NULL;
			pTempBlockTableRecord->newIterator(pIterator);
			for (; !pIterator->done(); pIterator->step())
			{
				pIterator->getEntity(pEnt, AcDb::kForRead);
				CAcDbObjLife entObj(pEnt);
				if (pEnt->isKindOf(AcDbPolyline::desc()))
				{
					AcGePoint3d location;
					AcDbPolyline* pPolyLine = (AcDbPolyline*)pEnt;
					for (int iVertIndex = 0; iVertIndex < nNum; iVertIndex++)
					{
						pPolyLine->getPointAt(iVertIndex, location);
						ptArr[iVertIndex].Set(location.x, location.y, location.z);
						ptArr[iVertIndex].x *= scaleXYZ.sx;
						ptArr[iVertIndex].y *= scaleXYZ.sy;
						ptArr[iVertIndex].z *= scaleXYZ.sz;
					}
					break;
				}
			}
			pTempBlockTableRecord->close();
			//���¸�ӡ��ʵ������
			for (int i = 0; i < nNum; i++)
			{
				if (fabs(rot_angle) > 0)	//ͼ������ת�Ƕ�
					rotate_point_around_axis(ptArr[i], rot_angle, f3dPoint(), 100 * f3dPoint(0, 0, 1));
				ptArr[i] += orig;
			}
		}
		else
			return FALSE;
	}
	else if (m_ciMKPos == 2)
	{	//��λ��
		if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle* pCircle = (AcDbCircle*)pEnt;
			double fRidius = pCircle->radius();
			if (fRidius * 2 != g_pncSysPara.m_fMKHoleD)
				return FALSE;
			AcGePoint3d center = pCircle->center();
			ptArr[0].Set(center.x + fRidius, center.y, 0);
			ptArr[1].Set(center.x - fRidius, center.y, 0);
			ptArr[2].Set(center.x, center.y + fRidius, 0);
			ptArr[3].Set(center.x, center.y - fRidius, 0);
		}
		else
			return FALSE;
	}
	return TRUE;
}
BOOL CPNCSysPara::IsJgCardBlockName(const char* sBlockName)
{
	if (sBlockName != NULL && strcmp(sBlockName, "JgCard") == 0)
		return TRUE;
	else if (m_sJgCardBlockName.GetLength() > 0 && m_sJgCardBlockName.EqualNoCase(sBlockName))
		return TRUE;
	else
		return false;
}
BOOL CPNCSysPara::IsPartLabelTitle(const char* sText)
{
	if (m_sPartLabelTitle.GetLength() > 0)
	{
		CString ss1(sText), ss2(m_sPartLabelTitle);
		ss1 = ss1.Trim();
		ss2 = ss2.Trim();
		ss1.Remove(' ');
		ss2.Remove(' ');
		if (ss2.GetLength()>1 && ss1.CompareNoCase(ss2) == 0)
			return true;
		else
			return false;
	}
	else
	{
		if (!(strstr(sText, "��") && strstr(sText, "��")) &&	//���š�������
			!(strstr(sText, "��") && strstr(sText, "��")))		//��š��������
			return false;
		if (strstr(sText, "�ļ�") != NULL)
			return false;	//�ų�"�ļ����"���µ���ȡ���� wht 19-05-13
		if(strstr(sText,":")!=NULL || strstr(sText, "��") != NULL)
			return false;	//�ų��ְ��ע�еļ��ţ����⽫�ְ������ȡΪ�Ǹ� wht 20-07-29
		return true;
	}
}
//////////////////////////////////////////////////////////////////////////
//��ȡUBOM�ĸ�����Ϣ�����浽rule.set
void PNCSysSetImportAttach()
{
	char file_name[MAX_PATH] = "", line_txt[MAX_PATH] = "", key_word[100] = "";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText, line_txt);
		CString sLine(line_txt);
		sLine.Replace('=', ' ');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		if (_stricmp(key_word, "PartLabelTitle") == 0)
		{
			skey = strtok(NULL, "=,;");
			CXhChar16 sPartLabelTitle;
			sprintf(sPartLabelTitle, "%s", skey);
			sPartLabelTitle.Replace(" ", "");
			sPartLabelTitle.Replace("\n", "");
			if (sPartLabelTitle.GetLength() > 0)	//�������������ļ��еĲ��� wht 20-08-19
				g_pncSysPara.m_sPartLabelTitle.Copy(sPartLabelTitle);
		}
		else if (_stricmp(key_word, "MaxLenErr") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.m_fMaxLenErr = atof(skey);
		}
	}
	fclose(fp);
}
void PNCSysSetImportDefault()
{
	char file_name[MAX_PATH]="", line_txt[MAX_PATH]="", key_word[100]="";
	GetAppPath(file_name);
#ifndef __UBOM_ONLY_
	strcat(file_name, "rule.set");
#else
	strcat(file_name, "ubom.cfg");
#endif
	FILE *fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
	int nTemp = 0;
	g_pncSysPara.hashBoltDList.Empty();
	g_pncSysPara.m_recogSchemaList.Empty();
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText, line_txt);
		CString sLine(line_txt);
		sLine.Replace('=', ' ');
		sLine.Replace('\n', ' ');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		//��������
		if (_stricmp(key_word, "DimStyle") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iDimStyle);
		else if (_stricmp(key_word, "PnKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sPnKey, "%s", skey);
			g_pncSysPara.m_sPnKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "ThickKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sThickKey, "%s", skey);
			g_pncSysPara.m_sThickKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "MatKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sMatKey, "%s", skey);
			g_pncSysPara.m_sMatKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "PnNumKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sPnNumKey, "%s", skey);
			g_pncSysPara.m_sPnNumKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "MKPos") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_ciMKPos);
		else if (_stricmp(key_word, "MKHole") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.m_fMKHoleD = atof(skey);
		}
		else if (_stricmp(key_word, "MapScale") == 0)
			sscanf(line_txt, "%s%f", key_word, &g_pncSysPara.m_fMapScale);
		else if (_stricmp(key_word, "bIncFilterLayer") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iLayerMode);
		else if (_stricmp(key_word, "RecogMode") == 0)
		{	//ʹ��%d��ȡBYTE�ͱ�����һ�θ���4���ֽ�Ӱ����������ȡֵ
			//��ʹ��%hhu��ȡBYTE������������Ч���ȶ�ȡ����ʱ�����У��ٸ�ֵ����Ӧ������ wht 19-10-05
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciRecogMode = nTemp;
		}
		else if (_stricmp(key_word, "BoltRecogMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBoltRecogMode = nTemp;
		}
		else if (_stricmp(key_word, "PartNoCirD") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey)
				g_pncSysPara.m_fPartNoCirD = atof(skey);
		}
		else if (_stricmp(key_word, "BendLineColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBendLineColorIndex = nTemp;
		}
		else if (_stricmp(key_word, "ProfileColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciProfileColorIndex = nTemp;
		}
		else if (_stricmp(key_word, "ProfileLineType") == 0)
			sscanf(line_txt, "%s%s", key_word, &g_pncSysPara.m_sProfileLineType);
		else if (_stricmp(key_word, "PixelScale") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey)
				g_pncSysPara.m_fPixelScale = atof(skey);
		}
		else if (_stricmp(key_word, "BoltDKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			CString skeyName(skey);
			if (strlen(skey) > 0)
			{
				BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.Add(skey);
				fgets(line_txt, MAX_PATH, fp);
				skey = strtok(line_txt, ";");
				if (!strcmp(skeyName, skey))
				{
					pBoltD->sGroupName.Printf(" ");
					pBoltD->sBlockName.Printf("%s", skey);
				}
				else
				{
					pBoltD->sGroupName.Printf("%s", skey);
					skey = strtok(NULL, ";");
					pBoltD->sBlockName.Printf("%s", skey);
				}
				skey = strtok(NULL, ";");
				pBoltD->diameter = atoi(skey);
				skey = strtok(NULL, ";");
				if (skey != NULL)
					pBoltD->hole_d = atof(skey);
			}
		}
		else if (_stricmp(key_word, "m_iDimStyle") == 0)
		{
			skey = strtok(NULL, ",;");
			if (strlen(skey) > 0)
			{
				RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.append();
				pSchema->m_iDimStyle = atoi(skey);
				skey = strtok(line_txt, ";");
				skey = strtok(NULL, ";");
				pSchema->m_sSchemaName = skey;
				pSchema->m_sSchemaName.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sPnKey = skey;
				pSchema->m_sPnKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sThickKey = skey;
				pSchema->m_sThickKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sMatKey = skey;
				pSchema->m_sMatKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sPnNumKey = skey;
				pSchema->m_sPnNumKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sFrontBendKey = skey;
				pSchema->m_sFrontBendKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sReverseBendKey = skey;
				pSchema->m_sReverseBendKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_bEditable = atoi(skey);
				skey = strtok(NULL, ";");
				pSchema->m_bEnable = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "bIncDeformed") == 0)
		{
			CXhChar16 sText;
			sscanf(line_txt, "%s%s", key_word, (char*)sText);
			if (sText.Equal("��"))
				g_pncSysPara.m_bIncDeformed = true;
			else
				g_pncSysPara.m_bIncDeformed = false;
		}
		else if (_stricmp(key_word, "PPIMode") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iPPiMode);
		else if (_stricmp(key_word, "LayoutMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciLayoutMode = nTemp;
		}
		else if (_stricmp(key_word, "ArrangeType") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciArrangeType = nTemp;
		}
		else if (_stricmp(key_word, "GroupType") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciGroupType = nTemp;
		}
		else if (_stricmp(key_word, "MapLength") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapLength);
		else if (_stricmp(key_word, "MapWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapWidth);
		else if (_stricmp(key_word, "MinDistance") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMinDistance);
		else if (_stricmp(key_word, "AxisXCalType") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iAxisXCalType);
		else if (_stricmp(key_word, "m_nMkRectWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectWidth);
		else if (_stricmp(key_word, "m_nMkRectLen") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectLen);
		else if (_stricmp(key_word, "m_nMaxEdgeLen") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMaxEdgeLen);
		else if (_stricmp(key_word, "m_nMaxHoleD") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMaxHoleD);
		else if (_stricmp(key_word, "JG_CARD") == 0)
		{
			sscanf(line_txt, "%s%s", key_word, (char*)g_pncSysPara.m_sJgCadName);
			g_pncSysPara.m_sJgCadName.Replace(" ", "");
			g_pncSysPara.m_sJgCadName.Replace("\n", "");
		}
		else if (_stricmp(key_word, "PartLabelTitle") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sPartLabelTitle, "%s", skey);
			g_pncSysPara.m_sPartLabelTitle.Replace(" ", "");
			g_pncSysPara.m_sPartLabelTitle.Replace("\n", "");
		}
		else if (_stricmp(key_word, "JgCardBlockName") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sJgCardBlockName, "%s", skey);
			g_pncSysPara.m_sJgCardBlockName.Replace(" ", "");
			g_pncSysPara.m_sJgCardBlockName.Replace("\n", "");
		}
#ifndef __UBOM_ONLY_
		else if (_stricmp(key_word, "CDrawDamBoard::BOARD_HEIGHT") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::BOARD_HEIGHT);
		else if (_stricmp(key_word, "CDrawDamBoard::m_bDrawAllBamBoard") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	}
	fclose(fp);
	//���������ļ��󼤻ǰʶ��ģ�� wht 19-10-30
	if (g_pncSysPara.m_recogSchemaList.GetNodeNum() <= 0)
	{
		RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.append();
		pSchema->m_iDimStyle = g_pncSysPara.m_iDimStyle;
		pSchema->m_sPnKey.Copy(g_pncSysPara.m_sPnKey);
		pSchema->m_sThickKey.Copy(g_pncSysPara.m_sThickKey);
		pSchema->m_sMatKey.Copy(g_pncSysPara.m_sMatKey);
		pSchema->m_sPnNumKey.Copy(g_pncSysPara.m_sPnNumKey);
		pSchema->m_bEnable = TRUE;
	}
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		if (pSchema->m_bEnable)
		{
			g_pncSysPara.ActiveRecogSchema(pSchema);
			break;
		}
	}
#if defined(__UBOM_ONLY_)
	//��ȡUBOM���õĸ�����Ϣ���û��޸ĺ��ŵ�rule.set
	PNCSysSetImportAttach();
#endif
}
void PNCSysSetExportDefault()
{
	char file_name[MAX_PATH]="";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("�򲻿�ָ���������ļ�!");
		return;
	}
	fprintf(fp, "��������\n");
	fprintf(fp, "JG_CARD=%s\n", (char*)g_pncSysPara.m_sJgCadName);
	fprintf(fp, "MaxLenErr=%.1f ;�Ǹֳ������\n", g_pncSysPara.m_fMaxLenErr);
	fprintf(fp, "PartLabelTitle=%s ;���ű���\n", (char*)g_pncSysPara.m_sPartLabelTitle);
	fprintf(fp, "JgCardBlockName=%s ;�Ǹֹ��տ�������\n", (char*)g_pncSysPara.m_sJgCardBlockName);
	fprintf(fp, "bIncDeformed=%s ;���ǻ���������\n", g_pncSysPara.m_bIncDeformed ? "��" : "��");
	fprintf(fp, "PPIMode=%d ;PPI�ļ�ģʽ\n", g_pncSysPara.m_iPPiMode);
	fprintf(fp, "LayoutMode=%d ;��ʾģʽ\n", g_pncSysPara.m_ciLayoutMode);
	fprintf(fp, "MapWidth=%d ;ͼֽ���\n", g_pncSysPara.m_nMapWidth);
	fprintf(fp, "MapLength=%d ;ͼֽ����\n", g_pncSysPara.m_nMapLength);
	fprintf(fp, "MinDistance=%d ;��С���\n", g_pncSysPara.m_nMinDistance);
	fprintf(fp, "ArrangeType=%d ;�ԱȲ��ַ���\n", (char*)g_pncSysPara.m_ciArrangeType);
	fprintf(fp, "GroupType=%d ;���Ʒ��鷽ʽ\n", (char*)g_pncSysPara.m_ciGroupType);
	fprintf(fp, "MKPos=%d ;��ȡ��ӡλ��\n", (char*)g_pncSysPara.m_ciMKPos);
	fprintf(fp, "MKHole=%g ;��λ��ֱ��\n", g_pncSysPara.m_fMKHoleD);
	fprintf(fp, "AxisXCalType=%d ;X����㷽ʽ\n", (char*)g_pncSysPara.m_iAxisXCalType);
	fprintf(fp, "m_nMkRectWidth=%d ;�ֺп��\n", g_pncSysPara.m_nMkRectWidth);
	fprintf(fp, "m_nMkRectLen=%d ;�ֺг���\n", g_pncSysPara.m_nMkRectLen);
	fprintf(fp, "m_nMaxEdgeLen=%d ;��߳�\n", g_pncSysPara.m_nMaxEdgeLen);
	fprintf(fp, "m_nMaxHoleD=%d ;�����˨��\n", g_pncSysPara.m_nMaxHoleD);
#ifndef __UBOM_ONLY_
	fprintf(fp, "CDrawDamBoard::BOARD_HEIGHT=%d ;����߶�\n", CDrawDamBoard::BOARD_HEIGHT);
	fprintf(fp, "CDrawDamBoard::m_bDrawAllBamBoard=%d ;�������е���\n", CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	fprintf(fp, "ͼ������\n");
	fprintf(fp, "bIncFilterLayer=%d ;���ù���Ĭ��ͼ��\n", g_pncSysPara.m_iLayerMode);
	fprintf(fp, "RecogMode=%d ;ʶ��ģʽ\n", g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "BoltRecogMode=%d ;��˨ʶ��ģʽ\n", g_pncSysPara.m_ciBoltRecogMode);
	fprintf(fp, "PartNoCirD=%.1f ;����ԲȦֱ��\n", g_pncSysPara.m_fPartNoCirD);
	fprintf(fp, "ProfileColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciProfileColorIndex);
	fprintf(fp, "BendLineColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciBendLineColorIndex);
	fprintf(fp, "ProfileLineType=%s ;����������\n", (char*)g_pncSysPara.m_sProfileLineType);
	fprintf(fp, "PixelScale=%.1f ;����������\n", g_pncSysPara.m_fPixelScale);
	fprintf(fp, "����ʶ������\n");
	fprintf(fp, "DimStyle=%d ;���ֱ�ע����\n", g_pncSysPara.m_iDimStyle);
	fprintf(fp, "PnKey=%s ;���ű�ʶ��\n", (char*)g_pncSysPara.m_sPnKey);
	fprintf(fp, "ThickKey=%s ;��ȱ�ʶ��\n", (char*)g_pncSysPara.m_sThickKey);
	fprintf(fp, "MatKey=%s ;���ʱ�ʶ��\n", (char*)g_pncSysPara.m_sMatKey);
	fprintf(fp, "PnNumKey=%s ;������ʶ��\n", (char*)g_pncSysPara.m_sPnNumKey);
	fprintf(fp, "��˨ʶ������\n");
	for (BOLT_BLOCK *pBoltBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBoltBlock; pBoltBlock = g_pncSysPara.hashBoltDList.GetNext())
	{
		fprintf(fp, "BoltDKey=%s;ͼ������;��˨ֱ��\n", g_pncSysPara.hashBoltDList.GetCursorKey());
		if (!strcmp(pBoltBlock->sGroupName, ""))
			fprintf(fp, "%s;", " ");
		else
			fprintf(fp, "%s;", (char*)pBoltBlock->sGroupName);
		fprintf(fp, "%s;", (char*)pBoltBlock->sBlockName);
		fprintf(fp, "%d;", pBoltBlock->diameter);
		fprintf(fp, "%.1f\n", pBoltBlock->hole_d);
	}
	fprintf(fp, "����ʶ������\n");
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		fprintf(fp, "m_iDimStyle=%d;", pSchema->m_iDimStyle);
		fprintf(fp, " %s;", (char*)pSchema->m_sSchemaName);
		fprintf(fp, " %s;", (char*)pSchema->m_sPnKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sThickKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sMatKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sPnNumKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sFrontBendKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sReverseBendKey);
		fprintf(fp, "%d;", pSchema->m_bEditable ? 1 : 0);
		fprintf(fp, "%d\n", pSchema->m_bEnable ? 1 : 0);
	}
	fprintf(fp, "����ʶ��\n");
	fprintf(fp, "MapScale=%.f ;���ű���\n", g_pncSysPara.m_fMapScale);
	fclose(fp);
}

void CPNCSysPara::ActiveRecogSchema(RECOG_SCHEMA *pSchema)
{
	if (pSchema != NULL)
	{
		m_sPnKey.Copy(pSchema->m_sPnKey);
		m_sMatKey.Copy(pSchema->m_sMatKey);
		m_sThickKey.Copy(pSchema->m_sThickKey);
		m_sPnNumKey.Copy(pSchema->m_sPnNumKey);
		m_sFrontBendKey.Copy(pSchema->m_sFrontBendKey);
		m_sReverseBendKey.Copy(pSchema->m_sReverseBendKey);
		m_iDimStyle = pSchema->m_iDimStyle;
	}
}