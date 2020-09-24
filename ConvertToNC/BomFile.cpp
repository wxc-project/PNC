#include "StdAfx.h"
#include "BomFile.h"
#include "ExcelOper.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"
#include "ProcessPart.h"
#include "CadToolFunc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#ifdef __UBOM_ONLY_
CBomConfig g_xBomCfg;
//////////////////////////////////////////////////////////////////////////
// BOM_FILE_CFG
BOOL BOM_FILE_CFG ::IsCurFormat(const char* file_path, BOOL bDisplayMsgBox)
{
	if (file_path == NULL)
		return FALSE;
	//1.根据文件名关键字判断表单类型
	char sFileName[MAX_PATH] = "";
	_splitpath(file_path, NULL, NULL, sFileName, NULL);
	if (m_sFileTypeKeyStr.GetLength() > 0)
	{	//根据设置的关键字，判断是否为TMA表单 wht 20-04-29
		CXhChar200 sCurFileName(sFileName);
		CXhChar200 sKeyStr(m_sFileTypeKeyStr);
		sCurFileName.ToUpper();
		sKeyStr.ToUpper();
		if (strstr(sCurFileName, sKeyStr) != NULL)
			return TRUE;
		else
			return FALSE;
	}
	else
	{	//2.根据表头识别文件类型
		if (m_xTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if (bDisplayMsgBox)
				AfxMessageBox("没有定制BOM数据列,读取失败!");
			else
				logerr.Log("没有定制BOM数据列,读取失败!");
			return FALSE;
		}
		//
		CHashStrList<DWORD> hashColIndex;
		m_xTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(file_path, sheetContentMap, 1, 52))
			return FALSE;

		VARIANT value;
		BOOL bValid = FALSE;
		for (int iCol = 0; iCol < CBomTblTitleCfg::T_COL_COUNT; iCol++)
		{
			CXhChar50 sCol(CBomTblTitleCfg::GetColName(iCol));
			DWORD* pColIndex = hashColIndex.GetValue(sCol);
			if (pColIndex == NULL)
				continue;
			bValid = FALSE;
			for (int iRow = 0; iRow < 10; iRow++)
			{
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
		return bValid;
	}
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
//////////////////////////////////////////////////////////////////////////
//CBomFile
CBomFile::CBomFile()
{
	m_hashPartByPartNo.CreateNewAtom = CreateBomPart;
	m_hashPartByPartNo.DeleteAtom = DeleteBomPart;
	m_pBelongObj = NULL;
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
	BOOL bHasPartNo = FALSE, bHasMat = FALSE, bHasSpec = FALSE, bValid = FALSE;
	int iContentRow = 0, nColNum = sheetContentMap.ColsCount();
	for (int iRow = 0; iRow < 10; iRow++)
	{
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			VARIANT value;
			sheetContentMap.GetValueAt(iRow, iCol, value);
			if (value.vt == VT_EMPTY)
				continue;
			CString str = VariantToString(value);
			str.Remove('\n');
			if (str.GetLength() > 99)
				continue;	//标题行
			if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_PART_NO, str))
			{
				bHasPartNo = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_PART_NO));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_MATERIAL, str))
			{
				bHasMat = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_MATERIAL));
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
			if(bHasPartNo && bHasMat && bHasSpec)
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
	CHashStrList<BOMPART*> hashBomPartByRepeatLabel;
	int nRowCount = sheetContentMap.RowsCount();
	for(int i=iStartRow;i<= nRowCount;i++)
	{
		VARIANT value;
		int nSingleNum = 0, nProcessNum = 0;
		double fLength = 0, fWeight = 0, fSumWeight = 0;
		CXhChar100 sPartNo, sMaterial, sSpec, sNote, sReplaceSpec, sValue;
		CXhChar100 sSingleNum, sProcessNum;
		BOOL bCutAngle = FALSE, bCutRoot = FALSE, bCutBer = FALSE, bPushFlat = FALSE;
		BOOL bKaiJiao = FALSE, bHeJiao = FALSE, bWeld = FALSE, bZhiWan = FALSE, bFootNail = FALSE;
		short siSubType = 0;
		//件号
		DWORD *pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.GetLength()<2)
			continue;
		//材质
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MATERIAL);
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
		{
			cls_id = BOMPART::TUBE;
			CXhChar100 sTemp(sSpec);
			int hits = sTemp.Replace("φ", " ");
			hits += sTemp.Replace("Φ", " ");
			hits += sTemp.Replace("/", " ");
			if (hits == 2)
				siSubType = BOMPART::SUB_TYPE_TUBE_WIRE;
		}
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
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				strstr(sValue, "焊"))
				bWeld = TRUE;
		}
		//制弯
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsZhiWan(sValue))
				bZhiWan = TRUE;
		}
		//切角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutAngle(sValue))
				bCutAngle = TRUE;
		}
		//压扁
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsPushFlat(sValue))
				bPushFlat = TRUE;
		}
		//刨根
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutRoot(sValue))
				bCutRoot = TRUE;
		}
		//铲背
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutBer(sValue))
				bCutBer = TRUE;
		}
		//开角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_KAI_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsKaiJiao(sValue))
				bKaiJiao = TRUE;
		}
		//合角
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_HE_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsHeJiao(sValue))
				bHeJiao = TRUE;
		}
		//带脚钉
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_FOOT_NAIL);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsFootNail(sValue))
				bFootNail = TRUE;
		}
		sMaterial.Remove(' ');	//移除空格 wht 20-07-30
		//材质、规格、单基数、加工数同时为空时，此行为无效行
		if (sMaterial.GetLength() <= 0 && sSpec.GetLength() <= 0 &&
			sSingleNum.GetLength() <= 0 && sProcessNum.GetLength() <= 0)
			continue;	//当前行为无效行，跳过此行 wht 20-03-05
		//填充哈希表
		if(sMaterial.GetLength()<=0 && sSpec.GetLength()<=0)
			continue;	//异常数据
		BOMPART* pBomPart = NULL;
		if (pBomPart = m_hashPartByPartNo.GetValue(sPartNo))
		{
			if (hashBomPartByRepeatLabel.GetValue(sPartNo) == NULL)
			{
				hashBomPartByRepeatLabel.SetValue(sPartNo, pBomPart);
				//添加sPartNo对应的第一个构件 wht 20-05-28
				repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, pBomPart->feature1));
			}
			repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, atoi(sProcessNum)));
		}
		//logerr.Log("存在重复件号：%s", (char*)sPartNo);
		double fWidth = 0, fThick = 0;
		CProcessPart::RestoreSpec(sSpec, &fWidth, &fThick);
		if (pBomPart == NULL)
		{
			pBomPart = m_hashPartByPartNo.Add(sPartNo, cls_id);
			pBomPart->siSubType = siSubType;
			pBomPart->SetPartNum(nSingleNum);
			pBomPart->feature1 = nProcessNum;
			pBomPart->sPartNo.Copy(sPartNo);
			pBomPart->sSpec.Copy(sSpec);
			pBomPart->sMaterial = sMaterial;
			pBomPart->cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			pBomPart->cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
			pBomPart->length = fLength;
			pBomPart->fPieceWeight = fWeight;
			pBomPart->fSumWeight = fSumWeight;
			pBomPart->wide = fWidth;
			pBomPart->thick = fThick;
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
				pBomJg->bHasFootNail = bFootNail;
				if (!bHeJiao && !bKaiJiao)
				{
					if (strstr(pBomPart->sNotes, "开合角"))
					{	//需要进一步区分是开角还是合角

					}
					else if (strstr(pBomPart->sNotes, "开角"))
						pBomJg->bKaiJiao = TRUE;
					else if (strstr(pBomPart->sNotes, "合角"))
						pBomJg->bHeJiao = TRUE;
				}
				if (!bFootNail)
				{
					if (strstr(pBomJg->sNotes, "带脚钉"))
						pBomJg->bHasFootNail = TRUE;
				}
				if (bPushFlat)
					pBomJg->nPushFlat = 1;
			}
			else if (cls_id == BOMPART::PLATE)
			{
				PART_PLATE* pBomPlate = (PART_PLATE*)pBomPart;
				pBomPlate->bWeldPart = bWeld;
				//钢板规格统一用厚度表示,不包括宽度 wht 20-07-30
				pBomPlate->sSpec.Printf("-%.f", fThick);
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
BOOL CBomFile::ImportExcelFile(BOM_FILE_CFG *pTblCfg)
{
	if (m_sFileName.GetLength() <= 0 || pTblCfg == NULL)
		return FALSE;
	DisplayCadProgress(0, "读取Excel表格数据.....");
	//1、打开指定文件
	DisplayCadProgress(10);
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
	{
		DisplayCadProgress(100);
		return FALSE;
	}
	//2、获取定制列信息
	DisplayCadProgress(30);
	CHashStrList<DWORD> hashColIndexByColTitle;
	pTblCfg->m_xTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = pTblCfg->m_xTblCfg.m_nStartRow - 1;
	//3、获取指定sheet页，读取sheet内容，解析数据
	BOOL bReadOK = FALSE;
	if (pTblCfg->m_sBomSheetName.GetLength() > 0)	
	{	//配置文件中指定了sheet加载表单
		DisplayCadProgress(50);
		ARRAY_LIST<int> sheetIndexList;
		CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, pTblCfg->m_sBomSheetName, sheetIndexList);
		for (int *pSheetIndex = sheetIndexList.GetFirst(); pSheetIndex; pSheetIndex = sheetIndexList.GetNext())
		{
			DisplayCadProgress(70 + (*pSheetIndex));
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, *pSheetIndex, 52);
			int iCurIndex = sheetIndexList.GetCurrentIndex();
			if (iCurIndex > 0 && iCurIndex <= 10 && pTblCfg->m_xArrTblCfg[iCurIndex - 1].m_sColIndexArr.GetLength() > 0)
			{
				CHashStrList<DWORD> hashTempColIndexByColTitle;
				pTblCfg->m_xArrTblCfg[iCurIndex - 1].GetHashColIndexByColTitleTbl(hashTempColIndexByColTitle);
				if (ParseSheetContent(sheetContentMap, hashTempColIndexByColTitle, iStartRow))
					bReadOK = TRUE;
			}
			else
			{
				if (ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow))
					bReadOK = TRUE;
			}
		}
	}
	else
	{	//配置文件中没有指定sheet加载表单
		int nSheetNum = excelobj.GetWorkSheetCount(), nValidSheetCount = 0;
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{
			DisplayCadProgress(70 + iSheet);
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet);
			if (ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow))
				nValidSheetCount++;
		}
		bReadOK = nValidSheetCount > 0 ? TRUE : FALSE;
	}
	DisplayCadProgress(100);
	if (!bReadOK)
		logerr.Log("缺少关键列(件号或规格或材质或单基数)!");
	return bReadOK;
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
//CBomImportCfg
const char* CBomConfig::KEY_PN			= "PartNo";
const char* CBomConfig::KEY_MAT			= "Material";
const char* CBomConfig::KEY_SPEC		= "Spec";
const char* CBomConfig::KEY_LEN			= "Length";
const char* CBomConfig::KEY_WIDE		= "Width";
const char* CBomConfig::KEY_SING_N		= "SingNum";
const char* CBomConfig::KEY_MANU_N		= "ManuNum";
const char* CBomConfig::KEY_SING_W		= "SingWeight";
const char* CBomConfig::KEY_MANU_W		= "SumWeight";
const char* CBomConfig::KEY_WELD		= "Weld";
const char* CBomConfig::KEY_ZHI_WAN		= "ZhiWan";
const char* CBomConfig::KEY_CUT_ANGLE	= "CutAngle";
const char* CBomConfig::KEY_CUT_ROOT	= "CutRoot";
const char* CBomConfig::KEY_CUT_BER		= "CutBer";
const char* CBomConfig::KEY_PUSH_FLAT	= "PushFlat";
const char* CBomConfig::KEY_KAI_JIAO	= "KaiJiao";
const char* CBomConfig::KEY_HE_JIAO		= "HeJiao";
const char* CBomConfig::KEY_FOO_NAIL	= "FootNail";
const char* CBomConfig::KEY_NOTES		= "Notes";
CBomConfig::CBomConfig(void)
{
	m_sProcessDescArr[TYPE_ZHI_WAN].Copy("火曲|卷边|制弯|");
	m_sProcessDescArr[TYPE_CUT_ANGLE].Copy("切角|切肢|");
	m_sProcessDescArr[TYPE_CUT_ROOT].Copy("清根|刨根|铲心|铲芯|");
	m_sProcessDescArr[TYPE_CUT_BER].Copy("铲背|");
	m_sProcessDescArr[TYPE_PUSH_FLAT].Copy("压扁|打扁|拍扁|");
	m_sProcessDescArr[TYPE_KAI_JIAO].Copy("开角");
	m_sProcessDescArr[TYPE_HE_JIAO].Copy("合角");
	m_sProcessDescArr[TYPE_FOO_NAIL].Copy("带脚钉|脚钉");
}
CBomConfig::~CBomConfig(void)
{
	
}

