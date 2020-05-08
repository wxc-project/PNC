#include "StdAfx.h"
#include "BomModel.h"
#include "ExcelOper.h"
#include "TblDef.h"
#include "DefCard.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "ComparePartNoString.h"
#include "PNCSysPara.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CBomModel g_xUbomModel;

static CXhChar100 VariantToString(VARIANT value)
{
	CXhChar100 sValue;
	if(value.vt==VT_BSTR)
		return sValue.Copy(CString(value.bstrVal));
	else if(value.vt==VT_R8)
		return sValue.Copy(CXhChar100(value.dblVal));
	else if(value.vt==VT_R4)
		return sValue.Copy(CXhChar100(value.fltVal));
	else if(value.vt==VT_INT)
		return sValue.Copy(CXhChar100(value.intVal));
	else 
		return sValue;
}
//
static BOMPART* CreateBomPart(int idClsType, const char* key, void* pContext)
{
	BOMPART *pPart = NULL;
	switch (idClsType) {
	case BOMPART::PLATE:
		pPart = new PART_PLATE();
		break;
	case BOMPART::ANGLE:
		pPart = new PART_ANGLE();
		break;
	case BOMPART::TUBE:
		pPart = new PART_TUBE;
		break;
	case BOMPART::SLOT:
		pPart = new PART_SLOT;
		break;
	default:
		pPart = new BOMPART();
		break;
	}
	return pPart;
}
static BOOL DeleteBomPart(BOMPART *pPart)
{
	if (pPart == NULL)
		return FALSE;
	switch (pPart->cPartType) {
	case BOMPART::PLATE:
		delete (PART_PLATE*)pPart;
		break;
	case BOMPART::ANGLE:
		delete (PART_ANGLE*)pPart;
		break;
	case BOMPART::TUBE:
		delete (PART_TUBE*)pPart;
		break;
	case BOMPART::SLOT:
		delete (PART_SLOT*)pPart;
		break;
	default:
		delete pPart;
		break;
	}
	return TRUE;
}
int compare_func(const CXhChar16& str1,const CXhChar16& str2)
{
	CString keyStr1(str1),keyStr2(str2);
	return ComparePartNoString(keyStr1, keyStr2, "SHGPT");
}
//////////////////////////////////////////////////////////////////////////
//CBomFile
CBomFile::CBomFile()
{
	m_hashPartByPartNo.CreateNewAtom = CreateBomPart;
	m_hashPartByPartNo.DeleteAtom = DeleteBomPart;
	m_pProject=NULL;
}
CBomFile::~CBomFile()
{

}
//
void CBomFile::UpdateProcessPart(const char* sOldKey,const char* sNewKey)
{
	BOMPART* pPart=m_hashPartByPartNo.GetValue(sOldKey);
	if(pPart==NULL)
	{
		logerr.Log("%s构件找不到",sOldKey);
		return;
	}
	pPart=m_hashPartByPartNo.ModifyKeyStr(sOldKey,sNewKey);
	if (pPart)
		pPart->sPartNo.Copy(sNewKey);
}
//解析BOMSHEET内容
BOOL CBomFile::ParseSheetContent(CVariant2dArray &sheetContentMap,CHashStrList<DWORD>& hashColIndex,int iStartRow)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	CLogErrorLife logLife;
	//1、判断列是否满足要求
	BOOL bHasColIndex = (hashColIndex.GetNodeNum() > 2) ? TRUE : FALSE;
	BOOL bHasPartNo = FALSE, bHasMet = FALSE, bHasSpec = FALSE, bValid = FALSE;
	int iContentRow = 0, nColNum = sheetContentMap.ColsCount();
	for (int iRow = 0; iRow < 10; iRow++)
	{
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			VARIANT value;
			sheetContentMap.GetValueAt(iRow, iCol, value);
			if (value.vt == VT_EMPTY)
				continue;
			CString str(VariantToString(value));
			str.Remove('\n');
			if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_PART_NO, str))
			{
				bHasPartNo = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_PART_NO));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_METERIAL, str))
			{
				bHasMet = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_METERIAL));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SPEC, str))
			{
				bHasSpec = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_SPEC));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_LEN, str))
			{
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_LEN));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SING_NUM, str))
			{
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_SING_NUM));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			if(bHasPartNo && bHasMet && bHasSpec)
			{
				bValid = TRUE;
				break;
			}
		}
		if (bValid)
		{
			iContentRow = iRow + 1;
			break;
		}
	}
	if (!bValid)
		return FALSE;
	//2.获取Excel所有单元格的值
	if (iStartRow <= 0)
		iStartRow = iContentRow;
	CStringArray repeatPartLabelArr;
	int nRowCount = sheetContentMap.RowsCount();
	for(int i=iStartRow;i<= nRowCount;i++)
	{
		VARIANT value;
		int nSingleNum = 0, nProcessNum = 0;
		double fLength = 0, fWeight = 0, fSumWeight = 0;
		CXhChar100 sPartNo, sMaterial, sSpec, sNote, sReplaceSpec, sValue;
		CXhChar100 sSingleNum, sProcessNum;
		BOOL bCutAngle = FALSE, bCutRoot = FALSE, bCutBer = FALSE, bPushFlat = FALSE;
		BOOL bKaiJiao = FALSE, bHeJiao = FALSE, bWeld = FALSE, bZhiWan = FALSE;
		//件号
		DWORD *pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.GetLength()<2)
			continue;
		//材质
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_METERIAL);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		sMaterial = VariantToString(value);
		//规格
		int cls_id=0;
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if (strstr(sSpec, "L") || strstr(sSpec, "∠"))
		{
			cls_id = BOMPART::ANGLE;
			if (strstr(sSpec, "∠"))
				sSpec.Replace("∠", "L");
			if (strstr(sSpec, "×"))
				sSpec.Replace("×", "*");
			if (strstr(sSpec, "x"))
				sSpec.Replace("x", "*");
			if (strstr(sSpec, "X"))
				sSpec.Replace("X", "*");
		}
		else if (strstr(sSpec, "-"))
		{
			cls_id = BOMPART::PLATE;
			if (strstr(sSpec, "x")|| strstr(sSpec, "X"))
			{
				char *skey = strtok((char*)sSpec, "x,X");
				sSpec.Copy(skey);
			}
		}
		else //if(strstr(sSpec,"φ"))
			cls_id = BOMPART::TUBE;
		//类型
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PARTTYPE);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			CXhChar100 sPartType = VariantToString(value);
			if (strstr(sPartType, "L") || strstr(sPartType, "∠") || strstr(sPartType, "角钢"))
				cls_id = BOMPART::ANGLE;
			else if (strstr(sPartType, "-") || strstr(sPartType, "钢板"))
				cls_id = BOMPART::PLATE;
			else
				cls_id = BOMPART::TUBE;
			//
			if (cls_id == BOMPART::ANGLE && strstr(sSpec, "L") == NULL)
			{
				sSpec.InsertBefore('L');
				if (strstr(sSpec, "×"))
					sSpec.Replace("×", "*");
				if (strstr(sSpec, "x"))
					sSpec.Replace("x", "*");
			}
			if (cls_id == BOMPART::PLATE && strstr(sSpec, "-") == NULL)
				sSpec.InsertBefore('-');
		}
		//代用规格
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_REPLACE_SPEC);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sReplaceSpec = VariantToString(value);
		}
		//长度
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_LEN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fLength = (float)atof(VariantToString(value));
		}
		//单基数
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sSingleNum = VariantToString(value);
			if(sSingleNum.GetLength()>0)
				nSingleNum = atoi(sSingleNum);
		}
		//加工数
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sProcessNum = VariantToString(value);
			if(sProcessNum.GetLength()>0)
				nProcessNum = atoi(sProcessNum);
		}
		//单基重量
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		//加工重量
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fSumWeight = atof(VariantToString(value));
		}
		//备注
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sNote = VariantToString(value);
		}
		//焊接
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_WELD);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bWeld = TRUE;
		}
		//制弯
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bZhiWan = TRUE;
		}
		//切角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutAngle = TRUE;
		}
		//压扁
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bPushFlat = TRUE;
		}
		//刨根
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutRoot = TRUE;
		}
		//铲背
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutBer = TRUE;
		}
		//开角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_KAI_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bKaiJiao = TRUE;
		}
		//合角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_HE_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bHeJiao = TRUE;
		}
		//材质、规格、单基数、加工数同时为空时，此行为无效行
		if (sMaterial.GetLength() <= 0 && sSpec.GetLength() <= 0 &&
			sSingleNum.GetLength() <= 0 && sProcessNum.GetLength() <= 0)
			continue;	//当前行为无效行，跳过此行 wht 20-03-05
		//填充哈希表
		if(sMaterial.GetLength()<=0 && sSpec.GetLength()<=0)
			continue;	//异常数据
		BOMPART* pBomPart = NULL;
		if (pBomPart = m_hashPartByPartNo.GetValue(sPartNo))
			repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d",(char*)sPartNo,sProcessNum));
			//logerr.Log("存在重复件号：%s", (char*)sPartNo);
		int nWidth = 0, nThick = 0;
		CProcessPart::RestoreSpec(sSpec, &nWidth, &nThick);
		if (pBomPart == NULL)
		{
			pBomPart = m_hashPartByPartNo.Add(sPartNo, cls_id);
			pBomPart->SetPartNum(nSingleNum);
			pBomPart->feature1 = nProcessNum;
			pBomPart->sPartNo.Copy(sPartNo);
			pBomPart->sSpec.Copy(sSpec);
			pBomPart->sMaterial = sMaterial;
			pBomPart->cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_JiangSu_HuaDian &&
				g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_ChengDu_DongFang)
				pBomPart->cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
			pBomPart->length = fLength;
			pBomPart->fPieceWeight = fWeight;
			pBomPart->fSumWeight = fSumWeight;
			pBomPart->wide = (double)nWidth;
			pBomPart->thick = (double)nThick;
			strcpy(pBomPart->sNotes, sNote);
			if (bZhiWan)
				pBomPart->siZhiWan = 1;
			if (cls_id == BOMPART::ANGLE)
			{
				PART_ANGLE* pBomJg = (PART_ANGLE*)pBomPart;
				pBomJg->bCutAngle = bCutAngle;
				pBomJg->bCutRoot = bCutRoot;
				pBomJg->bCutBer = bCutBer;
				pBomJg->bKaiJiao = bKaiJiao;
				pBomJg->bHeJiao = bHeJiao;
				pBomJg->bWeldPart = bWeld;
				if (strstr(pBomPart->sNotes, "开角"))
					pBomJg->bKaiJiao = TRUE;
				if (strstr(pBomPart->sNotes, "合角"))
					pBomJg->bHeJiao = TRUE;
				if (strstr(pBomJg->sNotes, "脚钉"))
					pBomJg->bHasFootNail = TRUE;
				if (bPushFlat)
					pBomJg->nPushFlat = 1;
			}
			else if (cls_id == BOMPART::PLATE)
			{
				PART_PLATE* pBomPlate = (PART_PLATE*)pBomPart;
				pBomPlate->bWeldPart = bWeld;
			}
		}
		else
		{	//件号重复，加工数按累加计算 wht 19-09-15
			pBomPart->feature1 += nProcessNum;
		}
	}
	if (repeatPartLabelArr.GetSize() > 0)
	{	//提示用户存在重复件号，件数按累加统计 wht 20-03-05
		logerr.Log("文件名：%s\n", (char*)m_sFileName);
		logerr.Log("存在重复件号（加工件数按累加计算）：\n");
		logerr.Log("件号\t\t\t加工数\n");
		for (int i = 0; i < repeatPartLabelArr.GetSize(); i++)
			logerr.Log(repeatPartLabelArr[i]);
	}
	return TRUE;
}
//导入放样料单EXCEL文件
BOOL CBomFile::ImportTmaExcelFile()
{
	if(m_sFileName.GetLength()<=0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	//获取定制列信息
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xTmaTblCfg.m_nStartRow-1;
	//解析数据
	int nSheetNum = excelobj.GetWorkSheetCount();
	int nValidSheetCount = 0, iValidSheet = (nSheetNum == 1) ? 1 : 0;
	BOOL bRetCode = FALSE;
	m_hashPartByPartNo.Empty();
	int iCfgSheetIndex = 0;
	if (g_xUbomModel.m_sTMABomSheetName.GetLength() > 0)	//根据配置文件中指定的sheet加载表单
	{
		iCfgSheetIndex = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sTMABomSheetName);
		if (iCfgSheetIndex > 0)
			iValidSheet = iCfgSheetIndex;
	}
	if (iCfgSheetIndex==0 && g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_QingDao_HaoMai)	//青岛豪迈读取放样原始材料表
		iValidSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, "放样原始材料表");
	if (iValidSheet > 0)
	{	//优先读取指定sheet
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iValidSheet);
		if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
		{
			nValidSheetCount++;
			bRetCode = TRUE;
		}
	}
	if (!bRetCode)
	{
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{	//2、解析数据
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet);
			if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
				nValidSheetCount++;
		}
	}
	if (nValidSheetCount == 0)
	{
		logerr.Log("缺少关键列(件号或规格或材质或单基数)!");
		return FALSE;
	}
	else
		return TRUE;
}
//导入ERP料单EXCEL文件
BOOL CBomFile::ImportErpExcelFile()
{
	if (m_sFileName.GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	//读取sheet内容
	int nSheetNum = excelobj.GetWorkSheetCount();
	int iValidSheet = (nSheetNum >= 1) ? 1 : 0;
	BOOL bRetCode = FALSE;
	if (g_xUbomModel.m_sERPBomSheetName.GetLength() > 0)	//根据配置文件中指定的sheet加载表单
		iValidSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sERPBomSheetName);
	//获取Excel内容存储至sheetContentMap中,建立列标题与列索引映射表hashColIndexByColTitle
	CVariant2dArray sheetContentMap(1,1);
	if (g_xUbomModel.m_sTMABomSheetName.GetLength() > 0)	//根据配置文件中指定的sheet加载表单
		iValidSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sTMABomSheetName);
	//最大支持52行，不设置最大列数时如果整行设置背景色会导致自动获取到的列数过大，加载Excel速度慢 wht 19-12-30
	//if(!CExcelOper::GetExcelContentOfSpecifySheet(m_sFileName,sheetContentMap,1,52))
	if (!CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iValidSheet,52))
		return false;
	//获取定制列信息
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xErpTblCfg.m_nStartRow-1;
	//解析数据
	m_hashPartByPartNo.Empty();
	return ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow);
}

