#include "stdafx.h"
#include "TestCode.h"
#include "Buffer.h"
#include "folder_dialog.h"
#include "PathManager.h"

#ifdef __ALFA_TEST_
CLogFile MyLogFile("F:\\Log\\MyPnc.log");
//////////////////////////////////////////////////////////////////////////
//PLATE_COMPARE
void PLATE_COMPARE::RunCompare()
{
	MyLogFile.Log("件号%s#:", m_sPartNo);
	if (m_pSrcPlate == NULL || m_pDestPlate == NULL)
	{
		if (m_pSrcPlate == NULL)
			MyLogFile.Log("\t源文件缺失!");
		else
			MyLogFile.Log("\t目标文件缺失!");
		return;
	}
	//基本信息比较
	if (m_pSrcPlate->m_fThick != m_pDestPlate->m_fThick)
	{
		MyLogFile.Log("\t钢板厚度不一致{%.1f!=%.1f}!",m_pSrcPlate->m_fThick,m_pDestPlate->m_fThick);
		return;
	}
	if (m_pSrcPlate->cMaterial != m_pDestPlate->cMaterial)
	{
		MyLogFile.Log("\t钢板材质不一致{%c!=%c}!", m_pSrcPlate->cMaterial, m_pDestPlate->cMaterial);
		return;
	}
	if (m_pSrcPlate->m_nSingleNum!=m_pDestPlate->m_nSingleNum)
	{
		MyLogFile.Log("\t单基数量不一致{%d!=%d}!", m_pSrcPlate->m_nSingleNum, m_pDestPlate->m_nSingleNum);
		return;
	}
	if (m_pSrcPlate->m_nProcessNum != m_pDestPlate->m_nProcessNum)
	{
		MyLogFile.Log("\t加工数量不一致{%d!=%d}!", m_pSrcPlate->m_nProcessNum, m_pDestPlate->m_nProcessNum);
		return;
	}
	//钢板外形比较
	if (m_pSrcPlate->vertex_list.GetNodeNum() != m_pDestPlate->vertex_list.GetNodeNum())
	{
		MyLogFile.Log("\t钢板轮廓点数量不一致{%d!=%d}!",m_pSrcPlate->vertex_list.GetNodeNum(),m_pDestPlate->vertex_list.GetNodeNum());
		return;
	}
	PROFILE_VER *pSrcVer = NULL, *pDestVer = NULL;
	for (pSrcVer=m_pSrcPlate->vertex_list.GetFirst();pSrcVer;
		pSrcVer=m_pSrcPlate->vertex_list.GetNext())
	{
		pDestVer = NULL;
		for (pDestVer = m_pDestPlate->vertex_list.GetFirst(); pDestVer;
			pDestVer = m_pDestPlate->vertex_list.GetNext())
		{
			if(pSrcVer->vertex.IsEqual(pSrcVer->vertex,EPS2))
				break;
		}
		if (pDestVer == NULL)
		{
			MyLogFile.Log("\t钢板轮廓点坐标不一致!");
			return;
		}
		if (pSrcVer->type != pDestVer->type)
		{
			MyLogFile.Log("\t钢板轮廓边类型不一致!");
			return;
		}
	}
	//螺栓信息比较
	if (m_pSrcPlate->m_xBoltInfoList.GetNodeNum() != m_pDestPlate->m_xBoltInfoList.GetNodeNum())
	{
		MyLogFile.Log("\t螺栓孔数量不一致!");
		return;
	}
	BOLT_INFO *pSrcHole = NULL, *pDestHole = NULL;
	for (pSrcHole=m_pSrcPlate->m_xBoltInfoList.GetFirst();pSrcHole;
		pSrcHole=m_pSrcPlate->m_xBoltInfoList.GetNext())
	{
		pDestHole = NULL;
		GEPOINT pt(pSrcHole->posX, pSrcHole->posY, 0);
		for (pDestHole = m_pDestPlate->m_xBoltInfoList.GetFirst(); pDestHole;
			pDestHole = m_pDestPlate->m_xBoltInfoList.GetNext())
		{
			if(pt.IsEqual(GEPOINT(pDestHole->posX,pDestHole->posY,0),EPS2))
				break;
		}
		if (pDestHole==NULL)
		{
			MyLogFile.Log("\t螺栓孔坐标不一致!");
			return;
		}
		if (pSrcHole->bolt_d != pDestHole->bolt_d)
		{
			MyLogFile.Log("\t螺栓孔直径不一致!");
			return;
		}
		if (pSrcHole->cFuncType != pDestHole->cFuncType)
		{
			MyLogFile.Log("\t螺栓孔类型不一致!");
			return;
		}
	}
	MyLogFile.Log("\t数据一致!");
}
//////////////////////////////////////////////////////////////////////////
//
bool LoadBufferFromFile(const char* sFileName, CBuffer &buffer)
{
	CFile file;
	if (!file.Open(sFileName, CFile::modeRead | CFile::typeBinary))
		return false;
	try
	{
		long file_len = 0;
		file.Read(&file_len, sizeof(long));
		buffer.Write(NULL, file_len);
		file.Read(buffer.GetBufferPtr(), buffer.GetLength());
		file.Close();
	}
	catch (CFileException*) {
		return false;
	}
	catch (CException*) {
		return false;
	}
	return true;
}
bool SearchPPIFiles(const char* sFolder, CHashStrList<CProcessPlate>& plateHash)
{
	if (!SetCurrentDirectory(sFolder))
		return false;	//指定的路径不存在
	CFileFind file_find;
	BOOL bFind = file_find.FindFile(".\\*.ppi");
	while (bFind)
	{
		bFind = file_find.FindNextFile();
		if (file_find.IsDots() || file_find.IsHidden() || file_find.IsReadOnly() ||
			file_find.IsSystem() || file_find.IsTemporary() || file_find.IsDirectory())
			continue;
		CString file_path = file_find.GetFilePath();
		CString file_ext = file_path.Right(4);	//取后缀名
		file_ext.MakeLower();
		if (file_ext.CompareNoCase(".ppi") == 0)
		{
			CBuffer buffer;
			if (!LoadBufferFromFile(file_path, buffer))
				continue;
			buffer.SeekToBegin();
			CXhChar16 sPartNo;
			BYTE cPartType = CProcessPart::RetrievedPartTypeAndLabelFromBuffer(buffer, sPartNo);
			buffer.SeekToBegin();
			CProcessPlate* pProcessPlate = plateHash.Add(sPartNo);
			pProcessPlate->FromPPIBuffer(buffer);
		}
	}
	return plateHash.GetNodeNum() > 0;
}

