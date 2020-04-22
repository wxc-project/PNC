#include "StdAfx.h"
#ifdef __BP_ONLY_
#include "StdArx.h"
#include "MenuFunc.h"
#include "BPSModel.h"
#include "OptimalSortDlg.h"
#include "LocalFeatureDef.h"
#include "XhLicAgent.h"
#include "RxTools.h"
#include "SysPara.h"
#include "UploadProcessCardDlg.h"
#include "ReConnServerDlg.h"
#include "objptr_list.h"
#include "ProcBarDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

struct ACAD_TEXT{
	AcDbObjectId entId;
	CXhChar100 sText;
	GEPOINT text_pos;
	CAngleProcessInfo *pAngleInfo;
	ACAD_TEXT(){pAngleInfo=NULL;}
	void Init(AcDbObjectId id,const char* text,GEPOINT &pos){
		entId=id;
		sText.Copy(text);
		text_pos=pos;
	}
};
struct ACAD_RECT{
	AcDbObjectId entId;
	POLYGON region;
	double fWidth;
	CAngleProcessInfo *pAngleInfo;
	ACAD_RECT(){pAngleInfo=NULL; fWidth=0;}
	void Init(AcDbPolyline *pPolyLine){
		if(pPolyLine==NULL)
			return;
		SCOPE_STRU scope;
		ARRAY_LIST<f3dPoint> vertex_list;
		entId=pPolyLine->id();
		int nNum=pPolyLine->numVerts();
		for(int iVertIndex = 0;iVertIndex<nNum;iVertIndex++)
		{
			AcGePoint3d location;
			pPolyLine->getPointAt(iVertIndex,location);
			vertex_list.append(f3dPoint(location.x,location.y,location.z));
			scope.VerifyVertex(f3dPoint(location.x,location.y,location.z));
		}
		region.CreatePolygonRgn(vertex_list.m_pData,vertex_list.GetSize());
		fWidth=scope.wide();
	}
};

/*CProcBarDlg *pProcDlg=NULL;
void DisplayProcess(int percent,const char *sTitle)
{
	if(percent>=100)
	{
		if(pProcDlg!=NULL)
		{
			pProcDlg->DestroyWindow();
			delete pProcDlg;
			pProcDlg=NULL;
		}
		return;
	}
	else if(pProcDlg==NULL)
		pProcDlg=new CProcBarDlg(NULL);
	if(pProcDlg->GetSafeHwnd()==NULL)
		pProcDlg->Create();
	static int prevPercent;
	if(percent!=0&&percent==prevPercent)
		return;	//跟上次进度一致不需要更新
	else
		prevPercent=percent;
	if(sTitle)
		pProcDlg->SetTitle(CString(sTitle));
	else
#ifdef AFX_TARG_ENU_ENGLISH
		pProcDlg->SetTitle("Process");
#else 
		pProcDlg->SetTitle("进度");
#endif
	pProcDlg->Refresh(percent);
}*/

