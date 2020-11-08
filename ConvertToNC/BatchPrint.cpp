#include "StdAfx.h"
#include "BatchPrint.h"
#include "acaplmgr.h"
#include "XhCharString.h"
#include "CadToolFunc.h"
#include "dbplotsetval.h"
#include "CadObjLife.h"

#ifdef __UBOM_ONLY_
static CXhChar500 GetTempFileFolderPath(bool bEmpty,const char* subFolder)
{	//��ȡ��ʱ�ļ�·��
	//CXhChar500 sSysPath;
	//GetSysPath(sSysPath);
	//CXhChar500 sTemp("%s\\PngTemp\\", (char*)sSysPath);
	CString sWorkPath;
	if(subFolder)
		GetCurWorkPath(sWorkPath, TRUE, subFolder, TRUE);
	else
		GetCurWorkPath(sWorkPath, TRUE, "PNG", TRUE);
	CXhChar500 sTemp(sWorkPath);
	//ȷ��·������
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
	//ɾ����ʱ�ļ����е���������
	return sTemp;
}

//////////////////////////////////////////////////////////////////////////
//
const char *PLOT_CFG::TOWER_TYPE		= "����";
const char *PLOT_CFG::PART_LABEL		= "����";
const char *PLOT_CFG::MAT_MARK			= "����";
const char *PLOT_CFG::MAT_BRIEF_MARK	= "���ʼ��ַ�";
const char *PLOT_CFG::PRE_TOWER_COUNT	= "������";
const char *PLOT_CFG::MANU_COUNT		= "�ӹ���";
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
{	//�жϴ�ӡ���Ƿ����
	if (!CPrintSet::SetPlotMedia(&m_xPngPlotCfg, TRUE))
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//����������ӡ
	CXhChar500 sTempPath= GetTempFileFolderPath(m_bEmptyFiles, "PNG"), sFilePath;
	SetCurrentDirectory(sTempPath);	//���õ�ǰ����·��
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPngPlotCfg;
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		CString sLabel = pScope->GetPartFileName(pPlotCfg, ".png");
		RunPrint(pScope, pPlotCfg, sLabel);
	}
	//��ͼƬ�ļ���
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPDF()
{	//�жϴ�ӡ���Ƿ����
	if (!CPrintSet::SetPlotMedia(&m_xPdfPlotCfg, TRUE))
		return false;
	CAcadSysVarLife varLife("FILEDIA", 0);
	//����������ӡ
	CXhChar500 sTempPath = GetTempFileFolderPath(m_bEmptyFiles, "PDF"), sFilePath;
	SetCurrentDirectory(sTempPath);	//���õ�ǰ����·��
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	PLOT_CFG* pPlotCfg = &m_xPdfPlotCfg;
	for (PRINT_SCOPE *pScope = m_pPrintScopeList->GetFirst(); pScope; pScope = m_pPrintScopeList->GetNext())
	{
		CString sLabel = pScope->GetPartFileName(pPlotCfg, ".pdf");
		RunPrint(pScope, pPlotCfg, sLabel);
	}
	//��ͼƬ�ļ���
	ShellExecute(NULL, "open", NULL, NULL, sTempPath, SW_SHOW);
	return true;
}

