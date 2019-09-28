// PPEDoc.cpp : implementation of the CPPEDoc class
//

#include "stdafx.h"
#include "PPE.h"
#include "PPEDoc.h"
#include "PPEView.h"
#include "ProcessPart.h"
#include "NewPartFileDlg.h"
#include "PartLib.h"
#include "MainFrm.h"
#include "PartTreeDlg.h"
#include "SysPara.h"
#include "PPEModel.h"
#include "NcPart.h"
#include "folder_dialog.h"
#include "LicFuncDef.h"
#include "ParseAdaptNo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
extern void MakeDirectory(char *path);
extern CXhChar200 GetValidFileName(CPPEModel *pModel, CProcessPart *pPart, const char* sPartNoPrefix = NULL);
/////////////////////////////////////////////////////////////////////////////
// CPPEDoc

IMPLEMENT_DYNCREATE(CPPEDoc, CDocument)

BEGIN_MESSAGE_MAP(CPPEDoc, CDocument)
	//{{AFX_MSG_MAP(CPPEDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_DXF, OnFileSaveDxf)
	ON_COMMAND(ID_FILE_SAVE_TTP, OnFileSaveTtp)
	ON_COMMAND(ID_FILE_SAVE_WKF, OnFileSaveWkf)
	ON_COMMAND(ID_FILE_SAVE_PBJ, OnFileSavePbj)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_PBJ,OnUpdateFileSavePbj)
	ON_COMMAND(ID_FILE_SAVE_PMZ, OnFileSavePmz)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_PMZ,OnUpdateFileSavePmz)
	ON_COMMAND(ID_FILE_SAVE_TXT,OnFileSaveTxt)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_TXT,OnUpdateFileSaveTxt)
	ON_COMMAND(ID_FILE_SAVE_NC,OnFileSaveNc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_NC,OnUpdateFileSaveNc)
	ON_COMMAND(ID_FILE_SAVE_CNC,OnFileSaveCnc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_CNC,OnUpdateFileSaveCnc)
	ON_COMMAND(ID_GEN_ANGLE_NC_FILE, &CPPEDoc::OnGenAngleNcFile)
	ON_UPDATE_COMMAND_UI(ID_GEN_ANGLE_NC_FILE,OnUpdateGenAngleNcFile)
	ON_COMMAND(ID_NEW_FILE, &CPPEDoc::OnNewFile)
	ON_COMMAND(ID_CREATE_PLATE_NC_DATA, OnCreatePlateNcData)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc construction/destruction

CPPEDoc::CPPEDoc()
{
	
}

CPPEDoc::~CPPEDoc()
{
}

BOOL CPPEDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	SetTitle("构件编辑器");
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPPEDoc serialization

void CPPEDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc diagnostics

#ifdef _DEBUG
void CPPEDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPPEDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc commands

CView* CPPEDoc::GetView(const CRuntimeClass *pClass)
{
	CView *pView;
	POSITION position;
	position = GetFirstViewPosition();
	for(;;)
	{
		if(position==NULL)
		{
			pView = NULL;
			break;
		}
		pView = GetNextView(position);
		if(pView->IsKindOf(pClass))
			break;
	}
	return pView;
}

