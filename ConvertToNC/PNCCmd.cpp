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
#include "folder_dialog.h"
#include "DimStyle.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//智能提取板的信息,提取成功后支持排版
//SmartExtractPlate
//////////////////////////////////////////////////////////////////////////
void SmartExtractPlate()
{
	//获取当前激活文件
	CString file_name;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc)
		file_name = pDoc->fileName();
	if (file_name.GetLength() <= 0 || file_name.CompareNoCase("Drawing1.dwg") == 0)
	{
		AfxMessageBox("请打开有效的文件!");
		return;
	}
	//提取当前文件的钢板信息
	CPNCModel::m_bSendCommand = FALSE;
	model.Empty();
	model.m_sCurWorkFile = file_name;
	SmartExtractPlate(&model);
}

void SmartExtractPlate(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	struct resbuf var;
#ifdef _ARX_2007
	acedGetVar(L"WORLDUCS", &var);
#else
	acedGetVar("WORLDUCS", &var);
#endif
	int iRet = var.resval.rint;
	if (iRet == 0)
	{	//用户坐标系与世界坐标系不一致，执行ucs命令，修订坐标系 wht 20-04-24
		if(CPNCModel::m_bSendCommand)
#ifdef _ARX_2007
			SendCommandToCad(L"ucs w ");
#else
			SendCommandToCad("ucs w ");
#endif
		else
		{
#ifdef _ARX_2007
			acedCommand(RTSTR, L"ucs", RTSTR, L"w", RTNONE);
#else
			acedCommand(RTSTR, "ucs", RTSTR, "w", RTNONE);
#endif
		}
	}
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
	AcDbEntity *pEnt = NULL;
	int index = 1,nNum= selectedEntList.GetNodeNum();
	DisplayProgress(0, "查找图纸文字标注,识别钢板件号信息.....");
	for (AcDbObjectId entId=selectedEntList.GetFirst(); entId.isValid();entId=selectedEntList.GetNext(), index++)
	{
		DisplayProgress(int(100 * index / nNum));
		CAcDbObjLife objLife(entId);
		if((pEnt = objLife.GetEnt())==NULL)
			continue;
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()) && !pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		textIdHash.SetValue(entId.asOldId(), entId);
		//
		BASIC_INFO baseInfo;
		GEPOINT dim_pos, dim_vec;
		CXhChar16 sPartNo;
		if (pEnt->isKindOf(AcDbText::desc()) || pEnt->isKindOf(AcDbMText::desc()))
		{
			if(!g_pncSysPara.ParsePartNoText(pEnt,sPartNo))
				continue;
			if (strlen(sPartNo) <= 0)
			{
				CXhChar500 sText = GetCadTextContent(pEnt);
				logerr.Log("钢板信息{%s}满足设置的文字识别规则，但识别失败请联系信狐客户!", (char*)sText);
				continue;
			}
			else
				dim_pos = GetCadTextDimPos(pEnt, &dim_vec);
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
				continue;
			if (baseInfo.m_sPartNo.GetLength() > 0)
				sPartNo.Copy(baseInfo.m_sPartNo);
			else
			{
				logerr.Log("通过图块识别钢板基本信息,识别失败请联系信狐客户!");
				continue;
			}
		}
		CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//件号相同，但件号文本对应的实体不相同提示件号重复 wht 19-07-22
			logerr.Log("件号{%s}有重复请确认!", (char*)sPartNo);
			continue;
		}
		//
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
			pPlateProcess->xPlate.m_nProcessNum = baseInfo.m_nNum;
			pPlateProcess->xPlate.m_nSingleNum = baseInfo.m_nNum;
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
	DisplayProgress(100);
	if(pModel->GetPlateNum()<=0)
	{
		if (AfxMessageBox("该文件不满足文字识别配置！是否调整文字识别配置？", MB_YESNO) == IDYES)
		{
			CAcModuleResourceOverride resOverride;
			CPNCSysSettingDlg dlg;
			dlg.m_iSelTabGroup = 1;
			dlg.DoModal();
		}
		return;
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
	int nSum = 0;
	nNum = pModel->GetPlateNum();
	DisplayProgress(0,"修订钢板信息<基本+螺栓+火曲>.....");
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE),nSum++)
	{
		DisplayProgress(int(100 * nSum / nNum));
		pPlateProcess->ExtractPlateRelaEnts();
		pPlateProcess->CheckProfileEdge();
		if(!pPlateProcess->UpdatePlateInfo())
			logerr.Log("件号%s板选择了错误的边界,请重新选择.(位置：%s)",(char*)pPlateProcess->GetPartNo(),(char*)CXhChar50(pPlateProcess->dim_pos));
		
	}
	DisplayProgress(100);
	//将提取的钢板信息导出到中性文件中
	CString file_path;
