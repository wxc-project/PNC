#include "StdAfx.h"
#include "BomModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "ArrayList.h"
#include "PNCCmd.h"
#include "PNCSysPara.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
CAngleProcessInfo::CAngleProcessInfo()
{
	keyId=NULL;
	partNumId=NULL;
}
CAngleProcessInfo::~CAngleProcessInfo()
{

}
//生成角钢工艺卡区域
void CAngleProcessInfo::CreateRgn()
{
	f3dPoint pt;
	ARRAY_LIST<f3dPoint> profileVertexList;
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	region.CreatePolygonRgn(profileVertexList.m_pData,profileVertexList.GetSize());
}
//判断坐标点是否在角钢工艺卡内
bool CAngleProcessInfo::PtInAngleRgn(const double* poscoord)
{
	if(region.GetAxisZ().IsZero())
		return false;
	int nRetCode=region.PtInRgn(poscoord);
	return nRetCode==1;
}
//根据数据点类型获取数据所在区域
f2dRect CAngleProcessInfo::GetAngleDataRect(BYTE data_type)
{
	f2dRect rect;
	if (data_type == ITEM_TYPE_PART_NO)
	{	//有的件号过长，件号文字放到了方框下方，故需增大方框的区域
		rect = g_pncSysPara.part_no_rect;
		rect.bottomRight.y -= g_pncSysPara.part_no_rect.Height();
	}
	else if (data_type == ITEM_TYPE_DES_MAT)
		rect = g_pncSysPara.mat_rect;
	else if (data_type == ITEM_TYPE_DES_GUIGE)
		rect = g_pncSysPara.guige_rect;
	else if (data_type == ITEM_TYPE_LENGTH)
		rect = g_pncSysPara.length_rect;
	else if (data_type == ITEM_TYPE_PIECE_WEIGHT)
		rect = g_pncSysPara.piece_weight_rect;
	else if (data_type == ITEM_TYPE_SUM_PART_NUM)
		rect = g_pncSysPara.jiagong_num_rect;
	else if (data_type == ITEM_TYPE_PART_NUM)
		rect = g_pncSysPara.danji_num_rect;
	else if (data_type == ITEM_TYPE_PART_NOTES)
		rect = g_pncSysPara.note_rect;
	else if (data_type == ITEM_TYPE_SUM_WEIGHT)
		rect = g_pncSysPara.sum_weight_rect;
	rect.topLeft.x+=orig_pt.x;
	rect.topLeft.y+=orig_pt.y;
	rect.bottomRight.x+=orig_pt.x;
	rect.bottomRight.y+=orig_pt.y;
	return rect;
}
//判断坐标点是否在指定类型的数据框中
bool CAngleProcessInfo::PtInDataRect(BYTE data_type,f3dPoint pos)
{
	f2dRect rect=GetAngleDataRect(data_type);
	f2dPoint pt(pos.x,pos.y);
	if(rect.PtInRect(pt))
		return true;
	else
		return false;
}
//初始化角钢信息
BYTE CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos,const char* sValue)
{
	BYTE cType = 0;
	if (PtInDataRect(ITEM_TYPE_PART_NO, data_pos))	//件号
	{
		m_xAngle.SetPartNo(sValue);
		cType = ITEM_TYPE_PART_NO;
	}
	else if (PtInDataRect(ITEM_TYPE_DES_MAT, data_pos))	//材质
	{
		m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sValue);
		if (strlen(sValue) == 5)	//初始化质量等级 wht 19-09-15
			m_xAngle.cQuality = toupper(sValue[4]);
		cType = ITEM_TYPE_DES_MAT;
	}
	else if(PtInDataRect(ITEM_TYPE_DES_GUIGE,data_pos))	//规格
	{	
		CXhChar50 sSpec(sValue);
		if(strstr(sSpec,"∠"))
			sSpec.Replace("∠","L");
		if(strstr(sSpec,"×"))
			sSpec.Replace("×","*");
		m_xAngle.SetSpec(sSpec);
		int nWidth=0,nThick=0;
		//从规格中提取材质 wht 19-08-05
		CXhChar16 sMaterial;
		CProcessPart::RestoreSpec(sSpec,&nWidth,&nThick,sMaterial);
		if (sMaterial.GetLength() > 0)
		{
			m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			m_xAngle.cQuality = CProcessPart::QueryBriefQuality(sMaterial);
		}
		m_xAngle.m_fWidth=(float)nWidth;
		m_xAngle.m_fThick=(float)nThick;
		cType = ITEM_TYPE_DES_GUIGE;
	}
	else if (PtInDataRect(ITEM_TYPE_LENGTH, data_pos))	//长度
	{
		m_xAngle.m_wLength = atoi(sValue);
		cType = ITEM_TYPE_LENGTH;
	}
	else if (PtInDataRect(ITEM_TYPE_PIECE_WEIGHT, data_pos))	//单重
	{
		m_xAngle.m_fWeight = (float)atof(sValue);
		cType = ITEM_TYPE_PIECE_WEIGHT;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NUM, data_pos))	//单基数
	{
		m_xAngle.m_nDanJiNum = atoi(sValue);
		cType = ITEM_TYPE_PART_NUM;
	}
	else if(PtInDataRect(ITEM_TYPE_SUM_PART_NUM,data_pos))	//加工数
	{
		m_xAngle.feature=atoi(sValue);
		cType = ITEM_TYPE_SUM_PART_NUM;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))	//备注
	{
		m_xAngle.SetNotes(sValue);
		cType = ITEM_TYPE_PART_NOTES;
	}
	else if (PtInDataRect(ITEM_TYPE_SUM_WEIGHT, data_pos))	//总重
	{
		m_xAngle.m_fSumWeight = (float)atof(sValue);
		cType = ITEM_TYPE_SUM_WEIGHT;
	}
	return cType;
}
//获取角钢数据点坐标
f3dPoint CAngleProcessInfo::GetAngleDataPos(BYTE data_type)
{
	f2dRect rect=GetAngleDataRect(data_type);
	double fx=(rect.topLeft.x+rect.bottomRight.x)*0.5;
	double fy=(rect.topLeft.y+rect.bottomRight.y)*0.5;
	return f3dPoint(fx,fy,0);
}
//更新角钢的加工数据
void CAngleProcessInfo::RefreshAngleNum()
{
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	f3dPoint data_pt=GetAngleDataPos(ITEM_TYPE_SUM_PART_NUM);
	CXhChar16 sPartNum("%d",m_xAngle.feature);
	if(partNumId==NULL)
	{	//添加角钢加工数
		acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
		AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
		if(pBlockTableRecord==NULL)
		{
			logerr.Log("块表打开失败");
			return;
		}
		DimText(pBlockTableRecord,data_pt,sPartNum,TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pBlockTableRecord->close();//关闭块表
		acDocManager->unlockDocument(curDoc());
	}
	else
	{	//改写角钢加工数
		acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,partNumId,AcDb::kForWrite);
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sPartNum));
#else
			pText->setTextString(sPartNum);
