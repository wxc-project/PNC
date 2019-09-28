#include "StdAfx.h"
#include "CadHighlightEntManager.h"

//设置命令对应的实体集合为高亮显示
CHashList<AcDbObjectId> CCadHighlightEntManager::hashHighlightEnts;
void CCadHighlightEntManager::SetEntSetHighlight(ARRAY_LIST<AcDbObjectId> &entIdList)
{
	if(entIdList.GetSize()==0)
		return;
	ads_name ent_name;
	for(AcDbObjectId *pEntId=entIdList.GetFirst();pEntId;pEntId=entIdList.GetNext())
	{
		if(acdbGetAdsName(ent_name,*pEntId)!=Acad::eOk)
			continue;
		ads_redraw(ent_name,3);//高亮显示
		hashHighlightEnts.SetValue((long)pEntId,*pEntId);
	}
	//更新界面
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}
void CCadHighlightEntManager::ReleaseHighlightEnts()
{
	for(AcDbObjectId *pEntId=hashHighlightEnts.GetFirst();pEntId;pEntId=hashHighlightEnts.GetNext())
	{
		ads_name ent_name;
		if(acdbGetAdsName(ent_name,*pEntId)!=Acad::eOk)
			continue;
		ads_redraw(ent_name,4);//取消高亮显示
	}
	hashHighlightEnts.Empty();
}