CString CBomFile::GetPartNumStr()
{
	int nSumNum = m_hashPartByPartNo.GetNodeNum();
	int nJgNum = 0, nPlateNum = 0, nOtherNum = 0;
	for (BOMPART *pPart = m_hashPartByPartNo.GetFirst(); pPart; pPart = m_hashPartByPartNo.GetNext())
	{
		if (pPart->cPartType==BOMPART::ANGLE)
			nJgNum++;
		else if (pPart->cPartType==BOMPART::PLATE)
			nPlateNum++;
	}
	nOtherNum = nSumNum - nPlateNum - nJgNum;
	CString sNum;
	if (nSumNum == 0)
		sNum = "0";
	else if (nPlateNum > 0 && nJgNum > 0 && nOtherNum > 0)
		sNum.Format("%d=%d+%d+%d", nSumNum, nJgNum, nPlateNum, nOtherNum);
	else if (nPlateNum > 0 && nJgNum > 0)
		sNum.Format("%d=%d+%d", nSumNum, nJgNum, nPlateNum);
	else
		sNum.Format("%d", nSumNum);
	return sNum;
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
		AppendDwgBomInfo(sValue,ibValue);
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
		if(pDwgInfo->IsJgDwgInfo())
			ibValue=1;
		file.Write(&ibValue,sizeof(int));
	}
}
BOOL CProjectTowerType::IsTmaBomFile(const char* sFilePath, BOOL bDisplayMsgBox /*= FALSE*/)
{
	char sFileName[MAX_PATH] = "";
	_splitpath(sFilePath, NULL, NULL, sFileName, NULL);
	if (g_xUbomModel.m_sTMABomFileKeyStr.GetLength() > 0)
	{	//根据设置的关键字，判断是否为TMA表单 wht 20-04-29
		CXhChar200 sCurFileName(sFileName);
		CXhChar200 sKeyStr(g_xUbomModel.m_sTMABomFileKeyStr);
		sCurFileName.ToUpper();
		sKeyStr.ToUpper();
		if (strstr(sCurFileName, sKeyStr)!=NULL)
			return TRUE;
		else
			return FALSE;
	}
	else if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
	{	//安徽宏源料单文件中含有关键字
		if (strstr(sFileName, "TMA") || strstr(sFileName, "tma"))
			return TRUE;
		else
			return FALSE;
	}
	else
	{	//根据定制列进行识别
		if (m_xLoftBom.GetPartNum() > 0)
			return FALSE;	//放样数据已读取
		if (g_xUbomModel.m_xTmaTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if (bDisplayMsgBox)
				AfxMessageBox("没有定制BOM数据列,读取失败!");
			else
				logerr.Log("没有定制BOM数据列,读取失败!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1))
			return FALSE;
		BOOL bValid = FALSE;
		int nColNum = hashColIndex.GetNodeNum();
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			CXhChar50 sCol(CBomTblTitleCfg::GetColName(iCol));
			DWORD* pColIndex = hashColIndex.GetValue(sCol);
			if(pColIndex==NULL)
				continue;
			BOOL bValid = FALSE;
			for (int iRow = 0; iRow < 10; iRow++)
			{
				VARIANT value;
				sheetContentMap.GetValueAt(iRow, *pColIndex, value);
				if (value.vt == VT_EMPTY)
					continue;
				CString str(value.bstrVal);
				str.Remove('\n');
				if (!bValid && CBomTblTitleCfg::IsMatchTitle(iCol, str))
				{
					bValid = TRUE;
					break;
				}
			}
			if (!bValid)
				return FALSE;
		}
	}
	return TRUE;
}
BOOL CProjectTowerType::IsErpBomFile(const char* sFilePath, BOOL bDisplayMsgBox /*= FALSE*/)
{
	char sFileName[MAX_PATH] = "";
	_splitpath(sFilePath, NULL, NULL, sFileName, NULL);
	if (g_xUbomModel.m_sERPBomFileKeyStr.GetLength() > 0)
	{	//根据设置的关键字，判断是否为TMA表单 wht 20-04-29
		CXhChar200 sCurFileName(sFileName);
		CXhChar200 sKeyStr(g_xUbomModel.m_sERPBomFileKeyStr);
		sCurFileName.ToUpper();
		sKeyStr.ToUpper();
		if (strstr(sCurFileName, sKeyStr) != NULL)
			return TRUE;
		else
			return FALSE;
	}
	else if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
	{
		if (strstr(sFileName, "ERP") || strstr(sFileName, "erp"))
			return TRUE;	//安徽宏源料单文件中含有关键字
		else
			return FALSE;
	}
	else
	{	//根据定制列进行识别
		if (m_xOrigBom.GetPartNum() > 0)
			return FALSE;	//放样数据已读取
		if (g_xUbomModel.m_xErpTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if(bDisplayMsgBox)
				AfxMessageBox("没有定制BOM数据列,读取失败!");
			else
				logerr.Log("没有定制BOM数据列,读取失败!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1))
			return FALSE;
		BOOL bValid = FALSE;
		int nColNum = hashColIndex.GetNodeNum();
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			CXhChar50 sCol(CBomTblTitleCfg::GetColName(iCol));
			DWORD* pColIndex = hashColIndex.GetValue(sCol);
			if (pColIndex == NULL)
				continue;
			BOOL bValid = FALSE;
			for (int iRow = 0; iRow < 10; iRow++)
			{
				VARIANT value;
				sheetContentMap.GetValueAt(iRow, *pColIndex, value);
				if (value.vt == VT_EMPTY)
					continue;
				CString str(value.bstrVal);
				if (!bValid && CBomTblTitleCfg::IsMatchTitle(iCol, str))
				{
					bValid = TRUE;
					break;
				}
			}
			if (!bValid)
				return FALSE;
		}
	}
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
		m_xLoftBom.ImportTmaExcelFile();
	}
	else
	{	
		m_xOrigBom.m_sBomName.Copy(sName);
		m_xOrigBom.m_sFileName.Copy(sFileName);
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImportErpExcelFile();
	}
}
//添加角钢DWGBOM信息
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName, BOOL bJgDxf)
{
	if (strlen(sFileName) <= 0)
		return FALSE;
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
	if (strstr(file_path, sFileName))	//激活指定文件
		acDocManager->activateDocument(pDoc);
	else
	{		//打开指定文件
#ifdef _ARX_2007
		acDocManager->appContextOpenDocument(_bstr_t(sFileName));
#else
		acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	//读取DWG文件信息
	CWaitCursor wait;
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile)
	{
		pDwgFile->ExtractDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
	}
	else
	{
		pDwgFile = dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		//读取DWG信息
		pDwgFile->ExtractDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
	}
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
	//获取定制列信息
	CHashStrList<DWORD> hashColIndex;
	g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
	hashBoolByPropName.Empty();
	//规格
	if(pSrcPart->cPartType!=pDesPart->cPartType||
		stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue(CBomModel::KEY_SPEC, TRUE);
	//材质
	if (pSrcPart->cMaterial != pDesPart->cMaterial)
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	if (pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	//单基数
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM) &&
		pSrcPart->GetPartNum() != pDesPart->GetPartNum())
	{	//青岛豪迈不比较单基数
		if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_QingDao_HaoMai)
			hashBoolByPropName.SetValue(CBomModel::KEY_SING_N, TRUE);
	}
	//加工数
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM) &&
		pSrcPart->feature1 != pDesPart->feature1)
	{	//安徽宏源不比较加工数，但需要修正加工数
		if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_AnHui_HongYuan)
			hashBoolByPropName.SetValue(CBomModel::KEY_MANU_N, TRUE);
	}
	//总重
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT) &&
		fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
		hashBoolByPropName.SetValue(CBomModel::KEY_MANU_W, TRUE);
	//焊接
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD) &&
		pSrcPart->bWeldPart != pDesPart->bWeldPart)
		hashBoolByPropName.SetValue(CBomModel::KEY_WELD, TRUE);
	//制弯
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN) &&
		pSrcPart->siZhiWan != pDesPart->siZhiWan)
		hashBoolByPropName.SetValue(CBomModel::KEY_ZHI_WAN, TRUE);
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{	//角钢信息对比
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		//长度对比
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_LEN))
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
			{	//江苏华电要求图纸长度大于放样长度
				if (pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue(CBomModel::KEY_LEN, TRUE);
			}
			else
			{	//判断数值是否相等
				if (fabsl(pSrcPart->length - pDesPart->length) > g_pncSysPara.m_fMaxLenErr)
					hashBoolByPropName.SetValue(CBomModel::KEY_LEN, TRUE);
			}
		}
		//切角
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) &&
			pSrcJg->bCutAngle != pDesJg->bCutAngle)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_ANGLE, TRUE);
		//压扁
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT) &&
			pSrcJg->nPushFlat != pDesJg->nPushFlat)
			hashBoolByPropName.SetValue(CBomModel::KEY_PUSH_FLAT, TRUE);
		//铲背
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER) &&
			pSrcJg->bCutBer != pDesJg->bCutBer)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_BER, TRUE);
		//刨根
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT) &&
			pSrcJg->bCutRoot != pDesJg->bCutRoot)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_ROOT, TRUE);
		//开角
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES) &&
			pSrcJg->bKaiJiao != pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue(CBomModel::KEY_KAI_JIAO, TRUE);
		//合角
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES) &&
			pSrcJg->bHeJiao != pDesJg->bHeJiao)
			hashBoolByPropName.SetValue(CBomModel::KEY_HE_JIAO, TRUE);
		//脚钉
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES) &&
			pSrcJg->bHasFootNail != pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue(CBomModel::KEY_FOO_NAIL, TRUE);
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
//进行角钢DWG文件的漏号检测
int CProjectTowerType::CompareLoftAndAngleDwgs()
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashAngleByPartNo;
	for(CDwgFileInfo* pJgDwg=dwgFileList.GetFirst();pJgDwg;pJgDwg=dwgFileList.GetNext())
	{
		if(!pJgDwg->IsJgDwgInfo())
			continue;
		for(CAngleProcessInfo* pJgInfo=pJgDwg->EnumFirstJg();pJgInfo;pJgInfo=pJgDwg->EnumNextJg())
			hashAngleByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo,TRUE);
	}
	for(BOMPART *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(pPart->cPartType!=BOMPART::ANGLE)
			continue;
		if(hashAngleByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart=NULL;
		pResult->pLoftPart=pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("已打开角钢DWG文件没有漏号情况!");
		return 0;
	}
	else
		return 1;
}
//进行钢板DWG文件的漏号检测
int CProjectTowerType::CompareLoftAndPlateDwgs()
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashPlateByPartNo;
	for(CDwgFileInfo* pPlateDwg=dwgFileList.GetFirst();pPlateDwg;pPlateDwg=dwgFileList.GetNext())
	{
		if(pPlateDwg->IsJgDwgInfo())
			continue;
		for(CPlateProcessInfo* pPlateInfo=pPlateDwg->EnumFirstPlate();pPlateInfo;pPlateInfo=pPlateDwg->EnumNextPlate())
			hashPlateByPartNo.SetValue(pPlateInfo->xPlate.GetPartNo(),TRUE);
	}
	for(BOMPART *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(pPart->cPartType!=BOMPART::PLATE)
			continue;
		if(hashPlateByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
		pResult->pOrgPart=NULL;
		pResult->pLoftPart=pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("已打开钢板DWG文件没有漏号情况!");
		return 0;
	}
	else
		return 1;
}
//
int CProjectTowerType::CompareLoftAndAngleDwg(const char* sFileName)
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
	CAngleProcessInfo* pJgInfo = NULL;
	for (pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = pDwgFile->EnumNextJg())
	{
		BOMPART *pDwgJg = &(pJgInfo->m_xAngle);
		BOMPART *pLoftJg=m_xLoftBom.FindPart(pJgInfo->m_xAngle.sPartNo);
		if(pLoftJg==NULL)
		{	//1、存在DWG构件，不存在放样构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pJgInfo->m_xAngle.sPartNo);
			pResult->pOrgPart = pDwgJg;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2、对比同一件号构件属性
			CompareData(pLoftJg, pDwgJg, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgJg->sPartNo);
				pResult->pOrgPart= pDwgJg;
				pResult->pLoftPart=pLoftJg;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//单张DWG进行校审时，不进行漏号检测
	//for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	//{
	//	if (pPart->cPartType!=BOMPART::ANGLE)
	//		continue;
	//	if (pDwgFile->FindAngleByPartNo(pPart->sPartNo))
	//		continue;
	//	//3、存在BOM构件，不存在DWG构件
	//	COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
	//	pResult->pOrgPart = NULL;
	//	pResult->pLoftPart = pPart;
	//}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("BOM角钢信息和DWG角钢信息相同!");
		return 0;
	}
	else
		return 1;
}
//
int CProjectTowerType::CompareLoftAndPlateDwg(const char* sFileName)
{
	const double COMPARE_EPS=0.5;
	if(m_xLoftBom.GetPartNum()<=0)
	{
		AfxMessageBox("缺少BOM信息!");
		return 2;
	}
	CDwgFileInfo* pDwgFile=FindDwgBomInfo(sFileName);
	if(pDwgFile==NULL)
	{
		AfxMessageBox("未找到指定的钢板DWG文件!");
		return 2;
	}
	//进行数据比对
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CPlateProcessInfo* pPlateInfo = NULL;
	for (pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
	{
		PART_PLATE* pDwgPlate = &(pPlateInfo->xBomPlate);
		BOMPART *pLoftPlate=m_xLoftBom.FindPart(pDwgPlate->sPartNo);
		if(pLoftPlate==NULL)
		{	//1、存在DWG构件，不存在放样构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
			pResult->pOrgPart = pDwgPlate;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2、对比同一件号构件属性
			CompareData(pLoftPlate, pDwgPlate, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
				pResult->pOrgPart = pDwgPlate;
				pResult->pLoftPart = pLoftPlate;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//单张DWG进行校审时，不需要显示漏号件
	//for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	//{
	//	if (pPart->cPartType != BOMPART::PLATE)
	//		continue;
	//	if(pDwgFile->FindPlateByPartNo(pPart->sPartNo))
	//		continue;
	//	//3、存在BOM构件，不存在DWG构件
	//	COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
	//	pResult->pOrgPart = NULL;
	//	pResult->pLoftPart = pPart;
	//}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("BOM钢板信息和DWG钢板信息相同!");
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
		for (size_t i = 0; i < g_xUbomModel.m_xBomTitleArr.size(); i++)
		{
			if(g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_NOTES))
				continue;	//备注不显示
			if(iCompareType== COMPARE_PLATE_DWG &&(
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ANGLE)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_PUSH_FLAT)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_BER)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ROOT)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_KAI_JIAO)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_HE_JIAO)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_FOO_NAIL)))
				continue;	//钢板不显示角钢工艺
			str_arr.Add(g_xUbomModel.m_xBomTitleArr[i].m_sTitle);
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
			for (size_t ii = 0; ii < g_xUbomModel.m_xBomTitleArr.size(); ii++)
			{
				if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_NOTES))
					continue;	//备注不显示
				BOOL bDiff = FALSE;
				if (pResult->hashBoolByPropName.GetValue(g_xUbomModel.m_xBomTitleArr[ii].m_sKey))
					bDiff = TRUE;
				if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_PN))
				{	//件号
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sPartNo));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_SPEC))
				{	//规格
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sSpec));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(pResult->pLoftPart->sSpec));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MAT))
				{	//材质
					map.SetValueAt(iRow, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_LEN))
				{	//长度
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_SING_N))
				{	//单基数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->GetPartNum()));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->GetPartNum()));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MANU_N))
				{	//加工数
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->feature1));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->feature1));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MANU_W))
				{	//加工重量
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->fSumWeight)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->fSumWeight)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_WELD))
				{	//焊接
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->bWeldPart ? COleVariant("*") : COleVariant(""));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_ZHI_WAN))
				{	//制弯
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				}
				else if (pResult->pOrgPart->cPartType == BOMPART::ANGLE && pResult->pLoftPart->cPartType==BOMPART::ANGLE)
				{
					PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
					PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
					if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_ANGLE))
					{	//切角
						map.SetValueAt(iRow, iCol, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_PUSH_FLAT))
					{	//打扁
						map.SetValueAt(iRow, iCol, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_BER))
					{	//铲背
						map.SetValueAt(iRow, iCol, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_ROOT))
					{	//刨角
						map.SetValueAt(iRow, iCol, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_KAI_JIAO))
					{	//开角
						map.SetValueAt(iRow, iCol, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_HE_JIAO))
					{	//合角
						map.SetValueAt(iRow, iCol, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_FOO_NAIL))
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
	if (iCompareType == COMPARE_ANGLE_DWGS)
		excel_sheet.SetName("角钢DWG缺少构件");
	else
		excel_sheet.SetName("钢板DWG缺少构件");
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
		map.SetValueAt(index,5,COleVariant((long)pResult->pLoftPart->feature1));
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
	if (iCompareType == COMPARE_ANGLE_DWGS || iCompareType == COMPARE_PLATE_DWGS)
		nSheet = 1;
	if (iCompareType == COMPARE_ANGLE_DWG || iCompareType == COMPARE_PLATE_DWG)
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
		if (pDwgFile->IsJgDwgInfo())
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
		if (!pDwgFile->IsJgDwgInfo())
			continue;
		pAngleInfo = pDwgFile->FindAngleByPartNo(sPartNo);
		if (pAngleInfo)
			break;
	}
	return pAngleInfo;
}
//////////////////////////////////////////////////////////////////////////
//CBomModel
UINT CBomModel::m_uiCustomizeSerial = 0;
CXhChar50 CBomModel::m_sCustomizeName;
BOOL CBomModel::m_bExeRppWhenArxLoad = TRUE;
CXhChar200 CBomModel::m_sTMABomFileKeyStr;
CXhChar200 CBomModel::m_sERPBomFileKeyStr;
CXhChar200 CBomModel::m_sTMABomSheetName;
CXhChar200 CBomModel::m_sERPBomSheetName;
const char* CBomModel::KEY_PN			= "PartNo";
const char* CBomModel::KEY_MAT			= "Material";
const char* CBomModel::KEY_SPEC			= "Spec";
const char* CBomModel::KEY_LEN			= "Length";
const char* CBomModel::KEY_WIDE			= "Width";
const char* CBomModel::KEY_SING_N		= "SingNum";
const char* CBomModel::KEY_MANU_N		= "ManuNum";
const char* CBomModel::KEY_MANU_W		= "SumWeight";
const char* CBomModel::KEY_WELD			= "Weld";
const char* CBomModel::KEY_ZHI_WAN		= "ZhiWan";
const char* CBomModel::KEY_CUT_ANGLE	= "CutAngle";
const char* CBomModel::KEY_CUT_ROOT		= "CutRoot";
const char* CBomModel::KEY_CUT_BER		= "CutBer";
const char* CBomModel::KEY_PUSH_FLAT	= "PushFlat";
const char* CBomModel::KEY_KAI_JIAO		= "KaiJiao";
const char* CBomModel::KEY_HE_JIAO		= "HeJiao";
const char* CBomModel::KEY_FOO_NAIL		= "FootNail";
const char* CBomModel::KEY_NOTES		= "Notes";
CBomModel::CBomModel(void)
{
	m_dwFunctionFlag = 0;
	CProjectTowerType* pProject = m_xPrjTowerTypeList.Add(0);
	pProject->m_sProjName.Copy("新建工程");
}
CBomModel::~CBomModel(void)
{
	
}
bool CBomModel::IsValidFunc(int iFuncType)
{
	if (iFuncType == CBomModel::FUNC_BOM_COMPARE)
		return (GetSingleWord(CBomModel::FUNC_BOM_COMPARE)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_BOM_AMEND)
		return (GetSingleWord(CBomModel::FUNC_BOM_AMEND)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_COMPARE)
		return (GetSingleWord(CBomModel::FUNC_DWG_COMPARE)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_NUM)
		return (GetSingleWord(CBomModel::FUNC_DWG_AMEND_NUM)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_WEIGHT)
		return (GetSingleWord(CBomModel::FUNC_DWG_AMEND_WEIGHT)&m_dwFunctionFlag) > 0;
	else
		return false;
}
DWORD CBomModel::AddFuncType(int iFuncType)
{
	DWORD dwFlag = 0;
	if (iFuncType == CBomModel::FUNC_BOM_COMPARE)
		dwFlag = GetSingleWord(CBomModel::FUNC_BOM_COMPARE);
	else if (iFuncType == CBomModel::FUNC_BOM_AMEND)
		dwFlag = GetSingleWord(CBomModel::FUNC_BOM_AMEND);
	else if (iFuncType == CBomModel::FUNC_DWG_COMPARE)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_COMPARE);
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_NUM)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_AMEND_NUM);
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_WEIGHT)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_AMEND_WEIGHT);
	m_dwFunctionFlag |= dwFlag;
	return m_dwFunctionFlag;
}
void CBomModel::InitBomTblCfg()
{
	char file_name[MAX_PATH] = "", line_txt[MAX_PATH] = "", key_word[100] = "";
	GetAppPath(file_name);
	strcat(file_name, "ubom.cfg");
	FILE *fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText, line_txt);
		CString sLine(line_txt);
		sLine.Replace('=', ' ');
		sLine.Replace('\n', ' ');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		if (_stricmp(key_word, "CLIENT_ID") == 0)
		{
			skey = strtok(NULL, "=,;");
			CBomModel::m_uiCustomizeSerial = atoi(skey);
		}
		else if (_stricmp(key_word, "CLIENT_NAME") == 0)
		{
			skey = strtok(NULL, "=,;");
			CBomModel::m_sCustomizeName.Copy(skey);
		}
		else if (_stricmp(key_word, "FUNC_FLAG") == 0)
		{
			skey = strtok(NULL, "=,;");
			sscanf(skey, "%X", &m_dwFunctionFlag);
		}
		else if (_stricmp(key_word, "TMA_BOM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 0)
			{
				m_xTmaTblCfg.m_sColIndexArr.Copy(skey);
				m_xTmaTblCfg.m_sColIndexArr.Replace(" ", "");
				//读取列数
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xTmaTblCfg.m_nColCount = atoi(skey);
				//内容起始行
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xTmaTblCfg.m_nStartRow = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "ERP_BOM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 0)
			{
				m_xErpTblCfg.m_sColIndexArr.Copy(skey);
				m_xErpTblCfg.m_sColIndexArr.Replace(" ", "");
				//读取列数
				skey = strtok(NULL, ";");
				if(skey != NULL)
					m_xErpTblCfg.m_nColCount = atoi(skey);
				//内容起始行
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xErpTblCfg.m_nStartRow = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "ExeRppWhenArxLoad") == 0)
		{
			skey = strtok(NULL, "=,;");
			if(skey!=NULL)
				CBomModel::m_bExeRppWhenArxLoad = atoi(skey);
		}
		else if (_stricmp(key_word, "TMABomFileKeyStr") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				CBomModel::m_sTMABomFileKeyStr.Copy(skey);
				CBomModel::m_sTMABomFileKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomFileKeyStr") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				CBomModel::m_sERPBomFileKeyStr.Copy(skey);
				CBomModel::m_sERPBomFileKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "TMABomSheetName") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				CBomModel::m_sTMABomSheetName.Copy(skey);
				CBomModel::m_sTMABomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomSheetName") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				CBomModel::m_sERPBomSheetName.Copy(skey);
				CBomModel::m_sERPBomSheetName.Remove(' ');
			}
		}
	}
	fclose(fp);
}
CDwgFileInfo *CBomModel::FindDwgFile(const char* file_path)
{
	CDwgFileInfo *pDwgFile = NULL;
	for (CProjectTowerType *pPrjTowerType = m_xPrjTowerTypeList.GetFirst(); pPrjTowerType; pPrjTowerType = m_xPrjTowerTypeList.GetNext())
	{
		pDwgFile = pPrjTowerType->FindDwgBomInfo(file_path);
		if (pDwgFile)
			break;
	}
	return pDwgFile;
}
int CBomModel::InitBomTitle()
{
	CHashStrList<DWORD> hashColIndex;
	m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
	m_xBomTitleArr.clear();
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_PN, "件号", 75));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_SPEC, "规格", 70));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_MAT, "材质", 53));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_LEN, "长度", 50));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_SING_N, "单基数", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_N, "加工数", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_W, "加工重量", 65));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_WELD, "焊接", 44));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_ZHI_WAN, "制弯", 44));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) ||
		hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT))
	{	//角钢工艺信息
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_ANGLE, "切角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_PUSH_FLAT, "打扁", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_BER, "铲背", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_ROOT, "刨角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_KAI_JIAO, "开角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_HE_JIAO, "合角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_FOO_NAIL, "带脚钉", 50));
	}
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_NOTES, "备注", 150));
	//计算初始宽度
	int nInitWidth = 250;
	for (size_t i = 0; i < m_xBomTitleArr.size(); i++)
		nInitWidth += m_xBomTitleArr[i].m_nWidth;
	return min(nInitWidth, 760);
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
#endif