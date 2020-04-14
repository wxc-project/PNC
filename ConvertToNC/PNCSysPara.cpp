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
	m_bReplaceSH = FALSE;
	m_nReplaceHD = 20;
	//自动排版设置
	m_bAutoLayout = FALSE;
	m_nMapWidth = 1500;
	m_nMapLength = 0;
	m_nMinDistance = 0;
	m_bMKPos = 0;
	m_nMkRectWidth = 30;
	m_nMkRectLen = 60;
	//图纸比例设置
	m_fMapScale = 1;
	m_iLayerMode = 0;
	m_ciRecogMode = 0;
	m_ciBoltRecogMode = BOLT_RECOG_DEFAULT;
	m_ciProfileColorIndex = 1;		//红色
	m_ciBendLineColorIndex = 190;	//紫色
	m_sProfileLineType.Copy("CONTINUOUS");
	m_fPixelScale = 0.8;
	m_fMaxLenErr = 0.5;
	//默认过滤图层名
	//默认加载文字识别设置
	g_pncSysPara.m_recogSchemaList.Empty();
	RECOG_SCHEMA *pSchema = InsertRecogSchema("单行1", 0, "#", "Q", "-");
	if (pSchema)
		pSchema->m_bEnable = TRUE;
	InsertRecogSchema("单行2", 0, "#", "Q", "-", "件");
	InsertRecogSchema("单行3", 0, "#", "Q", "-", "件", "正曲", "反曲");
	InsertRecogSchema("单行4", 0, "#", "Q", "-", "件", "外曲", "内曲");
	InsertRecogSchema("多行1", 1, "#", "Q", "-");
	InsertRecogSchema("多行2", 1, "#", "Q", "-", "件");
	InsertRecogSchema("多行3", 1, "#", "Q", "-", "件", "正曲", "反曲");
	InsertRecogSchema("多行4", 1, "#", "Q", "-", "件", "外曲", "内曲");
	InsertRecogSchema("多行5", 1, "件号:", "材质:", "板厚:");
	InsertRecogSchema("多行6", 1, "件号:", "材质:", "板厚:", "件数");
	InsertRecogSchema("多行7", 1, "件号:", "材质:", "板厚:", "件数", "正曲", "反曲");
	InsertRecogSchema("多行8", 1, "件号:", "材质:", "板厚:", "件数", "外曲", "内曲");
	
