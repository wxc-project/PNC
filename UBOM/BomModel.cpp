#include "StdAfx.h"
#include "BomModel.h"
#include "ExcelOper.h"
#include "TblDef.h"
#include "DefCard.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "BomTblTitleCfg.h"

CBomModel g_xUbomModel;
//TMA_BOM EXCEL 列标题
const int TMA_EXCEL_COL_COUNT				= 20;
const char* T_TMA_PART_NO					= "图纸号码";
const char* T_TMA_SPEC						= "规格";
const char* T_TMA_LEN						= "长度";
const char* T_TMA_METERIAL					= "材质";
const char* T_TMA_SINGLEBASE_NUM			= "单基";
const char* T_TMA_PROCESS_NUM				= "加工";
const char* T_TMA_SINGLEPIECE_WEIGHT		= "单重";
const char* T_TMA_NOTE						= "备注";
//ERP_BOM EXCLE 列标题
const int ERP_EXCEL_COL_COUNT				= 12;
const char* T_ERP_PART_NO					= "部件名";
const char* T_ERP_PART_TYPE					= "材料";
const char* T_ERP_METERIAL					= "材质";
const char* T_ERP_LEN						= "长度";
const char* T_ERP_SPEC						= "规格";
const char* T_ERP_NUM						= "数量";
const char* T_ERP_SING_WEIGHT				= "单重";
const char* T_ERP_NOTE						= "备注";

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
static CProcessPart* CreatePart(int idPartClsType,const char* key,void* pContext)
{
	CProcessPart* pPart=NULL;
	if(idPartClsType==CGeneralPart::TYPE_LINEANGLE)
		pPart=new CProcessAngle();
	else if(idPartClsType==CGeneralPart::TYPE_PLATE)
		pPart=new CProcessPlate();
	else
		pPart=new CProcessPart();	
	return pPart;
}
static BOOL DestroyPart(CProcessPart* pPart)
{
	if(pPart->m_cPartType==CGeneralPart::TYPE_LINEANGLE)
		delete (CProcessAngle*)pPart;
	else if(pPart->m_cPartType==CGeneralPart::TYPE_PLATE)
		delete (CProcessPlate*)pPart;
	else
		delete (CProcessPart*)pPart;
	return TRUE;
}
int compare_func(const CXhChar16& str1,const CXhChar16& str2)
{
	CString keyStr1(str1),keyStr2(str2);
	return keyStr1.Compare(keyStr2);
}
//////////////////////////////////////////////////////////////////////////
//CBomFile
CBomFile::CBomFile()
{
	m_hashPartByPartNo.CreateNewAtom=CreatePart;
	m_hashPartByPartNo.DeleteAtom=DestroyPart;
	m_bLoftBom=TRUE;
	m_pProject=NULL;
}
CBomFile::~CBomFile()
{

}
void CBomFile::ImPortBomFile(const char* sFileName,BOOL bLoftBom)
{
	if(strlen(sFileName)<=0)
		return;
	m_sFileName.Copy(sFileName);
	m_bLoftBom=bLoftBom;
	if(m_bLoftBom)
		ImportTmaExcelFile();
	else
		ImportErpExcelFile();
}
//
void CBomFile::UpdateProcessPart(const char* sOldKey,const char* sNewKey)
{
	CProcessPart* pPart=m_hashPartByPartNo.GetValue(sOldKey);
	if(pPart==NULL)
	{
		logerr.Log("%s构件找不到",sOldKey);
		return;
	}
	pPart=m_hashPartByPartNo.ModifyKeyStr(sOldKey,sNewKey);
	if(pPart)
		pPart->SetPartNo(sNewKey);
}
//解析放样BOMSHEET内容
BOOL CBomFile::ParseTmaSheetContent(CVariant2dArray &sheetContentMap)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	int iStartRow = 5;
	int startRowArr[2] = {5,4};
	CHashStrList<DWORD> hashColIndexByColTitle;
	for (int j = 0; j < 2; j++)
	{
		hashColIndexByColTitle.Empty();
		int iTestStartRow = startRowArr[j];
		for (int i = 0; i < TMA_EXCEL_COL_COUNT; i++)
		{
			VARIANT value;
			sheetContentMap.GetValueAt(iTestStartRow, i, value);
			CString str(value.bstrVal);
			if ((str.CompareNoCase(T_TMA_PART_NO) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_PART_NO, str))
				hashColIndexByColTitle.SetValue(T_TMA_PART_NO, i);
			else if ((str.CompareNoCase(T_TMA_SPEC) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SPEC, str))
				hashColIndexByColTitle.SetValue(T_TMA_SPEC, i);
			else if ((str.CompareNoCase(T_TMA_LEN) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_LEN, str))
				hashColIndexByColTitle.SetValue(T_TMA_LEN, i);
			else if ((str.CompareNoCase(T_TMA_METERIAL) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_METERIAL, str))
				hashColIndexByColTitle.SetValue(T_TMA_METERIAL, i);
			else if ((str.CompareNoCase(T_TMA_SINGLEBASE_NUM) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_NUM, str))
				hashColIndexByColTitle.SetValue(T_TMA_SINGLEBASE_NUM, i);
			else if ((str.CompareNoCase(T_TMA_PROCESS_NUM) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_MANU_NUM, str))
				hashColIndexByColTitle.SetValue(T_TMA_PROCESS_NUM, i);
			else if ((str.CompareNoCase(T_TMA_SINGLEPIECE_WEIGHT) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SING_WEIGHT, str))
				hashColIndexByColTitle.SetValue(T_TMA_SINGLEPIECE_WEIGHT, i);
			else if ((str.CompareNoCase(T_TMA_NOTE) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_NOTES, str))
				hashColIndexByColTitle.SetValue(T_TMA_NOTE, i);
		}
		if (hashColIndexByColTitle.GetValue(T_TMA_PART_NO) &&		//件号
			hashColIndexByColTitle.GetValue(T_TMA_SPEC) &&			//规格
			hashColIndexByColTitle.GetValue(T_TMA_METERIAL))		//材质
		{
			iStartRow = startRowArr[j];
			break;
		}
	}
	return ParseTmaSheetContentCore(sheetContentMap, hashColIndexByColTitle, iStartRow);
}