void DisplayProcess(int percent,const char *sTitle);
//智能提取角钢
BOOL InitBpsModel(BOOL bEmptyModel)
{
	if(!ValidateLocalizeFeature(FEATURE::BATCH_INTELLI_PRINT_PROCESSCARD))
		return FALSE;
	//1.识别角钢工艺卡模板
	CXhChar100 APP_PATH,sJgCardPath="D:\\BP角钢工艺卡.dwg";
	//GetAppPath(APP_PATH);
	//sJgCardPath.Printf("%sBP角钢工艺卡.dwg",(char*)APP_PATH);
	//if(strlen(sys.general_set.m_sJgCard)>0)
	//	sJgCardPath.Copy(sys.general_set.m_sJgCard);
	if(!CIdentifyManager::InitJgCardInfo(sJgCardPath))
	{
		AfxMessageBox(CXhChar100("角钢工艺卡模板读取失败:%s!",(char*)sJgCardPath));
		return FALSE;
	}
	//2.选取DWG中角钢工艺卡，提取角钢信息
	if(bEmptyModel)
		BPSModel.Empty();
	int retCode=0;
	ads_name ent_sel_set;
#ifdef _ARX_2007
	if(bEmptyModel)	//清空模型是默认选择所有实体，批量修改或者查看时不需要选择所有实体 wht 17-12-19
		retCode=acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
	retCode=acedSSGet(L"",NULL,NULL,NULL,ent_sel_set);
#else
	if(bEmptyModel)
		retCode=acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
	retCode=acedSSGet("",NULL,NULL,NULL,ent_sel_set);
#endif
	if(retCode==RTCAN)
	{	//用户按ESC取消
		acedSSFree(ent_sel_set);
		return FALSE;
	}
	CHashSet<AcDbObjectId> textIdHash;
	CHashSet<AcDbObjectId> polyLineIdHash;
	long i=0,ll;
	acedSSLength(ent_sel_set,&ll);
	if(ll<=0)
		return FALSE;
	//1.将CAD实体分类统计
	CHashList<ACAD_TEXT> hashTextByEntId;		//文字
	CXhPtrSet<ACAD_TEXT> partNoTextList;		//件号
	CHashList<ACAD_RECT> hashRectByEntId;		//矩形框
	CHashList<ACAD_TEXT> hashAngleCardBlockRefByEntId;	//块引用
	DisplayProcess(0,"分析数据");
	for(i=0;i<ll;i++)
	{
		ads_name entname;
		acedSSName(ent_sel_set,i,entname);
		AcDbObjectId entId;
		acdbGetObjectId(entId,entname);
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		f3dPoint entPt;
		BOOL bInBlockRef=TRUE;
		if(pEnt->isKindOf(AcDbText::desc()))
		{	//角钢工艺卡非块，根据"件号"文字提取角钢信息
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			CXhChar50 sText(_bstr_t(pText->textString()));
#else
			CXhChar50 sText(pText->textString());
#endif
			ACAD_TEXT *pAcadText=hashTextByEntId.Add(entId.handle());
			pAcadText->Init(entId,sText,GEPOINT(pText->position().x,pText->position().y,0));
			if(strstr(sText,"件")==NULL || strstr(sText,"号")==NULL)
				continue;
			partNoTextList.append(pAcadText);
		}
		else if(pEnt->isKindOf(AcDbMText::desc()))
		{	//角钢工艺卡非块，根据"件号"文字提取角钢信息
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			CXhChar50 sText(_bstr_t(pMText->contents()));
#else
			CXhChar50 sText(pMText->contents());
#endif
			ACAD_TEXT *pAcadText=hashTextByEntId.Add(entId.handle());
			pAcadText->Init(entId,sText,GEPOINT(pMText->location().x,pMText->location().y,0));
			if(strstr(sText,"件")==NULL || strstr(sText,"号")==NULL)
				continue;
			partNoTextList.append(pAcadText);
		}
		else if(pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//根据角钢工艺卡块提取角钢信息
			AcDbBlockTableRecord *pTempBlockTableRecord=NULL;
			AcDbBlockReference* pReference=(AcDbBlockReference*)pEnt;
			AcDbObjectId blockId=pReference->blockTableRecord();
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
			ACAD_TEXT *pAcadText=hashAngleCardBlockRefByEntId.Add(entId.handle());
			pAcadText->Init(entId,sName,GEPOINT(pReference->position().x,pReference->position().y,0));
		}
		else if(pEnt->isKindOf(AcDbPolyline::desc()))
		{
			ACAD_RECT *pRect=hashRectByEntId.Add(entId.handle());
			pRect->Init((AcDbPolyline*)pEnt);
		}
		DisplayProcess((100*i)/ll,"分析数据");
	}
	acedSSFree(ent_sel_set);
	DisplayProcess(100,"分析数据");

	if(hashAngleCardBlockRefByEntId.GetNodeNum()<=0&&partNoTextList.GetNodeNum()<=0)
		return FALSE;
	//2.初始化角钢信息
	int nNewJgCount=0;
	CHashStrList<CAngleProcessInfo*> hashJgPtrByPartLabel;
	for(CAngleProcessInfo *pJgInfo=BPSModel.EnumFirstJg();pJgInfo;pJgInfo=BPSModel.EnumNextJg())
		hashJgPtrByPartLabel.SetValue(pJgInfo->m_sPartNo,pJgInfo);
	//2.1 根据块引用识别角钢
	i=0;
	int nSumCount=hashAngleCardBlockRefByEntId.GetNodeNum();
	DisplayProcess(0,"提取角钢工艺卡");
	ARRAY_LIST<f3dPoint> vertex_list;
	for(ACAD_TEXT *pBlockRef=hashAngleCardBlockRefByEntId.GetFirst();pBlockRef;pBlockRef=hashAngleCardBlockRefByEntId.GetNext())
	{
		i++;
		DisplayProcess((100*i)/nSumCount,"提取角钢工艺卡");
		nNewJgCount++;
#ifdef _WIN64
		CAngleProcessInfo* pJgInfo=BPSModel.AppendJgInfo(pBlockRef->entId.asOldId());
#else
		CAngleProcessInfo* pJgInfo=BPSModel.AppendJgInfo((Adesk::UInt32)pBlockRef->entId.handle());
#endif
		pJgInfo->keyId=pBlockRef->entId;
		pJgInfo->m_bInBlockRef=TRUE;
		pJgInfo->sign_pt=pBlockRef->text_pos;	//角钢的识别标记点（打碎以件号标注点进行标记，图框块以图框原点进行标记）
		pJgInfo->InitOrig();					//初始化此角钢工艺卡与工艺卡模板的对应关系
		pJgInfo->m_bUpdatePng=TRUE;
		vertex_list.Empty();
		vertex_list.append(CIdentifyManager::GetLeftBtmPt()+pJgInfo->orig_pt);
		vertex_list.append(CIdentifyManager::GetLeftTopPt()+pJgInfo->orig_pt);
		vertex_list.append(CIdentifyManager::GetRightTopPt()+pJgInfo->orig_pt);
		vertex_list.append(CIdentifyManager::GetRightBtmPt()+pJgInfo->orig_pt);
		pJgInfo->CreateRgn(vertex_list);
		//
		for(ACAD_TEXT *pText=hashTextByEntId.GetFirst();pText;pText=hashTextByEntId.GetNext())
		{
			if(pText->pAngleInfo!=NULL||!pJgInfo->PtInAngleRgn(pText->text_pos))
				continue;
			pText->pAngleInfo=pJgInfo;
			pJgInfo->InitAngleInfo(pText->text_pos,pText->sText);
		}
		//第二次进行提取时,同一构件工艺卡图框发生变化，需删除之前的构件 wht 17-12-18
		CAngleProcessInfo **ppOldJgInfo=hashJgPtrByPartLabel.GetValue(pJgInfo->m_sPartNo);
		if(ppOldJgInfo!=NULL&&pJgInfo->keyId!=(*ppOldJgInfo)->keyId)
#ifdef _WIN64
			BPSModel.DeleteJgInfo((*ppOldJgInfo)->keyId.asOldId());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
#else
			BPSModel.DeleteJgInfo((Adesk::UInt32)(*ppOldJgInfo)->keyId.handle());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
#endif
	}
	DisplayProcess(100,"提取角钢工艺卡");
	//2.2 如果角钢数量与件号数量不一致，再根据件号识别角钢
	i=0;
	DisplayProcess(0,"提取角钢工艺卡");
	nSumCount=partNoTextList.GetNodeNum();
	for(ACAD_TEXT *pText=partNoTextList.GetFirst();pText;pText=partNoTextList.GetNext())
	{
		i++;
		DisplayProcess((100*i)/nSumCount,"提取角钢工艺卡");
		if(pText->pAngleInfo!=NULL)
			continue;
		CAngleProcessInfo **ppOldJgInfo=hashJgPtrByPartLabel.GetValue(pText->sText);
		nNewJgCount++;
#ifdef _WIN64
		if(ppOldJgInfo!=NULL&&pText->entId!=(*ppOldJgInfo)->keyId)
			BPSModel.DeleteJgInfo((*ppOldJgInfo)->keyId.asOldId());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
		CAngleProcessInfo* pJgInfo=BPSModel.AppendJgInfo(pText->entId.asOldId());
#else
		if(ppOldJgInfo!=NULL&&pText->entId!=(*ppOldJgInfo)->keyId)
			BPSModel.DeleteJgInfo((Adesk::UInt32)(*ppOldJgInfo)->keyId.handle());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
		CAngleProcessInfo* pJgInfo=BPSModel.AppendJgInfo((Adesk::UInt32)pText->entId.handle());
#endif
		pJgInfo->keyId=pText->entId;
		pJgInfo->m_bInBlockRef=FALSE;
		pJgInfo->sign_pt=pText->text_pos;	//角钢的识别标记点（打碎以件号标注点进行标记，图框块以图框原点进行标记）
		pJgInfo->InitOrig();				//初始化此角钢工艺卡与工艺卡模板的对应关系
		pJgInfo->m_bUpdatePng=TRUE;
		//根据PolyLine初始化工艺卡区域
		double fMaxWidth=0;
		ARRAY_LIST<f3dPoint> vertexList;
		for(ACAD_RECT *pRect=hashRectByEntId.GetFirst();pRect;pRect=hashRectByEntId.GetNext())
		{
			if(pRect->region.PtInRgn(pJgInfo->sign_pt)==1&&pRect->fWidth>fMaxWidth)
			{
				pRect->pAngleInfo=pJgInfo;
				fMaxWidth=pRect->fWidth;
				vertexList.Empty();
				for(int i=0;i<pRect->region.GetVertexCount();i++)
					vertexList.append(pRect->region.GetVertexAt(i));
				pJgInfo->CreateRgn(vertexList);
				break;
			}
		}
		//初始化角钢信息
		for(ACAD_TEXT *pText=hashTextByEntId.GetFirst();pText;pText=hashTextByEntId.GetNext())
		{
			if(pText->pAngleInfo!=NULL||!pJgInfo->PtInAngleRgn(pText->text_pos))
				continue;
			pText->pAngleInfo=pJgInfo;
			pJgInfo->InitAngleInfo(pText->text_pos,pText->sText);
		}
	}
	DisplayProcess(100,"提取角钢工艺卡");
	if(nNewJgCount>0)
		BPSModel.m_iRetrieveBatchNo++;
	if(BPSModel.GetJgNum()<=0)
	{
#if _ARX_2007
		acutPrintf(L"\n读取角钢DWG文件失败!");
#else
		acutPrintf("\n读取角钢DWG文件失败!");
#endif
	}
	BPSModel.CorrectAngles();
	return TRUE;
}
#include "GlobalFunc.h"
CXhChar500 GetTempFileFolderPath(bool bEmpty)
{	//获取临时文件路径
	CXhChar500 sSysPath;
	GetSysPath(sSysPath);
	CXhChar500 sTemp("%s\\PngTemp\\",(char*)sSysPath);
	//确保路径存在
	CFileFind fileFind;
	BOOL bContinue=fileFind.FindFile(CXhChar500("%s*.*",(char*)sTemp));
	if(!bContinue)
		MakeDirectory(sTemp);
	else
	{
		while(bContinue)
		{
			bContinue=fileFind.FindNextFile();

			CString ss1 = fileFind.GetFileName();
			CString ss2 = fileFind.GetFilePath();
			if(fileFind.IsDots()||fileFind.IsDirectory())
				continue;
			DeleteFile(fileFind.GetFilePath());
		}
	}
	//删除临时文件夹中的所有内容
	return sTemp;
}

