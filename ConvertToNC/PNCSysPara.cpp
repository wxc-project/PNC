#include "stdafx.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"
#include "LayerTable.h"
#include "PNCCryptCoreCode.h"
#include "DwgExtractor.h"

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
	CPlateRecogRule::Init();
	m_iPPiMode = 0;
	m_bIncDeformed = true;
	m_iAxisXCalType = 0;
	m_bUseMaxEdge = FALSE;
	m_nMaxEdgeLen = 2200;
	m_nMaxHoleD = 50;
	m_ciMKPos = 0;
	m_fMKHoleD = 10;
	m_fMapScale = 1;
	//自动排版设置
	m_ciLayoutMode = 1;
	m_ciArrangeType = 1;
	m_nMapWidth = 1500;
	m_nMapLength = 0;
	m_nMinDistance = 0;
	m_nMkRectWidth = 30;
	m_nMkRectLen = 60;
	m_nMkCircleRadius = 26;
	//识别设置
	m_ciRecogMode = 3;
	m_ciLayerMode = 0;
	m_ciProfileColorIndex = 1;		//红色
	m_ciBendLineColorIndex = 0;		//无颜色
	m_sProfileLineType.Copy("CONTINUOUS");
	m_fPixelScale = 0.6;
	m_ciBoltRecogMode = FILTER_PARTNO_CIR;
	m_fPartNoCirD = 0;
	m_fRoundLineLen = 24;
	//默认颜色设置
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
	standard_hole.m_fLS_SJ = 17.5;
	standard_hole.m_fLS_ZF = 21.5;
	standard_hole.m_fLS_YY = 21.5;
	//默认过滤图层名
#ifdef __PNC_
	m_xHashDefaultFilterLayers.SetValue(LayerTable::UnvisibleProfileLayer.layerName, LAYER_ITEM(LayerTable::UnvisibleProfileLayer.layerName, 1));//不可见轮廓线
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltSymbolLayer.layerName, LAYER_ITEM(LayerTable::BoltSymbolLayer.layerName, 1));		//螺栓图符
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BendLineLayer.layerName, LAYER_ITEM(LayerTable::BendLineLayer.layerName, 1));		//角钢火曲、钢板火曲
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BreakLineLayer.layerName, LAYER_ITEM(LayerTable::BreakLineLayer.layerName, 1));		//断开界线
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DimTextLayer.layerName, LAYER_ITEM(LayerTable::DimTextLayer.layerName, 1));		//文字标注图层
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltLineLayer.layerName, LAYER_ITEM(LayerTable::BoltLineLayer.layerName, 1));		//螺栓线
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DamagedSymbolLine.layerName, LAYER_ITEM(LayerTable::DamagedSymbolLine.layerName, 1));	//板边破损标记线
	m_xHashDefaultFilterLayers.SetValue(LayerTable::CommonLayer.layerName, LAYER_ITEM(LayerTable::CommonLayer.layerName, 1));			//其他