#endif
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
		}
		pEnt->close();
		acDocManager->unlockDocument(curDoc());
	}
}
void CAngleProcessInfo::RefreshAngleSumWeight()
{
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_SUM_WEIGHT);
	CXhChar16 sSumWeight("%.f", m_xAngle.m_fSumWeight);
	CLockDocumentLife lockDoc;
	if (sumWeightId == NULL)
	{	//添加角钢总重
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("块表打开失败");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sSumWeight, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		pBlockTableRecord->close();//关闭块表
	}
	else
	{	//改写角钢加工数
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, sumWeightId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sSumWeight));
#else
			pText->setTextString(sSumWeight);
#endif
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sSumWeight));
#else
			pMText->setContents(sSumWeight);
#endif
		}
		pEnt->close();
	}
}
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
CDwgFileInfo::CDwgFileInfo()
{
	m_bJgDwgFile=FALSE;
}
CDwgFileInfo::~CDwgFileInfo()
{

}
//初始化DWG文件信息
BOOL CDwgFileInfo::InitDwgInfo(const char* sFileName,BOOL bJgDxf)
{
	if(strlen(sFileName)<=0)
		return FALSE;
	m_bJgDwgFile=bJgDxf;
	m_sFileName.Copy(sFileName);
	if (m_bJgDwgFile)
		return RetrieveAngles();
	else
		return RetrievePlates();
}
//////////////////////////////////////////////////////////////////////////
//钢板DWG操作
//////////////////////////////////////////////////////////////////////////
//根据数据点坐标查找对应的钢板
CPlateProcessInfo* CDwgFileInfo::FindPlateByPt(f3dPoint text_pos)
{
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=m_xPncMode.EnumFirstPlate(FALSE);pPlateInfo;pPlateInfo=m_xPncMode.EnumNextPlate(FALSE))
	{
		if(pPlateInfo->IsInPlate(text_pos))
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
	CPlateProcessInfo* pInfo=NULL;
	CProcessPlate* pProcessPlate=NULL;
	BOOL bFinish=TRUE;
	for(pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate())
	{
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		pProcessPlate=(CProcessPlate*)m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pProcessPlate==NULL)
		{
			bFinish=FALSE;
			logerr.Log("TMA放样材料表中没有%s钢板",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			bFinish=FALSE;
			logerr.Log("%s钢板件数修改失败!",(char*)sPartNo);
			continue;
		}
		if(pInfo->xPlate.feature!=pProcessPlate->feature)
		{	//加工数不同进行修改
			pInfo->xPlate.feature=pProcessPlate->feature;	//加工数
			pInfo->RefreshPlateNum();
		}
	}
	if(bFinish)
		AfxMessageBox("钢板加工数修改完毕!");
}
//得到显示图元集合
int CDwgFileInfo::GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet)
{
	entSet.Empty();
	long ll=0;
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId;
	ads_name ent_sel_set,entname;
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	//初始化实体集合
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		entSet.SetValue(entId.handle(),entId);
		pEnt->close();
	}
	acedSSFree(ent_sel_set);
	return entSet.GetNodeNum();
}
//获取实体的线性ID
AcDbObjectId CDwgFileInfo::GetEntLineTypeId(AcDbEntity *pEnt)
{
	if (pEnt == NULL)
		return NULL;
	CXhChar50 sLineTypeName;
#ifdef _ARX_2007
	ACHAR* sValue = new ACHAR[50];
	sValue = pEnt->linetype();
	sLineTypeName.Copy((char*)_bstr_t(sValue));
	delete[] sValue;
#else
	char *sValue = new char[50];
	sValue = pEnt->linetype();
	sLineTypeName.Copy(sValue);
	delete[] sValue;
#endif
	AcDbObjectId linetypeId;
	if (stricmp(sLineTypeName, "ByLayer") == 0)
	{	//线型随层
		AcDbLayerTableRecord *pLayerTableRecord;
		acdbOpenObject(pLayerTableRecord, pEnt->layerId(), AcDb::kForRead);
		pLayerTableRecord->close();
		linetypeId = pLayerTableRecord->linetypeObjectId();
	}
	else if (stricmp(sLineTypeName, "ByBlock") == 0)
		linetypeId = m_idSolidLine;		//如果图元的线型类型为ByBlock,则线型就是实线
	else
		linetypeId = pEnt->linetypeId();
	return linetypeId;
}

