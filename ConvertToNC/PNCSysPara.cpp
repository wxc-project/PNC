#include "stdafx.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"
#include "LayerTable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPNCSysPara g_pncSysPara;
CSteelSealReactor *g_pSteelSealReactor = NULL;
//////////////////////////////////////////////////////////////////////////
//CDimRuleManager
CPNCSysPara::CPNCSysPara()
{
	Init();
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
	//Ĭ�Ϲ���ͼ����
#ifdef __PNC_
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
const int HASHTABLESIZE = 500;
const int STATUSHASHTABLESIZE = 500;
void CPNCSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//��������
	AddPropItem("general_set",PROPLIST_ITEM(id++,"��������","��������"));
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
		if (m_bIncDeformed)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_bUseMaxEdge") == id)
	{
		if (m_bUseMaxEdge)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_nMaxEdgeLen") == id)
		sText.Printf("%d", m_nMaxEdgeLen);
	else if (GetPropID("m_iPPiMode") == id)
	{
		if (m_iPPiMode == 0)
			sText.Copy("0.һ��һ��");
		else if (m_iPPiMode == 1)
			sText.Copy("1.һ����");
	}
	else if (GetPropID("AxisXCalType") == id)
	{
		if (m_iAxisXCalType == 0)
			sText.Copy("0.�������");
		else if (m_iAxisXCalType == 1)
			sText.Copy("1.��˨ƽ�б�����");
		else if (m_iAxisXCalType == 2)
			sText.Copy("2.���ӱ�����");
	}
	else if (GetPropID("m_ciMKPos") == id)
	{
		if (m_ciMKPos == 1)
			sText.Copy("1.��ӡ�ֺп�");
		else if (m_ciMKPos == 2)
			sText.Copy("2.��ӡ��λ��");
		else
			sText.Copy("0.�������ֱ�ע");
	}
	else if (GetPropID("m_fMKHoleD") == id)
		sText.Printf("%g", m_fMKHoleD);
	else if (GetPropID("m_nMaxHoleD") == id)
		sText.Printf("%d", m_nMaxHoleD);
	else if (GetPropID("m_fMapScale") == id)
		sText.Printf("%.f",m_fMapScale);
	else if (GetPropID("m_ciLayoutMode") == id)
	{
		if (m_ciLayoutMode == 1)
			sText.Copy("�ְ�Ա�");
		else if (m_ciLayoutMode == 2)
			sText.Copy("�Զ��Ű�");
		else if (m_ciLayoutMode == 3)
			sText.Copy("����Ԥ��");
		else if (m_ciLayoutMode == 4)
			sText.Copy("ͼԪɸѡ");
		else
			sText.Copy("ͼԪ��¡");
	}
	else if (GetPropID("m_ciArrangeType") == id)
	{
		if (m_ciArrangeType == 0)
			sText.Copy("0.����Ϊ��");
		else
			sText.Copy("1.����Ϊ��");
	}
	else if (GetPropID("m_ciGroupType") == id)
	{
		if (m_ciGroupType == 1)
			sText.Copy("1.���κ�");
		else if (m_ciGroupType == 2)
			sText.Copy("2.����&���");
		else if (m_ciGroupType == 3)
			sText.Copy("3.����");
		else if (m_ciGroupType == 4)
			sText.Copy("4.���");
		else
			sText.Copy("0.������");
	}
	else if (GetPropID("m_nMapLength") == id)
		sText.Printf("%d", m_nMapLength);
	else if (GetPropID("m_nMapWidth") == id)
		sText.Printf("%d", m_nMapWidth);
	else if (GetPropID("m_nMinDistance") == id)
		sText.Printf("%d", m_nMinDistance);
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
		sText.Printf("%d", m_nMkRectWidth);
	else if (GetPropID("m_nMkRectLen") == id)
		sText.Printf("%d", m_nMkRectLen);
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
		sText.Printf("%f", m_fPixelScale);
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

//��˨ͼ����Ҫ�У�Բ�Ρ������Ρ�����
double RecogHoleDByBlockRef(AcDbBlockTableRecord *pTempBlockTableRecord, double scale)
{
	if (pTempBlockTableRecord == NULL)
		return 0;
	double fHoleD = 0;
	AcDbEntity *pEnt = NULL;
	AcDbBlockTableRecordIterator *pIterator = NULL;
	pTempBlockTableRecord->newIterator(pIterator);
	if (pIterator)
	{
		SCOPE_STRU scope;
		for (; !pIterator->done(); pIterator->step())
		{
			pIterator->getEntity(pEnt, AcDb::kForRead);
			if (pEnt == NULL)
				continue;
			pEnt->close();
			AcDbCircle acad_cir;
			if (pEnt->isKindOf(AcDbCircle::desc()))
			{
				AcDbCircle *pCir = (AcDbCircle*)pEnt;
				acad_cir.setCenter(pCir->center());
				acad_cir.setNormal(pCir->normal());
				acad_cir.setRadius(pCir->radius());
				VerifyVertexByCADEnt(scope, &acad_cir);
			}
			else if (pEnt->isKindOf(AcDbPolyline::desc()))
			{	//��������Բ������������������Ϊ�������
				AcDbPolyline *pPolyLine = (AcDbPolyline*)pEnt;
				if (pPolyLine->numVerts() <= 0)
					continue;
				AcGePoint3d point;
				pPolyLine->getPointAt(0, point);
				double fRadius = GEPOINT(point.x, point.y).mod();
				acad_cir.setCenter(AcGePoint3d(0, 0, 0));
				acad_cir.setNormal(AcGeVector3d(0, 0, 1));
				acad_cir.setRadius(fRadius);
				VerifyVertexByCADEnt(scope, &acad_cir);
			}
			else
				continue;
		}
		fHoleD = max(scope.wide(), scope.high());
		fHoleD = fabs(fHoleD*scale);
		//�Լ���õ��Ŀ׾�����Բ������ȷ��С����һλ
		int nValue = (int)floor(fHoleD);		//��������
		double fValue = fHoleD - nValue;	//С������
		if (fValue < EPS2)	//�׾�Ϊ����
			fHoleD = nValue;
		else if (fValue > EPS_COS2)
			fHoleD = nValue + 1;
		else if (fabs(fValue - 0.5) < EPS2)
			fHoleD = nValue + 0.5;
		else
			fHoleD = ftoi(fHoleD);
	}
	return fHoleD;
}

BOOL CPNCSysPara::RecogBoltHole(AcDbEntity* pEnt, BOLT_HOLE& hole)
{
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbBlockReference::desc()))
	{
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
		if (sName.GetLength() <= 0)
			return FALSE;
		BOLT_BLOCK* pBoltD = hashBoltDList.GetValue(sName);
		if (pBoltD == NULL)
		{	//ֻ������δ��˨ͼ���Ŀ���д���������ܴ���ʶ��������Ϊ��˨�� wht 20-04-28
			return FALSE;
		}
		double fHoleD = 0;
		CAD_ENTITY* pLsBlockEnt = model.m_xBoltBlockHash.GetValue(pEnt->id().asOldId());
		if (pLsBlockEnt)
			fHoleD = pLsBlockEnt->m_fSize;
		else
		{
			fHoleD = RecogHoleDByBlockRef(pTempBlockTableRecord, pReference->scaleFactors().sx);
			CAD_ENTITY* pLsBlockEnt = model.m_xBoltBlockHash.Add(pEnt->id().asOldId());
			pLsBlockEnt->pos.x = hole.posX;
			pLsBlockEnt->pos.y = hole.posY;
			pLsBlockEnt->m_fSize = fHoleD;
		}
		if (fHoleD <= 0)
		{
			logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "������˨ͼ��{%s}����׾�ʧ�ܣ�", (char*)sName);
			return FALSE;
		}
		BOOL bValidLsBlock = TRUE;
		double fIncrement = (pBoltD->diameter > 0) ? fHoleD - pBoltD->diameter : 0;
		if (fIncrement > 2 || fIncrement < 0)
		{
			bValidLsBlock = FALSE;
			//����ͼԪ����õ���ֱ������˨��Ӧ��ֱ��������δָ����˨�׾�ʱ��ʾ�û� wht 20-08-14
			if (pBoltD->hole_d <= 0 || pBoltD->hole_d < pBoltD->diameter)
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "������˨ͼ��{%s}����Ŀ׾�{%.1f}������������������˨ͼ���Ӧ�Ŀ׾���", (char*)sName, fHoleD);
		}
		//
		hole.posX = (float)pReference->position().x;
		hole.posY = (float)pReference->position().y;
		if (pBoltD->diameter > 0)
		{	//ָ����˨���ֱ�������ձ�׼��˨����
			hole.ciSymbolType = 0;
			hole.d = pBoltD->diameter;
			if (!bValidLsBlock && pBoltD->hole_d > pBoltD->diameter)
				hole.increment = (float)(pBoltD->hole_d - pBoltD->diameter);
			else
				hole.increment = (float)(fHoleD - pBoltD->diameter);
		}
		else
		{	//δָ����˨���ֱ����������״���
			hole.ciSymbolType = 1;	//����ͼ��
			hole.d = fHoleD;
			hole.increment = 0;
		}
		return TRUE;
	}
	else if (pEnt->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle = (AcDbCircle*)pEnt;
		if (int(pCircle->radius()) <= 0)
			return FALSE;	//ȥ����
		double fDiameter = pCircle->radius() * 2;
		if (g_pncSysPara.m_ciMKPos == 2 &&
			fabs(g_pncSysPara.m_fMKHoleD - fDiameter) < EPS2)
			return FALSE;	//ȥ����λ��
		/* �����ֱ��ֱ������ֱ��������׾�����ֵ��
		/*������PPE��ͳһ����׾�����ֵʱ�ᶪʧ�˴���ȡ�Ŀ׾�����ֵ wht 19-09-12
		/*�Կ׾�����Բ������ȷ��С����һλ
		*/
		int nValue = (int)floor(fDiameter);	//��������
		double fValue = fDiameter - nValue;	//С������
		if (fValue < EPS2)	//�׾�Ϊ����
			fDiameter = nValue;
		else if (fValue > EPS_COS2)
			fDiameter = nValue + 1;
		else if (fabs(fValue - 0.5) < EPS2)
			fDiameter = nValue + 0.5;
		else
			fDiameter = ftoi(fDiameter);
		hole.d = fDiameter;
		hole.increment = 0;
		hole.ciSymbolType = 2;	//Ĭ�Ϲ��߿�
		hole.posX = (float)pCircle->center().x;
		hole.posY = (float)pCircle->center().y;
		return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::RecogBasicInfo(AcDbEntity* pEnt, BASIC_INFO& basicInfo)
{
	if (pEnt == NULL)
		return FALSE;
	//�ӿ��н����ְ���Ϣ
	if (pEnt->isKindOf(AcDbBlockReference::desc()))
	{
		AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
		BOOL bRetCode = false;
		AcDbEntity *pSubEnt = NULL;
		AcDbObjectIterator *pIter = pBlockRef->attributeIterator();
		for (pIter->start(); !pIter->done(); pIter->step())
		{
			CAcDbObjLife objLife(pIter->objectId());
			if ((pSubEnt = objLife.GetEnt()) == NULL)
				continue;
			if (!pSubEnt->isKindOf(AcDbAttribute::desc()))
				continue;
			AcDbAttribute *pAttr = (AcDbAttribute*)pSubEnt;
			CXhChar100 sTag, sText;
#ifdef _ARX_2007
			sTag.Copy(_bstr_t(pAttr->tag()));
			sText.Copy(_bstr_t(pAttr->textString()));
#else
			sTag.Copy(pAttr->tag());
			sText.Copy(pAttr->textString());
#endif
			if (sTag.GetLength() == 0 || sText.GetLength() == 0)
				continue;
			if (sTag.EqualNoCase("����&���&����"))
				bRetCode = RecogBasicInfo(pAttr, basicInfo);
			else if (sTag.EqualNoCase("����"))
			{
				CXhChar50 sTemp(sText);
				for (char* token = strtok(sTemp, "X="); token; token = strtok(NULL, "X="))
				{
					CXhChar16 sToken(token);
					if (sToken.Replace("��", "") > 0)
						basicInfo.m_nNum = atoi(sToken);
				}
			}
			else if (sTag.EqualNoCase("����"))
				basicInfo.m_sTaType.Copy(sText);
			else if (sTag.EqualNoCase("��ӡ"))
			{
				sText.Replace("��ӡ", "");
				sText.Replace(":", "");
				sText.Replace("��", "");
				basicInfo.m_sTaStampNo.Copy(sText);
			}
			else if (sTag.EqualNoCase("�׾�"))
				basicInfo.m_sBoltStr.Copy(sText);
			else if (sTag.EqualNoCase("���̴���"))
			{
				sText.Replace("���̴���", "");
				sText.Replace(":", "");
				sText.Replace("��", "");
				basicInfo.m_sPrjCode.Copy(sText);
			}
		}
		return bRetCode;
	}
	//���ַ����н����ְ���Ϣ
	CXhChar500 sText;
	vector<CString> lineList;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pMText->contents()));