BOOL CBomConfig::IsEqualDefaultProcessFlag(const char* sValue)
{
	if (sValue == NULL || strlen(sValue) <= 0)
		return FALSE;
	else if (stricmp(sValue, "*") == 0 || stricmp(sValue, "v") == 0 || stricmp(sValue, "V") == 0 ||
			 stricmp(sValue, "√") == 0 || stricmp(sValue, "1") == 0)
		return TRUE;
	else
		return FALSE;
}

BOOL CBomConfig::IsEqualProcessFlag(const char* sValue)
{
	if (sValue == NULL || strlen(sValue) <= 0 || m_sProcessFlag.GetLength() <= 0)
		return FALSE;
	else if (m_sProcessFlag.EqualNoCase(sValue))
		return TRUE;
	else
		return FALSE;
}
BOOL CBomConfig::IsHasTheProcess(const char* sValue, BYTE ciType)
{
	if (sValue==NULL || strlen(sValue) < 2)
		return FALSE;
	if (ciType == TYPE_FOO_NAIL)
	{	//
		if (strstr(sValue, "脚钉") && strstr(sValue, "无脚钉") == NULL 
			&& strstr(sValue, "不带脚钉") == NULL)
			return TRUE;
	}
	//工艺判断
	CXhChar500 sText(m_sProcessDescArr[ciType]);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL, "|"))
	{
		if (strlen(skey) <= 0)
			continue;
		if (strstr(sValue, skey))
			return TRUE;
	}
	if ((ciType == TYPE_HE_JIAO || ciType == TYPE_KAI_JIAO) &&
		(strstr(sValue, "°") || strstr(sValue, "度")))
	{	//从备注文字中解析开合角度 wht 20-08-20
		CXhChar500 sTempValue;	//+4.5°、-4.5°用构造函数传入会丢失°
		sTempValue.Copy(sValue);
		for (char *skey = strtok((char*)sTempValue, " ,，、"); skey; skey = strtok(NULL, " ,，、"))
		{
			if (strlen(skey) <= 0)
				continue;
			BOOL bHasPlus = strstr(skey, "+") != NULL;
			BOOL bHasMinus = strstr(skey, "-") != NULL;
			if (bHasPlus || bHasMinus)
			{
				if ((bHasPlus && ciType == TYPE_KAI_JIAO) ||
					(bHasMinus && ciType == TYPE_HE_JIAO))
					return TRUE;	//标注+4.5°、-4.5°
				else
					return FALSE;
			}
			else
			{	//标注95° 84°,根据角度识别开角还是合角 wht 20-08-20 
				CString sKaiHeJiao = skey;
				sKaiHeJiao.Replace("°", "");
				sKaiHeJiao.Replace("度", "");
				sKaiHeJiao.Replace("+", "");
				sKaiHeJiao.Replace("-", "");
				double fAngle = atof(sKaiHeJiao);
				if ((fAngle > 0 && fAngle < 90 && ciType==TYPE_HE_JIAO) ||
					(fAngle>90 && fAngle <180 && ciType==TYPE_KAI_JIAO))
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

bool CBomConfig::ExtractAngleCompareItems()
{
	if (m_sAngleCompareItemArr.GetLength() <= 0)
		return false;
	CXhChar500 sText(m_sAngleCompareItemArr);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL, "|"))
	{
		if (strlen(skey) <= 0)
			continue;
		if (strstr(skey, "件号"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PART_NO, TRUE);
		else if (strstr(skey, "材质"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		else if (strstr(skey, "规格"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		else if (strstr(skey, "单基数"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		else if (strstr(skey, "加工数"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		else if (strstr(skey, "长度"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		else if (strstr(skey, "焊接"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		else if (strstr(skey, "制弯"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
		else if (strstr(skey, "切角"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ANGLE, TRUE);
		else if (strstr(skey, "刨根"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ROOT, TRUE);
		else if (strstr(skey, "铲背"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_BER, TRUE);
		else if (strstr(skey, "打扁"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PUSH_FLAT, TRUE);
		else if (strstr(skey, "开角"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_KAI_JIAO, TRUE);
		else if (strstr(skey, "合角"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_HE_JIAO, TRUE);
		else if (strstr(skey, "脚钉"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_FOOT_NAIL, TRUE);
	}
	return hashCompareItemOfAngle.GetNodeNum() > 0;
}
bool CBomConfig::ExtractPlateCompareItems()
{
	if (m_sPlateCompareItemArr.GetLength() <= 0)
		return false;
	CXhChar500 sText(m_sPlateCompareItemArr);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL, "|"))
	{
		if (strlen(skey) <= 0)
			continue;
		if (strstr(skey, "件号"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_PART_NO, TRUE);
		else if (strstr(skey, "材质"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		else if (strstr(skey, "规格"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		else if (strstr(skey, "单基数"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		else if (strstr(skey, "加工数"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		else if (strstr(skey, "长度"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		else if (strstr(skey, "宽度"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_WIDE, TRUE);
		else if (strstr(skey, "焊接"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		else if (strstr(skey, "制弯"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
	}
	return hashCompareItemOfPlate.GetNodeNum() > 0;
}
int CBomConfig::InitBomTitle()
{
	CHashStrList<DWORD> hashColIndex;
	m_xTmaTblCfg.m_xTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
	//初始化显示列
	m_xBomTitleArr.clear();
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_PN, "件号", 75));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_SPEC, "规格", 70));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_MAT, "材质", 53));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_LEN, "长度", 50));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_SING_N, "单基数", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_N, "加工数", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_WEIGHT))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_SING_W, "单重", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_W, "总重", 65));
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
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_ROOT, "刨根", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_KAI_JIAO, "开角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_HE_JIAO, "合角", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_FOO_NAIL, "带脚钉", 50));
	}
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_NOTES, "备注", 150));
	//初始化角钢校审项目
	if (!ExtractAngleCompareItems())
	{	//配置中没有设置校审项，默认按照读取列进行校审
		hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PART_NO, TRUE);
		hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MANU_WEIGHT, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) ||
			hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT))
		{	//角钢工艺信息
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ANGLE, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PUSH_FLAT, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_BER, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ROOT, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_KAI_JIAO, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_HE_JIAO, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_FOOT_NAIL, TRUE);
		}
	}
	//初始化钢板校审项目
	if (!ExtractPlateCompareItems())
	{	//配置中没有设置校审项，默认按照读取列进行校审
		hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_PART_NO, TRUE);
		hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MANU_WEIGHT, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
	}
	//计算初始宽度
	int nInitWidth = 250;
	for (size_t i = 0; i < m_xBomTitleArr.size(); i++)
		nInitWidth += m_xBomTitleArr[i].m_nWidth;
	return min(nInitWidth, 760);
}

BOOL CBomConfig::IsTmaBomFile(const char* sFilePath, BOOL bDisplayMsgBox /*= FALSE*/)
{
	return m_xTmaTblCfg.IsCurFormat(sFilePath, bDisplayMsgBox);
}
BOOL CBomConfig::IsErpBomFile(const char* sFilePath, BOOL bDisplayMsgBox /*= FALSE*/)
{
	return m_xErpTblCfg.IsCurFormat(sFilePath, bDisplayMsgBox);
}
BOOL CBomConfig::IsPrintBomFile(const char* sFilePath, BOOL bDisplayMsgBox /*= FALSE*/)
{
	return m_xPrintTblCfg.IsCurFormat(sFilePath, bDisplayMsgBox);
}

BOOL CBomConfig::IsTitleCol(int index, const char*title_key)
{
	if (index < 0 || index >= (int)m_xBomTitleArr.size() || title_key ==NULL)
		return FALSE;
	return m_xBomTitleArr[index].m_sKey.Equal(title_key);
}
CXhChar16 CBomConfig::GetTitleKey(int index)
{
	if (index < 0 || index >= (int)GetBomTitleCount())
		return CXhChar16();
	else
		return m_xBomTitleArr[index].m_sKey;
}
CXhChar16 CBomConfig::GetTitleName(int index)
{
	if (index < 0 || index >= (int)GetBomTitleCount())
		return CXhChar16();
	else
		return m_xBomTitleArr[index].m_sTitle;
}
int CBomConfig::GetTitleWidth(int index)
{
	if (index < 0 || index >= (int)GetBomTitleCount())
		return 50;
	else
		return m_xBomTitleArr[index].m_nWidth;
}
#endif