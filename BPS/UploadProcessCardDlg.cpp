// UploadProcessCardDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "UploadProcessCardDlg.h"
#include "BPSModel.h"
#include "ComparePartNoString.h"
#include "SortFunc.h"
#include "ServerTowerType.h"
#include "SelectTowerTypeDlg.h"
#include "TMS.h"
#include "MD5.H"
#include "CadToolFunc.h"

//回调函数处理
static BOOL FireItemChanged(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,NM_LISTVIEW* pNMListView)
{	//选中项发生变化后更新属性栏
	if(pItem->m_idProp==NULL)
		return FALSE;
	CUploadProcessCardDlg *pDlg=(CUploadProcessCardDlg*)pListCtrl->GetParent();
	if(pDlg==NULL)
		return FALSE;
	CAngleProcessInfo* pJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
	SCOPE_STRU scope=pJgInfo->GetCADEntScope();
	ZoomAcadView(scope,10);
	pDlg->RefreshPicture();
	return TRUE;
}
static BOOL FireDeleteItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem)
{	
	if(pItem->m_idProp==NULL)
		return FALSE;
	CUploadProcessCardDlg *pDlg=(CUploadProcessCardDlg*)pListCtrl->GetParent();
	if(pDlg==NULL)
		return FALSE;
	CAngleProcessInfo* pJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
	if(pJgInfo)
	{
#ifdef _WIN64
		BPSModel.DeleteJgInfo(pJgInfo->keyId.asOldId());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
#else
		BPSModel.DeleteJgInfo((Adesk::UInt32)pJgInfo->keyId.handle());	//更新工艺卡构件时，如果件号entId发生变化需要删除原有角钢 wht 17-12-18
#endif
		for(CAngleProcessInfo *pInfo=pDlg->m_xJgList.GetFirst();pInfo;pInfo=pDlg->m_xJgList.GetNext())
		{
			if(pInfo->keyId==pJgInfo->keyId)
			{
				pDlg->m_xJgList.DeleteCursor();
				break;
			}
		}
	}
	return TRUE;
}
// CUploadProcessCardDlg 对话框

IMPLEMENT_DYNAMIC(CUploadProcessCardDlg, CDialog)

CUploadProcessCardDlg::CUploadProcessCardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUploadProcessCardDlg::IDD, pParent)
{
	m_pImageFile=NULL;
	image.bmBits=NULL;
	m_pTask=NULL;
	m_pTowerType=NULL;
}

CUploadProcessCardDlg::~CUploadProcessCardDlg()
{
	if(image.bmBits!=NULL)
		delete[] image.bmBits;
}

void CUploadProcessCardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_JG_LIST, m_xListCtrl);
	DDX_Control(pDX, IDC_CMB_TASK, m_cmbTask);
	DDX_Control(pDX, IDC_CMB_TOWERTYPE, m_cmbTowerType);
}

BEGIN_MESSAGE_MAP(CUploadProcessCardDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CUploadProcessCardDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_CMB_TASK, &CUploadProcessCardDlg::OnCbnSelchangeCmbTask)
	ON_CBN_SELCHANGE(IDC_CMB_TOWERTYPE, &CUploadProcessCardDlg::OnCbnSelchangeCmbTowerType)
END_MESSAGE_MAP()


// CUploadProcessCardDlg 消息处理程序


BOOL CUploadProcessCardDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_xListCtrl.EmptyColumnHeader();
	m_xListCtrl.AddColumnHeader("上传",40);
	m_xListCtrl.AddColumnHeader("件号",62);
	m_xListCtrl.AddColumnHeader("材质",42);
	m_xListCtrl.AddColumnHeader("规格",60);
	m_xListCtrl.AddColumnHeader("件数",45);
	m_xListCtrl.AddColumnHeader("备注",45);
	m_xListCtrl.AddColumnHeader("文件",62);
	m_xListCtrl.InitListCtrl(NULL,FALSE);
	m_xListCtrl.EnableSortItems(false);
	m_xListCtrl.SetItemChangedFunc(FireItemChanged);
	m_xListCtrl.SetDeleteItemFunc(FireDeleteItem);
	//
	m_sCardPath.Format("%s",(char*)CIdentifyManager::GetJgCardPath());
	//初始化列表框
	UpdateJgInfoList();
	RefreshListCtrl();
	//初始化任务塔型
	InitTaskTowerTypes();
	//移动对话框到右下角
	CRect rect;
	GetWindowRect(rect);
	int width = rect.Width();
	int height=rect.Height();
	int cx=GetSystemMetrics(SM_CXFULLSCREEN);
	int cy=GetSystemMetrics(SM_CYFULLSCREEN);
	rect.right = cx;
	rect.bottom = cy;
	rect.left=cx-width;
	rect.top=cy-height;
	MoveWindow(rect,TRUE);
	return TRUE;
}