bool CBatchPrint::PrintProcessCardToPaper()
{	//�жϴ�ӡ���Ƿ����
	if (!CPrintSet::SetPlotMedia(&m_xPaperPlotCfg, TRUE))
		return false;
	CXhChar500 sTempPath = GetTempFileFolderPath(FALSE, "Parper-Log");
	CTime time = CTime::GetCurrentTime();
	CString sTime = time.Format("%Y%m%d%H%M%S");
	CLogFile plotLog(CXhChar500("%s\\plot(%s).log",(char*)sTempPath,sTime));
	int i = 0;
	//����������ӡ
	if (m_pPrintScopeList == NULL)
		m_pPrintScopeList = &m_xPrintScopeList;
	for(PRINT_SCOPE *pScope= m_pPrintScopeList->GetFirst();pScope;pScope= m_pPrintScopeList->GetNext())
	{
		RunPrint(pScope, &m_xPaperPlotCfg);
		//�����־
		plotLog.Log("%d. %s# %s��%s��Rect:(%.f,%.f),(%.f,%.f)",
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
	//���ݴ�ӡ����������ţ����½���
	ZoomAcadView(pScope->GetCadEntScope(), 10);
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
	//ִ�д�ӡ����
	CString sCmd;
	sCmd.Append("-plot\n");			//��ӡָ��
	sCmd.Append("y\n");				//�Ƿ���Ҫ��ϸ��ӡ����[��(Y)/��(N)]
	sCmd.Append("\n");				//������
	sCmd.Append((char*)sDevice);	//�豸������
	sCmd.Append((char*)sPaperSize);	//ͼֽ�ߴ�
	if (m_ciPrintType == PRINT_TYPE_PDF||m_ciPrintType==PRINT_TYPE_PAPER)
		sCmd.Append("\n");			//����ͼֽ��λ[Ӣ��(I) / ����(M] <Ӣ��>:
	sCmd.Append((char*)sPaperRot);	//ͼ�η���:<����|����>
	sCmd.Append("N\n");				//�Ƿ����ӡ
	sCmd.Append("w\n");				//��ӡ����:<ָ������>
	sCmd.Append((char*)sRectLT);	//���Ͻ�
	sCmd.Append((char*)sRectRB);	//���½�
	sCmd.Append("\n");				//��ӡ����:<����>
	sCmd.Append("C\n");				//��ӡƫ�ƣ�<����>
	sCmd.Append("Y\n");				//�Ƿ���ʽ��ӡ
	sCmd.Append("monochrome.ctb\n");//��ӡ��ʽ����
	sCmd.Append("Y\n");				//��ӡ�߿�
	sCmd.Append("A\n");				//��ɫ��ӡ����
	if(strstr(sDevice,"DWG To PDF.pc3")|| strstr(sDevice,"PublishToWeb PNG.pc3"))
		sCmd.Append((char*)sFileName);	//�����ļ���
	else
		sCmd.Append("N\n");			//�Ƿ��ӡ���ļ�
	sCmd.Append("N\n");				//�Ƿ񱣴��ҳ�����õ��޸�
	sCmd.Append("Y\n");				//�Ƿ������ӡ
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
	//���ô�ӡ����ʼֵ
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

/*
//������ӡ
void batPlot()
{	// ȡ�õ�ǰlayout
	AcDbLayoutManager* pLayoutManager = acdbHostApplicationServices()->layoutManager(); //ȡ�ò��ֹ���������
	AcDbLayout* pLayout = pLayoutManager->findLayoutNamed(pLayoutManager->findActiveLayout(TRUE), TRUE);//��õ�ǰ����
	AcDbObjectId  m_layoutId = pLayout->objectId();//��ò��ֵ�Id
	//��ô�ӡ����֤������
	AcDbPlotSettingsValidator* pPSV = acdbHostApplicationServices()->plotSettingsValidator();
	pPSV->refreshLists(pLayout);
	//��ӡ������
	ACHAR* m_strDevice = _T("DWF6 ePlot.pc3");//��ӡ������
	pPSV->setPlotCfgName(pLayout, m_strDevice);//���ô�ӡ�豸
	ACHAR* m_mediaName = _T("ISO A4");//ͼֽ����
	pPSV->setCanonicalMediaName(pLayout, m_mediaName);//����ͼֽ�ߴ�
	pPSV->setPlotType(pLayout, AcDbPlotSettings::kWindow);//���ô�ӡ��ΧΪ����
	pPSV->setPlotWindowArea(pLayout, 100, 100, 200, 200);//���ô�ӡ��Χ,��������Χ�Ľ��򲻳���
	pPSV->setCurrentStyleSheet(pLayout, _T("JSTRI.ctb"));//���ô�ӡ��ʽ��
	pPSV->setPlotCentered(pLayout, true);//�Ƿ���д�ӡ
	pPSV->setUseStandardScale(pLayout, true);//�����Ƿ���ñ�׼����
	pPSV->setStdScaleType(pLayout, AcDbPlotSettings::kScaleToFit);//����ͼֽ
	pPSV->setPlotRotation(pLayout, AcDbPlotSettings::k90degrees);//���ô�ӡ����
	//pPSV->setPlotViewName(pLayout,_T("��ӡ1"));
	//׼����ӡ/////////////////////////////////////////////////////////////////////////
	AcPlPlotEngine* pEngine = NULL;
	if (AcPlPlotFactory::createPublishEngine(pEngine) != Acad::eOk)
	{
		acedAlert(_T("��ӡʧ��!"));
		return;
	}
	// ��ӡ���ȶԻ���
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
	AcPlPlotPageInfo pageInfo;//��ӡҳ��Ϣ
	AcPlPlotInfo plotInfo; //��ӡ��Ϣ
	// ���ò���
	plotInfo.setLayout(m_layoutId);
	// ���ò���
	plotInfo.setOverrideSettings(pLayout);
	AcPlPlotInfoValidator validator;//������ӡ��Ϣ��֤��
	validator.setMediaMatchingPolicy(AcPlPlotInfoValidator::kMatchEnabled);
	es = validator.validate(plotInfo);
	// begin document
	const TCHAR* szDocName = acDocManager->curDocument()->fileName();//��õ�ǰ���ļ���
	es = pEngine->beginDocument(plotInfo, szDocName, NULL, 1, true, NULL);
	//����ӡ���ͽ��ȶԻ�������Ϣ
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
	//������Դ
	pEngine->destroy();
	pEngine = NULL;
	pPlotProgDlg->destroy();
	pLayout->close();
}
*/
#endif