#include "StdAfx.h"
#include "PncModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "DimStyle.h"
#include "ArrayList.h"
#include "PNCCmd.h"
#include "PNCSysPara.h"

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//
BOOL PART_LABEL_DIM::IsPartLabelDim()
{
	if (m_xCirEnt.idCadEnt > 0 && m_xInnerText.idCadEnt > 0 &&
		m_xInnerText.sText != NULL && strlen(m_xInnerText.sText) > 0 &&
		GetSegI().iSeg > 0)
		return TRUE;
	else
		return FALSE;
}
SEGI PART_LABEL_DIM::GetSegI()
{
	SEGI segI;
	if (!ParsePartNo(m_xInnerText.sText, &segI, NULL))
		segI.iSeg = 0;
	return segI;
}
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
CDwgFileInfo::CDwgFileInfo()
{
	m_pProject = NULL;
}
CDwgFileInfo::~CDwgFileInfo()
{

}
BOOL CDwgFileInfo::ImportPrintBomExcelFile(const char* sFileName)
{
	m_xPrintBomFile.Empty();
	m_xPrintBomFile.m_sFileName.Copy(sFileName);
	return m_xPrintBomFile.ImportExcelFile(&g_xBomCfg.m_xPrintTblCfg);
}
//�ְ�DWG����
//�������ݵ�������Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPt(f3dPoint text_pos)
{
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=m_xPncMode.EnumFirstPlate();pPlateInfo;pPlateInfo=m_xPncMode.EnumNextPlate())
	{
		if(pPlateInfo->IsInPartRgn(text_pos))
			break;
	}
	return pPlateInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPartNo(const char* sPartNo)
{
	return m_xPncMode.PartFromPartNo(sPartNo);
}
//���¸ְ�ӹ�����
void CDwgFileInfo::ModifyPlateDwgPartNum()
{
	if(m_xPncMode.GetPlateNum()<=0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ�ӹ�����Ϣ.....");
	for(CPlateProcessInfo* pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pLoftBom ==NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!",(char*)sPartNo);
			continue;
		}
		//�ӹ�����ͬ�����޸�
		if(pInfo->xBomPlate.nSumPart != pLoftBom->nSumPart)
			pInfo->RefreshPlateNum(pLoftBom->nSumPart);
	}
	DisplayCadProgress(100);
}
//���¸ְ���
void CDwgFileInfo::ModifyPlateDwgSpec()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ�����Ϣ.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		if (pInfo->xBomPlate.thick != pLoftBom->thick)
		{	//���ͬ�����޸�
			pInfo->xBomPlate.thick = pLoftBom->thick;	//���
			pInfo->xBomPlate.sSpec = pLoftBom->sSpec;	//���
			pInfo->RefreshPlateSpec();
		}
	}
	DisplayCadProgress(100);
}
//���¸ְ����
void CDwgFileInfo::ModifyPlateDwgMaterial()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���¸ְ������Ϣ.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		if (toupper(pInfo->xBomPlate.cMaterial) != toupper(pLoftBom->cMaterial) ||
			(pInfo->xBomPlate.cMaterial != pLoftBom->cMaterial && !g_xUbomModel.m_bEqualH_h)||
			(pInfo->xBomPlate.cMaterial == 'A' && !pLoftBom->sMaterial.Equal(pInfo->xBomPlate.sMaterial)) ||
			(g_xUbomModel.m_bCmpQualityLevel && pInfo->xBomPlate.cQualityLevel != pLoftBom->cQualityLevel))
		{	//���ʲ�ͬ�����޸�
			pInfo->xBomPlate.cMaterial = pLoftBom->cMaterial;	//����
			pInfo->xBomPlate.sMaterial = pLoftBom->sMaterial;	//����
			pInfo->xBomPlate.cQualityLevel = pLoftBom->cQualityLevel;
			//pInfo->xPlate.cMaterial = pLoftBom->cMaterial;	//����
			//pInfo->xPlate.cQuality = pLoftBom->cQualityLevel;
			pInfo->RefreshPlateMat();
		}
	}
	DisplayCadProgress(100);
}
//���Ƹְ����ͼ������Ϣ���ɶ����������ƣ�
void CDwgFileInfo::FillPlateDwgData()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	std::vector<CPlateProcessInfo*> vectorInvalidPlate;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "���ְ�ӹ�����Ϣ");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�ְ�", (char*)sPartNo);
			vectorInvalidPlate.push_back(pInfo);
			continue;
		}
		if (pLoftBom->nSumPart <= 0)
		{
			logerr.Log("�ϵ������иְ�(%s)�ӹ���Ϊ0", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s�ְ�����޸�ʧ��!", (char*)sPartNo);
			continue;
		}
		//����ְ�ļӹ���
		pInfo->FillPlateNum(pLoftBom->nSumPart, pInfo->partNoId);
		//�����ظ����Ÿְ�
		for (AcDbObjectId objId = pInfo->repeatEntList.GetFirst(); objId; objId = pInfo->repeatEntList.GetNext())
		{
			if (pInfo->partNoId == objId)
				continue;
			pInfo->FillPlateNum(pLoftBom->nSumPart, objId);
		}
	}
	//��DWG�ļ���ɾ������Ҫ������
	for (size_t i = 0; i < vectorInvalidPlate.size(); i++)
	{
		if (!vectorInvalidPlate[i]->IsValid())
		{
			logerr.Log("(%s)�ְ���������ȡʧ��,�޷�ɾ��!", (char*)vectorInvalidPlate[i]->GetPartNo());
			continue;
		}
		//ɾ���øְ�Ĺ���ͼԪ
		vectorInvalidPlate[i]->ExtractRelaEnts();
		vectorInvalidPlate[i]->EraseRelaEnts();
		//ɾ���øøְ�
		m_xPncMode.DeletePlate(vectorInvalidPlate[i]->GetPartNo());
	}
	DisplayCadProgress(100);
}
//��ȡ���������,ȷ���պ�����
BOOL CDwgFileInfo::RetrievePlates(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CPNCModel::m_bSendCommand = TRUE;
	if (bSupportSelectEnts)
	{	//��ȡ�û�ѡ���ͼԪ
		CPNCModel tempMode;
		SmartExtractPlate(&tempMode, TRUE);
		//���ݿ���
		CPlateProcessInfo* pSrcPlate = NULL, *pDestPlate = NULL;
		for (pSrcPlate = tempMode.EnumFirstPlate(); pSrcPlate; pSrcPlate = tempMode.EnumNextPlate())
		{
			pDestPlate = m_xPncMode.GetPlateInfo(pSrcPlate->GetPartNo());
			if (pDestPlate == NULL)
				pDestPlate = m_xPncMode.AppendPlate(pSrcPlate->GetPartNo());
			pDestPlate->CopyAttributes(pSrcPlate);
		}
	}
	else
	{	//��ȡ����ͼԪ
		m_xPncMode.Empty();
		SmartExtractPlate(&m_xPncMode);
	}
	return TRUE;
}
//�Ǹ�DWG�ļ�����
//�������ݵ������������Ӧ�Ǹ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPt(f3dPoint data_pos)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(pJgInfo->IsInPartRgn(data_pos))
			break;
	}
	return pJgInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĽǸ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPartNo(const char* sPartNo)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(stricmp(pJgInfo->m_xAngle.sPartNo,sPartNo)==0)
			break;
	}
	return pJgInfo;
}
//���½Ǹּӹ���
void CDwgFileInfo::ModifyAngleDwgPartNum()
{
	if(m_hashJgInfo.GetNodeNum()<=0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "���½Ǹּӹ���.....");
	for(CAngleProcessInfo* pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pBomJg ==NULL)
		{	
			logerr.Log("�ϵ�������û��%s�Ǹ�",(char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.nSumPart != pBomJg->nSumPart)
		{
			pJgInfo->m_xAngle.nSumPart = pBomJg->nSumPart;	//�ӹ���
			pJgInfo->RefreshAngleNum();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֵ�����
void CDwgFileInfo::ModifyAngleDwgSingleNum()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "���½Ǹֵ�����.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.SetPartNum(pBomJg->GetPartNum());	//������
		pJgInfo->RefreshAngleSingleNum();
	}
	DisplayCadProgress(100);
}
//���½Ǹ�����
void CDwgFileInfo::ModifyAngleDwgSumWeight()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹ�����.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.fSumWeight != pBomJg->fSumWeight)
		{
			pJgInfo->m_xAngle.fSumWeight = pBomJg->fSumWeight;
			pJgInfo->RefreshAngleSumWeight();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֹ��
void CDwgFileInfo::ModifyAngleDwgSpec()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹֹ����Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (!pJgInfo->m_xAngle.sSpec.EqualNoCase(pBomJg->sSpec))
		{
			pJgInfo->m_xAngle.wide = pBomJg->wide;
			pJgInfo->m_xAngle.wingWideY = pBomJg->wingWideY;
			pJgInfo->m_xAngle.thick = pBomJg->thick;
			pJgInfo->m_xAngle.sSpec = pBomJg->sSpec;
			pJgInfo->RefreshAngleSpec();
		}
	}
	DisplayCadProgress(100);
}
//���½Ǹֲ���
void CDwgFileInfo::ModifyAngleDwgMaterial()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���½Ǹֲ�����Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			continue;
		}
		if (toupper(pJgInfo->m_xAngle.cMaterial) != toupper(pBomJg->cMaterial) ||
			(pJgInfo->m_xAngle.cMaterial != pBomJg->cMaterial && !g_xUbomModel.m_bEqualH_h) ||
			(pJgInfo->m_xAngle.cMaterial == 'A' && !pBomJg->sMaterial.Equal(pJgInfo->m_xAngle.sMaterial)) ||
			(g_xUbomModel.m_bCmpQualityLevel && pJgInfo->m_xAngle.cQualityLevel != pBomJg->cQualityLevel))
		{
			pJgInfo->m_xAngle.cMaterial = pBomJg->cMaterial;
			pJgInfo->m_xAngle.sMaterial = pBomJg->sMaterial;
			pJgInfo->m_xAngle.cQualityLevel = pBomJg->cQualityLevel;
			pJgInfo->RefreshAngleMaterial();
		}
	}
	DisplayCadProgress(100);
}
//��䵥Ԫ������
void CDwgFileInfo::DimGridData(AcDbBlockTableRecord *pBlockTableRecord, GEPOINT orgPt,
	GRID_DATA_STRU& grid_data, const char* sText)
{
	if (strlen(sText) <= 0 || pBlockTableRecord == NULL)
		return;
	GEPOINT dimPos;
	dimPos.x = (grid_data.max_x + grid_data.min_x)*0.5 + orgPt.x;
	dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
	AcDb::TextHorzMode hMode = AcDb::kTextCenter;
	AcDb::TextVertMode vMode = AcDb::kTextVertMid;
	if (grid_data.align_type > 0 && grid_data.align_type < 10)
	{	//���뷽ʽΪ1-9
		AcDb::TextHorzMode hModeArr[3] = { AcDb::kTextRight,AcDb::kTextLeft,AcDb::kTextCenter };
		AcDb::TextVertMode vModeArr[3] = { AcDb::kTextTop,AcDb::kTextVertMid,AcDb::kTextBottom };
		int iHIndex = grid_data.align_type % 3;
		int iVIndex = 0;
		if (grid_data.align_type == 4 || grid_data.align_type == 5 || grid_data.align_type == 6)
			iVIndex = 1;
		else if (grid_data.align_type == 7 || grid_data.align_type == 8 || grid_data.align_type == 9)
			iVIndex = 2;
		hMode = hModeArr[iHIndex];
		vMode = vModeArr[iVIndex];
		if (hMode == AcDb::kTextRight)
		{
			dimPos.x = grid_data.max_x + orgPt.x;
			dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
		}
		else if (hMode == AcDb::kTextLeft)
		{
			dimPos.x = grid_data.min_x + orgPt.x;
			dimPos.y = (grid_data.max_y + grid_data.min_y)*0.5 + orgPt.y;
		}
	}
	DimText(pBlockTableRecord, dimPos, sText, TextStyleTable::hzfs.textStyleId, grid_data.fTextHigh, 0, hMode, vMode);
}
//����Ǹֹ����ӿ�
void CDwgFileInfo::InsertSubJgCard(CAngleProcessInfo* pJgInfo, BOMPART* pBomPart)
{
	CLockDocumentLife lockCurDocLife;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();//�������¼ָ��
	if (pBlockTableRecord == NULL || pJgInfo == NULL)
		return;
	GEPOINT org_pt = pJgInfo->GetJgCardPosL_B();
	char sSubJgCardPath[MAX_PATH] = "", APP_PATH[MAX_PATH] = "";
	GetAppPath(APP_PATH);
	sprintf(sSubJgCardPath, "%s�Ǹֹ��տ�\\SubJgCard.dwg", APP_PATH);
	AcDbDatabase blkDb(Adesk::kFalse);//����յ����ݿ�
#ifdef _ARX_2007
	if (blkDb.readDwgFile((ACHAR*)_bstr_t(sSubJgCardPath), _SH_DENYRW, true) == Acad::eOk)
#else
	if (blkDb.readDwgFile(sSubJgCardPath, _SH_DENYRW, true) == Acad::eOk)
#endif
	{
		AcDbBlockTable *pTempBlockTable = NULL;
		blkDb.getBlockTable(pTempBlockTable, AcDb::kForRead);
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		pTempBlockTable->getAt(ACDB_MODEL_SPACE, pTempBlockTableRecord, AcDb::kForWrite);
		pTempBlockTable->close();//�رտ��
		AcDbBlockTableRecordIterator *pIterator = NULL;
		pTempBlockTableRecord->newIterator(pIterator);
		for (; !pIterator->done(); pIterator->step())
		{
			AcDbEntity *pEnt = NULL;
			pIterator->getEntity(pEnt, AcDb::kForWrite);
			if (pEnt == NULL)
				continue;
			GRID_DATA_STRU grid_data;
			if (!pEnt->isKindOf(AcDbPoint::desc()) ||
				!GetGridKey((AcDbPoint*)pEnt, &grid_data) ||
				grid_data.data_type != 2)
			{
				pEnt->close();
				continue;
			}
			if (grid_data.type_id == ITEM_TYPE_SUM_PART_NUM)
			{	//�ܼ���
				CXhChar50 ss("%d", pBomPart->feature1);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_SUM_WEIGHT)
			{	//����
				CXhChar50 ss("%g", pBomPart->fSumWeight);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_LSSUM_NUM)
			{	//�ܿ���
				CXhChar50 ss("%d", pBomPart->feature2);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PROCESS_NAME)
			{	//��������
				CXhChar100 ss=pJgInfo->GetJgProcessInfo();
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_TYPE)
			{	//����
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaType);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TOLERANCE)
			{	//���ϱ�׼
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sMatStandard);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TECH_REQ)
			{	//����Ҫ�������
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaAlias);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TASK_NO)
			{	//���񵥺�
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaskNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PRJ_NAME)
			{	//��������
				CXhChar50 ss;
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sPrjName);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_NUM)
			{	//����
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaNum);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_CODE_NO)
			{	//����(��ͬ��)
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sContractNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_DATE)
			{	//����
				CTime time = CTime::GetCurrentTime();
				CString ss = time.Format("%Y.%m.%d");
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else
				pEnt->close();
		}
		pTempBlockTableRecord->close();
	}
	//
	CXhChar50 sSubJgCard("SubJgCard");
	AcDbObjectId blockId = SearchBlock(sSubJgCard);
	if (blockId.isNull())
	{
		AcDbDatabase *pTempDb = NULL;
		blkDb.wblock(pTempDb);
#ifdef _ARX_2007
		GetCurDwg()->insert(blockId, _bstr_t(sSubJgCard), pTempDb);
#else
		GetCurDwg()->insert(blockId, sSubJgCard, pTempDb);
#endif
		delete pTempDb;
	}
	//�����տ������һ��ͼ��
	AcGeScale3d scaleXYZ(1.0, 1.0, 1.0);
	AcDbBlockReference *pBlkRef = new AcDbBlockReference;
	pBlkRef->setBlockTableRecord(blockId);
	pBlkRef->setPosition(AcGePoint3d(org_pt.x, org_pt.y, 0));
	pBlkRef->setRotation(0);
	pBlkRef->setScaleFactors(scaleXYZ);
	//��ͼ����ӵ������
	pBlockTableRecord->appendAcDbEntity(blockId, pBlkRef);
	pBlkRef->close();
	pBlockTableRecord->close();
}
//���ƽǸֹ��տ���Ϣ���ɶ����������ƣ�
void CDwgFileInfo::FillAngleDwgData()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	std::vector<CAngleProcessInfo*> vectorInvalidJg;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "���Ǹֹ��տ���Ϣ......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("�ϵ�������û��%s�Ǹ�", (char*)sPartNo);
			vectorInvalidJg.push_back(pJgInfo);
			continue;
		}
		//
		InsertSubJgCard(pJgInfo,pLoftBom);
	}
	//��DWG�ļ���ɾ������Ҫ�ĽǸֹ��տ�
	for (size_t i = 0; i < vectorInvalidJg.size(); i++)
	{
		//ɾ������ͼԪ
		vectorInvalidJg[i]->ExtractRelaEnts();
		vectorInvalidJg[i]->EraseRelaEnts();
		//ɾ���ýǸ�
		m_hashJgInfo.DeleteNode(vectorInvalidJg[i]->keyId.handle());
	}
	DisplayCadProgress(100);
}
//��ȡ�Ǹֲ���
BOOL CDwgFileInfo::RetrieveAngles(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CAcModuleResourceOverride resOverride;
	ShiftCSToWCS(TRUE);
	//ѡ������ʵ��ͼԪ
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, bSupportSelectEnts ? FALSE : TRUE);
	//���ݽǸֹ��տ���ʶ��Ǹ�
	int index = 1, nNum = allEntIdSet.GetNodeNum()*2;
	DisplayCadProgress(0, "ʶ��Ǹֹ��տ�.....");
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		//���ݽǸֹ��տ�����ȡ�Ǹ���Ϣ
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
		AcDbObjectId blockId = pReference->blockTableRecord();
		acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
		if (pTempBlockTableRecord == NULL)
			continue;
		pTempBlockTableRecord->close();
		CXhChar50 sName;
