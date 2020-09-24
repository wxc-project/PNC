#pragma once
#include "XeroExtractor.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "ProcessPart.h"
#include "SteelSealReactor.h"
#include "PNCDockBarManager.h"

//////////////////////////////////////////////////////////////////////////
struct BOLT_HOLE {
	BYTE ciSymbolType;	//0.��׼ͼ��|1.����ͼ��|2.Բ��
	double d;			//��˨����ֱ����M20��˨ʱd=20
	float increment;	//��˨�ױ���˨����ֱ���������϶ֵ
	float posX, posY;	//
	BOLT_HOLE() {
		posX = posY = increment = ciSymbolType = 0; d = 0;
	}
};
//////////////////////////////////////////////////////////////////////////
//CPNCSysPara
class CPNCModel;
#ifndef __UBOM_ONLY_
class CPNCSysPara : public CPlateExtractor
#else
class CPNCSysPara : public CPlateExtractor, public CJgCardExtractor
#endif
{
public:
	struct LAYER_ITEM{
		CXhChar50 m_sLayer;
		BOOL m_bMark;
		LAYER_ITEM(){m_bMark=FALSE;}
		LAYER_ITEM(const char* sName,BOOL bFilter)
		{
			m_sLayer.Copy(sName);
			m_bMark=bFilter;
		}
	};
	//��ɫ����
	struct COLOR_MODE {
		COLORREF crEdge;
		COLORREF crLS12;
		COLORREF crLS16;
		COLORREF crLS20;
		COLORREF crLS24;
		COLORREF crOtherLS;
	}crMode;
	//��׼��˨�׾�
	struct STANDARD_HOLE {
		double m_fLS12;
		double m_fLS16;
		double m_fLS20;
		double m_fLS24;
	}standard_hole;
private:
	CHashStrList<LAYER_ITEM> m_xHashDefaultFilterLayers;
	CHashStrList<LAYER_ITEM> m_xHashEdgeKeepLayers;
	CXhChar500 m_sPlateCardFileName;
public:
	//�������
	int m_iPPiMode;			//PPI�ļ�����ģʽ 0.һ��һ�� 1.һ����
	bool m_bIncDeformed;	//�Ƿ����˻�������
	BYTE m_ciMKPos;			//��ӡλ��:0.�������ֱ�ע|1.��ӡ�ֺп�|2.��ӡ��λ��
	double m_fMKHoleD;		//��λ��ֱ��
	BOOL m_bUseMaxEdge;		//�Ƿ��������߳�
	int m_nMaxEdgeLen;		//���߳�
	int m_nMaxHoleD;		//�����˨ֱ��
	int m_iAxisXCalType;	//�ӹ�����ϵX����㷽ʽ 0.��� 1.��˨����ƽ�б� 2.���ӱ�
	double m_fMapScale;		//����ͼ���Ʊ���
	//��ʾģʽ����
	static const BYTE LAYOUT_CLONE		= 0;	//��¡ģʽ
	static const BYTE LAYOUT_COMPARE	= 1;	//�Ա�ģʽ
	static const BYTE LAYOUT_PRINT		= 2;	//�Ű�ģʽ
	static const BYTE LAYOUT_PROCESS	= 3;	//Ԥ��ģʽ
	static const BYTE LAYOUT_FILTRATE	= 4;	//ɸѡģʽ
	BYTE m_ciLayoutMode;	//��ʾģʽ
	BYTE m_ciArrangeType;	//0.����Ϊ��|1.����Ϊ��
	BYTE m_ciGroupType;		//0.������|1.�κ�|2.����&���|3.����|4.���
	int m_nMapLength;		//ͼֽ���� 0��ʾ������ֽ�ų���
	int m_nMapWidth;		//ͼֽ���
	int m_nMinDistance;		//��С���
	int m_nMkRectLen;		//��ӡ�ֺг���
	int m_nMkRectWidth;		//��ӡ�ֺп��
	//��˨ʶ�����
	static const BYTE FILTER_PARTNO_CIR = 0X01;	//���˼�������ԲȦ
	static const BYTE RECOGN_HOLE_D_DIM = 0X02;	//ʶ��׾����ֱ�ע
	static const BYTE RECOGN_LS_CIRCLE  = 0X04; //������ͨ��˨ԲȦ
	BYTE m_ciBoltRecogMode;
	double m_fPartNoCirD;
	//����ʶ�����
	static const BYTE FILTER_BY_LINETYPE = 0;	//������ʶ��
	static const BYTE FILTER_BY_LAYER	 = 1;	//��ͼ��ʶ��
	static const BYTE FILTER_BY_COLOR	 = 2;	//����ɫʶ��
	static const BYTE FILTER_BY_PIXEL	 = 3;	//��ͼ��ʶ��
	BYTE m_ciRecogMode;
	BYTE m_ciLayerMode;		//ͼ�㴦��ʽ 0.ָ��������ͼ�� 1.����Ĭ��ͼ��
	BYTE m_ciProfileColorIndex;
	BYTE m_ciBendLineColorIndex;
	CXhChar16 m_sProfileLineType;
	double m_fPixelScale;
public:
	CPNCSysPara();
	~CPNCSysPara();
	void Init();
	//
	LAYER_ITEM* EnumFirst();
	LAYER_ITEM* EnumNext();
	LAYER_ITEM* AppendSpecItem(const char* sLayer){return m_xHashEdgeKeepLayers.Add(sLayer);}
	LAYER_ITEM* GetEdgeLayerItem(const char* sLayer){return m_xHashEdgeKeepLayers.GetValue(sLayer);}
	void EmptyEdgeLayerHash(){m_xHashEdgeKeepLayers.Empty();}
	//����ͼ�׿�����
	CXhChar500 GetCurPlateCardFileName();
	CXhChar500 SetCurPlateCardFileName(const char* file_name);
	//
	BOOL IsNeedFilterLayer(const char* sLayer);
	BOOL IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer=NULL);
	BOOL IsProfileEnt(AcDbEntity* pEnt);
	BOOL IsFilterPartNoCir() { return m_ciBoltRecogMode & FILTER_PARTNO_CIR; }
	BOOL IsRecogHoleDimText() { return m_ciBoltRecogMode & RECOGN_HOLE_D_DIM; }
	BOOL IsRecogCirByBoltD() { return m_ciBoltRecogMode & RECOGN_LS_CIRCLE; }
	//
	BOOL RecogBasicInfo(AcDbEntity* pEnt, BASIC_INFO& basicInfo);
	BOOL RecogArcEdge(AcDbEntity* pEnt, f3dArcLine& arcLine, BYTE& ciEdgeType);
	BOOL RecogBoltHole(AcDbEntity* pEnt, BOLT_HOLE& hole, CPNCModel* pBelongModel = NULL);
	BOOL RecogMkRect(AcDbEntity* pEnt, f3dPoint* ptArr, int nNum);
	//
	DECLARE_PROP_FUNC(CPNCSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);//ͨ������Id��ȡ����ֵ
};
extern CPNCSysPara g_pncSysPara;
extern CSteelSealReactor *g_pSteelSealReactor;	
//
void PNCSysSetImportDefault();
void PNCSysSetExportDefault();
bool PNCSysSetImportDefault(FILE* fp);
bool PNCSysSetExportDefault(FILE* fp);