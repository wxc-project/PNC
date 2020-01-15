#include "StdAfx.h"
#include "PNCCmd.h"
#include "CadToolFunc.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "PNCSysSettingDlg.h"
#include "InputMKRectDlg.h"
#include "BomModel.h"
#include "RevisionDlg.h"
#include "LicFuncDef.h"
#include "ProcBarDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CProcBarDlg *pProcDlg = NULL;
static int prevPercent = -1;
void DisplayProcess(int percent, char *sTitle)
{
	if (percent >= 100)
	{
		if (pProcDlg != NULL)
		{
			pProcDlg->DestroyWindow();
			delete pProcDlg;
			pProcDlg = NULL;
		}
		return;
	}
	else if (pProcDlg == NULL)
		pProcDlg = new CProcBarDlg(NULL);
	if (pProcDlg->GetSafeHwnd() == NULL)
		pProcDlg->Create();
	if (percent == prevPercent)
		return;	//跟上次进度一致不需要更新
	else
		prevPercent = percent;
	if (sTitle)
		pProcDlg->SetTitle(CString(sTitle));
	else
		pProcDlg->SetTitle("进度");
	pProcDlg->Refresh(percent);
}
static BOOL RecogBaseInfoFromBlockRef(AcDbBlockReference *pBlockRef,BASIC_INFO &baseInfo)
{
	if(pBlockRef==NULL)
		return false;
	BOOL bRetCode=false;
	AcDbEntity *pEnt=NULL;
	AcDbObjectIterator *pIter=pBlockRef->attributeIterator();
	for(pIter->start();!pIter->done();pIter->step())
	{
		acdbOpenAcDbEntity(pEnt,pIter->objectId(),AcDb::kForRead);
		CAcDbObjLife objLife(pEnt);
		if(pEnt==NULL||!pEnt->isKindOf(AcDbAttribute::desc()))
			continue;
		AcDbAttribute *pAttr=(AcDbAttribute*)pEnt;
		CXhChar100 sTag,sText;
#ifdef _ARX_2007
		sTag.Copy(_bstr_t(pAttr->tag()));
		sText.Copy(_bstr_t(pAttr->textString()));
#else
		sTag.Copy(pAttr->tag());
		sText.Copy(pAttr->textString());
#endif
		if(sTag.GetLength()==0||sText.GetLength()==0)
			continue;
		if(sTag.EqualNoCase("件号&规格&材质"))
			bRetCode=g_pncSysPara.RecogBasicInfo(pAttr,baseInfo);
		else if(sTag.EqualNoCase("数量"))
		{
			CXhChar50 sTemp(sText);
			for(char* token=strtok(sTemp,"X=");token;token=strtok(NULL,"X="))
			{
				CXhChar16 sToken(token);
				if(sToken.Replace("块","")>0)
					baseInfo.m_nNum=atoi(sToken);
			}
		}
		else if(sTag.EqualNoCase("塔型"))
			baseInfo.m_sTaType.Copy(sText);
		else if(sTag.EqualNoCase("钢印"))
		{
			sText.Replace("钢印","");
			sText.Replace(":","");
			sText.Replace("：","");
			baseInfo.m_sTaStampNo.Copy(sText);
		}
		else if(sTag.EqualNoCase("孔径"))
			baseInfo.m_sBoltStr.Copy(sText);
		else if(sTag.EqualNoCase("工程代码"))
		{
			sText.Replace("工程代码","");
			sText.Replace(":","");
			sText.Replace("：","");
			baseInfo.m_sPrjCode.Copy(sText);
		}
	}
	return bRetCode;
}


void UpdatePartList()
{
#ifndef __UBOM_ONLY_
	if(g_pncSysPara.m_bAutoLayout==CPNCSysPara::LAYOUT_SEG)
	{
		if(!IsShowDisplayPartListDockBar())
			DisplayPartListDockBar();
	}
	CPartListDlg *pPartListDlg = NULL;
#ifdef __SUPPORT_DOCK_UI_
	if(g_pPartListDockBar!=NULL)
		pPartListDlg=(CPartListDlg*)g_pPartListDockBar->GetDlgPtr();
#else
	pPartListDlg = g_pPartListDlg;
#endif
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
#endif
}
//////////////////////////////////////////////////////////////////////////
//智能提取板的信息,提取成功后支持排版
//SmartExtractPlate
//////////////////////////////////////////////////////////////////////////
void SmartExtractPlate()
{
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	CDwgFileInfo *pDwgFile = NULL;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc != NULL && g_pRevisionDlg != NULL)
	{
		CString file_path = pDoc->fileName();
		pDwgFile = g_xUbomModel.FindDwgFile(file_path);
	}
	if (pDwgFile)
		SmartExtractPlate(pDwgFile->GetPncModel());
	else
		SmartExtractPlate(&model);
