#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "ProcessPart.h"
#include <vector>

using std::vector;

class CPPEModel
{
private:
	CXhChar500 m_sFolderPath;	//�ļ���·��
	CSuperHashStrList<CProcessPart> m_hashPartByPartNo;
	CHashStrList<CXhChar500> m_hashFilePathByPartNo;
#ifdef __PNC_
	struct RELA_PLATE
	{
		CProcessPlate destPlateOfFlame;
		CProcessPlate destPlateOfPlasma;
		CProcessPlate destPlateOfPunch;
		CProcessPlate destPlateOfDrill;
		CProcessPlate destPlateOfLaser;
		CProcessPlate destPlateOfComposite;	//����ģʽ
		//
		void SetNewPartNo(const char* sNewPartNo)
		{
			destPlateOfFlame.SetPartNo(sNewPartNo);
			destPlateOfPlasma.SetPartNo(sNewPartNo);
			destPlateOfPunch.SetPartNo(sNewPartNo);
			destPlateOfDrill.SetPartNo(sNewPartNo);
			destPlateOfLaser.SetPartNo(sNewPartNo);
			destPlateOfComposite.SetPartNo(sNewPartNo);
		}
		void SetKeyId(int idKey)
		{
			destPlateOfFlame.SetKey(idKey);
			destPlateOfPlasma.SetKey(idKey);
			destPlateOfPunch.SetKey(idKey);
			destPlateOfDrill.SetKey(idKey);
			destPlateOfLaser.SetKey(idKey);
			destPlateOfComposite.SetKey(idKey);
		}
	};
	CHashStrList<RELA_PLATE> m_hashRelaPlateByPartNo;
#endif
public:
	CXhChar50 m_sVersion;		//�汾��
	CXhChar100 m_sCompanyName;	//��Ƶ�λ
	CXhChar100 m_sPrjCode;		//���̱��
	CXhChar100 m_sPrjName;		//��������
	CXhChar50 m_sTaType;		//����
	CXhChar50 m_sTaAlias;		//����
	CXhChar50 m_sTaStampNo;		//��ӡ��
	CXhChar16 m_sOperator;		//����Ա���Ʊ��ˣ�
	CXhChar16 m_sAuditor;		//�����
	CXhChar16 m_sCritic;		//������
	//�ļ����·��
	CXhChar500 m_sOutputPath;	//�ļ���·��
	//�ļ������ʽ����
	static const CXhChar16 KEY_TA_TYPE;
	static const CXhChar16 KEY_PART_NO;
	static const CXhChar16 KEY_PART_MAT;
	static const CXhChar16 KEY_PART_THICK;
	static const CXhChar16 KEY_SINGLE_NUM;
	static const CXhChar16 KEY_PROCESS_NUM;
	struct FILE_FORMAT 
	{
		vector<CXhChar16> m_sSplitters;
		vector<CXhChar16> m_sKeyMarkArr;
		//
		FILE_FORMAT() { ; }
		CXhChar100 GetFileFormatStr(){
			CXhChar100 sText;
			size_t nSplit = m_sSplitters.size();
			size_t nKeySize = m_sKeyMarkArr.size();
			for (size_t i = 0; i < nKeySize; i++)
			{
				if (sText.GetLength()>0 && nSplit >= i&& m_sSplitters[i - 1].GetLength() > 0)
					sText.Append(m_sSplitters[i - 1]);
				sText.Append(m_sKeyMarkArr[i]);
			}
			//�ļ���ʽĩβ������ָ���
			if(nKeySize >0 && nSplit == nKeySize && m_sSplitters[nSplit - 1].GetLength() > 0)
				sText.Append(m_sSplitters[nSplit - 1]);
			return sText;
		}
		bool IsValidFormat() { return m_sKeyMarkArr.size() > 0; }
	}file_format;
#ifdef __PNC_
	//PNC�Ĺ���Ȩ�޿���
	static const BYTE FUNC_IDENTITY_FLAME_CUT	  = 0x01;	//�����и�
	static const BYTE FUNC_IDENTITY_PLASMA_CUT	  = 0x02;	//�������и�
	static const BYTE FUNC_IDENTITY_PUNCH_PROCESS = 0x04;	//�崲�ӹ�
	static const BYTE FUNC_IDENTITY_DRILL_PROCESS = 0x08;	//�괲�ӹ�
	static const BYTE FUNC_IDENTITY_LASER_PROCESS = 0x10;	//���⸴�ϻ�
	static const BYTE FUNC_IDENTITY_HOLE_SORT	  = 0X11;	//��˨������
	DWORD m_dwFunctionFlag;
	bool VerifyValidFunction(BYTE ciFuncFlag) {
		if ((ciFuncFlag&m_dwFunctionFlag) > 0)
			return true;
		else
			return false;
	}
#endif
public:
	static ILog2File* log2file;
	static ILog2File* Log2File();//��Զ�᷵��һ����Чָ��,��ֻ��log2file!=NULLʱ,�Ż�������¼������־
public:
	CPPEModel(void);
	~CPPEModel(void);
	//
	void Empty();
	BOOL FromBuffer(CBuffer &buffer);
	void ToBuffer(CBuffer &buffer,const char *file_version);
	BOOL InitModelByFolderPath(const char *folder_path);
	void InitPlateMcsAndCutPt(bool bSaveToFile=false);	//��ʼ���ְ�ӹ�����ϵ�������
	void InitPlateRelaNcPlate();
	//���ݲ���
	CProcessPart* AddPart(const char *sPartNo,BYTE cType,const char *sFilePath=NULL);
	BOOL DeletePart(const char *sPartNo);
	CProcessPart* FromPartNo(const char *sPartNo, int iNcType = 0);
	CProcessPart* EnumPartFirst(){return m_hashPartByPartNo.GetFirst();}
	CProcessPart* EnumPartNext(){return m_hashPartByPartNo.GetNext();}
	DWORD PartCount(){return m_hashPartByPartNo.GetNodeNum();}
	void ModifyPartNo(const char* sOldPartNo,const char* sNewPartNo);
	void SyncRelaPlateInfo(CProcessPlate* pWorkPlate);
	//�����ļ�·��
	CXhChar500 GetFolderPath();
	CXhChar500 GetPartFilePath(const char *sPartNo);
	bool SavePartToFile(CProcessPart *pPart);
	//�õ����򼯺�(�Ǹּ��Ϻ͸ְ弯��)
	void GetSortedAngleSetAndPlateSet(CXhPtrSet<CProcessAngle> &angleSet,CXhPtrSet<CProcessPlate> &plateSet);
	void ReadPrjTowerInfoFromCfgFile(const char* cfg_file_path);
	//������ʾ�ص�����
	void(*DisplayProcess)(int percent, char *sTitle);
	void(*ExplodeText)(const char*sText, GEPOINT pt, double fTextH, double fRotAnge, ATOM_LIST<GELINE>& lineArr);
};
extern CPPEModel model;