bool PrintProcessCardToPNG(bool bEmptyPngFiles)
{	//判断打印机是否合理
	AcApLayoutManager *pLayMan = NULL;
	pLayMan = (AcApLayoutManager *) acdbHostApplicationServices()->layoutManager();
	AcDbLayout *pLayout = pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE),TRUE);
	if(pLayout==NULL)
		return false;
	pLayout->close();
	AcDbPlotSettings* pPlotSetting = (AcDbPlotSettings*)pLayout;
	CXhChar500 sPlotCfgName;
	Acad::ErrorStatus retCode;
#ifdef _ARX_2007
	const ACHAR* sValue;
	retCode=pPlotSetting->getPlotCfgName(sValue);
	sPlotCfgName.Copy((char*)_bstr_t(sValue));
#else
	char *sValue;
	retCode=pPlotSetting->getPlotCfgName(sValue);
	sPlotCfgName.Copy(sValue);
#endif
	if(retCode!=Acad::eOk)//||stricmp(sPlotCfgName,"PublishToWeb PNG.pc3")!=0) 
	{//获取输出设备名称
#ifdef _ARX_2007
		acutPrintf(L"\n当前激活打印设备不可用,请优先进行页面设置!");
#else
		acutPrintf("\n当前激活打印设备不可用,请优先进行页面设置!");
#endif
		return false;
	}
	//进行批量打印
	CXhChar500 sTempPath=GetTempFileFolderPath(bEmptyPngFiles),sFilePath;
	SetCurrentDirectory(sTempPath);	//设置当前工作路径
	CFileFind fileFind;
	for(CAngleProcessInfo *pInfo=BPSModel.EnumFirstJg();pInfo;pInfo=BPSModel.EnumNextJg())
	{
		if(!pInfo->m_bUpdatePng)
			continue;
		SCOPE_STRU scope=pInfo->GetCADEntScope();
		ads_point L_T,R_B;
		L_T[0]=scope.fMinX;
		L_T[1]=scope.fMaxY;
		L_T[2]=0;
		R_B[0]=scope.fMaxX;
		R_B[1]=scope.fMinY;
		R_B[2]=0;
		CString sLabel=pInfo->m_sPartNo;
		sLabel.Remove(' ');
		sFilePath.Printf("%s.png",sLabel);
		int nRetCode=0;
		pInfo->m_sCardPngFile.Copy(sFilePath);
		pInfo->m_sCardPngFilePath.Printf("%s\\%s",(char*)sTempPath,(char*)sFilePath);
		pInfo->m_bUpdatePng=FALSE;
#ifdef _ARX_2007
		acedCommand(RTSTR,L"CMDECHO",RTLONG,0,RTNONE);	//设置命令行是否产生回显
		nRetCode=acedCommand(RTSTR,L"-plot",RTSTR,L"y",	//是否需要详细打印配置[是(Y)/否(N)]
			RTSTR,L"",							//布局名
			RTSTR,L"PublishToWeb PNG.pc3",		//输出设备的名称
			RTSTR,L"Sun Hi-Res (1600.00 x 1280.00 像素)",//图纸尺寸:<A4>
			RTSTR,L"L",							//图形方向:<横向|纵向>
			RTSTR,L"N",							//是否反向打印
			RTSTR,L"w",							//打印区域:<指定窗口>
			RTPOINT,L_T,						//左上角
			RTPOINT,R_B,						//右下角
			RTSTR,L"",							//打印比例:<布满>
			RTSTR,L"C",							//打印偏移：<居中>
			RTSTR,L"N",							//是否按样式打印
			RTSTR,L".",							//打印样式名称
			RTSTR,L"Y",							//打印线宽
			RTSTR,L"A",							//着色打印设置
			RTSTR,(ACHAR*)_bstr_t(sFilePath),	//打印到文件	//此处设置完成路径时会失败，需要调用SetCurrentDirectory之后，此处直接输入文件名 wht 17-10-23
			RTSTR,L"N",							//是否保存对页面设置的修改
			RTSTR,L"Y",							//是否继续打印
			RTNONE);
#else
		acedCommand(RTSTR,"CMDECHO",RTLONG,0,RTNONE);		//设置命令行是否产生回显
		nRetCode=acedCommand(RTSTR,"-plot",RTSTR,"y",	//是否需要详细打印配置[是(Y)/否(N)]
			RTSTR,"",							//布局名
			RTSTR,"PublishToWeb PNG.pc3",		//输出设备的名称
			RTSTR,"Sun Hi-Res (1600.00 x 1280.00 像素)",//图纸尺寸:<A4>
			RTSTR,"L",							//图形方向:<横向|纵向>
			RTSTR,"N",							//是否反向打印
			RTSTR,"w",							//打印区域:<指定窗口>
			RTPOINT,L_T,						//左上角
			RTPOINT,R_B,						//右下角
			RTSTR,"",							//打印比例:<布满>
			RTSTR,"C",							//打印偏移：<居中>
			RTSTR,"N",							//是否按样式打印
			RTSTR,".",							//打印样式名称
			RTSTR,"Y",							//打印线宽
			RTSTR,"A",							//着色打印设置
			RTSTR,(char*)sFilePath,					//打印到文件
			RTSTR,"N",							//是否保存对页面设置的修改
			RTSTR,"Y",							//是否继续打印
			RTNONE);
#endif
	}
	return true;
}