#else
	CString file_path;
	GetCurWorkPath(file_path);
	if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
	{	//下料预审模式，保留上次提取的钢板信息，避免重复提取
		if (!model.m_sWorkPath.EqualNoCase(file_path))
		{
			model.Empty();
			model.m_sWorkPath.Copy(file_path);
		}
	}
	else
		model.Empty();
	SmartExtractPlate(&model);
#endif
}

void SmartExtractPlate(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	pModel->DisplayProcess = DisplayProcess;
	CLogErrorLife logErrLife;
	AcDbObjectId entId;
	AcDbEntity *pEnt=NULL;
	CHashSet<AcDbObjectId> selectedEntList;
	//默认选择所有的图形，方便后期的过滤使用
	ads_name ent_sel_set,entname;
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	long ll=0;
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	//初始化实体集合
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		CAcDbObjLife objLife(pEnt);
		pModel->m_xAllEntIdSet.SetValue(entId.asOldId(),entId);
		selectedEntList.SetValue(entId.asOldId(), entId);
	}
#ifndef __UBOM_ONLY_
	//进行手动框选
	int retCode=0;
#ifdef _ARX_2007
	retCode=acedSSGet(L"",NULL,NULL,NULL,ent_sel_set);
#else
	retCode=acedSSGet("",NULL,NULL,NULL,ent_sel_set);
#endif
	if(retCode==RTCAN)
	{	//用户按ESC取消
		acedSSFree(ent_sel_set);
		return;
	}
	else
	{	
		selectedEntList.Empty();
		acedSSLength(ent_sel_set,&ll);
		for(long i=0;i<ll;i++)
		{	//根据文字说明初始化点集合
			acedSSName(ent_sel_set,i,entname);
			acdbGetObjectId(entId,entname);
			acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
			if(pEnt==NULL)
				continue;
			CAcDbObjLife objLife(pEnt);
			selectedEntList.SetValue(entId.asOldId(),entId);
		}
	}
	acedSSFree(ent_sel_set);
#endif
	//从框选信息中提取中钢板的标识，统计钢板集合
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<GEPOINT> holePosList;
	BOLT_HOLE hole;
	for (entId=selectedEntList.GetFirst(); entId.isValid();entId=selectedEntList.GetNext())
	{
		acdbOpenAcDbEntity(pEnt, entId, AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		CAcDbObjLife objLife(pEnt);
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()) && !pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		textIdHash.SetValue(entId.asOldId(), entId);
		bool bValidAttrBlock = false;
		BASIC_INFO baseInfo;
		f3dPoint dim_pos, dim_vec;
		CXhChar500 sText;
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			//TMA生成钢板大样图之后，框选从一个文件复制到另外一个文件时，会导致position与alignmentPoint不一致
			//位置不一致，修改属性后会触发adjustAlignment,导致所有文字位置偏移，此处调用adjustAlignment调整pos位置，
			//再根据原始的position位置重设alignmentPoint wht 19-01-12
			//AcGePoint3d org_txt_pos = pText->position();//alignmentPoint();
			//AcGePoint3d org_align_pos = pText->alignmentPoint();
			//pText->adjustAlignment();	//调用adjustAlignment调整pos与alignmentPoint一致
			AcGePoint3d txt_pos = pText->position();//alignmentPoint();
			AcGePoint3d align_pos = pText->alignmentPoint();
			//AcGePoint3d offset_pos;
			//Sub_Pnt(offset_pos, align_pos, txt_pos);
			//pText->setPosition(org_txt_pos);
			//Add_Pnt(align_pos, org_txt_pos, offset_pos);
			//pText->setAlignmentPoint(align_pos);
			//
			if (!g_pncSysPara.IsMatchPNRule(sText))
				continue;
			//
			double fTestH = pText->height();
			double fWidth = TestDrawTextLength(sText, fTestH, pText->textStyle());
			AcDb::TextHorzMode  horzMode = pText->horizontalMode();
			AcDb::TextVertMode  vertMode = pText->verticalMode();
			//获取AcDbText插入点 wht 18-12-20
			//If vertical mode is AcDb::kTextBase and horizontal mode is either AcDb::kTextLeft, AcDb::kTextAlign, or AcDb::kTextFit,
			//then the position point is the insertion point for the text object and, for AcDb::kTextLeft, 
			//the alignment point (DXF group code 11) is automatically calculated based on the other parameters in the text object.
			if (vertMode == AcDb::kTextBase)
			{
				if (horzMode == AcDb::kTextLeft || horzMode == AcDb::kTextAlign || horzMode == AcDb::kTextFit)
					txt_pos = pText->position();
				else
					txt_pos = pText->alignmentPoint();
			}
			else
				txt_pos = pText->alignmentPoint();

			dim_vec.Set(cos(pText->rotation()), sin(pText->rotation()));
			dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
			//
			if (horzMode == AcDb::kTextLeft)
				dim_pos += dim_vec * (fWidth*0.5);
			else if (horzMode == AcDb::kTextRight)
				dim_pos -= dim_vec * (fWidth*0.5);
		}
		else if (pEnt->isKindOf(AcDbMText::desc()))
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pMText->contents()));
#else
			sText.Copy(pMText->contents());
