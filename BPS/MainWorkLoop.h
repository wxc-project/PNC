// WorkLoop.h: interface for the CWorkLoop class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINWORKLOOP_H__316E35B1_D46B_4391_9677_E27400FA1D65__INCLUDED_)
#define AFX_MAINWORKLOOP_H__316E35B1_D46B_4391_9677_E27400FA1D65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "list.h"
#include "HashTable.h"
#include "XhCharString.h"
#include "LicSvc.h"
#include "Communication.h"

struct LICSERV_MSGBUF
{	//消息头
	long msg_length;//消息长度
	long command_id;//命令标识
	BYTE src_code;	//源节点编号
	BYTE *lpBuffer;			//消息体
public:
	static const long APPLY_FOR_LICENSE = 1;	//申请终端使用授权
	static const long RETURN_LICENSE	= 2;	//返还终端使用授权
	static const long LICENSES_MODIFIED	= 3;	//合法终端用户信息发生变化
};

struct AUTHENTICATION{
	DWORD dwEndUserSerial;
	DWORD dwTimeStart;	//授权起始日期时间
	DWORD dwLastRunTime;//最近一次运行指定产品的时间
	DWORD dwTimeEnd;	//授权终止日期时间
	DWORD dwExpireTimes;//有效剩余使用次数，暂保留未启用
	AUTHENTICATION(){dwEndUserSerial=dwExpireTimes=0;dwTimeStart=dwTimeEnd=dwLastRunTime=0;}
};

struct ENDUSER
{
	DWORD serial;
	IDENTITY identity;	//终端计算机的标识指纹
	CXhChar50 ipstr;	//终端计算机IP地址
	CXhChar50 hostname;	//终端计算机的名称
	DWORD dwAcceptProductFlag;	//被接受的产品授权字
	void SetKey(DWORD key){serial=key;}
	ENDUSER(){serial=0;dwAcceptProductFlag=0;}
};
#pragma pack(push,2)
typedef struct IProductAcl{
	BYTE cProductType;
	BYTE ciMaxUsers;
public:
	IProductAcl(BYTE ciProductType=0){cProductType=ciProductType;ciMaxUsers=0;}
	virtual ~IProductAcl(){;}
	virtual BYTE MaxUsers()const{return ciMaxUsers;}
	virtual AUTHENTICATION* ActiveUsers(){return NULL;}
	AUTHENTICATION* FromIdentity(const BYTE ident_bytes[16],ENDUSER* pMatchedEndUser=NULL);
	BOOL RemoveIdentity(const BYTE ident_bytes[16]);
	AUTHENTICATION* AddAuthentication(AUTHENTICATION& authenticaion);
	static IProductAcl* CreateACLInstance(int nMaxUsers,DWORD key,void* pContext);
	static BOOL DeleteACLInstance(IProductAcl *pAcl);				//必设回调函数
}*IProductAclPtr;
template<class TYPE> struct PRODUCT_ACL_TEMPL : public IProductAcl{
	AUTHENTICATION uiActiveUsers[TYPE::MAX_COUNT];
	PRODUCT_ACL_TEMPL(BYTE ciProductType=0) : IProductAcl(ciProductType){
		ciMaxUsers=TYPE::MAX_COUNT;
	}
	virtual ~PRODUCT_ACL_TEMPL(){;}
	virtual AUTHENTICATION* ActiveUsers(){return uiActiveUsers;}
	virtual BYTE MaxUsers()const{return TYPE::MAX_COUNT;}
};
#pragma pack(pop)

