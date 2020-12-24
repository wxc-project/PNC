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
//钢板DWG操作
//根据数据点坐标查找对应的钢板
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
//根据件号查找对应的钢板
CPlateProcessInfo* CDwgFileInfo::FindPlateByPartNo(const char* sPartNo)
{
	return m_xPncMode.PartFromPartNo(sPartNo);
}
//更新钢板加工数据
void CDwgFileInfo::ModifyPlateDwgPartNum()
{
	if(m_xPncMode.GetPlateNum()<=0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "更新钢板加工数信息.....");
	for(CPlateProcessInfo* pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pLoftBom ==NULL)
		{
			logerr.Log("料单数据中没有%s钢板",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			logerr.Log("%s钢板件数修改失败!",(char*)sPartNo);
			continue;
		}
		//加工数不同进行修改
		if(pInfo->xBomPlate.nSumPart != pLoftBom->nSumPart)
			pInfo->RefreshPlateNum(pLoftBom->nSumPart);
	}
	DisplayCadProgress(100);
}
//更新钢板规格
void CDwgFileInfo::ModifyPlateDwgSpec()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "更新钢板规格信息.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("料单数据中没有%s钢板", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s钢板规格修改失败!", (char*)sPartNo);
			continue;
		}
		if (pInfo->xBomPlate.thick != pLoftBom->thick)
		{	//规格不同进行修改
			pInfo->xBomPlate.thick = pLoftBom->thick;	//规格
			pInfo->xBomPlate.sSpec = pLoftBom->sSpec;	//规格
			pInfo->RefreshPlateSpec();
		}
	}
	DisplayCadProgress(100);
}
//更新钢板材质
void CDwgFileInfo::ModifyPlateDwgMaterial()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "更新钢板材质信息.....");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("料单数据中没有%s钢板", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s钢板材质修改失败!", (char*)sPartNo);
			continue;
		}
		if (toupper(pInfo->xBomPlate.cMaterial) != toupper(pLoftBom->cMaterial) ||
			(pInfo->xBomPlate.cMaterial != pLoftBom->cMaterial && !g_xUbomModel.m_bEqualH_h)||
			(pInfo->xBomPlate.cMaterial == 'A' && !pLoftBom->sMaterial.Equal(pInfo->xBomPlate.sMaterial)) ||
			(g_xUbomModel.m_bCmpQualityLevel && pInfo->xBomPlate.cQualityLevel != pLoftBom->cQualityLevel))
		{	//材质不同进行修改
			pInfo->xBomPlate.cMaterial = pLoftBom->cMaterial;	//材质
			pInfo->xBomPlate.sMaterial = pLoftBom->sMaterial;	//材质
			pInfo->xBomPlate.cQualityLevel = pLoftBom->cQualityLevel;
			//pInfo->xPlate.cMaterial = pLoftBom->cMaterial;	//材质
			//pInfo->xPlate.cQuality = pLoftBom->cQualityLevel;
			pInfo->RefreshPlateMat();
		}
	}
	DisplayCadProgress(100);
}
//完善钢板大样图基本信息（成都铁塔厂定制）
void CDwgFileInfo::FillPlateDwgData()
{
	if (m_xPncMode.GetPlateNum() <= 0)
		return;
	std::vector<CPlateProcessInfo*> vectorInvalidPlate;
	int index = 1, nNum = m_xPncMode.GetPlateNum() + 1;
	DisplayCadProgress(0, "填充钢板加工数信息");
	for (CPlateProcessInfo* pInfo = EnumFirstPlate(); pInfo; pInfo = EnumNextPlate(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pInfo->xPlate.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("料单数据中没有%s钢板", (char*)sPartNo);
			vectorInvalidPlate.push_back(pInfo);
			continue;
		}
		if (pLoftBom->nSumPart <= 0)
		{
			logerr.Log("料单数据中钢板(%s)加工数为0", (char*)sPartNo);
			continue;
		}
		if (pInfo->partNoId == NULL)
		{
			logerr.Log("%s钢板材质修改失败!", (char*)sPartNo);
			continue;
		}
		//补充钢板的加工数
		pInfo->FillPlateNum(pLoftBom->nSumPart, pInfo->partNoId);
		//处理重复件号钢板
		for (AcDbObjectId objId = pInfo->repeatEntList.GetFirst(); objId; objId = pInfo->repeatEntList.GetNext())
		{
			if (pInfo->partNoId == objId)
				continue;
			pInfo->FillPlateNum(pLoftBom->nSumPart, objId);
		}
	}
	//从DWG文件中删除不需要的样板
	for (size_t i = 0; i < vectorInvalidPlate.size(); i++)
	{
		if (!vectorInvalidPlate[i]->IsValid())
		{
			logerr.Log("(%s)钢板轮廓点提取失败,无法删除!", (char*)vectorInvalidPlate[i]->GetPartNo());
			continue;
		}
		//删除该钢板的关联图元
		vectorInvalidPlate[i]->ExtractRelaEnts();
		vectorInvalidPlate[i]->EraseRelaEnts();
		//删除该该钢板
		m_xPncMode.DeletePlate(vectorInvalidPlate[i]->GetPartNo());
	}
	DisplayCadProgress(100);
}
//提取板的轮廓边,确定闭合区域
BOOL CDwgFileInfo::RetrievePlates(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CPNCModel::m_bSendCommand = TRUE;
	if (bSupportSelectEnts)
	{	//提取用户选择的图元
		CPNCModel tempMode;
		SmartExtractPlate(&tempMode, TRUE);
		//数据拷贝
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
	{	//提取所有图元
		m_xPncMode.Empty();
		SmartExtractPlate(&m_xPncMode);
	}
	return TRUE;
}
//角钢DWG文件操作
//根据数据点坐标查找所对应角钢
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
//根据件号查找对应的角钢
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
//更新角钢加工数
void CDwgFileInfo::ModifyAngleDwgPartNum()
{
	if(m_hashJgInfo.GetNodeNum()<=0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "更新角钢加工数.....");
	for(CAngleProcessInfo* pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo=pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pBomJg ==NULL)
		{	
			logerr.Log("料单数据中没有%s角钢",(char*)sPartNo);
			continue;
		}
		if (pJgInfo->m_xAngle.nSumPart != pBomJg->nSumPart)
		{
			pJgInfo->m_xAngle.nSumPart = pBomJg->nSumPart;	//加工数
			pJgInfo->RefreshAngleNum();
		}
	}
	DisplayCadProgress(100);
}
//更新角钢单基数
void CDwgFileInfo::ModifyAngleDwgSingleNum()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum()+1;
	DisplayCadProgress(0, "更新角钢单基数.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("料单数据中没有%s角钢", (char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.SetPartNum(pBomJg->GetPartNum());	//单基数
		pJgInfo->RefreshAngleSingleNum();
	}
	DisplayCadProgress(100);
}
//更新角钢总重
void CDwgFileInfo::ModifyAngleDwgSumWeight()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "更新角钢总重.....");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("料单数据中没有%s角钢", (char*)sPartNo);
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
//更新角钢规格
void CDwgFileInfo::ModifyAngleDwgSpec()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "更新角钢规格信息......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("料单数据中没有%s角钢", (char*)sPartNo);
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
//更新角钢材质
void CDwgFileInfo::ModifyAngleDwgMaterial()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "更新角钢材质信息......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.sPartNo;
		BOMPART* pBomJg = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pBomJg == NULL)
		{
			logerr.Log("料单数据中没有%s角钢", (char*)sPartNo);
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
//填充单元格数据
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
	{	//对齐方式为1-9
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
//插入角钢工艺子卡
void CDwgFileInfo::InsertSubJgCard(CAngleProcessInfo* pJgInfo, BOMPART* pBomPart)
{
	CLockDocumentLife lockCurDocLife;
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();//定义块表记录指针
	if (pBlockTableRecord == NULL || pJgInfo == NULL)
		return;
	GEPOINT org_pt = pJgInfo->GetJgCardPosL_B();
	char sSubJgCardPath[MAX_PATH] = "", APP_PATH[MAX_PATH] = "";
	GetAppPath(APP_PATH);
	sprintf(sSubJgCardPath, "%s角钢工艺卡\\SubJgCard.dwg", APP_PATH);
	AcDbDatabase blkDb(Adesk::kFalse);//定义空的数据库
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
		pTempBlockTable->close();//关闭块表
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
			{	//总件数
				CXhChar50 ss("%d", pBomPart->feature1);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_SUM_WEIGHT)
			{	//总重
				CXhChar50 ss("%g", pBomPart->fSumWeight);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_LSSUM_NUM)
			{	//总孔数
				CXhChar50 ss("%d", pBomPart->feature2);
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PROCESS_NAME)
			{	//工艺名称
				CXhChar100 ss=pJgInfo->GetJgProcessInfo();
				DimGridData(pBlockTableRecord, org_pt, grid_data, ss);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_TYPE)
			{	//塔型
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaType);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TOLERANCE)
			{	//材料标准
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sMatStandard);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TECH_REQ)
			{	//技术要求（塔规格）
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaAlias);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TASK_NO)
			{	//任务单号
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaskNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_PRJ_NAME)
			{	//工程名称
				CXhChar50 ss;
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sPrjName);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_TA_NUM)
			{	//基数
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sTaNum);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_CODE_NO)
			{	//代号(合同号)
				DimGridData(pBlockTableRecord, org_pt, grid_data, BelongModel()->m_xPrjInfo.m_sContractNo);
				pEnt->erase();
				pEnt->close();
			}
			else if (grid_data.type_id == ITEM_TYPE_DATE)
			{	//日期
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
	//将工艺卡打包成一个图块
	AcGeScale3d scaleXYZ(1.0, 1.0, 1.0);
	AcDbBlockReference *pBlkRef = new AcDbBlockReference;
	pBlkRef->setBlockTableRecord(blockId);
	pBlkRef->setPosition(AcGePoint3d(org_pt.x, org_pt.y, 0));
	pBlkRef->setRotation(0);
	pBlkRef->setScaleFactors(scaleXYZ);
	//将图块添加到块表中
	pBlockTableRecord->appendAcDbEntity(blockId, pBlkRef);
	pBlkRef->close();
	pBlockTableRecord->close();
}
//完善角钢工艺卡信息（成都铁塔厂定制）
void CDwgFileInfo::FillAngleDwgData()
{
	if (m_hashJgInfo.GetNodeNum() <= 0)
		return;
	std::vector<CAngleProcessInfo*> vectorInvalidJg;
	int index = 1, nNum = m_hashJgInfo.GetNodeNum() + 1;
	DisplayCadProgress(0, "填充角钢工艺卡信息......");
	for (CAngleProcessInfo* pJgInfo = EnumFirstJg(); pJgInfo; pJgInfo = EnumNextJg(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CXhChar16 sPartNo = pJgInfo->m_xAngle.GetPartNo();
		BOMPART* pLoftBom = m_pProject->m_xLoftBom.FindPart(sPartNo);
		if (pLoftBom == NULL)
		{
			logerr.Log("料单数据中没有%s角钢", (char*)sPartNo);
			vectorInvalidJg.push_back(pJgInfo);
			continue;
		}
		//
		InsertSubJgCard(pJgInfo,pLoftBom);
	}
	//从DWG文件中删除不需要的角钢工艺卡
	for (size_t i = 0; i < vectorInvalidJg.size(); i++)
	{
		//删除关联图元
		vectorInvalidJg[i]->ExtractRelaEnts();
		vectorInvalidJg[i]->EraseRelaEnts();
		//删除该角钢
		m_hashJgInfo.DeleteNode(vectorInvalidJg[i]->keyId.handle());
	}
	DisplayCadProgress(100);
}
//提取角钢操作
BOOL CDwgFileInfo::RetrieveAngles(BOOL bSupportSelectEnts /*= FALSE*/)
{
	CAcModuleResourceOverride resOverride;
	ShiftCSToWCS(TRUE);
	//选择所有实体图元
	CHashSet<AcDbObjectId> allEntIdSet;
	SelCadEntSet(allEntIdSet, bSupportSelectEnts ? FALSE : TRUE);
	//根据角钢工艺卡块识别角钢
	int index = 1, nNum = allEntIdSet.GetNodeNum()*2;
	DisplayCadProgress(0, "识别角钢工艺卡.....");
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId = allEntIdSet.GetFirst(); entId.isValid(); entId = allEntIdSet.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		//根据角钢工艺卡块提取角钢信息
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
		//添加角钢记录
		CAngleProcessInfo* pJgInfo = m_hashJgInfo.Add(entId.handle());
		pJgInfo->Empty();
		pJgInfo->keyId = entId;
		pJgInfo->SetOrig(GEPOINT(pReference->position().x, pReference->position().y));
	}
	//处理角钢工艺卡块打碎的情况：根据"件号"标题提取角钢信息
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
		textIdHash.SetValue(entId.handle(), entId);	//记录角钢工艺卡的实时文本
		//根据件号关键字识别角钢
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
			continue;	//无效的件号标题
		CAngleProcessInfo* pJgInfo = NULL;
		for (pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
		{
			if (pJgInfo->IsInPartRgn(testPt))
				break;
		}
		if (pJgInfo == NULL)
			pJgInfo = m_hashJgInfo.Add(entId.handle());
		//更新角钢信息 wht 20-07-29
		if(pJgInfo)
		{	//添加角钢记录
			//根据工艺卡模板中件号标记点计算该角钢工艺卡的原点位置
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
		logerr.Log("%s文件提取角钢失败",(char*)m_sFileName);
		return FALSE;
	}
	//根据角钢数据位置获取角钢信息
	index = 1;
	nNum = textIdHash.GetNodeNum();
	DisplayCadProgress(0, "初始化角钢信息.....");
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext(),index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(objId);
		if ((pEnt = objLife.GetEnt()) == NULL)
			continue;
		CXhChar50 sValue=GetCadTextContent(pEnt);
		GEPOINT text_pos = GetCadTextDimPos(pEnt);
		if(strlen(sValue)<=0)	//过滤空字符
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
	{	//根据焊接肋板初始化角钢焊接属性 wht 20-09-29
		//初始化件号标注文字内容
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
			if (pCadText == NULL)	//未找到文字的圆圈需移除
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
	//对提取的角钢信息进行合理性检查
	CHashStrList<BOOL> hashJgByPartNo;
	for (CAngleProcessInfo* pJgInfo = m_hashJgInfo.GetFirst(); pJgInfo; pJgInfo = m_hashJgInfo.GetNext())
	{
		if (pJgInfo->m_xAngle.sPartNo.GetLength() <= 0)
			m_hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		else
		{
			if (hashJgByPartNo.GetValue(pJgInfo->m_xAngle.sPartNo))
				logerr.Log("件号(%s)重复", (char*)pJgInfo->m_xAngle.sPartNo);
			else
				hashJgByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo, TRUE);
		}
#ifdef __ALFA_TEST_
		if(!pJgInfo->m_bInJgBlock)
			logerr.Log("(%s)角钢工艺卡是打碎的!", (char*)pJgInfo->m_xAngle.sPartNo);
#endif
	}
	m_hashJgInfo.Clean();
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s文件提取角钢失败",(char*)m_sFileName);
		return FALSE;
	}
	return TRUE;
}
#endif