void CPPEDoc::OpenFolder(const char* sFolderPath)
{
	if (strlen(sFolderPath) <= 0)
		return;
	model.InitModelByFolderPath(sFolderPath);
	CPartTreeDlg *pPartTreeDlg = ((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
	if (pPartTreeDlg)
		pPartTreeDlg->InitTreeView();
	//打开文件夹之后，根据配置初始化孔径增大值 wht 19-04-09
	CPPEView *pPPEView = theApp.GetView();
	if (pPPEView)
	{	//将当前构件设置为空，避免连续打开多个文件夹导致死机
		pPPEView->SetCurProcessPart(NULL);
		pPPEView->AmendHoleIncrement();
	}
}
void CPPEDoc::OnFileOpen() 
{
	CString sFolder= GetPathName();
	if (sFolder.GetLength() <= 0)
		sFolder.Format("%s", APP_PATH);
	if (InvokeFolderPickerDlg(sFolder))
	{
		OpenFolder(sFolder);
		SetPathName(sFolder);
	}
}
static bool CompareProcessPart(CProcessPart *pOrgPart,CProcessPart *pCurPart)
{
	if(pOrgPart->IsEqual(pCurPart))
	{
		if(!pOrgPart->mkpos.IsEqual(pCurPart->mkpos.x,pCurPart->mkpos.y,pCurPart->mkpos.z))
			return false;
		else if(pOrgPart->IsPlate()&&((CProcessPlate*)pOrgPart)->mcsFlg.wFlag!=((CProcessPlate*)pCurPart)->mcsFlg.wFlag)
			return false;
		else
			return true;
	}
	else 
		return false;
}
static void GetNeedReturnPartSet(CXhPtrSet<CProcessPart> *pPartSet)
{
	CPPEModel orgModel;
	theApp.m_xPPEModelBuffer.SeekToBegin();
	orgModel.FromBuffer(theApp.m_xPPEModelBuffer);
	for(CProcessPart *pPart=model.EnumPartFirst();pPart;pPart=model.EnumPartNext())
	{
		CProcessPart *pOrgPart=orgModel.FromPartNo(pPart->GetPartNo());
		if( pPart->m_dwInheritPropFlag==CProcessPart::PATTERN_OVERWRITE||	//覆盖模式构件均需存储
			(pPart->m_dwInheritPropFlag>0&&
			(pOrgPart==NULL||!CompareProcessPart(pOrgPart,pPart))))
			pPartSet->append(pPart);
	}
}
BOOL CPPEDoc::WriteToParentProcess()
{
	HANDLE hPipeWrite=NULL;
	hPipeWrite= GetStdHandle( STD_OUTPUT_HANDLE );

	if( hPipeWrite == INVALID_HANDLE_VALUE )
	{
		AfxMessageBox("获取管道句柄无效\n");
		return FALSE;
	}
	CProcessPart *pProcessPart=NULL;
	CPPEView *pView=(CPPEView*)GetView(RUNTIME_CLASS(CPPEView));
	if(pView==NULL||(pProcessPart=pView->GetCurProcessPart())==NULL)
		return FALSE;
	CBuffer buffer(10000);	//10kb
	//向匿名管道中写入数据返回给服务器
	buffer.WriteByte(1);		//数据继续标识
	buffer.WriteInteger(0);		//工艺构件缓存长度
	if(!theApp.starter.IsMultiPartsMode())
	{
		buffer.WriteDword(1);
		pProcessPart->ToPPIBuffer(buffer);
	}
	else
	{
		CXhPtrSet<CProcessPart> partSet;
		GetNeedReturnPartSet(&partSet);
		DWORD dwPartNum=partSet.GetNodeNum();
		buffer.WriteDword((DWORD)dwPartNum);
		for(CProcessPart *pPart=partSet.GetFirst();pPart;pPart=partSet.GetNext())
			pPart->ToPPIBuffer(buffer);
	}
	buffer.SeekPosition(1);		//写入实际工艺构件缓存长度
	buffer.WriteInteger(buffer.GetLength()-5);
	buffer.WriteToPipe(hPipeWrite,1024);
	((CMainFrame*)theApp.m_pMainWnd)->CloseProcess();
	return TRUE;
}

void CPPEDoc::OnFileSave() 
{
	if(theApp.starter.IsDuplexMode())
		WriteToParentProcess();
}

void CPPEDoc::OnFileSaveDxf()
{
	CNCPart::m_bDeformedProfile=TRUE;
	if(model.IsAllDeformedProfile()==FALSE)
	{
		if(AfxMessageBox("DXF文件中是否考虑火曲变形",MB_YESNO)==IDYES)
			CNCPart::m_bDeformedProfile=TRUE;
		else
			CNCPart::m_bDeformedProfile=FALSE;
	}
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_DXF_FILE);
}
void CPPEDoc::OnFileSaveTtp() 
{
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_TTP_FILE);
}
void CPPEDoc::OnFileSaveWkf()
{
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_WKF_FILE);
}
void CreatePlateNcData(int iNcMode, int iNcFileType);
void CPPEDoc::OnFileSavePbj()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	if(AfxMessageBox("PBJ文件中螺栓孔是否进行排序(已排序请选择否)",MB_YESNO)==IDYES)
		CNCPart::m_bSortHole=TRUE;
	else
		CNCPart::m_bSortHole=FALSE;
	CreatePlateNcData(CNCPart::PROCESS_MODE, CNCPart::PLATE_PBJ_FILE);
	//CNCPart::CreateAllPlateFiles(CNCPart::PLATE_PBJ_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSavePbj(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSavePmz()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	if(AfxMessageBox("PMZ文件中螺栓孔是否进行排序(已排序请选择否)",MB_YESNO)==IDYES)
		CNCPart::m_bSortHole=TRUE;
	else
		CNCPart::m_bSortHole=FALSE;
	CreatePlateNcData(CNCPart::PROCESS_MODE, CNCPart::PLATE_PMZ_FILE);
	//CNCPart::CreateAllPlateFiles(CNCPart::PLATE_PMZ_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSavePmz(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveTxt()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_TXT_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveTxt(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveNc()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_NC_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveNc(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveCnc()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CNCPart::CreateAllPlateFiles(CNCPart::PLATE_CNC_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveCnc(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}

