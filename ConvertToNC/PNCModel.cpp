#include "stdafx.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "PNCCryptCoreCode.h"
#include "ComparePartNoString.h"
#include "FileIO.h"
#include "ExcelOper.h"
#include "BatchPrint.h"
#include "Expression.h"
#ifdef __TIMER_COUNT_
#include "TimerCount.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPNCModel model;
#ifdef __UBOM_ONLY_
CUbomModel g_xUbomModel;
#endif
//////////////////////////////////////////////////////////////////////////
//
int compare_func(const CXhChar16& str1, const CXhChar16& str2)
{
	CString keyStr1(str1), keyStr2(str2);
	return ComparePartNoString(keyStr1, keyStr2, "SHGPT");
}
//////////////////////////////////////////////////////////////////////////
//ExtractPlateInfo
//ExtractPlateProfile
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

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//CProjectTowerType
CProjectTowerType::CProjectTowerType()
{

}
CProjectTowerType::~CProjectTowerType()
{

}
//读取料单工程文件
void CProjectTowerType::ReadProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath, FALSE);
	double version = 1.0;
	file.Read(&version, sizeof(double));
	file.ReadString(m_sProjName);
	CXhChar100 sValue;
	file.ReadString(sValue);	//TMA料单
	InitBomInfo(sValue, TRUE);
	file.ReadString(sValue);	//ERP料单
	InitBomInfo(sValue, FALSE);
	//DWG文件信息
	DWORD n = 0;
	file.Read(&n, sizeof(DWORD));
	for (DWORD i = 0; i < n; i++)
	{
		int ibValue = 0;
		file.ReadString(sValue);
		file.Read(&ibValue, sizeof(int));
		AppendDwgBomInfo(sValue);
	}
}
//导出料单工程文件
void CProjectTowerType::WriteProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath, TRUE);
	double version = 1.0;
	file.Write(&version, sizeof(double));
	file.WriteString(m_sProjName);
	file.WriteString(m_xLoftBom.m_sFileName);	//TMA料单
	file.WriteString(m_xOrigBom.m_sFileName);	//ERP料单
	//DWG文件信息
	DWORD n = dwgFileList.GetNodeNum();
	file.Write(&n, sizeof(DWORD));
	for (CDwgFileInfo* pDwgInfo = dwgFileList.GetFirst(); pDwgInfo; pDwgInfo = dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->m_sFileName);
		int ibValue = 0;
		if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() <= 0)
			ibValue = 1;
		else if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() > 0)
			ibValue = 2;
		file.Write(&ibValue, sizeof(int));
	}
}
//从料单中读取杆塔的工程信息
BOOL CProjectTowerType::ReadTowerPrjInfo(const char* sFileName)
{
	DisplayCadProgress(0, "读取杆塔工程信息......");
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(sFileName))
	{
		DisplayCadProgress(100);
		return FALSE;
	}
	_Worksheet   excel_sheet;    // 工作表
	Sheets       excel_sheets;
	LPDISPATCH   pWorksheets = excelobj.GetWorksheets();
	ASSERT(pWorksheets != NULL);
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)1));
	excel_sheet.AttachDispatch(pWorksheet);
	VARIANT value;
	//
	DisplayCadProgress(10);
	CString sCell = g_xUbomModel.m_xMapPrjCell["工程名称"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sPrjName = VariantToString(value);
	//
	DisplayCadProgress(25);
	sCell = g_xUbomModel.m_xMapPrjCell["工程塔型"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaType = VariantToString(value);
	//
	DisplayCadProgress(40);
	sCell = g_xUbomModel.m_xMapPrjCell["材料标准"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sMatStandard = VariantToString(value);
	//
	DisplayCadProgress(55);
	sCell = g_xUbomModel.m_xMapPrjCell["下达单号"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
	{
		if (value.vt == VT_R8)
			m_xPrjInfo.m_sTaskNo.Printf("%.0f", value.dblVal);
		else
			m_xPrjInfo.m_sTaskNo = VariantToString(value);
	}	
	//
	DisplayCadProgress(70);
	sCell = g_xUbomModel.m_xMapPrjCell["塔规格"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaAlias = VariantToString(value);
	//
	DisplayCadProgress(80);
	sCell = g_xUbomModel.m_xMapPrjCell["合同号"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sContractNo = VariantToString(value);
	//
	DisplayCadProgress(95);
	sCell = g_xUbomModel.m_xMapPrjCell["基数"];
	CExcelOper::GetRangeValue(excel_sheet, sCell.GetBuffer(), NULL, value);
	if (value.vt != VT_EMPTY)
		m_xPrjInfo.m_sTaNum = VariantToString(value);
	//
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
	DisplayCadProgress(100);
	return TRUE;
}
//初始化BOM信息
void CProjectTowerType::InitBomInfo(const char* sFileName, BOOL bLoftBom)
{
	CXhChar100 sName;
	_splitpath(sFileName, NULL, NULL, sName, NULL);
	if (bLoftBom)
	{
		m_xLoftBom.m_sBomName.Copy(sName);
		m_xLoftBom.m_sFileName.Copy(sFileName);
		m_xLoftBom.SetBelongModel(this);
		m_xLoftBom.ImportExcelFile(&g_xBomCfg.m_xTmaTblCfg);
	}
	else
	{
		m_xOrigBom.m_sBomName.Copy(sName);
		m_xOrigBom.m_sFileName.Copy(sFileName);
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImportExcelFile(&g_xBomCfg.m_xErpTblCfg);
	}
}
//添加角钢DWGBOM信息
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName)
{
	if (strlen(sFileName) <= 0)
		return NULL;
	//打开DWG文件
	CXhChar500 file_path;
	AcApDocument *pDoc = NULL;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, sFileName))
			break;
	}
	Acad::ErrorStatus eStatue;
	if (strstr(file_path, sFileName))	//激活指定文件
		eStatue = acDocManager->activateDocument(pDoc);
	else
	{		//打开指定文件
#ifdef _ARX_2007
		eStatue = acDocManager->appContextOpenDocument(_bstr_t(sFileName));
#else
		eStatue = acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	if (eStatue != Acad::eOk)
		return NULL;
	//读取DWG文件信息
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		CXhChar100 sName;
		_splitpath(sFileName, NULL, NULL, sName, NULL);
		pDwgFile = dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		pDwgFile->m_sFileName.Copy(sFileName);
		pDwgFile->m_sDwgName.Copy(sName);
	}
	return pDwgFile;
}
//删除DWG信息
void CProjectTowerType::DeleteDwgBomInfo(CDwgFileInfo* pDwgInfo)
{
	//关闭对应的DWG文件
	AcApDocument *pDoc = NULL;
	CXhChar500 file_path;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, pDwgInfo->m_sFileName))
			break;
	}
	if (pDoc)
		CloseDoc(pDoc);
	//数据模型中删除DWG信息
	for (CDwgFileInfo *pTempDwgInfo = dwgFileList.GetFirst(); pTempDwgInfo; pTempDwgInfo = dwgFileList.GetNext())
	{
		if (pTempDwgInfo == pDwgInfo)
		{
			dwgFileList.DeleteCursor();
			break;
		}
	}
	dwgFileList.Clean();
}
//
CDwgFileInfo* CProjectTowerType::FindDwgBomInfo(const char* sFileName)
{
	CDwgFileInfo* pDwgInfo = NULL;
	for (pDwgInfo = dwgFileList.GetFirst(); pDwgInfo; pDwgInfo = dwgFileList.GetNext())
	{
		if (strcmp(pDwgInfo->m_sFileName, sFileName) == 0)
			break;
	}
	return pDwgInfo;
}
//
void CProjectTowerType::CompareData(BOMPART* pSrcPart, BOMPART* pDesPart, 
				CHashStrList<BOOL> &hashBoolByPropName,BOOL bBomToBom /*= FALSE*/)
{
	hashBoolByPropName.Empty();
	//规格
	CString sSpec1(pSrcPart->GetSpec()), sSpec2(pDesPart->GetSpec());
	sSpec1 = sSpec1.Trim();
	sSpec2 = sSpec2.Trim();
	if (pSrcPart->cPartType != pDesPart->cPartType ||
		sSpec1.CompareNoCase(sSpec2) != 0) //stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_SPEC), TRUE);
	//材质
	if (toupper(pSrcPart->cMaterial) != toupper(pDesPart->cMaterial))
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MATERIAL), TRUE);
	else if (pSrcPart->cMaterial != pDesPart->cMaterial && !g_xUbomModel.m_bEqualH_h)
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MATERIAL), TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MATERIAL), TRUE);
	//质量等级
	if (g_xUbomModel.m_bCmpQualityLevel && pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MATERIAL), TRUE);
	//单基数(双料单对比特殊处理)
	if (bBomToBom && pSrcPart->GetPartNum() != pDesPart->GetPartNum() &&
		(g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_SING_NUM) ||
		g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_SING_NUM)))
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_SING_NUM), TRUE);
	//加工数(双料单对比特殊处理)
	if(bBomToBom && pSrcPart->nSumPart!=pDesPart->nSumPart &&
		(g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_NUM) ||
		g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_NUM)))
		hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MANU_NUM), TRUE);
	//角钢定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_SING_NUM), TRUE);
		//加工数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MANU_NUM), TRUE);
		//基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_TA_NUM) &&
			pSrcPart->nTaNum != pDesPart->nTaNum)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_TA_NUM), TRUE);
		//螺栓数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_LS_NUM) &&
			pSrcPart->nMSumLs != pDesPart->nMSumLs)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_LS_NUM), TRUE);
		//总重
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MANU_WEIGHT), TRUE);
		//焊接
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_WELD), TRUE);
		//制弯
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_ZHI_WAN), TRUE);
		//长度对比
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_LEN))
		{
			if (g_xUbomModel.m_uiCustomizeSerial == ID_JiangSu_HuaDian)
			{	//江苏华电要求图纸长度大于放样长度
				if (pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_LEN), TRUE);
			}
			else
			{	//判断数值是否相等
				if (fabsl(pSrcPart->length - pDesPart->length) > g_xUbomModel.m_fMaxLenErr)
					hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_LEN), TRUE);
			}
		}
		//切角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_ANGLE) &&
			pSrcJg->bCutAngle != pDesJg->bCutAngle)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_CUT_ANGLE), TRUE);
		//压扁
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_PUSH_FLAT) &&
			pSrcJg->nPushFlat != pDesJg->nPushFlat)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_PUSH_FLAT), TRUE);
		//铲背
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_BER) &&
			pSrcJg->bCutBer != pDesJg->bCutBer)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_CUT_BER), TRUE);
		//刨根
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_CUT_ROOT) &&
			pSrcJg->bCutRoot != pDesJg->bCutRoot)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_CUT_ROOT), TRUE);
		//开角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_KAI_JIAO) &&
			pSrcJg->bKaiJiao != pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_KAI_JIAO), TRUE);
		//合角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_HE_JIAO) &&
			pSrcJg->bHeJiao != pDesJg->bHeJiao)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_HE_JIAO), TRUE);
		//脚钉
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::I_FOOT_NAIL) &&
			pSrcJg->bHasFootNail != pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_FOOT_NAIL), TRUE);
	}
	//钢板定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::PLATE)
	{
		PART_PLATE* pSrcJg = (PART_PLATE*)pSrcPart;
		PART_PLATE* pDesJg = (PART_PLATE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
		{	//青岛豪迈不比较单基数
			if (g_xUbomModel.m_uiCustomizeSerial != ID_QingDao_HaoMai)
				hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_SING_NUM), TRUE);
		}
		//加工数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MANU_NUM), TRUE);
		//总重
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_MANU_WEIGHT), TRUE);
		//焊接
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_WELD), TRUE);
		//制弯
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::I_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(g_xBomCfg.GetCfgColName(CBomTblTitleCfg::I_ZHI_WAN), TRUE);
	}
}
//比对BOM信息 0.相同 1.不同 2.文件有误
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS = 0.5;
	m_hashCompareResultByPartNo.Empty();
	if (m_xLoftBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少放样BOM信息!");
		return 2;
	}
	if (m_xOrigBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少工艺科BOM信息");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for (BOMPART *pLoftPart = m_xLoftBom.EnumFirstPart(); pLoftPart; pLoftPart = m_xLoftBom.EnumNextPart())
	{
		BOMPART *pOrgPart = m_xOrigBom.FindPart(pLoftPart->sPartNo);
		if (pOrgPart == NULL)
		{	//1.存在放样构件，不存在原始构件
			COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
			pResult->pOrgPart = NULL;
			pResult->pLoftPart = pLoftPart;
		}
		else
		{	//2. 对比同一件号构件属性
			hashBoolByPropName.Empty();
			CompareData(pLoftPart, pOrgPart, hashBoolByPropName, TRUE);
			if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
			{
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
				pResult->pOrgPart = pOrgPart;
				pResult->pLoftPart = pLoftPart;
				for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
			}
		}
	}
	//2.3 遍历导入的原始表,查找是否有漏件情况
	for (BOMPART *pPart = m_xOrigBom.EnumFirstPart(); pPart; pPart = m_xOrigBom.EnumNextPart())
	{
		if (m_xLoftBom.FindPart(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart = pPart;
		pResult->pLoftPart = NULL;
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("放样BOM和工艺科BOM信息相同!");
		return 0;
	}
	else
		return 1;
}
//进行漏号检测
int CProjectTowerType::CompareLoftAndPartDwgs(BYTE ciTypeJ0_P1_A2 /*= 2*/)
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashPartByPartNo;
	for (CDwgFileInfo* pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetAngleNum() > 0)
		{
			for (CAngleProcessInfo* pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = pDwgFile->EnumNextJg())
				hashPartByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
		if (pDwgFile->GetPlateNum() > 0)
		{
			for (CPlateProcessInfo* pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
				hashPartByPartNo.SetValue(pPlateInfo->xPlate.GetPartNo(), TRUE);
		}
	}
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (ciTypeJ0_P1_A2 == 0 && pPart->cPartType != BOMPART::ANGLE)
			continue;	//角钢漏号检测
		if (ciTypeJ0_P1_A2 == 1 && pPart->cPartType != BOMPART::PLATE)
			continue;	//钢板漏号检测
		if (hashPartByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart = NULL;
		pResult->pLoftPart = pPart;
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("已打开DWG文件没有漏号情况!");
		return 0;
	}
	else
		return 1;
}
//
int CProjectTowerType::CompareLoftAndPartDwg(const char* sFileName)
{
	const double COMPARE_EPS = 0.5;
	if (m_xLoftBom.GetPartNum() <= 0)
	{
		AfxMessageBox("缺少BOM信息!");
		return 2;
	}
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		AfxMessageBox("未找到指定的角钢DWG文件!");
		return 2;
	}
	//进行比对
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	if (pDwgFile->GetAngleNum() > 0)
	{
		for (CAngleProcessInfo* pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo;
			pJgInfo = pDwgFile->EnumNextJg())
		{
			BOMPART *pDwgJg = &pJgInfo->m_xAngle;
			BOMPART *pLoftJg = m_xLoftBom.FindPart(pJgInfo->m_xAngle.sPartNo);
			if (pLoftJg == NULL)
			{	//1、存在DWG构件，不存在放样构件
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pJgInfo->m_xAngle.sPartNo);
				pResult->pOrgPart = pDwgJg;
				pResult->pLoftPart = NULL;
			}
			else
			{	//2、对比同一件号构件属性
				if (g_xUbomModel.m_uiCustomizeSerial == ID_SiChuan_ChengDu)
				{	//中电建成都铁塔特殊要求:备注中含有“坡口”字符且有刨根工艺的角钢，则忽略该角钢的刨根工艺
					if (((PART_ANGLE*)pLoftJg)->bCutRoot && strstr(pLoftJg->sNotes, "坡口"))
						((PART_ANGLE*)pLoftJg)->bCutRoot = FALSE;
				}
				CompareData(pLoftJg, pDwgJg, hashBoolByPropName);
				if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
				{
					COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgJg->sPartNo);
					pResult->pOrgPart = pDwgJg;
					pResult->pLoftPart = pLoftJg;
					for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
						pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
				}
			}
		}
	}
	if (pDwgFile->GetPlateNum() > 0)
	{
		for (CPlateProcessInfo* pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo;
			pPlateInfo = pDwgFile->EnumNextPlate())
		{
			PART_PLATE* pDwgPlate = &(pPlateInfo->xBomPlate);
			BOMPART *pLoftPlate = m_xLoftBom.FindPart(pDwgPlate->sPartNo);
			if (pLoftPlate == NULL)
			{	//1、存在DWG构件，不存在放样构件
				COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
				pResult->pOrgPart = pDwgPlate;
				pResult->pLoftPart = NULL;
			}
			else
			{	//2、对比同一件号构件属性
				CompareData(pLoftPlate, pDwgPlate, hashBoolByPropName);
				if (hashBoolByPropName.GetNodeNum() > 0)//结点数量
				{
					COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
					pResult->pOrgPart = pDwgPlate;
					pResult->pLoftPart = pLoftPlate;
					for (BOOL *pValue = hashBoolByPropName.GetFirst(); pValue; pValue = hashBoolByPropName.GetNext())
						pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(), *pValue);
				}
			}
		}
	}
	if (m_hashCompareResultByPartNo.GetNodeNum() == 0)
	{
		AfxMessageBox("BOM信息和DWG信息相同!");
		return 0;
	}
	else
		return 1;
}
//
void CProjectTowerType::AddCompareResultSheet(LPDISPATCH pSheet, int iSheet, int iCompareType)
{
	//对校审结果进行排序
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->sPartNo);
		else
			keyStrArr.append(pResult->pOrgPart->sPartNo);
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData, keyStrArr.GetSize(), compare_func);
	//生成Excel
	if (iSheet == 1)
	{
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		excel_sheet.SetName("校审结果");
		CStringArray str_arr;
		for (size_t i = 0; i < g_xBomCfg.GetBomTitleCount(); i++)
		{
			if (g_xBomCfg.IsTitleCol(i, CBomTblTitleCfg::I_NOTES))
				continue;	//备注不显示
			str_arr.Add(g_xBomCfg.GetTitleName(i));
		}
		str_arr.Add("数据来源");
		int nCol = str_arr.GetSize();
		ARRAY_LIST<double> col_arr;
		col_arr.SetSize(nCol);
		for (int i = 0; i < nCol; i++)
			col_arr[i] = 10;
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr.m_pData, TRUE);
		//填充内容
		int iRow = 0;
		CVariant2dArray map(GetResultCount() * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart == NULL)
				continue;
			int iCol = 0;
			for (size_t ii = 0; ii < g_xBomCfg.GetBomTitleCount(); ii++)
			{
				if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_NOTES))
					continue;	//备注不显示
				BOOL bDiff = FALSE;
				if (pResult->hashBoolByPropName.GetValue(g_xBomCfg.GetTitleName(ii)))
					bDiff = TRUE;
				if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_PART_NO))
				{	//件号
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sPartNo));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_SPEC))
				{	//规格
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sSpec));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(pResult->pLoftPart->sSpec));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_MATERIAL))
				{	//材质
					map.SetValueAt(iRow, iCol, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_LEN))
				{	//长度
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_SING_NUM))
				{	//单基数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->GetPartNum()));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->GetPartNum()));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_MANU_NUM))
				{	//加工数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->nSumPart));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->nSumPart));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_MANU_WEIGHT))
				{	//加工重量
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->fSumWeight)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->fSumWeight)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_WELD))
				{	//焊接
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->bWeldPart ? COleVariant("*") : COleVariant(""));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_ZHI_WAN))
				{	//制弯
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				}
				else if (pResult->pOrgPart->cPartType == BOMPART::ANGLE && pResult->pLoftPart->cPartType == BOMPART::ANGLE)
				{
					PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
					PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
					if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_CUT_ANGLE))
					{	//切角
						map.SetValueAt(iRow, iCol, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_PUSH_FLAT))
					{	//打扁
						map.SetValueAt(iRow, iCol, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_CUT_BER))
					{	//铲背
						map.SetValueAt(iRow, iCol, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_CUT_ROOT))
					{	//刨根
						map.SetValueAt(iRow, iCol, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_KAI_JIAO))
					{	//开角
						map.SetValueAt(iRow, iCol, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_HE_JIAO))
					{	//合角
						map.SetValueAt(iRow, iCol, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomTblTitleCfg::I_FOOT_NAIL))
					{	//带脚钉
						map.SetValueAt(iRow, iCol, pOrgJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
					}
				}
				if (bDiff)
				{	//数据不同，通过不同颜色进行标记
					CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("%C%d", 'A' + iCol, iRow + 2));
					CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("%C%d", 'A' + iCol, iRow + 3));
				}
				iCol += 1;
			}
			//数据来源
			map.SetValueAt(iRow, iCol, (iCompareType == 1) ? COleVariant(m_xOrigBom.m_sBomName) : COleVariant("DWG"));
			map.SetValueAt(iRow + 1, iCol, (iCompareType == 1) ? COleVariant(m_xLoftBom.m_sBomName) : COleVariant("Xls"));
			iRow += 2;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, iRow + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
	else if (iSheet == 2 || iSheet == 3)
	{
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		CStringArray str_arr;
		str_arr.SetSize(4);
		str_arr[0] = "件号"; str_arr[1] = "规格"; str_arr[2] = "材质"; str_arr[3] = "长度";
		double col_arr[4] = { 15,15,15,15 };
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
		if (iCompareType == COMPARE_BOM_FILE)
		{
			if (iSheet == 2)
				excel_sheet.SetName(CXhChar500("%s表多号", (char*)m_xOrigBom.m_sBomName));
			else
				excel_sheet.SetName(CXhChar100("%s表多号", (char*)m_xLoftBom.m_sBomName));
		}
		else
			excel_sheet.SetName((iSheet == 2) ? "DWG多号" : "DWG缺号");
		//填充内容
		int index = 0, nCol = 4, nResult = GetResultCount();
		CVariant2dArray map(nResult * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL)
				continue;
			BOMPART *pBomPart = NULL;
			if (iSheet == 2 && pResult->pOrgPart && pResult->pLoftPart == NULL)
				pBomPart = pResult->pOrgPart;
			if (iSheet == 3 && pResult->pLoftPart && pResult->pOrgPart == NULL)
				pBomPart = pResult->pLoftPart;
			if (pBomPart == NULL)
				continue;
			map.SetValueAt(index, 0, COleVariant(pBomPart->sPartNo));
			map.SetValueAt(index, 1, COleVariant(pBomPart->sSpec));
			map.SetValueAt(index, 2, COleVariant(CUbomModel::QueryMatMarkIncQuality(pBomPart)));
			map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pBomPart->length)));
			index++;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
}
void CProjectTowerType::AddDwgLackPartSheet(LPDISPATCH pSheet, int iCompareType)
{

	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet, FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("DWG缺少构件");
	//设置标题
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0] = "构件编号"; str_arr[1] = "设计规格"; str_arr[2] = "材质";
	str_arr[3] = "长度"; str_arr[4] = "单基数"; str_arr[5] = "加工数";
	double col_arr[6] = { 15,15,15,15,15,15 };
	CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
	//填充内容
	char cell_start[16] = "A1";
	char cell_end[16] = "A1";
	int nResult = GetResultCount();
	CVariant2dArray map(nResult * 2, 6);//获取Excel表格的范围
	int index = 0;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart)
			continue;
		map.SetValueAt(index, 0, COleVariant(pResult->pLoftPart->sPartNo));
		map.SetValueAt(index, 1, COleVariant(pResult->pLoftPart->sSpec));
		map.SetValueAt(index, 2, COleVariant(CUbomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
		map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pResult->pLoftPart->length)));
		map.SetValueAt(index, 4, COleVariant((long)pResult->pLoftPart->GetPartNum()));
		map.SetValueAt(index, 5, COleVariant((long)pResult->pLoftPart->nSumPart));
		index++;
	}
	_snprintf(cell_end, 15, "F%d", index + 1);
	if (index > 0)
		CExcelOper::SetRangeValue(excel_sheet, "A2", cell_end, map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", cell_end, COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet, "A1", cell_end, COleVariant(10.5));
}
//导出比较结果
void CProjectTowerType::ExportCompareResult(int iCompareType)
{
	int nSheet = 3;
	if (iCompareType == COMPARE_ALL_DWGS)
		nSheet = 1;
	if (iCompareType == COMPARE_DWG_FILE)
		nSheet = 2;
	LPDISPATCH pWorksheets = CExcelOper::CreateExcelWorksheets(nSheet);
	ASSERT(pWorksheets != NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for (int iSheet = 1; iSheet <= nSheet; iSheet++)
	{
		LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)iSheet));
		if (nSheet == 1)
			AddDwgLackPartSheet(pWorksheet, iCompareType);
		else
			AddCompareResultSheet(pWorksheet, iSheet, iCompareType);
	}
	excel_sheets.ReleaseDispatch();
}
//更新ERP料单中件号（材质为Q420,件号前加P，材质为Q345，件号前加H）
BOOL CProjectTowerType::ModifyErpBomPartNo(BYTE ciMatCharPosType)
{
	if (m_xOrigBom.m_sFileName.GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_xOrigBom.m_sFileName))
		return FALSE;
	LPDISPATCH pWorksheets = excelobj.GetWorksheets();
	if (pWorksheets == NULL)
	{
		AfxMessageBox("ERP料单文件打开失败!");
		return FALSE;
	}
	//获取Excel指定Sheet内容存储至sheetContentMap中
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short)1));
	_Worksheet   excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet);
	excel_sheet.Select();
	Range excel_usedRange, excel_range;
	excel_usedRange.AttachDispatch(excel_sheet.GetUsedRange());
	excel_range.AttachDispatch(excel_usedRange.GetRows());
	long nRowNum = excel_range.GetCount();
	//excel_usedRange计算行数时会少一行，原因未知。暂时在此处增加行数 wht 20-04-24
	nRowNum += 10;
	excel_range.AttachDispatch(excel_usedRange.GetColumns());
	long nColNum = excel_range.GetCount();
	CVariant2dArray sheetContentMap(1, 1);
	CXhChar50 cell = CExcelOper::GetCellPos(nColNum, nRowNum);
	LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"), COleVariant(cell));
	excel_range.AttachDispatch(pRange);
	sheetContentMap.var = excel_range.GetValue();
	excel_usedRange.ReleaseDispatch();
	excel_range.ReleaseDispatch();
	//更新指定sheet内容
	int iPartNoCol = 1, iMartCol = 3;
	for (int i = 1; i <= sheetContentMap.RowsCount(); i++)
	{
		VARIANT value;
		CXhChar16 sPartNo, sMaterial, sNewPartNo;
		//件号
		sheetContentMap.GetValueAt(i, iPartNoCol, value);
		if (value.vt == VT_EMPTY)
			continue;
		sPartNo = VariantToString(value);
		//材质
		sheetContentMap.GetValueAt(i, iMartCol, value);
		sMaterial = VariantToString(value);
		//更新件号数据
		if (strstr(sMaterial, "Q345") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q420") && strstr(sPartNo, "P") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("P%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sP", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
	}
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
	return TRUE;
}