BOOL WriteOrReadInfoFromReg(LPCTSTR lpszSection, LPCTSTR lpszEntry,BOOL bWrite,char *sValue)	
{
	char sSubKey[MAX_PATH]="";
	DWORD dwDataType,dwLength=MAX_PATH;
	sprintf(sSubKey,"Software\\Xerofox\\BPS\\%s",lpszSection);
	HKEY hKey;
	int nRetCode=RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_READ|KEY_WRITE,&hKey);
	if(hKey==NULL&&bWrite)
	{
		DWORD dw;
		HKEY hSoftwareKey;
		RegOpenKeyEx(HKEY_CURRENT_USER,"Software",0,KEY_WRITE,&hSoftwareKey);
		if(hSoftwareKey!=NULL)
		{
			RegCreateKeyEx(hSoftwareKey, CXhChar100("Xerofox\\BPS\\%s",lpszSection), 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL, &hKey, &dw);
			RegCloseKey(hSoftwareKey);
		}
	}
	BOOL bRetCode = FALSE;
	if(hKey!=NULL)
	{
		if(bWrite)
		{
			dwLength=strlen(sValue);
			nRetCode=RegSetValueEx(hKey,lpszEntry,NULL,REG_SZ,(BYTE*)&sValue[0],dwLength);
			bRetCode=(nRetCode==ERROR_SUCCESS);
		}
		else 
		{
			nRetCode=RegQueryValueEx(hKey,lpszEntry,NULL,&dwDataType,(BYTE*)&sValue[0],&dwLength); 
			bRetCode=(nRetCode==ERROR_SUCCESS);
		}
		RegCloseKey(hKey);
	}
	return bRetCode;
}