#ifdef __PNC_
	//InitDrawingEnv();
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
	//基本设置
	AddPropItem("general_set",PROPLIST_ITEM(id++,"常规设置","常规设置"));
	AddPropItem("m_sJgCadName", PROPLIST_ITEM(id++,"角钢工艺卡","当前使用的角钢工艺卡"));
	AddPropItem("m_fMaxLenErr", PROPLIST_ITEM(id++, "长度最大误差值", "数据校审时，长度比较的最大误差值"));
	AddPropItem("m_bIncDeformed",PROPLIST_ITEM(id++,"已考虑火曲变形","待提取的钢板图形已考虑火曲变形量","是|否"));
	AddPropItem("m_bReplaceSH", PROPLIST_ITEM(id++, "启用特殊孔代孔", "将特殊功用类型的孔进行代孔处理", "是|否"));
	AddPropItem("m_nReplaceHD", PROPLIST_ITEM(id++, "代孔直径", "进行代孔的螺栓直径", "12|16|20|24"));
	AddPropItem("m_iPPiMode",PROPLIST_ITEM(id++,"文件生成模型","PPI文件生成模式","0.一板一号|1.一板多号"));
	AddPropItem("m_bAutoLayout2", PROPLIST_ITEM(id++, "下料预审", "启用下料预审时，钢板提取之后按段排布，支持设定加工坐标系，支持设定钢印号位置。", "是|否"));
	AddPropItem("CDrawDamBoard::BOARD_HEIGHT", PROPLIST_ITEM(id++, "档板高度", "档板高度"));
	AddPropItem("CDrawDamBoard::m_bDrawAllBamBoard", PROPLIST_ITEM(id++, "档板显示模式", "档板显示模式", "0.仅显示选中钢板档板|1.显示所有档板"));
	AddPropItem("m_nMkRectLen", PROPLIST_ITEM(id++, "钢印字盒长度", "钢印字盒宽度"));
	AddPropItem("m_nMkRectWidth", PROPLIST_ITEM(id++, "钢印字盒宽度", "钢印字盒宽度"));
	AddPropItem("m_bAutoLayout1",PROPLIST_ITEM(id++,"打印排版","打印排版","是|否"));
	AddPropItem("m_nMapWidth",PROPLIST_ITEM(id++,"图纸宽度","图纸宽度"));
	AddPropItem("m_nMapLength",PROPLIST_ITEM(id++,"图纸长度","图纸长度"));
	AddPropItem("m_nMinDistance",PROPLIST_ITEM(id++,"最小间距","图形之间的最小间距"));
	AddPropItem("m_bMKPos",PROPLIST_ITEM(id++,"提取钢印位置","提取钢印位置","是|否"));
	AddPropItem("AxisXCalType",PROPLIST_ITEM(id++,"X轴计算方式","加工坐标系X轴的计算方式","0.最长边优先|1.螺栓平行边优先|2.焊接边优先"));
	AddPropItem("m_ciBoltRecogMode", PROPLIST_ITEM(id++, "螺栓识别模式", "螺栓识别模式", "0.默认|1.不过滤件号圆圈"));
	//图层设置
	AddPropItem("layer_set",PROPLIST_ITEM(id++,"识别模式","识别模式"));
	AddPropItem("m_iRecogMode",PROPLIST_ITEM(id++,"识别模式","钢板识别模式"));
	AddPropItem("m_iProfileLineTypeName", PROPLIST_ITEM(id++, "轮廓边线型", "钢板外轮廓边线型", "CONTINUOUS|HIDDEN|DASHDOT2X|DIVIDE|ZIGZAG"));
	AddPropItem("m_iProfileColorIndex",PROPLIST_ITEM(id++,"轮廓边颜色","钢板外轮廓边颜色"));
	AddPropItem("m_iBendLineColorIndex",PROPLIST_ITEM(id++,"火曲线颜色","钢板制弯线颜色"));
	AddPropItem("layer_mode",PROPLIST_ITEM(id++,"图层处理方式","轮廓边图层处理方式","0.指定轮廓边图层|1.过滤默认图层"));
	AddPropItem("m_fPixelScale", PROPLIST_ITEM(id++, "处理像素比例"));
	//图纸比例设置
	AddPropItem("map_scale_set",PROPLIST_ITEM(id++,"比例识别","图纸比例设置"));
	AddPropItem("m_fMapScale",PROPLIST_ITEM(id++,"缩放比例","绘图缩放比例"));
}
int CPNCSysPara::GetPropValueStr(long id,char* valueStr,UINT nMaxStrBufLen/*=100*/,CPropTreeItem *pItem/*=NULL*/)
{
	CXhChar100 sText;
	if (GetPropID("m_bIncDeformed") == id)
	{
		if (g_pncSysPara.m_bIncDeformed)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("m_sJgCadName") == id)
		sText = m_sJgCadName;
	else if(GetPropID("m_fMaxLenErr")==id)
		sText.Printf("%.f", g_pncSysPara.m_fMapScale);
	else if (GetPropID("m_bReplaceSH") == id)
	{
		if (g_pncSysPara.m_bReplaceSH)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("m_nReplaceHD") == id)
		sText.Printf("%d", g_pncSysPara.m_nReplaceHD);
	else if(GetPropID("m_iPPiMode")==id)
	{
		if(g_pncSysPara.m_iPPiMode==0)
			sText.Copy("0.一板一号");
		else if(g_pncSysPara.m_iPPiMode==1)
			sText.Copy("1.一板多号");
	}
	else if(GetPropID("AxisXCalType")==id)
	{
		if(g_pncSysPara.m_iAxisXCalType==0)
			sText.Copy("0.最长边优先");
		else if(g_pncSysPara.m_iAxisXCalType==1)
			sText.Copy("1.螺栓平行边优先");
		else if(g_pncSysPara.m_iAxisXCalType==2)
			sText.Copy("2.焊接边优先");
	}
	else if(GetPropID("m_bMKPos")==id)
	{
		if(g_pncSysPara.m_bMKPos)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("m_fMapScale")==id)
		sText.Printf("%.f",g_pncSysPara.m_fMapScale);
	else if(GetPropID("m_bAutoLayout1")==id)
	{
		if(m_bAutoLayout==CPNCSysPara::LAYOUT_PRINT)
			sText.Copy("是");
		else 
			sText.Copy("否");
	}
	else if (GetPropID("m_bAutoLayout2") == id)
	{
		if (m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("m_nMapLength")==id)
		sText.Printf("%d",g_pncSysPara.m_nMapLength);
	else if(GetPropID("m_nMapWidth")==id)
		sText.Printf("%d",g_pncSysPara.m_nMapWidth);
	else if(GetPropID("m_nMinDistance")==id)
		sText.Printf("%d",g_pncSysPara.m_nMinDistance);
#ifndef __UBOM_ONLY_ 
	else if(GetPropID("CDrawDamBoard::m_bDrawAllBamBoard")==id)
	{
		if(CDrawDamBoard::m_bDrawAllBamBoard)
			sText.Copy("1.显示所有档板");
		else 
			sText.Copy("0.仅显示选中钢板档板");
	}
	else if(GetPropID("CDrawDamBoard::BOARD_HEIGHT")==id)
		sText.Printf("%d",CDrawDamBoard::BOARD_HEIGHT);
#endif
	else if (GetPropID("m_nMkRectWidth") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectWidth);
	else if (GetPropID("m_nMkRectLen") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectLen);
	else if(GetPropID("layer_mode")==id)
	{
		if(m_iLayerMode==0)
			sText.Copy("0.指定轮廓边图层");
		else if(m_iLayerMode==1)
			sText.Copy("1.过滤默认图层");
	}
	else if(GetPropID("m_iRecogMode")==id)
	{
		if (m_ciRecogMode == FILTER_BY_LINETYPE)
			sText.Copy("0.按线型识别");
		else if (m_ciRecogMode == FILTER_BY_LAYER)
			sText.Copy("1.按图层识别");
		else if (m_ciRecogMode == FILTER_BY_COLOR)
			sText.Copy("2.按颜色识别");
		else if (m_ciRecogMode == FILTER_BY_PIXEL)
			sText.Copy("3.按像素识别");
	}
	else if (GetPropID("m_fPixelScale") == id)
	{
		sText.Printf("%f", g_pncSysPara.m_fPixelScale);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("m_ciBoltRecogMode") == id)
	{
		if (m_ciBoltRecogMode == BOLT_RECOG_NO_FILTER_PARTNO_CIR)
			sText.Copy("1.不过滤件号圆圈");
		else //if (m_ciBoltRecogMode == BOLT_RECOG_DEFAULT)
			sText.Copy("0.默认");
	}
	else if(GetPropID("m_iProfileColorIndex")==id)
		sText.Printf("RGB%X",GetColorFromIndex(m_ciProfileColorIndex));
	else if(GetPropID("m_iBendLineColorIndex")==id)
		sText.Printf("RGB%X",GetColorFromIndex(m_ciBendLineColorIndex));
	else if(GetPropID("m_iProfileLineTypeName")==id)
		sText.Copy(m_sProfileLineType);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return strlen(sText);
}

BOOL CPNCSysPara::IsNeedFilterLayer(const char* sLayer)
{
	if(m_ciRecogMode==FILTER_BY_LAYER)
	{
		if(m_iLayerMode==0&&m_xHashEdgeKeepLayers.GetNodeNum()>0)
		{	//用户指定轮廓边所在图层
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
		return TRUE;	//直线
	if (pEnt->isKindOf(AcDbArc::desc()))
		return TRUE;	//圆弧
	//if (pEnt->isKindOf(AcDbEllipse::desc()))
		//return TRUE;	//椭圆弧简化为直线处理了
	if (pEnt->isKindOf(AcDbPolyline::desc()))
		return TRUE;	//面域中包括轮廓边
	if (pEnt->isKindOf(AcDbRegion::desc()))
		return TRUE;	//多线段
	return FALSE;
}

BOOL CPNCSysPara::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=CPlateExtractor::IsBendLine(pAcDbLine,pRecognizer);
	if(!bRet)
	{	
		if(m_ciRecogMode==FILTER_BY_COLOR)
			bRet=(GetEntColorIndex(pAcDbLine)==m_ciBendLineColorIndex);
	}
	return bRet;
}

BOOL CPNCSysPara::RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum)
{
	if(m_bMKPos==FALSE)	//不需要提取钢印号
		return FALSE;
	if(pEnt->isKindOf(AcDbText::desc()))
	{	//获取钢印区
		AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
		CXhChar50 sText(_bstr_t(pText->textString()));
#else
		CXhChar50 sText(pText->textString());
#endif
		if(!sText.Equal("钢印区"))
			return FALSE;
		double len=DrawTextLength(sText,pText->height(),pText->textStyle());
		f3dPoint dim_vec(cos(pText->rotation()),sin(pText->rotation()));
		f3dPoint origin(pText->position().x,pText->position().y,pText->position().z);
		origin+=dim_vec*len*0.5;
		f2dRect rect;
		rect.topLeft.Set(origin.x-10,origin.y+10);
		rect.bottomRight.Set(origin.x+10,origin.y-10);
		ZoomAcadView(rect,200);		//对钢印区进行适当缩放
		ads_name seqent;
		AcDbObjectId initLastObjId,plineId;
		acdbEntLast(seqent);
		acdbGetObjectId(initLastObjId,seqent);
		ads_point base_pnt;
		base_pnt[X]=origin.x;
		base_pnt[Y]=origin.y;
		base_pnt[Z]=origin.z;
#ifdef _ARX_2007
		int resCode=acedCommand(RTSTR,L"-boundary",RTSTR,L"a",RTSTR,L"i",RTSTR,L"n",RTSTR,L"",RTSTR,L"",RTPOINT,base_pnt,RTSTR,L"",RTNONE);
#else
		int resCode=acedCommand(RTSTR,"-boundary",RTSTR,"a",RTSTR,"i",RTSTR,"n",RTSTR,"",RTSTR,"",RTPOINT,base_pnt,RTSTR,"",RTNONE);
#endif		
		if(resCode!=RTNORM)
			return FALSE;
		acdbEntLast(seqent);
		acdbGetObjectId(plineId,seqent);
		if(initLastObjId==plineId)
			return FALSE;
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
		AcDbPolyline *pPline=(AcDbPolyline*)pEnt;
		if(pPline==NULL||pPline->numVerts()!=nNum)
		{
			if(pPline)
			{
				pPline->erase(Adesk::kTrue);
				pPline->close();
			}
			return FALSE;
		}
		AcGePoint3d location;
		for(int iVertIndex=0;iVertIndex<nNum;iVertIndex++)
		{
			pPline->getPointAt(iVertIndex,location);
			ptArr[iVertIndex].Set(location.x,location.y,location.z);
		}
		pPline->erase(Adesk::kTrue);	//删除polyline对象
		pPline->close();
	}
	else if(pEnt->isKindOf(AcDbBlockReference::desc()))
	{	//钢印区图块
		AcDbBlockReference* pReference=(AcDbBlockReference*)pEnt;
		AcDbObjectId blockId=pReference->blockTableRecord();
		AcDbBlockTableRecord *pTempBlockTableRecord=NULL;
		acdbOpenObject(pTempBlockTableRecord,blockId,AcDb::kForRead);
		if(pTempBlockTableRecord==NULL)
			return FALSE;
		pTempBlockTableRecord->close();
		CXhChar50 sName;
#ifdef _ARX_2007
		ACHAR* sValue=new ACHAR[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy((char*)_bstr_t(sValue));
		delete[] sValue;
#else
		char *sValue=new char[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy(sValue);
		delete[] sValue;
#endif
		if(!sName.Equal("MK"))
			return FALSE;
		double rot_angle=pReference->rotation();
		f3dPoint orig(pReference->position().x,pReference->position().y,0);
		AcDbBlockTableRecordIterator *pIterator=NULL;
		pTempBlockTableRecord->newIterator( pIterator);
		for(;!pIterator->done();pIterator->step())
		{
			pIterator->getEntity(pEnt,AcDb::kForRead);
			CAcDbObjLife entObj(pEnt);
			if(pEnt->isKindOf(AcDbPolyline::desc()))
			{
				AcGePoint3d location;
				AcDbPolyline* pPolyLine=(AcDbPolyline*)pEnt;
				for(int iVertIndex=0;iVertIndex<nNum;iVertIndex++)
				{
					pPolyLine->getPointAt(iVertIndex,location);
					ptArr[iVertIndex].Set(location.x,location.y,location.z);
				}
				break;
			}
		}
		pTempBlockTableRecord->close();
		//更新钢印区实际坐标
		for(int i=0;i<nNum;i++)
		{
			if(fabs(rot_angle)>0)	//图块有旋转角度
				rotate_point_around_axis(ptArr[i],rot_angle,f3dPoint(),100*f3dPoint(0,0,1));
			ptArr[i]+=orig;
		}
	}
	else
		return FALSE;
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//

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
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		//常规设置
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
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_bMKPos);
		else if (_stricmp(key_word, "MapScale") == 0)
			sscanf(line_txt, "%s%f", key_word, &g_pncSysPara.m_fMapScale);
		else if (_stricmp(key_word, "bIncFilterLayer") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iLayerMode);
		else if (_stricmp(key_word, "RecogMode") == 0)
		{	//使用%d读取BYTE型变量，一次更新4个字节影响其它变量取值
			//可使用%hhu读取BYTE变量但测试无效，先读取到临时变量中，再赋值到相应变量中 wht 19-10-05
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciRecogMode = nTemp;
		}
		else if (_stricmp(key_word, "BoltRecogMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBoltRecogMode = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciBoltRecogMode);
		}
		else if (_stricmp(key_word, "BendLineColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBendLineColorIndex = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciBendLineColorIndex);
		}
		else if (_stricmp(key_word, "ProfileColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciProfileColorIndex = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciProfileColorIndex);
		}
		else if (_stricmp(key_word, "ProfileLineType") == 0)
			sscanf(line_txt, "%s%s", key_word, &g_pncSysPara.m_sProfileLineType);
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
			if (sText.Equal("是"))
				g_pncSysPara.m_bIncDeformed = true;
			else
				g_pncSysPara.m_bIncDeformed = false;
		}
		else if (_stricmp(key_word, "PPIMode") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iPPiMode);
		else if (_stricmp(key_word, "AutoLayout") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_bAutoLayout);
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
		else if (_stricmp(key_word, "JG_CARD") == 0)
			sscanf(line_txt, "%s%s", key_word, (char*)g_pncSysPara.m_sJgCadName);
#ifndef __UBOM_ONLY_
		else if (_stricmp(key_word, "CDrawDamBoard::BOARD_HEIGHT") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::BOARD_HEIGHT);
		else if (_stricmp(key_word, "CDrawDamBoard::m_bDrawAllBamBoard") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	}
	fclose(fp);
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
}
void PNCSysSetExportDefault()
{
	char file_name[MAX_PATH]="";
	GetAppPath(file_name);
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("打不开指定的配置文件!");
		return;
	}
	fprintf(fp, "基本设置\n");
	fprintf(fp, "JG_CARD=%s\n", (char*)g_pncSysPara.m_sJgCadName);
	fprintf(fp, "bIncDeformed=%s ;考虑火曲变形量\n", g_pncSysPara.m_bIncDeformed ? "是" : "否");
	fprintf(fp, "PPIMode=%d ;PPI文件模式\n", g_pncSysPara.m_iPPiMode);
	fprintf(fp, "AutoLayout=%d ;自动排版\n", g_pncSysPara.m_bAutoLayout);
	fprintf(fp, "MapWidth=%d ;图纸宽度\n", g_pncSysPara.m_nMapWidth);
	fprintf(fp, "MapLength=%d ;图纸长度\n", g_pncSysPara.m_nMapLength);
	fprintf(fp, "MinDistance=%d ;最小间距\n", g_pncSysPara.m_nMinDistance);
	fprintf(fp, "m_nMkRectWidth=%d ;字盒宽度\n", g_pncSysPara.m_nMkRectWidth);
	fprintf(fp, "m_nMkRectLen=%d ;字盒长度\n", g_pncSysPara.m_nMkRectLen);
#ifndef __UBOM_ONLY_
	fprintf(fp, "CDrawDamBoard::BOARD_HEIGHT=%d ;档板高度\n", CDrawDamBoard::BOARD_HEIGHT);
	fprintf(fp, "CDrawDamBoard::m_bDrawAllBamBoard=%d ;绘制所有档板\n", CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	fprintf(fp, "图层设置\n");
	fprintf(fp, "bIncFilterLayer=%d ;启用过滤默认图层\n", g_pncSysPara.m_iLayerMode);
	fprintf(fp, "RecogMode=%d ;识别模式\n", g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "BoltRecogMode=%d ;螺栓识别模式\n", g_pncSysPara.m_ciBoltRecogMode);
	fprintf(fp, "ProfileColorIndex=%d ;轮廓边颜色\n", g_pncSysPara.m_ciProfileColorIndex);
	fprintf(fp, "BendLineColorIndex=%d ;制弯线颜色\n", g_pncSysPara.m_ciBendLineColorIndex);
	fprintf(fp, "ProfileLineType=%s ;轮廓边线型\n", (char*)g_pncSysPara.m_sProfileLineType);
	fprintf(fp, "文字识别设置\n");
	fprintf(fp, "DimStyle=%d ;文字标注类型\n", g_pncSysPara.m_iDimStyle);
	fprintf(fp, "PnKey=%s ;件号标识符\n", (char*)g_pncSysPara.m_sPnKey);
	fprintf(fp, "ThickKey=%s ;厚度标识符\n", (char*)g_pncSysPara.m_sThickKey);
	fprintf(fp, "MatKey=%s ;材质标识符\n", (char*)g_pncSysPara.m_sMatKey);
	fprintf(fp, "PnNumKey=%s ;件数标识符\n", (char*)g_pncSysPara.m_sPnNumKey);
	fprintf(fp, "MKPos=%d ;提取钢印位置\n", (char*)g_pncSysPara.m_bMKPos);
	fprintf(fp, "AxisXCalType=%d ;X轴计算方式\n", (char*)g_pncSysPara.m_iAxisXCalType);
	fprintf(fp, "螺栓识别设置\n");
	for (BOLT_BLOCK *pBoltBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBoltBlock; pBoltBlock = g_pncSysPara.hashBoltDList.GetNext())
	{
		fprintf(fp, "BoltDKey=%s;图块名称;螺栓直径\n", g_pncSysPara.hashBoltDList.GetCursorKey());
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