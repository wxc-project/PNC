#pragma once
#ifndef __PNC_MENU_FUNC_H_
#define __PNC_MENU_FUNC_H_
#include "PNCModel.h"
//�ӹ�
void SmartExtractPlate();	//������ȡ�����Ϣ
void SmartExtractPlate(CPNCModel *pModel, BOOL bSupportSelectEnts = FALSE, CHashSet<AcDbObjectId>* pObjIdSet=NULL);
void EnvGeneralSet();		//ϵͳ����
#ifndef __UBOM_ONLY_
void SendPartEditor();		//�༭�ְ���Ϣ
void ShowPartList();		//��ʾ�ְ���Ϣ
void DrawPlates();			//���Ƹְ�
void InsertMKRect();		//�����ӡ��
void DrawProfileByTxtFile();//ͨ����ȡTxt�ļ���������
#else
void RevisionPartProcess();	//У�󹹼�������Ϣ
#endif
#ifdef __ALFA_TEST_
void InternalTest();		//���Դ���
#endif
#endif