BOOL CBomFile::ParseTmaSheetContentCore(CVariant2dArray &sheetContentMap, CHashStrList<DWORD> &hashColIndexByColTitle,int iStartRow)
{
	if( hashColIndexByColTitle.GetValue(T_TMA_PART_NO)==NULL||		//件号
		hashColIndexByColTitle.GetValue(T_TMA_SPEC)==NULL||			//规格
		hashColIndexByColTitle.GetValue(T_TMA_METERIAL)==NULL)		//材质
	{
		//logerr.Log("缺少关键列(件号或规格或材质或单基数)!");
		return FALSE;
	}
	//2.获取Excel所有单元格的值
	for(int i=iStartRow+2;i<= sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar100 sPartNo,sMeterial,sSpec,sNote;
		//件号
		DWORD *pColIndex=hashColIndexByColTitle.GetValue(T_TMA_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//规格
		int cls_id=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if(strstr(sSpec,"L")||strstr(sSpec,"∠"))
		{
			cls_id=CProcessPart::TYPE_LINEANGLE;
			if(strstr(sSpec,"∠"))
				sSpec.Replace("∠","L");
			if(strstr(sSpec,"×"))
				sSpec.Replace("×","*");
			if(strstr(sSpec,"x"))
				sSpec.Replace("x","*");
		}
		else if(strstr(sSpec,"-"))
			cls_id=CProcessPart::TYPE_PLATE;
		else //if(strstr(sSpec,"φ"))
			cls_id=CProcessPart::TYPE_LINETUBE;
		//长度
		float fLength=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_LEN);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fLength=(float)atof(VariantToString(value));
		//材质
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_METERIAL);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sMeterial=VariantToString(value);
		//单基数
		int nSingleNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_SINGLEBASE_NUM);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		nSingleNum=atoi(VariantToString(value));
		//加工数
		int nProcessNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_PROCESS_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nProcessNum = atoi(VariantToString(value));
		}
		//单基重量
		double fWeight=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_SINGLEPIECE_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		//备注
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_NOTE);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sNote=VariantToString(value);
		//填充哈希表
		CProcessPart* pProPart=m_hashPartByPartNo.Add(sPartNo,cls_id);
		pProPart->m_nDanJiNum=nSingleNum;
		pProPart->feature=nProcessNum;
		pProPart->SetPartNo(sPartNo);
		pProPart->SetSpec(sSpec);
		pProPart->SetNotes(sNote);
		pProPart->cMaterial=CBomModel::QueryBriefMatMark(sMeterial);
		if(pProPart->cMaterial=='A')	//非标准材质库，m_sRelatePartNo记录材质
			pProPart->m_sRelatePartNo.Copy(sMeterial);
		pProPart->m_wLength=(WORD)fLength;
		pProPart->m_fWeight=(float)fWeight;
		int nWidth=0,nThick=0;
		CBomModel::RestoreSpec(sSpec,&nWidth,&nThick);
		pProPart->m_fWidth=(float)nWidth;
		pProPart->m_fThick=(float)nThick;
	}
	return TRUE;
}
//导入放样料单EXCEL文件
BOOL CBomFile::ImportTmaExcelFile()
{
	if(m_sFileName.Length<=0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	LPDISPATCH pWorksheets=excelobj.GetWorksheets();
	if(pWorksheets==NULL)
		return FALSE;
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;	//工作表集合
	excel_sheets.AttachDispatch(pWorksheets);
	int nSheetNum=excel_sheets.GetCount();
	if(nSheetNum<1)
	{
		excel_sheets.ReleaseDispatch();
		return FALSE;
	}
	int index=1;
	int nValidSheetCount = 0;
	m_hashPartByPartNo.Empty();
	for(int iSheet=1;iSheet<=nSheetNum;iSheet++)
	{
		/*LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((short) iSheet));
		_Worksheet  excel_sheet;
		excel_sheet.AttachDispatch(pWorksheet);
		excel_sheet.Select();
		//1.获取Excel指定Sheet内容存储至sheetContentMap
		Range excel_usedRange,excel_range;
		excel_usedRange.AttachDispatch(excel_sheet.GetUsedRange());
		excel_range.AttachDispatch(excel_usedRange.GetRows());
		long nRowNum = excel_range.GetCount();
		excel_range.AttachDispatch(excel_usedRange.GetColumns());
		long nColNum = excel_range.GetCount();
		if(nColNum<TMA_EXCEL_COL_COUNT || strstr(excel_sheet.GetName(),"表头"))
			continue;
		char cell[20]="";
		sprintf(cell,"%C%d",'A'+TMA_EXCEL_COL_COUNT-1,nRowNum+2);
		LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"),COleVariant(cell));
		excel_range.AttachDispatch(pRange,FALSE);
		CVariant2dArray sheetContentMap(1,1);
		sheetContentMap.var=excel_range.GetValue();
		excel_range.ReleaseDispatch();*/
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet);
		//2、解析数据
		if (ParseTmaSheetContent(sheetContentMap))
			nValidSheetCount++;
		//excel_sheet.ReleaseDispatch();
	}
	excel_sheets.ReleaseDispatch();
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
	//1.获取Excel内容存储至sheetContentMap中,建立列标题与列索引映射表hashColIndexByColTitle
	CVariant2dArray sheetContentMap(1,1);
	if(!CExcelOper::GetExcelContentOfSpecifySheet(m_sFileName,sheetContentMap,1))
		return false;
	CHashStrList<DWORD> hashColIndexByColTitle;
	for(int i=0;i<ERP_EXCEL_COL_COUNT;i++)
	{
		VARIANT value;
		sheetContentMap.GetValueAt(0,i,value);
		if(CString(value.bstrVal).CompareNoCase(T_ERP_PART_NO)==0) 
			hashColIndexByColTitle.SetValue(T_ERP_PART_NO,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_PART_TYPE)==0)
			hashColIndexByColTitle.SetValue(T_ERP_PART_TYPE,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_METERIAL)==0)
			hashColIndexByColTitle.SetValue(T_ERP_METERIAL,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_LEN)==0)
			hashColIndexByColTitle.SetValue(T_ERP_LEN,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_SPEC)==0)
			hashColIndexByColTitle.SetValue(T_ERP_SPEC,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_NUM)==0)
			hashColIndexByColTitle.SetValue(T_ERP_NUM,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_SING_WEIGHT)==0)
			hashColIndexByColTitle.SetValue(T_ERP_SING_WEIGHT,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_NOTE)==0)
			hashColIndexByColTitle.SetValue(T_ERP_NOTE,i);
	}
	if( hashColIndexByColTitle.GetValue(T_ERP_PART_NO)==NULL||		//件号
		hashColIndexByColTitle.GetValue(T_ERP_SPEC)==NULL||			//规格
		hashColIndexByColTitle.GetValue(T_ERP_METERIAL)==NULL)		//材质
	{
		logerr.Log("缺少关键列(件号或规格或材质或单基数)!");
		return FALSE;
	}
	//2.获取Excel所有单元格的值
	m_hashPartByPartNo.Empty();
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar100 sPartNo,sPartType,sMeterial,sSpec,sNote;
		//件号
		DWORD *pColIndex=hashColIndexByColTitle.GetValue(T_ERP_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//材质
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_METERIAL);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sMeterial=VariantToString(value);
		//长度
		float fLength=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_LEN);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fLength=(float)atof(VariantToString(value));
		//规格
		int cls_id=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if(strstr(sSpec,"L")||strstr(sSpec,"∠"))
		{
			cls_id=CProcessPart::TYPE_LINEANGLE;
			if(strstr(sSpec,"∠"))
				sSpec.Replace("∠","L");
			if(strstr(sSpec,"×"))
				sSpec.Replace("×","*");
			if(strstr(sSpec,"x"))
				sSpec.Replace("x","*");
		}
		else if(strstr(sSpec,"-"))
			cls_id=CProcessPart::TYPE_PLATE;
		else //if(strstr(sSpec,"φ"))
			cls_id=CProcessPart::TYPE_LINETUBE;
		//单基数
		int nNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_NUM);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		nNum=atoi(VariantToString(value));
		//单重
		float fWeight=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_SING_WEIGHT);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fWeight=(float)atof(VariantToString(value));
		//备注
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_NOTE);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sNote=VariantToString(value);
		//填充哈希表
		CProcessPart* pProPart=m_hashPartByPartNo.Add(sPartNo,cls_id);
		pProPart->m_nDanJiNum=nNum;
		pProPart->SetPartNo(sPartNo);
		pProPart->SetSpec(sSpec);
		pProPart->SetNotes(sNote);
		pProPart->cMaterial=CBomModel::QueryBriefMatMark(sMeterial);
		if(pProPart->cMaterial=='A')
			pProPart->m_sRelatePartNo.Copy(sMeterial);
		pProPart->m_wLength=(WORD)fLength;
		pProPart->m_fWeight=fWeight;
		int nWidth=0,nThick=0;
		CBomModel::RestoreSpec(sSpec,&nWidth,&nThick);
		pProPart->m_fWidth=(float)nWidth;
		pProPart->m_fThick=(float)nThick;
	}
	return TRUE;
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
	file.WriteString(m_xLoftBom.GetFileName());	//TMA料单
	file.WriteString(m_xOrigBom.GetFileName());	//ERP料单
	//DWG文件信息
	DWORD n=dwgFileList.GetNodeNum();
	file.Write(&n,sizeof(DWORD));
	for(CDwgFileInfo* pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->GetFileName());
		int ibValue=0;
		if(pDwgInfo->IsJgDwgInfo())
			ibValue=1;
		file.Write(&ibValue,sizeof(int));
	}
}
//初始化BOM信息
void CProjectTowerType::InitBomInfo(const char* sFileName,BOOL bLoftBom)
{
	if(bLoftBom)
	{
		m_xLoftBom.SetBelongModel(this);
		m_xLoftBom.ImPortBomFile(sFileName,bLoftBom);
	}
	else
	{	
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImPortBomFile(sFileName,bLoftBom);
	}
}
//比对BOM信息 0.相同 1.不同 2.文件有误
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS=0.5;
	m_hashCompareResultByPartNo.Empty();
	if(m_xLoftBom.GetPartNum()<=0)
	{
		logerr.Log("缺少放样BOM信息!");
		return 2;
	}
	if(m_xOrigBom.GetPartNum()<=0)
	{
		logerr.Log("缺少工艺科BOM信息");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for(CProcessPart *pLoftPart=m_xLoftBom.EnumFirstPart();pLoftPart;pLoftPart=m_xLoftBom.EnumNextPart())
	{	
		CProcessPart *pOrgPart = m_xOrigBom.FindPart(pLoftPart->GetPartNo());
		if (pOrgPart == NULL)
		{	//1.存在放样构件，不存在原始构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->GetPartNo());
			pResult->pOrgPart=NULL;
			pResult->pLoftPart=pLoftPart;
		}
		else 
		{	//2. 对比同一件号构件属性
			hashBoolByPropName.Empty();
			if(pOrgPart->cMaterial!=pLoftPart->cMaterial||
				(pOrgPart->cMaterial=='A'&&!pOrgPart->m_sRelatePartNo.Equal(pLoftPart->m_sRelatePartNo)))		//材质
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(pOrgPart->IsAngle() && fabsl(pOrgPart->m_wLength-pLoftPart->m_wLength)>50)	//根据客户需求，长度比较误差在50以内
				hashBoolByPropName.SetValue("m_fLength",TRUE);
			if(pOrgPart->m_nDanJiNum!=pLoftPart->m_nDanJiNum)		//单基数
				hashBoolByPropName.SetValue("m_nDanJiNum",TRUE);
			if(stricmp(pOrgPart->GetSpec(FALSE),pLoftPart->GetSpec(FALSE))!=0)	//规格
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->GetPartNo());
				pResult->pOrgPart=pOrgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 遍历导入的原始表,查找是否有漏件情况
	for(CProcessPart *pPart=m_xOrigBom.EnumFirstPart();pPart;pPart=m_xOrigBom.EnumNextPart())
	{
		if(m_xLoftBom.FindPart(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
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
			hashAngleByPartNo.SetValue(pJgInfo->m_xAngle.GetPartNo(),TRUE);
	}
	for(CProcessPart *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(!pPart->IsAngle())
			continue;
		if(hashAngleByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
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
	for(CProcessPart *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(!pPart->IsPlate())
			continue;
		if(hashPlateByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
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
//添加角钢DWGBOM信息
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf)
{
	if(strlen(sFileName)<=0)
		return FALSE;
	//打开DWG文件
	CXhChar500 file_path;
	AcApDocument *pDoc=NULL;
	AcApDocumentIterator *pIter=acDocManager->newAcApDocumentIterator();
	for(;!pIter->done();pIter->step())
	{
		pDoc=pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if(strstr(file_path,sFileName))
			break;
	}
	if(strstr(file_path,sFileName))	//激活指定文件
		acDocManager->activateDocument(pDoc);
	else
	{		//打开指定文件
#ifdef _ARX_2007
		acDocManager->appContextOpenDocument((ACHAR*)sFileName);
#else
		acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	//读取DWG文件信息
	CDwgFileInfo* pDwgFile=FindDwgBomInfo(sFileName);
	if(pDwgFile)
	{	
		pDwgFile->InitDwgInfo(sFileName,bJgDxf);
		return pDwgFile;
	}
	else
	{	
		pDwgFile=dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		//读取DWG信息
		pDwgFile->InitDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
		/*if(pDwgFile->InitDwgInfo(sFileName,bJgDxf))
			return pDwgFile;
		else
		{
			dwgFileList.DeleteCursor();
			dwgFileList.Clean();
			return NULL;
		}*/
	}
}
//
CDwgFileInfo* CProjectTowerType::FindDwgBomInfo(const char* sFileName)
{
	CDwgFileInfo* pDwgInfo=NULL;
	for(pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		if(strcmp(pDwgInfo->GetFileName(),sFileName)==0)
			break;
	}
	return pDwgInfo;
}
//
int CProjectTowerType::CompareLoftAndAngleDwg(const char* sFileName)
{
	const double COMPARE_EPS=0.5;
	if(m_xLoftBom.GetPartNum()<=0)
	{
		logerr.Log("缺少放样BOM信息!");
		return 2;
	}
	CDwgFileInfo* pJgDwgFile=FindDwgBomInfo(sFileName);
	if(pJgDwgFile==NULL)
	{
		logerr.Log("未找到指定的角钢DWG文件!");
		return 2;
	}
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=pJgDwgFile->EnumFirstJg();pJgInfo;pJgInfo=pJgDwgFile->EnumNextJg())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		CProcessPart *pLoftJg=m_xLoftBom.FindPart(sPartNo);
		if(pLoftJg==NULL)
		{	//1、存在DWG构件，不存在放样构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
			pResult->pOrgPart=&(pJgInfo->m_xAngle);
			pResult->pLoftPart=NULL;
		}
		else
		{	//2、对比同一件号构件属性
			hashBoolByPropName.Empty();
			if(pJgInfo->m_xAngle.cMaterial!= pLoftJg->cMaterial||
				(pLoftJg->cMaterial=='A'&&!pLoftJg->m_sRelatePartNo.Equal(pJgInfo->m_xAngle.m_sRelatePartNo)))			//材质
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(fabsl(pJgInfo->m_xAngle.m_wLength-pLoftJg->m_wLength)>0)		//长度--根据客户需求,长度比较要完全一致
				hashBoolByPropName.SetValue("m_fLength",TRUE);
			if(pJgInfo->m_xAngle.m_nDanJiNum!=pLoftJg->m_nDanJiNum)			//单基数
				hashBoolByPropName.SetValue("m_nDanJiNum",TRUE);
			if(stricmp(pJgInfo->m_xAngle.GetSpec(FALSE),pLoftJg->GetSpec(FALSE))!=0)	//规格
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
				pResult->pOrgPart=&(pJgInfo->m_xAngle);
				pResult->pLoftPart=pLoftJg;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("放样角钢信息和DWG角钢信息相同!");
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
		logerr.Log("缺少放样BOM信息!");
		return 2;
	}
	CDwgFileInfo* pPlateDwgFile=FindDwgBomInfo(sFileName);
	if(pPlateDwgFile==NULL)
	{
		logerr.Log("未找到指定的钢板DWG文件!");
		return 2;
	}
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=pPlateDwgFile->EnumFirstPlate();pPlateInfo;pPlateInfo=pPlateDwgFile->EnumNextPlate())
	{
		CXhChar16 sPartNo=pPlateInfo->xPlate.GetPartNo();
		CProcessPart *pLoftPlate=m_xLoftBom.FindPart(sPartNo);
		if(pLoftPlate==NULL)
		{	//1、存在DWG构件，不存在放样构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
			pResult->pOrgPart=&(pPlateInfo->xPlate);
			pResult->pLoftPart=NULL;
		}
		else
		{	//2、对比同一件号构件属性
			hashBoolByPropName.Empty();
			if(pPlateInfo->xPlate.cMaterial!=pLoftPlate->cMaterial||
				(pLoftPlate->cMaterial=='A'&&!pLoftPlate->m_sRelatePartNo.Equal(pPlateInfo->xPlate.m_sRelatePartNo)))				//材质
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(stricmp(pPlateInfo->xPlate.GetSpec(FALSE),pLoftPlate->GetSpec(FALSE))!=0)	//规格
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//结点数量
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
				pResult->pOrgPart=&(pPlateInfo->xPlate);
				pResult->pLoftPart=pLoftPlate;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("放样钢板信息和DWG钢板信息相同!");
		return 0;
	}
	else
		return 1;
}
//
void CProjectTowerType::AddBomResultSheet(LPDISPATCH pSheet,int iSheet)
{
	//对校审结果进行排序
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())//遍历存储的结果表
	{
		if(pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->GetPartNo());
		else
			keyStrArr.append(pResult->pOrgPart->GetPartNo());
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData,keyStrArr.GetSize(),compare_func);
	//添加标题行
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="构件编号";str_arr[1]="设计规格";str_arr[2]="材质";
	str_arr[3]="长度";str_arr[4]="单基数";str_arr[5]="数据来源";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//填充内容
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//获取Excel表格的范围
	int index=0;
	if(iSheet==1)
	{	//第一种结果：ERP和TMA表中的数据信息不同
		excel_sheet.SetName("校审结果");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//合并行
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			map.SetValueAt(index,5,COleVariant(CXhChar16("ERP")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pLoftPart->cMaterial,pResult->pLoftPart->m_sRelatePartNo)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_fLength"))
			{	
				map.SetValueAt(index+1,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
				_snprintf(cell_start,15,"D%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"D%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_nDanJiNum"))
			{	
				map.SetValueAt(index+1,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
				_snprintf(cell_start,15,"E%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"E%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,5,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//第二种结果：放样表里有数据，导入原始表里没有数据
		excel_sheet.SetName("TMA表专有构件");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pLoftPart->cMaterial,pResult->pLoftPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
			index++;
		}
	}
	else if(iSheet==3)
	{	//第三种结果：导入原始表里有数据，放样表里没有数据
		excel_sheet.SetName("ERP表专有构件");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			index++;
		}
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddAngleResultSheet(LPDISPATCH pSheet,int iSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	//添加标题栏
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="构件编号";str_arr[1]="设计规格";str_arr[2]="材质";
	str_arr[3]="长度";str_arr[4]="单基数";str_arr[5]="数据来源";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//填充内容
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//获取Excel表格的范围
	int index=0;
	if(iSheet==1)
	{	//第一种结果：DWG和TMA表中的数据信息不同
		excel_sheet.SetName("校审结果");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{	
			if(pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//合并行
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			map.SetValueAt(index,5,COleVariant(CXhChar16("DWG")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pLoftPart->cMaterial,pResult->pLoftPart->m_sRelatePartNo)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_fLength"))
			{	
				map.SetValueAt(index+1,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
				_snprintf(cell_start,15,"D%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"D%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_nDanJiNum"))
			{	
				map.SetValueAt(index+1,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
				_snprintf(cell_start,15,"E%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"E%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,5,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//第二种结果：DWG文件中里有数据，放样表里没有数据
		excel_sheet.SetName("DWG表专有构件");
		index++;
		for (COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{
			if(pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			index++;
		}
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddPlateResultSheet(LPDISPATCH pSheet,int iSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	//添加标题栏
	CStringArray str_arr;
	str_arr.SetSize(4);
	str_arr[0]="构件编号";str_arr[1]="设计规格";
	str_arr[2]="材质";	  str_arr[3]="数据来源";
	double col_arr[4]={15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//填充内容
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,4);//获取Excel表格的范围
	int index=0;
	if(iSheet==1)
	{	//第一种结果：DWG和TMA表中的数据信息不同
		excel_sheet.SetName("校审结果");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{	
			if(pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//合并行
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			map.SetValueAt(index,3,COleVariant(CXhChar16("DWG")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pLoftPart->cMaterial,pResult->pLoftPart->m_sRelatePartNo)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,3,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//第二种结果：DWG文件中里有数据，放样表里没有数据
		excel_sheet.SetName("DWG表专有构件");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult = EnumNextResult())
		{
			if(pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pOrgPart->cMaterial,pResult->pOrgPart->m_sRelatePartNo)));
			index++;
		}
	}
	_snprintf(cell_end,15,"D%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddDwgLackPartSheet(LPDISPATCH pSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("TMA表专有构件");
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
		map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->GetPartNo()));
		map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
		map.SetValueAt(index,2,COleVariant(CBomModel::QuerySteelMatMark(pResult->pLoftPart->cMaterial,pResult->pLoftPart->m_sRelatePartNo)));
		map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
		map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
		map.SetValueAt(index,5,COleVariant((long)pResult->pLoftPart->feature));
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
	int nSheet=2;
	if(iCompareType==COMPARE_BOM_FILE)
		nSheet=3;
	else if(iCompareType==COMPARE_ANGLE_DWGS || 
		iCompareType==COMPARE_PLATE_DWGS)
		nSheet=1;
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(nSheet);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for(int iSheet=1;iSheet<=nSheet;iSheet++)
	{
		LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((short)iSheet));
		if(iCompareType==COMPARE_BOM_FILE)			//料单对比结果		
			AddBomResultSheet(pWorksheet,iSheet);
		else if(iCompareType==COMPARE_ANGLE_DWG)	//角钢DWG对比结果
			AddAngleResultSheet(pWorksheet,iSheet);
		else if(iCompareType==COMPARE_PLATE_DWG)	//钢板DWG对比结果
			AddPlateResultSheet(pWorksheet,iSheet);
		else //if(iCompareType==COMPARE_ANGLE_DWGS)
			AddDwgLackPartSheet(pWorksheet);
	}
	excel_sheets.ReleaseDispatch();
}
//更新ERP料单中件号（材质为Q420,件号前加P，材质为Q345，件号前加H）
BOOL CProjectTowerType::ModifyErpBomPartNo(BYTE ciMatCharPosType)
{
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_xOrigBom.GetFileName()))
		return FALSE;
	LPDISPATCH pWorksheets = excelobj.GetWorksheets();
	if(pWorksheets==NULL)
	{
		logerr.Log("ERP料单文件打开失败!");
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
	excel_range.AttachDispatch(excel_usedRange.GetColumns());
	long nColNum = excel_range.GetCount();
	CVariant2dArray sheetContentMap(1,1);
	CXhChar50 cell=CExcelOper::GetCellPos(nColNum,nRowNum);
	LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"),COleVariant(cell));
	excel_range.AttachDispatch(pRange,FALSE);
	sheetContentMap.var=excel_range.GetValue();
	excel_usedRange.ReleaseDispatch();
	excel_range.ReleaseDispatch();
	//更新指定sheet内容
	int iPartNoCol=1,iMartCol=3;
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar16 sPartNo,sMeterial,sNewPartNo;
		//件号
		sheetContentMap.GetValueAt(i,iPartNoCol,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//材质
		sheetContentMap.GetValueAt(i,iMartCol,value);
		sMeterial=VariantToString(value);
		//更新件号数据
		if(strstr(sMeterial,"Q345") && strstr(sPartNo,"H")==NULL)
		{
			if(ciMatCharPosType==0)
				sNewPartNo.Printf("H%s",(char*)sPartNo);
			else if(ciMatCharPosType==1)
				sNewPartNo.Printf("%sH",(char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet,iPartNoCol,i+1,sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo,sNewPartNo);
		}
		if(strstr(sMeterial,"Q420") && strstr(sPartNo,"P")==NULL)
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
//////////////////////////////////////////////////////////////////////////
//
CIdentifyManager::CIdentifyManager()
{
	fMaxX=0;
	fMaxY=0;
	fMinX=0;
	fMinY=0;
	fTextHigh=0;
	fPnDistX=0;
	fPnDistY=0;
}
CIdentifyManager::~CIdentifyManager()
{
	
}
BOOL CIdentifyManager::IsMatchPNRule(const char* sText)
{
	if(strlen(sText)<=0)
		return FALSE;
	if(strstr(sText,m_sPartNoKey)==NULL)
		return FALSE;
	CXhChar100 sValue(sText);
	sValue.Replace("　"," ");
	int nNum=0,nKeyNum=0;
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{	
		nNum++;
		if(strstr(sKey,m_sPartNoKey))
			nKeyNum++;
	}
	if(nKeyNum>1)
		return FALSE;
	return TRUE;
}
void CIdentifyManager::ParsePartNoText(const char* sText,CXhChar16& sPartNo)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("　"," ");	 //将全角空格换成半角空格以便统一处理
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sPartNoKey))
		{	//提取件号
			str.Copy(sKey);
			str.Replace(m_sPartNoKey,"");
			sPartNo.Copy(str);
		}
	}
}
BOOL CIdentifyManager::IsMatchThickRule(const char* sText)
{
	if(strlen(sText)<=0)
		return FALSE;
	if(strstr(sText,m_sThickKey)==NULL)
		return FALSE;
	if(strstr(sText,m_sPartNoKey))
		return FALSE;
	if(strstr(sText,"Q")==NULL)
		return FALSE;
	return TRUE;
}
BOOL CIdentifyManager::IsMatchMatRule(const char* sText)
{
	if(strstr(sText,"Q235"))
		return TRUE;
	if(strstr(sText,"Q345"))
		return TRUE;
	if(strstr(sText,"Q390"))
		return TRUE;
	if(strstr(sText,"Q420"))
		return TRUE;
	if(strstr(sText,"Q460"))
		return TRUE;
	return FALSE;
}
//提取钢板厚度
void CIdentifyManager::ParseThickText(const char* sText,int& nThick)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("　"," ");	 //将全角空格换成半角空格以便统一处理
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sThickKey))
		{
			str.Copy(sKey);
			if(strstr(str,"mm"))
				str.Replace("mm","");
			str.Replace(m_sThickKey,"");
			nThick=atoi(str);
			return;
		}
	}
}
//提取钢板材质
void CIdentifyManager::ParseMatText(const char* sText,char& cMat)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("　"," ");	 //将全角空格换成半角空格以便统一处理
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,"Q"))
		{
			cMat=CBomModel::QueryBriefMatMark(sKey);
			return;
		}
	}
}
//提取钢板件数
void CIdentifyManager::ParseNumText(const char* sText,int& nNum)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("　"," ");	 //将全角空格换成半角空格以便统一处理
	if(strstr(sValue,m_sThickKey))
	{	//板厚+材质+件数
		for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
		{
			if(strstr(sKey,m_sNumKey1) || strstr(sKey,m_sNumKey2))
			{
				str.Copy(sKey);
				str.Replace(m_sNumKey1,"");
				str.Replace(m_sNumKey2,"");
				nNum=atoi(str);
				return;
			}
		}
	}
	else
	{	//钢板件数单独一行
		sValue.Replace(" ","");
		sValue.Replace(m_sNumKey1,"");
		sValue.Replace(m_sNumKey2,"");
		sValue.Replace("各","");
		nNum=atoi(sValue);
	}
}
bool CIdentifyManager::InitJgCardInfo(const char* sFileName)
{
	if(strlen(sFileName)<=0)
		return false;
	f3dPoint startPt, endPt;
	char APP_PATH[MAX_PATH],dwg_file[MAX_PATH];
	GetAppPath(APP_PATH);
	sprintf(dwg_file,"%s%s",APP_PATH,sFileName);
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	AcDbDatabase blkDb(Adesk::kFalse);//定义空的数据库
#ifdef _ARX_2007
	if(blkDb.readDwgFile((ACHAR*)_bstr_t(dwg_file),_SH_DENYRW,true)==Acad::eOk)
#else
	if(blkDb.readDwgFile(dwg_file,_SH_DENYRW,true)==Acad::eOk)
#endif
	{
		AcDbEntity *pEnt;
		AcDbBlockTable *pTempBlockTable;
		blkDb.getBlockTable(pTempBlockTable,AcDb::kForRead);
		//获得当前图形块表记录指针
		AcDbBlockTableRecord *pTempBlockTableRecord;//定义块表记录指针
		//以写方式打开模型空间，获得块表记录指针
		pTempBlockTable->getAt(ACDB_MODEL_SPACE,pTempBlockTableRecord,AcDb::kForRead);
		pTempBlockTable->close();//关闭块表
		AcDbBlockTableRecordIterator *pIterator=NULL;
		pTempBlockTableRecord->newIterator( pIterator);
		SCOPE_STRU scope;
		CXhChar50 sText;
		int nPartLabelCount = 0;
		for(;!pIterator->done();pIterator->step())
		{
			pIterator->getEntity(pEnt,AcDb::kForRead);
			pEnt->close();
			if(pEnt->isKindOf(AcDbLine::desc()))
			{
				AcDbLine* pLine=(AcDbLine*)pEnt;
				Cpy_Pnt(startPt,pLine->startPoint());
				Cpy_Pnt(endPt,pLine->endPoint());
				scope.VerifyVertex(startPt);
				scope.VerifyVertex(endPt);
				continue;
			}
			if(pEnt->isKindOf(AcDbMText::desc()))
			{
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
				double fPosX = pMText->location().x;
				double fPosY = pMText->location().y;
				if (nPartLabelCount == 0 || fPnDistX<fPosX || fPnDistY>fPosY)
				{
					fPnDistX = fPosX;
					fPnDistY = fPosY;
				}
				nPartLabelCount++;
				continue;
			}
			if(pEnt->isKindOf(AcDbText::desc()))
			{
				AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
				sText.Copy(_bstr_t(pText->textString()));
#else
				sText.Copy(pText->textString());
#endif
				if ((strstr(sText, "件") == NULL && strstr(sText, "编") == NULL) || strstr(sText, "号") == NULL)
					continue;
				if (strstr(sText, "图号") != NULL || strstr(sText, "文件编号") != NULL)
					continue;
				double fPosX = pText->position().x;
				double fPosY = pText->position().y;
				if (nPartLabelCount == 0 || fPnDistX<fPosX || fPnDistY>fPosY)
				{
					fPnDistX = fPosX;
					fPnDistY = fPosY;
				}
				nPartLabelCount++;
				continue;
			}
			if(!pEnt->isKindOf(AcDbPoint::desc()))
				continue;
			GRID_DATA_STRU grid_data;
			if(!GetGridKey((AcDbPoint*)pEnt,&grid_data))
				continue;
			if(grid_data.type_id==ITEM_TYPE_PART_NO)		//件号
			{
				part_no_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				part_no_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_DES_MAT)	//设计材质
			{
				mat_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				mat_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_DES_GUIGE)	//设计规格
			{
				guige_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				guige_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_LENGTH)	//长度
			{
				length_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				length_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_PIECE_WEIGHT)	//单重
			{
				piece_weight_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				piece_weight_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_PART_NUM)	//单基数
			{
				danji_num_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				danji_num_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
			else if(grid_data.type_id==ITEM_TYPE_SUM_PART_NUM)	//加工数
			{
				jiagong_num_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				jiagong_num_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
				fTextHigh=grid_data.fTextHigh;
			}
			else if(grid_data.type_id==ITEM_TYPE_PART_NOTES)	//备注
			{
				note_rect.topLeft.Set(grid_data.min_x,grid_data.max_y);
				note_rect.bottomRight.Set(grid_data.max_x,grid_data.min_y);
			}
		}
		//工艺卡矩形区域
		fMinX=scope.fMinX;
		fMinY=scope.fMinY;
		fMaxX=scope.fMaxX;
		fMaxY=scope.fMaxY;
		return true;
	}
	return false;
}
f3dPoint CIdentifyManager::GetJgCardOrigin(f3dPoint partNo_pt)
{
	return f3dPoint(partNo_pt.x-fPnDistX,partNo_pt.y-fPnDistY,0);
}
//////////////////////////////////////////////////////////////////////////
//CBomModel
CBomModel::CBomModel(void)
{
	
}
CBomModel::~CBomModel(void)
{
	
}
//
CXhChar16 CBomModel::QuerySteelMatMark(char cMat,char* matStr/*=NULL*/)
{
	CXhChar16 sMatMark;
	if('H'==cMat)
		sMatMark.Copy("Q345");
	else if('G'==cMat)
		sMatMark.Copy("Q390");
	else if('P'==cMat)
		sMatMark.Copy("Q420");
	else if('T'==cMat)
		sMatMark.Copy("Q460");
	else if('S'==cMat)
		sMatMark.Copy("Q235");
	else if(matStr)
		sMatMark.Copy(matStr);
	return sMatMark;
}
char CBomModel::QueryBriefMatMark(const char* sMatMark)
{
	char cMat='A';
	if(strstr(sMatMark,"Q235"))
		cMat='S';
	else if(strstr(sMatMark,"Q345"))
		cMat='H';
	else if(strstr(sMatMark,"Q390"))
		cMat='G';
	else if(strstr(sMatMark,"Q420"))
		cMat='P';
	else if(strstr(sMatMark,"Q460"))
		cMat='T';
	return cMat;
}
bool CBomModel::InitBomModel()
{
	//读取配置文件，初始化识别规则
	CLogErrorLife logErrLife;
	char APP_PATH[MAX_PATH],cfg_file[MAX_PATH];
	GetAppPath(APP_PATH);
	sprintf(cfg_file,"%subom.cfg",APP_PATH);
	if(strlen(cfg_file)<=0)
	{
		logerr.Log("配置文件读取失败!请确认是否存在%s文件。",cfg_file);
		return false;
	}
	FILE *fp =fopen(cfg_file,"rt");
	if(fp==NULL)
	{
		logerr.Log("配置文件读取失败!请确认是否存在%s文件。",cfg_file);
		return false;
	}
	CXhChar200 line_txt,sLine;
	BOOL bAngle=FALSE,bPlate=FALSE;
	while(!feof(fp))
	{
		if(fgets(line_txt,200,fp)==NULL)
			continue;
		if(stricmp(line_txt,"END")==0)
			break;
		strcpy(sLine,line_txt);
		sLine.Replace('=',' ');
		char szTokens[]= " =\n" ;
		char* skey=strtok(line_txt,szTokens);
		if(skey&&stricmp(skey,"ANGLE_DWG_RULE")==0)
		{
			bAngle=TRUE;
			bPlate=FALSE;
			continue;
		}
		else if(skey&&stricmp(skey,"PLATE_DWG_RULE")==0)
		{
			bAngle=FALSE;
			bPlate=TRUE;
			continue;
		}
		if(bAngle)
		{
			if(skey&&stricmp(skey,"JG_CARD")==0)
			{
				skey=strtok(NULL,szTokens);
				m_sJgCadName.Copy(skey);
				manager.InitJgCardInfo(skey);
			}
		}
		else if(bPlate)
		{
			if(skey&&stricmp(skey,"sPartNoKey")==0)
			{
				skey=strtok(NULL,szTokens);
				manager.m_sPartNoKey.Copy(skey);
			}
			else if(skey&&stricmp(skey,"sThickKey")==0)
			{
				skey=strtok(NULL,szTokens);
				manager.m_sThickKey.Copy(skey);
			}
			else if(skey&&stricmp(skey,"sNumKey")==0)
			{
				skey=strtok(NULL,szTokens);
				sLine.Copy(skey);
				sLine.Replace('|',' ');
				sscanf(sLine,"%s%s",(char*)manager.m_sNumKey1,(char*)manager.m_sNumKey2);
			}
		}
	}
	fclose(fp);
	return true;
}
void CBomModel::RestoreSpec(const char* spec,int *width,int *thick,char *matStr/*=NULL*/)
{
	char sMat[16]="",cMark1=' ',cMark2=' ';
	if(strstr(spec,"Q")==(char*)spec)
	{
		if(strstr(spec,"L"))
			sscanf(spec,"%[^L]%c%d%c%d",sMat,&cMark1,width,&cMark2,thick);
		else if(strstr(spec,"-"))
			sscanf(spec,"%[^-]%c%d",sMat,&cMark1,thick);
	}
	else if(strstr(spec,"L"))
	{
		CXhChar16 sSpec(spec);
		sSpec.Replace("L","");
		sSpec.Replace("*"," ");
		sSpec.Replace("X"," ");
		sscanf(sSpec,"%d%d",width,thick);
	}
	else if (strstr(spec,"-"))
		sscanf(spec,"%c%d",sMat,thick);
	//else if(spec,"φ")
	//sscanf(spec,"%c%d%c%d",sMat,)
	if(matStr)
		strcpy(matStr,sMat);
}
void CBomModel::SendCommandToCad(CString sCmd)
{
	if(strlen(sCmd)<=0)
		return;
	COPYDATASTRUCT cmd_msg;
	cmd_msg.dwData=(DWORD)1;
	cmd_msg.cbData=(DWORD)_tcslen(sCmd)+1;
	cmd_msg.lpData=sCmd.GetBuffer(sCmd.GetLength()+1);
	SendMessage(adsw_acadMainWnd(),WM_COPYDATA,(WPARAM)adsw_acadMainWnd(),(LPARAM)&cmd_msg);
}