#endif			
			//此处使用\P分割可能会误将2310P中的材质字符抹去，需要特殊处理将\P替换\W wht 19-09-09
			if (sText.GetLength() > 0)
			{
				CXhChar500 sNewText;
				char cPreChar = sText.At(0);
				sNewText.Append(cPreChar);
				for (int i = 1; i < sText.GetLength(); i++)
				{
					char cCurChar = sText.At(i);
					if (cPreChar == '\\'&&cCurChar == 'P')
						sNewText.Append('W');
					else
						sNewText.Append(cCurChar);
					cPreChar = cCurChar;
				}
				sText.Copy(sNewText);
			}
			ATOM_LIST<CXhChar200> lineTextList;
			for (char* sKey = strtok(sText, "\\W"); sKey; sKey = strtok(NULL, "\\W"))
			{
				CXhChar200 sTemp(sKey);
				sTemp.Replace("\\W", "");
				lineTextList.append(sTemp);
			}
			if (lineTextList.GetNodeNum() > 0)
			{
				BOOL bFindPartNo = FALSE;
				for (CXhChar200 *pLineText = lineTextList.GetFirst(); pLineText; pLineText = lineTextList.GetNext())
				{
					if (g_pncSysPara.IsMatchPNRule(*pLineText))
					{
						sText.Copy(*pLineText);
						bFindPartNo = TRUE;
						break;
					}
				}
				if (!bFindPartNo)
					continue;
			}
			else
			{
				if (!g_pncSysPara.IsMatchPNRule(sText))
					continue;
			}
			AcGePoint3d txt_pos = pMText->location();
			double fTestH = pMText->actualHeight();
			double fWidth = pMText->actualWidth();
			dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
			dim_vec.Set(cos(pMText->rotation()), sin(pMText->rotation()));

			AcDbMText::AttachmentPoint align_type = pMText->attachment();
			if (align_type == AcDbMText::kTopLeft || align_type == AcDbMText::kMiddleLeft || align_type == AcDbMText::kBottomLeft)
				dim_pos += dim_vec * (fWidth*0.5);
			else if (align_type == AcDbMText::kTopRight || align_type == AcDbMText::kMiddleRight || align_type == AcDbMText::kBottomRight)
				dim_pos -= dim_vec * (fWidth*0.5);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//从块中解析钢板信息
			AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
			if (RecogBaseInfoFromBlockRef(pBlockRef, baseInfo))
			{
				AcGePoint3d txt_pos = pBlockRef->position();
				dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
				dim_vec.Set(cos(pBlockRef->rotation()), sin(pBlockRef->rotation()));
				bValidAttrBlock = true;
			}
			else
			{
				if (g_pncSysPara.RecogBoltHole(pEnt, hole))
					holePosList.append(GEPOINT(hole.posX, hole.posY));
				continue;
			}
		}
		CXhChar16 sPartNo;
		if (bValidAttrBlock)
			sPartNo.Copy(baseInfo.m_sPartNo);
		else
		{
			BYTE ciRetCode = g_pncSysPara.ParsePartNoText(sText, sPartNo);
			if (ciRetCode == CPlateExtractor::PART_LABEL_WELD)
				continue;	//当前件号为焊接子件件号 wht 19-07-22
		}
		CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(sPartNo);
		if (strlen(sPartNo) <= 0 ||
			(pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId)))
		{	//件号相同，但件号文本对应的实体不相同提示件号重复 wht 19-07-22
			if (strlen(sPartNo) <= 0)
				logerr.Log("钢板信息%s,识别机制不能识别!", (char*)sText);
			else //件号相同但是钢板对应的entId不同，需要提醒用户 wht 19-04-24
				logerr.Log("钢板%s,件号重复请确认!", (char*)sText);
			continue;
		}
		CPlateProcessInfo* pPlateProcess = pModel->AppendPlate(sPartNo);
		pPlateProcess->m_bNeedExtract = TRUE;	//选择CAD实体包括当前件号时设置位需要提取
		pPlateProcess->dim_pos = dim_pos;
		pPlateProcess->dim_vec = dim_vec;
		if (bValidAttrBlock)
		{
			pPlateProcess->plateInfoBlockRefId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = baseInfo.m_cMat;
			pPlateProcess->xPlate.m_fThick = (float)baseInfo.m_nThick;
			pPlateProcess->xPlate.feature = baseInfo.m_nNum;
			pPlateProcess->xPlate.mkpos = dim_pos;
			pPlateProcess->xPlate.mkVec = dim_vec;
			if (pModel->m_sTaStampNo.GetLength() <= 0 && baseInfo.m_sTaStampNo.GetLength() > 0)
				pModel->m_sTaStampNo.Copy(baseInfo.m_sTaStampNo);
			if (pModel->m_sTaType.GetLength() <= 0 && baseInfo.m_sTaType.GetLength() > 0)
				pModel->m_sTaType.Copy(baseInfo.m_sTaType);
			if (pModel->m_sPrjCode.GetLength() <= 0 && baseInfo.m_sPrjCode.GetLength() > 0)
				pModel->m_sPrjCode.Copy(baseInfo.m_sPrjCode);
		}
		else
		{
			pPlateProcess->partNoId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = 'S';
		}
	}
	if(pModel->GetPlateNum()<=0)
	{
		logerr.Log("识别机制不能识别该文件的钢板信息!");
		return;
	}
	//设置备用提取位置（用于处理因钢板过小，文字标注放到钢板外的情况）wht 19-02-01
	const int HOLE_SEARCH_SCOPE = 60;
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE))
	{
		for(GEPOINT *pPt=holePosList.GetFirst();pPt;pPt=holePosList.GetNext())
		{
			if(DISTANCE(pPlateProcess->dim_pos,*pPt)<HOLE_SEARCH_SCOPE)
			{
				pPlateProcess->inner_dim_pos=*pPt;
				pPlateProcess->m_bHasInnerDimPos=TRUE;
				break;
			}
		}
	}
	//提取钢板的轮廓边
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		pModel->ExtractPlateProfileEx(selectedEntList);
	else
		pModel->ExtractPlateProfile(selectedEntList);
