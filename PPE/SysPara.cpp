#include "stdafx.h"
#include "SysPara.h"
#include "Expression.h"
#include "NcPart.h"
#ifndef _LEGACY_LICENSE
#include "XhLdsLm.h"
#else
#include "Lic.h"
#endif

//////////////////////////////////////////////////////////////////////////
//FILE_INFO_PARA
NC_INFO_PARA::NC_INFO_PARA()
{
	m_sThick.Copy("*"); 
	m_dwFileFlag = CNCPart::PLATE_DXF_FILE;
	m_bCutSpecialHole = TRUE;
	m_wEnlargedSpace = 0;
	m_bReserveBigSH = FALSE;
	m_bReduceSmallSH = TRUE;
	m_ciHoldSortType = 0;
	m_bSortHasBigSH = FALSE;
	m_bOutputBendLine = m_bOutputBendType = FALSE;
}
DWORD NC_INFO_PARA::AddFileFlag(DWORD dwFlag)
{
	m_dwFileFlag |= dwFlag;
	return m_dwFileFlag;
}
bool NC_INFO_PARA::IsValidFile(DWORD dwFlag) {
	if ((dwFlag&m_dwFileFlag) > 0)
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
//CSysPara
IMPLEMENT_PROP_FUNC(CSysPara);

const int HASHTABLESIZE=500;
const int STATUSHASHTABLESIZE=500;
void CSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//
	AddPropItem("Model",PROPLIST_ITEM(id++,"设计信息"));
	AddPropItem("model.m_sCompanyName",PROPLIST_ITEM(id++,"设计单位"));
	AddPropItem("model.m_sPrjCode",PROPLIST_ITEM(id++,"工程编号"));
	AddPropItem("model.m_sPrjName",PROPLIST_ITEM(id++,"工程名称"));
	AddPropItem("model.m_sTaType",PROPLIST_ITEM(id++,"塔型"));
	AddPropItem("model.m_sTaAlias",PROPLIST_ITEM(id++,"代号"));
	AddPropItem("model.m_sTaStampNo",PROPLIST_ITEM(id++,"钢印号"));
	AddPropItem("model.m_sOperator",PROPLIST_ITEM(id++,"操作员"));
	AddPropItem("model.m_sAuditor",PROPLIST_ITEM(id++,"审核人"));
	AddPropItem("model.m_sCritic",PROPLIST_ITEM(id++,"评审人"));
	//
	AddPropItem("NC",PROPLIST_ITEM(id++,"NC数据"));
	AddPropItem("nc.m_fMKHoleD",PROPLIST_ITEM(id++,"号位孔直径"));
	AddPropItem("nc.m_bDispMkRect",PROPLIST_ITEM(id++,"显示字盒","显示字盒","是|否"));
	AddPropItem("nc.m_ciMkVect", PROPLIST_ITEM(id++, "字盒朝向", "字盒朝向", "0.保持原朝向|1.保持水平"));
	AddPropItem("nc.m_fMKRectW",PROPLIST_ITEM(id++,"字盒宽度","字盒矩形宽度"));
	AddPropItem("nc.m_fMKRectL",PROPLIST_ITEM(id++,"字盒长度","字盒矩形长度"));
	AddPropItem("nc.m_fBaffleHigh",PROPLIST_ITEM(id++,"挡板高度"));
	AddPropItem("nc.m_iNcMode",PROPLIST_ITEM(id++,"钢板NC模式","生成钢板DXF文件的模式","切割|板床|激光|切割+板床|切割+板床+激光"));
	AddPropItem("nc.m_fLimitSH",PROPLIST_ITEM(id++,"特殊孔界限值","特殊孔一般通过切割进行加工"));
	AddPropItem("CutLimitSH", PROPLIST_ITEM(id++, "切割式特殊孔(大孔)", "通过切割工艺进行加工的特殊孔"));
	AddPropItem("ProLimitSH", PROPLIST_ITEM(id++, "板床式特殊孔(小孔)", "通过板床工艺进行加工的特殊孔"));
	AddPropItem("nc.m_sNcDriverPath",PROPLIST_ITEM(id++,"角钢NC驱动"));
	AddPropItem("nc.m_ciDisplayType", PROPLIST_ITEM(id++, "当前显示模式", "当前显示的加工模式", "原始|火焰|等离子|冲床|钻床|激光|复合模式"));
	//文件设置
	AddPropItem("FileSet", PROPLIST_ITEM(id++, "文件设置"));
	AddPropItem("FileFormat", PROPLIST_ITEM(id++, "输出文件格式"));
	AddPropItem("OutputPath", PROPLIST_ITEM(id++, "输出文件路径"));
	AddPropItem("PbjPara",PROPLIST_ITEM(id++,"PBJ设置"));
	AddPropItem("pbj.m_bIncVertex",PROPLIST_ITEM(id++,"输出顶点","导出PBJ时，是否包括顶点","是|否"));
	AddPropItem("pbj.m_bAutoSplitFile",PROPLIST_ITEM(id++,"自动拆分","螺栓直径种类大于3时，自动拆分为多个pbj文件","是|否"));
	AddPropItem("pbj.m_bMergeHole", PROPLIST_ITEM(id++, "合并加工", "非标孔按标准孔加工(如：19.5按17.5教工)；3种孔径时将数量少的一种合并至相邻孔径加工，后续环节再扩孔", "是|否"));
	AddPropItem("PmzPara", PROPLIST_ITEM(id++, "PMZ设置"));
	AddPropItem("pmz.m_iPmzMode", PROPLIST_ITEM(id++, "文件模式", "0.单文件,所有孔在一个文件中；1.多文件，一类孔输出一个文件", "0.单文件|1.多文件"));
	AddPropItem("pmz.m_bIncVertex", PROPLIST_ITEM(id++, "输出顶点", "导出PBJ时，是否包括顶点", "是|否"));
	AddPropItem("pmz.m_bPmzCheck", PROPLIST_ITEM(id++, "输出预审Pmz", "输出用于预审的Pmz文件，一个钢板上取3~4个孔做为定位孔,用于加工前预审", "是|否"));
	//等离子切割显示
	AddPropItem("plasmaCut",PROPLIST_ITEM(id++,"等离子切割设置"));
	AddPropItem("plasmaCut.m_sOutLineLen",PROPLIST_ITEM(id++,"引出线长","引出线长","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("plasmaCut.m_sIntoLineLen",PROPLIST_ITEM(id++,"引入线长","引入线长","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("plasmaCut.m_bInitPosFarOrg",PROPLIST_ITEM(id++,"初始点位置","初始点位置","0.靠近原点|1.远离原点"));
	AddPropItem("plasmaCut.m_bCutPosInInitPos",PROPLIST_ITEM(id++,"切入点定位","切入点位置","0.在指定轮廓点|1.始终在初始点"));
	//火焰切割显示
	AddPropItem("flameCut",PROPLIST_ITEM(id++,"火焰切割设置"));
	AddPropItem("flameCut.m_sOutLineLen",PROPLIST_ITEM(id++,"引出线长","引出线长","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("flameCut.m_sIntoLineLen",PROPLIST_ITEM(id++,"引入线长","引出线长","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("flameCut.m_bInitPosFarOrg",PROPLIST_ITEM(id++,"初始点位置","初始点","0.靠近原点|1.远离原点"));
	AddPropItem("flameCut.m_bCutPosInInitPos",PROPLIST_ITEM(id++,"切入点定位","切入点位置","0.在指定轮廓点|1.始终在初始点"));
	//输出设置
	AddPropItem("OutPutSet", PROPLIST_ITEM(id++, "输出设置"));
	AddPropItem("nc.m_iDxfMode", PROPLIST_ITEM(id++, "按厚度分类", "生成DXF是否进行分类", "是|否"));
	//切割下料
	AddPropItem("nc.bFlameCut", PROPLIST_ITEM(id++, "火焰切割", "", "启用|禁用"));
	AddPropItem("nc.FlamePara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "轮廓边增大值", "轮廓边增大值"));
	AddPropItem("nc.FlamePara.m_bCutSpecialHole", PROPLIST_ITEM(id++, "切割大号孔", "输出切割数据时输出特殊孔切割路径", "是|否"));
	AddPropItem("nc.FlamePara.m_xHoleIncrement", PROPLIST_ITEM(id++, "孔径增大值", "", ""));
	AddPropItem("nc.FlamePara.m_sThick", PROPLIST_ITEM(id++, "板厚范围", "*所有厚度,a单个厚度,b-c厚度区间"));
	AddPropItem("nc.FlamePara.m_dwFileFlag", PROPLIST_ITEM(id++, "生成文件", "", "DXF|TXT|CNC|DXF+TXT|DXF+CNC|DXF+TXT+CNC"));
	AddPropItem("nc.bPlasmaCut", PROPLIST_ITEM(id++, "等离子切割", "", "启用|禁用"));
	AddPropItem("nc.PlasmaPara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "轮廓边增大值", "轮廓边增大值"));
	AddPropItem("nc.PlasmaPara.m_bCutSpecialHole", PROPLIST_ITEM(id++, "切割大号孔", "", "是|否"));
	AddPropItem("nc.PlasmaPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "孔径增大值", "", ""));
	AddPropItem("nc.PlasmaPara.m_sThick", PROPLIST_ITEM(id++, "板厚范围", "*所有厚度,a单个厚度,b-c厚度区间"));
	AddPropItem("nc.PlasmaPara.m_dwFileFlag", PROPLIST_ITEM(id++, "生成文件", "", "DXF|NC|DXF+NC"));
	//板床加工
	AddPropItem("nc.bPunchPress", PROPLIST_ITEM(id++, "冲床加工", "", "启用|禁用"));
	AddPropItem("nc.PunchPara.m_bReserveBigSH", PROPLIST_ITEM(id++, "输出切割大号孔", "是否保留大号特殊孔", "是|否"));
	AddPropItem("nc.PunchPara.m_bReduceSmallSH", PROPLIST_ITEM(id++, "降级处理小号孔", "降级处理小号孔", "是|否"));
	AddPropItem("nc.PunchPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "孔径增大值", "", ""));
	AddPropItem("nc.PunchPara.m_ciHoldSortType", PROPLIST_ITEM(id++, "圆孔排序方案", "", "0.混排|1.分组+距离|2.分组+孔径"));
	AddPropItem("nc.PunchPara.m_bSortHasBigSH", PROPLIST_ITEM(id++, "切割孔参与排序", "圆孔排序时是否考虑切割大号特殊孔", "是|否"));
	AddPropItem("nc.PunchPara.m_sThick", PROPLIST_ITEM(id++, "板厚范围","*所有厚度,a单个厚度,b-c厚度区间"));
	AddPropItem("nc.PunchPara.m_dwFileFlag", PROPLIST_ITEM(id++, "生成文件", "", "DXF|PBJ|WKF|TTP|DXF+PBJ|DXF+WKF|DXF+TTP|DXF+PBJ+WKF"));
	AddPropItem("nc.bDrillPress", PROPLIST_ITEM(id++, "钻床加工", "", "启用|禁用"));
	AddPropItem("nc.DrillPara.m_bReserveBigSH", PROPLIST_ITEM(id++, "输出切割特殊孔", "是否保留特殊孔", "是|否"));
	AddPropItem("nc.DrillPara.m_bReduceSmallSH", PROPLIST_ITEM(id++, "降级处理小号孔", "降级处理小号孔", "是|否"));
	AddPropItem("nc.DrillPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "孔径增大值", "", ""));
	AddPropItem("nc.DrillPara.m_ciHoldSortType", PROPLIST_ITEM(id++, "圆孔排序方案", "", "0.混排|1.分组+距离|2.分组+孔径"));
	AddPropItem("nc.DrillPara.m_bSortHasBigSH", PROPLIST_ITEM(id++, "切割孔参与排序", "圆孔排序时是否考虑切割大号特殊孔", "是|否"));
	AddPropItem("nc.DrillPara.m_sThick", PROPLIST_ITEM(id++, "板厚范围", "*所有厚度,a单个厚度,b-c厚度区间"));
	AddPropItem("nc.DrillPara.m_dwFileFlag", PROPLIST_ITEM(id++, "生成文件", "", "DXF|PMZ|DXF+PMZ"));
	//激光加工
	AddPropItem("nc.bLaser", PROPLIST_ITEM(id++, "激光复合机", "", "启用|禁用"));
	AddPropItem("nc.LaserPara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "轮廓边增大值", "轮廓边增大值"));
	AddPropItem("nc.LaserPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "孔径增大值", "", ""));
	AddPropItem("nc.LaserPara.m_sThick", PROPLIST_ITEM(id++, "板厚范围", "*所有厚度,a单个厚度,b-c厚度区间"));
	AddPropItem("nc.LaserPara.Config", PROPLIST_ITEM(id++, "样板明细"));
	AddPropItem("nc.LaserPara.m_dwFileFlag", PROPLIST_ITEM(id++, "生成文件"));
	AddPropItem("nc.LaserPara.m_bOutputBendLine", PROPLIST_ITEM(id++, "输出制弯线", "", "是|否"));
	AddPropItem("nc.LaserPara.m_bOutputBendType", PROPLIST_ITEM(id++, "输出制弯类型", "", "是|否"));
	//
	AddPropItem("holeIncrement.m_fDatum",PROPLIST_ITEM(id++,"孔径增大值"));
	AddPropItem("holeIncrement.m_fM12",PROPLIST_ITEM(id++,"M12标准增量"));
	AddPropItem("holeIncrement.m_fM16",PROPLIST_ITEM(id++,"M16标准增量"));
	AddPropItem("holeIncrement.m_fM20",PROPLIST_ITEM(id++,"M20标准增量"));
	AddPropItem("holeIncrement.m_fM24",PROPLIST_ITEM(id++,"M24标准增量"));
	AddPropItem("holeIncrement.m_fCutSH", PROPLIST_ITEM(id++, "切割特殊孔增量"));
	AddPropItem("holeIncrement.m_fProSH", PROPLIST_ITEM(id++, "板床特殊孔增量"));
	//颜色配置方案
	AddPropItem("CRMODE",PROPLIST_ITEM(id++,"颜色方案"));
	AddPropItem("crMode.crLS12",PROPLIST_ITEM(id++,"螺栓M12","M12孔径颜色"));
	AddPropItem("crMode.crLS16",PROPLIST_ITEM(id++,"螺栓M16","M16孔径颜色"));
	AddPropItem("crMode.crLS20",PROPLIST_ITEM(id++,"螺栓M20","M20孔径颜色"));
	AddPropItem("crMode.crLS24",PROPLIST_ITEM(id++,"螺栓M24","M24孔径颜色"));
	AddPropItem("crMode.crOtherLS",PROPLIST_ITEM(id++,"其他螺栓","其他孔径颜色"));
	AddPropItem("crMode.crMarK",PROPLIST_ITEM(id++,"钢印号","钢印号颜色"));
	AddPropItem("crMode.crEdge", PROPLIST_ITEM(id++, "轮廓边", "轮廓边的颜色"));
	AddPropItem("crMode.crHuoQu", PROPLIST_ITEM(id++, "火曲线", "火曲线的颜色"));
	AddPropItem("crMode.crText", PROPLIST_ITEM(id++, "显示文本", "文本颜色"));
	//
	AddPropItem("JgDrawing",PROPLIST_ITEM(id++,"工艺卡"));
	AddPropItem("jgDrawing.fDimTextSize",PROPLIST_ITEM(id++,"字体高度"));
	AddPropItem("jgDrawing.iDimPrecision",PROPLIST_ITEM(id++,"精确度","","1.0mm|0.5mm|0.1mm|"));
	AddPropItem("jgDrawing.fRealToDraw",PROPLIST_ITEM(id++,"绘图比例"));	
	AddPropItem("jgDrawing.fDimArrowSize",PROPLIST_ITEM(id++,"标注箭头尺寸"));
	AddPropItem("jgDrawing.fTextXFactor",PROPLIST_ITEM(id++,"字体宽高比"));
	AddPropItem("jgDrawing.fPartNoTextSize",PROPLIST_ITEM(id++,"件号字高"));
	AddPropItem("jgDrawing.iPartNoFrameStyle",PROPLIST_ITEM(id++,"件号边框类型",
		"选择自动判断时按编号长度自动判断边框类型,首选为圆圈,编号过长时使用腰圆矩形","0.圆圈|1.腰圆矩形|2.普通矩形|3.自动判断"));
	AddPropItem("jgDrawing.fPartNoMargin",PROPLIST_ITEM(id++,"件号间隙"));
	AddPropItem("jgDrawing.fPartNoCirD",PROPLIST_ITEM(id++,"件号圈直径"));
	AddPropItem("jgDrawing.fPartGuigeTextSize",PROPLIST_ITEM(id++,"规格字高"));
	AddPropItem("jgDrawing.iMatCharPosType",PROPLIST_ITEM(id++,"材质字符位置",
		"构件编号材质字符位置","0.不需要材质字符|1.构件编号前|2.构件编号后"));
	//角钢构件图设置
	AddPropItem("jgDrawing.fLsDistThreshold",PROPLIST_ITEM(id++,"角钢长度自动调整螺栓间距阈值"));
	AddPropItem("jgDrawing.fLsDistZoomCoef",PROPLIST_ITEM(id++,"螺栓间距缩放系数"));
	AddPropItem("jgDrawing.bOneCardMultiPart",PROPLIST_ITEM(id++,"一卡多件","","是|否"));
	AddPropItem("jgDrawing.bModulateLongJg",PROPLIST_ITEM(id++,"调整角钢长度","","是|否"));	
	AddPropItem("jgDrawing.iJgZoomSchema",PROPLIST_ITEM(id++,"角钢缩放方案",
		"2.长宽同比缩放及3.长宽分别缩放均为按工艺卡绘图区域自动计算缩放比例,长宽分别缩放时尽量填充整个绘图区域。",
		"0.无缩放1:1绘制|1.使用构件图比例|2.长宽同比缩放|3.长宽分别缩放"));
	AddPropItem("jgDrawing.bMaxExtendAngleLength",PROPLIST_ITEM(id++,"最大限度延伸角钢绘制长度",
		"是否在角钢工艺卡绘图区中最大限度延伸角钢绘制长度。","是|否"));
	AddPropItem("jgDrawing.iJgGDimStyle",PROPLIST_ITEM(id++,"角钢心距标注位置","","0.始端|1.中间|2.自动判断"));
	AddPropItem("jgDrawing.nMaxBoltNumStartDimG",PROPLIST_ITEM(id++,"最大螺栓数"));	
	//仅标尺寸线选项为重庆江电提出（主要原因是方便日后质检人员盲检） wjh-2017.11.28
	AddPropItem("jgDrawing.iLsSpaceDimStyle",PROPLIST_ITEM(id++,"螺栓间距标注方式","","0.沿X轴方向|1.沿Y轴方向|2.自动判断|3.不标注间距|4.仅标尺寸线"));
	AddPropItem("jgDrawing.nMaxBoltNumAlongX",PROPLIST_ITEM(id++,"最大螺栓数"));
	AddPropItem("jgDrawing.bDimCutAngle",PROPLIST_ITEM(id++,"标注切角","","是|否"));
	AddPropItem("jgDrawing.bDimCutAngleMap",PROPLIST_ITEM(id++,"标注切角示意图","","是|否"));
	AddPropItem("jgDrawing.bDimPushFlatMap",PROPLIST_ITEM(id++,"标注压扁示意图","","是|否"));
	AddPropItem("jgDrawing.bDimKaiHe",PROPLIST_ITEM(id++,"标注角钢开合角","","是|否"));
	AddPropItem("jgDrawing.bDimKaiheAngleMap",PROPLIST_ITEM(id++,"标注角钢开合角示意图","","是|否"));
	AddPropItem("jgDrawing.bDimKaiheSumLen",PROPLIST_ITEM(id++,"标注角钢开合区域总长","","是|否"));
	AddPropItem("jgDrawing.bDimKaiheAngle",PROPLIST_ITEM(id++,"标注角钢开合角度数","","是|否"));	
	AddPropItem("jgDrawing.bDimKaiheSegLen",PROPLIST_ITEM(id++,"标注角钢开合区域分段长","","是|否"));
	AddPropItem("jgDrawing.bDimKaiheScopeMap",PROPLIST_ITEM(id++,"标注开合区域标识符","","是|否"));
	AddPropItem("jgDrawing.bJgUseSimpleLsMap",PROPLIST_ITEM(id++,"角钢使用简化螺栓图符","","是|否"));
	AddPropItem("jgDrawing.bDimLsAbsoluteDist",PROPLIST_ITEM(id++,"标注螺栓绝对尺寸","","是|否"));
	AddPropItem("jgDrawing.bMergeLsAbsoluteDist",PROPLIST_ITEM(id++,"合并相邻等距螺栓绝对尺寸","","是|否"));
	AddPropItem("jgDrawing.bDimRibPlatePartNo",PROPLIST_ITEM(id++,"标注角钢肋板编号","","是|否"));
	AddPropItem("jgDrawing.bDimRibPlateSetUpPos",PROPLIST_ITEM(id++,"标注角钢肋板安装位置","","是|否"));	
	AddPropItem("jgDrawing.iCutAngleDimType",PROPLIST_ITEM(id++,"角钢切角标注样式",
		"B:端头尺寸 L:肢边尺寸 C:筋边尺寸\r\n样式一:切角=>CT LXB,切肢=>BC LXB,切整肢=>CF L;\r\n样式二:切角=>BXL 切肢=>CXL 切筋=>BXC(切大角=切筋+切肢)",
		"0.样式一|1.样式二"));
	AddPropItem("jgDrawing.fKaiHeJiaoThreshold",PROPLIST_ITEM(id++,"开合角标注阈值(°)",
		"角钢两肢夹角与90°的偏差值大于该阈值时认为需要进行开合角标注。"));
	AddPropItem("jgDrawing.sAngleCardPath",PROPLIST_ITEM(id++,"角钢工艺卡"));
	AddPropItem("Font",PROPLIST_ITEM(id++,"字体设置"));
	AddPropItem("font.fTextHeight",PROPLIST_ITEM(id++,"显示文字字高"));
	AddPropItem("font.fDxfTextSize", PROPLIST_ITEM(id++, "DXF文字字高"));
	AddPropItem("font.fDimTextSize",PROPLIST_ITEM(id++,"尺寸标注字高"));
	AddPropItem("font.fPartNoTextSize",PROPLIST_ITEM(id++,"构件编号字高"));
}
CSysPara::CSysPara(void)
{
	dock.pagePartList.bDisplay=TRUE;
	dock.pagePartList.uDockBarID=AFX_IDW_DOCKBAR_RIGHT;
	dock.pageProp.bDisplay=TRUE;
	dock.pageProp.uDockBarID=AFX_IDW_DOCKBAR_LEFT;
	dock.m_nLeftPageWidth=270;
	dock.m_nRightPageWidth=250;

	nc.m_bDispMkRect=FALSE;
	nc.m_fMKHoleD = 10;
	nc.m_fMKRectW = 30;
	nc.m_fMKRectL = 60;
	nc.m_fBaffleHigh = 260;
	nc.m_iNcMode = CNCPart::FLAME_MODE;
	nc.m_fLimitSH=25;
	nc.m_iDxfMode=0;
	nc.m_sNcDriverPath.Empty();
	nc.m_ciDisplayType=CNCPart::PUNCH_MODE;
	//
	pbj.m_bAutoSplitFile=FALSE;
	pbj.m_bIncVertex=TRUE;
	pbj.m_bMergeHole = FALSE;
	//
	pmz.m_iPmzMode = 1;
	pmz.m_bIncVertex = FALSE;
	pmz.m_bPmzCheck = TRUE;
	//等离子切割
	plasmaCut.m_sOutLineLen.Copy("4");
	plasmaCut.m_sIntoLineLen.Copy("4");
	plasmaCut.m_bCutPosInInitPos=TRUE;
	plasmaCut.m_bInitPosFarOrg=TRUE;
	//火焰切割
	flameCut.m_sOutLineLen.Copy("4");
	flameCut.m_sIntoLineLen.Copy("4");
	flameCut.m_bCutPosInInitPos=TRUE;
	flameCut.m_bInitPosFarOrg=TRUE;
	//
	holeIncrement.m_fDatum=1.5;
	holeIncrement.m_fM12=1.5;
	holeIncrement.m_fM16=1.5;
	holeIncrement.m_fM20=1.5;
	holeIncrement.m_fM24=1.5;
	holeIncrement.m_fCutSH=0;
	holeIncrement.m_fProSH = 0;
	nc.m_xFlamePara.m_xHoleIncrement = holeIncrement;
	nc.m_xPlasmaPara.m_xHoleIncrement = holeIncrement;
	nc.m_xPunchPara.m_xHoleIncrement = holeIncrement;
	nc.m_xDrillPara.m_xHoleIncrement = holeIncrement;
	nc.m_xLaserPara.m_xHoleIncrement = holeIncrement;
	//颜色方案
	crMode.crLS12 = RGB(0, 255, 255);
	crMode.crLS16 = RGB(0, 255, 0);
	crMode.crLS20 = RGB(255, 0, 255);
	crMode.crLS24 = RGB(255, 165, 79);
	crMode.crOtherLS = RGB(128, 0, 64);
	crMode.crEdge = RGB(0, 0, 0);
	crMode.crHuoQu = RGB(128, 0, 255);
	crMode.crText = RGB(255, 0, 0);
	crMode.crMark = RGB(0, 0, 255);
	//角钢构件图		
	jgDrawing.iDimPrecision =0;			
	jgDrawing.fRealToDraw=10;			
	jgDrawing.fDimArrowSize=1.5;		
	jgDrawing.fTextXFactor=0.7; 
	jgDrawing.iPartNoFrameStyle=3;	
	jgDrawing.fPartNoMargin=0.3;
	jgDrawing.fPartNoCirD=8.0;
	jgDrawing.fPartGuigeTextSize=3.0;
	jgDrawing.iMatCharPosType=0;

	jgDrawing.iJgZoomSchema=3;				//0.1:1绘制 1.使用构件图比例 2.长宽同比缩放 3.长宽分别缩放
	jgDrawing.bModulateLongJg=TRUE;			//调整角钢长度
	jgDrawing.bOneCardMultiPart=FALSE;		//角钢允许一卡多件情况
	jgDrawing.iJgGDimStyle=2;				//0.始端标注  1.中间标注 2.自动判断
	jgDrawing.bMaxExtendAngleLength=TRUE;
	jgDrawing.nMaxBoltNumStartDimG=100;		//集中在始端标注准距支持的最大螺栓数
	jgDrawing.iLsSpaceDimStyle=2;			//0.X轴方向	  1.Y轴方向  2.自动判断 3.不标注
	jgDrawing.nMaxBoltNumAlongX=50;			//沿X轴方向标注支持的最大螺栓个数
	jgDrawing.bDimCutAngle=TRUE;			//标注角钢切角
	jgDrawing.bDimCutAngleMap=TRUE;			//标注角钢切角示意图
	jgDrawing.bDimPushFlatMap=TRUE;			//标注压扁示意图
	jgDrawing.bJgUseSimpleLsMap=FALSE;		//角钢使用简化螺栓图符
	jgDrawing.bDimLsAbsoluteDist=TRUE;		//标注螺栓绝对尺寸
	jgDrawing.bMergeLsAbsoluteDist=TRUE;	//合并相邻等距螺栓绝对尺寸
	jgDrawing.bDimRibPlatePartNo=TRUE;		//标注角钢肋板编号
	jgDrawing.bDimRibPlateSetUpPos=TRUE;	//标注角钢肋板安装位置
	jgDrawing.iCutAngleDimType=0;			//角钢切角标注样式 
	//开合角标注
	jgDrawing.bDimKaiHe=TRUE;				//标注角钢开合角
	jgDrawing.bDimKaiheAngleMap=TRUE;		//标注角钢开合角示意图
	jgDrawing.fKaiHeJiaoThreshold=2;		//默认开合角标注阈值为2°
	jgDrawing.bDimKaiheSumLen=TRUE;
	jgDrawing.bDimKaiheAngle=TRUE;
	jgDrawing.bDimKaiheSegLen=TRUE;
	jgDrawing.bDimKaiheScopeMap=FALSE;
	//
	jgDrawing.sAngleCardPath.Empty();
	//
	font.fTextHeight = 30;
	font.fDxfTextSize = 2;
	font.fDimTextSize=2.5;
	font.fPartNoTextSize=3.0;
}

CSysPara::~CSysPara(void)
{
}

BOOL CSysPara::Write(CString file_path)	//写配置文件
{
	CString version("2.8");
	if(file_path.IsEmpty())
		return FALSE;
	CFile file;
	if(!file.Open(file_path, CFile::modeCreate|CFile::modeWrite))
		return FALSE;
	BYTE byte=0;
	CArchive ar(&file,CArchive::store);
	ar<<version;
	ar<<dock.m_nLeftPageWidth;
	ar<<dock.m_nRightPageWidth;
	ar<<(WORD)dock.pagePartList.bDisplay;
	ar<<dock.pagePartList.uDockBarID;
	ar<<(WORD)dock.pageProp.bDisplay;
	ar<<dock.pageProp.uDockBarID;

	
	ar<<font.fTextHeight;
	ar<<nc.m_fBaffleHigh;
	ar<<byte;//nc.m_iMKMode;
	ar<<nc.m_fMKRectW;
	ar<<nc.m_fMKRectL;
	ar<<nc.m_fMKHoleD;
	ar<<nc.m_sNcDriverPath;
	ar<<nc.m_iNcMode;
	//
	ar<<holeIncrement.m_fDatum;
	ar<<holeIncrement.m_fM12;
	ar<<holeIncrement.m_fM16;
	ar<<holeIncrement.m_fM20;
	ar<<holeIncrement.m_fM24;
	ar<<holeIncrement.m_fCutSH;
	ar<<holeIncrement.m_fProSH;
	//等离子切割
	ar<<nc.m_ciDisplayType;
	ar<<CString(plasmaCut.m_sOutLineLen);
	ar<<CString(plasmaCut.m_sIntoLineLen);
	ar<<(WORD)plasmaCut.m_bInitPosFarOrg;
	ar<<(WORD)plasmaCut.m_bCutPosInInitPos;
	//火焰切割
	ar<<CString(flameCut.m_sOutLineLen);
	ar<<CString(flameCut.m_sIntoLineLen);
	ar<<(WORD)flameCut.m_bInitPosFarOrg;
	ar<<(WORD)flameCut.m_bCutPosInInitPos;
	ar<<font.fDimTextSize;
	ar<<jgDrawing.iDimPrecision;
	ar<<jgDrawing.fRealToDraw;
	ar<<jgDrawing.fDimArrowSize;
	ar<<jgDrawing.fTextXFactor;
	ar<<font.fPartNoTextSize;
	ar<<jgDrawing.iPartNoFrameStyle;
	ar<<jgDrawing.fPartNoMargin;
	ar<<jgDrawing.fPartNoCirD;
	ar<<jgDrawing.fPartGuigeTextSize;
	ar<<jgDrawing.iMatCharPosType;

	ar<<jgDrawing.iJgZoomSchema;
	ar<<(WORD)jgDrawing.bModulateLongJg;
	ar<<(WORD)jgDrawing.bOneCardMultiPart;
	ar<<jgDrawing.iJgGDimStyle;
	ar<<(WORD)jgDrawing.bMaxExtendAngleLength;
	ar<<jgDrawing.nMaxBoltNumStartDimG;
	ar<<jgDrawing.iLsSpaceDimStyle;
	ar<<jgDrawing.nMaxBoltNumAlongX;
	ar<<(WORD)jgDrawing.bDimCutAngle;
	ar<<(WORD)jgDrawing.bDimCutAngleMap;
	ar<<(WORD)jgDrawing.bDimPushFlatMap;
	ar<<(WORD)jgDrawing.bJgUseSimpleLsMap;
	ar<<(WORD)jgDrawing.bDimLsAbsoluteDist;
	ar<<(WORD)jgDrawing.bMergeLsAbsoluteDist;
	ar<<(WORD)jgDrawing.bDimRibPlatePartNo;
	ar<<(WORD)jgDrawing.bDimRibPlateSetUpPos;
	ar<<jgDrawing.iCutAngleDimType;
	//开合角标注
	ar<<(WORD)jgDrawing.bDimKaiHe;
	ar<<(WORD)jgDrawing.bDimKaiheAngleMap;
	ar<<jgDrawing.fKaiHeJiaoThreshold;
	ar<<(WORD)jgDrawing.bDimKaiheSumLen;
	ar<<(WORD)jgDrawing.bDimKaiheAngle;
	ar<<(WORD)jgDrawing.bDimKaiheSegLen;
	ar<<(WORD)jgDrawing.bDimKaiheScopeMap;
	//角钢工艺卡
	ar<<jgDrawing.sAngleCardPath;
	//PBJ参数
	ar<<(WORD)pbj.m_bIncVertex;
	ar<<(WORD)pbj.m_bAutoSplitFile;
	ar << (WORD)pbj.m_bMergeHole;
	//PMZ参数 wht 19-07-02
	ar << (WORD)pmz.m_iPmzMode;
	ar << (WORD)pmz.m_bIncVertex;
	ar << (WORD)pmz.m_bPmzCheck;
	//钢板NC输出设置
	ar << CString(nc.m_xFlamePara.m_sThick);
	ar << (DWORD)nc.m_xFlamePara.m_dwFileFlag;
	ar << CString(nc.m_xPlasmaPara.m_sThick);
	ar << (DWORD)nc.m_xPlasmaPara.m_dwFileFlag;
	ar << CString(nc.m_xPunchPara.m_sThick);
	ar << (DWORD)nc.m_xPunchPara.m_dwFileFlag;
	ar << CString(nc.m_xDrillPara.m_sThick);
	ar << (DWORD)nc.m_xDrillPara.m_dwFileFlag;
	ar << CString(nc.m_xLaserPara.m_sThick);
	for (int i = 0; i < 5; i++)
	{
		NC_INFO_PARA* pNcPare = NULL;
		if (i == 0)
			pNcPare = &nc.m_xFlamePara;
		else if (i == 1)
			pNcPare = &nc.m_xPlasmaPara;
		else if (i == 2)
			pNcPare = &nc.m_xPunchPara;
		else if (i == 3)
			pNcPare = &nc.m_xDrillPara;
		else //if (i == 4)
			pNcPare = &nc.m_xLaserPara;
		ar << pNcPare->m_xHoleIncrement.m_fDatum;
		ar << pNcPare->m_xHoleIncrement.m_fM12;
		ar << pNcPare->m_xHoleIncrement.m_fM16;
		ar << pNcPare->m_xHoleIncrement.m_fM20;
		ar << pNcPare->m_xHoleIncrement.m_fM24;
		ar << pNcPare->m_xHoleIncrement.m_fCutSH;
		ar << pNcPare->m_xHoleIncrement.m_fProSH;
	}
	//工程信息
	ar << CString(model.m_sCompanyName);
	ar << CString(model.m_sPrjCode);
	ar << CString(model.m_sPrjName);
	ar << CString(model.m_sTaType);
	ar << CString(model.m_sTaAlias);
	ar << CString(model.m_sTaStampNo);
	ar << CString(model.m_sOperator);
	ar << CString(model.m_sAuditor);
	ar << CString(model.m_sCritic);
	ar << CString(model.m_sOutputPath);
	ar << model.file_format.m_sSplitters.size();
	for (size_t i = 0; i < model.file_format.m_sSplitters.size(); i++)
		ar << CString(model.file_format.m_sSplitters[i]);
	ar << model.file_format.m_sKeyMarkArr.size();
	for (size_t i = 0; i < model.file_format.m_sKeyMarkArr.size(); i++)
		ar << CString(model.file_format.m_sKeyMarkArr[i]);
	//更新注册表内容 wxc 16-11-21
	WriteSysParaToReg("MKRectL");
	WriteSysParaToReg("MKRectW");
	WriteSysParaToReg("MKHoleD");
	WriteSysParaToReg("SideBaffleHigh");
	WriteSysParaToReg("M12Color");
	WriteSysParaToReg("M16Color");
	WriteSysParaToReg("M20Color");
	WriteSysParaToReg("M24Color");
	WriteSysParaToReg("OtherColor");
	WriteSysParaToReg("MarkColor");
	WriteSysParaToReg("EdgeColor");
	WriteSysParaToReg("HuoQuColor");
	WriteSysParaToReg("TextColor");
	WriteSysParaToReg("TextHeight");
	WriteSysParaToReg("DxfTextSize");
	WriteSysParaToReg("LimitSH");
	WriteSysParaToReg("DxfMode");	
	WriteSysParaToReg("DispMkRect");
	WriteSysParaToReg("MKVectType");
	WriteSysParaToReg("m_ciDisplayType");
	WriteSysParaToReg("DrillNeedSH");
	WriteSysParaToReg("DrillReduceSmallSH");
	WriteSysParaToReg("DrillHoldSortType");
	WriteSysParaToReg("DrillSortHasBigSH");
	WriteSysParaToReg("PunchNeedSH");
	WriteSysParaToReg("PunchReduceSmallSH");
	WriteSysParaToReg("PunchHoldSortType");
	WriteSysParaToReg("PunchSortHasBigSH");
	WriteSysParaToReg("flameCut.m_bCutPosInInitPos");
	WriteSysParaToReg("flameCut.m_bInitPosFarOrg");
	WriteSysParaToReg("flameCut.m_sIntoLineLen");
	WriteSysParaToReg("flameCut.m_sOutLineLen");
	WriteSysParaToReg("flameCut.m_wEnlargedSpace");
	WriteSysParaToReg("flameCut.m_bCutSpecialHole");
	WriteSysParaToReg("plasmaCut.m_bCutPosInInitPos");
	WriteSysParaToReg("plasmaCut.m_bInitPosFarOrg");
	WriteSysParaToReg("plasmaCut.m_sIntoLineLen");
	WriteSysParaToReg("plasmaCut.m_sOutLineLen");
	WriteSysParaToReg("plasmaCut.m_wEnlargedSpace");
	WriteSysParaToReg("plasmaCut.m_bCutSpecialHole");
	WriteSysParaToReg("laserPara.m_bOutputBendLine");
	WriteSysParaToReg("laserPara.m_bOutputBendType");
	WriteSysParaToReg("laserPara.m_wEnlargedSpace");
	WriteSysParaToReg("PbjIncVertex");
	WriteSysParaToReg("PbjAutoSplitFile");
	WriteSysParaToReg("PbjMergeHole");
	WriteSysParaToReg("PmzMode");
	WriteSysParaToReg("PmzIncVertex");
	WriteSysParaToReg("PmzCheck");
	return TRUE;
}
//From FileVersion.cpp
BOOL CSysPara::Read(CString file_path)	//读配置文件
{
	CString version;
	WORD w;BYTE byte;
	if(file_path.IsEmpty())
		return FALSE;
	CFile file;
	if(!file.Open(file_path, CFile::modeRead))
		return FALSE;
	int nValue = 0;
	double fValue;
	double fVersion;
	CArchive ar(&file,CArchive::load);

	ar>>version;
	fVersion=atof(version);
	ar>>dock.m_nLeftPageWidth;
	ar>>dock.m_nRightPageWidth;
	ar>>w;	dock.pagePartList.bDisplay=w;
	ar>>dock.pagePartList.uDockBarID;
	ar>>w;	dock.pageProp.bDisplay=w;
	ar>>dock.pageProp.uDockBarID;

	if (fVersion < 2.8)
		ar >> fValue;
	if(fVersion>=1.5)
		ar>>font.fTextHeight;
	if (fVersion >= 1.4 && fVersion <2.8)
		ar >> nValue;
		
	ar>>nc.m_fBaffleHigh;
	if(fVersion>=1.3)
	{
		ar>>byte;//nc.m_iMKMode;
		ar>>nc.m_fMKRectW;
		ar>>nc.m_fMKRectL;
	}
	ar>>nc.m_fMKHoleD;
	ar>>nc.m_sNcDriverPath;
	if(fVersion>=1.2)
		ar>>nc.m_iNcMode;
	if(fVersion>=1.6)
	{
		ar>>holeIncrement.m_fDatum;
		ar>>holeIncrement.m_fM12;
		ar>>holeIncrement.m_fM16;
		ar>>holeIncrement.m_fM20;
		ar>>holeIncrement.m_fM24;
		ar>>holeIncrement.m_fCutSH;
		if (fVersion >= 2.7)
			ar >> holeIncrement.m_fProSH;
	}
	if(fVersion>=1.8)
	{	
		ar>>nc.m_ciDisplayType;
		//等离子切割
		CString sValue;
		ar>>sValue;plasmaCut.m_sOutLineLen.Copy(sValue);
		ar>>sValue;plasmaCut.m_sIntoLineLen.Copy(sValue);
		ar>>w;plasmaCut.m_bInitPosFarOrg=w;
		ar>>w;plasmaCut.m_bCutPosInInitPos=w;
		if (fVersion < 2.8)
			ar >> w;
		if (fVersion >= 2.4 && fVersion<2.8)
			ar >> w;
		//火焰切割
		ar>>sValue;flameCut.m_sOutLineLen.Copy(sValue);
		ar>>sValue;flameCut.m_sIntoLineLen.Copy(sValue);
		ar>>w;flameCut.m_bInitPosFarOrg=w;
		ar>>w; flameCut.m_bCutPosInInitPos=w;
		if (fVersion < 2.8)
			ar >> w;
		if (fVersion >= 2.4 && fVersion < 2.8)
			ar >> w;
	}
	else if(fVersion>=1.7)
	{	//等离子切割
		ar>>fValue;plasmaCut.m_sOutLineLen.Printf("%f",fValue);
		ar>>fValue;plasmaCut.m_sIntoLineLen.Printf("%f",fValue);
		ar>>w;plasmaCut.m_bInitPosFarOrg=w;
		ar>>w;plasmaCut.m_bCutPosInInitPos=w;
		//火焰切割
		ar>>fValue;flameCut.m_sOutLineLen.Printf("%f",fValue);
		ar>>fValue;flameCut.m_sIntoLineLen.Printf("%f",fValue);
		ar>>w;flameCut.m_bInitPosFarOrg=w;
		ar>>w; flameCut.m_bCutPosInInitPos=w;
	}
	ar>>font.fDimTextSize;
	ar>>jgDrawing.iDimPrecision;
	ar>>jgDrawing.fRealToDraw;
	ar>>jgDrawing.fDimArrowSize;
	ar>>jgDrawing.fTextXFactor;
	ar>>font.fPartNoTextSize;
	ar>>jgDrawing.iPartNoFrameStyle;
	ar>>jgDrawing.fPartNoMargin;
	ar>>jgDrawing.fPartNoCirD;
	ar>>jgDrawing.fPartGuigeTextSize;
	ar>>jgDrawing.iMatCharPosType;

	ar>>jgDrawing.iJgZoomSchema;
	ar>>w;	jgDrawing.bModulateLongJg=w;
	ar>>w;	jgDrawing.bOneCardMultiPart=w;
	ar>>jgDrawing.iJgGDimStyle;
	ar>>w;	jgDrawing.bMaxExtendAngleLength=w;
	ar>>jgDrawing.nMaxBoltNumStartDimG;
	ar>>jgDrawing.iLsSpaceDimStyle;
	ar>>jgDrawing.nMaxBoltNumAlongX;
	ar>>w;	jgDrawing.bDimCutAngle=w;
	ar>>w;	jgDrawing.bDimCutAngleMap=w;
	ar>>w;	jgDrawing.bDimPushFlatMap=w;
	ar>>w;	jgDrawing.bJgUseSimpleLsMap=w;
	ar>>w;	jgDrawing.bDimLsAbsoluteDist=w;
	ar>>w;	jgDrawing.bMergeLsAbsoluteDist=w;
	ar>>w;	jgDrawing.bDimRibPlatePartNo=w;
	ar>>w;	jgDrawing.bDimRibPlateSetUpPos=w;
	ar>>jgDrawing.iCutAngleDimType;
	//开合角标注
	ar>>w;	jgDrawing.bDimKaiHe=w;
	ar>>w;	jgDrawing.bDimKaiheAngleMap=w;
	ar>>jgDrawing.fKaiHeJiaoThreshold;
	ar>>w;	jgDrawing.bDimKaiheSumLen=w;
	ar>>w;	jgDrawing.bDimKaiheAngle=w;
	ar>>w;	jgDrawing.bDimKaiheSegLen=w;
	ar>>w;	jgDrawing.bDimKaiheScopeMap=w;
	//角钢工艺卡路径
	if(compareVersion(version,"1.1")>=0)
		ar>>jgDrawing.sAngleCardPath;
	//PBJ参数
	if(compareVersion(version,"1.9")>=0)
	{
		if (fVersion < 2.8)
			ar >> w;
		ar>>w;	pbj.m_bIncVertex=w;
		ar>>w;	pbj.m_bAutoSplitFile=w;
	}
	if (compareVersion(version, "2.3") >= 0)
	{
		ar >> w;	pbj.m_bMergeHole = w;
	}
	//PMZ参数
	if (compareVersion(version, "2.1") >= 0)
	{
		ar >> w;	pmz.m_iPmzMode = w;
		ar >> w;	pmz.m_bIncVertex = w;
		ar >> w;	pmz.m_bPmzCheck = w;
	}
	//钢板NC输出设置
	if (compareVersion(version, "2.0") >= 0)
	{
		DWORD dw;
		CString sValue;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xFlamePara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xFlamePara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xPlasmaPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xPlasmaPara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xPunchPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xPunchPara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xDrillPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xDrillPara.m_dwFileFlag = dw;
		ar >> sValue; nc.m_xLaserPara.m_sThick.Copy(sValue);
	}
	if (compareVersion(version, "2.8") >= 0)
	{
		for (int i = 0; i < 5; i++)
		{
			NC_INFO_PARA* pNcPare = NULL;
			if (i == 0)
				pNcPare = &nc.m_xFlamePara;
			else if (i == 1)
				pNcPare = &nc.m_xPlasmaPara;
			else if (i == 2)
				pNcPare = &nc.m_xPunchPara;
			else if (i == 3)
				pNcPare = &nc.m_xDrillPara;
			else //if (i == 4)
				pNcPare = &nc.m_xLaserPara;
			ar >> pNcPare->m_xHoleIncrement.m_fDatum;
			ar >> pNcPare->m_xHoleIncrement.m_fM12;
			ar >> pNcPare->m_xHoleIncrement.m_fM16;
			ar >> pNcPare->m_xHoleIncrement.m_fM20;
			ar >> pNcPare->m_xHoleIncrement.m_fM24;
			ar >> pNcPare->m_xHoleIncrement.m_fCutSH;
			ar >> pNcPare->m_xHoleIncrement.m_fProSH;
		}
	}
	//
	if (compareVersion(version, "2.2") >= 0)
	{
		CString sValue;
		ar >> sValue; model.m_sCompanyName.Copy(sValue);
		ar >> sValue; model.m_sPrjCode.Copy(sValue);
		ar >> sValue; model.m_sPrjName.Copy(sValue);
		ar >> sValue; model.m_sTaType.Copy(sValue);
		ar >> sValue; model.m_sTaAlias.Copy(sValue);
		ar >> sValue; model.m_sTaStampNo.Copy(sValue);
		ar >> sValue; model.m_sOperator.Copy(sValue);
		ar >> sValue; model.m_sAuditor.Copy(sValue);
		ar >> sValue; model.m_sCritic.Copy(sValue);
	}
	if (compareVersion(version, "2.6") >= 0)
	{
		CString sValue;
		ar >> sValue;
		model.m_sOutputPath.Copy(sValue);
	}
	if (compareVersion(version, "2.5") >= 0)
	{
		int nSize = 0;
		ar >> nSize;
		CString sValue;
		for (int i = 0; i < nSize; i++)
		{
			ar >> sValue;
			model.file_format.m_sSplitters.push_back(CXhChar16(sValue));
		}
		ar >> nSize;
		for (int i = 0; i < nSize; i++)
		{
			ar >> sValue;
			model.file_format.m_sKeyMarkArr.push_back(CXhChar16(sValue));
		}
	}
	//获取注册表内容
	ReadSysParaFromReg("MKRectL");
	ReadSysParaFromReg("MKRectW");
	ReadSysParaFromReg("MKHoleD");
	ReadSysParaFromReg("SideBaffleHigh");
	ReadSysParaFromReg("M12Color");
	ReadSysParaFromReg("M16Color");
	ReadSysParaFromReg("M20Color");
	ReadSysParaFromReg("M24Color");
	ReadSysParaFromReg("OtherColor");
	ReadSysParaFromReg("MarkColor");
	ReadSysParaFromReg("EdgeColor");
	ReadSysParaFromReg("HuoQuColor");
	ReadSysParaFromReg("TextColor");
	ReadSysParaFromReg("TextHeight");
	ReadSysParaFromReg("DxfTextSize");
	ReadSysParaFromReg("LimitSH");
	ReadSysParaFromReg("DxfMode");
	ReadSysParaFromReg("DispMkRect");
	ReadSysParaFromReg("MKVectType");
	ReadSysParaFromReg("m_ciDisplayType");
	ReadSysParaFromReg("DrillNeedSH");
	ReadSysParaFromReg("DrillReduceSmallSH");
	ReadSysParaFromReg("DrillHoldSortType");
	ReadSysParaFromReg("DrillSortHasBigSH");
	ReadSysParaFromReg("PunchNeedSH");
	ReadSysParaFromReg("PunchReduceSmallSH");
	ReadSysParaFromReg("PunchHoldSortType");
	ReadSysParaFromReg("PunchSortHasBigSH");
	ReadSysParaFromReg("flameCut.m_bCutPosInInitPos");
	ReadSysParaFromReg("flameCut.flameCut.m_bInitPosFarOrg");
	ReadSysParaFromReg("flameCut.m_sIntoLineLen");
	ReadSysParaFromReg("flameCut.m_sOutLineLen");
	ReadSysParaFromReg("flameCut.m_wEnlargedSpace");
	ReadSysParaFromReg("flameCut.m_bCutSpecialHole");
	ReadSysParaFromReg("plasmaCut.m_bCutPosInInitPos");
	ReadSysParaFromReg("plasmaCut.m_bInitPosFarOrg");
	ReadSysParaFromReg("plasmaCut.m_sIntoLineLen");
	ReadSysParaFromReg("plasmaCut.m_sOutLineLen");
	ReadSysParaFromReg("plasmaCut.m_wEnlargedSpace");
	ReadSysParaFromReg("plasmaCut.m_bCutSpecialHole");
	ReadSysParaFromReg("laserPara.m_bOutputBendLine");
	ReadSysParaFromReg("laserPara.m_bOutputBendType");
	ReadSysParaFromReg("laserPara.m_wEnlargedSpace");
	ReadSysParaFromReg("PbjIncVertex");
	ReadSysParaFromReg("PbjAutoSplitFile");
	ReadSysParaFromReg("PbjMergeHole");
	ReadSysParaFromReg("PmzMode");
	ReadSysParaFromReg("PmzIncVertex");
	ReadSysParaFromReg("PmzCheck");
	return TRUE;
}
//保存共用参数至注册表
void CSysPara::WriteSysParaToReg(LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH]="";
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	DWORD dwLength=0;
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_WRITE,&hKey)==ERROR_SUCCESS&&hKey)
	{
		if (stricmp(lpszEntry, "MKRectL") == 0)
			sprintf(sValue, "%f", nc.m_fMKRectL);
		else if (stricmp(lpszEntry, "MKRectW") == 0)
			sprintf(sValue, "%f", nc.m_fMKRectW);
		else if (stricmp(lpszEntry, "MKHoleD") == 0)
			sprintf(sValue, "%f", nc.m_fMKHoleD);
		else if (stricmp(lpszEntry, "SideBaffleHigh") == 0)
			sprintf(sValue, "%f", nc.m_fBaffleHigh);
		else if (stricmp(lpszEntry, "M12Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS12);
		else if (stricmp(lpszEntry, "M16Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS16);
		else if (stricmp(lpszEntry, "M20Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS20);
		else if (stricmp(lpszEntry, "M24Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS24);
		else if (stricmp(lpszEntry, "OtherColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crOtherLS);
		else if (stricmp(lpszEntry, "MarkColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crMark);
		else if (stricmp(lpszEntry, "EdgeColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crEdge);
		else if (stricmp(lpszEntry, "HuoQuColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crHuoQu);
		else if (stricmp(lpszEntry, "TextColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crText);
		else if (stricmp(lpszEntry, "TextHeight") == 0)
			sprintf(sValue, "%f", font.fTextHeight);
		else if (stricmp(lpszEntry, "DxfTextSize") == 0)
			sprintf(sValue, "%f", font.fDxfTextSize);
		else if (stricmp(lpszEntry, "LimitSH") == 0)
			sprintf(sValue, "%f", nc.m_fLimitSH);
		else if (stricmp(lpszEntry, "DxfMode") == 0)
			sprintf(sValue, "%d", nc.m_iDxfMode);
		else if (stricmp(lpszEntry, "DrillNeedSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bReserveBigSH);
		else if (stricmp(lpszEntry, "DrillReduceSmallSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bReduceSmallSH);
		else if (stricmp(lpszEntry, "DrillHoldSortType") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_ciHoldSortType);
		else if (stricmp(lpszEntry, "DrillSortHasBigSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bSortHasBigSH);
		else if (stricmp(lpszEntry, "PunchNeedSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bReserveBigSH);
		else if (stricmp(lpszEntry, "PunchReduceSmallSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bReduceSmallSH);
		else if (stricmp(lpszEntry, "PunchHoldSortType") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_ciHoldSortType);
		else if (stricmp(lpszEntry, "PunchSortHasBigSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bSortHasBigSH);
		else if (stricmp(lpszEntry, "DispMkRect") == 0)
			sprintf(sValue, "%d", nc.m_bDispMkRect);
		else if (stricmp(lpszEntry, "MKVectType") == 0)
			sprintf(sValue, "%d", nc.m_ciMkVect);
		else if (stricmp(lpszEntry, "m_ciDisplayType") == 0)
			sprintf(sValue, "%d", nc.m_ciDisplayType);
		
		//
		else if (stricmp(lpszEntry, "PbjIncVertex") == 0)
			sprintf(sValue, "%d", pbj.m_bIncVertex);
		else if (stricmp(lpszEntry, "PbjAutoSplitFile") == 0)
			sprintf(sValue, "%d", pbj.m_bAutoSplitFile);
		else if (stricmp(lpszEntry, "PbjMergeHole") == 0)
			sprintf(sValue, "%d", pbj.m_bMergeHole);
		//PMZ参数
		else if (stricmp(lpszEntry, "PmzMode") == 0)
			sprintf(sValue, "%d", pmz.m_iPmzMode);
		else if (stricmp(lpszEntry, "PmzIncVertex") == 0)
			sprintf(sValue, "%d", pmz.m_bIncVertex);
		else if (stricmp(lpszEntry, "PmzCheck") == 0)
			sprintf(sValue, "%d", pmz.m_bPmzCheck);
		//
		else if (stricmp(lpszEntry, "flameCut.m_bCutPosInInitPos") == 0)
			sprintf(sValue, "%d", flameCut.m_bCutPosInInitPos);
		else if (stricmp(lpszEntry, "flameCut.m_bInitPosFarOrg") == 0)
			sprintf(sValue, "%d", flameCut.m_bInitPosFarOrg);
		else if (stricmp(lpszEntry, "flameCut.m_sIntoLineLen") == 0)
			strcpy(sValue, flameCut.m_sIntoLineLen);
		else if (stricmp(lpszEntry, "flameCut.m_sOutLineLen") == 0)
			sprintf(sValue, flameCut.m_sOutLineLen);
		else if (stricmp(lpszEntry, "flameCut.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xFlamePara.m_wEnlargedSpace);
		else if (stricmp(lpszEntry, "flameCut.m_bCutSpecialHole") == 0)
			sprintf(sValue, "%d", nc.m_xFlamePara.m_bCutSpecialHole);
		//
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutPosInInitPos") == 0)
			sprintf(sValue, "%d", plasmaCut.m_bCutPosInInitPos);
		else if (stricmp(lpszEntry, "plasmaCut.m_bInitPosFarOrg") == 0)
			sprintf(sValue, "%d", plasmaCut.m_bInitPosFarOrg);
		else if (stricmp(lpszEntry, "plasmaCut.m_sIntoLineLen") == 0)
			strcpy(sValue, plasmaCut.m_sIntoLineLen);
		else if (stricmp(lpszEntry, "plasmaCut.m_sOutLineLen") == 0)
			strcpy(sValue, plasmaCut.m_sOutLineLen);
		else if (stricmp(lpszEntry, "plasmaCut.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xPlasmaPara.m_wEnlargedSpace);
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutSpecialHole") == 0)
			sprintf(sValue, "%d", nc.m_xPlasmaPara.m_bCutSpecialHole);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendLine") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_bOutputBendLine);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendType") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_bOutputBendType);
		else if (stricmp(lpszEntry, "laserPara.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_wEnlargedSpace);
		dwLength=strlen(sValue);
		RegSetValueEx(hKey,lpszEntry,NULL,REG_SZ,(BYTE*)&sValue[0],dwLength);
		RegCloseKey(hKey);
	}
}
//
void CSysPara::ReadSysParaFromReg(LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH],tem_str[100]="";;
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	HKEY hKey;
	DWORD dwDataType,dwLength=MAX_PATH;
	if(RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_READ,&hKey)==ERROR_SUCCESS&&hKey &&
		RegQueryValueEx(hKey,lpszEntry,NULL,&dwDataType,(BYTE*)&sValue[0],&dwLength)==ERROR_SUCCESS)
	{
		if (stricmp(lpszEntry, "MKRectL") == 0)
			nc.m_fMKRectL = atof(sValue);
		else if (stricmp(lpszEntry, "MKRectW") == 0)
			nc.m_fMKRectW = atof(sValue);
		else if (stricmp(lpszEntry, "MKHoleD") == 0)
			nc.m_fMKHoleD = atof(sValue);
		else if (stricmp(lpszEntry, "SideBaffleHigh") == 0)
			nc.m_fBaffleHigh = atof(sValue);
		else if (stricmp(lpszEntry, "TextHeight") == 0)
			font.fTextHeight = atof(sValue);
		else if (stricmp(lpszEntry, "DxfTextSize") == 0)
			font.fDxfTextSize = atof(sValue);
		else if (stricmp(lpszEntry, "LimitSH") == 0)
			nc.m_fLimitSH = atof(sValue);
		else if (stricmp(lpszEntry, "DxfMode") == 0)
			nc.m_iDxfMode = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillNeedSH") == 0)
			nc.m_xDrillPara.m_bReserveBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillReduceSmallSH") == 0)
			nc.m_xDrillPara.m_bReduceSmallSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillHoldSortType") == 0)
			nc.m_xDrillPara.m_ciHoldSortType = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillSortHasBigSH") == 0)
			nc.m_xDrillPara.m_bSortHasBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchNeedSH") == 0)
			nc.m_xPunchPara.m_bReserveBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchReduceSmallSH") == 0)
			nc.m_xPunchPara.m_bReduceSmallSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchHoldSortType") == 0)
			nc.m_xPunchPara.m_ciHoldSortType = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchSortHasBigSH") == 0)
			nc.m_xPunchPara.m_bSortHasBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DispMkRect") == 0)
			nc.m_bDispMkRect = atoi(sValue);
		else if (stricmp(lpszEntry, "MKVectType") == 0)
			nc.m_ciMkVect = atoi(sValue);
		else if (stricmp(lpszEntry, "m_ciDisplayType") == 0)
			nc.m_ciDisplayType = atoi(sValue);
		//
		else if (stricmp(lpszEntry, "PbjIncVertex") == 0)
			pbj.m_bIncVertex = atoi(sValue);
		else if (stricmp(lpszEntry, "PbjAutoSplitFile") == 0)
			pbj.m_bAutoSplitFile = atoi(sValue);
		else if (stricmp(lpszEntry, "PbjMergeHole") == 0)
			pbj.m_bMergeHole = atoi(sValue);
		//
		//PMZ参数
		else if (stricmp(lpszEntry, "PmzMode") == 0)
			pmz.m_iPmzMode = atoi(sValue);
		else if (stricmp(lpszEntry, "PmzIncVertex") == 0)
			pmz.m_bIncVertex = atoi(sValue);
		else if (stricmp(lpszEntry, "PmzCheck") == 0)
			pmz.m_bPmzCheck = atoi(sValue);
		//
		else if(stricmp(lpszEntry,"M12Color")==0)
		{
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crLS12);
		}
		else if(stricmp(lpszEntry,"M16Color")==0)
		{	
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crLS16);
		}
		else if(stricmp(lpszEntry,"M20Color")==0)
		{	
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crLS20);
		}
		else if(stricmp(lpszEntry,"M24Color")==0)
		{	
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crLS24);
		}
		else if(stricmp(lpszEntry,"OtherColor")==0)
		{	
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crOtherLS);
		}
		else if(stricmp(lpszEntry,"MarkColor")==0)
		{
			sprintf(tem_str,"%s",sValue);
			memmove(tem_str, tem_str+3, 97);
			sscanf(tem_str,"%X",&crMode.crMark);
		}
		else if (stricmp(lpszEntry, "EdgeColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crEdge);
		}
		else if (stricmp(lpszEntry, "HuoQuColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crHuoQu);
		}
		else if (stricmp(lpszEntry, "TextColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crText);
		}
		else if(stricmp(lpszEntry,"flameCut.m_bInitPosFarOrg")==0)
			flameCut.m_bInitPosFarOrg=atoi(sValue);
		else if(stricmp(lpszEntry,"flameCut.m_bCutPosInInitPos")==0)
			flameCut.m_bCutPosInInitPos=atoi(sValue);
		else if(stricmp(lpszEntry,"flameCut.m_sIntoLineLen")==0)
			flameCut.m_sIntoLineLen.Copy(sValue);
		else if(stricmp(lpszEntry,"flameCut.m_sOutLineLen")==0)
			flameCut.m_sOutLineLen.Copy(sValue);
		else if(stricmp(lpszEntry,"flameCut.m_wEnlargedSpace")==0)
			nc.m_xFlamePara.m_wEnlargedSpace=atoi(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_bCutSpecialHole") == 0)
			nc.m_xFlamePara.m_bCutSpecialHole = atoi(sValue);
		else if(stricmp(lpszEntry,"plasmaCut.m_bInitPosFarOrg")==0)
			plasmaCut.m_bInitPosFarOrg=atoi(sValue);
		else if(stricmp(lpszEntry,"plasmaCut.m_bCutPosInInitPos")==0)
			plasmaCut.m_bCutPosInInitPos=atoi(sValue);
		else if(stricmp(lpszEntry,"plasmaCut.m_sIntoLineLen")==0)
			plasmaCut.m_sIntoLineLen.Copy(sValue);
		else if(stricmp(lpszEntry,"plasmaCut.m_sOutLineLen")==0)
			plasmaCut.m_sOutLineLen.Copy(sValue);
		else if(stricmp(lpszEntry,"plasmaCut.m_wEnlargedSpace")==0)
			nc.m_xPlasmaPara.m_wEnlargedSpace=atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutSpecialHole") == 0)
			nc.m_xPlasmaPara.m_bCutSpecialHole = atoi(sValue);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendLine") == 0)
			nc.m_xLaserPara.m_bOutputBendLine = atoi(sValue);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendType") == 0)
			nc.m_xLaserPara.m_bOutputBendType = atoi(sValue);
		else if(stricmp(lpszEntry, "laserPara.m_wEnlargedSpace")==0)
			nc.m_xLaserPara.m_wEnlargedSpace = atoi(sValue);
		RegCloseKey(hKey);
	}
}
void CSysPara::UpdateHoleIncrement(double fHoleInc)
{
	if(fabs(holeIncrement.m_fM12-holeIncrement.m_fDatum)<=EPS)
		holeIncrement.m_fM12=fHoleInc;
	if(fabs(holeIncrement.m_fM16-holeIncrement.m_fDatum)<=EPS)
		holeIncrement.m_fM16=fHoleInc;
	if(fabs(holeIncrement.m_fM20-holeIncrement.m_fDatum)<=EPS)
		holeIncrement.m_fM20=fHoleInc;
	if(fabs(holeIncrement.m_fM24-holeIncrement.m_fDatum)<=EPS)
		holeIncrement.m_fM24=fHoleInc;
	if(fabs(holeIncrement.m_fCutSH-holeIncrement.m_fDatum)<=EPS)
		holeIncrement.m_fCutSH=fHoleInc;
	if (fabs(holeIncrement.m_fProSH-holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fProSH=fHoleInc;
	holeIncrement.m_fDatum=fHoleInc;
}
int CSysPara::GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen/*=100*/)
{
	BOOL bContinueJustify = FALSE;
	CXhChar200 sText;
	if(GetPropID("font.fTextHeight")==id)
	{
		sText.Printf("%f",font.fTextHeight);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("font.fDxfTextSize") == id)
	{
		sText.Printf("%f", font.fDxfTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_bDispMkRect")==id)
	{
		if(nc.m_bDispMkRect)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("nc.m_ciMkVect") == id)
	{
		if (nc.m_ciMkVect == 0)
			sText.Copy("0.保持原朝向");
		else
			sText.Copy("1.保持水平");
	}
	else if(GetPropID("nc.m_fMKRectW")==id)
	{
		sText.Printf("%f",nc.m_fMKRectW);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_fMKRectL")==id)
	{
		sText.Printf("%f",nc.m_fMKRectL);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_fMKHoleD")==id)
	{
		sText.Printf("%f",nc.m_fMKHoleD);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_iNcMode")==id)
	{
		if(IsValidNcFlag(CNCPart::FLAME_MODE))
			sText.Append("火焰", '+');
		if(IsValidNcFlag(CNCPart::PLASMA_MODE))
			sText.Append("等离子",'+');
		if(IsValidNcFlag(CNCPart::PUNCH_MODE))
			sText.Append("冲床",'+');
		if (IsValidNcFlag(CNCPart::DRILL_MODE))
			sText.Append("钻床", '+');
		if (IsValidNcFlag(CNCPart::LASER_MODE))
			sText.Append("激光", '+');
	}
	else if(GetPropID("nc.m_fLimitSH")==id)
	{
		sText.Printf("%f",nc.m_fLimitSH);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("CutLimitSH") == id)
		sText.Printf(">= %g", nc.m_fLimitSH);
	else if (GetPropID("ProLimitSH") == id)
		sText.Printf("< %g", nc.m_fLimitSH);
	else if(GetPropID("nc.DrillPara.m_bReserveBigSH")==id)
	{
		if(nc.m_xDrillPara.m_bReserveBigSH)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("nc.DrillPara.m_bSortHasBigSH") == id)
	{
		if (nc.m_xDrillPara.m_bSortHasBigSH)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("nc.DrillPara.m_bReduceSmallSH") == id)
	{
		if (nc.m_xDrillPara.m_bReduceSmallSH)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("nc.DrillPara.m_ciHoldSortType") == id)
	{
		if (nc.m_xDrillPara.m_ciHoldSortType == 0)
			sText.Copy("0.混排");
		else if (nc.m_xDrillPara.m_ciHoldSortType == 1)
			sText.Copy("1.分组+距离");
		else if (nc.m_xDrillPara.m_ciHoldSortType == 2)
			sText.Copy("2.分组+孔径");
	}
	else if(GetPropID("nc.m_fBaffleHigh")==id)
	{
		sText.Printf("%f",nc.m_fBaffleHigh);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_iDxfMode")==id)
	{
		if(nc.m_iDxfMode==0)
			sText.Copy("否");
		else if(nc.m_iDxfMode==1)
			sText.Copy("是");
	}
	else if(GetPropID("nc.m_sNcDriverPath")==id)
		sText.Copy(nc.m_sNcDriverPath);

	else if (GetPropID("nc.bFlameCut") == id)
	{
		if (IsValidNcFlag(CNCPart::FLAME_MODE))
			strcpy(sText, "启用");
		else
			strcpy(sText, "禁用");
	}
	else if (GetPropID("nc.FlamePara.m_sThick") == id)
		strcpy(sText, nc.m_xFlamePara.m_sThick);
	else if (GetPropID("nc.FlamePara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_TXT_FILE))
			sValue.Append("TXT", '+');
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_CNC_FILE))
			sValue.Append("CNC", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bPlasmaCut") == id)
	{
		if (IsValidNcFlag(CNCPart::PLASMA_MODE))
			strcpy(sText, "启用");
		else
			strcpy(sText, "禁用");
	}
	else if (GetPropID("nc.PlasmaPara.m_sThick") == id)
		strcpy(sText, nc.m_xPlasmaPara.m_sThick);
	else if (GetPropID("nc.PlasmaPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_NC_FILE))
			sValue.Append("NC", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bPunchPress") == id)
	{
		if (IsValidNcFlag(CNCPart::PUNCH_MODE))
			strcpy(sText, "启用");
		else
			strcpy(sText, "禁用");
	}
	else if (GetPropID("nc.PunchPara.m_sThick") == id)
		strcpy(sText, nc.m_xPunchPara.m_sThick);
	else if (GetPropID("nc.PunchPara.m_bReserveBigSH") == id)
	{
		if (nc.m_xPunchPara.m_bReserveBigSH)
			strcpy(sText, "是");
		else
			strcpy(sText, "否");
	}
	else if (GetPropID("nc.PunchPara.m_bSortHasBigSH") == id)
	{
		if (nc.m_xPunchPara.m_bSortHasBigSH)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("nc.PunchPara.m_bReduceSmallSH") == id)
	{
		if (nc.m_xPunchPara.m_bReduceSmallSH)
			strcpy(sText, "是");
		else
			strcpy(sText, "否");
	}
	else if (GetPropID("nc.PunchPara.m_ciHoldSortType") == id)
	{
		if (nc.m_xPunchPara.m_ciHoldSortType == 0)
			sText.Copy("0.混排");
		else if (nc.m_xPunchPara.m_ciHoldSortType == 1)
			sText.Copy("1.分组+距离");
		else if (nc.m_xPunchPara.m_ciHoldSortType == 2)
			sText.Copy("2.分组+孔径");
	}
	else if (GetPropID("nc.PunchPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_PBJ_FILE))
			sValue.Append("PBJ", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_WKF_FILE))
			sValue.Append("WKF", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_TTP_FILE))
			sValue.Append("TTP", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bDrillPress") == id)
	{
		if (IsValidNcFlag(CNCPart::DRILL_MODE))
			strcpy(sText, "启用");
		else
			strcpy(sText, "禁用");
	}
	else if (GetPropID("nc.DrillPara.m_sThick") == id)
		strcpy(sText, nc.m_xDrillPara.m_sThick);
	else if (GetPropID("nc.DrillPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_PMZ_FILE))
			sValue.Append("PMZ", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bLaser") == id)
	{
		if (IsValidNcFlag(CNCPart::LASER_MODE))
			strcpy(sText, "启用");
		else
			strcpy(sText, "禁用");
	}
	else if(GetPropID("nc.LaserPara.m_wEnlargedSpace")==id)
		sText.Printf("%d", nc.m_xLaserPara.m_wEnlargedSpace);
	else if (GetPropID("nc.LaserPara.m_sThick") == id)
		strcpy(sText, nc.m_xLaserPara.m_sThick);
	else if (GetPropID("nc.LaserPara.m_dwFileFlag") == id)
		strcpy(sText, "DXF");
	else if (GetPropID("nc.LaserPara.m_bOutputBendLine") == id)
	{
		if (nc.m_xLaserPara.m_bOutputBendLine)
			sText.Copy("是");
		else //if (nc.m_xLaserPara.m_bOutputBendLine)
			sText.Copy("否");
	}
	else if (GetPropID("nc.LaserPara.m_bOutputBendType") == id)
	{
		if (nc.m_xLaserPara.m_bOutputBendType)
			sText.Copy("是");
		else //if (nc.m_xLaserPara.m_bOutputBendType)
			sText.Copy("否");
	}
	else if (GetPropID("FileFormat") == id)
		sText.Copy(model.file_format.GetFileFormatStr());
	else if(GetPropID("OutputPath")==id)
		sText.Copy(model.m_sOutputPath);
	else if(GetPropID("pbj.m_bIncVertex")==id)
	{
		if(pbj.m_bIncVertex)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("pbj.m_bAutoSplitFile")==id)
	{
		if(pbj.m_bAutoSplitFile)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("pbj.m_bMergeHole") == id)
	{
	if (pbj.m_bMergeHole)
		sText.Copy("是");
	else
		sText.Copy("否");
	}
	else if (GetPropID("pmz.m_iPmzMode") == id)
	{
		if (pmz.m_iPmzMode == 1)
			sText.Copy("1.多文件");
		else //if(pmz.m_iPmzMode == 0)
			sText.Copy("0.单文件");
	}
	else if (GetPropID("pmz.m_bIncVertex") == id)
	{
		if (pmz.m_bIncVertex)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if (GetPropID("pmz.m_bPmzCheck") == id)
	{
		if (pmz.m_bPmzCheck)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
#ifdef __PNC_
	else if(GetPropID("nc.m_ciDisplayType")==id)
	{
		if (g_sysPara.nc.m_ciDisplayType == CNCPart::FLAME_MODE)
			sText.Copy("火焰");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::PLASMA_MODE)
			sText.Copy("等离子");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::PUNCH_MODE)
			sText.Copy("冲床");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::DRILL_MODE)
			sText.Copy("钻床");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::LASER_MODE)
			sText.Copy("激光");
		else if (g_sysPara.nc.m_ciDisplayType > 0)
			sText.Copy("复合模式");
		else
			sText.Copy("原始");
	}
	//等离子切割
	else if(GetPropID("plasmaCut.m_sOutLineLen")==id)
		sText.Copy(plasmaCut.m_sOutLineLen);
	else if(GetPropID("plasmaCut.m_sIntoLineLen")==id)
		sText.Copy(plasmaCut.m_sIntoLineLen);
	else if(GetPropID("plasmaCut.m_bInitPosFarOrg")==id)
	{
		if(!g_sysPara.plasmaCut.m_bInitPosFarOrg)
			sText.Copy("0.靠近原点");
		else
			sText.Copy("1.远离原点");
	}
	else if(GetPropID("plasmaCut.m_bCutPosInInitPos")==id)
	{
		if(plasmaCut.m_bCutPosInInitPos)
			sText.Copy("1.始终在初始点");
		else
			sText.Copy("0.在指定轮廓点");
	}
	else if(GetPropID("nc.PlasmaPara.m_wEnlargedSpace")==id)
		sText.Printf("%d",nc.m_xPlasmaPara.m_wEnlargedSpace);
	else if (GetPropID("nc.PlasmaPara.m_bCutSpecialHole") == id)
	{
		if (nc.m_xPlasmaPara.m_bCutSpecialHole)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	//火焰切割
	else if(GetPropID("flameCut.m_sOutLineLen")==id)
		sText.Copy(flameCut.m_sOutLineLen);
	else if(GetPropID("flameCut.m_sIntoLineLen")==id)
		sText.Copy(flameCut.m_sIntoLineLen);
	else if(GetPropID("flameCut.m_bInitPosFarOrg")==id)
	{
		if(!g_sysPara.flameCut.m_bInitPosFarOrg)
			sText.Copy("0.靠近原点");
		else
			sText.Copy("1.远离原点");
	}
	else if(GetPropID("flameCut.m_bCutPosInInitPos")==id)
	{
		if(flameCut.m_bCutPosInInitPos)
			sText.Copy("1.始终在初始点");
		else
			sText.Copy("0.在指定轮廓点");
	}
	else if(GetPropID("nc.FlamePara.m_wEnlargedSpace")==id)
		sText.Printf("%d", nc.m_xFlamePara.m_wEnlargedSpace);
	else if (GetPropID("nc.FlamePara.m_bCutSpecialHole") == id)
	{
		if(nc.m_xFlamePara.m_bCutSpecialHole)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
#endif
	else if(GetPropID("holeIncrement.m_fDatum")==id)
	{
		sText.Printf("%f",holeIncrement.m_fDatum);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM12")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM12);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM16")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM16);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM20")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM20);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM24")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM24);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fCutSH")==id)
	{
		sText.Printf("%f",holeIncrement.m_fCutSH);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("holeIncrement.m_fProSH") == id)
	{
		sText.Printf("%f", holeIncrement.m_fProSH);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("crMode.crLS12")==id)
		sText.Printf("RGB%X",crMode.crLS12);
	else if(GetPropID("crMode.crLS16")==id)
		sText.Printf("RGB%X",crMode.crLS16);
	else if(GetPropID("crMode.crLS20")==id)
		sText.Printf("RGB%X",crMode.crLS20);
	else if(GetPropID("crMode.crLS24")==id)
		sText.Printf("RGB%X",crMode.crLS24);
	else if(GetPropID("crMode.crOtherLS")==id)
		sText.Printf("RGB%X",crMode.crOtherLS);
	else if(GetPropID("crMode.crMarK")==id)
		sText.Printf("RGB%X",crMode.crMark);
	else if(GetPropID("crMode.crEdge")==id)
		sText.Printf("RGB%X",crMode.crEdge);
	else if(GetPropID("crMode.crHuoQu")==id)
		sText.Printf("RGB%X", crMode.crHuoQu);
	else if (GetPropID("crMode.crText") == id)
		sText.Printf("RGB%X", crMode.crText);
	else if(GetPropID("font.fDimTextSize")==id)
	{
		sText.Printf("%f",font.fDimTextSize);
		SimplifiedNumString(sText);
	}
	else
		bContinueJustify = TRUE;
	if (!bContinueJustify)
	{	//if-else判断太多编译通不过去，所以只能在中间断开
		if (valueStr)
			StrCopy(valueStr, sText, nMaxStrBufLen);
		return strlen(sText);
	}
	bContinueJustify = FALSE;
	if(GetPropID("jgDrawing.iDimPrecision")==id)
	{
		if(jgDrawing.iDimPrecision==0)
			sText.Copy("1.0mm");
		else if(jgDrawing.iDimPrecision==1)
			sText.Copy("0.5mm");
		else //if(jgDrawing.iDimPrecision==2)
			sText.Copy("0.1mm");
	}
	else if(GetPropID("jgDrawing.fRealToDraw")==id)
	{
		sText.Printf("%f",jgDrawing.fRealToDraw);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fDimArrowSize")==id)
	{
		sText.Printf("%f",jgDrawing.fDimArrowSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fTextXFactor")==id)
	{
		sText.Printf("%f",jgDrawing.fTextXFactor);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("font.fPartNoTextSize")==id)
	{
		sText.Printf("%f",font.fPartNoTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.iPartNoFrameStyle")==id)
	{
		if(jgDrawing.iPartNoFrameStyle==0)
			sText.Printf("%s","0.圆圈");
		else if(jgDrawing.iPartNoFrameStyle==1)
			sText.Printf("%s","1.腰圆矩形");
		else if(jgDrawing.iPartNoFrameStyle==2)
			sText.Printf("%s","2.普通矩形");
		else 
			sText.Printf("%s","3.自动判断");
	}
	else if(GetPropID("jgDrawing.fPartNoMargin")==id)
	{
		sText.Printf("%f",jgDrawing.fPartNoMargin);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fPartNoCirD")==id)
	{
		sText.Printf("%f",jgDrawing.fPartNoCirD);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fPartGuigeTextSize")==id)
	{
		sText.Printf("%f",jgDrawing.fPartGuigeTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.iMatCharPosType")==id)
	{
		if(jgDrawing.iMatCharPosType==0)
			sText.Printf("%s","0.不需要材质字符");
		else if(jgDrawing.iMatCharPosType==1)
			sText.Printf("%s","1.构件编号前");
		else if(jgDrawing.iMatCharPosType==2)
			sText.Printf("%s","2.构件编号后");
	}
	//角钢构件图设置
	else if(GetPropID("jgDrawing.fLsDistThreshold")==id)
	{
		sText.Printf("%f",jgDrawing.fLsDistThreshold);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fLsDistZoomCoef")==id)
	{
		sText.Printf("%f",jgDrawing.fLsDistZoomCoef);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.bOneCardMultiPart")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bModulateLongJg")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.iJgZoomSchema")==id)
	{
		if(jgDrawing.iJgZoomSchema==1)
			sText.Printf("1.使用构件图比例");
		else if(jgDrawing.iJgZoomSchema==2)
			sText.Printf("2.长宽同比缩放");
		else if(jgDrawing.iJgZoomSchema==3)
			sText.Printf("3.长宽分别缩放");
		else 
			sText.Printf("0.无缩放1:1绘制");
	}
	else if(GetPropID("jgDrawing.bMaxExtendAngleLength")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.iJgGDimStyle")==id)
	{
		if(jgDrawing.iJgGDimStyle==0)
			sText.Printf("%s","0.始端");
		else if(jgDrawing.iJgGDimStyle==1)
			sText.Printf("%s","1.中间");
		else 
			sText.Printf("%s","2.自动判断");
	}
	else if(GetPropID("jgDrawing.nMaxBoltNumStartDimG")==id)
		sText.Printf("%d",jgDrawing.nMaxBoltNumStartDimG);
	else if(GetPropID("jgDrawing.iLsSpaceDimStyle")==id)
	{
		if(jgDrawing.iLsSpaceDimStyle==0)
			sText.Printf("%s","0.沿X轴方向");
		else if(jgDrawing.iLsSpaceDimStyle==1)
			sText.Printf("%s","1.沿Y轴方向");
		else if(jgDrawing.iLsSpaceDimStyle==3)
			sText.Printf("%s","3.不标注间距");
		else if(jgDrawing.iLsSpaceDimStyle==4)
			sText.Printf("%s","4.仅标尺寸线");
		else
			sText.Printf("%s","2.自动判断");
	}
	else if(GetPropID("jgDrawing.nMaxBoltNumAlongX")==id)
		sText.Printf("%d",jgDrawing.nMaxBoltNumAlongX);
	else if(GetPropID("jgDrawing.bDimCutAngle")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimCutAngleMap")==id)
	{
		if(jgDrawing.bDimCutAngleMap)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimPushFlatMap")==id)
	{
		if(jgDrawing.bDimPushFlatMap)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiHe")==id)
	{
		if(jgDrawing.bDimKaiHe)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiheAngleMap")==id)
	{
		if(jgDrawing.bDimKaiheAngleMap)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiheSumLen")==id)
	{
		if(jgDrawing.bDimKaiheSumLen)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiheAngle")==id)
	{
		if(jgDrawing.bDimKaiheAngle)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiheSegLen")==id)
	{
		if(jgDrawing.bDimKaiheSegLen)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimKaiheScopeMap")==id)
	{
		if(jgDrawing.bDimKaiheScopeMap)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bJgUseSimpleLsMap")==id)
	{
		if(jgDrawing.bJgUseSimpleLsMap)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimLsAbsoluteDist")==id)
	{
		if(jgDrawing.bDimLsAbsoluteDist)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bMergeLsAbsoluteDist")==id)
	{
		if(jgDrawing.bMergeLsAbsoluteDist)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimRibPlatePartNo")==id)
	{
		if(jgDrawing.bDimRibPlatePartNo)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.bDimRibPlateSetUpPos")==id)
	{
		if(jgDrawing.bDimRibPlateSetUpPos)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("jgDrawing.iCutAngleDimType")==id)
	{
		if(jgDrawing.iCutAngleDimType==1)
			sText.Printf("%s","1.样式二");
		else
			sText.Printf("%s","0.样式一");
	}
	else if(GetPropID("jgDrawing.fKaiHeJiaoThreshold")==id)
	{
		sText.Printf("%f",jgDrawing.fKaiHeJiaoThreshold);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.sAngleCardPath")==id)
		sText.Copy(jgDrawing.sAngleCardPath);
	else if(GetPropID("model.m_sCompanyName")==id)
		sText.Copy(model.m_sCompanyName);
	else if(GetPropID("model.m_sPrjCode")==id)
		sText.Copy(model.m_sPrjCode);
	else if(GetPropID("model.m_sPrjName")==id)
		sText.Copy(model.m_sPrjName);
	else if(GetPropID("model.m_sTaType")==id)
		sText.Copy(model.m_sTaType);
	else if(GetPropID("model.m_sTaAlias")==id)
		sText.Copy(model.m_sTaAlias);
	else if(GetPropID("model.m_sTaStampNo")==id)
		sText.Copy(model.m_sTaStampNo);
	else if(GetPropID("model.m_sOperator")==id)
		sText.Copy(model.m_sOperator);
	else if(GetPropID("model.m_sAuditor")==id)
		sText.Copy(model.m_sAuditor);
	else if(GetPropID("model.m_sCritic")==id)
		sText.Copy(model.m_sCritic);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return sText.Length();
}
void CSysPara::AngleDrawingParaToBuffer(CBuffer &buffer)
{
	buffer.WriteDouble(font.fDimTextSize);
	buffer.WriteInteger(jgDrawing.iDimPrecision);
	buffer.WriteDouble(jgDrawing.fRealToDraw);
	buffer.WriteDouble(jgDrawing.fDimArrowSize);
	buffer.WriteDouble(jgDrawing.fTextXFactor);
	buffer.WriteDouble(font.fPartNoTextSize);
	buffer.WriteInteger(jgDrawing.iPartNoFrameStyle);
	buffer.WriteDouble(jgDrawing.fPartNoMargin);
	buffer.WriteDouble(jgDrawing.fPartNoCirD);
	buffer.WriteDouble(jgDrawing.fPartGuigeTextSize);
	buffer.WriteInteger(jgDrawing.iMatCharPosType);
	buffer.WriteInteger(jgDrawing.bModulateLongJg);
	buffer.WriteInteger(jgDrawing.iJgZoomSchema);
	buffer.WriteInteger(jgDrawing.bMaxExtendAngleLength);
	//buffer.WriteDouble(jgDrawing.fLsDistThreshold);		//角钢长度自动调整螺栓间距阈值(大于此间距时就要进行调整);
	//buffer.WriteDouble(jgDrawing.fLsDistZoomCoef);		//螺栓间距缩放系数
	buffer.WriteInteger(jgDrawing.bOneCardMultiPart);
	buffer.WriteInteger(jgDrawing.iJgGDimStyle);
	buffer.WriteInteger(jgDrawing.nMaxBoltNumStartDimG);
	buffer.WriteInteger(jgDrawing.iLsSpaceDimStyle);
	buffer.WriteInteger(jgDrawing.nMaxBoltNumAlongX);
	buffer.WriteInteger(jgDrawing.bDimCutAngle);
	buffer.WriteInteger(jgDrawing.bDimCutAngleMap);
	buffer.WriteInteger(jgDrawing.bDimPushFlatMap);
	buffer.WriteInteger(jgDrawing.bJgUseSimpleLsMap);
	buffer.WriteInteger(jgDrawing.bDimLsAbsoluteDist);
	buffer.WriteInteger(jgDrawing.bMergeLsAbsoluteDist);
	buffer.WriteInteger(jgDrawing.bDimRibPlatePartNo);
	buffer.WriteInteger(jgDrawing.bDimRibPlateSetUpPos);
	buffer.WriteInteger(jgDrawing.iCutAngleDimType);
	buffer.WriteInteger(jgDrawing.bDimKaiHe);
	buffer.WriteInteger(jgDrawing.bDimKaiheAngleMap);
	buffer.WriteDouble(jgDrawing.fKaiHeJiaoThreshold);
	buffer.WriteInteger(jgDrawing.bDimKaiheSumLen);
	buffer.WriteInteger(jgDrawing.bDimKaiheAngle);
	buffer.WriteInteger(jgDrawing.bDimKaiheSegLen);
	buffer.WriteInteger(jgDrawing.bDimKaiheScopeMap);
}
CSysPara g_sysPara;