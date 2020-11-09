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
	else if (bEmpty)
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
//PLOT_CFG
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
//CPrintSet
CString CPrintSet::GetPrintDeviceCmbItemStr()
{
	CString sDeviceStr;
	CLockDocumentLife lock;
	AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
	//get the active layout
	AcDbLayout* pLayout = pLayMan ? pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE) : NULL;
	AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
	if (pLayout && pPsv)
	{
		pPsv->refreshLists(pLayout);        //refresh the Plot Config list
		AcArray<const ACHAR*> mDeviceList;
		pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
		sDeviceStr.Empty();
		for (int i = 0; i < mDeviceList.length(); i++)
		{
			CString sTemp = mDeviceList.at(i);
			if (sDeviceStr.GetLength() > 0)
				sDeviceStr.AppendChar('|');
			sDeviceStr.Append(sTemp);
		}
	}
	if (pLayout)
		pLayout->close();
	return sDeviceStr;
}

CString CPrintSet::GetPaperSizeCmbItemStr(const char* sDeviceName)
{
	CString sMediaNameStr;
	CLockDocumentLife lock;
	AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
	//get the active layout
	AcDbLayout* pLayout = pLayMan ? pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE) : NULL;
	AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
	if (pLayout && pPsv && sDeviceName)
	{
		pPsv->refreshLists(pLayout);        //refresh the Plot Config list
		AcArray<const ACHAR*> mDeviceList;
		pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
		BOOL bValidName = FALSE;
		for (int i = 0; i < mDeviceList.length(); i++)
		{
			CString sTemp = mDeviceList.at(i);
			if (sTemp.CompareNoCase(sDeviceName) == 0)
			{
				bValidName = TRUE;
				break;
			}
		}
		if (bValidName)
		{
#ifdef _ARX_2007
			pPsv->setPlotCfgName(pLayout, _bstr_t(sDeviceName));
#else
			pPsv->setPlotCfgName(pLayout, sDeviceName);
#endif
			//list all the paper sizes in the given Plot configuration
			AcArray<const ACHAR*> mMediaList;
			pPsv->canonicalMediaNameList(pLayout, mMediaList);
			for (int i = 0; i < mMediaList.length(); i++)
			{
				CString sTemp = mMediaList.at(i);
				if (sMediaNameStr.GetLength() > 0)
					sMediaNameStr.AppendChar('|');
				sMediaNameStr.Append(sTemp);
			}
		}
	}
	if (pLayout)
		pLayout->close();
	return sMediaNameStr;
}

CXhChar500 CPrintSet::GetPlotCfgName(bool bPromptInfo)
{	//判断打印机是否合理
	CXhChar500 sPlotCfgName;
	AcApLayoutManager* pLayMan = NULL;
	pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
	AcDbLayout* pLayout = pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE);
	if (pLayout != NULL)
	{
		pLayout->close();
		AcDbPlotSettings* pPlotSetting = (AcDbPlotSettings*)pLayout;
		Acad::ErrorStatus retCode;
#ifdef _ARX_2007
		const ACHAR* sValue;
		retCode = pPlotSetting->getPlotCfgName(sValue);
		if (sValue != NULL)
			sPlotCfgName.Copy((char*)_bstr_t(sValue));
#else
		char* sValue;
		retCode = pPlotSetting->getPlotCfgName(sValue);
		if (sValue != NULL)
			sPlotCfgName.Copy(sValue);
#endif
		if (retCode != Acad::eOk || stricmp(sPlotCfgName, "无") == 0)
		{	//获取输出设备名称
			if (bPromptInfo)
			{
#ifdef _ARX_2007
				acutPrintf(L"\n当前激活打印设备不可用,请优先进行页面设置!");
#else
				acutPrintf("\n当前激活打印设备不可用,请优先进行页面设置!");
#endif
			}
			sPlotCfgName.Empty();
		}
	}
	return sPlotCfgName;
}

