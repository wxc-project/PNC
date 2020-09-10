#include "StdAfx.h"
#include "BatchPrint.h"
#include "acaplmgr.h"
#include "XhCharString.h"
#include "CadToolFunc.h"
#include "dbplotsetval.h"
#include "CadObjLife.h"

#ifdef __UBOM_ONLY_
static CXhChar500 GetTempFileFolderPath(bool bEmpty,const char* subFolder)
{	//获取临时文件路径
	//CXhChar500 sSysPath;
	//GetSysPath(sSysPath);
	//CXhChar500 sTemp("%s\\PngTemp\\", (char*)sSysPath);
	CString sWorkPath;
	if(subFolder)
		GetCurWorkPath(sWorkPath, TRUE, subFolder, TRUE);
	else
		GetCurWorkPath(sWorkPath, TRUE, "PNG", TRUE);
	CXhChar500 sTemp(sWorkPath);
	//确保路径存在
	CFileFind fileFind;
	BOOL bContinue = fileFind.FindFile(CXhChar500("%s*.*", (char*)sTemp));
	if (!bContinue)
		MakeDirectory(sTemp);
	else
	{
		while (bContinue)
		{
			bContinue = fileFind.FindNextFile();

			CString ss1 = fileFind.GetFileName();
			CString ss2 = fileFind.GetFilePath();
			if (fileFind.IsDots() || fileFind.IsDirectory())
				continue;
			DeleteFile(fileFind.GetFilePath());
		}
	}
	//删除临时文件夹中的所有内容
	return sTemp;
}

//////////////////////////////////////////////////////////////////////////
//
const char *PLOT_CFG::TOWER_TYPE		= "塔型";
const char *PLOT_CFG::PART_LABEL		= "件号";
const char *PLOT_CFG::MAT_MARK			= "材质";
const char *PLOT_CFG::MAT_BRIEF_MARK	= "材质简化字符";
const char *PLOT_CFG::PRE_TOWER_COUNT	= "单基数";
const char *PLOT_CFG::MANU_COUNT		= "加工数";
CXhChar500 PLOT_CFG::GetPngFileNameByTemplate(const char* sTowerType, const char* sPartLabel,
											  const char* sMaterial, char cMatBriefMark,
											  int nNumPreTower, int nManuNum)
{
	CXhChar500 sFileName(m_sFileNameTemplate);
	if (m_sFileNameTemplate.GetLength() > 0)
	{
		sFileName.Replace(TOWER_TYPE, sTowerType);
		sFileName.Replace(MAT_BRIEF_MARK, CXhChar16("%C", cMatBriefMark));
		sFileName.Replace(MAT_MARK, sMaterial);
		sFileName.Replace(PART_LABEL, sPartLabel);
		sFileName.Replace(PRE_TOWER_COUNT, CXhChar16("%d", nNumPreTower));
		sFileName.Replace(MANU_COUNT, CXhChar16("%d", nManuNum));
	}
	else
		sFileName.Copy(sPartLabel);
	return sFileName;
}

//////////////////////////////////////////////////////////////////////////
// PRINT_SCOPE
PRINT_SCOPE::PRINT_SCOPE()
{
	m_cMatBriefMark = 0;
	m_pPlate = NULL;
	m_pAngle = NULL;
	m_nManuNum = m_nNumPreTower = 0;
	m_cMatBriefMark = 0;
}
PRINT_SCOPE::PRINT_SCOPE(SCOPE_STRU scope, const char* sLabel, char cMatBriefMark,
						 const char* sTowerType, const char* sMat, int numPreTower, int manuNum)
{
	m_cMatBriefMark = 0;
	m_pPlate = NULL;
	m_pAngle = NULL;
	m_nManuNum = m_nNumPreTower = 0;
	m_cMatBriefMark = 0;
	Init(scope, sLabel, cMatBriefMark, sTowerType, sMat, numPreTower, manuNum);
}

