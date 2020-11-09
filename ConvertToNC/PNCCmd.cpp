#include "StdAfx.h"
#include "PNCCmd.h"
#include "CadToolFunc.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "PNCSysSettingDlg.h"
#include "InputMKRectDlg.h"
#include "PncModel.h"
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
//������ȡ�����Ϣ,��ȡ�ɹ���֧���Ű�
//SmartExtractPlate
//////////////////////////////////////////////////////////////////////////
void SmartExtractPlate()
{
	//��ȡ��ǰ�����ļ�
	CString file_name;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc)
		file_name = pDoc->fileName();
	if (file_name.GetLength() <= 0 || file_name.CompareNoCase("Drawing1.dwg") == 0)
	{
		AfxMessageBox("�����Ч���ļ�!");
		return;
	}
	//��ȡ��ǰ�ļ��ĸְ���Ϣ
	CPNCModel::m_bSendCommand = FALSE;
	model.Empty();
	model.m_sCurWorkFile = file_name;
	SmartExtractPlate(&model, TRUE);
}

void SmartExtractPlate(CPNCModel *pModel, BOOL bSupportSelectEnts/*=FALSE*/,CHashSet<AcDbObjectId> *pObjIdSet/*=NULL*/)
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
	{	//�û�����ϵ����������ϵ��һ�£�ִ��ucs����޶�����ϵ wht 20-04-24
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
	if (bSupportSelectEnts)
	{	//�����ֶ���ѡ
		//��������ȡ����ʱ����Ҫѡ�����е�ͼ�η�����ڵĹ���ʹ��
		if (g_pncSysPara.m_ciRecogMode != CPNCSysPara::FILTER_BY_PIXEL)
			SelCadEntSet(pModel->m_xAllEntIdSet, TRUE);
		if (pObjIdSet)
		{
			for (AcDbObjectId entId = pObjIdSet->GetFirst(); entId; entId = pObjIdSet->GetNext())
				selectedEntList.SetValue(entId.asOldId(), entId);
		}
		if (selectedEntList.GetNodeNum() <= 0)
		{
			if (!SelCadEntSet(selectedEntList))
				return;
		}
	}
	else
	{	//��������ͼԪ
		SelCadEntSet(pModel->m_xAllEntIdSet, TRUE);
		for (AcDbObjectId entId = pModel->m_xAllEntIdSet.GetFirst(); entId; entId = pModel->m_xAllEntIdSet.GetNext())
			selectedEntList.SetValue(entId.asOldId(), entId);
	}
	//�ӿ�ѡ��Ϣ����ȡ�иְ�ı�ʶ��ͳ�Ƹְ弯��
	CHashSet<AcDbObjectId> textIdHash;
	AcDbEntity *pEnt = NULL;
	int index = 1,nNum= selectedEntList.GetNodeNum();
	DisplayCadProgress(0, "����ͼֽ���ֱ�ע,ʶ��ְ������Ϣ.....");
	for (AcDbObjectId entId=selectedEntList.GetFirst(); entId.isValid();entId=selectedEntList.GetNext(), index++)
	{
		DisplayCadProgress(int(100 * index / nNum));
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
				logerr.Log("�ְ���Ϣ{%s}�������õ�����ʶ����򣬵�ʶ��ʧ������ϵ�ź��ͷ�!", (char*)sText);
				continue;
			}
			else
				dim_pos = GetCadTextDimPos(pEnt, &dim_vec);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//�ӿ��н����ְ���Ϣ
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
				logerr.Log("ͨ��ͼ��ʶ��ְ������Ϣ,ʶ��ʧ������ϵ�ź��ͷ�!");
				continue;
			}
		}
		CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//������ͬ���������ı���Ӧ��ʵ�岻��ͬ��ʾ�����ظ� wht 19-07-22
			logerr.Log("����{%s}���ظ���ȷ��!", (char*)sPartNo);
			pExistPlate->m_dwErrorType |= CPlateProcessInfo::ERROR_REPEAT_PART_LABEL;
			continue;
		}
		//
		CPlateProcessInfo* pPlateProcess = pModel->AppendPlate(sPartNo);
		pPlateProcess->m_pBelongModel = pModel;
		pPlateProcess->m_bNeedExtract = TRUE;	//ѡ��CADʵ�������ǰ����ʱ����λ��Ҫ��ȡ
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
			if (pModel->m_xPrjInfo.m_sTaStampNo.GetLength() <= 0 && baseInfo.m_sTaStampNo.GetLength() > 0)
				pModel->m_xPrjInfo.m_sTaStampNo.Copy(baseInfo.m_sTaStampNo);
			if (pModel->m_xPrjInfo.m_sTaType.GetLength() <= 0 && baseInfo.m_sTaType.GetLength() > 0)
				pModel->m_xPrjInfo.m_sTaType.Copy(baseInfo.m_sTaType);
			if (pModel->m_xPrjInfo.m_sPrjCode.GetLength() <= 0 && baseInfo.m_sPrjCode.GetLength() > 0)
				pModel->m_xPrjInfo.m_sPrjCode.Copy(baseInfo.m_sPrjCode);
		}
		else
		{
			pPlateProcess->partNoId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = 'S';
		}
	}
	DisplayCadProgress(100);
	if (selectedEntList.GetNodeNum() <= 0)
	{
		AfxMessageBox("δѡ����ȡ���ݣ��޷�ִ����ȡ������");
		return;
	}
	else if(pModel->GetPlateNum()<=0)
	{
		if (AfxMessageBox("���ļ�����������ʶ�����ã��Ƿ��������ʶ�����ã�", MB_YESNO) == IDYES)
		{
			CAcModuleResourceOverride resOverride;
			CPNCSysSettingDlg dlg;
			dlg.m_iSelTabGroup = 1;
			dlg.DoModal();
		}
		return;
	}
	//��ȡ�ְ����˨�ף���˨�顢Բ�����ǡ����Ρ���Բ��
	pModel->ExtractPlateBoltEnts(selectedEntList);
	//��ȡ�ְ��������
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		pModel->ExtractPlateProfileEx(selectedEntList);
	else
		pModel->ExtractPlateProfile(selectedEntList);
