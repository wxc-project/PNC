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
class CPNCSysPara : public CPlateExtractor , public CJgCardExtractor
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
	//颜色方案
	struct COLOR_MODE {
		COLORREF crEdge;
		COLORREF crLS12;
		COLORREF crLS16;
		COLORREF crLS20;
		COLORREF crLS24;
		COLORREF crOtherLS;
	}crMode;
	//标准螺栓孔径
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
	int m_iPPiMode;			//PPI文件生成模式 0.一板一号 1.一板多号
	bool m_bIncDeformed;	//是否考虑了火曲变形
	BOOL m_bMKPos;			//是否获取钢印区位置
	BOOL m_bUseMaxEdge;		//是否启用最大边长
	int m_nMaxEdgeLen;		//最大边长
	int m_nMkRectLen;		//钢印字盒长度
	int m_nMkRectWidth;		//钢印字盒宽度
	int m_nMaxHoleD;		//最大螺栓直径

	static const BYTE LAYOUT_NONE	 = 0;	//
	static const BYTE LAYOUT_COMPARE = 1;
	static const BYTE LAYOUT_PRINT	 = 2;
	static const BYTE LAYOUT_SEG	 = 3;
	BYTE m_ciLayoutMode;	//显示模式 0.克隆模式|1.对比模式|2.排版模式|3.预审模式
	BYTE m_ciArrangeType;	//0.以行为主|1.以列为主
	int m_nMapLength;		//图纸长度 0表示不设置纸张长度
	int m_nMapWidth;		//图纸宽度
	int m_nMinDistance;		//最小间距
	double m_fMapScale;		//
	int m_iLayerMode;		//图层处理方式 0.指定轮廓边图层 1.过滤默认图层
	int m_iAxisXCalType;	//加工坐标系X轴计算方式 0.最长边 1.螺栓射线平行边 2.焊接边
	int m_nBoltPadDimSearchScope;	//螺栓垫板标注搜索范围，默认为50 wht 19-02-01
	static const BYTE FILTER_PARTNO_CIR = 0X01;	//过滤件号特殊圆圈
	static const BYTE RECOGN_HOLE_D_DIM = 0X02;	//识别孔径文字标注
	static const BYTE RECOGN_LS_CIRCLE  = 0X04; //处理普通螺栓圆圈
	BYTE m_ciBoltRecogMode;
	static const BYTE FILTER_BY_LINETYPE = 0;
	static const BYTE FILTER_BY_LAYER	 = 1;
	static const BYTE FILTER_BY_COLOR	 = 2;
	static const BYTE FILTER_BY_PIXEL	 = 3;
	BYTE m_ciRecogMode;		//0.按图层&线型识别 1.按颜色识别
	BYTE m_ciProfileColorIndex;
	BYTE m_ciBendLineColorIndex;
	CXhChar16 m_sProfileLineType;
	double m_fPixelScale;
	//
	CXhChar100 m_sJgCadName;		//角钢工艺卡
	CXhChar16 m_sPartLabelTitle;	//件号标题
	CXhChar50 m_sJgCardBlockName;	//角钢工艺卡块名称 wht 19-09-24
	double m_fMaxLenErr;			//长度最大误差值
	BOOL m_bCmpQualityLevel;		//质量等级校审
	BOOL m_bEqualH_h;				//Q345是否等于Q355
public:
	CPNCSysPara();
	~CPNCSysPara();
	//
	LAYER_ITEM* EnumFirst();
	LAYER_ITEM* EnumNext();
	LAYER_ITEM* AppendSpecItem(const char* sLayer){return m_xHashEdgeKeepLayers.Add(sLayer);}
	LAYER_ITEM* GetEdgeLayerItem(const char* sLayer){return m_xHashEdgeKeepLayers.GetValue(sLayer);}
	void EmptyEdgeLayerHash(){m_xHashEdgeKeepLayers.Empty();}
	void Init();
	RECOG_SCHEMA* InsertRecogSchema(const char* name, int dimStyle, const char* partNoKey,
		const char* matKey, const char* thickKey, const char* partCountKey = "",
		const char* frontBendKey = "", const char* reverseBendKey = "", BOOL bEditable = FALSE);
	void ActiveRecogSchema(RECOG_SCHEMA *pSchema);
	//
	BOOL RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum);
	BOOL IsNeedFilterLayer(const char* sLayer);
	BOOL IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer=NULL);
	BOOL IsProfileEnt(AcDbEntity* pEnt);
	BOOL IsPartLabelTitle(const char* sText);
	BOOL IsJgCardBlockName(const char* sBlockName);
	BOOL IsFilterPartNoCir() { return m_ciBoltRecogMode & FILTER_PARTNO_CIR; }
	BOOL IsRecogHoleDimText() { return m_ciBoltRecogMode & RECOGN_HOLE_D_DIM; }
	BOOL IsRecogCirByBoltD() { return m_ciBoltRecogMode & RECOGN_LS_CIRCLE; }
	//
	DECLARE_PROP_FUNC(CPNCSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);//通过属性Id获取属性值
};
extern CPNCSysPara g_pncSysPara;
extern CSteelSealReactor *g_pSteelSealReactor;	

//
void PNCSysSetImportDefault();
void PNCSysSetExportDefault();
#endif