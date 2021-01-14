#include "stdafx.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "CadToolFunc.h"
#include "PNCCryptCoreCode.h"
#ifdef __TIMER_COUNT_
#include "TimerCount.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPNCModel model;
//////////////////////////////////////////////////////////////////////////
//CPNCModel
CPNCModel::CPNCModel()
{
	Empty();
}
CPNCModel::~CPNCModel()
{

}
void CPNCModel::Empty()
{
	m_hashPlateInfo.Empty();
	m_sCurWorkFile.Empty();
}
//
bool CPNCModel::ExtractPlates(CString sDwgFile, BOOL bSupportSelectEnts/*=FALSE*/)
{
	model.m_sCurWorkFile = sDwgFile;
	IExtractor* pExtractor = g_xExtractorManager.GetExtractor(IExtractor::PLATE);
	if (pExtractor)
		return pExtractor->ExtractPlates(this->m_hashPlateInfo, bSupportSelectEnts);
	return false;
}
//
void CPNCModel::CreatePlatePPiFile(const char* work_path)
{
	for (CPlateProcessInfo* pPlateProcess = m_hashPlateInfo.GetFirst(); pPlateProcess; pPlateProcess = m_hashPlateInfo.GetNext())
	{	
		if (!pPlateProcess->IsValid())
			continue;
		pPlateProcess->CreatePPiFile(work_path);
	}
}
//自动排版
void CPNCModel::DrawPlatesToLayout()
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f2dRect minRect;
	int minDistance = g_pncSysPara.m_nMinDistance;
	int hight = g_pncSysPara.m_nMapWidth;
	int paperLen = (g_pncSysPara.m_nMapLength <= 0) ? 100000 : g_pncSysPara.m_nMapLength;
	int paperWidth = (hight <= 0) ? 0 : hight;
	CSortedModel sortedModel(this);
	sortedModel.DividPlatesByPartNo();
	double paperX = 0, paperY = 0;
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		CHashStrList<CDrawingRect> hashDrawingRectByLabel;
		for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
		{
			fPtList ptList;
			minRect = pPlate->GetMinWrapRect((double)minDistance, &ptList, paperWidth);
			CDrawingRect *pRect = hashDrawingRectByLabel.Add(pPlate->GetPartNo());
			pRect->m_pDrawing = pPlate;
			pRect->height = minRect.Height();
			pRect->width = minRect.Width();
			pRect->m_vertexArr.Empty();
			for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
				pRect->m_vertexArr.append(GEPOINT(*pPt));
			pRect->topLeft.Set(minRect.topLeft.x, minRect.topLeft.y);
		}
		paperX = 0;
		while (hashDrawingRectByLabel.GetNodeNum() > 0)
		{
			CDrawingRectLayout rectLayout;
			for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				rectLayout.drawRectArr.Add(*pRect);
			if (rectLayout.Relayout(hight, paperLen) == FALSE)
			{	//所有的板都布局失败
				for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				{
					CPlateProcessInfo *pPlateDraw = (CPlateProcessInfo*)pRect->m_pDrawing;
					if (pRect->m_bException)
						logerr.Log("钢板%s排版失败,钢板矩形宽度大于指定出图宽度!", (char*)pPlateDraw->GetPartNo());
					else
						logerr.Log("钢板%s排版失败!", (char*)pPlateDraw->GetPartNo());
				}
				break;
			}
			else
			{	//布局成功，但是其中可能某些板没有布局成功
				AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY), f3dPoint(paperX + paperLen, paperY));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY - hight), f3dPoint(paperX + paperLen, paperY - hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, paperY), f3dPoint(paperX, paperY - hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX + paperLen, paperY), f3dPoint(paperX + paperLen, paperY - hight));
				pBlockTableRecord->close();
				f3dPoint topLeft;
				for (int i = 0; i < rectLayout.drawRectArr.GetSize(); i++)
				{
					CDrawingRect drawingRect = rectLayout.drawRectArr[i];
					CPlateProcessInfo *pPlate = (CPlateProcessInfo*)drawingRect.m_pDrawing;
					if (!drawingRect.m_bLayout)
						continue;
					topLeft = drawingRect.topLeft;
					topLeft.x += paperX;
					topLeft.y += paperY;
					GEPOINT center(topLeft.x + drawingRect.width*0.5, topLeft.y - drawingRect.height*0.5);
					pPlate->DrawPlate(&topLeft, FALSE, TRUE, &center);
					//
					hashDrawingRectByLabel.DeleteNode(pPlate->GetPartNo());
				}
				paperX += (paperLen + 50);
			}
		}
		paperY -= (hight + 50);
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
void CPNCModel::DrawPlatesToFiltrate()
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f3dPoint datum_pos;
	double fSegSpace = 0;
	CSortedModel sortedModel(this);
	if (g_pncSysPara.m_ciGroupType == 1)
		sortedModel.DividPlatesBySeg();			//根据段号对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 2)
		sortedModel.DividPlatesByThickMat();	//根据板厚材质对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 3)
		sortedModel.DividPlatesByMat();			//根据材质对钢板进行分组
	else if (g_pncSysPara.m_ciGroupType == 4)
		sortedModel.DividPlatesByThick();		//根据板厚对钢板进行分组
	else
		sortedModel.DividPlatesByPartNo();		//根据件号对钢板进行分组
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;
		pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		datum_pos.x = 0;
		datum_pos.y += fSegSpace;
		double fRectH = pGroup->GetMaxHight() + 20;
		fSegSpace = fRectH + 50;
		for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
		{
			SCOPE_STRU scope = pPlate->GetCADEntScope();
			pPlate->InitLayoutVertex(scope, 0);
			pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
			datum_pos.x += scope.wide() + 50;
		}
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//钢板对比
void CPNCModel::DrawPlatesToCompare()
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	f3dPoint datum_pos;
	double fSegSpace = 0;
	CSortedModel sortedModel(this);
	sortedModel.DividPlatesBySeg();			//根据段号对钢板进行分组
	for (CSortedModel::PARTGROUP* pGroup = sortedModel.hashPlateGroup.GetFirst(); pGroup;
		pGroup = sortedModel.hashPlateGroup.GetNext())
	{
		if (g_pncSysPara.m_ciArrangeType == 0)
		{	//以行为主
			datum_pos.x = 0;
			datum_pos.y += fSegSpace;
			double fRectH = pGroup->GetMaxHight() + 20;
			fSegSpace = fRectH * 2 + 100;
			for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
			{
				SCOPE_STRU scope = pPlate->GetCADEntScope();
				pPlate->InitLayoutVertex(scope, 0);
				pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
				pPlate->DrawPlateProfile(&f3dPoint(datum_pos.x, datum_pos.y + fRectH));
				datum_pos.x += scope.wide() + 50;
			}
		}
		else
		{	//以列为主
			datum_pos.x += fSegSpace;
			datum_pos.y = 0;
			double fRectW = pGroup->GetMaxWidth() + 20;
			fSegSpace = fRectW * 2 + 500;
			for (CPlateProcessInfo *pPlate = pGroup->EnumFirstPlate(); pPlate; pPlate = pGroup->EnumNextPlate())
			{
				SCOPE_STRU scope = pPlate->GetCADEntScope();
				pPlate->InitLayoutVertex(scope, 1);
				pPlate->DrawPlate(&f3dPoint(datum_pos.x, datum_pos.y));
				pPlate->DrawPlateProfile(&f3dPoint(datum_pos.x + fRectW, datum_pos.y));
				datum_pos.y -= (scope.high() + 50);
			}
		}
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//下料预审
void CPNCModel::DrawPlatesToProcess()
{
	CLockDocumentLife lockCurDocumentLife;
	if (m_hashPlateInfo.GetNodeNum() <= 0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	DRAGSET.ClearEntSet();
	CSortedModel sortedModel(this);
	int nSegCount = 0;
	SEGI prevSegI, curSegI;
	f3dPoint datum_pos;
	const int PAPER_WIDTH = 1500;
	//输出
	for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
	{
		CXhChar16 sPartNo = pPlate->GetPartNo();
		ParsePartNo(sPartNo, &curSegI, NULL, "SHPGT");
		CXhChar16 sSegStr = curSegI.ToString();
		if (sSegStr.GetLength() > 3)
		{	//段号字符串长度大于3时，再次从段号中提取一次分段号（处理5401-48类件号） wht 19-03-07
			SEGI segI;
			if (ParsePartNo(sSegStr, &segI, NULL, "SHPGT"))
				curSegI = segI;
		}
		f2dRect rect;
		pPlate->InitLayoutVertexByBottomEdgeIndex(rect);
		if (rect.Width() < EPS2 || rect.Height() <= EPS2)
			rect = pPlate->GetPnDimRect();
		double wide = (rect.Width() + rect.Height())*1.5;
		double high = rect.Height();
		f3dPoint leftBtm(datum_pos.x, datum_pos.y + high);
		pPlate->DrawPlate(&leftBtm, TRUE);
		datum_pos.x += wide;
		if (prevSegI.iSeg == 0)
			prevSegI = curSegI;
		else if (prevSegI.iSeg != curSegI.iSeg)
		{
			prevSegI = curSegI;
			datum_pos.x = 0;
			datum_pos.y -= PAPER_WIDTH;
		}
	}
	CXhPtrSet<CPlateProcessInfo> needUpdatePlateList;
	for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
	{
		//初始化轮廓边对应关系
		pPlate->InitEdgeEntIdMap();	
		//调整钢印位置
		if (pPlate->AutoCorrectedSteelSealPos())
			needUpdatePlateList.append(pPlate);
	}
	//更新字盒子位置之后，同步更新PPI文件中钢印号位置
	for (CPlateProcessInfo *pPlate = needUpdatePlateList.GetFirst(); pPlate; pPlate = needUpdatePlateList.GetNext())
	{	
		pPlate->SyncSteelSealPos();
		//更新PPI文件
		CString file_path;
		GetCurWorkPath(file_path);
		pPlate->CreatePPiFile(file_path);
	}
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//钢板相关图元克隆
void CPNCModel::DrawPlatesToClone()
{
	CLockDocumentLife lockCurDocumentLife;
	DRAGSET.ClearEntSet();
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
		pPlate->DrawPlate();
#ifdef __DRAG_ENT_
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#endif
}
//
void CPNCModel::DrawPlates()
{
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{
		logerr.Log("缺少钢板信息，请先正确提取钢板信息！");
		return;
	}
	if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PRINT)
		return DrawPlatesToLayout();
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_PROCESS)
		return DrawPlatesToProcess();
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_COMPARE)
		return DrawPlatesToCompare();
	else if (g_pncSysPara.m_ciLayoutMode == CPNCSysPara::LAYOUT_FILTRATE)
		return DrawPlatesToFiltrate();
	else
		return DrawPlatesToClone();
}