void CUploadProcessCardDlg::InitTaskTowerTypes()
{	//1.加载在放样塔型列表
	CBuffer buffer;
	if(!TMS.QueryStateManuTasks(&buffer,1))
	{
		AfxMessageBox("数据加载失败！");
		return;
	}
	buffer.SeekToBegin();
	AgentServer.ParseManuTasksFromBuffer(buffer,&AgentServer.hashManuTasks);
	//2.根据角钢工艺卡中的塔型名称找到对应的任务及塔型
	CXhChar100 sTowerType;
	for(CAngleProcessInfo *pAngleInfo=BPSModel.EnumFirstJg();pAngleInfo;pAngleInfo=BPSModel.EnumNextJg())
	{
		if(pAngleInfo->m_sTowerType.Length()>0)
		{
			sTowerType.Copy(pAngleInfo->m_sTowerType);
			break;
		}
	}
	CServerManuTask *pInitTask=NULL;
	CServerTowerType *pInitTowerType=NULL;
	for(CServerManuTask *pTask=AgentServer.hashManuTasks.GetFirst();pTask;pTask=AgentServer.hashManuTasks.GetNext())
	{
		if(BPSModel.m_idManuTask==pTask->Id)
			m_pTask=pTask;
		for(CServerTowerType *pTowerType=pTask->hashTowerTypes.GetFirst();pTowerType;pTowerType=pTask->hashTowerTypes.GetNext())
		{
			if(sTowerType.GetLength()>0&&pTowerType->m_sName.Copy(sTowerType))
			{
				pInitTowerType=pTowerType;
				pInitTask=pTask;
			}
			if(BPSModel.m_idTowerType==pTowerType->Id)
			{
				m_pTowerType=pTowerType;
				m_pTask=pTask;
			}
		}
	}
	if(m_pTowerType==NULL)
	{
		m_pTask=pInitTask;
		m_pTowerType=pInitTowerType;
	}
	//3.初始化任务及塔型下拉框
	while(m_cmbTask.GetCount()>0)
		m_cmbTask.DeleteString(0);
	int iCurSel=0;
	for(CServerManuTask *pTask=AgentServer.hashManuTasks.GetFirst();pTask;pTask=AgentServer.hashManuTasks.GetNext())
	{
		int iItem=m_cmbTask.AddString(CXhChar500("%s(id:%d)",(char*)pTask->m_sName,pTask->Id));
		m_cmbTask.SetItemData(iItem,(DWORD)pTask);
		if(m_pTask==pTask)
			iCurSel=iItem;
	}
	if(m_cmbTask.GetCount()>0)
		m_cmbTask.SetCurSel(iCurSel);
	OnCbnSelchangeCmbTask();
}

void CUploadProcessCardDlg::OnCbnSelchangeCmbTask()
{
	int iCurTaskSel=m_cmbTask.GetCurSel();
	if(iCurTaskSel<0)
		return;
	CServerManuTask *pTask=(CServerManuTask*)m_cmbTask.GetItemData(iCurTaskSel);
	if(pTask==NULL)
		return;
	while(m_cmbTowerType.GetCount()>0)
		m_cmbTowerType.DeleteString(0);
	int iCurSel=0;
	for(CServerTowerType *pTowerType=pTask->hashTowerTypes.GetFirst();pTowerType;pTowerType=pTask->hashTowerTypes.GetNext())
	{
		int iItem=m_cmbTowerType.AddString(CXhChar500("%s(id:%d)",(char*)pTowerType->m_sName,pTowerType->Id));
		m_cmbTowerType.SetItemData(iItem,(DWORD)pTowerType);
		if(pTowerType==m_pTowerType)
			iCurSel=iItem;
	}
	if(m_cmbTowerType.GetCount()>0)
		m_cmbTowerType.SetCurSel(iCurSel);
	if(m_cmbTowerType.GetCount()==1)
		m_pTowerType=pTask->hashTowerTypes.GetFirst();
	bool bVisible=m_cmbTowerType.GetCount()>1;
	m_cmbTowerType.ShowWindow(bVisible?SW_SHOW:SW_HIDE);
	GetDlgItem(IDC_S_TOWERTYPE)->ShowWindow(bVisible?SW_SHOW:SW_HIDE);
}