#ifdef __CONNECT_REMOTE_SERVER_
#include "TMS.h"
BOOL LogonTMSServer()
{
	CReConnServerDlg logdlg;
	logdlg.m_bInputPassword=TRUE;
	CXhChar200 sServerName,sUserName,sPassword;
	WriteOrReadInfoFromReg("Port","ServerName",FALSE,sServerName);
	WriteOrReadInfoFromReg("Port","UserName",FALSE,sUserName);
	WriteOrReadInfoFromReg("Port","Password",FALSE,sPassword);
	logdlg.m_sServerName=sServerName;
	logdlg.m_sUserName=sUserName;
	logdlg.m_sPassword=sPassword;
	CAcModuleResourceOverride resOverride;
	if(logdlg.DoModal()==IDOK)
	{
		WriteOrReadInfoFromReg("Port","ServerName",TRUE,logdlg.m_sServerName.GetBuffer());
		WriteOrReadInfoFromReg("Port","UserName",TRUE,logdlg.m_sUserName.GetBuffer());
		WriteOrReadInfoFromReg("Port","Password",TRUE,logdlg.m_sPassword.GetBuffer());
		if(logdlg.m_sServerName.GetLength()>0)
			TMS.SetServerUrl(logdlg.m_sServerName.Trim());
		BOOL bRetCode=TMS.LoginUser(logdlg.m_sUserName,logdlg.m_sPassword);
		return bRetCode;
	}
	else 
		return FALSE;
}