CPlateProcessInfo* CPNCModel::GetPlateInfo(AcDbObjectId partNoEntId)
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(); pPlate; pPlate = EnumNextPlate())
	{
		if (pPlate->partNoId == partNoEntId)
			return pPlate;
	}
	return NULL;
}



void CPNCModel::WritePrjTowerInfoToCfgFile(const char* cfg_file_path)
{
	if(cfg_file_path==NULL||strlen(cfg_file_path)<=0)
		return;
	FILE *fp=fopen(cfg_file_path,"wt");
	if(fp==NULL)
		return;
	if(m_xPrjInfo.m_sPrjName.GetLength()>0)
		fprintf(fp,"PROJECT_NAME=%s\n",(char*)m_xPrjInfo.m_sPrjName);
	if(m_xPrjInfo.m_sPrjCode.GetLength()>0)
		fprintf(fp,"PROJECT_CODE=%s\n",(char*)m_xPrjInfo.m_sPrjCode);
	if(m_xPrjInfo.m_sTaType.GetLength()>0)
		fprintf(fp,"TOWER_NAME=%s\n",(char*)m_xPrjInfo.m_sTaType);
	if(m_xPrjInfo.m_sTaAlias.GetLength()>0)
		fprintf(fp,"TOWER_CODE=%s\n",(char*)m_xPrjInfo.m_sTaAlias);
	if(m_xPrjInfo.m_sTaStampNo.GetLength()>0)
		fprintf(fp,"STAMP_NO=%s\n",(char*)m_xPrjInfo.m_sTaStampNo);
	fclose(fp);
}
