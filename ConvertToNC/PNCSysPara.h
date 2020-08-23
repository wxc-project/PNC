#ifndef __PNC_SYS_PARAMETER_
#define __PNC_SYS_PARAMETER_
#include "HashTable.h"
#include "XhCharString.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "ProcessPart.h"
#include "XeroExtractor.h"
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"
#include "SteelSealReactor.h"
#include "DockBarManager.h"
//////////////////////////////////////////////////////////////////////////
//
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
public:
	int m_iPPiMode;			//PPI�ļ�����ģʽ 0.һ��һ�� 1.һ����
	bool m_bIncDeformed;	//�Ƿ����˻�������
	BYTE m_ciMKPos;			//��ӡλ��:0.�������ֱ�ע|1.��ӡ�ֺп�|2.��ӡ��λ��
	double m_fMKHoleD;		//��λ��ֱ��
	BOOL m_bUseMaxEdge;		//�Ƿ��������߳�
	int m_nMaxEdgeLen;		//���߳�
	int m_nMkRectLen;		//��ӡ�ֺг���
	int m_nMkRectWidth;		//��ӡ�ֺп��
	int m_nMaxHoleD;		//�����˨ֱ��

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
	double m_fMapScale;		//
	int m_iLayerMode;		//ͼ�㴦��ʽ 0.ָ��������ͼ�� 1.����Ĭ��ͼ��
	int m_iAxisXCalType;	//�ӹ�����ϵX����㷽ʽ 0.��� 1.��˨����ƽ�б� 2.���ӱ�
	int m_nBoltPadDimSearchScope;	//��˨����ע������Χ��Ĭ��Ϊ50 wht 19-02-01
	static const BYTE FILTER_PARTNO_CIR = 0X01;	//���˼�������ԲȦ
	static const BYTE RECOGN_HOLE_D_DIM = 0X02;	//ʶ��׾����ֱ�ע
	static const BYTE RECOGN_LS_CIRCLE  = 0X04; //������ͨ��˨ԲȦ
	BYTE m_ciBoltRecogMode;
	double m_fPartNoCirD;
	static const BYTE FILTER_BY_LINETYPE = 0;
	static const BYTE FILTER_BY_LAYER	 = 1;
	static const BYTE FILTER_BY_COLOR	 = 2;
	static const BYTE FILTER_BY_PIXEL	 = 3;
	BYTE m_ciRecogMode;		//0.��ͼ��&����ʶ�� 1.����ɫʶ��
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
	//
	BOOL RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum);
	BOOL IsNeedFilterLayer(const char* sLayer);
	BOOL IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer=NULL);
	BOOL IsProfileEnt(AcDbEntity* pEnt);
	BOOL IsFilterPartNoCir() { return m_ciBoltRecogMode & FILTER_PARTNO_CIR; }
	BOOL IsRecogHoleDimText() { return m_ciBoltRecogMode & RECOGN_HOLE_D_DIM; }
	BOOL IsRecogCirByBoltD() { return m_ciBoltRecogMode & RECOGN_LS_CIRCLE; }
	//
	DECLARE_PROP_FUNC(CPNCSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);//ͨ������Id��ȡ����ֵ
};
extern CPNCSysPara g_pncSysPara;
extern CSteelSealReactor *g_pSteelSealReactor;	
//
void PNCSysSetImportDefault();
void PNCSysSetExportDefault();
#endif