struct USERS_COUNT2  {static const int MAX_COUNT =   2;};
struct USERS_COUNT3  {static const int MAX_COUNT =   3;};
struct USERS_COUNT5  {static const int MAX_COUNT =   5;};
struct USERS_COUNT10 {static const int MAX_COUNT =  10;};
struct USERS_COUNT20 {static const int MAX_COUNT =  20;};
struct USERS_COUNT30 {static const int MAX_COUNT =  30;};
struct USERS_COUNT50 {static const int MAX_COUNT =  50;};
struct USERS_COUNT80 {static const int MAX_COUNT =  80;};
struct USERS_COUNT100{static const int MAX_COUNT = 100;};
typedef PRODUCT_ACL_TEMPL<USERS_COUNT2> PRODUCT_ACL2;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT3> PRODUCT_ACL3;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT5> PRODUCT_ACL5;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT10> PRODUCT_ACL10;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT20> PRODUCT_ACL20;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT30> PRODUCT_ACL30;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT50> PRODUCT_ACL50;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT80> PRODUCT_ACL80;
typedef PRODUCT_ACL_TEMPL<USERS_COUNT100> PRODUCT_ACL100;

#include <afxole.h>         // MFC OLE classes
#include <afxpriv.h> // for wide-character conversion
#include "comdef.h"

class CUserDatabase  
{
	IStorage *m_pRootStg,*m_pUsersStg,*m_pProductAclStg;
	IEnumSTATSTG *m_pUserEnum,*m_pProductAclEnum;
	BOOL OpenStorage(const char* stg_name,BOOL bCreate);
	BOOL EnumUser(ENDUSER *pUser,BOOL bFirst);
	BOOL EnumProductACL(IProductAcl *pACL,BOOL bIncAuth,BOOL bFirst);
	BOOL OpenSrvDbFile(const char *file_path=NULL);
public:
	CUserDatabase(const char *file_path=NULL);
	virtual ~CUserDatabase();
	void Close();
	BOOL SetAdminPassword(char new_password[16],char old_password[16]);
	BOOL QueryAdminPassword(char password[16]);

	BOOL QueryFirstUser(ENDUSER* pUser);
	BOOL QueryNextUser(ENDUSER* pUser);
	BOOL SetUser(ENDUSER enduser);
	BOOL QueryUser(const char* sIdentity,ENDUSER *pUser);
	BOOL DelUser(BYTE identity[16]);

	BOOL QueryFirstProductACL(IProductAcl* pACL);
	BOOL QueryNextProductACL(IProductAcl* pACL);
	BOOL SetProductACL(IProductAcl* pACL);
	BOOL QueryProductACL(IProductAcl* pACL);
	BOOL DelProductACL(BYTE cProductType);
};

class CMainWorkLoop  
{
public:
	static SECURITY_ATTRIBUTES sa;
	static BOOL m_bQuitNow;
	static bool LoadProductSvcFile(const char* svcfilename,BYTE* pciProductType=NULL);
	static DWORD GenerateClientLicenseFile(BYTE cProductType,BYTE enduser_identity[16],BYTE* details,
		char* exportLicBytes=NULL,WORD wRenewDays=15);
	static bool LoadProductSvcFile(BYTE cProductType);
	static UINT IssueEndUserLicFile(BYTE cProductType,BYTE identity[16],BYTE* license);
	static BOOL ApplyForLicense(CCommunicationObject &commCore,LICSERV_MSGBUF &msg);
	static BOOL ReturnLicense(CCommunicationObject &commCore,LICSERV_MSGBUF &msg);
	static void LicenseValidate(CCommunicationObject &commCore,LICSERV_MSGBUF &msg);	//重新加载Database
	static long m_nMaxHandle;
	static CHashListEx<ENDUSER> hashEndUsers;
	static CHashPtrList<CLicService> hashLicSrvs;
	static BOOL QueryEndUser(const BYTE identity[16],ENDUSER* pUser){return FALSE;}
	static CSuperHashList<IProductAcl> hashProductSrvs;//[PRODUCT_IBOM];
	static UINT cursor_pipeline_no;
	CMainWorkLoop();
	virtual ~CMainWorkLoop();
	static bool InitDatabaseFromFile();
	static UINT StartTaskListen(LPVOID lpPara,BOOL bPipeConnect);
	static UINT StartPipeTaskListen(LPVOID lpPara);
	static UINT StartSocketTaskListen(LPVOID lpPara);
};

#endif // !defined(AFX_MAINWORKLOOP_H__316E35B1_D46B_4391_9677_E27400FA1D65__INCLUDED_)