BOOL CPrintSet::SetPlotMedia(PLOT_CFG *pPlotCfg, bool bPromptInfo)
{
	CLockDocumentLife lock;
	AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
	//get the active layout
	AcDbLayout* pLayout = pLayMan ? pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE) : NULL;
	AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
	if (pLayout && pPsv)
	{
		pPsv->refreshLists(pLayout);        //refresh the Plot Config list
		//
		//AcArray<const ACHAR*> mDeviceList;
		//pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
		//ACHAR* m_strDevice = _T("DWF6 ePlot.pc3");//打印机名字
#ifdef _ARX_2007
		Acad::ErrorStatus es = pPsv->setPlotCfgName(pLayout, _bstr_t(pPlotCfg->m_sDeviceName));     //设置打印设备
#else
		Acad::ErrorStatus es = pPsv->setPlotCfgName(pLayout, pPlotCfg->m_sDeviceName);            //设置打印设备
#endif
		if (es != Acad::eOk)
		{	//获取输出设备名称
			if (bPromptInfo)
			{
				CXhChar500 sError("\n名称:%s,设置错误，请在设置中确认后重试！", (char*)pPlotCfg->m_sDeviceName);
#ifdef _ARX_2007
				acutPrintf(_bstr_t(sError));
#else
				acutPrintf(sError);
#endif
			}
			pLayout->close();
			return FALSE;
		}
		//ACHAR* m_mediaName = _T("ISO A4");//图纸名称
#ifdef _ARX_2007
		pPsv->setCanonicalMediaName(pLayout, _bstr_t(pPlotCfg->m_sPaperSize));//设置图纸尺寸
		pPsv->setCurrentStyleSheet(pLayout, L"monochrome.ctb");//设置打印样式表
#else
		pPsv->setCanonicalMediaName(pLayout, pPlotCfg->m_sPaperSize);//设置图纸尺寸
		pPsv->setCurrentStyleSheet(pLayout, _T("monochrome.ctb"));//设置打印样式表
#endif
		pPsv->setPlotType(pLayout, AcDbPlotSettings::kWindow);//设置打印范围为窗口
		pPsv->setPlotWindowArea(pLayout, 100, 100, 200, 200);//设置打印范围,超出给范围的将打不出来
		pPsv->setPlotCentered(pLayout, true);//是否居中打印
		pPsv->setUseStandardScale(pLayout, true);//设置是否采用标准比例
		pPsv->setStdScaleType(pLayout, AcDbPlotSettings::kScaleToFit);//布满图纸
		pPsv->setPlotRotation(pLayout, AcDbPlotSettings::k0degrees);//设置打印方向
		//
		pLayout->close();
		return (es == Acad::eOk);
	}
	else
		return FALSE;
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
	m_nManuNum = m_pAngle->m_xAngle.nSumPart;
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
	m_nManuNum = m_pPlate->xBomPlate.nSumPart;
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
bool CBatchPrint::PrintProcessCardToPNG()
{	//判断打印机是否合理
	if (!CPrintSet::SetPlotMedia(&m_xPngPlotCfg, TRUE))
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//进行批量打印
	CXhChar500 sTempPath= GetTempFileFolderPath(m_bEmptyFiles, "PNG"), sFilePath;
	SetCurrentDirectory(sTempPath);	//设置当前工作路径
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPngPlotCfg;
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		CString sLabel = pScope->GetPartFileName(pPlotCfg, ".png");
		RunPrint(pScope, pPlotCfg, sLabel);
	}
	//打开图片文件夹
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPDF()
{	//判断打印机是否合理
	if (!CPrintSet::SetPlotMedia(&m_xPdfPlotCfg, TRUE))
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//进行批量打印
	CXhChar500 sTempPath = GetTempFileFolderPath(m_bEmptyFiles, "PDF"), sFilePath;
	SetCurrentDirectory(sTempPath);	//设置当前工作路径
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPdfPlotCfg;
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		CString sLabel = pScope->GetPartFileName(pPlotCfg, ".pdf");
		RunPrint(pScope, pPlotCfg, sLabel);
	}
	//打开图片文件夹
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPaper()
{	//判断打印机是否合理
	if (!CPrintSet::SetPlotMedia(&m_xPaperPlotCfg, TRUE))
		return false;
	CXhChar500 sTempPath = GetTempFileFolderPath(FALSE, "Parper-Log");
	CTime time = CTime::GetCurrentTime();
	CString sTime = time.Format("%Y%m%d%H%M%S");
	CLogFile plotLog(CXhChar500("%s\\plot(%s).log",(char*)sTempPath,sTime));
	int i = 0;
	//进行批量打印
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	for(PRINT_SCOPE *pScope= m_pPrintScopeList->GetFirst();pScope;pScope= m_pPrintScopeList->GetNext())
	{
		RunPrint(pScope, &m_xPaperPlotCfg);
		//输出日志
		plotLog.Log("%d. %s# %s，%s，Rect:(%.f,%.f),(%.f,%.f)",
			++i, (char*)pScope->m_sPartLabel, (char*)m_xPaperPlotCfg.m_sDeviceName,
			(char*)m_xPaperPlotCfg.m_sPaperSize, pScope->L_T[X], pScope->L_T[Y], 
			pScope->R_B[X], pScope->R_B[Y]);
	}
	return true;
}