BOOL CProjectTowerType::ModifyTmaBomPartNo(BYTE ciMatCharPosType)
{
	ARRAY_LIST<BOMPART*> partPtrList;
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
		partPtrList.append(pPart);
	char cMaterial;
	CXhChar16 sPartNo, sMaterial, sNewPartNo;
	for (int i = 0; i < partPtrList.GetSize(); i++)
	{
		sPartNo = partPtrList[i]->sPartNo;
		cMaterial = partPtrList[i]->cMaterial;
		sMaterial = CProcessPart::QuerySteelMatMark(cMaterial);
		//更新件号数据
		if (strstr(sMaterial, "Q345") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q420") && strstr(sPartNo, "P") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("P%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sP", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
	}
	return TRUE;
}
CPlateProcessInfo *CProjectTowerType::FindPlateInfoByPartNo(const char* sPartNo)
{
	CPlateProcessInfo *pPlateInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetPlateNum() <= 0)
			continue;
		pPlateInfo = pDwgFile->FindPlateByPartNo(sPartNo);
		if (pPlateInfo)
			break;
	}
	return pPlateInfo;
}
CAngleProcessInfo *CProjectTowerType::FindAngleInfoByPartNo(const char* sPartNo)
{
	CAngleProcessInfo *pAngleInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->GetAngleNum() <= 0)
			continue;
		pAngleInfo = pDwgFile->FindAngleByPartNo(sPartNo);
		if (pAngleInfo)
			break;
	}
	return pAngleInfo;
}
//////////////////////////////////////////////////////////////////////////
//CBomModel
CUbomModel::CUbomModel(void)
{
	InitBomModel();
	//初始化客户信息
	m_xMapClientInfo["安徽宏源"] = ID_AnHui_HongYuan;
	m_xMapClientInfo["安徽汀阳"] = ID_AnHui_TingYang;
	m_xMapClientInfo["中电建成都铁塔"] = ID_SiChuan_ChengDu;
	m_xMapClientInfo["江苏华电"] = ID_JiangSu_HuaDian;
	m_xMapClientInfo["成都东方"] = ID_ChengDu_DongFang;
	m_xMapClientInfo["青岛豪迈"] = ID_QingDao_HaoMai;
	m_xMapClientInfo["青岛强力钢结构"] = ID_QingDao_QLGJG;
	m_xMapClientInfo["青岛载力"] = ID_QingDao_ZAILI;
	m_xMapClientInfo["五洲鼎益"] = ID_WUZHOU_DINGYI;
	m_xMapClientInfo["山东华安"] = ID_SHANDONG_HAUAN;
	m_xMapClientInfo["青岛鼎兴"] = ID_QingDao_DingXing;
	m_xMapClientInfo["青岛百斯特"] = ID_QingDao_BaiSiTe;
	m_xMapClientInfo["青岛汇金通"] = ID_QingDao_HuiJinTong;
	m_xMapClientInfo["洛阳龙羽"] = ID_LuoYang_LongYu;
	m_xMapClientInfo["江苏电装"] = ID_JiangSu_DianZhuang;
	m_xMapClientInfo["河南永光"] = ID_HeNan_YongGuang;
	m_xMapClientInfo["广东安恒"] = ID_GuangDong_AnHeng;
	m_xMapClientInfo["广东禅涛"] = ID_GuangDong_ChanTao;
	m_xMapClientInfo["重庆江电"] = ID_ChongQing_JiangDian;
	m_xMapClientInfo["青岛武晓"] = ID_QingDao_WuXiao;
	//初始化成都铁塔ERP料单中的指定单元
	m_xMapPrjCell["工程名称"] = "D1";
	m_xMapPrjCell["工程塔型"] = "F1";
	m_xMapPrjCell["材料标准"] = "H1";
	m_xMapPrjCell["下达单号"] = "L1";
	m_xMapPrjCell["塔规格"] = "J1";
	m_xMapPrjCell["合同号"] = "B1";
	m_xMapPrjCell["基数"] = "N1";

}
CUbomModel::~CUbomModel(void)
{

}
void CUbomModel::InitBomModel()
{
	m_dwFunctionFlag = 0;
	m_uiCustomizeSerial = 0;
	m_bExeRppWhenArxLoad = TRUE;
	m_bExtractPltesWhenOpenFile = TRUE;
	m_bExtractAnglesWhenOpenFile = TRUE;
	m_fMaxLenErr = 0.5;
	m_bCmpQualityLevel = TRUE;
	m_bEqualH_h = FALSE;
	m_sJgCadName.Empty();
	m_sJgCadPartLabel.Empty();
	m_sJgCardBlockName.Empty();
	m_sNotPrintFilter.Empty();
	m_ciPrintSortType = 0;
	m_uiJgCadPartLabelMat = 0;
	//
	m_xPrjTowerTypeList.Empty();
	CProjectTowerType* pProject = m_xPrjTowerTypeList.Add(0);
	pProject->m_sProjName.Copy("新建工程");
}
bool CUbomModel::IsValidFunc(int iFuncType)
{
	if (iFuncType > 0)
		return (m_dwFunctionFlag&GetSingleWord(iFuncType)) > 0;
	else
		return false;
}
CXhChar16 CUbomModel::QueryMatMarkIncQuality(BOMPART *pPart)
{
	CXhChar16 sMatMark;
	if (pPart)
	{
		sMatMark = CProcessPart::QuerySteelMatMark(pPart->cMaterial);
		if (pPart->cQualityLevel != 0 && !sMatMark.EqualNoCase(pPart->sMaterial))
			sMatMark.Append(pPart->cQualityLevel);
	}
	return sMatMark;
}
BOOL CUbomModel::IsNeedPrint(BOMPART *pPart, const char* sNotes)
{
	if (m_sNotPrintFilter.GetLength() <= 0)
		return TRUE;
	CXhChar500 sFilter(m_sNotPrintFilter);
	sFilter.Replace("&&", ",");
	CExpression expression;
	EXPRESSION_VAR *pVar = expression.varList.Append();
	strcpy(pVar->variableStr, "WIDTH");
	pVar->fValue = pPart->wide;
	pVar = expression.varList.Append();
	strcpy(pVar->variableStr, "NOTES");
	if (sNotes != NULL)
		pVar->fValue = strlen(sNotes);
	else
		pVar->fValue = strlen(pPart->sNotes);

	bool bRetCode = true;
	//1.将过滤条件拆分为多个逻辑表达式，目前只支持&&
	for (char* sKey = strtok(sFilter, ","); sKey; sKey = strtok(NULL, ","))
	{	//2.逐个解析表达式
		if (!expression.SolveLogicalExpression(sKey))
		{
			bRetCode = false;
			break;
		}
	}
	return !bRetCode;
}
BOOL CUbomModel::IsJgCardBlockName(const char* sBlockName)
{
	if (sBlockName != NULL && strcmp(sBlockName, "JgCard") == 0)
		return TRUE;
	else if (m_sJgCardBlockName.GetLength() > 0 && m_sJgCardBlockName.EqualNoCase(sBlockName))
		return TRUE;
	else
		return false;
}
BOOL CUbomModel::IsPartLabelTitle(const char* sText)
{
	if (m_sJgCadPartLabel.GetLength() > 0)
	{
		CString ss1(sText), ss2(m_sJgCadPartLabel);
		ss1 = ss1.Trim();
		ss2 = ss2.Trim();
		ss1.Remove(' ');
		ss2.Remove(' ');
		if (ss2.GetLength() > 1 && ss1.CompareNoCase(ss2) == 0)
			return true;
		else
			return false;
	}
	else
	{
		if (!(strstr(sText, "件") && strstr(sText, "号")) &&	//件号、零件编号
			!(strstr(sText, "编") && strstr(sText, "号")))		//编号、构件编号
			return false;
		if (strstr(sText, "文件") != NULL)
			return false;	//排除"文件编号"导致的提取错误 wht 19-05-13
		if (strstr(sText, ":") != NULL || strstr(sText, "：") != NULL)
			return false;	//排除钢板标注中的件号，避免将钢板错误提取为角钢 wht 20-07-29
		return true;
	}
}
#endif
