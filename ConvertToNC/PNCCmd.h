#pragma once
#ifndef __PNC_MENU_FUNC_H_
#define __PNC_MENU_FUNC_H_
#include "PNCModel.h"
//加工
void SmartExtractPlate();	//智能提取板的信息
void SmartExtractPlate(CPNCModel *pModel);
void ManualExtractPlate();	//手动提取钢板信息
void ManualExtractPlate(CPNCModel *pModel);
void EnvGeneralSet();		//系统设置
#ifndef __UBOM_ONLY_
void SendPartEditor();		//编辑钢板信息
void LayoutPlates();		//自动排版
void InsertMKRect();		//插入钢印区
void DrawProfileByTxtFile();//通过读取Txt文件绘制外形
#endif
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
void RevisionPartProcess();	//校审构件工艺信息
#endif
#endif