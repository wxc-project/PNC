#include "StdAfx.h"
#include "BomExport.h"
#include "BomModel.h"
#include "PathManager.h"
#include "MD5.H"
#include "LocalFeatureDef.h"
#include "PartLib.h"

#ifdef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//
CBomExport g_xBomExport;
CBomExport::CBomExport()
{

}

void CBomExport::Init()
{
	CXhChar500 lic_file,lib_path;
	if (GetLicFile(lic_file) == FALSE)
		return;
	char* separator = SearchChar(lic_file, '.', true);
	if (separator == NULL)
		return;
	separator -= 3;
	strcpy(separator, "BomExport.dll");
	m_hBomExport = LoadLibrary(lic_file);
	if (m_hBomExport)
	{
		GetSupportBOMType = (DefGetSupportBOMType)GetProcAddress(m_hBomExport, "GetSupportBOMType");
		GetSupportDataBufferVersion = (DefGetSupportDataBufferVersion)GetProcAddress(m_hBomExport, "GetSupportDataBufferVersion");
		CreateExcelBomFile = (DefCreateExcelBomFile)GetProcAddress(m_hBomExport, "CreateExcelBomFile");
		RecogniseReport = (DefRecogniseReport)GetProcAddress(m_hBomExport, "RecogniseReport");
		GetBomExcelFormat = (DefGetExcelFormat)GetProcAddress(m_hBomExport, "GetExcelFormat");
		GetBomExcelFormatEx = (DefGetExcelFormatEx)GetProcAddress(m_hBomExport, "GetExcelFormatEx");
	}
	else
	{
		GetSupportBOMType = NULL;
		GetSupportDataBufferVersion = NULL;
		CreateExcelBomFile = NULL;
		RecogniseReport = NULL;
		GetBomExcelFormat = NULL;
		GetBomExcelFormatEx = NULL;
	}
}

CBomExport::~CBomExport()
{
	if (m_hBomExport != NULL)
	{
		FreeLibrary(m_hBomExport);
		m_hBomExport = NULL;
	}
	GetSupportBOMType = NULL;
	GetSupportDataBufferVersion = NULL;
	CreateExcelBomFile = NULL;
	RecogniseReport = NULL;
	GetBomExcelFormat = NULL;
	GetBomExcelFormatEx = NULL;
}


static double GetPartArea(double size_para1, double size_para2, double size_para3, int idPartClsType, char cSubType/*='L'*/)
{
	if (idPartClsType == BOMPART::ANGLE)
	{
		double wing_wide = size_para1;
		double wing_thick = size_para2;
		double wing_wide_y = size_para3;
		if (cSubType != 'T'&&cSubType != 'D'&&cSubType != 'X'&&cSubType != 'L')
			cSubType = 'L';	//防止出现错误数据
		JG_PARA_TYPE *pJgPara = NULL;
		if (wing_wide_y <= 0 || wing_wide_y == wing_wide)
			pJgPara = FindJgType(wing_wide, wing_thick, cSubType);
		if (pJgPara)
			return pJgPara->area * 100;
		else	//如库内设定，只能计算近似值
		{
			if (wing_wide_y > 0)
				return (wing_wide + wing_wide_y)*wing_thick - wing_thick * wing_thick;
			else
				return wing_wide * wing_thick * 2 - wing_thick * wing_thick;
		}
	}
	else if (idPartClsType == BOMPART::TUBE)
	{
		double d = size_para1;
		double t = size_para2;
		return Pi * (d*t - t * t);
	}
	else if (idPartClsType == BOMPART::FLAT)
	{
		double w = size_para1, t = size_para2;
		return w * t;
	}
	else
		return 0;
}
void CBomExport::ExportExcelFile(CProjectTowerType *pPrjTowerType)
{
	if (m_hBomExport == NULL|| CreateExcelBomFile==NULL || pPrjTowerType==NULL)
		return;
	CModelBOM bomModel;
#ifdef __UBOM_ONLY_
	for (CDwgFileInfo *pDwgFile = pPrjTowerType->dwgFileList.GetFirst(); pDwgFile; pDwgFile = pPrjTowerType->dwgFileList.GetNext())
	{
		for (CAngleProcessInfo *pAngleInfo = pDwgFile->EnumFirstJg(); pAngleInfo; pAngleInfo = pDwgFile->EnumNextJg())
		{
			SUPERLIST_NODE<BOMPART> *pNode = bomModel.listParts.AttachNode(BOMPART::ANGLE);
			CBuffer buffer(1024);
			pAngleInfo->m_xAngle.ToBuffer(buffer);
			buffer.SeekToBegin();
			pNode->pDataObj->FromBuffer(buffer);
			if (pNode->pDataObj->GetPartNum() <= 0 && pAngleInfo->m_xAngle.feature1>0)
				pNode->pDataObj->SetPartNum(pAngleInfo->m_xAngle.feature1);
			if (pNode->pDataObj->fPieceWeight <= 0)
			{
				double fUnitWeight = 0;
				JG_PARA_TYPE *pJgType = FindJgType(pNode->pDataObj->wide, pNode->pDataObj->thick);
				if (pJgType)
					fUnitWeight = pJgType->theroy_weight;
				else
					fUnitWeight = GetPartArea(pNode->pDataObj->wide, pNode->pDataObj->thick, 0, BOMPART::ANGLE, 0)*7.85e-3;
				pNode->pDataObj->fPieceWeight = fUnitWeight * pNode->pDataObj->length*0.001;
			}
			if (pNode->pDataObj->fSumWeight <= 0)
				pNode->pDataObj->fSumWeight = pNode->pDataObj->fPieceWeight*pNode->pDataObj->GetPartNum();
		}
		for (CPlateProcessInfo *pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
		{
			SUPERLIST_NODE<BOMPART> *pNode = bomModel.listParts.AttachNode(BOMPART::PLATE);
			CBuffer buffer(1024);
			pPlateInfo->xBomPlate.ToBuffer(buffer);
			buffer.SeekToBegin();
			pNode->pDataObj->FromBuffer(buffer);
		}
	}
#endif
	CBuffer buffer;
	int nSupportVersion = GetSupportDataBufferVersion != NULL ? GetSupportDataBufferVersion() : 0;
	UINT uiBomKey = ValidateLocalizeFeature(FEATURE::LOCALIZE_CUSTOMIZE_BOM_KEY);
	if (uiBomKey > 0 && nSupportVersion >= 17)
	{	//Bom版本从V17支持根据fac文件设置的Key进行内容加密 wht 20-05-06
		BYTE bomKeyArr[16] = { 0 };
		MD5_CTX md5;
		md5.MD5Update((BYTE*)&uiBomKey, 4);
		md5.MD5Final(bomKeyArr);
		bomModel.ToBuffer(buffer, nSupportVersion, bomKeyArr);
	}
	else
		bomModel.ToBuffer(buffer, nSupportVersion);
	if (CreateExcelBomFile != NULL && GetSupportDataBufferVersion != NULL)
		CreateExcelBomFile(buffer.GetBufferPtr(), buffer.GetLength(), NULL, 0);
}
bool CBomExport::IsVaild()
{
	if (m_hBomExport == NULL || CreateExcelBomFile == NULL)
		return false;
	else
		return true;
}
#endif