#ifndef __UBOM_ONLY_
	//处理钢板一板多号的情况
	pModel->MergeManyPartNo();
	//根据轮廓闭合区域更新钢板的基本信息+螺栓信息+轮廓边信息
	int nSum=0,nValid=0;
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE),nSum++)
	{
		pPlateProcess->ExtractPlateRelaEnts();
		if(!pPlateProcess->UpdatePlateInfo())
			logerr.Log("件号%s板选择了错误的边界,请重新选择.(位置：%s)",(char*)pPlateProcess->GetPartNo(),(char*)CXhChar50(pPlateProcess->dim_pos));
		else
			nValid++;
	}
	//将提取的钢板信息导出到中性文件中
	CString file_path;
	GetCurWorkPath(file_path);
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE))
	{	//生成PPI文件,保存到到当前工作路径下
		if(pPlateProcess->vertexList.GetNodeNum()>3)
			pPlateProcess->CreatePPiFile(file_path);
	}
	//绘制提取的钢板外形--支持排版
	pModel->LayoutPlates(g_pncSysPara.m_bAutoLayout);
	//完成提取后统一设置钢板提取状态为FALSE wht 19-06-17
	for (CPlateProcessInfo* pPlateProcess = pModel->EnumFirstPlate(TRUE); pPlateProcess; pPlateProcess = pModel->EnumNextPlate(TRUE))
	{	
		pPlateProcess->m_bNeedExtract = FALSE;
	}
	//写工程塔型配置文件 wht 19-01-12
	CString cfg_path;
	cfg_path.Format("%sconfig.ini",file_path);
	pModel->WritePrjTowerInfoToCfgFile(cfg_path);
	//
	UpdatePartList();
	//
	AfxMessageBox(CXhChar100("共提取钢板%d个，成功%d个，失败%d!",nSum,nValid,nSum-nValid));