void AppendOrUpdateProcessCards()
{
	if(TMS.m_pServer==NULL)
	{
		if(!LogonTMSServer())
			AfxMessageBox("登录服务器失败!");
	}
	if(InitBpsModel(FALSE))
		PrintProcessCardToPNG(false);
	CUploadProcessCardDlg dlg;
	CAcModuleResourceOverride resOverride;
	dlg.DoModal();
}
void RetrievedProcessCards()
{
	if(TMS.m_pServer==NULL)
	{
		if(!LogonTMSServer())
			AfxMessageBox("登录服务器失败!");
	}
	//1.从DWG中提取工艺卡信息
	if(InitBpsModel(TRUE))
	{	//2.将工艺卡信息保存为图片文件
		PrintProcessCardToPNG(true);
	}
	CUploadProcessCardDlg dlg;
	CAcModuleResourceOverride resOverride;
	dlg.DoModal();
}
#endif

void BatchPrintPartProcessCards()
{
	//1.从DWG中提取角钢工艺卡
	InitBpsModel(TRUE);
	//2.进行批量打印
	OptimalSortSet();	
}

//优化排序设置
void OptimalSortSet()
{
	if(!ValidateLocalizeFeature(FEATURE::BATCH_INTELLI_PRINT_PROCESSCARD))
		return;
	COptimalSortDlg dlg;
	CAcModuleResourceOverride resOverride;
	if(dlg.DoModal()==IDOK)
	{
		//判断打印机是否合理
		AcApLayoutManager *pLayMan = NULL;
		pLayMan = (AcApLayoutManager *) acdbHostApplicationServices()->layoutManager();
		AcDbLayout *pLayout = pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE),TRUE);
		if(pLayout==NULL)
			return;
		pLayout->close();
		AcDbPlotSettings* pPlotSetting = (AcDbPlotSettings*)pLayout;
		CXhChar500 sPlotCfgName;
		Acad::ErrorStatus retCode;
