#pragma once
#include "PNCModel.h"
//�ӹ�
void SmartExtractPlate();	//������ȡ�����Ϣ
void SmartExtractPlate(CPNCModel *pModel, BOOL bSupportSelectEnts = FALSE);
void EnvGeneralSet();		//ϵͳ����
#ifndef __UBOM_ONLY_
void SendPartEditor();		//�༭�ְ���Ϣ
void ShowPartList();		//��ʾ�ְ���Ϣ
void DrawPlates();			//���Ƹְ�
void InsertMKRect();		//�����ӡ��
void DrawProfileByTxtFile();//ͨ����ȡTxt�ļ���������
void ExplodeText();			//�����ı�(�ı�����ת��Ϊ�����)
#else
void RevisionPartProcess();	//У�󹹼�������Ϣ
#endif
#ifdef __ALFA_TEST_
void InternalTest();		//���Դ���
#endif