void TestPnc()
{
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc == NULL)
		return;
	CString file_path = pDoc->fileName();
	if (file_path.CompareNoCase("Drawing1.dwg") == 0)	//默认的DWG文件
		return;
	int index = file_path.ReverseFind('\\');	//反向查找'\\'
	file_path = file_path.Left(index);		//移除文件名
	CString sSrcFolder = file_path;
	CHashStrList<CProcessPlate> srcPlateHash;
	if (InvokeFolderPickerDlg(sSrcFolder))
	{
		if (!SearchPPIFiles(sSrcFolder, srcPlateHash))
		{
			AfxMessageBox("原始PPI文件获取失败!");
			return;
		}
	}
	CString sDestFolder = file_path;
	CHashStrList<CProcessPlate> destPlateHash;
	if (InvokeFolderPickerDlg(sDestFolder))
	{
		if (!SearchPPIFiles(sDestFolder, destPlateHash))
		{
			AfxMessageBox("目标PPI文件获取失败");
			return;
		}
	}
	//进行文件的对比
	CLogErrorLife logErrLife(&MyLogFile);
	CHashStrList<PLATE_COMPARE> plateCompareItems;
	if (srcPlateHash.GetNodeNum() != destPlateHash.GetNodeNum())
		MyLogFile.Log("钢板数量不一样");
	PLATE_COMPARE* pCmpItem = NULL;
	CProcessPlate* pPlate = NULL;
	for (pPlate = srcPlateHash.GetFirst(); pPlate; pPlate = srcPlateHash.GetNext())
	{
		pCmpItem = plateCompareItems.Add(pPlate->GetPartNo());
		strcpy(pCmpItem->m_sPartNo, pPlate->GetPartNo());
		pCmpItem->m_pSrcPlate = pPlate;
		pCmpItem->m_pDestPlate = destPlateHash.GetValue(pCmpItem->m_sPartNo);
	}
	for (pPlate = destPlateHash.GetFirst(); pPlate; pPlate = destPlateHash.GetNext())
	{
		if(plateCompareItems.GetValue(pPlate->GetPartNo()))
			continue;	//已存在该构件
		pCmpItem = plateCompareItems.Add(pPlate->GetPartNo());
		strcpy(pCmpItem->m_sPartNo, pPlate->GetPartNo());
		pCmpItem->m_pSrcPlate = NULL;
		pCmpItem->m_pDestPlate = pPlate;
	}
	for (pCmpItem = plateCompareItems.GetFirst(); pCmpItem; pCmpItem = plateCompareItems.GetNext())
		pCmpItem->RunCompare();
}
#endif