#endif
}
CPNCSysPara::~CPNCSysPara()
{
	m_xBoltBlockRecog.Empty();
	m_xHashDefaultFilterLayers.Empty();
	m_xHashEdgeKeepLayers.Empty();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumFirst()
{
	if(m_ciLayerMode==1)
		return m_xHashDefaultFilterLayers.GetFirst();
	else
		return m_xHashEdgeKeepLayers.GetFirst();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumNext()
{
	if(m_ciLayerMode==1)
		return m_xHashDefaultFilterLayers.GetNext();
	else
		return m_xHashEdgeKeepLayers.GetNext();
}
//件号圆圈处理模式(占第一位)
BYTE CPNCSysPara::get_ciPartNoCirMode() const
{
	BYTE ciTypeFlag = m_ciBoltRecogMode & 0X01;
	return ciTypeFlag;
}
BYTE CPNCSysPara::set_ciPartNoCirMode(BYTE ciPartNoCirMode)
{
	BYTE ciTypeFlag = ciPartNoCirMode & 0X01;
	m_ciBoltRecogMode &= 0xfe;
	m_ciBoltRecogMode |= ciTypeFlag;
	return ciPartNoCirMode & 0X01;
}
//孔径标注处理模式(占第二位)
BYTE CPNCSysPara::get_ciHoleDimTextMode() const
{
	BYTE ciTypeFlag = m_ciBoltRecogMode & 0X02;
	ciTypeFlag >>= 1;
	return ciTypeFlag;
}
BYTE CPNCSysPara::set_ciHoleDimTextMode(BYTE ciHoleDimTextMode)
{
	BYTE ciTypeFlag = ciHoleDimTextMode & 0X01;
	ciTypeFlag <<= 1;
	m_ciBoltRecogMode &= 0xfd;
	m_ciBoltRecogMode |= ciTypeFlag;
	return ciHoleDimTextMode & 0X01;
}
//圆孔螺栓处理模式(占第三位)
BYTE CPNCSysPara::get_ciCircleLsMode() const
{
	BYTE ciTypeFlag = m_ciBoltRecogMode & 0X04;
	ciTypeFlag >>= 2;
	return ciTypeFlag;
}
BYTE CPNCSysPara::set_ciCircleLsMode(BYTE ciCircleLsMode)
{
	BYTE ciTypeFlag = ciCircleLsMode & 0X01;
	ciTypeFlag <<= 2;
	m_ciBoltRecogMode &= 0xfb;
	m_ciBoltRecogMode |= ciTypeFlag;
	return ciCircleLsMode & 0X01;
}
//多段线螺栓处理模式(占五、六位)
BYTE CPNCSysPara::get_ciPolylineLsMode() const
{
	BYTE ciTypeFlag = m_ciBoltRecogMode & 0x30;
	ciTypeFlag >>= 4;
	return ciTypeFlag;
}
BYTE CPNCSysPara::set_ciPolylineLsMode(BYTE ciPolylineLsMode)
{
	BYTE ciTypeFlag = ciPolylineLsMode & 0x03;
	ciTypeFlag <<= 4;
	m_ciBoltRecogMode &= 0xcf;
	m_ciBoltRecogMode |= ciTypeFlag;
	return ciPolylineLsMode & 0x03;
}
IMPLEMENT_PROP_FUNC(CPNCSysPara);
const int HASHTABLESIZE = 500;
const int STATUSHASHTABLESIZE = 500;
int CPNCSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//基本设置
	AddPropItem("general_set",PROPLIST_ITEM(id++,"常规设置","常规设置"));
	AddPropItem("m_bIncDeformed",PROPLIST_ITEM(id++,"已考虑火曲变形","待提取的钢板图形已考虑火曲变形量","是|否"));
	AddPropItem("m_iPPiMode",PROPLIST_ITEM(id++,"文件生成模型","PPI文件生成模式","0.一板一号|1.一板多号"));
	AddPropItem("m_ciMKPos",PROPLIST_ITEM(id++,"钢印识别方式","提取钢印位置","0.件号文字标注|1.钢印字盒块|2.钢印号位孔"));
	AddPropItem("m_fMKHoleD", PROPLIST_ITEM(id++, "号位孔直径"));
	AddPropItem("m_nMaxHoleD", PROPLIST_ITEM(id++, "最大螺栓孔径", "最大螺栓孔径"));
	AddPropItem("AxisXCalType",PROPLIST_ITEM(id++,"X轴计算方式","加工坐标系X轴的计算方式","0.最长边优先|1.螺栓平行边优先|2.焊接边优先"));
	AddPropItem("m_bUseMaxEdge", PROPLIST_ITEM(id++, "处理特殊边长", "用于处理料片提取", "是|否"));
	AddPropItem("m_nMaxEdgeLen", PROPLIST_ITEM(id++, "最大边长", "进行料片提取的特殊边长"));
	//识别模式
	AddPropItem("RecogMode",PROPLIST_ITEM(id++,"识别模式","识别模式"));
	AddPropItem("m_fMapScale", PROPLIST_ITEM(id++, "识别比例", "绘图缩放比例"));
	AddPropItem("m_iRecogMode", PROPLIST_ITEM(id++, "轮廓边识别", "钢板识别模式", "0.按线型识别|1.按图层识别|2.按颜色识别|3.智能识别"));
	AddPropItem("m_iProfileLineTypeName", PROPLIST_ITEM(id++, "轮廓边线型", "钢板外轮廓边线型", "CONTINUOUS|HIDDEN|DASHDOT2X|DIVIDE|ZIGZAG"));
	AddPropItem("m_iProfileColorIndex",PROPLIST_ITEM(id++,"轮廓边颜色","钢板外轮廓边颜色"));
	AddPropItem("m_iBendLineColorIndex",PROPLIST_ITEM(id++,"火曲线颜色","钢板制弯线颜色"));
	AddPropItem("layer_mode",PROPLIST_ITEM(id++,"图层处理方式","轮廓边图层处理方式","0.指定轮廓边图层|1.过滤默认图层"));
	AddPropItem("m_fPixelScale", PROPLIST_ITEM(id++, "处理像素比例"));
	AddPropItem("FilterPartNoCir", PROPLIST_ITEM(id++, "件号专属圆圈", "", "过滤|不过滤"));
	AddPropItem("m_fPartNoCirD", PROPLIST_ITEM(id++, "件号圆圈直径", ""));
	AddPropItem("RecogHoleDimText", PROPLIST_ITEM(id++, "孔径文字标注", "特殊孔径标注(文字说明或直径标注)", "按标注处理|不进行处理"));
	AddPropItem("RecogLsCircle", PROPLIST_ITEM(id++, "圆孔式螺栓", "非图符块的圆圈表示的螺栓", "统一按实际孔径处理|根据标准孔进行筛选"));
	AddPropItem("RecogLsBlock", PROPLIST_ITEM(id++, "图块式螺栓", "用螺栓图符块表示的螺栓","螺栓图块设置"));
	AddPropItem("standardM12", PROPLIST_ITEM(id++, "M12标准孔径"));
	AddPropItem("standardM16", PROPLIST_ITEM(id++, "M16标准孔径"));
	AddPropItem("standardM20", PROPLIST_ITEM(id++, "M20标准孔径"));
	AddPropItem("standardM24", PROPLIST_ITEM(id++, "M24标准孔径"));
	AddPropItem("standard_SJ", PROPLIST_ITEM(id++, "三角螺栓孔径"));
	AddPropItem("standard_ZF", PROPLIST_ITEM(id++, "方形螺栓孔径"));
	AddPropItem("standard_YY", PROPLIST_ITEM(id++, "腰圆螺栓孔径"));
	AddPropItem("RecogLsPolyline", PROPLIST_ITEM(id++, "多段线螺栓", "非图符块的多段线螺栓(三角、矩形、腰圆等)", "不进行处理|自动计算孔径|指定标准孔径"));
	AddPropItem("RoundLineLen", PROPLIST_ITEM(id++, "腰圆过渡长度", ""));
	//显示模式
	AddPropItem("DisplayMode", PROPLIST_ITEM(id++, "显示模式"));
	AddPropItem("m_ciLayoutMode", PROPLIST_ITEM(id++, "显示布局模式","","图元克隆|钢板对比|自动排版|下料预审|图元筛选"));
	AddPropItem("m_nMapWidth", PROPLIST_ITEM(id++, "图纸宽度", "图纸宽度"));
	AddPropItem("m_nMapLength", PROPLIST_ITEM(id++, "图纸长度", "图纸长度"));
	AddPropItem("m_nMinDistance", PROPLIST_ITEM(id++, "最小间距", "图形之间的最小间距"));
	AddPropItem("CDrawDamBoard::BOARD_HEIGHT", PROPLIST_ITEM(id++, "档板高度", "档板高度"));
	AddPropItem("CDrawDamBoard::m_bDrawAllBamBoard", PROPLIST_ITEM(id++, "档板显示模式", "档板显示模式", "0.仅显示选中钢板档板|1.显示所有档板"));
	AddPropItem("m_nMkRectLen", PROPLIST_ITEM(id++, "钢印字盒长度", "钢印字盒宽度"));
	AddPropItem("m_nMkRectWidth", PROPLIST_ITEM(id++, "钢印字盒宽度", "钢印字盒宽度"));
	AddPropItem("m_nMkCircleRadius", PROPLIST_ITEM(id++, "钢印圆半径", "钢印圆半径"));
	AddPropItem("m_ciArrangeType", PROPLIST_ITEM(id++, "布局方案", "","0.以行为主|1.以列为主"));
	AddPropItem("m_ciGroupType", PROPLIST_ITEM(id++, "分组方案", "", "1.按段号|2.材质&厚度|3.材质|4.厚度"));
	AddPropItem("crMode", PROPLIST_ITEM(id++, "颜色方案"));
	AddPropItem("crMode.crEdge", PROPLIST_ITEM(id++, "轮廓边颜色"));
	AddPropItem("crMode.crLS12", PROPLIST_ITEM(id++, "M12孔径颜色"));
	AddPropItem("crMode.crLS16", PROPLIST_ITEM(id++, "M16孔径颜色"));
	AddPropItem("crMode.crLS20", PROPLIST_ITEM(id++, "M20孔径颜色"));
	AddPropItem("crMode.crLS24", PROPLIST_ITEM(id++, "M24孔径颜色"));
	AddPropItem("crMode.crOtherLS", PROPLIST_ITEM(id++, "其他孔径颜色"));
	return id;
}
int CPNCSysPara::GetPropValueStr(long id, char* valueStr, UINT nMaxStrBufLen/*=100*/, CPropTreeItem *pItem/*=NULL*/)
{
	CXhChar100 sText;
	if (GetPropID("m_bIncDeformed") == id)
	{
		if (m_bIncDeformed)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("m_bUseMaxEdge") == id)
	{
		if (m_bUseMaxEdge)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("m_nMaxEdgeLen") == id)
		sText.Printf("%d", m_nMaxEdgeLen);
	else if (GetPropID("m_iPPiMode") == id)
	{
		if (m_iPPiMode == 0)
			sText.Copy("0.一板一号");
		else if (m_iPPiMode == 1)
			sText.Copy("1.一板多号");
	}
	else if (GetPropID("AxisXCalType") == id)
	{
		if (m_iAxisXCalType == 0)
			sText.Copy("0.最长边优先");
		else if (m_iAxisXCalType == 1)
			sText.Copy("1.螺栓平行边优先");
		else if (m_iAxisXCalType == 2)
			sText.Copy("2.焊接边优先");
	}
	else if (GetPropID("m_ciMKPos") == id)
	{
		if (m_ciMKPos == 1)
			sText.Copy("1.钢印字盒块");
		else if (m_ciMKPos == 2)
			sText.Copy("2.钢印号位孔");
		else
			sText.Copy("0.件号文字标注");
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
			sText.Copy("钢板对比");
		else if (m_ciLayoutMode == 2)
			sText.Copy("自动排版");
		else if (m_ciLayoutMode == 3)
			sText.Copy("下料预审");
		else if (m_ciLayoutMode == 4)
			sText.Copy("图元筛选");
		else
			sText.Copy("图元克隆");
	}
	else if (GetPropID("m_ciArrangeType") == id)
	{
		if (m_ciArrangeType == 0)
			sText.Copy("0.以行为主");
		else
			sText.Copy("1.以列为主");
	}
	else if (GetPropID("m_ciGroupType") == id)
	{
		if (m_ciGroupType == 1)
			sText.Copy("1.按段号");
		else if (m_ciGroupType == 2)
			sText.Copy("2.材质&厚度");
		else if (m_ciGroupType == 3)
			sText.Copy("3.材质");
		else if (m_ciGroupType == 4)
			sText.Copy("4.厚度");
		else
			sText.Copy("0.不分组");
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
			sText.Copy("1.显示所有档板");
		else
			sText.Copy("0.仅显示选中钢板档板");
	}
	else if (GetPropID("CDrawDamBoard::BOARD_HEIGHT") == id)
		sText.Printf("%d", CDrawDamBoard::BOARD_HEIGHT);
#endif
	else if (GetPropID("m_nMkRectWidth") == id)
		sText.Printf("%d", m_nMkRectWidth);
	else if (GetPropID("m_nMkRectLen") == id)
		sText.Printf("%d", m_nMkRectLen);
	else if (GetPropID("m_nMkCircleRadius") == id)
		sText.Printf("%d", m_nMkCircleRadius);
	else if (GetPropID("layer_mode") == id)
	{
		if (m_ciLayerMode == 0)
			sText.Copy("0.指定轮廓边图层");
		else if (m_ciLayerMode == 1)
			sText.Copy("1.过滤默认图层");
	}
	else if (GetPropID("m_iRecogMode") == id)
	{
		if (m_ciRecogMode == FILTER_BY_LINETYPE)
			sText.Copy("0.按线型识别");
		else if (m_ciRecogMode == FILTER_BY_LAYER)
			sText.Copy("1.按图层识别");
		else if (m_ciRecogMode == FILTER_BY_COLOR)
			sText.Copy("2.按颜色识别");
		else if (m_ciRecogMode == FILTER_BY_PIXEL)
			sText.Copy("3.智能识别");
	}
	else if (GetPropID("m_fPixelScale") == id)
	{
		sText.Printf("%f", m_fPixelScale);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("FilterPartNoCir") == id)
	{
		if (IsFilterPartNoCir())
			sText.Copy("过滤");
		else
			sText.Copy("不过滤");
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
	else if(GetPropID("standard_SJ") == id)
		sText.Printf("%g", standard_hole.m_fLS_SJ);
	else if (GetPropID("standard_ZF") == id)
		sText.Printf("%g", standard_hole.m_fLS_ZF);
	else if (GetPropID("standard_YY") == id)
		sText.Printf("%g", standard_hole.m_fLS_YY);
	else if (GetPropID("RecogLsBlock") == id)
		sText.Copy("根据图符设置处理");
	else if (GetPropID("RecogHoleDimText") == id)
	{
		if (IsRecogHoleDimText())
			sText.Copy("按标注处理");
		else
			sText.Copy("不进行处理");
	}
	else if (GetPropID("RecogLsCircle") == id)
	{
		if (IsRecogCirByBoltD())
			sText.Copy("根据标准孔进行筛选");
		else
			sText.Copy("统一按特殊孔处理");
	}
	else if (GetPropID("RecogLsPolyline") == id)
	{
		if (m_ciPolylineLsMode == 0)
			sText.Copy("不进行处理");
		else if (m_ciPolylineLsMode == 1)
			sText.Copy("自动计算孔径");
		else
			sText.Copy("指定标准孔径");
	}
	else if(GetPropID("RoundLineLen")==id)
		sText.Printf("%g", m_fRoundLineLen);
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
		if(m_ciLayerMode==0&&m_xHashEdgeKeepLayers.GetNodeNum()>0)
		{	//用户指定轮廓边所在图层
			LAYER_ITEM* pItem=GetEdgeLayerItem(sLayer);
			if(pItem && pItem->m_bMark)
				return FALSE;
			else
				return TRUE;
		}
		else if(m_ciLayerMode==1&&m_xHashDefaultFilterLayers.GetValue(sLayer))
			return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::IsProfileEnt(AcDbEntity* pEnt)
{
	if (pEnt == NULL)
		return FALSE;
	if (pEnt->isKindOf(AcDbLine::desc()))
		return TRUE;	//直线
	if (pEnt->isKindOf(AcDbArc::desc()))
		return TRUE;	//圆弧
	if (pEnt->isKindOf(AcDbEllipse::desc()))
		return TRUE;	//椭圆弧
	if (pEnt->isKindOf(AcDbPolyline::desc()))
		return TRUE;	//面域中包括轮廓边
	if (pEnt->isKindOf(AcDbRegion::desc()))
		return TRUE;	//多线段
	if (pEnt->isKindOf(AcDbCircle::desc()))
	{	//智能提取时，处理圆板
		AcDbCircle* pCir = (AcDbCircle*)pEnt;
		if (pCir->radius() * 2 > m_nMaxHoleD)
			return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=CPlateRecogRule::IsBendLine(pAcDbLine,pRecognizer);
	if (!bRet && m_ciBendLineColorIndex > 0 && m_ciBendLineColorIndex < 255)
		bRet = (GetEntColorIndex(pAcDbLine) == m_ciBendLineColorIndex);
	return bRet;
}

//螺栓图符主要有：圆形、三角形、方形
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
		BOLT_BLOCK* pBoltD = GetBlotBlockByName(sName);
		if (pBoltD == NULL)
		{	//只对设置未螺栓图符的块进行处理，否则可能错误识别其它块为螺栓孔 wht 20-04-28
			return FALSE;
		}
		CBoltEntGroup* pLsBlockEnt = CPlateExtractor::GetExtractor()->FindBoltGroup(CXhChar50("%d", pEnt->id().asOldId()));
		double 	fHoleD = pLsBlockEnt ? pLsBlockEnt->m_fHoleD : 0;
		if (fHoleD <= 0)
		{
			logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "根据螺栓图符{%s}计算孔径失败！", (char*)sName);
			return FALSE;
		}
		BOOL bValidLsBlock = TRUE;
		double fIncrement = (pBoltD->diameter > 0) ? fHoleD - pBoltD->diameter : 0;
		if (fIncrement > 2 || fIncrement < 0)
		{
			bValidLsBlock = FALSE;
			//根据图元计算得到的直径，螺栓对应的直径不符且未指定螺栓孔径时提示用户 wht 20-08-14
			if (pBoltD->hole_d <= 0 || pBoltD->hole_d < pBoltD->diameter)
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "根据螺栓图符{%s}计算的孔径{%.1f}不合理，请优先设置螺栓图块对应的孔径！", (char*)sName, fHoleD);
		}
		//
		hole.posX = (float)pReference->position().x;
		hole.posY = (float)pReference->position().y;
		if (pBoltD->diameter > 0)
		{	//指定螺栓块的直径，按照标准螺栓处理
			hole.ciSymbolType = 0;
			hole.d = pBoltD->diameter;
			if (!bValidLsBlock && pBoltD->hole_d > pBoltD->diameter)
				hole.increment = (float)(pBoltD->hole_d - pBoltD->diameter);
			else
				hole.increment = (float)(fHoleD - pBoltD->diameter);
		}
		else
		{	//未指定螺栓块的直径，按特殊孔处理
			hole.ciSymbolType = 1;	//特殊图块
			hole.d = fHoleD;
			hole.increment = 0;
		}
		return TRUE;
	}
	else if (pEnt->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle = (AcDbCircle*)pEnt;
		if (int(pCircle->radius()) <= 0)
			return FALSE;	//去除点
		double fDiameter = pCircle->radius() * 2;
		if (g_pncSysPara.m_ciMKPos == 2 &&
			fabs(g_pncSysPara.m_fMKHoleD - fDiameter) < EPS2)
			return FALSE;	//去除号位孔
		/* 特殊孔直径直接设置直径，不设孔径增大值，
		/*否则在PPE中统一处理孔径增大值时会丢失此处提取的孔径增大值 wht 19-09-12
		/*对孔径进行圆整，精确到小数点一位
		*/
		GEPOINT center(pCircle->center().x, pCircle->center().y, pCircle->center().z);
		CBoltEntGroup* pLsBlockEnt = CPlateExtractor::GetExtractor()->FindBoltGroup(IExtractor::MakePosKeyStr(center));
		if (pLsBlockEnt == NULL)
		{
			logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "丢失圆孔位置(%s)的螺栓", IExtractor::MakePosKeyStr(center));
			return FALSE;
		}
		hole.increment = 0;
		hole.ciSymbolType = 2;	//默认挂线孔
		hole.d = pLsBlockEnt->m_fHoleD;
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
	//从块中解析钢板信息
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
			if (sTag.EqualNoCase("件号&规格&材质"))
				bRetCode = RecogBasicInfo(pAttr, basicInfo);
			else if (sTag.EqualNoCase("数量"))
			{
				CXhChar50 sTemp(sText);
				for (char* token = strtok(sTemp, "X="); token; token = strtok(NULL, "X="))
				{
					CXhChar16 sToken(token);
					if (sToken.Replace("块", "") > 0)
						basicInfo.m_nNum = atoi(sToken);
				}
			}
			else if (sTag.EqualNoCase("塔型"))
				basicInfo.m_sTaType.Copy(sText);
			else if (sTag.EqualNoCase("钢印"))
			{
				sText.Replace("钢印", "");
				sText.Replace(":", "");
				sText.Replace("：", "");
				basicInfo.m_sTaStampNo.Copy(sText);
			}
			else if (sTag.EqualNoCase("工程代码"))
			{
				sText.Replace("工程代码", "");
				sText.Replace(":", "");
				sText.Replace("：", "");
				basicInfo.m_sPrjCode.Copy(sText);
			}
		}
		return bRetCode;
	}
	//从字符串中解析钢板信息
	BOOL bRet = FALSE;
	vector<CString> lineList;
	SplitMultiText(pEnt, lineList);
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
			//记录构件数量对应的实体Id wht 19-08-05
			basicInfo.m_idCadEntNum = pEnt->id().asOldId();
			bRet = TRUE;
		}
		if (IsMatchPNRule(sTemp))
		{
			ParsePartNoText(sTemp, basicInfo.m_sPartNo);
			bRet = TRUE;
		}
		if (strstr(sTemp, "塔型"))
		{
			sTemp.Replace("塔型", "");
			sTemp.Replace(":", "");
			sTemp.Replace("：", "");
			basicInfo.m_sTaType.Copy(sTemp);
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
		{	//过滤错误的圆弧(例如：有时pEnt是一个点,但是属性显示为圆弧)
			//保证startPt-endPt不重叠 wht 19-11-11
			ciEdgeType = 2;
			return arcLine.CreateMethod3(startPt, endPt, norm, radius, center);
		}
	}
	else if (pEnt->isKindOf(AcDbEllipse::desc()))
	{	//更新钢板顶点参数(椭圆)
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
		return FALSE;	//不需要提取钢印号
	if (m_ciMKPos == 1)
	{	//钢印矩形块
		if (pEnt->isKindOf(AcDbText::desc()))
		{	//获取钢印区
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			CXhChar50 sText(_bstr_t(pText->textString()));
#else
			CXhChar50 sText(pText->textString());
#endif
			if (!sText.Equal("钢印区"))
				return FALSE;
			double len = DrawTextLength(sText, pText->height(), pText->textStyle());
			f3dPoint dim_vec(cos(pText->rotation()), sin(pText->rotation()));
			f3dPoint origin(pText->position().x, pText->position().y, pText->position().z);
			origin += dim_vec * len*0.5;
			f2dRect rect;
			rect.topLeft.Set(origin.x - 10, origin.y + 10);
			rect.bottomRight.Set(origin.x + 10, origin.y - 10);
			ZoomAcadView(rect, 200);		//对钢印区进行适当缩放
			ads_name seqent;
			AcDbObjectId initLastObjId, plineId;
			acdbEntLast(seqent);
			acdbGetObjectId(initLastObjId, seqent);
			ads_point base_pnt;
			base_pnt[X] = origin.x;
			base_pnt[Y] = origin.y;
			base_pnt[Z] = origin.z;
			int resCode = RTNORM;
			if (IExtractor::m_bSendCommand)
			{
				CXhChar50 sCmd, sPos("%.2f,%.2f", origin.x, origin.y);
				sCmd.Printf("-boundary a i n\n \n%s\n ", (char*)sPos);
#ifdef _ARX_2007
				SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
				SendCommandToCad(sCmd);
#endif
			}
			else
			{
#ifdef _ARX_2007
				resCode = acedCommand(RTSTR, L"-boundary", RTSTR, L"a", RTSTR, L"i", RTSTR, L"n", RTSTR, L"", RTSTR, L"", RTPOINT, base_pnt, RTSTR, L"", RTNONE);
#else
				resCode = acedCommand(RTSTR, "-boundary", RTSTR, "a", RTSTR, "i", RTSTR, "n", RTSTR, "", RTSTR, "", RTPOINT, base_pnt, RTSTR, "", RTNONE);
#endif		
			}
			if (resCode != RTNORM)
				return FALSE;
			acdbEntLast(seqent);
			acdbGetObjectId(plineId, seqent);
			if (initLastObjId == plineId)
			{
				if (IExtractor::m_bSendCommand)
#ifdef _ARX_2007
					SendCommandToCad(L" \n ");
#else
					SendCommandToCad(" \n ");
#endif
				else
					acedCommand(RTSTR, "");
				return FALSE;
			}
			AcDbEntity *pEnt = NULL;
			XhAcdbOpenAcDbEntity(pEnt, plineId, AcDb::kForWrite);
			AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
			if (pPline == NULL || pPline->numVerts() != nNum)
			{
				if (pPline)
				{
					pPline->erase(Adesk::kTrue);
					pPline->close();
				}
				if (IExtractor::m_bSendCommand)
#ifdef _ARX_2007
					SendCommandToCad(L" \n ");
#else
					SendCommandToCad(" \n ");
#endif
				else
					acedCommand(RTSTR, "");
				return FALSE;
			}
			AcGePoint3d location;
			for (int iVertIndex = 0; iVertIndex < nNum; iVertIndex++)
			{
				pPline->getPointAt(iVertIndex, location);
				ptArr[iVertIndex].Set(location.x, location.y, location.z);
			}
			pPline->erase(Adesk::kTrue);	//删除polyline对象
			pPline->close();
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//钢印区图块
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
					AcDbPolyline* pPolyLine = (AcDbPolyline*)pEnt;
					if (pPolyLine->numVerts() != nNum)
						continue;
					for (int iVertIndex = 0; iVertIndex < nNum; iVertIndex++)
					{
						AcGePoint3d location;
						pPolyLine->getPointAt(iVertIndex, location);
						ptArr[iVertIndex].Set(location.x, location.y, 0);
						ptArr[iVertIndex].x *= scaleXYZ.sx;
						ptArr[iVertIndex].y *= scaleXYZ.sy;
					}
					break;
				}
			}
			pTempBlockTableRecord->close();
			//更新钢印区实际坐标
			GEPOINT norm = scaleXYZ.sz < 0 ? f3dPoint(0, 0, -1) : f3dPoint(0, 0, 1);
			for (int i = 0; i < nNum; i++)
			{
				if (scaleXYZ.sz < 0)		//图块有翻转，需调整钢印区四个轮廓点坐标
					ptArr[i].x *= -1;		
				if (fabs(rot_angle) > 0)	//图块有旋转角度
					rotate_point_around_axis(ptArr[i], rot_angle, f3dPoint(), 100 * norm);
				ptArr[i] += orig;
			}
		}
		else
			return FALSE;
	}
	else if (m_ciMKPos == 2)
	{	//号位孔
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
BOOL CPNCSysPara::IsHasPlateWeldTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "焊") || strstr(sText, "weld"))
		return TRUE;
	return FALSE;
}
//
BOOL CPNCSysPara::IsHasPlateBendTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "卷边") || strstr(sText, "火曲") ||
		strstr(sText, "外曲") || strstr(sText, "内曲") ||
		strstr(sText, "正曲") || strstr(sText, "反曲"))
		return TRUE;
	return FALSE;
}
#ifdef __UBOM_ONLY_
BOOL CPNCSysPara::IsHasFootNailTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	CXhChar200 sTemp(sText);
	if (strstr(sText, "带脚钉"))
		return TRUE;
	else if (strstr(sText, "脚钉"))
	{
		for (char *skey = strtok((char*)sTemp, " ,，、"); skey; skey = strtok(NULL, " ,，、"))
		{
			if (stricmp(skey, "脚钉") == 0)
				return TRUE;
		}
	}
	return FALSE;
}
BOOL CPNCSysPara::IsHasAngleWeldTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "主焊件") || 
		strstr(sText, "焊于") ||
		strstr(sText, "焊接")||
		strstr(sText, "电焊"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasAngleBendTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "制弯") || strstr(sText, "火曲") ||
		strstr(sText, "外曲") || strstr(sText, "内曲") ||
		strstr(sText, "正曲") || strstr(sText, "反曲") ||
		strstr(sText, "棱线夹角") || strstr(sText, "肢面夹角"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasCutAngleTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "切角") || strstr(sText, "切肢"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasCutRootTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "清根") || strstr(sText, "刨根") ||
		strstr(sText, "铲芯") || strstr(sText, "铲心") ||
		strstr(sText, "刨角"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasCutBerTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "铲背"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasKaiJiaoTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "开角"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasHeJiaoTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "合角"))
		return TRUE;
	return FALSE;
}
BOOL CPNCSysPara::IsHasPushFlatTag(const char* sText)
{
	if (sText == NULL || strlen(sText) <= 0)
		return FALSE;
	if (strstr(sText, "压扁") ||
		strstr(sText, "打扁") ||
		strstr(sText, "拍扁"))
		return TRUE;
	return FALSE;
}
#endif
//////////////////////////////////////////////////////////////////////////
//
void PNCSysSetImportDefault()
{
	char file_name[MAX_PATH] = "";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE* fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
	PNCSysSetImportDefault(fp);
	fclose(fp);
}