#ifdef __ALFA_TEST_
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		GetCurWorkPath(file_path, TRUE, "pixel", TRUE);
	else
		GetCurWorkPath(file_path, TRUE, "rule", TRUE);
#else
	GetCurWorkPath(file_path);
#endif
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE))
	{	//生成PPI文件,保存到到当前工作路径下
		if(pPlateProcess->IsValid())
			pPlateProcess->CreatePPiFile(file_path);
		//完成提取后统一设置钢板提取状态为FALSE wht 19-06-17
		pPlateProcess->m_bNeedExtract = FALSE;
	}
	//写工程塔型配置文件 wht 19-01-12
	if (pModel->m_sTaType.GetLength() > 0)
	{
		CString cfg_path = file_path + "config.ini";
		cfg_path.Format("%sconfig.ini", file_path);
		pModel->WritePrjTowerInfoToCfgFile(cfg_path);
	}
	//绘制提取的钢板外形--支持排版
	pModel->DrawPlates();
	//通过菜单提取的钢板需更新构件列表
	if (CPNCModel::m_bSendCommand == FALSE)
	{
		CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
		if (pPartListDlg != NULL)
			pPartListDlg->UpdatePartList();
	}
#else
	//UBOM只需要更新钢板的基本信息
	pModel->MergeManyPartNo();
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt==NULL)
			continue;
		if(!pEnt->isKindOf(AcDbText::desc())&&!pEnt->isKindOf(AcDbMText::desc()))
			continue;
		GEPOINT dim_pos=GetCadTextDimPos(pEnt);
		CPlateProcessInfo* pPlateInfo = pModel->GetPlateInfo(dim_pos);
		if(pPlateInfo==NULL)
			continue;	//
		BASIC_INFO baseInfo;
		if (g_pncSysPara.RecogBasicInfo(pEnt, baseInfo))
		{
			if (baseInfo.m_cMat > 0)
				pPlateInfo->xPlate.cMaterial = baseInfo.m_cMat;
			if (baseInfo.m_cQuality > 0)
				pPlateInfo->xPlate.cQuality = baseInfo.m_cQuality;
			if (baseInfo.m_nThick > 0)
				pPlateInfo->xPlate.m_fThick = (float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
				pPlateInfo->xPlate.m_nSingleNum = pPlateInfo->xPlate.m_nProcessNum = baseInfo.m_nNum;
			if (baseInfo.m_idCadEntNum != 0)
				pPlateInfo->partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
		}
		//else //焊接字样可能在基本信息中例如：141#正焊
		{
			CXhChar100 sText = GetCadTextContent(pEnt);
			if (strstr(sText, "卷边") || strstr(sText, "火曲") || 
				strstr(sText, "外曲") || strstr(sText, "内曲")||
				strstr(sText, "正曲") || strstr(sText, "反曲"))
				pPlateInfo->xBomPlate.siZhiWan = 1;
			if (strstr(sText,"焊"))
				pPlateInfo->xBomPlate.bWeldPart = TRUE;
		}
	}
	pModel->SplitManyPartNo();
	for (CPlateProcessInfo* pPlateInfo = pModel->EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = pModel->EnumNextPlate(FALSE))
	{
		pPlateInfo->xBomPlate.sPartNo = pPlateInfo->GetPartNo();
		pPlateInfo->xBomPlate.cMaterial = pPlateInfo->xPlate.cMaterial;
		pPlateInfo->xBomPlate.cQualityLevel = pPlateInfo->xPlate.cQuality;
		pPlateInfo->xBomPlate.thick = pPlateInfo->xPlate.m_fThick;
		pPlateInfo->xBomPlate.feature1 = pPlateInfo->xPlate.m_nProcessNum;	//加工数
		pPlateInfo->xBomPlate.SetPartNum(pPlateInfo->xPlate.m_nSingleNum);	//单基数
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("钢板%s信息提取失败!", (char*)pPlateInfo->xBomPlate.sPartNo);
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
	if(pExistPlate->IsValid())
		pExistPlate->CreatePPiFile(file_path);
	//绘制提取的钢板外形--支持排版
	pModel->DrawPlates();
	//更新构件列表
	CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
#else
	pExistPlate->xBomPlate.cMaterial = pExistPlate->xPlate.cMaterial;
	pExistPlate->xBomPlate.cQualityLevel = pExistPlate->xPlate.cQuality;
	pExistPlate->xBomPlate.thick = pExistPlate->xPlate.m_fThick;
	pExistPlate->xBomPlate.sSpec.Printf("-%.f", pExistPlate->xBomPlate.thick);
	pExistPlate->xBomPlate.feature1 = pExistPlate->xPlate.m_nProcessNum;	//加工数
	pExistPlate->xBomPlate.SetPartNum(pExistPlate->xPlate.m_nSingleNum);	//单基数
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
//显示钢板信息
//ShowPartList
//////////////////////////////////////////////////////////////////////////
void ShowPartList()
{	
	g_xDockBarManager.DisplayPartListDockBar(CPartListDlg::m_nDlgWidth);
	//更新构件列表
	CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
}
//////////////////////////////////////////////////////////////////////////
//绘制钢板
//DrawPlates
//////////////////////////////////////////////////////////////////////////
void DrawPlates()
{
	int draw_type = 0, idRet = 0, group_type = 0;
	BOOL bDrawByVert = TRUE;
#ifdef _ARX_2007
	ACHAR result[20] = { 0 };
	idRet = acedGetString(NULL, L"\n输入绘制模式[对比(1)/排版(2)/下料(3)/筛选(4)]]<1>:", result);
	if (idRet == RTNORM)
		draw_type = (wcslen(result) > 0) ? atoi(_bstr_t(result)) : 1;
	if(draw_type == 1)
	{
		if (acedGetString(NULL, L"\n布局以列为主？[是(Y)/否(N)]<Y>:", result) == RTNORM)
		{
			if (wcslen(result) <= 0 || wcsicmp(result, L"Y") == 0 || wcsicmp(result, L"y") == 0)
				bDrawByVert = TRUE;
			else
				bDrawByVert = FALSE;
		}
	}
	else if (draw_type == 4)
	{
		if (acedGetString(NULL, L"\n筛选方式[段号(1)/材质&厚度(2)/材质(3)/厚度(4)]<1>:", result) == RTNORM)
		{
			if (wcslen(result) <= 0 || result[0] == '1')
				group_type = 1;
			else
				group_type = result[0] - '0';
		}
	}
#else
	char result[20] = "";
	idRet = acedGetString(NULL, _T("\n输入绘制模式[对比(1)/排版(2)/下料(3)/筛选(4)]<1>:"), result);
	if (idRet == RTNORM)
		draw_type = (strlen(result) > 0) ? atoi(result) : 1;
	if (draw_type == 1)
	{
		if (acedGetString(NULL, _T("\n布局以列为主？[是(Y)/否(N)]<Y>:"), result) == RTNORM)
		{
			if (strlen(result) <= 0 || stricmp(result, "Y") == 0 || stricmp(result, "y") == 0)
				bDrawByVert = TRUE;
			else
				bDrawByVert = FALSE;
		}
	}
	else if (draw_type == 4)
	{
		if (acedGetString(NULL, _T("\n筛选方式[段号(1)/材质&厚度(2)/材质(3)/厚度(4)]<1>:"), result) == RTNORM)
		{
			if (strlen(result) <= 0 || result[0] == '1')
				group_type = 1;
			else
				group_type = result[0] - '0';
		}
	}
#endif
	if(draw_type==0)
	{
#ifdef _ARX_2007
		acutPrintf(L"\n无效的输入!");
#else
		acutPrintf(_T("\n无效的输入!"));
#endif
		return;
	}
	//绘制
	g_pncSysPara.m_ciLayoutMode = draw_type;
	if (draw_type == 1)
		g_pncSysPara.m_ciArrangeType = bDrawByVert;
	else if (draw_type == 4)
		g_pncSysPara.m_ciGroupType = group_type;
	model.DrawPlates();
	//更新构件列表
	CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
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
bool ReadVertexArrFromFile(const char* filePath, vector <CPlateObject::VERTEX>& vertexArr)
{
	if (filePath == NULL)
		return false;
	FILE *fp = fopen(filePath, "rt");
	if (fp == NULL)
		return false;
	char line_txt[MAX_PATH] = { 0 };
	GEPOINT prevPt, curPt, center;
	while (!feof(fp))
	{
		if (fgets(line_txt, 200, fp) == NULL)
			continue;
		CXhChar200 sLine(line_txt);
		BOOL bHasX = (sLine.Replace('X', ' ') > 0) ? TRUE : FALSE;
		BOOL bHasY = (sLine.Replace('Y', ' ') > 0) ? TRUE : FALSE;
		BOOL bHasI = (sLine.Replace('I', ' ') > 0) ? TRUE : FALSE;
		BOOL bHasJ = (sLine.Replace('J', ' ') > 0) ? TRUE : FALSE;
		strcpy(line_txt, sLine);
		char szTokens[] = " =\n";
		char* skey = strtok(line_txt, szTokens);
		if (skey == NULL)
			continue;
		double x = 0, y = 0, i = 0, j = 0;
		CPlateObject::VERTEX vertrex;
		if (strstr(skey, "G00"))
		{	//快速定位点
			if (bHasX)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					x = atof(skey);
			}
			if (bHasY)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					y = atof(skey);
			}
			curPt.x = prevPt.x + x;
			curPt.y = prevPt.y + y;
			//
			vertrex.pos = curPt;
			vertrex.tag.lParam = 1;
			vertexArr.push_back(vertrex);
			prevPt = curPt;
		}
		else if (strstr(skey, "G01"))
		{	//直线插补
			if (bHasX)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					x = atof(skey);
			}
			if (bHasY)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					y = atof(skey);
			}
			curPt.x = prevPt.x + x;
			curPt.y = prevPt.y + y;
			//
			vertrex.pos = curPt;
			vertexArr.push_back(vertrex);
			prevPt = curPt;
		}
		else if (strstr(skey, "G02")|| strstr(skey, "G03"))
		{	//顺时针圆弧插补
			if (bHasX)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					x = atof(skey);
			}
			if (bHasY)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					y = atof(skey);
			}
			if (bHasI)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					i = atof(skey);
			}
			if (bHasJ)
			{
				skey = strtok(NULL, szTokens);
				if (skey)
					j = atof(skey);
			}
			curPt.x = prevPt.x + x;
			curPt.y = prevPt.y + y;
			center.x = prevPt.x + i;
			center.y = prevPt.y + j;
			//查找前一个顶点
			for (int index = 0; index < vertexArr.size(); index++)
			{
				if (vertexArr.at(index).pos.IsEqual(prevPt, EPS2)&&
					vertexArr.at(index).ciEdgeType == 1)
				{
					vertexArr.at(index).ciEdgeType = 2;
					vertexArr.at(index).arc.center = center;
					vertexArr.at(index).arc.radius = DISTANCE(center, curPt);
					if(strstr(skey, "G02"))
						vertexArr.at(index).arc.work_norm.Set(0, 0, -1);
					else
						vertexArr.at(index).arc.work_norm.Set(0, 0, 1);
					break;
				}
			}
			//添加新顶点
			vertrex.pos = curPt;
			vertexArr.push_back(vertrex);
			prevPt = curPt;
		}
		else
			continue;
	}
	fclose(fp);
	return true;
}