PRINT_SCOPE::PRINT_SCOPE(f2dRect rect, const char* sLabel, char cMatBriefMark,
						 const char* sTowerType, const char* sMat, int numPreTower, int manuNum)
{
	m_cMatBriefMark = 0;
	m_pPlate = NULL;
	m_pAngle = NULL;
	m_nManuNum = m_nNumPreTower = 0;
	m_cMatBriefMark = 0;
	Init(rect, sLabel, cMatBriefMark, sTowerType, sMat, numPreTower, manuNum);
}

void PRINT_SCOPE::Init(SCOPE_STRU scope, const char* sLabel, char cMatBriefMark,
					   const char* sTowerType, const char* sMat, int numPreTower, int manuNum)
{
	L_T[X] = scope.fMinX;
	L_T[Y] = scope.fMaxY;
	R_B[X] = scope.fMaxX;
	R_B[Y] = scope.fMinY;
	if (sLabel)
		m_sPartLabel.Copy(sLabel);
	m_cMatBriefMark = cMatBriefMark;
	if (sTowerType)
		m_sTowerType.Copy(sTowerType);
	if (sMat)
		m_sMaterial.Copy(sMat);
	m_nNumPreTower = numPreTower;
	m_nManuNum = manuNum;
}

void PRINT_SCOPE::Init(f2dRect rect, const char* sLabel, char cMatBriefMark,
					   const char* sTowerType, const char* sMat, int numPreTower, int manuNum)
{
	L_T[X] = rect.topLeft.x;
	L_T[Y] = rect.topLeft.y;
	R_B[X] = rect.bottomRight.x;
	R_B[Y] = rect.bottomRight.y;
	if (sLabel)
		m_sPartLabel.Copy(sLabel);
	m_cMatBriefMark = cMatBriefMark;
	if (sTowerType)
		m_sTowerType.Copy(sTowerType);
	if (sMat)
		m_sMaterial.Copy(sMat);
	m_nNumPreTower = numPreTower;
	m_nManuNum = manuNum;
}

void PRINT_SCOPE::Init(CAngleProcessInfo *pAngle)
{
	m_pAngle = pAngle;
	if (pAngle == NULL)
		return;
	SCOPE_STRU scope = m_pAngle->GetCADEntScope();
	scope.fMaxX += MARGIN;
	scope.fMaxY += MARGIN;
	scope.fMinX -= MARGIN;
	scope.fMinY -= MARGIN;
	//
	L_T[X] = scope.fMinX;
	L_T[Y] = scope.fMaxY;
	R_B[X] = scope.fMaxX;
	R_B[Y] = scope.fMinY;
	m_sPartLabel.Copy(m_pAngle->m_xAngle.sPartNo);
	m_cMatBriefMark = m_pAngle->m_xAngle.cMaterial;
	m_sTowerType.Copy(m_pAngle->m_sTowerType);
	m_sMaterial.Copy(m_pAngle->m_xAngle.sMaterial);
	m_nNumPreTower = m_pAngle->m_xAngle.GetPartNum();
	m_nManuNum = m_pAngle->m_xAngle.feature1;
}
void PRINT_SCOPE::Init(CPlateProcessInfo *pPlate)
{
	m_pPlate = pPlate;
	if (m_pPlate == NULL)
		return;
	SCOPE_STRU scope = m_pPlate->GetCADEntScope();
	scope.fMaxX += MARGIN;
	scope.fMaxY += MARGIN;
	scope.fMinX -= MARGIN;
	scope.fMinY -= MARGIN;
	//
	L_T[X] = scope.fMinX;
	L_T[Y] = scope.fMaxY;
	R_B[X] = scope.fMaxX;
	R_B[Y] = scope.fMinY;
	m_sPartLabel.Copy(m_pPlate->xBomPlate.sPartNo);
	m_cMatBriefMark = m_pPlate->xBomPlate.cMaterial;
	m_sTowerType.Copy(m_pPlate->m_xBaseInfo.m_sTaType);
	m_sMaterial.Copy(m_pPlate->xBomPlate.sMaterial);
	m_nNumPreTower = m_pPlate->xBomPlate.GetPartNum();
	m_nManuNum = m_pPlate->xBomPlate.feature1;
}

