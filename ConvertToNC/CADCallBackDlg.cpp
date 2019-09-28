//<LOCALE_TRANSLATE BY hxr /> 2015-04-25
// CADCallBackDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CADCallBackDlg.h"
#include "dbents.h"
//#include "RxTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCADCallBackDlg dialog

CCADCallBackDlg::CCADCallBackDlg(UINT nIDTemplate,CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent)
{
	
	//{{AFX_DATA_INIT(CCADCallBackDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nPickEntType=1;	//0.点 1.其余实体 wht 11-06-29
	m_iBreakType=1;
	m_bInernalStart=FALSE;
	m_bPauseBreakExit=FALSE;
	FireCallBackFunc=NULL;
	m_nResultEnt=1;	//默认选择一个实体
	m_bFireListItemChange=TRUE;
	m_arrCmdPickPrompt.RemoveAll();
}


void CCADCallBackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCADCallBackDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCADCallBackDlg, CDialog)
	//{{AFX_MSG_MAP(CCADCallBackDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADCallBackDlg message handlers

#if defined(_WIN64)
INT_PTR CCADCallBackDlg::DoModal()
#else
int CCADCallBackDlg::DoModal() 
#endif
{
	//m_bInernalStart=FALSE;
	int ret=-1;
	ads_name prevPickEnt;
	ads_name_clear(prevPickEnt);
	while(1)
	{
		ret=(int)CDialog::DoModal();
		if(m_bPauseBreakExit)
		{
			if(m_iBreakType==1)
			{	
				//通过选择实体获取实体的ID
				ads_name pick_ent;
				ads_point pick_pt,prev_pt;
				resultList.Empty();
				for(int i=0;i<m_nResultEnt;i++)
				{
					AcDbEntity *pEnt=NULL;
					char sPrompt[200]=""; 
					if(i<m_arrCmdPickPrompt.GetSize())	//外部设定
						strcpy(sPrompt,m_arrCmdPickPrompt[i]);
					else if(m_nPickEntType==0)	//默认的提示字符串
#ifdef AFX_TARG_ENU_ENGLISH
						sprintf(sPrompt,"\n Please select no. %d outline vertex<Enter Confirm>:",i+1);
					else if(m_nPickEntType==1)	//数据扩展点对应的节点
						sprintf(sPrompt,"\n Please select no. %d node<Enter Confirm>:",i+1);
					else if(m_nPickEntType==2)	//选择杆件心线
						sprintf(sPrompt,"\n Please select no. %d rod(bolt line)<right click finish select>:",i+1);
					else 
						sprintf(sPrompt,"\n Please select no.%d object<Enter Confirm>:",i+1);
#else
						sprintf(sPrompt,"\n请选择第%d个轮廓点<Enter确认>:",i+1);
					else if(m_nPickEntType==1)	//数据扩展点对应的节点
						sprintf(sPrompt,"\n请选择第%d个节点<Enter确认>:",i+1);
					else if(m_nPickEntType==2)	//选择杆件心线
						sprintf(sPrompt,"\n请选择第%d根杆件(心线)<右键结束选择>:",i+1);
					else 
						sprintf(sPrompt,"\n请选择第%d个对象<Enter确认>:",i+1);
#endif
					if(m_nPickEntType==0)
					{
						ads_point temp_pt;
						if(i==0)
						{
						#ifdef _ARX_2007
							if(acedGetPoint(NULL,_bstr_t(sPrompt),temp_pt)==RTNORM)
						#else	
							if(acedGetPoint(NULL,sPrompt,temp_pt)==RTNORM)
						#endif
							{
								CAD_SCREEN_ENT *pCADEnt=resultList.append();
								pCADEnt->m_ptPick.Set(temp_pt[X],temp_pt[Y],temp_pt[Z]);
								prev_pt[X]=temp_pt[X];
								prev_pt[Y]=temp_pt[Y];
								prev_pt[Z]=temp_pt[Z];
							}
							else 
							{
								if(m_nPickEntType==2&&!ads_name_nil(prevPickEnt))	//手动添加相似形
									acedRedraw(prevPickEnt,4);	//取消之前选中图元的选中状态
								break;
							}
						}
						else 
						{
						#ifdef _ARX_2007
							if(acedGetPoint(prev_pt,_bstr_t(sPrompt),temp_pt)==RTNORM)
						#else	
							if(acedGetPoint(prev_pt,sPrompt,temp_pt)==RTNORM)
						#endif
							{
								CAD_SCREEN_ENT *pCADEnt=resultList.append();
								pCADEnt->m_ptPick.Set(temp_pt[X],temp_pt[Y],temp_pt[Z]);
								prev_pt[X]=temp_pt[X];
								prev_pt[Y]=temp_pt[Y];
								prev_pt[Z]=temp_pt[Z];
							}
							else 
								break;
						}
					}
					else 
					{
						if(m_nPickEntType==1)
						{	//数据点对应节点 wht 11-07-13
					   #ifdef _ARX_2007
							acedCommand(RTSTR,L"PDMODE",RTSTR,L"34",RTNONE);//显示点 X
					   #else
							acedCommand(RTSTR,"REGEN",RTNONE);
					   #endif
						}
						if(m_nPickEntType==2)	//手动添加相似形
						{
							if(!ads_name_nil(prevPickEnt))
								acedRedraw(prevPickEnt,4);	//取消之前选中图元的选中状态
							ads_name_set(pick_ent,prevPickEnt);
						}
					#ifdef _ARX_2007
						if(ads_entsel(_bstr_t(sPrompt),pick_ent,pick_pt)==RTNORM)
					#else
						if(ads_entsel(sPrompt,pick_ent,pick_pt)==RTNORM)
					#endif
						{
							CAD_SCREEN_ENT *pCADEnt=resultList.append();
							acdbGetObjectId(pCADEnt->m_idEnt,pick_ent);
							acdbOpenObject(pEnt,pCADEnt->m_idEnt,AcDb::kForRead);
							if(pEnt)
							{
								if(pEnt->isKindOf(AcDbPoint::desc()))
								{	//点
									AcDbPoint *pPoint=(AcDbPoint*)pEnt;
									m_ptPick.Set(pPoint->position().x,pPoint->position().y,pPoint->position().z);
									pCADEnt->m_ptPick=m_ptPick;
								}
								else if(pEnt->isKindOf(AcDbLine::desc()))
								{	//直线
									
								}
								//long hObj=GetTaAtomHandle(pEnt);
								//pCADEnt->m_pObj=Ta.FromHandle(hObj);
								//高亮显示实体 wht 12-03-22
								ads_name ent_name;
								acdbGetAdsName(ent_name,pCADEnt->m_idEnt);
								ads_redraw(ent_name,3);//高亮显示
								
								pEnt->close();
								if(m_nPickEntType==1)
								{	//数据点对应节点 wht 11-07-13
							   #ifdef _ARX_2007
									acedCommand(RTSTR,L"PDMODE",RTSTR,L"0",RTNONE);//显示点 .
							   #else
									acedCommand(RTSTR,"REGEN",RTNONE);
							   #endif
								}
							}
						}
						else 
						{
							if(m_nPickEntType==1)
							{	//数据点对应节点 wht 11-07-13
						   #ifdef _ARX_2007
							   acedCommand(RTSTR,L"PDMODE",RTSTR,L"0",RTNONE);//显示点 .
						   #else
							   acedCommand(RTSTR,"REGEN",RTNONE);
						   #endif
							}
							break;
						}
					}
				}
			}
			else if(m_iBreakType==3)
			{	//执行命令
				if(FireCallBackFunc)
					FireCallBackFunc(this);
			}
			m_bInernalStart=TRUE;
			m_bPauseBreakExit=FALSE;
		}
		else
			break;
	}
	return ret;
}