void DrawProfileByTxtFile()
{	
	CFileDialog dlg(TRUE, "txt", "切割NC文件", 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "工程文件(*.txt)|*.txt||");
	if (dlg.DoModal() != IDOK)
		return;
	//
	vector <CPlateObject::VERTEX> vertexArr;
	ReadVertexArrFromFile(dlg.GetPathName(), vertexArr);
	//
	DRAGSET.ClearEntSet();
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	//绘制轮廓边
	int n = vertexArr.size();
	for (int i = 0; i < n-1; i++)
	{
		CPlateObject::VERTEX curVer = vertexArr.at(i);
		CPlateObject::VERTEX nxtVer = vertexArr.at((i + 1) % n);
		CreateAcadPoint(pBlockTableRecord, curVer.pos);
		DimText(pBlockTableRecord, curVer.pos, CXhChar16("%d", i),
			TextStyleTable::hzfs.textStyleId, 2, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		//路径
		if (nxtVer.tag.lParam == 1)
			continue;	//下段路径的起始
		GEPOINT ptS = curVer.pos, ptE = nxtVer.pos;
		if (curVer.ciEdgeType == 1)
			CreateAcadLine(pBlockTableRecord, ptS, ptE);
		else
		{	//圆弧
			GEPOINT org = curVer.arc.center, norm = curVer.arc.work_norm;
			f3dArcLine arcline;
			arcline.CreateMethod3(ptS, ptE, norm, curVer.arc.radius, org);
			CreateAcadArcLine(pBlockTableRecord, arcline.Center(), arcline.Start(), arcline.SectorAngle(), arcline.WorkNorm());
		}
	}
	pBlockTableRecord->close();
	//
	ads_point base;
	base[X] = 0;
	base[Y] = 0;
	base[Z] = 0;
	DragEntSet(base, "请点取构件图的插入点");
#ifdef _ARX_2007
	acedCommand(RTSTR, L"PDMODE", RTSTR, L"34", RTNONE);	//显示点
#else
	acedCommand(RTSTR, "PDMODE", RTSTR, "34", RTNONE);		//显示点 X
#endif
}
//////////////////////////////////////////////////////////////////////////
//打碎文本,将文本类型转换为多段线
//ExplodeText
//////////////////////////////////////////////////////////////////////////
void ExplodeText()
{
	CLogErrorLife logErrLife;
	CAcModuleResourceOverride useThisRes;
	g_xDockBarManager.DisplayExplodeTxtDockBar();
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
	CLogErrorLife logErrLife;
	if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE)||
		g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_BATCH_PRINT))
	{	//加载角钢工艺卡
		char APP_PATH[MAX_PATH] = "", sJgCardPath[MAX_PATH] = "";
		GetAppPath(APP_PATH);
		sprintf(sJgCardPath, "%s%s", APP_PATH, (char*)g_pncSysPara.m_sJgCadName);
		if (!g_pncSysPara.InitJgCardInfo(sJgCardPath))
		{
			logerr.Log(CXhChar200("角钢工艺卡读取失败(%s)!", sJgCardPath));
			return;
		}
	}
	//显示对话框
	int nWidth = g_xUbomModel.InitBomTitle();
	g_xDockBarManager.DisplayRevisionDockBar(nWidth);
}
#endif
//////////////////////////////////////////////////////////////////////////
//内部测试代码
//InternalTest
//////////////////////////////////////////////////////////////////////////
#ifdef __ALFA_TEST_
#include "TestCode.h"
void InternalTest()
{
	return TestPnc();
}
#endif