//提取板的轮廓边,确定闭合区域
BOOL CDwgFileInfo::RetrievePlates()
{
	//SmartExtractPlate();
	return TRUE;
}

BOOL CDwgFileInfo::ReviseThePlate(const char* sPartNo)
{
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//角钢DWG文件操作
//////////////////////////////////////////////////////////////////////////
void CDwgFileInfo::CorrectAngles()
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		if(sPartNo.GetLength()<=0)
			m_hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		if (pJgInfo->m_xAngle.cMaterial == 0)
			pJgInfo->m_xAngle.cMaterial = 'S';
	}
	m_hashJgInfo.Clean();
}
//根据数据点坐标查找所对应角钢
CAngleProcessInfo* CDwgFileInfo::FindAngleByPt(f3dPoint data_pos)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(pJgInfo->PtInAngleRgn(data_pos))
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
		if(stricmp(pJgInfo->m_xAngle.GetPartNo(),sPartNo)==0)
			break;
	}
	return pJgInfo;
}
//更新角钢加工数
void CDwgFileInfo::ModifyAngleDwgPartNum()
{
	if(m_hashJgInfo.GetNodeNum()<=0)
		return;
	CAngleProcessInfo* pJgInfo=NULL;
	CProcessAngle* pProcessJg=NULL;
	BOOL bFinish=TRUE;
	for(pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		pProcessJg=(CProcessAngle*)m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pProcessJg==NULL)
		{	
			bFinish=FALSE;
			logerr.Log("TMA材料表中没有%s角钢",(char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.feature=pProcessJg->feature;	//加工数
		pJgInfo->m_xAngle.m_fSumWeight = pProcessJg->m_fSumWeight;
		pJgInfo->RefreshAngleNum();
		pJgInfo->RefreshAngleSumWeight();
	}
	if(bFinish)
		AfxMessageBox("角钢加工数修改完毕!");
}
//提取角钢操作
BOOL CDwgFileInfo::RetrieveAngles()
{
	CAcModuleResourceOverride resOverride;
	ads_name ent_sel_set,entname;
	CHashSet<AcDbObjectId> textIdHash;
	//根据工艺卡块识别角钢
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId,blockId;
	long ll;
	f3dPoint orig_pt;
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		CXhChar50 sText;
		if(pEnt->isKindOf(AcDbText::desc()))
		{	//角钢工艺卡非块，根据"件号"文字提取角钢信息
			textIdHash.SetValue(entId.handle(),entId);
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if((strstr(sText,"件")==NULL&&strstr(sText,"编")==NULL) || strstr(sText,"号")==NULL)
				continue;
			if (strstr(sText, "图号") != NULL || strstr(sText, "文件编号") != NULL)
				continue;
			orig_pt= g_pncSysPara.GetJgCardOrigin(f3dPoint(pText->position().x,pText->position().y,0));
		}
		else if(pEnt->isKindOf(AcDbMText::desc()))
		{	//角钢工艺卡非块，根据"件号"文字提取角钢信息
			textIdHash.SetValue(entId.handle(),entId);
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pMText->contents()));
#else
			sText.Copy(pMText->contents());
#endif
			if ((strstr(sText, "件") == NULL && strstr(sText, "编") == NULL) || strstr(sText, "号") == NULL)
				continue;
			if (strstr(sText, "图号") != NULL || strstr(sText, "文件编号") != NULL)
				continue;
			orig_pt= g_pncSysPara.GetJgCardOrigin(f3dPoint(pMText->location().x,pMText->location().y,0));
		}
		else if(pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//根据角钢工艺卡块提取角钢信息
			AcDbBlockTableRecord *pTempBlockTableRecord=NULL;
			AcDbBlockReference* pReference=(AcDbBlockReference*)pEnt;
			blockId=pReference->blockTableRecord();
			acdbOpenObject(pTempBlockTableRecord,blockId,AcDb::kForRead);
			if(pTempBlockTableRecord==NULL)
				continue;
			pTempBlockTableRecord->close();
			CXhChar50 sName;
#ifdef _ARX_2007
			ACHAR* sValue=new ACHAR[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy((char*)_bstr_t(sValue));
			delete[] sValue;
#else
			char *sValue=new char[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy(sValue);
			delete[] sValue;
#endif
			if(strcmp(sName,"JgCard")!=0)
				continue;
			orig_pt.Set(pReference->position().x,pReference->position().y,0);
		}
		else
			continue;
		//添加角钢记录
		CAngleProcessInfo* pJgInfo=NULL;
		pJgInfo=m_hashJgInfo.Add(entId.handle());
		pJgInfo->keyId=entId;
		pJgInfo->m_xAngle.cMaterial='S';
		pJgInfo->SetOrig(orig_pt);
		pJgInfo->CreateRgn();
	}
	acedSSFree(ent_sel_set);
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s文件提取角钢失败",(char*)m_sFileName);
		return FALSE;
	}
	//根据角钢数据位置获取角钢信息
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		CXhChar50 sValue;
		f3dPoint text_pos;
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
			text_pos.Set(pText->alignmentPoint().x,pText->alignmentPoint().y,pText->alignmentPoint().z);
			if(text_pos.IsZero())
				text_pos.Set(pText->position().x,pText->position().y,pText->position().z);
#ifdef _ARX_2007
			sValue.Copy(_bstr_t(pText->textString()));
#else
			sValue.Copy(pText->textString());
#endif
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
			text_pos.Set(pMText->location().x,pMText->location().y,pMText->location().z);//contents
#ifdef _ARX_2007
			sValue.Copy(_bstr_t(pMText->contents()));
#else
			sValue.Copy(pMText->contents());
#endif
		}
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
		}
	}
	//对提取的角钢信息进行合理性检查
	CorrectAngles();
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s文件提取角钢失败",(char*)m_sFileName);
		return FALSE;
	}
	return TRUE;
}
#endif