#else
	//UBOM只需要更新钢板的基本信息
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		acdbOpenAcDbEntity(pEnt, objId, AcDb::kForRead);
		CAcDbObjLife objLife(pEnt);
		if (pEnt == NULL)
			continue;
		if (!pEnt->isKindOf(AcDbText::desc()))
			continue;
		GEPOINT dim_pos;
		CXhChar500 sText;
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			double fTestH = pText->height();
			double fWidth = TestDrawTextLength(sText, fTestH, pText->textStyle());
			AcDb::TextHorzMode  horzMode = pText->horizontalMode();
			AcDb::TextVertMode  vertMode = pText->verticalMode();
			AcGePoint3d txt_pos;
			if (vertMode == AcDb::kTextBase)
			{
				if (horzMode == AcDb::kTextLeft || horzMode == AcDb::kTextAlign || horzMode == AcDb::kTextFit)
					txt_pos = pText->position();
				else
					txt_pos = pText->alignmentPoint();
			}
			else
				txt_pos = pText->alignmentPoint();
			GEPOINT vec(cos(pText->rotation()), sin(pText->rotation()));
			dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
			if (horzMode == AcDb::kTextLeft)
				dim_pos += vec * (fWidth*0.5);
			else if (horzMode == AcDb::kTextRight)
				dim_pos -= vec * (fWidth*0.5);
		}
		else if (pEnt->isKindOf(AcDbMText::desc()))
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pMText->contents()));
#else
			sText.Copy(pMText->contents());