void CUploadProcessCardDlg::OnCbnSelchangeCmbTowerType()
{
	int iCurSel=m_cmbTask.GetCurSel();
	if(iCurSel<0)
		return;
	m_pTowerType=(CServerTowerType*)m_cmbTowerType.GetItemData(iCurSel);
}

CImageFile* CUploadProcessCardDlg::ReadImageFile(const char* image_file)
{
	m_pImageFile=NULL;
	if(image_file==NULL)
		return NULL;
	char drive[4],dir[MAX_PATH],fname[MAX_PATH],ext[MAX_PATH];
	_splitpath(image_file,drive,dir,fname,ext);
	if(stricmp(ext,".jpg")==0)
		m_pImageFile=&m_xJpegImage;
	else if(stricmp(ext,".png")==0)
		m_pImageFile=&m_xPngImage;
	else
		return NULL;
	FILE* fp=fopen(image_file,"rb");
	if(fp==NULL)
		return NULL;
	m_pImageFile->ReadImageFile(fp);
	fclose(fp);
	
	image.bmHeight=m_pImageFile->GetHeight();
	image.bmWidthBytes=m_pImageFile->GetEffWidth();
	image.bmWidth=m_pImageFile->GetWidth();
	image.bmBitsPixel=m_pImageFile->GetBpp();
	image.bmType=0;
	if(image.bmBits!=NULL)
		delete[] image.bmBits;
	image.bmBits = new BYTE[image.bmHeight*image.bmWidthBytes];
	m_pImageFile->GetBitmapBits(image.bmHeight*image.bmWidthBytes,(BYTE*)image.bmBits);
	return m_pImageFile;
}

void CUploadProcessCardDlg::RefreshPrompt()
{	//更新提示字符串
	CWnd *pWnd=GetDlgItem(IDC_S_PROMPT_INFO);
	if(pWnd==NULL)
		return;
	int nCount=m_xListCtrl.GetItemCount();
	int iCurSel=m_xListCtrl.GetSelectedItem();
	CXhChar100 sPrompt("总共%d构件!",nCount);
	if(iCurSel>0)
	{
		CSuperGridCtrl::CTreeItem *pSelItem=m_xListCtrl.GetTreeItem(iCurSel);
		if(pSelItem==NULL||pSelItem->m_idProp==NULL)
			return;
		CAngleProcessInfo *pPartInfo=(CAngleProcessInfo*)pSelItem->m_idProp;
		sPrompt.Append(CXhChar100("文件名:%s",(char*)pPartInfo->m_sPartNo));
	}
	pWnd->SetWindowText(sPrompt);
}