#ifdef _ARX_2007
		ACHAR* sValue = new ACHAR[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy((char*)_bstr_t(sValue));
		delete[] sValue;
#else
		char *sValue = new char[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy(sValue);
		delete[] sValue;
#endif
		if (!g_xUbomModel.IsJgCardBlockName(sName))
			continue;
		//��ӽǸּ�¼
		CAngleProcessInfo* pJgInfo = m_hashJgInfo.Add(entId.handle());
		pJgInfo->Empty();
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
	}
	//����Ǹֹ��տ����������������"����"������ȡ�Ǹ���Ϣ
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<PART_LABEL_DIM> labelDimList;
	ATOM_LIST<CAD_ENTITY> cadTextList;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			PART_LABEL_DIM *pLabelDim = labelDimList.append();
			pLabelDim->m_xCirEnt.ciEntType = TYPE_CIRCLE;
			pLabelDim->m_xCirEnt.idCadEnt = pEnt->objectId().asOldId();
			pLabelDim->m_xCirEnt.m_fSize = pCir->radius();
			Cpy_Pnt(pLabelDim->m_xCirEnt.pos, pCir->center());
			continue;
		}
		if(!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()))
			continue;
		textIdHash.SetValue(entId.handle(), entId);	//��¼�Ǹֹ��տ���ʵʱ�ı�
		//���ݼ��Źؼ���ʶ��Ǹ�
		CXhChar100 sText = GetCadTextContent(pEnt);
		GEPOINT testPt = GetCadTextDimPos(pEnt);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText *pText = (AcDbText*)pEnt;
			CAD_ENTITY *pCadText = cadTextList.append();
			pCadText->ciEntType = TYPE_TEXT;
			pCadText->idCadEnt = entId.asOldId();
			pCadText->m_fSize = pText->height();
			pCadText->pos = testPt;
			strncpy(pCadText->sText, sText, 99);
		}
		if (!g_xUbomModel.IsPartLabelTitle(sText))
			continue;	//��Ч�ļ��ű���
		CAngleProcessInfo* pJgInfo = NULL;
		for (pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
		{
			if (pJgInfo->IsInPartRgn(testPt))
				break;
		}
		if (pJgInfo == NULL)
			pJgInfo = m_hashJgInfo.Add(entId.handle());
		//���½Ǹ���Ϣ wht 20-07-29
		if(pJgInfo)
		{	//��ӽǸּ�¼
			//���ݹ��տ�ģ���м��ű�ǵ����ýǸֹ��տ���ԭ��λ��
			GEPOINT orig_pt = g_pncSysPara.GetJgCardOrigin(testPt);
			pJgInfo->Empty();
			pJgInfo->keyId = entId;
			pJgInfo->m_bInJgBlock = false;
			pJgInfo->SetOrig(orig_pt);
		}
	}
	DisplayCadProgress(100);
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//���ݽǸ�����λ�û�ȡ�Ǹ���Ϣ
	index = 1;
	nNum = textIdHash.GetNodeNum();
	DisplayCadProgress(0, "��ʼ���Ǹ���Ϣ.....");
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		CXhChar50 sValue=GetCadTextContent(pEnt);
		GEPOINT text_pos = GetCadTextDimPos(pEnt);
		if(strlen(sValue)<=0)	//���˿��ַ�
			continue;
		CAngleProcessInfo* pJgInfo=FindAngleByPt(text_pos);
		if (pJgInfo)
		{
			BYTE cType = pJgInfo->InitAngleInfo(text_pos, sValue);
			if (cType == ITEM_TYPE_SUM_PART_NUM)
				pJgInfo->partNumId = objId;
			else if (cType == ITEM_TYPE_SUM_WEIGHT)
				pJgInfo->sumWeightId = objId;
			else if (cType == ITEM_TYPE_PART_NUM)
				pJgInfo->singleNumId = objId;
			else if (cType == ITEM_TYPE_DES_MAT)
				pJgInfo->materialId = objId;
			else if (cType == ITEM_TYPE_DES_GUIGE)
				pJgInfo->specId = objId;
		}
	}
	DisplayCadProgress(100);
	if (!CAngleProcessInfo::bInitGYByGYRect)
	{	//���ݺ����߰��ʼ���Ǹֺ������� wht 20-09-29
		//��ʼ�����ű�ע��������
		for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
		{
			CAD_ENTITY *pCadText = NULL;
			for (pCadText = cadTextList.GetFirst(); pCadText; pCadText = cadTextList.GetNext())
			{
				if (pLabelDim->m_xCirEnt.IsInScope(pCadText->pos))
				{
					pLabelDim->m_xInnerText.ciEntType = pCadText->ciEntType;
					pLabelDim->m_xInnerText.idCadEnt = pCadText->idCadEnt;
					pLabelDim->m_xInnerText.m_fSize = pCadText->m_fSize;
					pLabelDim->m_xInnerText.pos = pCadText->pos;
					strcpy(pLabelDim->m_xInnerText.sText, pCadText->sText);
					break;
				}
			}
			if (pCadText == NULL)	//δ�ҵ����ֵ�ԲȦ���Ƴ�
				labelDimList.DeleteCursor();
		}
		labelDimList.Clean();
		for (PART_LABEL_DIM *pLabelDim = labelDimList.GetFirst(); pLabelDim; pLabelDim = labelDimList.GetNext())
		{
			SEGI segI = pLabelDim->GetSegI();
			if (segI.iSeg <= 0)
				continue;
			CAngleProcessInfo* pJgInfo = FindAngleByPt(pLabelDim->m_xCirEnt.pos);
			if (pJgInfo && !pJgInfo->m_xAngle.bWeldPart)
				pJgInfo->m_xAngle.bWeldPart = TRUE;
		}
	}
	//����ȡ�ĽǸ���Ϣ���к����Լ��
	CHashStrList<BOOL> hashJgByPartNo;
	for (CAngleProcessInfo* pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
	{
		if (pJgInfo->m_xAngle.sPartNo.GetLength() <= 0)
			m_hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		else
		{
			if (hashJgByPartNo.GetValue(pJgInfo->m_xAngle.sPartNo))
				logerr.Log("����(%s)�ظ�", (char*)pJgInfo->m_xAngle.sPartNo);
			else
				hashJgByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
#ifdef __ALFA_TEST_
		if(!pJgInfo->m_bInJgBlock)
			logerr.Log("(%s)�Ǹֹ��տ��Ǵ����!", (char*)pJgInfo->m_xAngle.sPartNo);
#endif
	}
	m_hashJgInfo.Clean();
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	return TRUE;
}
#endif