#endif
			double fTestH = pMText->actualHeight();
			double fWidth = pMText->actualWidth();
			GEPOINT vec(cos(pMText->rotation()), sin(pMText->rotation()));
			AcDbMText::AttachmentPoint align_type = pMText->attachment();
			dim_pos.Set(pMText->location().x, pMText->location().y, 0);
			if (align_type == AcDbMText::kTopLeft || align_type == AcDbMText::kMiddleLeft || align_type == AcDbMText::kBottomLeft)
				dim_pos += vec * (fWidth*0.5);
			else if (align_type == AcDbMText::kTopRight || align_type == AcDbMText::kMiddleRight || align_type == AcDbMText::kBottomRight)
				dim_pos -= vec * (fWidth*0.5);
		}
		CPlateProcessInfo* pPlateInfo = pModel->GetPlateInfo(dim_pos),*pTemPlate=NULL;
		if (pPlateInfo)
		{
			BASIC_INFO baseInfo;
			if (g_pncSysPara.RecogBasicInfo(pEnt, baseInfo))
			{
				if (baseInfo.m_sPartNo.GetLength() > 0&&
					(pTemPlate = pModel->GetPlateInfo(baseInfo.m_sPartNo)))
				{
					pPlateInfo = pTemPlate;
					pPlateInfo->xBomPlate.sPartNo = baseInfo.m_sPartNo;
				}
				if (baseInfo.m_cMat > 0)
					pPlateInfo->xBomPlate.cMaterial = baseInfo.m_cMat;
				if (baseInfo.m_cQuality > 0)
					pPlateInfo->xBomPlate.cQualityLevel = baseInfo.m_cQuality;
				if (baseInfo.m_nThick > 0)
					pPlateInfo->xBomPlate.thick = (float)baseInfo.m_nThick;
				if (baseInfo.m_nNum > 0)
					pPlateInfo->xBomPlate.SetPartNum(baseInfo.m_nNum);
				if (baseInfo.m_idCadEntNum != 0)
					pPlateInfo->partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			}
			else
			{
				if (strstr(sText, "卷边") || strstr(sText, "火曲") || strstr(sText, "外曲") || strstr(sText, "内曲"))
					pPlateInfo->xBomPlate.siZhiWan = 1;
				if (strstr(sText,"焊接"))
					pPlateInfo->xBomPlate.bWeldPart = TRUE;
			}
		}
	}
	for (CPlateProcessInfo* pPlateInfo = pModel->EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = pModel->EnumNextPlate(FALSE))
	{
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("钢板%s信息提取失败!", (char*)pPlateInfo->xPlate.GetPartNo());
		else
			pPlateInfo->xBomPlate.sSpec.Printf("-%.f", pPlateInfo->xBomPlate.thick);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//改正钢板的信息
//ReviseThePlate
//////////////////////////////////////////////////////////////////////////
void ManualExtractPlate()
{
	CLogErrorLife logErrLife;
	CHashSet<AcDbObjectId> selectedEntList;
	//框选待修正钢板的轮廓边
	ads_name ent_sel_set;
	int retCode=0;
#ifdef _ARX_2007
	retCode=acedSSGet(L"",NULL,NULL,NULL,ent_sel_set);
#else
	retCode=acedSSGet("",NULL,NULL,NULL,ent_sel_set);
#endif
	if(retCode==RTCAN)
	{	//用户按ESC取消
		acedSSFree(ent_sel_set);
		return;
	}
	else
	{	
		long ll;
		acedSSLength(ent_sel_set,&ll);
		for(long i=0;i<ll;i++)
		{	//根据文字说明初始化点集合
			ads_name entname;
			acedSSName(ent_sel_set,i,entname);
			AcDbObjectId entId;
			acdbGetObjectId(entId,entname);
			AcDbEntity* pEnt;
			acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
			if(pEnt==NULL)
				continue;
			selectedEntList.SetValue(entId.asOldId(),entId);
			pEnt->close();
		}
	}
	acedSSFree(ent_sel_set);
	//根据选择集合提取钢板的轮廓边，初始化顶点和螺栓信息
	CPlateProcessInfo plate;
	if(!plate.InitProfileBySelEnts(selectedEntList))
	{
		logerr.Log("选择的轮廓边有误，构不成有效闭合区域！");
		return;
	}
	//根据轮廓闭合区域更新基本信息+螺栓信息+轮廓边信息
	plate.ExtractPlateRelaEnts();
	if(!plate.UpdatePlateInfo(TRUE))
	{
		logerr.Log("件号%s板提取有误",(char*)plate.GetPartNo());
		return;
	}
	//在模型列表中添加钢板信息 wht 19-12-21
	CPlateProcessInfo *pExistPlate = model.GetPlateInfo(plate.GetPartNo());
	if (pExistPlate == NULL)
		pExistPlate = model.AppendPlate(plate.GetPartNo());
	pExistPlate->InitProfileBySelEnts(selectedEntList);
	pExistPlate->ExtractPlateRelaEnts();
	pExistPlate->UpdatePlateInfo(TRUE);

	//生成PPI文件,保存到到当前工作路径下
	CString file_path;
	GetCurWorkPath(file_path);
	if(pExistPlate->vertexList.GetNodeNum()>3)
		pExistPlate->CreatePPiFile(file_path);
	//绘制提取的钢板外形--支持排版
	model.LayoutPlates(g_pncSysPara.m_bAutoLayout);
	//
	UpdatePartList();
	/*
	//绘制钢板外形及螺栓，便于查看提取是否正确
	DRAGSET.ClearEntSet();
	plate.DrawPlate(NULL);
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base,"请点取构件图的插入点");
	*/
}

#ifndef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//编辑钢板信息
//CreatePartEditProcess
//WriteToClient
//////////////////////////////////////////////////////////////////////////
BOOL CreatePartEditProcess( HANDLE hClientPipeRead, HANDLE hClientPipeWrite )
{
	TCHAR cmd_str[MAX_PATH];
	GetAppPath(cmd_str);
	strcat(cmd_str,"PPE.exe -child");

	STARTUPINFO startInfo;
	memset( &startInfo, 0 , sizeof( STARTUPINFO ) );

	startInfo.cb= sizeof( STARTUPINFO );
	startInfo.dwFlags |= STARTF_USESTDHANDLES;
	startInfo.hStdError= 0;;
	startInfo.hStdInput= hClientPipeRead;
	startInfo.hStdOutput= hClientPipeWrite;

	PROCESS_INFORMATION processInfo;
	memset( &processInfo, 0, sizeof(PROCESS_INFORMATION) );
	BOOL b=CreateProcess( NULL,cmd_str,
		NULL,NULL, TRUE,CREATE_NEW_CONSOLE, NULL, NULL,&startInfo,&processInfo );
	DWORD er=GetLastError();
	return b;
}
BOOL WriteToClient(HANDLE hPipeWrite)
{
	if( hPipeWrite == INVALID_HANDLE_VALUE )
		return FALSE;
	CBuffer buffer(10000);	//10Kb
	DWORD version[2]={0,20170615};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=2;
	pByteVer[2]=0;
	pByteVer[3]=0;
	BYTE cProductType=PRODUCT_PNC;
	//1、写入加密狗证书信息
	char APP_PATH[MAX_PATH];
	GetAppPath(APP_PATH);
	buffer.WriteByte(cProductType);
	buffer.WriteDword(version[0]);
	buffer.WriteDword(version[1]);
	buffer.WriteString(APP_PATH,MAX_PATH);
	//2、写入PPE工作模式信息
	CString sFilePath;
	if(GetCurWorkPath(sFilePath,FALSE)==FALSE)
	{
		buffer.WriteByte(0);
		buffer.WriteInteger(0);
	}
	else
	{
		buffer.WriteByte(6);
		CBuffer file_buffer(10000);
		file_buffer.WriteString(sFilePath);	//写入PPI文件路径
		buffer.WriteDword(file_buffer.GetLength());
		buffer.Write(file_buffer.GetBufferPtr(),file_buffer.GetLength());
	}
	//4、向匿名管道中写入数据
	return buffer.WriteToPipe(hPipeWrite,1024);
}
void SendPartEditor()
{
	CLogErrorLife logErrLife;
	//1、创建第一个管道: 用于服务器端向客户端发送内容
	SECURITY_ATTRIBUTES attrib;
	attrib.nLength = sizeof( SECURITY_ATTRIBUTES );
	attrib.bInheritHandle= true;
	attrib.lpSecurityDescriptor = NULL;
	HANDLE hPipeClientRead=NULL, hPipeSrcWrite=NULL;
	if( !CreatePipe( &hPipeClientRead, &hPipeSrcWrite, &attrib, 0 ) )
	{
		logerr.Log("创建匿名管道失败!GetLastError= %d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcWriteDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcWrite, GetCurrentProcess(), &hPipeSrcWriteDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("复制句柄失败,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcWrite );
	//2、创建第二个管道，用于客户端向服务器端发送内容
	HANDLE hPipeClientWrite=NULL, hPipeSrcRead=NULL;
	if( !CreatePipe( &hPipeSrcRead, &hPipeClientWrite, &attrib, 0) )
	{
		logerr.Log("创建第二个匿名管道失败,GetLastError=%d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcReadDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcRead, GetCurrentProcess(), &hPipeSrcReadDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("复制第二个句柄失败,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcRead );
	//3、创建子进程,
	if( !CreatePartEditProcess( hPipeClientRead, hPipeClientWrite ) )
	{
		logerr.Log("创建子进程失败\n" );
		return;
	}
	//4、进行数据操作---支持传送多个构件
	if( !WriteToClient(hPipeSrcWriteDup))
	{
		logerr.Log("数据传输失败");
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
//钢板布局,自动排版
//LayoutPlates
//////////////////////////////////////////////////////////////////////////
void LayoutPlates()
{	
	CLogErrorLife logeErrLife;
	model.LayoutPlates(TRUE);
}
//////////////////////////////////////////////////////////////////////////
//实时插入钢印区
//InsertMKRect		
//////////////////////////////////////////////////////////////////////////
void InsertMKRect()
{
	CAcModuleResourceOverride resOverride;
	CXhChar16 blkname("MK");
	AcDbObjectId blockId=SearchBlock(blkname);
	if(blockId.isNull())
	{	//新建块表记录指针，生成钢印标记图元
		double txt_height=2;
		int nRectL=60,nRectW=30;
		CInputMKRectDlg dlg;
		dlg.m_nRectL=nRectL;
		dlg.m_nRectW=nRectW;
		dlg.m_fTextH=txt_height;
		if(dlg.DoModal()==IDOK)
		{
			nRectL=dlg.m_nRectL;
			nRectW=dlg.m_nRectW;
			txt_height=dlg.m_fTextH;
		}
		DRAGSET.ClearEntSet();
		AcDbBlockTable *pTempBlockTable;
		GetCurDwg()->getBlockTable(pTempBlockTable,AcDb::kForWrite);
		AcDbBlockTableRecord *pTempBlockTableRecord=new AcDbBlockTableRecord();//定义块表记录指针
#ifdef _ARX_2007
		pTempBlockTableRecord->setName((ACHAR*)_bstr_t(CXhChar16("MK")));
#else
		pTempBlockTableRecord->setName(CXhChar16("MK"));
#endif
		pTempBlockTable->add(blockId,pTempBlockTableRecord);
		pTempBlockTable->close();
		//生成钢印标记图元
		f3dPoint topLeft(0,nRectW),dim_pos(nRectL*0.5,nRectW*0.5,0);
		CreateAcadRect(pTempBlockTableRecord,topLeft,nRectL,nRectW);	//插入矩形区
		DimText(pTempBlockTableRecord,dim_pos,CXhChar16("钢印区"),TextStyleTable::hzfs.textStyleId,txt_height,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pTempBlockTableRecord->close();
	}
	//将钢印图元生成图块，提示用户插入指定位置
	AcDbBlockTableRecord* pBlockTableRecord=GetBlockTableRecord();
	while(1)
	{
		DRAGSET.ClearEntSet();
		f3dPoint insert_pos;
		AcDbObjectId newEntId;
		AcDbBlockReference *pBlkRef = new AcDbBlockReference;
		AcGeScale3d scaleXYZ(1.0,1.0,1.0);
		pBlkRef->setBlockTableRecord(blockId);
		pBlkRef->setPosition(AcGePoint3d(insert_pos.x,insert_pos.y,0));
		pBlkRef->setRotation(0);
		pBlkRef->setScaleFactors(scaleXYZ);
		DRAGSET.AppendAcDbEntity(pBlockTableRecord,newEntId,pBlkRef);
		pBlkRef->close();
#ifdef AFX_TARG_ENU_ENGLISH
		int retCode=DragEntSet(insert_pos,"input the insertion point\n");
#else
		int retCode=DragEntSet(insert_pos,"输入插入点\n");
#endif
		if(retCode==RTCAN)
			break;
	}
	pBlockTableRecord->close();
#ifdef _ARX_2007
	ads_command(RTSTR,L"RE",RTNONE);
#else
	ads_command(RTSTR,"RE",RTNONE);
#endif
}
//////////////////////////////////////////////////////////////////////////
//通过读取Txt文件绘制外形
//ReadVertexArrFromFile
//////////////////////////////////////////////////////////////////////////
bool ReadVertexArrFromFile(const char* filePath, fPtList &ptList)
{
	if (filePath == NULL)
		return false;
	FILE *fp = fopen(filePath, "rt");
	if (fp == NULL)
		return false;
	char line_txt[200] = { 0 };
	CXhChar200 sLine;
	f3dPoint *pPrevPt = NULL;
	while (!feof(fp))
	{
		if (fgets(line_txt, 200, fp) == NULL)
			continue;
		strcpy(sLine, line_txt);
		sLine.Replace('X', ' ');
		sLine.Replace('Y', ' ');
		sLine.Replace('I', ' ');
		sLine.Replace('J', ' ');
		strcpy(line_txt, sLine);
		char szTokens[] = " =\n";
		double x = 0, y = 0;
		char* skey = strtok(line_txt, szTokens);
		if (skey == NULL)
			continue;
		x = atof(skey);
		skey = strtok(NULL, szTokens);
		if (skey == NULL)
			y = 0;
		else
			y = atof(skey);
		f3dPoint *pPt = ptList.append();
		if (pPrevPt != NULL)
		{
			pPt->x = pPrevPt->x + x;
			pPt->y = pPrevPt->y + y;
		}
		else
		{
			pPt->x = x;
			pPt->y = y;
		}
		pPrevPt = pPt;
	}
	fclose(fp);
	return true;
}

void DrawProfileByTxtFile()
{	//1.读取轮廓顶点
	fPtList ptList;
	ReadVertexArrFromFile("D:\\Text.txt", ptList);
	ARRAY_LIST<f3dPoint> ptArr;
	for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
	{
		ptArr.append(*pPt);
	}
	//2.绘制外形
	DRAGSET.ClearEntSet();
	AcDbBlockTableRecord *pBlockTblRec = GetBlockTableRecord();
	CreateAcadPolyLine(pBlockTblRec, ptArr.Data(), ptArr.Count, 1);
	pBlockTblRec->close();
	ads_point insert_pos;
#ifdef AFX_TARG_ENU_ENGLISH
	int retCode = DragEntSet(insert_pos, "input the insertion point\n");
#else
	int retCode = DragEntSet(insert_pos, "输入插入点\n");
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////
//系统设置
//EnvGeneralSet
//////////////////////////////////////////////////////////////////////////
void EnvGeneralSet()
{
	CAcModuleResourceOverride resOverride;
	CLogErrorLife logErrLife;
	CPNCSysSettingDlg dlg;
	dlg.DoModal();
}
//////////////////////////////////////////////////////////////////////////
//校审构件工艺信息
//RevisionPartProcess
//////////////////////////////////////////////////////////////////////////
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
void RevisionPartProcess()
{
#ifdef __PNC_
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("无UBOM对料授权，请联系供应商！");
		return;
	}
#else
	if (!VerifyValidFunction(TMA_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("无UBOM对料授权，请联系供应商！");
		return;
	}
#endif
	CLogErrorLife logErrLife;
	char APP_PATH[MAX_PATH]="", sJgCardPath[MAX_PATH]="";
	GetAppPath(APP_PATH);
	sprintf(sJgCardPath, "%s%s", APP_PATH, (char*)g_pncSysPara.m_sJgCadName);
	if(!g_pncSysPara.InitJgCardInfo(sJgCardPath))
	{
		logerr.Log(CXhChar200("角钢工艺卡读取失败(%s)!",sJgCardPath));
		return;
	}
	g_xUbomModel.InitBomTblCfg();
	g_pRevisionDlg->InitRevisionDlg();
}
#endif