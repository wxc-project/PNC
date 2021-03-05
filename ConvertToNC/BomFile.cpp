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
	//1.�����ļ����ؼ����жϱ�����
	char sFileName[MAX_PATH] = "";
	_splitpath(file_path, NULL, NULL, sFileName, NULL);
	if (m_sFileTypeKeyStr.GetLength() > 0)
	{	//�������õĹؼ��֣��ж��Ƿ�ΪTMA�� wht 20-04-29
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
	{	//2.���ݱ�ͷʶ���ļ�����
		if (m_xTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			if (bDisplayMsgBox)
				AfxMessageBox("û�ж���BOM������,��ȡʧ��!");
			else
				logerr.Log("û�ж���BOM������,��ȡʧ��!");
			return FALSE;
		}
		//
		CHashStrList<DWORD> hashColIndex;
		m_xTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(file_path, sheetContentMap, 1, 52))
			return FALSE;
		BOOL bValid = FALSE;
		for (TitleColPtr pColS=CBomTblTitleCfg::ColIterS(); pColS != CBomTblTitleCfg::ColIterE(); pColS++)
		{
			DWORD* pColIndex = hashColIndex.GetValue(pColS->second);
			if (pColIndex == NULL)
				continue;
			VARIANT value;
			for (int iRow = 0; iRow < 10; iRow++)
			{
				sheetContentMap.GetValueAt(iRow, *pColIndex, value);
				if (value.vt == VT_EMPTY)
					continue;
				CString str = VariantToString(value);
				str.Remove('\n');
				if (!bValid && CBomTblTitleCfg::IsMatchTitle(pColS->first, str))
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
	if (hashColIndex.GetNodeNum() < 3)
	{
		logerr.Log("�ϵ�û�����ö�ȡ����Ϣ,��鿴�����ļ�!");
		return FALSE;
	}
	BOOL bValid = FALSE;
	for (TitleColPtr pColS = CBomTblTitleCfg::ColIterS(); pColS != CBomTblTitleCfg::ColIterE(); pColS++)
	{
		DWORD* pColIndex = hashColIndex.GetValue(pColS->second);
		if (pColIndex == NULL)
			continue;
		VARIANT value;
		for (int iRow = 0; iRow < 10; iRow++)
		{
			sheetContentMap.GetValueAt(iRow, *pColIndex, value);
			if (value.vt == VT_EMPTY)
				continue;
			CString str= VariantToString(value);
			str.Remove('\n');
			if (!bValid && CBomTblTitleCfg::IsMatchTitle(pColS->first, str))
			{
				bValid = TRUE;
				break;
			}
		}
		if (!bValid)
			return FALSE;
	}
	//2.��ȡExcel���е�Ԫ���ֵ
	CStringArray repeatPartLabelArr;
	CHashStrList<BOMPART*> hashBomPartByRepeatLabel;
	int nRowCount = sheetContentMap.RowsCount();
	for(int i=iStartRow;i<= nRowCount;i++)
	{
		VARIANT value;
		int nSingleNum = 0, nProcessNum = 0, nTaNum = 0, nLsNum = 0, nPnSumNum = 0, nLsSumNum = 0;
		double fLength = 0, fWeight = 0, fSumWeight = 0;
		CXhChar100 sPartNo, sMaterial, sSpec, sNote, sReplaceSpec, sValue;
		CXhChar100 sSingleNum, sProcessNum, sTaNum;
		BOOL bCutAngle = FALSE, bCutRoot = FALSE, bCutBer = FALSE, bPushFlat = FALSE;
		BOOL bKaiJiao = FALSE, bHeJiao = FALSE, bWeld = FALSE, bZhiWan = FALSE, bFootNail = FALSE;
		BOOL bSynchroParseMatSpec = FALSE;
		short siSubType = 0;
		//����
		DWORD *pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_PART_NO));
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.GetLength()<2)
			continue;
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_MATERIAL));
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		sMaterial = VariantToString(value);
		//���
		int cls_id=0;
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_SPEC));
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if (sMaterial.Equal(sSpec))
		{	//����&�����ͬһ����Ԫ����
			bSynchroParseMatSpec = TRUE;
		}
		sSpec.Replace("��", "*");
		sSpec.Replace("x", "*");
		sSpec.Replace("X", "*");
		if (strstr(sSpec, "L") || strstr(sSpec, "��"))
		{
			cls_id = BOMPART::ANGLE;
			if (strstr(sSpec, "��"))
				sSpec.Replace("��", "L");
		}
		else if (strstr(sSpec, "-"))
		{
			cls_id = BOMPART::PLATE;
			if (strstr(sSpec, "*"))
			{
				char *skey = strtok((char*)sSpec, "*");
				sSpec.Copy(skey);
			}
		}
		else if ((strstr(sSpec, "��") || strstr(sSpec, "��")) && strstr(sSpec, "*"))
			cls_id = BOMPART::TUBE;
		else
		{
			cls_id = BOMPART::ACCESSORY;
			if (strstr(sSpec, "/"))
				siSubType = BOMPART::SUB_TYPE_TUBE_WIRE;
			else if (strstr(sSpec, "��") || strstr(sSpec, "��"))
				siSubType = BOMPART::SUB_TYPE_ROUND;
			else if (strstr(sSpec, "C"))
				siSubType = BOMPART::SUB_TYPE_STAY_ROPE;
			else if (strstr(sSpec, "G"))
				siSubType = BOMPART::SUB_TYPE_STEEL_GRATING;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_PARTTYPE));
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
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_REPLACE_SPEC));
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sReplaceSpec = VariantToString(value);
		}
		//����
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_LEN));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fLength = (float)atof(VariantToString(value));
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_TA_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sTaNum = VariantToString(value);
			if (sTaNum.GetLength() > 0)
				nTaNum = atoi(sTaNum);
		}
		//������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_SING_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sSingleNum = VariantToString(value);
			if(sSingleNum.GetLength()>0)
				nSingleNum = atoi(sSingleNum);
		}
		//�ӹ���
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_MANU_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sProcessNum = VariantToString(value);
			if(sProcessNum.GetLength()>0)
				nProcessNum = atoi(sProcessNum);
		}
		//��˨��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_LS_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nLsNum = atoi(VariantToString(value));
		}
		//�ӹ�����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_SUM_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nPnSumNum = atoi(VariantToString(value));
		}
		//��˨����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_LS_SUM_NUM));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nLsSumNum = atoi(VariantToString(value));
		}
		//��������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_SING_WEIGHT));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		//�ӹ�����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_MANU_WEIGHT));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fSumWeight = atof(VariantToString(value));
		}
		//��ע
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_NOTES));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sNote = VariantToString(value);
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_WELD));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				strstr(sValue, "��"))
				bWeld = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_ZHI_WAN));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsZhiWan(sValue))
				bZhiWan = TRUE;
		}
		//�н�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_CUT_ANGLE));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutAngle(sValue))
				bCutAngle = TRUE;
		}
		//ѹ��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_PUSH_FLAT));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsPushFlat(sValue))
				bPushFlat = TRUE;
		}
		//�ٸ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_CUT_ROOT));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutRoot(sValue))
				bCutRoot = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_CUT_BER));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsCutBer(sValue))
				bCutBer = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_KAI_JIAO));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsKaiJiao(sValue))
				bKaiJiao = TRUE;
		}
		//�Ͻ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_HE_JIAO));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsHeJiao(sValue))
				bHeJiao = TRUE;
		}
		//���Ŷ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::I_FOOT_NAIL));
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (g_xBomCfg.IsEqualDefaultProcessFlag(sValue) ||
				g_xBomCfg.IsEqualProcessFlag(sValue) ||
				g_xBomCfg.IsFootNail(sValue))
				bFootNail = TRUE;
		}
		sMaterial.Remove(' ');	//�Ƴ��ո� wht 20-07-30
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
				repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, pBomPart->nSumPart));
			}
			repeatPartLabelArr.Add(CXhChar100("%s\t\t\t%d", (char*)sPartNo, atoi(sProcessNum)));
		}
		//
		double fWidth = 0, fThick = 0;
		if (bSynchroParseMatSpec)
		{	//ͬ����������������Ϣ
			CXhChar100 sMatSpec = sMaterial, sMat;
			CProcessPart::RestoreSpec(sMatSpec, &fWidth, &fThick, sMat);
			if (sMat.GetLength() > 0)
			{
				sMat.Remove(' ');
				sSpec.Replace(sMat, "");
				sSpec.Remove(' ');
				sMaterial = sMat;
			}
			else
				sMaterial.Copy("Q235");
			if (fWidth > 0 && fThick > 0 && fThick > fWidth)
			{
				double fValue = fThick;
				fThick = fWidth;
				fWidth = fValue;
			}
		}
		else
			CProcessPart::RestoreSpec(sSpec, &fWidth, &fThick);
		if (fWidth <= 0 && fThick <= 0)
			continue;	//�쳣����
		if (pBomPart == NULL)
		{
			pBomPart = m_hashPartByPartNo.Add(sPartNo, cls_id);
			pBomPart->siSubType = siSubType;
			pBomPart->SetPartNum(nSingleNum);
			pBomPart->nSumPart = nProcessNum;
			pBomPart->nTaNum = nTaNum;
			pBomPart->nMSumLs = nLsNum;
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
			pBomPart->feature1 = nPnSumNum;
			pBomPart->feature2 = nLsSumNum;
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
					if (strstr(pBomPart->sNotes, "���Ͻ�"))
					{	//��Ҫ��һ�������ǿ��ǻ��ǺϽ�

					}
					else if (strstr(pBomPart->sNotes, "����"))
						pBomJg->bKaiJiao = TRUE;
					else if (strstr(pBomPart->sNotes, "�Ͻ�"))
						pBomJg->bHeJiao = TRUE;
				}
				if (!bFootNail)
				{
					if (strstr(pBomJg->sNotes, "���Ŷ�"))
						pBomJg->bHasFootNail = TRUE;
				}
				if (bPushFlat)
					pBomJg->nPushFlat = 1;
			}
			else if (cls_id == BOMPART::PLATE)
			{
				PART_PLATE* pBomPlate = (PART_PLATE*)pBomPart;
				pBomPlate->bWeldPart = bWeld;
				//�ְ���ͳһ�ú�ȱ�ʾ,��������� wht 20-07-30
				pBomPlate->sSpec.Printf("-%.f", fThick);
			}
		}
		else
		{	//�����ظ����ӹ������ۼӼ��� wht 19-09-15
			pBomPart->nSumPart += nProcessNum;
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
BOOL CBomFile::ImportExcelFile(BOM_FILE_CFG *pTblCfg)
{
	if (m_sFileName.GetLength() <= 0 || pTblCfg == NULL)
		return FALSE;
	DisplayCadProgress(0, "��ȡExcel�������.....");
	//1����ָ���ļ�
	DisplayCadProgress(10);
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
	{
		DisplayCadProgress(100);
		return FALSE;
	}
	//2����ȡ��������Ϣ
	DisplayCadProgress(30);
	CHashStrList<DWORD> hashColIndexByColTitle;
	pTblCfg->m_xTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = pTblCfg->m_xTblCfg.m_nStartRow - 1;
	//3����ȡָ��sheetҳ����ȡsheet���ݣ���������
	BOOL bReadOK = FALSE;
	if (pTblCfg->m_sBomSheetName.GetLength() > 0)	
	{	//�����ļ���ָ����sheet���ر�
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
	{	//�����ļ���û��ָ��sheet���ر�
		int nSheetNum = excelobj.GetWorkSheetCount(), nValidSheetCount = 0;
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{
			DisplayCadProgress(70 + iSheet);
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet, 30);
			if (ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow))
				nValidSheetCount++;
		}
		bReadOK = nValidSheetCount > 0 ? TRUE : FALSE;
	}
	DisplayCadProgress(100);
	if (!bReadOK)
		logerr.Log("ȱ�ٹؼ���(���Ż������ʻ򵥻���)!");
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
CBomConfig::CBomConfig(void)
{
	m_sProcessDescArr[TYPE_ZHI_WAN].Copy("����|���|����|");
	m_sProcessDescArr[TYPE_CUT_ANGLE].Copy("�н�|��֫|");
	m_sProcessDescArr[TYPE_CUT_ROOT].Copy("���|�ٸ�|����|��о|");
	m_sProcessDescArr[TYPE_CUT_BER].Copy("����|");
	m_sProcessDescArr[TYPE_PUSH_FLAT].Copy("ѹ��|���|�ı�|");
	m_sProcessDescArr[TYPE_KAI_JIAO].Copy("����");
	m_sProcessDescArr[TYPE_HE_JIAO].Copy("�Ͻ�");
	m_sProcessDescArr[TYPE_FOO_NAIL].Copy("���Ŷ�|�Ŷ�");
}
CBomConfig::~CBomConfig(void)
{
	
}
void CBomConfig::Init()
{
	//TMA
	m_xTmaTblCfg.m_sFileTypeKeyStr.Empty();
	m_xTmaTblCfg.m_sBomSheetName.Empty();
	m_xTmaTblCfg.m_xTblCfg.m_sColIndexArr.Empty();
	//ERP
	m_xErpTblCfg.m_sFileTypeKeyStr.Empty();
	m_xErpTblCfg.m_sBomSheetName.Empty();
	m_xErpTblCfg.m_xTblCfg.m_sColIndexArr.Empty();
	//PRINT
	m_xPrintTblCfg.m_sFileTypeKeyStr.Empty();
	m_xPrintTblCfg.m_sBomSheetName.Empty();
	m_xPrintTblCfg.m_xTblCfg.m_sColIndexArr.Empty();
	//
	m_sAngleCompareItemArr.Empty();
	m_sPlateCompareItemArr.Empty();
	//
	for (TitleColPtr iter=CBomTblTitleCfg::ColIterS(); iter!=CBomTblTitleCfg::ColIterE(); iter++)
	{
		hashCompareItemOfAngle[iter->first] = FALSE;
		hashCompareItemOfPlate[iter->first] = FALSE;
	}
}
BOOL CBomConfig::IsEqualDefaultProcessFlag(const char* sValue)
{
	if (sValue == NULL || strlen(sValue) <= 0)
		return FALSE;
	else if (stricmp(sValue, "*") == 0 || stricmp(sValue, "v") == 0 || stricmp(sValue, "V") == 0 ||
			 stricmp(sValue, "��") == 0 || stricmp(sValue, "1") == 0)
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
	if ((ciType == TYPE_HE_JIAO || ciType == TYPE_KAI_JIAO) &&
		(strstr(sValue, "��") || strstr(sValue, "��")))
	{	//�ӱ�ע�����н������ϽǶ� wht 20-08-20
		BOOL bKaiJiao = strstr(sValue, "����") != NULL;
		BOOL bHeJiao = strstr(sValue, "�Ͻ�") != NULL;
		if (bKaiJiao || bHeJiao)
		{
			if ((bKaiJiao && ciType == TYPE_KAI_JIAO) ||
				(bHeJiao && ciType == TYPE_HE_JIAO))
				return TRUE;
			else
				return FALSE;
		}
		CXhChar500 sTempValue;	//+4.5�㡢-4.5���ù��캯������ᶪʧ��
		sTempValue.Copy(sValue);
		for (char *skey = strtok((char*)sTempValue, " ,����"); skey; skey = strtok(NULL, " ,����"))
		{
			if (strlen(skey) <= 0)
				continue;
			BOOL bHasPlus = strstr(skey, "+") != NULL;
			BOOL bHasMinus = strstr(skey, "-") != NULL;
			if (bHasPlus || bHasMinus)
			{
				if ((bHasPlus && ciType == TYPE_KAI_JIAO) ||
					(bHasMinus && ciType == TYPE_HE_JIAO))
					return TRUE;	//��ע+4.5�㡢-4.5��
				else
					return FALSE;
			}
			else
			{	//��ע95�� 84��,���ݽǶ�ʶ�𿪽ǻ��ǺϽ� wht 20-08-20 
				CString sKaiHeJiao = skey;
				sKaiHeJiao.Replace("��", "");
				sKaiHeJiao.Replace("��", "");
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
		for (TitleColPtr iter = CBomTblTitleCfg::ColIterS(); iter != CBomTblTitleCfg::ColIterE(); iter++)
		{
			if (CBomTblTitleCfg::IsMatchTitle(iter->first, skey))
			{
				hashCompareItemOfAngle[iter->first] = TRUE;
				break;
			}
		}
	}
	return hashCompareItemOfAngle[CBomTblTitleCfg::I_PART_NO]==TRUE;
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
		for (TitleColPtr iter = CBomTblTitleCfg::ColIterS(); iter != CBomTblTitleCfg::ColIterE(); iter++)
		{
			if (CBomTblTitleCfg::IsMatchTitle(iter->first, skey))
			{
				hashCompareItemOfPlate[iter->first] = TRUE;
				break;
			}
		}
	}
	return hashCompareItemOfPlate[CBomTblTitleCfg::I_PART_NO]==TRUE;
}
int CBomConfig::InitBomTitle()
{
	std::map<int, int> mapRealCol;
	m_xTmaTblCfg.m_xTblCfg.GetRealColIndex(mapRealCol);
	//��ʼ����ʾ��
	m_xBomTitleArr.clear();
	if (mapRealCol[CBomTblTitleCfg::I_PART_NO] >= 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_PART_NO, 75));
	if (mapRealCol[CBomTblTitleCfg::I_SPEC] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_SPEC, 70));
	if (mapRealCol[CBomTblTitleCfg::I_MATERIAL] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_MATERIAL, 50));
	if (mapRealCol[CBomTblTitleCfg::I_LEN] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_LEN, 50));
	if (mapRealCol[CBomTblTitleCfg::I_TA_NUM] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_TA_NUM, 50));
	if (mapRealCol[CBomTblTitleCfg::I_SING_NUM] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_SING_NUM, 52));
	if (mapRealCol[CBomTblTitleCfg::I_MANU_NUM] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_MANU_NUM, 52));
	if (mapRealCol[CBomTblTitleCfg::I_SING_WEIGHT] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_SING_WEIGHT, 50));
	if (mapRealCol[CBomTblTitleCfg::I_MANU_WEIGHT] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_MANU_WEIGHT, 65));
	if (mapRealCol[CBomTblTitleCfg::I_LS_NUM] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_LS_NUM, 50));
	if (mapRealCol[CBomTblTitleCfg::I_WELD] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_WELD, 40));
	if (mapRealCol[CBomTblTitleCfg::I_ZHI_WAN] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_ZHI_WAN, 40));
	if (mapRealCol[CBomTblTitleCfg::I_CUT_ANGLE] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_CUT_ANGLE, 40));
	if (mapRealCol[CBomTblTitleCfg::I_PUSH_FLAT] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_PUSH_FLAT, 40));
	if (mapRealCol[CBomTblTitleCfg::I_CUT_BER] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_CUT_BER, 40));
	if (mapRealCol[CBomTblTitleCfg::I_CUT_ROOT] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_CUT_ROOT, 40));
	if (mapRealCol[CBomTblTitleCfg::I_KAI_JIAO] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_KAI_JIAO, 40));
	if (mapRealCol[CBomTblTitleCfg::I_HE_JIAO] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_HE_JIAO, 40));
	if (mapRealCol[CBomTblTitleCfg::I_FOOT_NAIL] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_FOOT_NAIL, 50));
	if (mapRealCol[CBomTblTitleCfg::I_NOTES] > 0)
		m_xBomTitleArr.push_back(BOM_TITLE(CBomTblTitleCfg::I_NOTES, 150));
	//��ʼ���Ǹ�У����Ŀ
	std::map<int, int>::iterator iterS, iterE = mapRealCol.end();
	if (!ExtractAngleCompareItems())
	{	//������û������У���Ĭ�ϰ��ն�ȡ�н���У��
		for (iterS = mapRealCol.begin(); iterS != iterE; iterS++)
			hashCompareItemOfAngle[iterS->first] = (iterS->second > 0) ? TRUE : FALSE;
	}
	//��ʼ���ְ�У����Ŀ
	if (!ExtractPlateCompareItems())
	{	//������û������У���Ĭ�ϰ��ն�ȡ�н���У��
		for (iterS = mapRealCol.begin(); iterS != iterE; iterS++)
			hashCompareItemOfAngle[iterS->first] = (iterS->second > 0) ? TRUE : FALSE;
	}
	//�����ʼ���
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

BOOL CBomConfig::IsTitleCol(int index, int iCfgCol)
{
	if (index < 0 || index >= (int)m_xBomTitleArr.size())
		return FALSE;
	return m_xBomTitleArr[index].m_iCfgCol == iCfgCol;
}
CXhChar16 CBomConfig::GetTitleName(int index)
{
	if (index < 0 || index >= (int)GetBomTitleCount())
		return CXhChar16();
	else
	{
		int iCfgCol = m_xBomTitleArr[index].m_iCfgCol;
		return CXhChar16(CBomTblTitleCfg::GetColName(iCfgCol));
	}
}
CXhChar16 CBomConfig::GetCfgColName(int iCfgCol)
{
	if (iCfgCol < CBomTblTitleCfg::GetColNum())
		return CXhChar16(CBomTblTitleCfg::GetColName(iCfgCol));
	else
		return CXhChar16();
}
int CBomConfig::GetTitleWidth(int index)
{
	if (index < 0 || index >= (int)GetBomTitleCount())
		return 50;
	else
		return m_xBomTitleArr[index].m_nWidth;
}
#endif