bool CBatchPrint::RunPrint(PRINT_SCOPE *pScope, PLOT_CFG* pPlotCfg, const char* file_path /*= NULL*/)
{
	if (pPlotCfg == NULL || pScope == NULL)
		return false;
	CXhChar16 sPaperRot = (pPlotCfg->m_ciPaperRot == 'P') ? "P" : "L";
	CXhChar50 sDevice, sPaperSize, sFileName, sRectLT, sRectRB;
	if (pPlotCfg->m_sDeviceName.GetLength() > 0)
		sDevice.Copy(pPlotCfg->m_sDeviceName);
	if (pPlotCfg->m_sPaperSize.GetLength() > 0)
		sPaperSize.Copy(pPlotCfg->m_sPaperSize);
	if (file_path != NULL && strlen(file_path) > 0)
		sFileName.Copy(file_path);
	sRectLT.Printf("%g,%g\n", pScope->L_T[X], pScope->L_T[Y]);
	sRectRB.Printf("%g,%g\n", pScope->R_B[X], pScope->R_B[Y]);
	sDevice.Append("\n");
	sPaperSize.Append("\n");
	sPaperRot.Append("\n");
	sFileName.Append("\n");
	//根据打印区域进行缩放，更新界面
	ZoomAcadView(pScope->GetCadEntScope(), 10);
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
	//执行打印命令
	CString sCmd;
	sCmd.Append("-plot\n");			//打印指令
	sCmd.Append("y\n");				//是否需要详细打印配置[是(Y)/否(N)]
	sCmd.Append("\n");				//布局名
	sCmd.Append((char*)sDevice);	//设备的名称
	sCmd.Append((char*)sPaperSize);	//图纸尺寸
	if (m_ciPrintType == PRINT_TYPE_PDF||m_ciPrintType==PRINT_TYPE_PAPER)
		sCmd.Append("\n");			//输入图纸单位[英寸(I) / 毫米(M] <英寸>:
	sCmd.Append((char*)sPaperRot);	//图形方向:<横向|纵向>
	sCmd.Append("N\n");				//是否反向打印
	sCmd.Append("w\n");				//打印区域:<指定窗口>
	sCmd.Append((char*)sRectLT);	//左上角
	sCmd.Append((char*)sRectRB);	//右下角
	sCmd.Append("\n");				//打印比例:<布满>
	sCmd.Append("C\n");				//打印偏移：<居中>
	sCmd.Append("Y\n");				//是否按样式打印
	sCmd.Append("monochrome.ctb\n");//打印样式名称
	sCmd.Append("Y\n");				//打印线宽
	sCmd.Append("A\n");				//着色打印设置
	if(strstr(sDevice,"DWG To PDF.pc3")|| strstr(sDevice,"PublishToWeb PNG.pc3"))
		sCmd.Append((char*)sFileName);	//输入文件名
	else
		sCmd.Append("N\n");			//是否打印到文件
	sCmd.Append("N\n");				//是否保存对页面设置的修改
	sCmd.Append("Y\n");				//是否继续打印
#ifdef _ARX_2007
	SendCommandToCad(L"CMDECHO 0\n");
	SendCommandToCad((ACHAR*)_bstr_t(sCmd));
#else
	SendCommandToCad("CMDECHO 0\n");
	SendCommandToCad((char*)sCmd.GetBuffer());
#endif
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
		PrintProcessCardToPNG();
	else if (m_ciPrintType == PRINT_TYPE_PAPER)
		PrintProcessCardToPaper();
	else if (m_ciPrintType == PRINT_TYPE_PDF)
		PrintProcessCardToPDF();
	return true;
}
#endif