void CUploadProcessCardDlg::RefreshPicture()
{
	RefreshPrompt();
	CWnd *pPicture=(CWnd*)GetDlgItem(IDC_S_CARD);
	if(pPicture==NULL)
		return;
	int iCurSel=m_xListCtrl.GetSelectedItem();
	if(iCurSel<0)
		return;
	CSuperGridCtrl::CTreeItem *pSelItem=m_xListCtrl.GetTreeItem(iCurSel);
	if(pSelItem==NULL||pSelItem->m_idProp==NULL)
		return;
	CAngleProcessInfo *pPartInfo=(CAngleProcessInfo*)pSelItem->m_idProp;
	//1.读取图片文件
	CImageFile *pImageFile=ReadImageFile(pPartInfo->m_sCardPngFilePath);
	if(pImageFile==NULL)
		return;
	//2.计算图片文件显示区域并显示图片
	CRect rectWndDraw;
	pPicture->GetClientRect(&rectWndDraw);  
	BITMAPINFO bmpInfo;
	memset(&bmpInfo,0,sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biBitCount=image.bmBitsPixel;
	bmpInfo.bmiHeader.biHeight = image.bmHeight;
	bmpInfo.bmiHeader.biWidth = image.bmWidth;
	bmpInfo.bmiHeader.biPlanes=1;
	bmpInfo.bmiHeader.biSize=40;
	bmpInfo.bmiHeader.biSizeImage=image.bmWidthBytes*image.bmHeight;
	CDC *pDC=pPicture->GetDC();
	::StretchDIBits(pDC->GetSafeHdc(),rectWndDraw.left,rectWndDraw.top,rectWndDraw.Width(),rectWndDraw.Height(),
		0,0,image.bmWidth,image.bmHeight,image.bmBits,&bmpInfo,DIB_RGB_COLORS,SRCCOPY);
}

void CUploadProcessCardDlg::RefreshListCtrl()
{
	m_xListCtrl.DeleteAllItems();
	CSuperGridCtrl::CTreeItem *pItem=NULL;
	for(CAngleProcessInfo* pJgInfo=m_xJgList.GetFirst();pJgInfo;pJgInfo=m_xJgList.GetNext())
	{
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		if(pJgInfo->m_bUploadToServer)
			lpInfo->SetSubItemText(0,"√",TRUE);	//上传状态
		else if(BPSModel.m_iRetrieveBatchNo>2&&pJgInfo->m_iBatchNo==BPSModel.m_iRetrieveBatchNo-1)
			lpInfo->SetSubItemText(0,"*",TRUE);		//有更新
		else
			lpInfo->SetSubItemText(0,"",TRUE);		//未上传
		lpInfo->SetSubItemText(1,pJgInfo->m_sPartNo,TRUE);		//件号
		CXhChar16 sMat=CBPSModel::QuerySteelMatMark(pJgInfo->m_cMaterial);
		lpInfo->SetSubItemText(2,sMat,TRUE);//材质	
		lpInfo->SetSubItemText(3,pJgInfo->m_sSpec,TRUE);		//规格
		lpInfo->SetSubItemText(4,CXhChar50("%d",pJgInfo->m_nSumNum),TRUE);	//件数
		lpInfo->SetSubItemText(5,pJgInfo->m_sNotes);
		lpInfo->SetSubItemText(6,pJgInfo->m_sCardPngFile,TRUE);
		pItem=m_xListCtrl.InsertRootItem(lpInfo);
		pItem->m_idProp=(long)pJgInfo;
	}
	RefreshPrompt();
	UpdateData(FALSE);
}
static int CompareFun1(const CAngleProcessInfo& jginfo1,const CAngleProcessInfo& jginfo2)
{
	//首先根据材料名称进行排序
	if(jginfo1.m_ciType>jginfo2.m_ciType)
		return 1;
	else if(jginfo1.m_ciType<jginfo2.m_ciType)
		return -1;
	//然后根据材质进行排序
	/*int iMatMark1=CBPSModel::QuerySteelMatIndex(jginfo1.m_cMaterial);
	int iMatMark2=CBPSModel::QuerySteelMatIndex(jginfo2.m_cMaterial);
	if(iMatMark1>iMatMark2)
		return 1;
	else if(iMatMark1<iMatMark2)
		return -1;
	//最后根据材质
	if(jginfo1.m_ciType==1)
	{
		if(jginfo1.m_nWidth>jginfo2.m_nWidth)
			return 1;
		else if(jginfo1.m_nWidth<jginfo2.m_nWidth)
			return -1;
		if(jginfo1.m_nThick>jginfo2.m_nThick)
			return 1;
		else if(jginfo1.m_nThick<jginfo2.m_nThick)
			return -1;
	}*/
	return ComparePartNoString(jginfo1.m_sPartNo,jginfo2.m_sPartNo);
}
void CUploadProcessCardDlg::UpdateJgInfoList()
{
	m_xJgList.Empty();
	m_nRecord=0;
	for(CAngleProcessInfo* pJgInfo=BPSModel.EnumFirstJg();pJgInfo;pJgInfo=BPSModel.EnumNextJg())
	{
		if(pJgInfo->m_ciType!=CAngleProcessInfo::TYPE_JG)
			continue;
		m_nRecord++;
		CAngleProcessInfo* pSelJg=m_xJgList.append();
		pSelJg->CopyProperty(pJgInfo);
	}
	//按照材料名称-材质-规格的顺序进行排序
	if(m_xJgList.GetSize()>0)
		CQuickSort<CAngleProcessInfo>::QuickSort(m_xJgList.m_pData,m_xJgList.GetSize(),CompareFun1);
}

void CUploadProcessCardDlg::OnOK()
{
	
	CDialog::OnOK();
}

/*
static const char *Hex2ASC(const BYTE *Hex, int Len)
{
	static char  ASC[4096 * 2]={0};
	int i=0;
	for(i = 0; i < Len; i++)
	{
		ASC[i * 2] = "0123456789ABCDEF"[Hex[i] >> 4];
		ASC[i * 2 + 1] = "0123456789ABCDEF"[Hex[i] & 0x0F];
	}
	ASC[i * 2] = '\0';
	return ASC;
}
const char* MD5Str(const unsigned char* src,int len)
{
	typedef struct {
		ULONG i[2];                          
		ULONG buf[4];                        
		unsigned char in[64];                
		unsigned char digest[16];            
	} MD5_CTX;
	#define MD5DIGESTLEN 16
	#define PROTO_LIST(list)    list
	
	typedef void (WINAPI* PMD5Init) PROTO_LIST ((MD5_CTX *));
	typedef void (WINAPI* PMD5Update) PROTO_LIST ((MD5_CTX *, const unsigned char *, unsigned int));
	typedef void (WINAPI* PMD5Final )PROTO_LIST ((MD5_CTX *));
	PMD5Init MD5Init = NULL;
	PMD5Update MD5Update = NULL;
	PMD5Final MD5Final = NULL;
	HINSTANCE hDLL;
	if((hDLL = LoadLibrary("advapi32.dll")) > 0)
	{
		MD5Init = (PMD5Init)GetProcAddress(hDLL,"MD5Init");
		MD5Update = (PMD5Update)GetProcAddress(hDLL,"MD5Update");
		MD5Final = (PMD5Final)GetProcAddress(hDLL,"MD5Final");
	}
	else
		return NULL;
	
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx,src,len);
	MD5Final(&ctx);
	return Hex2ASC(ctx.digest,16);
}
*/

int SerialTowerTypeParts(CBuffer &buffer,CServerTowerType *pTowerType,BOOL bIncFile)
{
	buffer.ClearContents();
	buffer.WriteInteger(pTowerType->Id);
	if (BPSModel.GetJgNum()==0)
		buffer.WriteInteger(-1);
	else
	{
		SEGI segI;
		buffer.WriteInteger(BPSModel.GetJgNum());
		for(CAngleProcessInfo *pJgInfo=BPSModel.EnumFirstJg();pJgInfo;pJgInfo=BPSModel.EnumNextJg())
		{
			ParsePartNo(pJgInfo->m_sPartNo,&segI,NULL);
			buffer.WriteInteger(segI.key.number);
			buffer.WriteString(segI.Prefix());
			buffer.WriteString(segI.Suffix());
			buffer.WriteString(pJgInfo->m_sPartNo);
			CXhChar16 sMat=CBPSModel::QuerySteelMatMark(pJgInfo->m_cMaterial);
			buffer.WriteString(sMat);
			buffer.WriteString(pJgInfo->m_sSpec);
			buffer.WriteString(pJgInfo->m_sNotes);
			buffer.WriteInteger(pJgInfo->m_nM12);			//M12孔数
			buffer.WriteInteger(pJgInfo->m_nM16);			//M16孔数
			buffer.WriteInteger(pJgInfo->m_nM18);			//M18孔数
			buffer.WriteInteger(pJgInfo->m_nM20);			//M20孔数
			buffer.WriteInteger(pJgInfo->m_nM22);			//M22孔数
			buffer.WriteInteger(pJgInfo->m_nM24);			//M24孔数
			buffer.WriteInteger(pJgInfo->m_nSumHoleCount);	//总孔数
			//角钢工艺
			buffer.WriteInteger(pJgInfo->m_bCutRoot);		//刨根
			buffer.WriteInteger(pJgInfo->m_bCutBer);		//铲背
			buffer.WriteInteger(pJgInfo->m_bMakBend);		//制弯
			buffer.WriteInteger(pJgInfo->m_bPushFlat);		//压扁
			buffer.WriteInteger(pJgInfo->m_bCutEdge);		//铲边
			buffer.WriteInteger(pJgInfo->m_bRollEdge);		//卷边
			buffer.WriteInteger(pJgInfo->m_bWeld);			//焊接
			buffer.WriteInteger(pJgInfo->m_bKaiJiao);		//开角
			buffer.WriteInteger(pJgInfo->m_bHeJiao);		//合角
			buffer.WriteInteger(pJgInfo->m_bCutAngle);		//切角
			long buf_size=0;
			CHAR_ARRAY contens(buf_size);
			if (bIncFile)
			{
				FILE* fp=fopen(pJgInfo->m_sCardPngFilePath,"rb");
				if (fp!=NULL)
				{
					fseek(fp,0,SEEK_END);
					buf_size=ftell(fp);
					fseek(fp,0,SEEK_SET);
					if(buf_size>0)
					{
						contens.Resize(buf_size);
						fread(contens,buf_size,1,fp);
					}
					fclose(fp);
				}
			}
			CXhChar200 sMd5;
			if(contens.Size()>0)
				sMd5.Copy(MD5Str((BYTE*)contens.Data(),contens.Size()));
			buffer.WriteString(sMd5);	//写入构件MD5
			if(buf_size<=0)
				buffer.WriteInteger((int)-1);
			else
			{
				buffer.WriteInteger(buf_size);
				buffer.WriteString(pJgInfo->m_sCardPngFile);
				buffer.Write(contens,buf_size);
			}
		}
	}
	return buffer.GetLength();
}

void CUploadProcessCardDlg::OnBnClickedOk()
{
	if(m_pTowerType==NULL)
	{
		AfxMessageBox("未选择任务无法上传!");
		return;
	}
	if(BPSModel.GetJgNum()<=0)
	{
		AfxMessageBox("无内容不需要上传!");
		return;
	}
	//序列化上传内容
	CBuffer buffer(10000000);
	SerialTowerTypeParts(buffer,m_pTowerType,TRUE);
	CXhChar500 sErrorMsg;
	if(TMS.UploadFileObject(-1,&buffer,true,false,sErrorMsg))
	{	
		CHashStrList<CXhChar16> hashPartLabel;
		if(sErrorMsg.GetLength()>0)
		{
			for(char* key=strtok(sErrorMsg,"#");key;key=strtok(NULL,"#"))
				hashPartLabel.SetValue(key,CXhChar16(key));
		}
		//更新构件上传状态
		for(CAngleProcessInfo *pInfo=BPSModel.EnumFirstJg();pInfo;pInfo=BPSModel.EnumNextJg())
		{
			if(hashPartLabel.GetValue(pInfo->m_sPartNo)==NULL)
				pInfo->m_bUploadToServer=TRUE;
			else
				pInfo->m_bUploadToServer=FALSE;
		}

		POSITION pos = m_xListCtrl.GetRootHeadPosition();
		while(pos!=NULL)
		{
			CSuperGridCtrl::CTreeItem* pItem=m_xListCtrl.GetNextRoot(pos);
			CAngleProcessInfo* pSelJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
			if(pSelJgInfo!=NULL)
			{
				if(hashPartLabel.GetValue(pSelJgInfo->m_sPartNo)==NULL)
				{
					pSelJgInfo->m_bUploadToServer=TRUE;
					m_xListCtrl.SetSubItemText(pItem,0,"√");
				}
				else
				{
					pSelJgInfo->m_bUploadToServer=FALSE;
					m_xListCtrl.SetSubItemText(pItem,0,"x");
				}
			}
		}
		AfxMessageBox("上传成功!");
	}
	else
		AfxMessageBox("上传失败!");
}