#else
		sText.Copy(pMText->contents());
#endif
		for (char* sKey = strtok(sText, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CString sTemp = sKey;
			sTemp.Replace("\\P", "");
			lineList.push_back(sTemp);
		}
	}
	else if (pEnt->isKindOf(AcDbAttribute::desc()))
	{
		AcDbAttribute* pText = (AcDbAttribute*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
	}
	BOOL bRet = FALSE;
	if (lineList.size() > 0)
	{
		for (size_t i = 0; i < lineList.size(); i++)
		{
			CString sTemp = lineList.at(i);
			if (IsMatchThickRule(sTemp))
			{
				ParseThickText(sTemp, basicInfo.m_nThick);
				bRet = TRUE;
			}
			if (IsMatchMatRule(sTemp))
			{
				ParseMatText(sTemp, basicInfo.m_cMat, basicInfo.m_cQuality);
				bRet = TRUE;
			}
			if (IsMatchNumRule(sTemp))
			{
				ParseNumText(sTemp, basicInfo.m_nNum);
				//��¼����������Ӧ��ʵ��Id wht 19-08-05
				basicInfo.m_idCadEntNum = pEnt->id().asOldId();
				bRet = TRUE;
			}
			if (IsMatchPNRule(sTemp))
			{
				ParsePartNoText(sTemp, basicInfo.m_sPartNo);
				bRet = TRUE;
			}
			if (strstr(sTemp, "����"))
			{
				sTemp.Replace("����", "");
				sTemp.Replace(":", "");
				sTemp.Replace("��", "");
				basicInfo.m_sTaType.Copy(sTemp);
			}
		}
	}
	else
	{
		if (IsMatchThickRule(sText))
		{
			ParseThickText(sText, basicInfo.m_nThick);
			bRet = TRUE;
		}
		if (IsMatchMatRule(sText))
		{
			ParseMatText(sText, basicInfo.m_cMat, basicInfo.m_cQuality);
			bRet = TRUE;
		}
		if (IsMatchNumRule(sText))
		{
			ParseNumText(sText, basicInfo.m_nNum);
			//��¼����������Ӧ��ʵ��Id wht 19-08-05
			basicInfo.m_idCadEntNum = pEnt->id().asOldId();
			bRet = TRUE;
		}
		if (IsMatchPNRule(sText))
		{
			ParsePartNoText(sText, basicInfo.m_sPartNo);
			bRet = TRUE;
		}
		if (strstr(sText, "����"))
		{
			sText.Replace("����", "");
			sText.Replace(":", "");
			sText.Replace("��", "");
			basicInfo.m_sTaType.Copy(sText);
			bRet = TRUE;
		}
	}
	return bRet;
}
BOOL CPNCSysPara::RecogArcEdge(AcDbEntity* pEnt, f3dArcLine& arcLine, BYTE& ciEdgeType)
{
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc = (AcDbArc*)pEnt;
		AcGePoint3d pt;
		f3dPoint startPt, endPt, center, norm;
		pArc->getStartPoint(pt);
		Cpy_Pnt(startPt, pt);
		pArc->getEndPoint(pt);
		Cpy_Pnt(endPt, pt);
		Cpy_Pnt(center, pArc->center());
		Cpy_Pnt(norm, pArc->normal());
		double radius = pArc->radius();
		double angle = (pArc->endAngle() - pArc->startAngle());
		if (radius > 0 && fabs(angle) > 0 && DISTANCE(startPt, endPt) > EPS)
		{	//���˴����Բ��(���磺��ʱpEnt��һ����,����������ʾΪԲ��)
			//��֤startPt-endPt���ص� wht 19-11-11
			ciEdgeType = 2;
			return arcLine.CreateMethod3(startPt, endPt, norm, radius, center);
		}
	}
	else if (pEnt->isKindOf(AcDbEllipse::desc()))
	{	//���¸ְ嶥�����(��Բ)
		AcDbEllipse* pEllipse = (AcDbEllipse*)pEnt;
		AcGePoint3d pt;
		AcGeVector3d minorAxis;
		f3dPoint startPt, endPt, center, min_vec, maj_vec, column_norm, work_norm;
		pEllipse->getStartPoint(pt);
		Cpy_Pnt(startPt, pt);
		pEllipse->getEndPoint(pt);
		Cpy_Pnt(endPt, pt);
		Cpy_Pnt(center, pEllipse->center());
		Cpy_Pnt(min_vec, pEllipse->minorAxis());
		Cpy_Pnt(maj_vec, pEllipse->majorAxis());
		Cpy_Pnt(work_norm, pEllipse->normal());
		double min_R = min_vec.mod();
		double maj_R = maj_vec.mod();
		double cosa = min_R / maj_R;
		double sina = SQRT(1 - cosa * cosa);
		column_norm = work_norm;
		RotateVectorAroundVector(column_norm, sina, cosa, min_vec);
		//
		ciEdgeType = 3;
		return arcLine.CreateEllipse(center, startPt, endPt, column_norm, work_norm, min_R);
	}
	return FALSE;
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
			if (fabs(m_fMKHoleD - fRidius * 2) >= EPS2)
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
//////////////////////////////////////////////////////////////////////////
//
void PNCSysSetImportDefault()
{
	char file_name[MAX_PATH] = "";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
	g_pncSysPara.hashBoltDList.Empty();
	g_pncSysPara.m_recogSchemaList.Empty();
	int nValue = 0;
	char line_txt[MAX_PATH] = "", sText[MAX_PATH] = "", key_word[100] = "";
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		strcpy(sText, line_txt);
		CString sLine(line_txt);
		sLine.Replace('=', ' ');
		sLine.Replace('\n', ' ');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		if (_stricmp(key_word, "bIncDeformed") == 0)
		{
			CXhChar16 sText;
			sscanf(line_txt, "%s%s", key_word, (char*)sText);
			if (sText.Equal("��"))
				g_pncSysPara.m_bIncDeformed = true;
			else
				g_pncSysPara.m_bIncDeformed = false;
		}
		else if (_stricmp(key_word, "m_nMaxEdgeLen") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMaxEdgeLen);
		else if (_stricmp(key_word, "m_nMaxHoleD") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMaxHoleD);
		else if (_stricmp(key_word, "MKPos") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciMKPos = nValue;
		}
		else if (_stricmp(key_word, "MKHole") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.m_fMKHoleD = atof(skey);
		}
		else if (_stricmp(key_word, "AxisXCalType") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iAxisXCalType);
		else if (_stricmp(key_word, "PPIMode") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iPPiMode);
		else if (_stricmp(key_word, "LayoutMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciLayoutMode = nValue;
		}
		else if (_stricmp(key_word, "ArrangeType") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciArrangeType = nValue;
		}
		else if (_stricmp(key_word, "GroupType") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciGroupType = nValue;
		}
		else if (_stricmp(key_word, "MapWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapWidth);
		else if (_stricmp(key_word, "MapLength") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapLength);
		else if (_stricmp(key_word, "MinDistance") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMinDistance);
		else if (_stricmp(key_word, "m_nMkRectWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectWidth);
		else if (_stricmp(key_word, "m_nMkRectLen") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectLen);
#ifndef __UBOM_ONLY_
		else if (_stricmp(key_word, "CDrawDamBoard::BOARD_HEIGHT") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::BOARD_HEIGHT);
		else if (_stricmp(key_word, "CDrawDamBoard::m_bDrawAllBamBoard") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::m_bDrawAllBamBoard);
#endif
		else if (_stricmp(key_word, "RecogMode") == 0)
		{	//ʹ��%d��ȡBYTE�ͱ�����һ�θ���4���ֽ�Ӱ����������ȡֵ
			//��ʹ��%hhu��ȡBYTE������������Ч���ȶ�ȡ����ʱ�����У��ٸ�ֵ����Ӧ������ wht 19-10-05
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciRecogMode = nValue;
		}
		else if (_stricmp(key_word, "bIncFilterLayer") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iLayerMode);
		else if (_stricmp(key_word, "ProfileLineType") == 0)
			sscanf(line_txt, "%s%s", key_word, &g_pncSysPara.m_sProfileLineType);
		else if (_stricmp(key_word, "PixelScale") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey)
				g_pncSysPara.m_fPixelScale = atof(skey);
		}
		else if (_stricmp(key_word, "ProfileColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciProfileColorIndex = nValue;
		}
		else if (_stricmp(key_word, "BendLineColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciBendLineColorIndex = nValue;
		}
		else if (_stricmp(key_word, "BoltRecogMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciBoltRecogMode = nValue;
		}
		else if (_stricmp(key_word, "PartNoCirD") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey)
				g_pncSysPara.m_fPartNoCirD = atof(skey);
		}
		else if (_stricmp(key_word, "MapScale") == 0)
			sscanf(line_txt, "%s%f", key_word, &g_pncSysPara.m_fMapScale);
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
}
void PNCSysSetExportDefault()
{
	char file_name[MAX_PATH] = "";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("�򲻿�ָ���������ļ�!");
		return;
	}
	fprintf(fp, "��������\n");
	fprintf(fp, "bIncDeformed=%s ;���ǻ���������\n", g_pncSysPara.m_bIncDeformed ? "��" : "��");
	fprintf(fp, "m_nMaxEdgeLen=%d ;��߳�\n", g_pncSysPara.m_nMaxEdgeLen);
	fprintf(fp, "m_nMaxHoleD=%d ;�����˨��\n", g_pncSysPara.m_nMaxHoleD);
	fprintf(fp, "MKPos=%d ;��ȡ��ӡλ��\n", g_pncSysPara.m_ciMKPos);
	fprintf(fp, "MKHole=%g ;��λ��ֱ��\n", g_pncSysPara.m_fMKHoleD);
	fprintf(fp, "AxisXCalType=%d ;X����㷽ʽ\n", g_pncSysPara.m_iAxisXCalType);
	fprintf(fp, "PPIMode=%d ;PPI�ļ�ģʽ\n", g_pncSysPara.m_iPPiMode);
	fprintf(fp, "��ʾ����\n");
	fprintf(fp, "LayoutMode=%d ;��ʾģʽ\n", g_pncSysPara.m_ciLayoutMode);
	fprintf(fp, "ArrangeType=%d ;�ԱȲ��ַ���\n", g_pncSysPara.m_ciArrangeType);
	fprintf(fp, "GroupType=%d ;���Ʒ��鷽ʽ\n", g_pncSysPara.m_ciGroupType);
	fprintf(fp, "MapWidth=%d ;ͼֽ���\n", g_pncSysPara.m_nMapWidth);
	fprintf(fp, "MapLength=%d ;ͼֽ����\n", g_pncSysPara.m_nMapLength);
	fprintf(fp, "MinDistance=%d ;��С���\n", g_pncSysPara.m_nMinDistance);
	fprintf(fp, "m_nMkRectWidth=%d ;�ֺп��\n", g_pncSysPara.m_nMkRectWidth);
	fprintf(fp, "m_nMkRectLen=%d ;�ֺг���\n", g_pncSysPara.m_nMkRectLen);
#ifndef __UBOM_ONLY_
	fprintf(fp, "CDrawDamBoard::BOARD_HEIGHT=%d ;����߶�\n", CDrawDamBoard::BOARD_HEIGHT);
	fprintf(fp, "CDrawDamBoard::m_bDrawAllBamBoard=%d ;�������е���\n", CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	fprintf(fp, "ʶ������\n");
	fprintf(fp, "RecogMode=%d ;ʶ��ģʽ\n", g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "bIncFilterLayer=%d ;���ù���Ĭ��ͼ��\n", g_pncSysPara.m_iLayerMode);
	fprintf(fp, "ProfileLineType=%s ;����������\n", (char*)g_pncSysPara.m_sProfileLineType);
	fprintf(fp, "PixelScale=%.1f ;ͼ�����\n", g_pncSysPara.m_fPixelScale);
	fprintf(fp, "ProfileColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciProfileColorIndex);
	fprintf(fp, "BendLineColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciBendLineColorIndex);
	fprintf(fp, "BoltRecogMode=%d ;��˨ʶ��ģʽ\n", g_pncSysPara.m_ciBoltRecogMode);
	fprintf(fp, "PartNoCirD=%.1f ;����ԲȦֱ��\n", g_pncSysPara.m_fPartNoCirD);
	fprintf(fp, "MapScale=%.f ;���ű���\n", g_pncSysPara.m_fMapScale);
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
	fclose(fp);
}