static bool CreatePlateNcFiles(CHashStrList<PLATE_GROUP> &hashPlateByThickMat, char* thickSetStr,
							   const char* mainFolder,int iNcMode,int iNcFileType,BOOL bIsPmzCheck=FALSE)
{
	CXhChar16 sSubFolder;
	if (CNCPart::CUT_MODE == iNcMode)
		sSubFolder.Copy("切割下料");
	else if (CNCPart::PROCESS_MODE == iNcMode)
		sSubFolder.Copy("板床加工");
	else if (CNCPart::LASER_MODE == iNcMode)
		sSubFolder.Copy("激光复合机");
	else
		return false;
	CXhChar16 sNcFileFolder, sNcExt;
	if (CNCPart::PLATE_DXF_FILE == iNcFileType)		//钢板DXF类型文件
		sNcFileFolder.Copy("DXF");
	else if (CNCPart::PLATE_NC_FILE == iNcFileType)	//钢板NC类型文件
		sNcFileFolder.Copy("NC");
	else if (CNCPart::PLATE_TXT_FILE == iNcFileType)	//钢板TXT类型文件
		sNcFileFolder.Copy("TXT");
	else if (CNCPart::PLATE_CNC_FILE == iNcFileType)	//钢板CNC类型文件
		sNcFileFolder.Copy("CNC");
	else if (CNCPart::PLATE_PMZ_FILE == iNcFileType) //钢板PMZ类型文件
		sNcFileFolder.Copy("PMZ");
	else if (CNCPart::PLATE_PBJ_FILE == iNcFileType) //钢板PBJ类型文件
		sNcFileFolder.Copy("PBJ");
	else if (CNCPart::PLATE_TTP_FILE == iNcFileType)	//钢板TTP类型文件
		sNcFileFolder.Copy("TTP");
	else if (CNCPart::PLATE_WKF_FILE == iNcFileType)	//济南法特WKF文件
		sNcFileFolder.Copy("WKF");
	else
		return false;
	CFileFind fileFind;
	CXhChar16 sMat;
	CXhChar100 sTitle("进度");
	CXhChar500 sFilePath;
	sNcExt.Copy(sNcFileFolder);
	sNcExt.ToLower();
	sTitle.Printf("生成%s的%s文件进度", (char*)sSubFolder, (char*)sNcFileFolder);
	CXhChar500 sNcFileDir("%s\\%s\\%s", (char*)mainFolder, (char*)sSubFolder,(char*)sNcFileFolder);
	if (CNCPart::PLATE_PMZ_FILE == iNcFileType && bIsPmzCheck) 
		sNcFileFolder.Append("预审");	//钢板PMZ预审文件
	CHashList<SEGI> thickHash;
	if(thickSetStr)
		GetSegNoHashTblBySegStr(thickSetStr, thickHash);
	model.DisplayProcess(0, sTitle);
	int index = 0, num = model.PartCount();
	for (PLATE_GROUP *pPlateGroup = hashPlateByThickMat.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThickMat.GetNext())
	{
		if(thickHash.GetNodeNum()>0 && thickHash.GetValue(pPlateGroup->thick)==NULL)
			continue;
		if (g_sysPara.nc.m_iDxfMode == 1)	//按厚度创建文件目录
		{
			QuerySteelMatMark(pPlateGroup->cMaterial, sMat);
			sNcFileDir.Printf("%s\\%s\\%s\\厚度-%d-%s", (char*)mainFolder, (char*)sSubFolder, (char*)sNcFileFolder, pPlateGroup->thick, (char*)sMat);
		}
		if (!fileFind.FindFile(sNcFileDir))
			MakeDirectory(sNcFileDir);
		for (CProcessPlate *pPlate = pPlateGroup->plateSet.GetFirst(); pPlate; pPlate = pPlateGroup->plateSet.GetNext())
		{
			index++;
			model.DisplayProcess(ftoi(100 * index / num), sTitle);
			CXhChar100  sFileName = GetValidFileName(&model, pPlate);
			if (sFileName.Length() <= 0)
				continue;
			if(model.m_sTaType.GetLength()>0)
				sFilePath.Printf("%s\\%s-%s.%s", (char*)sNcFileDir,(char*)model.m_sTaType, (char*)sFileName, (char*)sNcExt);
			else
				sFilePath.Printf("%s\\%s.%s", (char*)sNcFileDir, (char*)sFileName,(char*)sNcExt);
			if (CNCPart::PLATE_DXF_FILE == iNcFileType)			//钢板DXF类型文件
				CNCPart::CreatePlateDxfFile(pPlate, sFilePath, iNcMode);
#ifdef __PNC_
			else if (CNCPart::PLATE_NC_FILE == iNcFileType)		//钢板NC类型文件
				CNCPart::CreatePlateNcFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_TXT_FILE == iNcFileType)	//钢板TXT类型文件
				CNCPart::CreatePlateTxtFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_CNC_FILE == iNcFileType)	//钢板CNC类型文件
				CNCPart::CreatePlateCncFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_PMZ_FILE == iNcFileType)	//钢板PMZ类型文件
			{
				if(bIsPmzCheck)
					CNCPart::CreatePlatePmzCheckFile(pPlate, sFilePath);
				else
					CNCPart::CreatePlatePmzFile(pPlate, sFilePath);
			}
			else if (CNCPart::PLATE_PBJ_FILE == iNcFileType)	//钢板PBJ类型文件
				CNCPart::CreatePlatePbjFile(pPlate, sFilePath);
