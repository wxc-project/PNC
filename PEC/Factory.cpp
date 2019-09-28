#include "stdafx.h"
#include "Hashtable.h"
#include "PEC.h"

CHashList<CPEC*> g_hashEditorBySerial;
class CPartsEditorsLife{
public:
	~CPartsEditorsLife(){
		for(CPEC** ppEditor=g_hashEditorBySerial.GetFirst();ppEditor;ppEditor=g_hashEditorBySerial.GetNext())
		{
			if(*ppEditor==NULL)
				continue;
			CPEC* pModel=(CPEC*)*ppEditor;
			delete pModel;
		}
		g_hashEditorBySerial.Empty();
	}
};
CPartsEditorsLife editorsLife;

IPEC* CPartsEditorFactory::CreatePartsEditorInstance()
{
	int iNo=1;
	do{
		if(g_hashEditorBySerial.GetValue(iNo)!=NULL)
			iNo++;
		else	//ÕÒµ½Ò»¸ö¿ÕºÅ
			break;
	}while(true);
	CPEC* pEditor = new CPEC(iNo);
	g_hashEditorBySerial.SetValue(iNo,pEditor);
	return pEditor;
};
IPEC* CPartsEditorFactory::PartsEditorFromSerial(long serial)
{
	CPEC** ppEditor=g_hashEditorBySerial.GetValue(serial);
	if(ppEditor&&*ppEditor!=NULL)
		return *ppEditor;
	else
		return NULL;
}
BOOL CPartsEditorFactory::Destroy(long h)
{
	CPEC** ppEditor=g_hashEditorBySerial.GetValue(h);
	if(ppEditor==NULL||*ppEditor==NULL)
		return FALSE;
	CPEC* pModel=(CPEC*)*ppEditor;
	delete pModel;
	return g_hashEditorBySerial.DeleteNode(h);
}