bool PNCSysSetImportDefault(FILE *fp)
{
	if (fp == NULL)
		return false;
	int nTemp = 0;
	g_pncSysPara.EmptyBoltBlockRecog();
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
			if (sText.Equal("是"))
				g_pncSysPara.m_bIncDeformed = true;
			else
				g_pncSysPara.m_bIncDeformed = false;
		}
		else if (_stricmp(key_word, "m_bUseMaxEdge") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_bUseMaxEdge);
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
		else if (_stricmp(key_word, "m_nMkCircleRadius") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkCircleRadius);
		else if (_stricmp(key_word, "standard_SJ") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.standard_hole.m_fLS_SJ = atof(skey);
		}
		else if (_stricmp(key_word, "standard_ZF") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.standard_hole.m_fLS_ZF = atof(skey);
		}
			
		else if (_stricmp(key_word, "standard_YY") == 0)
		{
			skey = strtok(NULL, "=,;");
			g_pncSysPara.standard_hole.m_fLS_YY = atof(skey);
		}
			
#ifndef __UBOM_ONLY_
		else if (_stricmp(key_word, "CDrawDamBoard::BOARD_HEIGHT") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::BOARD_HEIGHT);
		else if (_stricmp(key_word, "CDrawDamBoard::m_bDrawAllBamBoard") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::m_bDrawAllBamBoard);