CXhChar500 PRINT_SCOPE::GetPartFileName(PLOT_CFG* pPlotCfg, const char* extension)
{
	m_sPartLabel.Remove(' ');
	CXhChar500 sFileName;
	if (pPlotCfg)
	{
		sFileName = pPlotCfg->GetPngFileNameByTemplate(m_sTowerType, m_sPartLabel, m_sMaterial, m_cMatBriefMark, m_nNumPreTower, m_nManuNum);
		//if (extension != NULL)
		//	sFileName.Append(extension);
	}
	return sFileName;
	/*
	CXhChar100 sPngFileName;
	if (m_cMatBriefMark != 0 && ciMatType == INC_MAT_PREFIX)
		sPngFileName.Printf("%c%s.png", m_cMatBriefMark, (char*)m_sPartLabel);
	else if (m_cMatBriefMark != 0 && ciMatType == INC_MAT_PREFIX)
		sPngFileName.Printf("%s%c.png", (char*)m_sPartLabel, m_cMatBriefMark);
	else
		sPngFileName.Printf("%s.png", (char*)m_sPartLabel);
	return sPngFileName;
	*/
}
SCOPE_STRU PRINT_SCOPE::GetCadEntScope()
{
	SCOPE_STRU scope;
	scope.fMinX = L_T[X];
	scope.fMaxX = R_B[X];
	scope.fMinY = R_B[Y];
	scope.fMaxY = L_T[Y];
	scope.fMinZ = scope.fMaxZ = 0;
	return scope;
}