#ifdef _ARX_2007
		const ACHAR* sValue;
		retCode=pPlotSetting->getPlotCfgName(sValue);
		sPlotCfgName.Copy((char*)_bstr_t(sValue));
#else
		char *sValue;
		retCode=pPlotSetting->getPlotCfgName(sValue);
		sPlotCfgName.Copy(sValue);
#endif
		if(retCode!=Acad::eOk||stricmp(sPlotCfgName,"无")==0) 
		{//获取输出设备名称
#ifdef _ARX_2007
			acutPrintf(L"\n当前激活打印设备不可用,请优先进行页面设置!");
#else
			acutPrintf("\n当前激活打印设备不可用,请优先进行页面设置!");
#endif
			return;
		}
		//进行批量打印
		for(SCOPE_STRU* pScopy=dlg.m_xPrintScopyList.GetFirst();pScopy;pScopy=dlg.m_xPrintScopyList.GetNext())
		{
			ads_point L_T,R_B;
			L_T[0]=pScopy->fMinX;
			L_T[1]=pScopy->fMaxY;
			L_T[2]=0;
			R_B[0]=pScopy->fMaxX;
			R_B[1]=pScopy->fMinY;
			R_B[2]=0;
#ifdef _ARX_2007
			acedCommand(RTSTR,L"CMDECHO",RTLONG,0,RTNONE);		//设置命令行是否产生回显
			acedCommand(RTSTR,L"-plot",RTSTR,L"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR,L"",							//布局名
				RTSTR,L"",							//输出设备的名称
				RTSTR,L"",							//图纸尺寸:<A4>
				RTSTR,L"",							//图纸单位:<毫米>
				RTSTR,L"",							//图形方向:<横向|纵向>
				RTSTR,L"",							//是否反向打印
				RTSTR,L"w",							//打印区域:<指定窗口>
				RTPOINT,L_T,RTPOINT,R_B,			//左上角、右下角
				RTSTR,L"",							//打印比例:<布满>
				RTSTR,L"",							//打印偏移：<居中>
				RTSTR,L"",							//是否按样式打印
				RTSTR,L"",							//打印样式名称
				RTSTR,L"",							//打印线宽
				RTSTR,L"",							//着色打印设置
				RTSTR,L"",							//打印到文件
				RTSTR,L"",							//是否保存页面设置更改
				RTSTR,L"",							//是否继续打印
				RTNONE);
#else
			acedCommand(RTSTR,"CMDECHO",RTLONG,0,RTNONE);		//设置命令行是否产生回显
			acedCommand(RTSTR,"-plot",RTSTR,"y",	//是否需要详细打印配置[是(Y)/否(N)]
				RTSTR,"",							//布局名
				RTSTR,"",							//输出设备的名称
				RTSTR,"",							//图纸尺寸:<A4>
				RTSTR,"",							//图纸单位:<毫米>
				RTSTR,"",							//图形方向:<横向|纵向>
				RTSTR,"",							//是否反向打印
				RTSTR,"w",							//打印区域:<指定窗口>
				RTPOINT,L_T,RTPOINT,R_B,			//左上角、右下角
				RTSTR,"",							//打印比例:<布满>
				RTSTR,"",							//打印偏移：<居中>
				RTSTR,"",							//是否按样式打印
				RTSTR,"",							//打印样式名称
				RTSTR,"",							//打印线宽
				RTSTR,"",							//着色打印设置
				RTSTR,"",							//打印到文件
				RTSTR,"",							//是否保存页面设置更改
				RTSTR,"",							//是否继续打印
				RTNONE);
#endif
		}
	}
}
#endif