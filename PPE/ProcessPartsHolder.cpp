#include "stdafx.h"
#include "ProcessPartsHolder.h"
#include "PPEView.h"
#include "direct.h"
#include "LogFile.h"
#include "PPE.h"
#include "ArrayList.h"
#include "SortFunc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void MakeDirectory(char *path)
{
	char bak_path[MAX_PATH],drive[MAX_PATH];
	strcpy(bak_path,path);
	char *dir = strtok(bak_path,"/\\");
	if(strlen(dir)==2&&dir[1]==':')
	{
		strcpy(drive,dir);
		strcat(drive,"\\");
		_chdir(drive);
		dir = strtok(NULL,"/\\");
	}
	while(dir)
	{
		_mkdir(dir);
		_chdir(dir);
		dir = strtok(NULL,"/\\");
	}
}
//////////////////////////////////////////////////////////////////////////
// CProcessPartsHolder
CProcessPartsHolder::CProcessPartsHolder()
{
	m_iCurFileIndex=0;
	m_sFolderPath="";
	m_cPartType=-1;
	m_arrPlateFileName.RemoveAll();
	m_arrAngleFileName.RemoveAll();
}
CProcessPartsHolder::~CProcessPartsHolder()
{

}

CString CProcessPartsHolder::GetCurFileNameByIndex(int index)
{
	if(m_iCurFileIndex>=0&&m_iCurFileIndex<m_arrAngleFileName.GetCount())
		return m_arrAngleFileName[m_iCurFileIndex];
	else if(m_iCurFileIndex>=m_arrAngleFileName.GetCount()&&m_iCurFileIndex<m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount())
		return m_arrPlateFileName[m_iCurFileIndex-m_arrAngleFileName.GetCount()];
	else 
		return "";
}
CString CProcessPartsHolder::GetCurFileName()
{
	return GetCurFileNameByIndex(m_iCurFileIndex);
}
CString CProcessPartsHolder::GetFilePathByIndex(int curIndex)
{
	return m_sFolderPath+"\\"+GetCurFileNameByIndex(curIndex);
}
BOOL LoadBufferFromFile(CString sFileName,CBuffer &buffer)
{
	FILE *fp=fopen(sFileName,"rb");
	if(fp==NULL)
		return FALSE;
	try{
		long file_len;
		fread(&file_len,sizeof(long),1,fp);
		buffer.Write(NULL,file_len);
		fread(buffer.GetBufferPtr(),file_len,1,fp);
		fclose(fp);
	}catch (CFileException*){
		return FALSE;
	}
	catch (CException*){
		return FALSE;
	}
	return TRUE;
}
BOOL CProcessPartsHolder::UpdateCurPartFromFile(int iCurFileIndex)
{
	if(iCurFileIndex<0&&iCurFileIndex>m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1)
		return FALSE;
	CString sFilePath=GetFilePathByIndex(iCurFileIndex);
	CBuffer curPartBuffer;
	if(!LoadBufferFromFile(sFilePath,curPartBuffer))
		return FALSE;
	curPartBuffer.SeekToBegin();
	m_cPartType=CProcessPart::ReadPartTypeFromBuffer(curPartBuffer);
	g_pPartEditor->ClearProcessParts();
	g_pPartEditor->AddProcessPart(curPartBuffer,0);
	g_pPartEditor->Draw();
	g_pSolidDraw->BuildDisplayList();
	g_p2dDraw->RenderDrawing();
	//更新属性栏
	CPPEView *pView=theApp.GetView();
	if(pView)
		pView->UpdatePropertyPage();
	return TRUE;
}
void CProcessPartsHolder::SaveCurPartInfoToFile()
{	
	if(m_iCurFileIndex<0&&m_iCurFileIndex>m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1)
		return;
	CString sFilePath=GetFilePathByIndex(m_iCurFileIndex);
	FILE *fp=fopen(sFilePath,"wb");
	if(fp==NULL)
		return;
	CBuffer buffer;
	if(!g_pPartEditor->GetProcessPart(buffer))
	{
		logerr.Log("保存文件{%d}时失败！",m_iCurFileIndex);
		return;
	}
	long file_len=buffer.GetLength();
	fwrite(&file_len,sizeof(long),1,fp);
	fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
	fclose(fp);
}
int CompareFileName(const CString& item1,const CString& item2)
{
	return item1.CompareNoCase(item2);
}
void CProcessPartsHolder::Init(CString sFolderPath)
{
	m_sFolderPath = sFolderPath;
	//
	char fn[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	sprintf(fn,"%s\\*.ppi",sFolderPath);
	HANDLE hFindFile = FindFirstFile(fn, &FindFileData);
	m_arrPlateFileName.RemoveAll();
	m_arrAngleFileName.RemoveAll();
	if(hFindFile==INVALID_HANDLE_VALUE)
	{
		AfxMessageBox("未找到构件工艺信息中性文件(*.ppi)!\n");
		return;
	}
	do{
		CBuffer buffer;
		if(!LoadBufferFromFile(sFolderPath+"\\"+FindFileData.cFileName,buffer))
			continue;
		buffer.SeekToBegin();
		BYTE cPartType=CProcessPart::ReadPartTypeFromBuffer(buffer);
		if(cPartType==CProcessPart::TYPE_LINEANGLE)
			m_arrAngleFileName.Add(FindFileData.cFileName);
		else if(cPartType==CProcessPart::TYPE_PLATE)
			m_arrPlateFileName.Add(FindFileData.cFileName);
	}while(FindNextFile(hFindFile,&FindFileData));
	if(m_arrPlateFileName.GetSize()>0)
		CQuickSort<CString>::QuickSort(m_arrPlateFileName.GetData(),m_arrPlateFileName.GetSize(),CompareFileName);
	if(m_arrAngleFileName.GetSize()>0)
		CQuickSort<CString>::QuickSort(m_arrAngleFileName.GetData(),m_arrAngleFileName.GetSize(),CompareFileName);
	FindClose(hFindFile);
}
void CProcessPartsHolder::JumpToFirstPart()
{
	m_iCurFileIndex=0;
	UpdateCurPartFromFile(m_iCurFileIndex);
}
void CProcessPartsHolder::JumpToPrePart()
{
	if(m_iCurFileIndex>0)
		m_iCurFileIndex--;
	UpdateCurPartFromFile(m_iCurFileIndex);
}
void CProcessPartsHolder::JumpToNextPart()
{
	if(m_iCurFileIndex<m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1)
		m_iCurFileIndex++;
	UpdateCurPartFromFile(m_iCurFileIndex);
}
void CProcessPartsHolder::JumpToLastPart()
{
	m_iCurFileIndex=m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1;
	UpdateCurPartFromFile(m_iCurFileIndex);
}
void CProcessPartsHolder::JumpToSpecPart(char *sPartFileName)
{
	BOOL bContinue=TRUE;
	for(int i=0;i<m_arrAngleFileName.GetCount();i++)
	{
		if(m_arrAngleFileName[i].CompareNoCase(sPartFileName)==0)
		{
			m_iCurFileIndex=i;
			bContinue=FALSE;
			break;
		}
	}
	if(bContinue)
	{
		for(int i=0;i<m_arrPlateFileName.GetCount();i++)
		{
			if(m_arrPlateFileName[i].CompareNoCase(sPartFileName)==0)
			{
				m_iCurFileIndex=i+m_arrAngleFileName.GetCount();
				break;
			}
		}
	}
	UpdateCurPartFromFile(m_iCurFileIndex);
}
void CProcessPartsHolder::CreateAllPlateDxfFile()
{
	char sFilePath[MAX_PATH]="";
	for(int i=0;i<m_arrPlateFileName.GetSize();i++)
	{
		m_iCurFileIndex=i;
		UpdateCurPartFromFile(m_iCurFileIndex);
		sprintf(sFilePath,GetCurFileNameByIndex(m_iCurFileIndex));
		int len=strlen(sFilePath);
		if(strlen(sFilePath)>3)
		{
			sFilePath[len-3]='d';
			sFilePath[len-2]='x';
			sFilePath[len-1]='f';
			g_pPartEditor->CreatePlateDxfFile(sFilePath);
		}
	}
}
void CProcessPartsHolder::CreateAllPlateTtpFile()
{
	char sFilePath[MAX_PATH]="";
	for(int i=0;i<m_arrPlateFileName.GetSize();i++)
	{
		m_iCurFileIndex=i;
		UpdateCurPartFromFile(m_iCurFileIndex);
		sprintf(sFilePath,GetCurFileNameByIndex(m_iCurFileIndex));
		int len=strlen(sFilePath);
		if(strlen(sFilePath)>3)
		{
			sFilePath[len-3]='t';
			sFilePath[len-2]='t';
			sFilePath[len-1]='p';
			g_pPartEditor->CreatePlateDxfFile(sFilePath);
		}
	}
}
void CProcessPartsHolder::CreateAllAngleNcFile(char* drv_path,char* sPartNoPrefix)
{
	char sNcFolder[MAX_PATH]="";
	strcpy(sNcFolder,m_sFolderPath);
	strcat(sNcFolder,"\\NC\\");
	MakeDirectory(sNcFolder);
	for(int i=0;i<m_arrAngleFileName.GetSize();i++)
	{
		m_iCurFileIndex=i;
		UpdateCurPartFromFile(m_iCurFileIndex);
		g_pPartEditor->GenAngleNcFile(drv_path,sNcFolder,sPartNoPrefix);
	}
}
CXhChar200 CProcessPartsHolder::AddPart(CProcessPart *pPart)
{
	if(pPart==NULL)
		return "";
	char sFilePath[MAX_PATH];
	CXhChar200 sFileName("%s#%s%c.ppi",(char*)pPart->GetPartNo(),(char*)pPart->GetSpec(),pPart->cMaterial);
	if(m_sFolderPath.GetLength()<=0)
	{
		m_sFolderPath="D:\\构件中性文件";
		MakeDirectory(m_sFolderPath.GetBuffer());
	}
	sprintf(sFilePath,"%s\\%s",m_sFolderPath,(char*)sFileName);
	FILE *fp=fopen(sFilePath,"wb");
	if(fp)
	{
		CBuffer buffer;
		pPart->ToBuffer(buffer);
		buffer.SeekToBegin();
		long file_len=buffer.GetLength();
		fwrite(&file_len,sizeof(long),1,fp);
		fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
		fclose(fp);
		if(pPart->m_cPartType==CProcessPart::TYPE_PLATE)
			m_arrPlateFileName.Add(sFileName);
		else if(pPart->m_cPartType==CProcessPart::TYPE_LINEANGLE)
			m_arrAngleFileName.Add(sFileName);
		return sFileName;
	}
	return "";
}
CProcessPart* CProcessPartsHolder::GetCurProcessPart()
{
	if(m_iCurFileIndex<0&&m_iCurFileIndex>m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1)
		return NULL;
	CBuffer partBuffer;
	if(!g_pPartEditor->GetProcessPart(partBuffer))
		return NULL;
	partBuffer.SeekToBegin();
	if(m_cPartType==CProcessPart::TYPE_PLATE)
	{
		m_xPlate.FromBuffer(partBuffer);
		return &m_xPlate;
	}
	else if(m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		m_xAngle.FromBuffer(partBuffer);
		return &m_xAngle;
	}
	else 
		return NULL;
}
bool CProcessPartsHolder::UpdatePartInfoToEditor()
{
	if(m_iCurFileIndex<0&&m_iCurFileIndex>m_arrPlateFileName.GetCount()+m_arrAngleFileName.GetCount()-1)
		return false;
	CBuffer partBuffer;
	if(m_cPartType==CProcessPart::TYPE_PLATE)
	{
		m_xPlate.ToBuffer(partBuffer);
		g_pPartEditor->UpdateProcessPart(partBuffer);
	}
	else if(m_cPartType==CProcessPart::TYPE_LINEANGLE)
	{
		m_xAngle.ToBuffer(partBuffer);
		g_pPartEditor->UpdateProcessPart(partBuffer);
	}
	else 
		return false;
	SaveCurPartInfoToFile();
	return true;
}
void CProcessPartsHolder::EscapeCurEditPart()
{
	m_cPartType=-1;
	g_pPartEditor->ClearProcessParts();
}
CProcessPartsHolder g_partInfoHodler;
ISolidDraw *g_pSolidDraw;
ISolidSet *g_pSolidSet;
ISolidSnap *g_pSolidSnap;
ISolidOper *g_pSolidOper;
I2dDrawing *g_p2dDraw;
IPEC* g_pPartEditor;