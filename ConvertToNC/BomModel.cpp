#include "StdAfx.h"
#include "BomModel.h"
#include "ExcelOper.h"
#include "TblDef.h"
#include "DefCard.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "ComparePartNoString.h"
#include "PNCSysPara.h"
#include "Expression.h"
#include "FileIO.h"
#include "BatchPrint.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#ifdef __UBOM_ONLY_
CBomModel g_xUbomModel;

int compare_func(const CXhChar16& str1, const CXhChar16& str2)
{
	CString keyStr1(str1), keyStr2(str2);
	return ComparePartNoString(keyStr1, keyStr2, "SHGPT");
}
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
	CFileBuffer file(sFilePath,FALSE);
	double version=1.0;
	file.Read(&version,sizeof(double));
	file.ReadString(m_sProjName);
	CXhChar100 sValue;
	file.ReadString(sValue);	//TMA料单
	InitBomInfo(sValue,TRUE);
	file.ReadString(sValue);	//ERP料单
	InitBomInfo(sValue,FALSE);
	//DWG文件信息
	DWORD n=0;
	file.Read(&n,sizeof(DWORD));
	for(DWORD i=0;i<n;i++)
	{
		int ibValue=0;
		file.ReadString(sValue);
		file.Read(&ibValue,sizeof(int));
		AppendDwgBomInfo(sValue);
	}
}
//导出料单工程文件
void CProjectTowerType::WriteProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath,TRUE);
	double version=1.0;
	file.Write(&version,sizeof(double));
	file.WriteString(m_sProjName);
	file.WriteString(m_xLoftBom.m_sFileName);	//TMA料单
	file.WriteString(m_xOrigBom.m_sFileName);	//ERP料单
	//DWG文件信息
	DWORD n=dwgFileList.GetNodeNum();
	file.Write(&n,sizeof(DWORD));
	for(CDwgFileInfo* pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->m_sFileName);
		int ibValue=0;
		if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() <= 0)
			ibValue = 1;
		else if (pDwgInfo->GetAngleNum() > 0 && pDwgInfo->GetPlateNum() > 0)
			ibValue = 2;
		file.Write(&ibValue,sizeof(int));
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
	sCell= g_xUbomModel.m_xMapPrjCell["工程塔型"];
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
		m_xPrjInfo.m_sTaskNo = VariantToString(value);
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
void CProjectTowerType::InitBomInfo(const char* sFileName,BOOL bLoftBom)
{
	CXhChar100 sName;
	_splitpath(sFileName, NULL, NULL, sName, NULL);
	if(bLoftBom)
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
void CProjectTowerType::CompareData(BOMPART* pSrcPart, BOMPART* pDesPart, CHashStrList<BOOL> &hashBoolByPropName)
{
	hashBoolByPropName.Empty();
	//规格
	CString sSpec1(pSrcPart->GetSpec()), sSpec2(pDesPart->GetSpec());
	sSpec1 = sSpec1.Trim();
	sSpec2 = sSpec2.Trim();
	BYTE cPartType1 = pSrcPart->cPartType;
	if (pSrcPart->siSubType == BOMPART::SUB_TYPE_COMMON_PLATE)
		cPartType1 = BOMPART::PLATE;
	else if (pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_WIRE ||
			 pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN ||
			 pSrcPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN)
		cPartType1 = BOMPART::TUBE;
	BYTE cPartType2 = pDesPart->cPartType;
	if (pDesPart->siSubType == BOMPART::SUB_TYPE_COMMON_PLATE)
		cPartType2 = BOMPART::PLATE;
	else if (pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_WIRE ||
			pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN ||
			pDesPart->siSubType == BOMPART::SUB_TYPE_TUBE_MAIN)
		cPartType2 = BOMPART::TUBE;

	if(cPartType2!=cPartType1||
		sSpec1.CompareNoCase(sSpec2)!=0 ) //stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue(CBomConfig::KEY_SPEC, TRUE);
	//材质
	if(toupper(pSrcPart->cMaterial) != toupper(pDesPart->cMaterial))
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	else if(pSrcPart->cMaterial!=pDesPart->cMaterial && !g_xUbomModel.m_bEqualH_h)
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	//质量等级
	if (g_xUbomModel.m_bCmpQualityLevel && pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue(CBomConfig::KEY_MAT, TRUE);
	//角钢定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
			hashBoolByPropName.SetValue(CBomConfig::KEY_SING_N, TRUE);
		//加工数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_N, TRUE);
		//基数
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_TA_MUM) &&
			pSrcPart->nTaNum != pDesPart->nTaNum)
			hashBoolByPropName.SetValue(CBomConfig::KEY_TA_NUM, TRUE);
		//总重
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_W, TRUE);
		//焊接
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_WELD, TRUE);
		//制弯
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomConfig::KEY_ZHI_WAN, TRUE);
		//长度对比
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_LEN))
		{
			if (g_xUbomModel.m_uiCustomizeSerial == ID_JiangSu_HuaDian)
			{	//江苏华电要求图纸长度大于放样长度
				if (pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue(CBomConfig::KEY_LEN, TRUE);
			}
			else
			{	//判断数值是否相等
				if (fabsl(pSrcPart->length - pDesPart->length) > g_xUbomModel.m_fMaxLenErr)
					hashBoolByPropName.SetValue(CBomConfig::KEY_LEN, TRUE);
			}
		}
		//切角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_CUT_ANGLE) &&
			pSrcJg->bCutAngle != pDesJg->bCutAngle)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_ANGLE, TRUE);
		//压扁
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_PUSH_FLAT) &&
			pSrcJg->nPushFlat != pDesJg->nPushFlat)
			hashBoolByPropName.SetValue(CBomConfig::KEY_PUSH_FLAT, TRUE);
		//铲背
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_CUT_BER) &&
			pSrcJg->bCutBer != pDesJg->bCutBer)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_BER, TRUE);
		//刨根
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_CUT_ROOT) &&
			pSrcJg->bCutRoot != pDesJg->bCutRoot)
			hashBoolByPropName.SetValue(CBomConfig::KEY_CUT_ROOT, TRUE);
		//开角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_KAI_JIAO) &&
			pSrcJg->bKaiJiao != pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue(CBomConfig::KEY_KAI_JIAO, TRUE);
		//合角
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_HE_JIAO) &&
			pSrcJg->bHeJiao != pDesJg->bHeJiao)
			hashBoolByPropName.SetValue(CBomConfig::KEY_HE_JIAO, TRUE);
		//脚钉
		if (g_xBomCfg.IsAngleCompareItem(CBomTblTitleCfg::T_FOOT_NAIL) &&
			pSrcJg->bHasFootNail != pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue(CBomConfig::KEY_FOO_NAIL, TRUE);
	}
	//钢板定制校审项
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::PLATE)
	{
		PART_PLATE* pSrcJg = (PART_PLATE*)pSrcPart;
		PART_PLATE* pDesJg = (PART_PLATE*)pDesPart;
		//单基数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::T_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
		{	//青岛豪迈不比较单基数
			if (g_xUbomModel.m_uiCustomizeSerial != ID_QingDao_HaoMai)
				hashBoolByPropName.SetValue(CBomConfig::KEY_SING_N, TRUE);
		}
		//加工数
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::T_MANU_NUM) &&
			pSrcPart->nSumPart != pDesPart->nSumPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_N, TRUE);
		//总重
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::T_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomConfig::KEY_MANU_W, TRUE);
		//焊接
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomConfig::KEY_WELD, TRUE);
		//制弯
		if (g_xBomCfg.IsPlateCompareItem(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomConfig::KEY_ZHI_WAN, TRUE);
	}
}
//比对BOM信息 0.相同 1.不同 2.文件有误
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS=0.5;
	m_hashCompareResultByPartNo.Empty();
	if(m_xLoftBom.GetPartNum()<=0)
	{
		AfxMessageBox("缺少放样BOM信息!");
		return 2;
	}
	if(m_xOrigBom.GetPartNum()<=0)
	{
		AfxMessageBox("缺少工艺科BOM信息");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for(BOMPART *pLoftPart=m_xLoftBom.EnumFirstPart();pLoftPart;pLoftPart=m_xLoftBom.EnumNextPart())
	{	
		BOMPART *pOrgPart = m_xOrigBom.FindPart(pLoftPart->sPartNo);
		if (pOrgPart == NULL)
		{	//1.存在放样构件，不存在原始构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
			pResult->pOrgPart=NULL;
			pResult->pLoftPart=pLoftPart;
		}
		else 
		{	//2. 对比同一件号构件属性
			hashBoolByPropName.Empty();
			CompareData(pLoftPart, pOrgPart, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
				pResult->pOrgPart=pOrgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 遍历导入的原始表,查找是否有漏件情况
	for(BOMPART *pPart=m_xOrigBom.EnumFirstPart();pPart;pPart=m_xOrigBom.EnumNextPart())
	{
		if(m_xLoftBom.FindPart(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart=pPart;
		pResult->pLoftPart=NULL;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
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
	for(CDwgFileInfo* pDwgFile =dwgFileList.GetFirst(); pDwgFile; pDwgFile =dwgFileList.GetNext())
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
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart=NULL;
		pResult->pLoftPart=pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
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
			if(g_xBomCfg.IsTitleCol(i, CBomConfig::KEY_NOTES))
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
				if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_NOTES))
					continue;	//备注不显示
				BOOL bDiff = FALSE;
				if (pResult->hashBoolByPropName.GetValue(g_xBomCfg.GetTitleKey(ii)))
					bDiff = TRUE;
				if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_PN))
				{	//件号
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sPartNo));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_SPEC))
				{	//规格
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sSpec));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(pResult->pLoftPart->sSpec));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MAT))
				{	//材质
					map.SetValueAt(iRow, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_LEN))
				{	//长度
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_SING_N))
				{	//单基数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->GetPartNum()));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->GetPartNum()));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MANU_N))
				{	//加工数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->nSumPart));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->nSumPart));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_MANU_W))
				{	//加工重量
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->fSumWeight)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->fSumWeight)));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_WELD))
				{	//焊接
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->bWeldPart ? COleVariant("*") : COleVariant(""));
				}
				else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_ZHI_WAN))
				{	//制弯
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				}
				else if (pResult->pOrgPart->cPartType == BOMPART::ANGLE && pResult->pLoftPart->cPartType==BOMPART::ANGLE)
				{
					PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
					PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
					if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_ANGLE))
					{	//切角
						map.SetValueAt(iRow, iCol, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_PUSH_FLAT))
					{	//打扁
						map.SetValueAt(iRow, iCol, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_BER))
					{	//铲背
						map.SetValueAt(iRow, iCol, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_CUT_ROOT))
					{	//刨根
						map.SetValueAt(iRow, iCol, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_KAI_JIAO))
					{	//开角
						map.SetValueAt(iRow, iCol, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_HE_JIAO))
					{	//合角
						map.SetValueAt(iRow, iCol, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xBomCfg.IsTitleCol(ii, CBomConfig::KEY_FOO_NAIL))
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
			if(iSheet==2)
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
			if(pBomPart==NULL)
				continue;
			map.SetValueAt(index, 0, COleVariant(pBomPart->sPartNo));
			map.SetValueAt(index, 1, COleVariant(pBomPart->sSpec));
			map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pBomPart)));
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
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("DWG缺少构件");
	//设置标题
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="构件编号";str_arr[1]="设计规格";str_arr[2]="材质";
	str_arr[3]="长度";str_arr[4]="单基数";str_arr[5]="加工数";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//填充内容
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//获取Excel表格的范围
	int index=0;
	for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
	{
		if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart)
			continue;
		map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->sPartNo));
		map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->sSpec));
		map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
		map.SetValueAt(index,3,COleVariant(CXhChar50("%.0f",pResult->pLoftPart->length)));
		map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->GetPartNum()));
		map.SetValueAt(index,5,COleVariant((long)pResult->pLoftPart->nSumPart));
		index++;
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
//导出比较结果
void CProjectTowerType::ExportCompareResult(int iCompareType)
{
	int nSheet = 3;
	if (iCompareType == COMPARE_ALL_DWGS)
		nSheet = 1;
	if (iCompareType == COMPARE_DWG_FILE)
		nSheet = 2;
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(nSheet);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for(int iSheet=1;iSheet<=nSheet;iSheet++)
	{
		LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((short)iSheet));
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
	if(pWorksheets==NULL)
	{
		AfxMessageBox("ERP料单文件打开失败!");
		return FALSE;
	}
	//获取Excel指定Sheet内容存储至sheetContentMap中
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short) 1));
	_Worksheet   excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet);
	excel_sheet.Select();
	Range excel_usedRange,excel_range;
	excel_usedRange.AttachDispatch(excel_sheet.GetUsedRange());
	excel_range.AttachDispatch(excel_usedRange.GetRows());
	long nRowNum = excel_range.GetCount();
	//excel_usedRange计算行数时会少一行，原因未知。暂时在此处增加行数 wht 20-04-24
	nRowNum += 10;
	excel_range.AttachDispatch(excel_usedRange.GetColumns());
	long nColNum = excel_range.GetCount();
	CVariant2dArray sheetContentMap(1,1);
	CXhChar50 cell=CExcelOper::GetCellPos(nColNum,nRowNum);
	LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"),COleVariant(cell));
	excel_range.AttachDispatch(pRange);
	sheetContentMap.var=excel_range.GetValue();
	excel_usedRange.ReleaseDispatch();
	excel_range.ReleaseDispatch();
	//更新指定sheet内容
	int iPartNoCol=1,iMartCol=3;
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar16 sPartNo,sMaterial,sNewPartNo;
		//件号
		sheetContentMap.GetValueAt(i,iPartNoCol,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//材质
		sheetContentMap.GetValueAt(i,iMartCol,value);
		sMaterial =VariantToString(value);
		//更新件号数据
		if(strstr(sMaterial,"Q345") && strstr(sPartNo,"H")==NULL)
		{
			if(ciMatCharPosType==0)
				sNewPartNo.Printf("H%s",(char*)sPartNo);
			else if(ciMatCharPosType==1)
				sNewPartNo.Printf("%sH",(char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet,iPartNoCol,i+1,sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo,sNewPartNo);
		}
		if(strstr(sMaterial,"Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if(strstr(sMaterial,"Q420") && strstr(sPartNo,"P")==NULL)
		{	
			if(ciMatCharPosType==0)
				sNewPartNo.Printf("P%s",(char*)sPartNo);
			else if(ciMatCharPosType==1)
				sNewPartNo.Printf("%sP",(char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet,iPartNoCol,i+1,sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo,sNewPartNo);
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
		pPlateInfo=pDwgFile->FindPlateByPartNo(sPartNo);
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
CBomModel::CBomModel(void)
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
CBomModel::~CBomModel(void)
{
	
}
void CBomModel::InitBomModel()
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
bool CBomModel::IsValidFunc(int iFuncType)
{
	if (iFuncType > 0)
		return (m_dwFunctionFlag&GetSingleWord(iFuncType)) > 0;
	else
		return false;
}
CXhChar16 CBomModel::QueryMatMarkIncQuality(BOMPART *pPart)
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
BOOL CBomModel::IsNeedPrint(BOMPART *pPart,const char* sNotes)
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
	if(sNotes!=NULL)
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
BOOL CBomModel::IsJgCardBlockName(const char* sBlockName)
{
	if (sBlockName != NULL && strcmp(sBlockName, "JgCard") == 0)
		return TRUE;
	else if (m_sJgCardBlockName.GetLength() > 0 && m_sJgCardBlockName.EqualNoCase(sBlockName))
		return TRUE;
	else
		return false;
}
BOOL CBomModel::IsPartLabelTitle(const char* sText)
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
//////////////////////////////////////////////////////////////////////////
//读取UBOM配置信息
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
		if(fp==NULL)
			continue;
		//读取文件，获取客户ID
		int nClientId = 0;
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
				g_xUbomModel.m_xMapClientCfgFile.insert(std::make_pair(nClientId, file_name));
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
			if(skey!=NULL)
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
		else if (_stricmp(key_word, "PRINT_BOM") == 0)
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
	if (g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE) ||
		g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_BATCH_PRINT))
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
	//导出UBOM下的钢板提取规则设置
	PNCSysSetExportDefault();
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