//////////////////////////////////////////////////////////////////////////
// CBatchPrint
PLOT_CFG CBatchPrint::m_xPdfPlotCfg;
PLOT_CFG CBatchPrint::m_xPngPlotCfg;
PLOT_CFG CBatchPrint::m_xPaperPlotCfg;
bool CBatchPrint::PrintProcessCardToPNG(bool bEmptyPngFiles, bool bSendCmd)
{	//判断打印机是否合理
	CPrintSet::SetPlotMedia(&m_xPngPlotCfg, TRUE);
	CXhChar500 sPlotCfgName = CPrintSet::GetPlotCfgName(TRUE);
	if (sPlotCfgName.GetLength() <= 0)
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//进行批量打印
	CXhChar500 sTempPath= GetTempFileFolderPath(bEmptyPngFiles, "PNG"), sFilePath;
	SetCurrentDirectory(sTempPath);	//设置当前工作路径
	CXhChar50 sDevice("\n");	// "PublishToWeb PNG.pc3\n");
	CXhChar50 sPaperSize("\n");	//"Sun Hi-Res (1600.00 x 1280.00 像素)\n");
	if (m_xPngPlotCfg.m_sDeviceName.GetLength() > 0 &&
		!m_xPngPlotCfg.m_sDeviceName.EqualNoCase("无"))
	{
		if (m_xPngPlotCfg.m_sDeviceName.GetLength() > 0)
		{
			sDevice.Copy(m_xPngPlotCfg.m_sDeviceName);
			sDevice.Append('\n');
		}
		if (m_xPngPlotCfg.m_sPaperSize.GetLength() > 0)
		{
			sPaperSize.Copy(m_xPngPlotCfg.m_sPaperSize);
			sPaperSize.Append('\n');
		}
	}
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPngPlotCfg;
	CXhChar16 sExtension(".png");
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		ZoomAcadView(pScope->GetCadEntScope(), 10);
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();

		CString sLabel = pScope->GetPartFileName(pPlotCfg, sExtension);
		sFilePath.Printf("%s%s", (char*)sTempPath, sLabel);
		int nRetCode = 0;
		if (bSendCmd)
		{
			CXhChar50 sLeftTop("%.f,%.f\n", pScope->L_T[X], pScope->L_T[Y]);
			CXhChar50 sRightBottom("%.f,%.f\n", pScope->R_B[X], pScope->R_B[Y]);
			CXhChar500 sCmd("-plot\ny\n\n%s%sL\nN\nw\n%s%s\nC\nY\nmonochrome.ctb\nY\nA\n%s\nN\nY\n",
				(char*)sDevice, (char*)sPaperSize, (char*)sLeftTop, (char*)sRightBottom, (char*)sFilePath);
#ifdef _ARX_2007
			SendCommandToCad(L"CMDECHO 0\n");
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad("CMDECHO 0\n");
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			acedCommand(RTSTR, L"CMDECHO", RTLONG, 0, RTNONE);	//设置命令行是否产生回显
			nRetCode = acedCommand(RTSTR, L"-plot", RTSTR, L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, L"",								//布局名
				RTSTR, (ACHAR*)_bstr_t(sDevice),		//输出设备的名称
				RTSTR, (ACHAR*)_bstr_t(sPaperSize),		//图纸尺寸:<A4>
				RTSTR, L"L",							//图形方向:<横向|纵向>
				RTSTR, L"N",							//是否反向打印
				RTSTR, L"w",							//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,					//左上角
				RTPOINT, pScope->R_B,					//右下角
				RTSTR, L"",								//打印比例:<布满>
				RTSTR, L"C",							//打印偏移：<居中>
				RTSTR, L"Y",							//是否按样式打印
				RTSTR, L".",							//打印样式名称
				RTSTR, L"Y",							//打印线宽
				RTSTR, L"A",							//着色打印设置
				RTSTR, (ACHAR*)_bstr_t(sFilePath),	//打印到文件	//此处设置完成路径时会失败，需要调用SetCurrentDirectory之后，此处直接输入文件名 wht 17-10-23
				RTSTR, L"N",							//是否保存对页面设置的修改
				RTSTR, L"Y",							//是否继续打印
				RTNONE);
#else
			acedCommand(RTSTR, "CMDECHO", RTLONG, 0, RTNONE);		//设置命令行是否产生回显
			nRetCode = acedCommand(RTSTR, L"-plot", RTSTR, L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, "",							//布局名
				RTSTR, sDevice,						//输出设备的名称
				RTSTR, sPaperSize,					//图纸尺寸:<A4>
				RTSTR, "L",							//图形方向:<横向|纵向>
				RTSTR, "N",							//是否反向打印
				RTSTR, "w",							//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,				//左上角
				RTPOINT, pScope->R_B,				//右下角
				RTSTR, "",							//打印比例:<布满>
				RTSTR, "C",							//打印偏移：<居中>
				RTSTR, "Y",							//是否按样式打印
				RTSTR, "monochrome.ctb",			//打印样式名称
				RTSTR, "Y",							//打印线宽
				RTSTR, "A",							//着色打印设置
				RTSTR, sFilePath,					//打印到文件
				RTSTR, "N",							//是否保存对页面设置的修改
				RTSTR, "Y",							//是否继续打印
				RTNONE);
#endif
		}
	}
	//打开图片文件夹
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPDF(bool bEmptyPdfFiles, bool bSendCmd)
{	//判断打印机是否合理
	CPrintSet::SetPlotMedia(&m_xPdfPlotCfg, TRUE);
	CXhChar500 sPlotCfgName = CPrintSet::GetPlotCfgName(TRUE);
	if (sPlotCfgName.GetLength() <= 0)
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//进行批量打印
	CXhChar500 sTempPath = GetTempFileFolderPath(bEmptyPdfFiles, "PDF"), sFilePath;
	SetCurrentDirectory(sTempPath);	//设置当前工作路径
	CXhChar50 sDevice("\n");		// "Microsoft Print To PDF\n");
	CXhChar50 sPaperSize("\n");		//"A4"
	if (m_xPdfPlotCfg.m_sDeviceName.GetLength() > 0 &&
		!m_xPdfPlotCfg.m_sDeviceName.EqualNoCase("无"))
	{
		if (m_xPdfPlotCfg.m_sDeviceName.GetLength() > 0)
		{
			sDevice.Copy(m_xPdfPlotCfg.m_sDeviceName);
			sDevice.Append('\n');
		}
		if (m_xPdfPlotCfg.m_sPaperSize.GetLength() > 0)
		{
			sPaperSize.Copy(m_xPdfPlotCfg.m_sPaperSize);
			sPaperSize.Append('\n');
		}
	}
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPdfPlotCfg;
	CXhChar16 sExtension(".pdf");
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		ZoomAcadView(pScope->GetCadEntScope(), 10);
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();

		CString sLabel = pScope->GetPartFileName(pPlotCfg,sExtension);
		sFilePath.Printf("%s%s", (char*)sTempPath,sLabel);
		int nRetCode = 0;
		if (bSendCmd)
		{
			CXhChar50 sLeftTop("%.f,%.f\n", pScope->L_T[X], pScope->L_T[Y]);
			CXhChar50 sRightBottom("%.f,%.f\n", pScope->R_B[X], pScope->R_B[Y]);
			CXhChar500 sCmd("-plot\ny\n\n%s%s\nL\nN\nw\n%s%s\nC\nY\nmonochrome.ctb\nY\nA\n%s\nN\nY\n",
				(char*)sDevice, (char*)sPaperSize, (char*)sLeftTop, (char*)sRightBottom, (char*)sFilePath);
#ifdef _ARX_2007
			SendCommandToCad(L"CMDECHO 0\n");
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad("CMDECHO 0\n");
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			acedCommand(RTSTR, L"CMDECHO", RTLONG, 0, RTNONE);	//设置命令行是否产生回显
			nRetCode = acedCommand(RTSTR, L"-plot", RTSTR, L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, L"",								//布局名
				RTSTR, (ACHAR*)_bstr_t(sDevice),		//输出设备的名称
				RTSTR, (ACHAR*)_bstr_t(sPaperSize),		//图纸尺寸:<A4>
				RTSTR, L"",								//输入图纸单位[英寸(I) / 毫米(M] <英寸>:	//PDF独有
				RTSTR, L"L",							//图形方向:<横向|纵向>
				RTSTR, L"N",							//是否反向打印
				RTSTR, L"w",							//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,					//左上角
				RTPOINT, pScope->R_B,					//右下角
				RTSTR, L"",								//打印比例:<布满>
				RTSTR, L"C",							//打印偏移：<居中>
				RTSTR, L"Y",							//是否按样式打印
				RTSTR, L".",							//打印样式名称
				RTSTR, L"Y",							//打印线宽
				RTSTR, L"A",							//着色打印设置
				RTSTR, (ACHAR*)_bstr_t(sFilePath),	//打印到文件	//此处设置完成路径时会失败，需要调用SetCurrentDirectory之后，此处直接输入文件名 wht 17-10-23
				RTSTR, L"N",							//是否保存对页面设置的修改
				RTSTR, L"Y",							//是否继续打印
				RTNONE);
#else
			acedCommand(RTSTR, "CMDECHO", RTLONG, 0, RTNONE);		//设置命令行是否产生回显
			nRetCode = acedCommand(RTSTR, L"-plot", RTSTR, L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, "",							//布局名
				RTSTR, sDevice,						//输出设备的名称
				RTSTR, sPaperSize,					//图纸尺寸:<A4>
				RTSTR, "",							//输入图纸单位[英寸(I) / 毫米(M] <英寸>:	//PDF独有
				RTSTR, "L",							//图形方向:<横向|纵向>
				RTSTR, "N",							//是否反向打印
				RTSTR, "w",							//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,				//左上角
				RTPOINT, pScope->R_B,				//右下角
				RTSTR, "",							//打印比例:<布满>
				RTSTR, "C",							//打印偏移：<居中>
				RTSTR, "Y",							//是否按样式打印
				RTSTR, "monochrome.ctb",			//打印样式名称
				RTSTR, "Y",							//打印线宽
				RTSTR, "A",							//着色打印设置
				RTSTR, sFilePath,					//打印到文件
				RTSTR, "N",							//是否保存对页面设置的修改
				RTSTR, "Y",							//是否继续打印
				RTNONE);
#endif
		}
	}
	//打开图片文件夹
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPaper(bool bSendCmd)
{	//判断打印机是否合理
	CXhChar500 sPlotCfgName = CPrintSet::GetPlotCfgName(TRUE);
	if (sPlotCfgName.GetLength() <= 0)
		return false;
	CXhChar50 sDevice("\n");
	CXhChar50 sPaperSize("\n");
	if (m_xPaperPlotCfg.m_sDeviceName.GetLength() > 0)
		sDevice.Printf("%s\n", (char*)m_xPaperPlotCfg.m_sDeviceName);
	if (m_xPaperPlotCfg.m_sPaperSize.GetLength() > 0)
		sPaperSize.Printf("%s\n", (char*)m_xPaperPlotCfg.m_sPaperSize);
	//进行批量打印
	if (m_pPrintScopeList==NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	for(PRINT_SCOPE *pScope= m_pPrintScopeList->GetFirst();pScope;pScope= m_pPrintScopeList->GetNext())
	{
		ZoomAcadView(pScope->GetCadEntScope(),10);
		//更新界面
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
		if (bSendCmd)
		{
			CXhChar50 sLeftTop("%.f,%.f\n", pScope->L_T[X], pScope->L_T[Y]);
			CXhChar50 sRightBottom("%.f,%.f\n", pScope->R_B[X], pScope->R_B[Y]);
			CXhChar500 sCmd("-plot\ny\n\n%s%s\nL\nN\nw\n%s%s\nC\nY\nmonochrome.ctb\nY\nA\n\nN\nY\n",
				(char*)sDevice, (char*)sPaperSize, (char*)sLeftTop, (char*)sRightBottom);
#ifdef _ARX_2007
			SendCommandToCad(L"CMDECHO 0\n");
			SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
			SendCommandToCad("CMDECHO 0\n");
			SendCommandToCad(sCmd);
#endif
		}
		else
		{
#ifdef _ARX_2007
			acedCommand(RTSTR, L"CMDECHO", RTLONG, 0, RTNONE);		//设置命令行是否产生回显
			acedCommand(RTSTR, L"-plot", RTSTR, L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, L"",							//布局名
				RTSTR, sDevice,						//输出设备的名称
				RTSTR, sPaperSize,					//图纸尺寸:<A4>
				RTSTR, L"",							//图纸单位:<毫米>
				RTSTR, L"L",						//图形方向:<横向|纵向>
				RTSTR, L"N",							//是否反向打印
				RTSTR, L"w",						//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,				//左上角
				RTPOINT, pScope->R_B,				//右下角
				RTSTR, L"",							//打印比例:<布满>
				RTSTR, L"C",						//打印偏移：<居中>
				RTSTR, L"Y",						//是否按样式打印
				RTSTR, L"monochrome.ctb",			//打印样式名称
				RTSTR, L"Y",						//打印线宽
				RTSTR, L"A",						//着色打印设置
				RTSTR, L"",							//打印到文件
				RTSTR, L"N",						//是否保存页面设置更改
				RTSTR, L"Y",						//是否继续打印
				RTNONE);
#else
			acedCommand(RTSTR, "CMDECHO", RTLONG, 0, RTNONE);		//设置命令行是否产生回显
			acedCommand(RTSTR, "-plot", RTSTR, "y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR, "",							//布局名
				RTSTR, sDevice,						//输出设备的名称
				RTSTR, sPaperSize,					//图纸尺寸:<A4>
				RTSTR, "",							//图纸单位:<毫米>
				RTSTR, "L",							//图形方向:<横向|纵向>
				RTSTR, "N",							//是否反向打印
				RTSTR, "w",							//打印区域:<指定窗口>
				RTPOINT, pScope->L_T,				//左上角
				RTPOINT, pScope->R_B,				//右下角
				RTSTR, "",							//打印比例:<布满>
				RTSTR, "C",							//打印偏移：<居中>
				RTSTR, "Y",							//是否按样式打印
				RTSTR, "monochrome.ctb",			//打印样式名称
				RTSTR, "Y",							//打印线宽
				RTSTR, "A",							//着色打印设置
				RTSTR, "",							//打印到文件
				RTSTR, "N",							//是否保存页面设置更改
				RTSTR, "Y",							//是否继续打印
				RTNONE);
#endif
		}
	}
	return true;
}

CBatchPrint::CBatchPrint(ATOM_LIST<PRINT_SCOPE> *pPrintScopeList, bool bSendCmd, BYTE ciPrintType, bool bEmptyPngFiles /*= FALSE*/)
{
	m_pPrintScopeList=pPrintScopeList;
	m_ciPrintType = ciPrintType;
	m_bEmptyFiles = bEmptyPngFiles;
	m_bSendCmd = bSendCmd;
	//设置打印机初始值
	if (m_ciPrintType == PRINT_TYPE_PNG)
	{	//
		if (m_xPngPlotCfg.m_sDeviceName.GetLength() <= 0)
			m_xPngPlotCfg.m_sDeviceName.Copy("PublishToWeb PNG.pc3");
	}
	else if (m_ciPrintType == PRINT_TYPE_PDF)
	{
		if (m_xPdfPlotCfg.m_sDeviceName.GetLength() <= 0)
			m_xPdfPlotCfg.m_sDeviceName.Copy("DWG To PDF.pc3");
	}
	else if (m_ciPrintType == PRINT_TYPE_PAPER)
	{
		if (m_xPaperPlotCfg.m_sPaperSize.GetLength() <= 0)
			m_xPaperPlotCfg.m_sPaperSize.Copy("A4");
	}
}

bool CBatchPrint::Print()
{
	if (m_ciPrintType == PRINT_TYPE_PNG)
		PrintProcessCardToPNG(m_bEmptyFiles, m_bSendCmd);
	else if (m_ciPrintType == PRINT_TYPE_PAPER)
		PrintProcessCardToPaper(m_bSendCmd);
	else if (m_ciPrintType == PRINT_TYPE_PDF)
		PrintProcessCardToPDF(m_bEmptyFiles, m_bSendCmd);
	return true;
}

/*
//批量打印
void batPlot()
{	// 取得当前layout
	AcDbLayoutManager* pLayoutManager = acdbHostApplicationServices()->layoutManager(); //取得布局管理器对象
	AcDbLayout* pLayout = pLayoutManager->findLayoutNamed(pLayoutManager->findActiveLayout(TRUE), TRUE);//获得当前布局
	AcDbObjectId  m_layoutId = pLayout->objectId();//获得布局的Id
	//获得打印机验证器对象
	AcDbPlotSettingsValidator* pPSV = acdbHostApplicationServices()->plotSettingsValidator();
	pPSV->refreshLists(pLayout);
	//打印机设置
	ACHAR* m_strDevice = _T("DWF6 ePlot.pc3");//打印机名字
	pPSV->setPlotCfgName(pLayout, m_strDevice);//设置打印设备
	ACHAR* m_mediaName = _T("ISO A4");//图纸名称
	pPSV->setCanonicalMediaName(pLayout, m_mediaName);//设置图纸尺寸
	pPSV->setPlotType(pLayout, AcDbPlotSettings::kWindow);//设置打印范围为窗口
	pPSV->setPlotWindowArea(pLayout, 100, 100, 200, 200);//设置打印范围,超出给范围的将打不出来
	pPSV->setCurrentStyleSheet(pLayout, _T("JSTRI.ctb"));//设置打印样式表
	pPSV->setPlotCentered(pLayout, true);//是否居中打印
	pPSV->setUseStandardScale(pLayout, true);//设置是否采用标准比例
	pPSV->setStdScaleType(pLayout, AcDbPlotSettings::kScaleToFit);//布满图纸
	pPSV->setPlotRotation(pLayout, AcDbPlotSettings::k90degrees);//设置打印方向
	//pPSV->setPlotViewName(pLayout,_T("打印1"));
	//准备打印/////////////////////////////////////////////////////////////////////////
	AcPlPlotEngine* pEngine = NULL;
	if (AcPlPlotFactory::createPublishEngine(pEngine) != Acad::eOk)
	{
		acedAlert(_T("打印失败!"));
		return;
	}
	// 打印进度对话框
	AcPlPlotProgressDialog* pPlotProgDlg = acplCreatePlotProgressDialog(acedGetAcadFrame()->m_hWnd, false, 1);
	pPlotProgDlg->setPlotMsgString(AcPlPlotProgressDialog::kDialogTitle, _T("lot API Progress"));
	pPlotProgDlg->setPlotMsgString(AcPlPlotProgressDialog::kCancelJobBtnMsg, _T("Cancel Job"));
	pPlotProgDlg->setPlotMsgString(AcPlPlotProgressDialog::kCancelSheetBtnMsg, _T("Cancel Sheet"));
	pPlotProgDlg->setPlotMsgString(AcPlPlotProgressDialog::kSheetSetProgressCaption, _T("Job Progress"));
	pPlotProgDlg->setPlotMsgString(AcPlPlotProgressDialog::kSheetProgressCaption, _T("Sheet Progress"));
	pPlotProgDlg->setPlotProgressRange(0, 100);
	pPlotProgDlg->onBeginPlot();
	pPlotProgDlg->setIsVisible(true);
	//begin plot
	Acad::ErrorStatus es = pEngine->beginPlot(pPlotProgDlg);
	AcPlPlotPageInfo pageInfo;//打印页信息
	AcPlPlotInfo plotInfo; //打印信息
	// 设置布局
	plotInfo.setLayout(m_layoutId);
	// 重置参数
	plotInfo.setOverrideSettings(pLayout);
	AcPlPlotInfoValidator validator;//创建打印信息验证器
	validator.setMediaMatchingPolicy(AcPlPlotInfoValidator::kMatchEnabled);
	es = validator.validate(plotInfo);
	// begin document
	const TCHAR* szDocName = acDocManager->curDocument()->fileName();//获得当前的文件名
	es = pEngine->beginDocument(plotInfo, szDocName, NULL, 1, true, NULL);
	//给打印机和进度对话框发送消息
	pPlotProgDlg->onBeginSheet();
	pPlotProgDlg->setSheetProgressRange(0, 100);
	pPlotProgDlg->setSheetProgressPos(0);
	//begin page
	es = pEngine->beginPage(pageInfo, plotInfo, true);
	es = pEngine->beginGenerateGraphics();
	es = pEngine->endGenerateGraphics();
	//end page
	es = pEngine->endPage();
	pPlotProgDlg->setSheetProgressPos(100);
	pPlotProgDlg->onEndSheet();
	pPlotProgDlg->setPlotProgressPos(100);
	//end document
	es = pEngine->endDocument();
	//end plot
	es = pEngine->endPlot();
	//返回资源
	pEngine->destroy();
	pEngine = NULL;
	pPlotProgDlg->destroy();
	pLayout->close();
}
*/
#endif