#ifndef __UBOM_ONLY_
	//����ְ�һ���ŵ����
	pModel->MergeManyPartNo();
	//���������պ�������¸ְ�Ļ�����Ϣ+��˨��Ϣ+��������Ϣ
	int nSum = 0;
	nNum = pModel->GetPlateNum();
	DisplayCadProgress(0,"�޶��ְ���Ϣ<����+��˨+����>.....");
	CHashStrList<CXhChar16> hashPartLabelByLabel;
	for (CPlateProcessInfo* pPlateProcess = pModel->EnumFirstPlate(FALSE); pPlateProcess; pPlateProcess = pModel->EnumNextPlate(FALSE))
		hashPartLabelByLabel.SetValue(pPlateProcess->GetPartNo(), pPlateProcess->GetPartNo());
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE),nSum++)
	{
		DisplayCadProgress(int(100 * nSum / nNum));
		pPlateProcess->ExtractPlateRelaEnts();
		pPlateProcess->CheckProfileEdge();
		pPlateProcess->UpdateBoltHoles(&hashPartLabelByLabel);
		if(!pPlateProcess->UpdatePlateInfo())
			logerr.Log("����%s��ѡ���˴���ı߽�,������ѡ��.(λ�ã�%s)",(char*)pPlateProcess->GetPartNo(),(char*)CXhChar50(pPlateProcess->dim_pos));
		if (pPlateProcess->IsValid())
			pPlateProcess->InitPPiInfo();
		//����DrawPlate֮������޸�NeedExtract״̬����������ȡÿ�ζ��������иְ������ wht 20-10-11
		//�����ȡ��ͳһ���øְ���ȡ״̬ΪFALSE wht 19-06-17
		//pPlateProcess->m_bNeedExtract = FALSE;
	}
	DisplayCadProgress(100);
	//����ȡ�ĸְ���Ϣ�����������ļ���
	if (g_pncSysPara.m_iPPiMode == 0)
		pModel->SplitManyPartNo();
	if (CPlateProcessInfo::m_bCreatePPIFile)
	{
		CString file_path;
		GetCurWorkPath(file_path);
		pModel->CreatePlatePPiFile(file_path);
		//д�������������ļ� wht 19-01-12
		if (pModel->m_xPrjInfo.m_sTaType.GetLength() > 0)
		{
			CString cfg_path = file_path + "config.ini";
			cfg_path.Format("%sconfig.ini", file_path);
			pModel->WritePrjTowerInfoToCfgFile(cfg_path);
		}
	}
	//������ȡ�ĸְ�����--֧���Ű�
	pModel->DrawPlates(!CPlateProcessInfo::m_bCreatePPIFile);
	//�������֮�󽫸ְ�����Ϊ����ȡ��֧��ֻ��������ȡ�ĸְ� wht 20-10-11
	for (CPlateProcessInfo* pPlateProcess = pModel->EnumFirstPlate(TRUE); pPlateProcess; pPlateProcess = pModel->EnumNextPlate(TRUE), nSum++)
	{	//�����ȡ��ͳһ���øְ���ȡ״̬ΪFALSE wht 19-06-17
		pPlateProcess->m_bNeedExtract = FALSE;
	}
	//ͨ���˵���ȡ�ĸְ�����¹����б�
	if (CPNCModel::m_bSendCommand == FALSE)
	{
		CPartListDlg *pPartListDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
		if (pPartListDlg != NULL && pPartListDlg->GetSafeHwnd()!=NULL)
		{
			pPartListDlg->RefreshCtrlState();
			pPartListDlg->RelayoutWnd();
			pPartListDlg->UpdatePartList();
		}
	}