#endif
			else if (CNCPart::PLATE_TTP_FILE == iNcFileType)	//钢板TTP类型文件
				CNCPart::CreatePlateTtpFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_WKF_FILE == iNcFileType)	//济南法特WKF文件
				CNCPart::CreatePlateWkfFile(pPlate, sFilePath);
		}
	}
	model.DisplayProcess(100, sTitle);
	return true;
}


static void CreatePlateNcData(int iNcMode, int iNcFileType)
{
#ifdef __PNC_
	//根据板厚对钢板进行分类
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		if (!pPart->IsPlate())
			continue;
		int thick = (int)(pPart->GetThick());
		CXhChar16 sMat;
		QuerySteelMatMark(pPart->cMaterial, sMat);
		CXhChar50 sKey("%s_%d", (char*)sMat, thick);
		PLATE_GROUP* pPlateGroup = hashPlateByThickMat.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateByThickMat.Add(sKey);
			pPlateGroup->thick = thick;
			pPlateGroup->cMaterial = pPart->cMaterial;
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->plateSet.append((CProcessPlate*)pPart);
	}
	//生成钢板所需NC数据
	CXhChar500 sFolder = model.GetFolderPath();
	CNCPart::m_bDeformedProfile = TRUE;	//PNC提取的钢板默认已考虑火曲变形
	CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, iNcMode, iNcFileType);
	if (CNCPart::PLATE_PMZ_FILE == iNcFileType && g_sysPara.pmz.m_bPmzCheck)	//输出钻床加工PMZ预审格式文件 wht 19-07-02
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, iNcMode, CNCPart::PLATE_PMZ_FILE, TRUE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}

