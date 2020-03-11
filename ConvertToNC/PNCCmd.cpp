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

void UpdatePartList()
{
#ifndef __UBOM_ONLY_
	CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
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
	if (pDoc != NULL)
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
	CHashSet<AcDbObjectId> selectedEntList;
	//默认选择所有的图形，方便后期的过滤使用
	SelCadEntSet(pModel->m_xAllEntIdSet, TRUE);
#ifndef __UBOM_ONLY_
	//PNC支持进行手动框选
	if (!SelCadEntSet(selectedEntList))
		return;	
#else
	//UBOM默认处理所有图元
	for (AcDbObjectId entId = pModel->m_xAllEntIdSet.GetFirst(); entId; entId = pModel->m_xAllEntIdSet.GetNext())
		selectedEntList.SetValue(entId.asOldId(), entId);
#endif
	//从框选信息中提取中钢板的标识，统计钢板集合
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<GEPOINT> holePosList;
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId=selectedEntList.GetFirst(); entId.isValid();entId=selectedEntList.GetNext())
	{
		CAcDbObjLife objLife(entId);
		if((pEnt = objLife.GetEnt())==NULL)
			continue;
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()) && !pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		textIdHash.SetValue(entId.asOldId(), entId);
		//
		BASIC_INFO baseInfo;
		GEPOINT dim_pos, dim_vec;
		CXhChar500 sText;
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if (!g_pncSysPara.IsMatchPNRule(sText))
				continue;
			dim_pos = GetCadTextDimPos(pText, &dim_vec);
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
			dim_pos = GetCadTextDimPos(pMText, &dim_vec);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//从块中解析钢板信息
			AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
			if(g_pncSysPara.RecogBasicInfo(pBlockRef,baseInfo))
			{
				AcGePoint3d txt_pos = pBlockRef->position();
				dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
				dim_vec.Set(cos(pBlockRef->rotation()), sin(pBlockRef->rotation()));
			}
			else
			{
				BOLT_HOLE hole;
				if (g_pncSysPara.RecogBoltHole(pEnt, hole))
					holePosList.append(GEPOINT(hole.posX, hole.posY));
				continue;
			}
		}
		CXhChar16 sPartNo;
		if (baseInfo.m_sPartNo.GetLength() > 0)
			sPartNo.Copy(baseInfo.m_sPartNo);
		else
		{
			BYTE ciRetCode = g_pncSysPara.ParsePartNoText(sText, sPartNo);
			if (ciRetCode == CPlateExtractor::PART_LABEL_WELD)
				continue;	//当前件号为焊接子件件号 wht 19-07-22
		}
		if(strlen(sPartNo) <= 0)
		{
			logerr.Log("钢板信息%s,识别机制不能识别!", (char*)sText);
			continue;
		}
		CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//件号相同，但件号文本对应的实体不相同提示件号重复 wht 19-07-22
			logerr.Log("钢板%s,件号重复请确认!", (char*)sText);
			continue;
		}
		CPlateProcessInfo* pPlateProcess = pModel->AppendPlate(sPartNo);
		pPlateProcess->m_bNeedExtract = TRUE;	//选择CAD实体包括当前件号时设置位需要提取
		pPlateProcess->dim_pos = dim_pos;
		pPlateProcess->dim_vec = dim_vec;
		if (baseInfo.m_sPartNo.GetLength() > 0)
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
	for (CPlateProcessInfo* pPlateInfo = pModel->EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = pModel->EnumNextPlate(FALSE))
		pPlateInfo->xBomPlate.sPartNo = pPlateInfo->GetPartNo();
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt==NULL || !pEnt->isKindOf(AcDbText::desc()))
			continue;
		GEPOINT dim_pos=GetCadTextDimPos(pEnt);
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
					pPlateInfo->xBomPlate.feature1 = baseInfo.m_nNum;	//加工数
				if (baseInfo.m_idCadEntNum != 0)
					pPlateInfo->partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			}
			else
			{
				CXhChar100 sText = GetCadTextContent(pEnt);
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
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	CDwgFileInfo *pDwgFile = NULL;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc != NULL)
	{
		CString file_path = pDoc->fileName();
		pDwgFile = g_xUbomModel.FindDwgFile(file_path);
	}
	if (pDwgFile)
		ManualExtractPlate(pDwgFile->GetPncModel());
	else
		ManualExtractPlate(&model);
#else
	ManualExtractPlate(&model);
#endif
}
void ManualExtractPlate(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	CLogErrorLife logErrLife;
	//框选待修正钢板的轮廓边
	CHashSet<AcDbObjectId> selectedEntList;
	if (!SelCadEntSet(selectedEntList))
		return;
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
	CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(plate.GetPartNo());
	if (pExistPlate == NULL)
		pExistPlate = pModel->AppendPlate(plate.GetPartNo());
	pExistPlate->InitProfileBySelEnts(selectedEntList);
	pExistPlate->ExtractPlateRelaEnts();
	pExistPlate->UpdatePlateInfo(TRUE);
#ifndef __UBOM_ONLY_
	//生成PPI文件,保存到到当前工作路径下
	CString file_path;
	GetCurWorkPath(file_path);
	if(pExistPlate->vertexList.GetNodeNum()>3)
		pExistPlate->CreatePPiFile(file_path);
	//绘制提取的钢板外形--支持排版
	pModel->LayoutPlates(g_pncSysPara.m_bAutoLayout);
	//
	UpdatePartList();
#else
	pExistPlate->xBomPlate.cMaterial = pExistPlate->xPlate.cMaterial;
	pExistPlate->xBomPlate.cQualityLevel = pExistPlate->xPlate.cQuality;
	pExistPlate->xBomPlate.thick = pExistPlate->xPlate.m_fThick;
	pExistPlate->xBomPlate.sSpec.Printf("-%.f", pExistPlate->xBomPlate.thick);
	pExistPlate->xBomPlate.feature1 = pExistPlate->xPlate.feature;	//加工数
#endif
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
	//加载角钢工艺卡
	CLogErrorLife logErrLife;
	char APP_PATH[MAX_PATH]="", sJgCardPath[MAX_PATH]="";
	GetAppPath(APP_PATH);
	sprintf(sJgCardPath, "%s%s", APP_PATH, (char*)g_pncSysPara.m_sJgCadName);
	if(!g_pncSysPara.InitJgCardInfo(sJgCardPath))
	{
		logerr.Log(CXhChar200("角钢工艺卡读取失败(%s)!",sJgCardPath));
		return;
	}
	//显示对话框
	int nWidth = CRevisionDlg::GetDialogInitWidthByCustomizeSerial(g_xUbomModel.m_uiCustomizeSerial);
	g_xDockBarManager.DisplayRevisionDockBar(nWidth);
	CRevisionDlg* pRevisionDlg = g_xDockBarManager.GetRevisionDlgPtr();
	if (pRevisionDlg)
		pRevisionDlg->DisplayProcess = DisplayProcess;
}
#endif