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

#ifndef __UBOM_ONLY_
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
	CString file_path;
	GetCurWorkPath(file_path);
	IExtractor::m_bSendCommand = FALSE;
	if (model.m_sCurWorkFile.CompareNoCase(file_name) == 0)
	{	//ͬһ�ĵ����ִδ���
		CPNCModel tempMode;
		tempMode.ExtractPlates(file_name, TRUE);
		tempMode.DrawPlates();
		tempMode.CreatePlatePPiFile(file_path);
		//���ݿ���
		CPlateProcessInfo* pSrcPlate = NULL, *pDestPlate = NULL;
		for (pSrcPlate = tempMode.EnumFirstPlate(); pSrcPlate; pSrcPlate = tempMode.EnumNextPlate())
		{
			pDestPlate = model.GetPlateInfo(pSrcPlate->GetPartNo());
			if (pDestPlate == NULL)
				pDestPlate = model.AppendPlate(pSrcPlate->GetPartNo());
			pDestPlate->CopyAttributes(pSrcPlate);
		}
	}
	else
	{
		model.Empty();
		model.ExtractPlates(file_name, TRUE);
		model.DrawPlates();
		model.CreatePlatePPiFile(file_path);
	}
	//ͨ���˵���ȡ�ĸְ�����¹����б�
	CPartListDlg *pPartListDlg = g_xPNCDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL && pPartListDlg->GetSafeHwnd() != NULL)
	{
		pPartListDlg->RefreshCtrlState();
		pPartListDlg->RelayoutWnd();
		pPartListDlg->UpdatePartList();
	}
}
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
	//д�������������ļ� wht 19-01-12
	CString file_path;
	GetCurWorkPath(file_path);
	if (model.m_xPrjInfo.m_sTaType.GetLength() > 0)
	{
		CString cfg_path = file_path + "config.ini";
		cfg_path.Format("%sconfig.ini", file_path);
		model.WritePrjTowerInfoToCfgFile(cfg_path);
	}
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
bool ReadVertexArrFromFile(const char* filePath, vector <VERTEX>& vertexArr)
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
		VERTEX vertrex;
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
	vector <VERTEX> vertexArr;
	ReadVertexArrFromFile(dlg.GetPathName(), vertexArr);
	//
	DRAGSET.ClearEntSet();
	AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
	//����������
	int n = vertexArr.size();
	for (int i = 0; i < n-1; i++)
	{
		VERTEX curVer = vertexArr.at(i);
		VERTEX nxtVer = vertexArr.at((i + 1) % n);
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