#endif
		else if (_stricmp(key_word, "RecogMode") == 0)
		{	//使用%d读取BYTE型变量，一次更新4个字节影响其它变量取值
			//可使用%hhu读取BYTE变量但测试无效，先读取到临时变量中，再赋值到相应变量中 wht 19-10-05
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciRecogMode = nValue;
		}
		else if (_stricmp(key_word, "bIncFilterLayer") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nValue);
			g_pncSysPara.m_ciLayerMode = nValue;
		}
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
		else if (_stricmp(key_word, "RoundLineLen") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey)
				g_pncSysPara.m_fRoundLineLen = atof(skey);
		}
		else if (_stricmp(key_word, "MapScale") == 0)
			sscanf(line_txt, "%s%f", key_word, &g_pncSysPara.m_fMapScale);
		else if (_stricmp(key_word, "BoltDKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			CString skeyName(skey);
			if (strlen(skey) > 0)
			{
				BOLT_BLOCK *pBoltD = g_pncSysPara.AddBoltBlock(skey);
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
	//加载配置文件后激活当前识别模型 wht 19-10-30
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
	return true;
}
void PNCSysSetExportDefault()
{
	char file_name[MAX_PATH] = "";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE* fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("打不开指定的配置文件!");
		return;
	}
	PNCSysSetExportDefault(fp);
	fclose(fp);
}

bool PNCSysSetExportDefault(FILE *fp)
{
	if (fp == NULL)
		return false;
	fprintf(fp, "基本设置\n");
	fprintf(fp, "bIncDeformed=%s ;考虑火曲变形量\n", g_pncSysPara.m_bIncDeformed ? "是" : "否");
	fprintf(fp, "m_bUseMaxEdge=%d ;启用最大边长\n", g_pncSysPara.m_bUseMaxEdge);
	fprintf(fp, "m_nMaxEdgeLen=%d ;最长边长\n", g_pncSysPara.m_nMaxEdgeLen);
	fprintf(fp, "m_nMaxHoleD=%d ;最大螺栓孔\n", g_pncSysPara.m_nMaxHoleD);
	fprintf(fp, "MKPos=%d ;提取钢印位置\n", g_pncSysPara.m_ciMKPos);
	fprintf(fp, "MKHole=%g ;号位孔直径\n", g_pncSysPara.m_fMKHoleD);
	fprintf(fp, "AxisXCalType=%d ;X轴计算方式\n", g_pncSysPara.m_iAxisXCalType);
	fprintf(fp, "PPIMode=%d ;PPI文件模式\n", g_pncSysPara.m_iPPiMode);
	fprintf(fp, "MapScale=%.f ;缩放比例\n", g_pncSysPara.m_fMapScale);
	fprintf(fp, "显示设置\n");
	fprintf(fp, "LayoutMode=%d ;显示模式\n", g_pncSysPara.m_ciLayoutMode);
	fprintf(fp, "ArrangeType=%d ;对比布局方案\n", g_pncSysPara.m_ciArrangeType);
	fprintf(fp, "GroupType=%d ;绘制分组方式\n", g_pncSysPara.m_ciGroupType);
	fprintf(fp, "MapWidth=%d ;图纸宽度\n", g_pncSysPara.m_nMapWidth);
	fprintf(fp, "MapLength=%d ;图纸长度\n", g_pncSysPara.m_nMapLength);
	fprintf(fp, "MinDistance=%d ;最小间距\n", g_pncSysPara.m_nMinDistance);
	fprintf(fp, "m_nMkRectWidth=%d ;字盒宽度\n", g_pncSysPara.m_nMkRectWidth);
	fprintf(fp, "m_nMkRectLen=%d ;字盒长度\n", g_pncSysPara.m_nMkRectLen);
	fprintf(fp, "m_nMkCircleRadius=%d ;圆半径\n", g_pncSysPara.m_nMkCircleRadius);
	fprintf(fp, "standard_SJ=%.1f ;三角螺栓孔径\n", g_pncSysPara.standard_hole.m_fLS_SJ);
	fprintf(fp, "standard_ZF=%.1f ;方形螺栓孔径\n", g_pncSysPara.standard_hole.m_fLS_ZF);
	fprintf(fp, "standard_YY=%.1f ;腰圆螺栓孔径\n", g_pncSysPara.standard_hole.m_fLS_YY);
#ifndef __UBOM_ONLY_
	fprintf(fp, "CDrawDamBoard::BOARD_HEIGHT=%d ;档板高度\n", CDrawDamBoard::BOARD_HEIGHT);
	fprintf(fp, "CDrawDamBoard::m_bDrawAllBamBoard=%d ;绘制所有档板\n", CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	fprintf(fp, "识别设置\n");
	fprintf(fp, "RecogMode=%d ;识别模式\n", g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "bIncFilterLayer=%d ;启用过滤默认图层\n", g_pncSysPara.m_ciLayerMode);
	fprintf(fp, "ProfileLineType=%s ;轮廓边线型\n", (char*)g_pncSysPara.m_sProfileLineType);
	fprintf(fp, "PixelScale=%.1f ;图像比例\n", g_pncSysPara.m_fPixelScale);
	fprintf(fp, "ProfileColorIndex=%d ;轮廓边颜色\n", g_pncSysPara.m_ciProfileColorIndex);
	fprintf(fp, "BendLineColorIndex=%d ;制弯线颜色\n", g_pncSysPara.m_ciBendLineColorIndex);
	fprintf(fp, "BoltRecogMode=%d ;螺栓识别模式\n", g_pncSysPara.m_ciBoltRecogMode);
	fprintf(fp, "PartNoCirD=%.1f ;件号圆圈直径\n", g_pncSysPara.m_fPartNoCirD);
	fprintf(fp, "RoundLineLen=%.1f ;腰圆过渡长度\n", g_pncSysPara.m_fRoundLineLen);
	fprintf(fp, "螺栓识别设置\n");
	for (BOLT_BLOCK *pBoltBlock = g_pncSysPara.EnumFirstBlotBlock(); pBoltBlock; pBoltBlock = g_pncSysPara.EnumNextBlotBlock())
	{
		fprintf(fp, "BoltDKey=%s;图块名称;螺栓直径\n", (char*)g_pncSysPara.GetBoltBlockCurKey());
		if (!strcmp(pBoltBlock->sGroupName, ""))
			fprintf(fp, "%s;", " ");
		else
			fprintf(fp, "%s;", (char*)pBoltBlock->sGroupName);
		fprintf(fp, "%s;", (char*)pBoltBlock->sBlockName);
		fprintf(fp, "%d;", pBoltBlock->diameter);
		fprintf(fp, "%.1f\n", pBoltBlock->hole_d);
	}
	fprintf(fp, "文字识别设置\n");
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
	fprintf(fp, "比例识别\n");
	fprintf(fp, "MapScale=%.f ;缩放比例\n", g_pncSysPara.m_fMapScale);
	return true;
}


static int SplitPathStrToArr(CString& sPath, CStringArray& strPathArr)
{
	if (!strPathArr.IsEmpty())
		strPathArr.RemoveAll();
	int i = 0, i2 = 0;
	while ((i2 = sPath.Find(';', i)) != -1)
	{
		strPathArr.Add(sPath.Mid(i, i2 - i));
		i = i2 + 1;
	}
	if (i < sPath.GetLength())
		strPathArr.Add(sPath.Right(sPath.GetLength() - i));
	return strPathArr.GetSize();
}

static CString PathArrToString(CStringArray& strArr)
{
	CString sPath = "";
	for (int i = 0; i < strArr.GetSize(); i++)
	{
		if (sPath.GetLength() > 0)
			sPath.Append(";");
		sPath.Append(strArr[i]);
	}
	return sPath;
}

CXhChar500 CPNCSysPara::GetCurPlateCardFileName()
{
	CXhChar500 sCurPath;
	CStringArray strPathArr;
	if (SplitPathStrToArr(CString(m_sPlateCardFileName), strPathArr) > 0)
		sCurPath.Copy(strPathArr[0]);
	return sCurPath;
}

CXhChar500 CPNCSysPara::SetCurPlateCardFileName(const char* file_name)
{
	CStringArray destPathArr;
	if (file_name != NULL)
		destPathArr.Add(file_name);
	CStringArray strPathArr;
	if (SplitPathStrToArr(CString(m_sPlateCardFileName), strPathArr) > 0)
	{
		for (int i = 0; i < strPathArr.GetSize(); i++)
		{
			if (strPathArr[i].CompareNoCase(file_name) == 0)
				continue;
			destPathArr.Add(CXhChar500(strPathArr[i]));
		}
	}
	CString sPath = PathArrToString(destPathArr);
	m_sPlateCardFileName = sPath;
	return CXhChar500(sPath);
}
#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//读取UBOM配置信息
#include "BatchPrint.h"

static char* InitBomTblTitleCfg(char* skey, CBomTblTitleCfg *pBomTblCfg)
{
	if (skey == NULL)
		return NULL;
	skey = strtok(NULL, ";");
	if (skey != NULL && strlen(skey) > 0 && pBomTblCfg)
	{
		pBomTblCfg->m_sColIndexArr.Copy(skey);
		pBomTblCfg->m_sColIndexArr.Replace(" ", "");
		//读取列数
		skey = strtok(NULL, ";");
		if (skey != NULL)
			pBomTblCfg->m_nColCount = atoi(skey);
		//内容起始行
		skey = strtok(NULL, ";");
		if (skey != NULL)
			pBomTblCfg->m_nStartRow = atoi(skey);
	}
	return skey;
}
//遍历记录所有用户配置
void TraversalUbomConfigFiles()
{
	char APP_PATH[MAX_PATH] = "";
	GetAppPath(APP_PATH);
	CFileFind file_find;
	BOOL bFind = file_find.FindFile(CXhChar200("%s\\配置\\*.cfg", APP_PATH));
	while (bFind)
	{
		bFind = file_find.FindNextFile();
		if (file_find.IsDots() || file_find.IsHidden() || file_find.IsReadOnly() ||
			file_find.IsSystem() || file_find.IsTemporary() || file_find.IsDirectory())
			continue;
		CString file_path = file_find.GetFilePath();
		CString file_name = file_find.GetFileName();
		CString str_ext = file_path.Right(4);	//取后缀名
		str_ext.MakeLower();
		if (str_ext.CompareNoCase(".cfg") != 0)
			continue;
		FILE* fp = fopen(file_path, "rt");
		if (fp == NULL)
			continue;
		//读取文件，获取客户ID
		int nClientId = 0;
		CString sClientName;
		char line_txt[MAX_PATH] = "";
		while (!feof(fp))
		{
			if (fgets(line_txt, MAX_PATH, fp) == NULL)
				break;
			CString sLine(line_txt);
			sLine.Remove('\n');
			sLine.Remove('\t');
			sprintf(line_txt, "%s", sLine);
			char *skey = strtok(line_txt, "=");
			CString key_word(skey);
			key_word.Remove(' ');
			if (_stricmp(key_word, "CLIENT_ID") == 0)
			{
				skey = strtok(NULL, ";");
				nClientId = atoi(skey);
			}
			else if (_stricmp(key_word, "CLIENT_NAME") == 0)
			{
				skey = strtok(NULL, ";");
				sClientName = skey;
			}
			if (nClientId > 0 && sClientName.GetLength() > 0)
			{
				std::pair<CString, CString> item = std::make_pair(sClientName, file_name);
				g_xUbomModel.m_xMapClientCfgFile.insert(std::make_pair(nClientId, item));
				break;
			}
		}
		fclose(fp);
	}
	file_find.Close();
}
//初始化配置
void ImportUbomConfigFile(const char* file_path/*=NULL*/)
{
	FILE *fp = NULL;
	if (file_path && strlen(file_path) > 0)
		fp = fopen(file_path, "rt");
	if (fp == NULL)
	{	//读取默认的配置文件
		char file_name[MAX_PATH] = "";
		GetAppPath(file_name);
		strcat(file_name, "ubom.cfg");
		fp = fopen(file_name, "rt");
	}
	if (fp == NULL)
		return;
	g_xUbomModel.InitBomModel();
	g_xBomCfg.Init();
	g_pncSysPara.EmptyBoltBlockRecog();
	g_pncSysPara.m_recogSchemaList.Empty();
	char line_txt[MAX_PATH] = "";
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		CString sLine(line_txt);
		sLine.Remove('\n');
		sLine.Remove('\t');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok(line_txt, "=");
		CString key_word(skey);
		key_word.Remove(' ');
		if (_stricmp(key_word, "CLIENT_ID") == 0)
		{
			skey = strtok(NULL, ";");
			g_xUbomModel.m_uiCustomizeSerial = atoi(skey);
		}
		else if (_stricmp(key_word, "CLIENT_NAME") == 0)
		{
			skey = strtok(NULL, ";");
			g_xUbomModel.m_sCustomizeName.Copy(skey);
		}
		else if (_stricmp(key_word, "FUNC_FLAG") == 0)
		{
			skey = strtok(NULL, ";");
			sscanf(skey, "%X", &g_xUbomModel.m_dwFunctionFlag);
		}
		else if (_stricmp(key_word, "ExeRppWhenArxLoad") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_bExeRppWhenArxLoad = atoi(skey);
		}
		else if (_stricmp(key_word, "AutoExtractPlates") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_bExtractPltesWhenOpenFile = atoi(skey);
		}
		else if (_stricmp(key_word, "AutoExtractAngles") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_bExtractAnglesWhenOpenFile = atoi(skey);
		}
		else if (_stricmp(key_word, "NOT_PRINT") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_sNotPrintFilter.Copy(skey);
		}
		else if (_stricmp(key_word, "PrintSortType") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_ciPrintSortType = atoi(skey);
		}
		else if (_stricmp(key_word, "MaxLenErr") == 0)
		{
			skey = strtok(NULL, ";");
			g_xUbomModel.m_fMaxLenErr = atof(skey);
		}
		else if (_stricmp(key_word, "JG_CARD") == 0)
		{
			skey = strtok(NULL, ";");
			g_xUbomModel.m_sJgCadName.Copy(skey);
			g_xUbomModel.m_sJgCadName.Remove(' ');
		}
		else if (_stricmp(key_word, "JgCadPartLabel") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 1)
			{
				g_xUbomModel.m_sJgCadPartLabel.Copy(skey);
				g_xUbomModel.m_sJgCadPartLabel.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "JgCadPartLabelMat") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
				g_xUbomModel.m_uiJgCadPartLabelMat = atoi(skey);
		}
		else if (_stricmp(key_word, "JgCardBlockName") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 1)
			{
				g_xUbomModel.m_sJgCardBlockName.Copy(skey);
				g_xUbomModel.m_sJgCardBlockName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PlateRecogMode") == 0)
		{
			skey = strtok(NULL, ";");
			g_pncSysPara.m_ciRecogMode = atoi(skey);
		}
		else if (_stricmp(key_word, "PlateDimStyle") == 0)
		{
			skey = strtok(NULL, ";");
			if (strlen(skey) > 0)
			{
				RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.append();
				pSchema->m_iDimStyle = atoi(skey);
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
		else if (_stricmp(key_word, "TMA_BOM") == 0)
			InitBomTblTitleCfg(skey, &g_xBomCfg.m_xTmaTblCfg.m_xTblCfg);
		else if (_stricmp(key_word, "ERP_BOM") == 0)
			InitBomTblTitleCfg(skey, &g_xBomCfg.m_xErpTblCfg.m_xTblCfg);
		else if (_stricmp(key_word, "PRINT_BOM") == 0 || _stricmp(key_word, "JG_BOM") == 0)
			InitBomTblTitleCfg(skey, &g_xBomCfg.m_xPrintTblCfg.m_xTblCfg);
		else if (_stricmp(key_word, "TMABomFileKeyStr") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xTmaTblCfg.m_sFileTypeKeyStr.Copy(skey);
				g_xBomCfg.m_xTmaTblCfg.m_sFileTypeKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomFileKeyStr") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xErpTblCfg.m_sFileTypeKeyStr.Copy(skey);
				g_xBomCfg.m_xErpTblCfg.m_sFileTypeKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PrintBomFileKeyStr") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xPrintTblCfg.m_sFileTypeKeyStr.Copy(skey);
				g_xBomCfg.m_xPrintTblCfg.m_sFileTypeKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "TMABomSheetName") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xTmaTblCfg.m_sBomSheetName.Copy(skey);
				g_xBomCfg.m_xTmaTblCfg.m_sBomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomSheetName") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xErpTblCfg.m_sBomSheetName.Copy(skey);
				g_xBomCfg.m_xErpTblCfg.m_sBomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "JGPrintSheetName") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_xPrintTblCfg.m_sBomSheetName.Copy(skey);
				g_xBomCfg.m_xPrintTblCfg.m_sBomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ANGLE_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sAngleCompareItemArr.Copy(skey);
				g_xBomCfg.m_sAngleCompareItemArr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PLATE_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sPlateCompareItemArr.Copy(skey);
				g_xBomCfg.m_sPlateCompareItemArr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ZHIWAN_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_ZHI_WAN].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_ZHI_WAN].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_ANGLE_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ANGLE].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ANGLE].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_ROOT_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ROOT].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ROOT].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_BER_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_BER].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_BER].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PUSH_FLAT_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_PUSH_FLAT].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_PUSH_FLAT].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "KAI_JIAO_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_KAI_JIAO].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_KAI_JIAO].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "HE_JIAO_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_HE_JIAO].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_HE_JIAO].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "FOO_NAIL_ITEM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_FOO_NAIL].Copy(skey);
				g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_FOO_NAIL].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ProcessFlag") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				g_xBomCfg.m_sProcessFlag.Copy(skey);
				g_xBomCfg.m_sProcessFlag.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "Paper.DeviceName") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				CBatchPrint::m_xPaperPlotCfg.m_sDeviceName.Copy(skey);
				CBatchPrint::m_xPaperPlotCfg.m_sDeviceName.TrimLeft();
				CBatchPrint::m_xPaperPlotCfg.m_sDeviceName.TrimRight();
			}
		}
		else if (_stricmp(key_word, "Paper.PaperSize") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL)
			{
				CBatchPrint::m_xPaperPlotCfg.m_sPaperSize.Copy(skey);
				CBatchPrint::m_xPaperPlotCfg.m_sPaperSize.TrimLeft();
				CBatchPrint::m_xPaperPlotCfg.m_sPaperSize.TrimRight();
			}
		}
		else
		{
			for (int i = 0; i < BOM_FILE_CFG::MAX_SHEET_COUNT; i++)
			{
				if (_stricmp(key_word, CXhChar16("TMA_BOM_%d", i + 1)) == 0)
					InitBomTblTitleCfg(skey, &g_xBomCfg.m_xTmaTblCfg.m_xArrTblCfg[i]);
				else if (_stricmp(key_word, CXhChar16("ERP_BOM_%d", i + 1)) == 0)
					InitBomTblTitleCfg(skey, &g_xBomCfg.m_xErpTblCfg.m_xArrTblCfg[i]);
				else if (_stricmp(key_word, CXhChar16("PRINT_BOM_%d", i + 1)) == 0)
					InitBomTblTitleCfg(skey, &g_xBomCfg.m_xPrintTblCfg.m_xArrTblCfg[i]);
			}
		}
	}
	fclose(fp);
	//中电建成都铁塔特殊要求:角钢工艺信息只通过工艺数据点进行提取 wxc-2020.12.24
	if(g_xUbomModel.m_uiCustomizeSerial== ID_SiChuan_ChengDu)
		CAngleProcessInfo::bInitGYByGYRect = TRUE;
	//初始化钢板识别规则
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		if (pSchema->m_bEnable)
		{
			g_pncSysPara.ActiveRecogSchema(pSchema);
			break;
		}
	}
	//初始化角钢工艺卡识别规则
	if (g_xUbomModel.IsValidFunc(CUbomModel::FUNC_DWG_COMPARE) ||
		g_xUbomModel.IsValidFunc(CUbomModel::FUNC_DWG_BATCH_PRINT))
	{	//加载角钢工艺卡
		char APP_PATH[MAX_PATH] = "", sJgCardPath[MAX_PATH] = "", sJgCardPath2[MAX_PATH] = "";
		GetAppPath(APP_PATH);
		sprintf(sJgCardPath, "%s%s", APP_PATH, (char*)g_xUbomModel.m_sJgCadName);
		sprintf(sJgCardPath2, "%s角钢工艺卡\\%s", APP_PATH, (char*)g_xUbomModel.m_sJgCadName);
		if (!g_pncSysPara.InitJgCardInfo(sJgCardPath) &&	//在根目录中查找工艺卡模板
			!g_pncSysPara.InitJgCardInfo(sJgCardPath2))		//在“角钢工艺卡”子目录中查找工艺卡模板 wht 20-07-18
		{
			logerr.Log(CXhChar200("角钢工艺卡读取失败(%s)!", sJgCardPath));
		}
	}
	//初始化批量打印设置
	if (CBatchPrint::m_xPngPlotCfg.m_sDeviceName.GetLength() <= 0)
		CBatchPrint::m_xPngPlotCfg.m_sDeviceName.Copy("PublishToWeb PNG.pc3");
	if (CBatchPrint::m_xPdfPlotCfg.m_sDeviceName.GetLength() <= 0)
		CBatchPrint::m_xPdfPlotCfg.m_sDeviceName.Copy("DWG To PDF.pc3");
	if (CBatchPrint::m_xPaperPlotCfg.m_sPaperSize.GetLength() <= 0)
		CBatchPrint::m_xPaperPlotCfg.m_sPaperSize.Copy("A4");
	if (file_path == NULL)
	{	//导入特定设置的钢板提取规则rule.set
		PNCSysSetImportDefault();
	}
}
void ExportUbomConfigFile()
{
	char file_name[MAX_PATH] = "";
	GetAppPath(file_name);
	strcat(file_name, "ubom.cfg");
	FILE *fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("打不开指定的配置文件!");
		return;
	}
	fprintf(fp, "**基本设置\n");
	fprintf(fp, "CLIENT_ID=%d ;\n", g_xUbomModel.m_uiCustomizeSerial);
	fprintf(fp, "CLIENT_NAME=%s ;\n", (char*)g_xUbomModel.m_sCustomizeName);
	fprintf(fp, "FUNC_FLAG=0x%X ;\n", g_xUbomModel.m_dwFunctionFlag);
	fprintf(fp, "ExeRppWhenArxLoad=%d ;\n", g_xUbomModel.m_bExeRppWhenArxLoad);
	fprintf(fp, "AutoExtractPlates=%d ;\n", g_xUbomModel.m_bExtractPltesWhenOpenFile);
	fprintf(fp, "AutoExtractAngles=%d ;\n", g_xUbomModel.m_bExtractAnglesWhenOpenFile);
	fprintf(fp, "MaxLenErr=%.1f ;\n", g_xUbomModel.m_fMaxLenErr);
	fprintf(fp, "NOT_PRINT=%s ;\n", (char*)g_xUbomModel.m_sNotPrintFilter);
	fprintf(fp, "PrintSortType=%d ;\n", g_xUbomModel.m_ciPrintSortType);
	fprintf(fp, "JgCadPartLabelMat=%d ;\n", g_xUbomModel.m_uiJgCadPartLabelMat);
	fprintf(fp, "**DWG识别设置\n");
	fprintf(fp, "JG_CARD=%s ;\n", (char*)g_xUbomModel.m_sJgCadName);
	fprintf(fp, "JgCadPartLabel=%s ;\n", (char*)g_xUbomModel.m_sJgCadPartLabel);
	fprintf(fp, "JgCardBlockName=%s ;\n", (char*)g_xUbomModel.m_sJgCardBlockName);
	fprintf(fp, "PlateRecogMode=%d ;\n", g_pncSysPara.m_ciRecogMode);
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		fprintf(fp, "PlateDimStyle=%d;", pSchema->m_iDimStyle);
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
	fprintf(fp, "**表格识别设置\n");
	//fprintf(fp, "TMA_BOM=%s ;\n", (char*)g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "TMABomFileKeyStr=%s ;\n", (char*)g_xBomCfg.m_xTmaTblCfg.m_sFileTypeKeyStr);
	fprintf(fp, "TMABomSheetName=%s ;\n", (char*)g_xBomCfg.m_xTmaTblCfg.m_sBomSheetName);
	//fprintf(fp, "ERP_BOM=%s ;\n", (char*)g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "ERPBomFileKeyStr=%s ;\n", (char*)g_xBomCfg.m_xErpTblCfg.m_sFileTypeKeyStr);
	fprintf(fp, "ERPBomSheetName=%s ;\n", (char*)g_xBomCfg.m_xErpTblCfg.m_sBomSheetName);
	//fprintf(fp, "PRINT_BOM=%s ;\n", (char*)g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "PrintBomFileKeyStr=%s ;\n", (char*)g_xBomCfg.m_xPrintTblCfg.m_sFileTypeKeyStr);
	fprintf(fp, "JGPrintSheetName=%s ;\n", (char*)g_xBomCfg.m_xPrintTblCfg.m_sBomSheetName);
	fprintf(fp, "**数据校审项设置\n");
	//fprintf(fp, "ANGLE_ITEM=%s ;\n", (char*)g_xUbomModel.m_sNotPrintFilter);
	//fprintf(fp, "PLATE_ITEM=%s ;\n", (char*)g_xUbomModel.m_sNotPrintFilter);
	fprintf(fp, "**工艺描述设置\n");
	fprintf(fp, "ZHIWAN_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_ZHI_WAN]);
	fprintf(fp, "CUT_ANGLE_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ANGLE]);
	fprintf(fp, "CUT_ROOT_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_ROOT]);
	fprintf(fp, "CUT_BER_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_CUT_BER]);
	fprintf(fp, "PUSH_FLAT_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_PUSH_FLAT]);
	fprintf(fp, "KAI_JIAO_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_KAI_JIAO]);
	fprintf(fp, "HE_JIAO_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_HE_JIAO]);
	fprintf(fp, "FOO_NAIL_ITEM=%s ;\n", (char*)g_xBomCfg.m_sProcessDescArr[CBomConfig::TYPE_FOO_NAIL]);
	fprintf(fp, "ProcessFlag=%s ;\n", (char*)g_xBomCfg.m_sProcessFlag);
	fclose(fp);
}
#endif