#else
	//UBOMֻ��Ҫ���¸ְ�Ļ�����Ϣ
	for (CPlateProcessInfo* pPlateProcess = pModel->EnumFirstPlate(TRUE); pPlateProcess; pPlateProcess = pModel->EnumNextPlate(TRUE))
	{	
		if(!pPlateProcess->IsValid())
			pPlateProcess->CreateRgnByText();
	}
	//
	pModel->MergeManyPartNo();
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt==NULL)
			continue;
		if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//������纸ͼ��ĸְ�
			AcDbBlockReference* pReference = (AcDbBlockReference*)pEnt;
			AcDbObjectId blockId = pReference->blockTableRecord();
			AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
			acdbOpenObject(pTempBlockTableRecord, blockId, AcDb::kForRead);
			if (pTempBlockTableRecord == NULL)
				continue;
			pTempBlockTableRecord->close();
			CXhChar50 sName;
#ifdef _ARX_2007
			ACHAR* sValue = new ACHAR[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy((char*)_bstr_t(sValue));
			delete[] sValue;
#else
			char *sValue = new char[50];
			pTempBlockTableRecord->getName(sValue);
			sName.Copy(sValue);
			delete[] sValue;
#endif
			if(!g_pncSysPara.IsHasPlateWeldTag(sName))
				continue;
			AcGePoint3d pos = pReference->position();
			CPlateProcessInfo* pPlateInfo = pModel->GetPlateInfo(GEPOINT(pos.x,pos.y));
			if (pPlateInfo == NULL)
				continue;
			pPlateInfo->xBomPlate.bWeldPart = TRUE;
		}
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
		//�������������ڻ�����Ϣ�����磺141#����
		CXhChar100 sText = GetCadTextContent(pEnt);
		if(g_pncSysPara.IsHasPlateBendTag(sText))
			pPlateInfo->xBomPlate.siZhiWan = 1;
		if(g_pncSysPara.IsHasPlateWeldTag(sText))
			pPlateInfo->xBomPlate.bWeldPart = TRUE;
	}
	pModel->SplitManyPartNo();
	for (CPlateProcessInfo* pPlateInfo = pModel->EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = pModel->EnumNextPlate(FALSE))
	{
		pPlateInfo->xBomPlate.sPartNo = pPlateInfo->GetPartNo();
		pPlateInfo->xBomPlate.cMaterial = pPlateInfo->xPlate.cMaterial;
		pPlateInfo->xBomPlate.cQualityLevel = pPlateInfo->xPlate.cQuality;
		pPlateInfo->xBomPlate.thick = pPlateInfo->xPlate.m_fThick;
		pPlateInfo->xBomPlate.nSumPart = pPlateInfo->xPlate.m_nProcessNum;	//�ӹ���
		pPlateInfo->xBomPlate.AddPart(pPlateInfo->xPlate.m_nSingleNum);		//������
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("�ְ�%s��Ϣ��ȡʧ��!", (char*)pPlateInfo->xBomPlate.sPartNo);
		else
			pPlateInfo->xBomPlate.sSpec.Printf("-%.f", pPlateInfo->xBomPlate.thick);
	}
