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
		logerr.Log("%s�����Ҳ���",sOldKey);
		return;
	}
	pPart=m_hashPartByPartNo.ModifyKeyStr(sOldKey,sNewKey);
	if (pPart)
		pPart->sPartNo.Copy(sNewKey);
}
//����BOMSHEET����
BOOL CBomFile::ParseSheetContent(CVariant2dArray &sheetContentMap,CHashStrList<DWORD>& hashColIndex,int iStartRow)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	CLogErrorLife logLife;
	//1���ж����Ƿ�����Ҫ��
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
			CString str(VariantToString(value));
			str.Remove('\n');
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
	//2.��ȡExcel���е�Ԫ���ֵ
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
		//����
		DWORD *pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.GetLength()<2)
			continue;
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MATERIAL);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		sMaterial = VariantToString(value);
		//���
		int cls_id=0;
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if (strstr(sSpec, "L") || strstr(sSpec, "��"))
		{
			cls_id = BOMPART::ANGLE;
			if (strstr(sSpec, "��"))
				sSpec.Replace("��", "L");
			if (strstr(sSpec, "��"))
				sSpec.Replace("��", "*");
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
		else //if(strstr(sSpec,"��"))
			cls_id = BOMPART::TUBE;
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PARTTYPE);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			CXhChar100 sPartType = VariantToString(value);
			if (strstr(sPartType, "L") || strstr(sPartType, "��") || strstr(sPartType, "�Ǹ�"))
				cls_id = BOMPART::ANGLE;
			else if (strstr(sPartType, "-") || strstr(sPartType, "�ְ�"))
				cls_id = BOMPART::PLATE;
			else
				cls_id = BOMPART::TUBE;
			//
			if (cls_id == BOMPART::ANGLE && strstr(sSpec, "L") == NULL)
			{
				sSpec.InsertBefore('L');
				if (strstr(sSpec, "��"))
					sSpec.Replace("��", "*");
				if (strstr(sSpec, "x"))
					sSpec.Replace("x", "*");
			}
			if (cls_id == BOMPART::PLATE && strstr(sSpec, "-") == NULL)
				sSpec.InsertBefore('-');
		}
		//���ù��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_REPLACE_SPEC);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sReplaceSpec = VariantToString(value);
		}
		//����
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_LEN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fLength = (float)atof(VariantToString(value));
		}
		//������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sSingleNum = VariantToString(value);
			if(sSingleNum.GetLength()>0)
				nSingleNum = atoi(sSingleNum);
		}
		//�ӹ���
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sProcessNum = VariantToString(value);
			if(sProcessNum.GetLength()>0)
				nProcessNum = atoi(sProcessNum);
		}
		//��������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		//�ӹ�����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fSumWeight = atof(VariantToString(value));
		}
		//��ע
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sNote = VariantToString(value);
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_WELD);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || strstr(sValue, "��"))
				bWeld = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsZhiWan(sValue))
				bZhiWan = TRUE;
		}
		//�н�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsCutAngle(sValue))
				bCutAngle = TRUE;
		}
		//ѹ��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsPushFlat(sValue))
				bPushFlat = TRUE;
		}
		//�ٸ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsCutRoot(sValue))
				bCutRoot = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsCutBer(sValue))
				bCutBer = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_KAI_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsKaiJiao(sValue))
				bKaiJiao = TRUE;
		}
		//�Ͻ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_HE_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsHeJiao(sValue))
				bHeJiao = TRUE;
		}
		//���Ŷ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_FOOT_NAIL);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*") || g_xUbomModel.IsFootNail(sValue))
				bFootNail = TRUE;
		}
		//���ʡ���񡢵��������ӹ���ͬʱΪ��ʱ������Ϊ��Ч��
		if (sMaterial.GetLength() <= 0 && sSpec.GetLength() <= 0 &&
			sSingleNum.GetLength() <= 0 && sProcessNum.GetLength() <= 0)
			continue;	//��ǰ��Ϊ��Ч�У��������� wht 20-03-05
		//����ϣ��
		if(sMaterial.GetLength()<=0 && sSpec.GetLength()<=0)
			continue;	//�쳣����
		BOMPART* pBomPart = NULL;
		if (pBomPart = m_hashPartByPartNo.GetValue(sPartNo))
		{
			if (hashBomPartByRepeatLabel.GetValue(sPartNo) == NULL)
			{
				hashBomPartByRepeatLabel.SetValue(sPartNo, pBomPart);
				//���sPartNo��Ӧ�ĵ�һ������ wht 20-05-28
				repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, pBomPart->feature1));
			}
			repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, atoi(sProcessNum)));
		}
			//logerr.Log("�����ظ����ţ�%s", (char*)sPartNo);
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
				pBomJg->bHasFootNail = bFootNail;
				if (strstr(pBomPart->sNotes, "����"))
					pBomJg->bKaiJiao = TRUE;
				if (strstr(pBomPart->sNotes, "�Ͻ�"))
					pBomJg->bHeJiao = TRUE;
				if (strstr(pBomJg->sNotes, "���Ŷ�"))
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
		{	//�����ظ����ӹ������ۼӼ��� wht 19-09-15
			pBomPart->feature1 += nProcessNum;
		}
	}
	if (repeatPartLabelArr.GetSize() > 0)
	{	//��ʾ�û������ظ����ţ��������ۼ�ͳ�� wht 20-03-05
		logerr.Log("�ļ�����%s\n", (char*)m_sFileName);
		logerr.Log("�����ظ����ţ��ӹ��������ۼӼ��㣩��\n");
		logerr.Log("����\t\t\t�ӹ���\n");
		for (int i = 0; i < repeatPartLabelArr.GetSize(); i++)
			logerr.Log(repeatPartLabelArr[i]);
	}
	return TRUE;
}
//��������ϵ�EXCEL�ļ�
BOOL CBomFile::ImportTmaExcelFile()
{
	if(m_sFileName.GetLength()<=0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	//��ȡ��������Ϣ
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xTmaTblCfg.m_nStartRow-1;
	//��������
	int nSheetNum = excelobj.GetWorkSheetCount();
	int nValidSheetCount = 0, iValidSheet = (nSheetNum == 1) ? 1 : 0;
	BOOL bRetCode = FALSE;
	m_hashPartByPartNo.Empty();
	int iCfgSheetIndex = 0;
	if (g_xUbomModel.m_sTMABomSheetName.GetLength() > 0)	//���������ļ���ָ����sheet���ر�
	{
		iCfgSheetIndex = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sTMABomSheetName);
		if (iCfgSheetIndex > 0)
			iValidSheet = iCfgSheetIndex;
	}
	if (iCfgSheetIndex==0 && g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_QingDao_HaoMai)	//�ൺ������ȡ����ԭʼ���ϱ�
		iValidSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, "����ԭʼ���ϱ�");
	if (iValidSheet > 0)
	{	//���ȶ�ȡָ��sheet
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iValidSheet, 52);
		if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
		{
			nValidSheetCount++;
			bRetCode = TRUE;
		}
	}
	if (!bRetCode)
	{
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{	//2����������
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet, 52);
			if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
				nValidSheetCount++;
		}
	}
	if (nValidSheetCount == 0)
	{
		logerr.Log("ȱ�ٹؼ���(���Ż������ʻ򵥻���)!");
		return FALSE;
	}
	else
		return TRUE;
}
//����ERP�ϵ�EXCEL�ļ�
BOOL CBomFile::ImportErpExcelFile()
{
	if (m_sFileName.GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	//��ȡsheet����
	int nSheetNum = excelobj.GetWorkSheetCount();
	int iValidSheet = (nSheetNum >= 1) ? 1 : 0;
	if (g_xUbomModel.m_sERPBomSheetName.GetLength() > 0)	//���������ļ���ָ����sheet���ر�
		iValidSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, g_xUbomModel.m_sERPBomSheetName);
	//��ȡExcel���ݴ洢��sheetContentMap��
	//���֧��52�У��������������ʱ����������ñ���ɫ�ᵼ���Զ���ȡ�����������󣬼���Excel�ٶ��� wht 19-12-30
	CVariant2dArray sheetContentMap(1, 1);
	if (!CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iValidSheet,52))
		return false;
	//��ȡ��������Ϣ,�����б�����������ӳ���hashColIndexByColTitle
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xErpTblCfg.m_nStartRow-1;
	//��������
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
//��ȡ�ϵ������ļ�
void CProjectTowerType::ReadProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath,FALSE);
	double version=1.0;
	file.Read(&version,sizeof(double));
	file.ReadString(m_sProjName);
	CXhChar100 sValue;
	file.ReadString(sValue);	//TMA�ϵ�
	InitBomInfo(sValue,TRUE);
	file.ReadString(sValue);	//ERP�ϵ�
	InitBomInfo(sValue,FALSE);
	//DWG�ļ���Ϣ
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
//�����ϵ������ļ�
void CProjectTowerType::WriteProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath,TRUE);
	double version=1.0;
	file.Write(&version,sizeof(double));
	file.WriteString(m_sProjName);
	file.WriteString(m_xLoftBom.m_sFileName);	//TMA�ϵ�
	file.WriteString(m_xOrigBom.m_sFileName);	//ERP�ϵ�
	//DWG�ļ���Ϣ
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
	{	//�������õĹؼ��֣��ж��Ƿ�ΪTMA�� wht 20-04-29
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
	{	//���պ�Դ�ϵ��ļ��к��йؼ���
		if (strstr(sFileName, "TMA") || strstr(sFileName, "tma"))
			return TRUE;
		else
			return FALSE;
	}
	else
	{	//���ݶ����н���ʶ��
		if (m_xLoftBom.GetPartNum() > 0)
			return FALSE;	//���������Ѷ�ȡ
		if (g_xUbomModel.m_xTmaTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if (bDisplayMsgBox)
				AfxMessageBox("û�ж���BOM������,��ȡʧ��!");
			else
				logerr.Log("û�ж���BOM������,��ȡʧ��!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1, 52))
			return FALSE;
		BOOL bValid = FALSE;
		for (int iCol = 0; iCol < CBomTblTitleCfg::T_COL_COUNT; iCol++)
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
	{	//�������õĹؼ��֣��ж��Ƿ�ΪTMA�� wht 20-04-29
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
			return TRUE;	//���պ�Դ�ϵ��ļ��к��йؼ���
		else
			return FALSE;
	}
	else
	{	//���ݶ����н���ʶ��
		if (m_xOrigBom.GetPartNum() > 0)
			return FALSE;	//���������Ѷ�ȡ
		if (g_xUbomModel.m_xErpTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if(bDisplayMsgBox)
				AfxMessageBox("û�ж���BOM������,��ȡʧ��!");
			else
				logerr.Log("û�ж���BOM������,��ȡʧ��!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1, 52))
			return FALSE;
		BOOL bValid = FALSE;
		for (int iCol = 0; iCol < CBomTblTitleCfg::T_COL_COUNT; iCol++)
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
//��ʼ��BOM��Ϣ
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
//��ӽǸ�DWGBOM��Ϣ
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName, BOOL bJgDxf)
{
	if (strlen(sFileName) <= 0)
		return FALSE;
	//��DWG�ļ�
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
	if (strstr(file_path, sFileName))	//����ָ���ļ�
		acDocManager->activateDocument(pDoc);
	else
	{		//��ָ���ļ�
#ifdef _ARX_2007
		acDocManager->appContextOpenDocument(_bstr_t(sFileName));
#else
		acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	//��ȡDWG�ļ���Ϣ
	//CWaitCursor wait;
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
		//��ȡDWG��Ϣ
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
	hashBoolByPropName.Empty();
	//���
	if(pSrcPart->cPartType!=pDesPart->cPartType||
		stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue(CBomModel::KEY_SPEC, TRUE);
	//����
	if(toupper(pSrcPart->cMaterial) != toupper(pDesPart->cMaterial))
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	else if(pSrcPart->cMaterial!=pDesPart->cMaterial && !g_pncSysPara.m_bEqualH_h)
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	//�����ȼ�
	if (g_pncSysPara.m_bCmpQualityLevel && pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue(CBomModel::KEY_MAT, TRUE);
	//�Ǹֶ���У����
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		//������
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
			hashBoolByPropName.SetValue(CBomModel::KEY_SING_N, TRUE);
		//�ӹ���
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_MANU_NUM) &&
			pSrcPart->feature1 != pDesPart->feature1)
			if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_AnHui_HongYuan)
				hashBoolByPropName.SetValue(CBomModel::KEY_MANU_N, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomModel::KEY_MANU_W, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomModel::KEY_WELD, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomModel::KEY_ZHI_WAN, TRUE);
		//���ȶԱ�
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_LEN))
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
			{	//���ջ���Ҫ��ͼֽ���ȴ��ڷ�������
				if (pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue(CBomModel::KEY_LEN, TRUE);
			}
			else
			{	//�ж���ֵ�Ƿ����
				if (fabsl(pSrcPart->length - pDesPart->length) > g_pncSysPara.m_fMaxLenErr)
					hashBoolByPropName.SetValue(CBomModel::KEY_LEN, TRUE);
			}
		}
		//�н�
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) &&
			pSrcJg->bCutAngle != pDesJg->bCutAngle)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_ANGLE, TRUE);
		//ѹ��
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_PUSH_FLAT) &&
			pSrcJg->nPushFlat != pDesJg->nPushFlat)
			hashBoolByPropName.SetValue(CBomModel::KEY_PUSH_FLAT, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_CUT_BER) &&
			pSrcJg->bCutBer != pDesJg->bCutBer)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_BER, TRUE);
		//�ٸ�
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_CUT_ROOT) &&
			pSrcJg->bCutRoot != pDesJg->bCutRoot)
			hashBoolByPropName.SetValue(CBomModel::KEY_CUT_ROOT, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_KAI_JIAO) &&
			pSrcJg->bKaiJiao != pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue(CBomModel::KEY_KAI_JIAO, TRUE);
		//�Ͻ�
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_HE_JIAO) &&
			pSrcJg->bHeJiao != pDesJg->bHeJiao)
			hashBoolByPropName.SetValue(CBomModel::KEY_HE_JIAO, TRUE);
		//�Ŷ�
		if (g_xUbomModel.hashCompareItemOfAngle.GetValue(CBomTblTitleCfg::T_FOOT_NAIL) &&
			pSrcJg->bHasFootNail != pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue(CBomModel::KEY_FOO_NAIL, TRUE);
	}
	//�ְ嶨��У����
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::PLATE)
	{
		PART_PLATE* pSrcJg = (PART_PLATE*)pSrcPart;
		PART_PLATE* pDesJg = (PART_PLATE*)pDesPart;
		//������
		if (g_xUbomModel.hashCompareItemOfPlate.GetValue(CBomTblTitleCfg::T_SING_NUM) &&
			pSrcPart->GetPartNum() != pDesPart->GetPartNum())
		{	//�ൺ�������Ƚϵ�����
			if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_QingDao_HaoMai)
				hashBoolByPropName.SetValue(CBomModel::KEY_SING_N, TRUE);
		}
		//�ӹ���
		if (g_xUbomModel.hashCompareItemOfPlate.GetValue(CBomTblTitleCfg::T_MANU_NUM) &&
			pSrcPart->feature1 != pDesPart->feature1)
		{	//���պ�Դ���Ƚϼӹ���������Ҫ�����ӹ���
			if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_AnHui_HongYuan)
				hashBoolByPropName.SetValue(CBomModel::KEY_MANU_N, TRUE);
		}
		//����
		if (g_xUbomModel.hashCompareItemOfPlate.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT) &&
			fabs(pSrcPart->fSumWeight - pDesPart->fSumWeight) > 0)
			hashBoolByPropName.SetValue(CBomModel::KEY_MANU_W, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfPlate.GetValue(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue(CBomModel::KEY_WELD, TRUE);
		//����
		if (g_xUbomModel.hashCompareItemOfPlate.GetValue(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue(CBomModel::KEY_ZHI_WAN, TRUE);
	}
}
//�ȶ�BOM��Ϣ 0.��ͬ 1.��ͬ 2.�ļ�����
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS=0.5;
	m_hashCompareResultByPartNo.Empty();
	if(m_xLoftBom.GetPartNum()<=0)
	{
		AfxMessageBox("ȱ�ٷ���BOM��Ϣ!");
		return 2;
	}
	if(m_xOrigBom.GetPartNum()<=0)
	{
		AfxMessageBox("ȱ�ٹ��տ�BOM��Ϣ");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for(BOMPART *pLoftPart=m_xLoftBom.EnumFirstPart();pLoftPart;pLoftPart=m_xLoftBom.EnumNextPart())
	{	
		BOMPART *pOrgPart = m_xOrigBom.FindPart(pLoftPart->sPartNo);
		if (pOrgPart == NULL)
		{	//1.���ڷ���������������ԭʼ����
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
			pResult->pOrgPart=NULL;
			pResult->pLoftPart=pLoftPart;
		}
		else 
		{	//2. �Ա�ͬһ���Ź�������
			hashBoolByPropName.Empty();
			CompareData(pLoftPart, pOrgPart, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
				pResult->pOrgPart=pOrgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 ���������ԭʼ��,�����Ƿ���©�����
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
		AfxMessageBox("����BOM�͹��տ�BOM��Ϣ��ͬ!");
		return 0;
	}
	else
		return 1;
}
//���нǸ�DWG�ļ���©�ż��
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
		AfxMessageBox("�Ѵ򿪽Ǹ�DWG�ļ�û��©�����!");
		return 0;
	}
	else
		return 1;
}
//���иְ�DWG�ļ���©�ż��
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
		AfxMessageBox("�Ѵ򿪸ְ�DWG�ļ�û��©�����!");
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
		AfxMessageBox("ȱ��BOM��Ϣ!");
		return 2;
	}
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		AfxMessageBox("δ�ҵ�ָ���ĽǸ�DWG�ļ�!");
		return 2;
	}
	//���бȶ�
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CAngleProcessInfo* pJgInfo = NULL;
	for (pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = pDwgFile->EnumNextJg())
	{
		BOMPART *pDwgJg = &(pJgInfo->m_xAngle);
		BOMPART *pLoftJg=m_xLoftBom.FindPart(pJgInfo->m_xAngle.sPartNo);
		if(pLoftJg==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pJgInfo->m_xAngle.sPartNo);
			pResult->pOrgPart = pDwgJg;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			CompareData(pLoftJg, pDwgJg, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgJg->sPartNo);
				pResult->pOrgPart= pDwgJg;
				pResult->pLoftPart=pLoftJg;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//����DWG����У��ʱ��������©�ż��
	//for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	//{
	//	if (pPart->cPartType!=BOMPART::ANGLE)
	//		continue;
	//	if (pDwgFile->FindAngleByPartNo(pPart->sPartNo))
	//		continue;
	//	//3������BOM������������DWG����
	//	COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
	//	pResult->pOrgPart = NULL;
	//	pResult->pLoftPart = pPart;
	//}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("BOM�Ǹ���Ϣ��DWG�Ǹ���Ϣ��ͬ!");
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
		AfxMessageBox("ȱ��BOM��Ϣ!");
		return 2;
	}
	CDwgFileInfo* pDwgFile=FindDwgBomInfo(sFileName);
	if(pDwgFile==NULL)
	{
		AfxMessageBox("δ�ҵ�ָ���ĸְ�DWG�ļ�!");
		return 2;
	}
	//�������ݱȶ�
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CPlateProcessInfo* pPlateInfo = NULL;
	for (pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
	{
		PART_PLATE* pDwgPlate = &(pPlateInfo->xBomPlate);
		BOMPART *pLoftPlate=m_xLoftBom.FindPart(pDwgPlate->sPartNo);
		if(pLoftPlate==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
			pResult->pOrgPart = pDwgPlate;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			CompareData(pLoftPlate, pDwgPlate, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
				pResult->pOrgPart = pDwgPlate;
				pResult->pLoftPart = pLoftPlate;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//����DWG����У��ʱ������Ҫ��ʾ©�ż�
	//for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	//{
	//	if (pPart->cPartType != BOMPART::PLATE)
	//		continue;
	//	if(pDwgFile->FindPlateByPartNo(pPart->sPartNo))
	//		continue;
	//	//3������BOM������������DWG����
	//	COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
	//	pResult->pOrgPart = NULL;
	//	pResult->pLoftPart = pPart;
	//}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("BOM�ְ���Ϣ��DWG�ְ���Ϣ��ͬ!");
		return 0;
	}
	else
		return 1;
}
//
void CProjectTowerType::AddCompareResultSheet(LPDISPATCH pSheet, int iSheet, int iCompareType)
{
	//��У������������
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->sPartNo);
		else
			keyStrArr.append(pResult->pOrgPart->sPartNo);
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData, keyStrArr.GetSize(), compare_func);
	//����Excel
	if (iSheet == 1)
	{
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		excel_sheet.SetName("У����");
		CStringArray str_arr;
		for (size_t i = 0; i < g_xUbomModel.m_xBomTitleArr.size(); i++)
		{
			if(g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_NOTES))
				continue;	//��ע����ʾ
			if(iCompareType== COMPARE_PLATE_DWG &&(
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ANGLE)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_PUSH_FLAT)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_BER)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_CUT_ROOT)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_KAI_JIAO)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_HE_JIAO)||
				g_xUbomModel.m_xBomTitleArr[i].m_sKey.Equal(CBomModel::KEY_FOO_NAIL)))
				continue;	//�ְ岻��ʾ�Ǹֹ���
			str_arr.Add(g_xUbomModel.m_xBomTitleArr[i].m_sTitle);
		}
		str_arr.Add("������Դ");
		int nCol = str_arr.GetSize();
		ARRAY_LIST<double> col_arr;
		col_arr.SetSize(nCol);
		for (int i = 0; i < nCol; i++)
			col_arr[i] = 10;
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr.m_pData, TRUE);
		//�������
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
					continue;	//��ע����ʾ
				BOOL bDiff = FALSE;
				if (pResult->hashBoolByPropName.GetValue(g_xUbomModel.m_xBomTitleArr[ii].m_sKey))
					bDiff = TRUE;
				if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_PN))
				{	//����
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sPartNo));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_SPEC))
				{	//���
					map.SetValueAt(iRow, iCol, COleVariant(pResult->pOrgPart->sSpec));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(pResult->pLoftPart->sSpec));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MAT))
				{	//����
					map.SetValueAt(iRow, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_LEN))
				{	//����
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_SING_N))
				{	//������
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->GetPartNum()));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->GetPartNum()));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MANU_N))
				{	//�ӹ���
					map.SetValueAt(iRow, iCol, COleVariant((long)pResult->pOrgPart->feature1));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant((long)pResult->pLoftPart->feature1));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_MANU_W))
				{	//�ӹ�����
					map.SetValueAt(iRow, iCol, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->fSumWeight)));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->fSumWeight)));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_WELD))
				{	//����
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->bWeldPart ? COleVariant("*") : COleVariant(""));
				}
				else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_ZHI_WAN))
				{	//����
					map.SetValueAt(iRow, iCol, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
					if (bDiff)
						map.SetValueAt(iRow + 1, iCol, pResult->pLoftPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				}
				else if (pResult->pOrgPart->cPartType == BOMPART::ANGLE && pResult->pLoftPart->cPartType==BOMPART::ANGLE)
				{
					PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
					PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
					if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_ANGLE))
					{	//�н�
						map.SetValueAt(iRow, iCol, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_PUSH_FLAT))
					{	//���
						map.SetValueAt(iRow, iCol, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_BER))
					{	//����
						map.SetValueAt(iRow, iCol, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_CUT_ROOT))
					{	//�ٽ�
						map.SetValueAt(iRow, iCol, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_KAI_JIAO))
					{	//����
						map.SetValueAt(iRow, iCol, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_HE_JIAO))
					{	//�Ͻ�
						map.SetValueAt(iRow, iCol, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
					}
					else if (g_xUbomModel.m_xBomTitleArr[ii].m_sKey.Equal(CBomModel::KEY_FOO_NAIL))
					{	//���Ŷ�
						map.SetValueAt(iRow, iCol, pOrgJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
						if (bDiff)
							map.SetValueAt(iRow + 1, iCol, pLoftJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
					}
				}
				if (bDiff)
				{	//���ݲ�ͬ��ͨ����ͬ��ɫ���б��
					CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("%C%d", 'A' + iCol, iRow + 2));
					CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("%C%d", 'A' + iCol, iRow + 3));
				}
				iCol += 1;
			}
			//������Դ
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
		str_arr[0] = "����"; str_arr[1] = "���"; str_arr[2] = "����"; str_arr[3] = "����";
		double col_arr[4] = { 15,15,15,15 };
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
		if (iCompareType == COMPARE_BOM_FILE)
		{
			if(iSheet==2)
				excel_sheet.SetName(CXhChar500("%s����", (char*)m_xOrigBom.m_sBomName));
			else
				excel_sheet.SetName(CXhChar100("%s����", (char*)m_xLoftBom.m_sBomName));
		}
		else
			excel_sheet.SetName((iSheet == 2) ? "DWG���" : "DWGȱ��");
		//�������
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
		excel_sheet.SetName("�Ǹ�DWGȱ�ٹ���");
	else
		excel_sheet.SetName("�ְ�DWGȱ�ٹ���");
	//���ñ���
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="�������";str_arr[1]="��ƹ��";str_arr[2]="����";
	str_arr[3]="����";str_arr[4]="������";str_arr[5]="�ӹ���";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//�������
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//��ȡExcel���ķ�Χ
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
//�����ȽϽ��
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
//����ERP�ϵ��м��ţ�����ΪQ420,����ǰ��P������ΪQ345������ǰ��H��
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
		AfxMessageBox("ERP�ϵ��ļ���ʧ��!");
		return FALSE;
	}
	//��ȡExcelָ��Sheet���ݴ洢��sheetContentMap��
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
	//excel_usedRange��������ʱ����һ�У�ԭ��δ֪����ʱ�ڴ˴��������� wht 20-04-24
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
	//����ָ��sheet����
	int iPartNoCol=1,iMartCol=3;
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar16 sPartNo,sMaterial,sNewPartNo;
		//����
		sheetContentMap.GetValueAt(i,iPartNoCol,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//����
		sheetContentMap.GetValueAt(i,iMartCol,value);
		sMaterial =VariantToString(value);
		//���¼�������
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
		//���¼�������
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
	m_uiCustomizeSerial = 0;
	m_bExeRppWhenArxLoad = TRUE;
	CProjectTowerType* pProject = m_xPrjTowerTypeList.Add(0);
	pProject->m_sProjName.Copy("�½�����");
	//
	m_sProcessDescArr[TYPE_ZHI_WAN].Copy("����|���|����|");
	m_sProcessDescArr[TYPE_CUT_ANGLE].Copy("�н�|��֫|");
	m_sProcessDescArr[TYPE_CUT_ROOT].Copy("���|�ٸ�|");
	m_sProcessDescArr[TYPE_CUT_BER].Copy("����|");
	m_sProcessDescArr[TYPE_PUSH_FLAT].Copy("ѹ��|���|");
	m_sProcessDescArr[TYPE_KAI_JIAO].Copy("����");
	m_sProcessDescArr[TYPE_HE_JIAO].Copy("�Ͻ�");
	m_sProcessDescArr[TYPE_FOO_NAIL].Copy("���Ŷ�");
}
CBomModel::~CBomModel(void)
{
	
}
BOOL CBomModel::IsHasTheProcess(const char* sValue, BYTE ciType)
{
	if (strlen(sValue) < 2)
		return FALSE;
	if (ciType == TYPE_FOO_NAIL)
	{	//
		if (strstr(sValue, "�Ŷ�") && strstr(sValue, "�޽Ŷ�") == NULL 
			&& strstr(sValue, "�����Ŷ�") == NULL)
			return TRUE;
	}
	//�����ж�
	CXhChar500 sText(m_sProcessDescArr[ciType]);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL, "|"))
	{
		if (strlen(skey) <= 0)
			continue;
		if (strstr(sValue, skey))
			return TRUE;
	}
	return FALSE;
}
bool CBomModel::IsValidFunc(int iFuncType)
{
	if (iFuncType == CBomModel::FUNC_BOM_COMPARE)
		return (GetSingleWord(CBomModel::FUNC_BOM_COMPARE)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_BOM_AMEND)
		return (GetSingleWord(CBomModel::FUNC_BOM_AMEND)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_COMPARE)
		return (GetSingleWord(CBomModel::FUNC_DWG_COMPARE)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_SUM_NUM)
		return (GetSingleWord(CBomModel::FUNC_DWG_AMEND_SUM_NUM)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_WEIGHT)
		return (GetSingleWord(CBomModel::FUNC_DWG_AMEND_WEIGHT)&m_dwFunctionFlag) > 0;
	else if(iFuncType==CBomModel::FUNC_DWG_AMEND_SING_N)
		return (GetSingleWord(CBomModel::FUNC_DWG_AMEND_SING_N)&m_dwFunctionFlag) > 0;
	else if (iFuncType == CBomModel::FUNC_DWG_BATCH_PRINT)
		return (GetSingleWord(CBomModel::FUNC_DWG_BATCH_PRINT)&m_dwFunctionFlag) > 0;
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
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_SUM_NUM)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_AMEND_SUM_NUM);
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_WEIGHT)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_AMEND_WEIGHT);
	else if (iFuncType == CBomModel::FUNC_DWG_AMEND_SING_N)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_AMEND_SING_N);
	else if (iFuncType == CBomModel::FUNC_DWG_BATCH_PRINT)
		dwFlag = GetSingleWord(CBomModel::FUNC_DWG_BATCH_PRINT);
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
			m_uiCustomizeSerial = atoi(skey);
		}
		else if (_stricmp(key_word, "CLIENT_NAME") == 0)
		{
			skey = strtok(NULL, "=,;");
			m_sCustomizeName.Copy(skey);
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
				//��ȡ����
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xTmaTblCfg.m_nColCount = atoi(skey);
				//������ʼ��
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
				//��ȡ����
				skey = strtok(NULL, ";");
				if(skey != NULL)
					m_xErpTblCfg.m_nColCount = atoi(skey);
				//������ʼ��
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xErpTblCfg.m_nStartRow = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "JG_BOM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 0)
			{
				m_xJgPrintCfg.m_sColIndexArr.Copy(skey);
				m_xJgPrintCfg.m_sColIndexArr.Replace(" ", "");
				//��ȡ����
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xJgPrintCfg.m_nColCount = atoi(skey);
				//������ʼ��
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xJgPrintCfg.m_nStartRow = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "ExeRppWhenArxLoad") == 0)
		{
			skey = strtok(NULL, "=,;");
			if(skey!=NULL)
				m_bExeRppWhenArxLoad = atoi(skey);
		}
		else if (_stricmp(key_word, "TMABomFileKeyStr") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sTMABomFileKeyStr.Copy(skey);
				m_sTMABomFileKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomFileKeyStr") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sERPBomFileKeyStr.Copy(skey);
				m_sERPBomFileKeyStr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "TMABomSheetName") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sTMABomSheetName.Copy(skey);
				m_sTMABomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ERPBomSheetName") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sERPBomSheetName.Copy(skey);
				m_sERPBomSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "JGPrintSheetName") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sJGPrintSheetName.Copy(skey);
				m_sJGPrintSheetName.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ANGLE_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sAngleCompareItemArr.Copy(skey);
				m_sAngleCompareItemArr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PLATE_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sPlateCompareItemArr.Copy(skey);
				m_sPlateCompareItemArr.Remove(' ');
			}
		}
		else if (_stricmp(key_word, "ZHIWAN_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_ZHI_WAN].Copy(skey);
				m_sProcessDescArr[TYPE_ZHI_WAN].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_ANGLE_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_CUT_ANGLE].Copy(skey);
				m_sProcessDescArr[TYPE_CUT_ANGLE].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_ROOT_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_CUT_ROOT].Copy(skey);
				m_sProcessDescArr[TYPE_CUT_ROOT].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "CUT_BER_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_CUT_BER].Copy(skey);
				m_sProcessDescArr[TYPE_CUT_BER].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "PUSH_FLAT_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_PUSH_FLAT].Copy(skey);
				m_sProcessDescArr[TYPE_PUSH_FLAT].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "KAI_JIAO_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_KAI_JIAO].Copy(skey);
				m_sProcessDescArr[TYPE_KAI_JIAO].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "HE_JIAO_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_HE_JIAO].Copy(skey);
				m_sProcessDescArr[TYPE_HE_JIAO].Remove(' ');
			}
		}
		else if (_stricmp(key_word, "FOO_NAIL_ITEM") == 0)
		{
			skey = strtok(NULL, "=,;");
			if (skey != NULL)
			{
				m_sProcessDescArr[TYPE_FOO_NAIL].Copy(skey);
				m_sProcessDescArr[TYPE_FOO_NAIL].Remove(' ');
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
bool CBomModel::ExtractAngleCompareItems()
{
	if (m_sAngleCompareItemArr.GetLength() <= 0)
		return false;
	CXhChar500 sText(m_sAngleCompareItemArr);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL,"|"))
	{
		if(strlen(skey)<=0)
			continue;
		if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PART_NO,TRUE);
		else if(strstr(skey,"����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		else if (strstr(skey, "���"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		else if (strstr(skey, "������"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		else if (strstr(skey, "�ӹ���"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
		else if (strstr(skey, "�н�"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ANGLE, TRUE);
		else if (strstr(skey, "�ٸ�"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ROOT, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_BER, TRUE);
		else if (strstr(skey, "���"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PUSH_FLAT, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_KAI_JIAO, TRUE);
		else if (strstr(skey, "�Ͻ�"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_HE_JIAO, TRUE);
		else if (strstr(skey, "�Ŷ�"))
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_FOOT_NAIL, TRUE);
	}
	return hashCompareItemOfAngle.GetNodeNum() > 0;
}
bool CBomModel::ExtractPlateCompareItems()
{
	if (m_sPlateCompareItemArr.GetLength() <= 0)
		return false;
	CXhChar500 sText(m_sPlateCompareItemArr);
	for (char *skey = strtok((char*)sText, "|"); skey; skey = strtok(NULL, "|"))
	{
		if (strlen(skey) <= 0)
			continue;
		if (strstr(skey, "����"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_PART_NO, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MATERIAL, TRUE);
		else if (strstr(skey, "���"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SPEC, TRUE);
		else if (strstr(skey, "������"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_SING_NUM, TRUE);
		else if (strstr(skey, "�ӹ���"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_MANU_NUM, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_LEN, TRUE);
		else if (strstr(skey, "���"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_WIDE, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_WELD, TRUE);
		else if (strstr(skey, "����"))
			hashCompareItemOfPlate.SetValue(CBomTblTitleCfg::T_ZHI_WAN, TRUE);
	}
	return hashCompareItemOfPlate.GetNodeNum() > 0;
}
int CBomModel::InitBomTitle()
{
	CHashStrList<DWORD> hashColIndex;
	m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
	//��ʼ����ʾ��
	m_xBomTitleArr.clear();
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_PN, "����", 75));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_SPEC, "���", 70));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_MAT, "����", 53));
	m_xBomTitleArr.push_back(BOM_TITLE(KEY_LEN, "����", 50));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_SING_N, "������", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_N, "�ӹ���", 52));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_MANU_W, "�ӹ�����", 65));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_WELD, "����", 44));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_ZHI_WAN, "����", 44));
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) ||
		hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT))
	{	//�Ǹֹ�����Ϣ
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_ANGLE, "�н�", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_PUSH_FLAT, "���", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_BER, "����", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_CUT_ROOT, "�ٽ�", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_KAI_JIAO, "����", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_HE_JIAO, "�Ͻ�", 44));
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_FOO_NAIL, "���Ŷ�", 50));
	}
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES))
		m_xBomTitleArr.push_back(BOM_TITLE(KEY_NOTES, "��ע", 150));
	//��ʼ���Ǹ�У����Ŀ
	if (!ExtractAngleCompareItems())
	{	//������û������У���Ĭ�ϰ��ն�ȡ�н���У��
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
		{	//�Ǹֹ�����Ϣ
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ANGLE, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_PUSH_FLAT, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_BER, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_CUT_ROOT, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_KAI_JIAO, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_HE_JIAO, TRUE);
			hashCompareItemOfAngle.SetValue(CBomTblTitleCfg::T_FOOT_NAIL, TRUE);
		}
	}
	//��ʼ���ְ�У����Ŀ
	if (!ExtractPlateCompareItems())
	{	//������û������У���Ĭ�ϰ��ն�ȡ�н���У��
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
	//�����ʼ���
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