void CPPEDoc::OnCreatePlateNcData()
{
#ifdef __PNC_
	//根据板厚对钢板进行分类
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		if (!pPart->IsPlate())
			continue;
		int thick = (int)(pPart->GetThick());
		CXhChar16 sMat;
		QuerySteelMatMark(pPart->cMaterial, sMat);
		CXhChar50 sKey("%s_%d", (char*)sMat, thick);
		PLATE_GROUP* pPlateGroup = hashPlateByThickMat.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateByThickMat.Add(sKey);
			pPlateGroup->thick = thick;
			pPlateGroup->cMaterial = pPart->cMaterial;
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->plateSet.append((CProcessPlate*)pPart);
	}
	//生成钢板所需NC数据
	CXhChar500 sFolder = model.GetFolderPath();
	CNCPart::m_bDeformedProfile = TRUE;	//PNC提取的钢板默认已考虑火曲变形
	if (g_sysPara.IsValidNcFlag(CNCPart::CUT_MODE))
	{	//切割下料
		if (g_sysPara.nc.m_bFlameCut)
		{	//火焰切割，根据用户输入厚度范围过滤钢板
			if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_TXT_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xFlamePara.m_sThick, sFolder, CNCPart::CUT_MODE, CNCPart::PLATE_TXT_FILE);
			if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_CNC_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xFlamePara.m_sThick, sFolder, CNCPart::CUT_MODE, CNCPart::PLATE_CNC_FILE);
			if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_DXF_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xFlamePara.m_sThick, sFolder, CNCPart::CUT_MODE, CNCPart::PLATE_DXF_FILE);
		}
		if (g_sysPara.nc.m_bPlasmaCut)
		{	//等离子切割，根据用户输入厚度范围过滤钢板
			if(g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_NC_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPlasmaPara.m_sThick, sFolder, CNCPart::CUT_MODE, CNCPart::PLATE_NC_FILE);
			if (g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPlasmaPara.m_sThick, sFolder, CNCPart::CUT_MODE, CNCPart::PLATE_DXF_FILE);
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::PROCESS_MODE))
	{	//板床加工
		if (g_sysPara.nc.m_bPunchPress)
		{	//冲床，根据用户输入厚度范围过滤钢板
			if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_PBJ_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPunchPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_PBJ_FILE);
			if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_WKF_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPunchPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_WKF_FILE);
			if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPunchPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_DXF_FILE);
		}
		if (g_sysPara.nc.m_bDrillPress)
		{	//钻床，根据用户输入厚度范围过滤钢板
			if(g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_PMZ_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_PMZ_FILE);
			if (g_sysPara.pmz.m_bPmzCheck)	//输出钻床加工PMZ预审格式文件 wht 19-07-02
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_PMZ_FILE,TRUE);
			if (g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
				CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, CNCPart::PROCESS_MODE, CNCPart::PLATE_DXF_FILE);
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
	{	//激光处理
		CreatePlateNcFiles(hashPlateByThickMat, NULL, sFolder, CNCPart::LASER_MODE, CNCPart::PLATE_DXF_FILE);
	}
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
void CPPEDoc::OnGenAngleNcFile()
{
#ifndef __PNC_
	CLogErrorLife logLife;
	if(g_sysPara.nc.m_sNcDriverPath.GetLength()<=0)
	{
		logerr.Log("没有选择角钢NC驱动文件");
		return;
	}
	CNCPart::CreateAllAngleNcFile(&model,g_sysPara.nc.m_sNcDriverPath.GetBuffer(),"",model.GetFolderPath());
#endif
}
void CPPEDoc::OnUpdateGenAngleNcFile(CCmdUI *pCmdUI)
{
#ifndef __PNC_
	pCmdUI->Enable(TRUE);
#else
	pCmdUI->Enable(FALSE);
#endif
}
char restore_JG_guige(char* guige, double &wing_wide, double &wing_thick)
{
	char mark,material;	//symbol='L' mark='X';
	sscanf(guige,"%lf%c%lf%c",&wing_wide,&mark,&wing_thick,&material);
	material  = (char)toupper(material);//角钢材质A3(S)或Q345(H)

	return material;
}
CXhChar500 GetValidWorkDir(const char* work_dir, CPPEModel *pModel = NULL, const char* sub_folder = NULL);
void CPPEDoc::OnNewFile()
{
	CNewPartFileDlg newFileDlg;
	if(newFileDlg.DoModal()!=IDOK)
		return;
	CXhChar500 sFolderPath=GetValidWorkDir(model.GetFolderPath());
	CProcessAngle processAngle;
	CProcessPlate processPlate;
	CProcessPart *pTempPart=NULL;
	if(newFileDlg.m_bImprotNcFile)
	{
#ifdef __PNC_
		processPlate.cMaterial='S';
		pTempPart=&processPlate;
		CFileDialog dlg(TRUE,"cnc",NULL,NULL,"TXT文件(*.txt)|*.txt|CNC文件(*.cnc)|*.cnc|All Files (*.*)|*.*||");
		if(dlg.DoModal()!=IDOK)
			return;
		CNCPart::ImprotPlateCncOrTextFile(&processPlate,dlg.GetPathName());
#endif
	}
	else
	{
		//1、初始化工艺角钢信息
		pTempPart=&processAngle;
		processAngle.SetPartNo(newFileDlg.m_sPartNo.GetBuffer());
		processAngle.cMaterial=QuerySteelBriefMatMark(newFileDlg.m_sMaterial.GetBuffer());
		double fWidth=0,fThick=0,fLen=0;
		restore_JG_guige(newFileDlg.m_sSpec.GetBuffer(),fWidth,fThick);
		processAngle.m_fWidth=(float)fWidth;
		processAngle.m_fThick=(float)fThick;
		processAngle.m_wLength=(WORD)atof(newFileDlg.m_sLen);
		CXhChar100 sSpec("L%s",newFileDlg.m_sSpec);
		processAngle.SetSpec(sSpec);
	}
	//2、将角钢信息保存到ppi文件中
	CString sFileName;
	sFileName.Format("%s#%s%c.ppi",(char*)pTempPart->GetPartNo(),(char*)pTempPart->GetSpec(),pTempPart->cMaterial);
	CXhChar200 sFilePath("%s%s",(char*)sFolderPath,sFileName);
	CProcessPart *pNewPart=model.AddPart(pTempPart->GetPartNo(),pTempPart->m_cPartType,sFilePath);
	pTempPart->ClonePart(pNewPart);
	CNCPart::CreatePPIFile(pNewPart,sFilePath);
	//3、刷新界面
	CPartTreeDlg *pPartTreeDlg=((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
	if(pPartTreeDlg)
		pPartTreeDlg->InsertTreeItem(sFileName.GetBuffer(),pNewPart);
	CPPEView *pView=theApp.GetView();
	if(pView)
	{
		pView->UpdateCurWorkPartByPartNo(pNewPart->GetPartNo());
		pView->Refresh();
		pView->UpdatePropertyPage();
	}
}