#endif
}
#ifndef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//�༭�ְ���Ϣ
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
	//1��д����ܹ�֤����Ϣ
	char APP_PATH[MAX_PATH];
	GetAppPath(APP_PATH);
	buffer.WriteByte(cProductType);
	buffer.WriteDword(version[0]);
	buffer.WriteDword(version[1]);
	buffer.WriteString(APP_PATH,MAX_PATH);
	//2��д��PPE����ģʽ��Ϣ
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
		file_buffer.WriteString(sFilePath);	//д��PPI�ļ�·��
		buffer.WriteDword(file_buffer.GetLength());
		buffer.Write(file_buffer.GetBufferPtr(),file_buffer.GetLength());
	}
	//4���������ܵ���д������
	return buffer.WriteToPipe(hPipeWrite,1024);
}
void SendPartEditor()
{
	CLogErrorLife logErrLife;
	//1��������һ���ܵ�: ���ڷ���������ͻ��˷�������
	SECURITY_ATTRIBUTES attrib;
	attrib.nLength = sizeof( SECURITY_ATTRIBUTES );
	attrib.bInheritHandle= true;
	attrib.lpSecurityDescriptor = NULL;
	HANDLE hPipeClientRead=NULL, hPipeSrcWrite=NULL;
	if( !CreatePipe( &hPipeClientRead, &hPipeSrcWrite, &attrib, 0 ) )
	{
		logerr.Log("���������ܵ�ʧ��!GetLastError= %d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcWriteDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcWrite, GetCurrentProcess(), &hPipeSrcWriteDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("���ƾ��ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcWrite );
	//2�������ڶ����ܵ������ڿͻ�����������˷�������
	HANDLE hPipeClientWrite=NULL, hPipeSrcRead=NULL;
	if( !CreatePipe( &hPipeSrcRead, &hPipeClientWrite, &attrib, 0) )
	{
		logerr.Log("�����ڶ��������ܵ�ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcReadDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcRead, GetCurrentProcess(), &hPipeSrcReadDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("���Ƶڶ������ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcRead );
	//3�������ӽ���,
	if( !CreatePartEditProcess( hPipeClientRead, hPipeClientWrite ) )
	{
		logerr.Log("�����ӽ���ʧ��\n" );
		return;
	}
	//4���������ݲ���---֧�ִ��Ͷ������
	if( !WriteToClient(hPipeSrcWriteDup))
	{
		logerr.Log("���ݴ���ʧ��");
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
//��ʾ�ְ���Ϣ
//ShowPartList
//////////////////////////////////////////////////////////////////////////
void ShowPartList()
{	
	g_xPNCDockBarManager.DisplayPartListDockBar(CPartListDlg::m_nDlgWidth);
	//���¹����б�
	CPartListDlg *pPartListDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
}
//////////////////////////////////////////////////////////////////////////
//���Ƹְ�
//DrawPlates
//////////////////////////////////////////////////////////////////////////
void DrawPlates()
{
	int draw_type = 0, idRet = 0, group_type = 0;
	BOOL bDrawByVert = TRUE;
#ifdef _ARX_2007
	ACHAR result[20] = { 0 };
	idRet = acedGetString(NULL, L"\n�������ģʽ[�Ա�(1)/�Ű�(2)/����(3)/ɸѡ(4)]]<1>:", result);
	if (idRet == RTNORM)
		draw_type = (wcslen(result) > 0) ? atoi(_bstr_t(result)) : 1;
	if (draw_type == 1)
	{
		if (acedGetString(NULL, L"\n��������Ϊ����[��(Y)/��(N)]<Y>:", result) == RTNORM)
		{
			if (wcslen(result) <= 0 || wcsicmp(result, L"Y") == 0 || wcsicmp(result, L"y") == 0)
				bDrawByVert = TRUE;
			else
				bDrawByVert = FALSE;
		}
	}
	else if (draw_type == 4)
	{
		if (acedGetString(NULL, L"\nɸѡ��ʽ[�κ�(1)/����&���(2)/����(3)/���(4)]<1>:", result) == RTNORM)
		{
			if (wcslen(result) <= 0 || result[0] == '1')
				group_type = 1;
			else
				group_type = result[0] - '0';
		}
	}
#else
	char result[20] = "";
	idRet = acedGetString(NULL, _T("\n�������ģʽ[�Ա�(1)/�Ű�(2)/����(3)/ɸѡ(4)]<1>:"), result);
	if (idRet == RTNORM)
		draw_type = (strlen(result) > 0) ? atoi(result) : 1;
	if (draw_type == 1)
	{
		if (acedGetString(NULL, _T("\n��������Ϊ����[��(Y)/��(N)]<Y>:"), result) == RTNORM)
		{
			if (strlen(result) <= 0 || stricmp(result, "Y") == 0 || stricmp(result, "y") == 0)
				bDrawByVert = TRUE;
			else
				bDrawByVert = FALSE;
		}
	}
	else if (draw_type == 4)
	{
		if (acedGetString(NULL, _T("\nɸѡ��ʽ[�κ�(1)/����&���(2)/����(3)/���(4)]<1>:"), result) == RTNORM)
		{
			if (strlen(result) <= 0 || result[0] == '1')
				group_type = 1;
			else
				group_type = result[0] - '0';
		}
	}
#endif
	if (draw_type == 0)
	{
#ifdef _ARX_2007
		acutPrintf(L"\n��Ч������!");
#else
		acutPrintf(_T("\n��Ч������!"));
#endif
		return;
	}
	//����
	g_pncSysPara.m_ciLayoutMode = draw_type;
	if (draw_type == 1)
		g_pncSysPara.m_ciArrangeType = bDrawByVert;
	else if (draw_type == 4)
		g_pncSysPara.m_ciGroupType = group_type;
	model.DrawPlates();
	//���¹����б�
	CPartListDlg *pPartListDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL && pPartListDlg->GetSafeHwnd() != NULL)
	{
		pPartListDlg->RefreshCtrlState();
		pPartListDlg->RelayoutWnd();
		pPartListDlg->UpdatePartList();
	}
}
//////////////////////////////////////////////////////////////////////////
//ʵʱ�����ӡ��
//InsertMKRect		
//////////////////////////////////////////////////////////////////////////
void InsertMKRect()
{
	CAcModuleResourceOverride resOverride;
	CXhChar16 blkname("MK");
	AcDbObjectId blockId=SearchBlock(blkname);
	if(blockId.isNull())
	{	//�½�����¼ָ�룬���ɸ�ӡ���ͼԪ
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
		AcDbBlockTableRecord *pTempBlockTableRecord=new AcDbBlockTableRecord();//�������¼ָ��
#ifdef _ARX_2007
		pTempBlockTableRecord->setName((ACHAR*)_bstr_t(CXhChar16("MK")));
#else
		pTempBlockTableRecord->setName(CXhChar16("MK"));
#endif
		pTempBlockTable->add(blockId,pTempBlockTableRecord);
		pTempBlockTable->close();
		//���ɸ�ӡ���ͼԪ
		f3dPoint topLeft(0,nRectW),dim_pos(nRectL*0.5,nRectW*0.5,0);
		CreateAcadRect(pTempBlockTableRecord,topLeft,nRectL,nRectW);	//���������
		DimText(pTempBlockTableRecord,dim_pos,CXhChar16("��ӡ��"),TextStyleTable::hzfs.textStyleId,txt_height,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pTempBlockTableRecord->close();
	}
	//����ӡͼԪ����ͼ�飬��ʾ�û�����ָ��λ��
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
		int retCode=DragEntSet(insert_pos,"��������\n");
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
//ͨ����ȡTxt�ļ���������
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
		{	//���ٶ�λ��
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
		{	//ֱ�߲岹
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
		{	//˳ʱ��Բ���岹
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
			//����ǰһ������
			for (size_t index = 0; index < vertexArr.size(); index++)
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
			//����¶���
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
	CFileDialog dlg(TRUE, "txt", "�и�NC�ļ�", 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "�����ļ�(*.txt)|*.txt||");
	if (dlg.DoModal() != IDOK)
		return;
	//
	vector <CPlateObject::VERTEX> vertexArr;
	ReadVertexArrFromFile(dlg.GetPathName(), vertexArr);
	//
	DRAGSET.ClearEntSet();
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	//����������
	int n = vertexArr.size();
	for (int i = 0; i < n-1; i++)
	{
		CPlateObject::VERTEX curVer = vertexArr.at(i);
		CPlateObject::VERTEX nxtVer = vertexArr.at((i + 1) % n);
		CreateAcadPoint(pBlockTableRecord, curVer.pos);
		DimText(pBlockTableRecord, curVer.pos, CXhChar16("%d", i),
			TextStyleTable::hzfs.textStyleId, 2, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		//·��
		if (nxtVer.tag.lParam == 1)
			continue;	//�¶�·������ʼ
		GEPOINT ptS = curVer.pos, ptE = nxtVer.pos;
		if (curVer.ciEdgeType == 1)
			CreateAcadLine(pBlockTableRecord, ptS, ptE);
		else
		{	//Բ��
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
	DragEntSet(base, "���ȡ����ͼ�Ĳ����");
#ifdef _ARX_2007
	acedCommand(RTSTR, L"PDMODE", RTSTR, L"34", RTNONE);	//��ʾ��
#else
	acedCommand(RTSTR, "PDMODE", RTSTR, "34", RTNONE);		//��ʾ�� X
#endif
}
#endif
//////////////////////////////////////////////////////////////////////////
//ϵͳ����
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
//У�󹹼�������Ϣ
//RevisionPartProcess
//////////////////////////////////////////////////////////////////////////
#ifdef __UBOM_ONLY_
void RevisionPartProcess()
{
	CLogErrorLife logErrLife;
	//��ʾ�Ի���
	int nWidth = g_xBomCfg.InitBomTitle();
	g_xPNCDockBarManager.DisplayRevisionDockBar(nWidth);
}
#endif
//////////////////////////////////////////////////////////////////////////
//�ڲ����Դ���
//InternalTest
//////////////////////////////////////////////////////////////////////////
#ifdef __ALFA_TEST_
#include "TestCode.h"
void InternalTest()
{
	return TestPnc();
}
#endif