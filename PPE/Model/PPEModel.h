#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "ProcessPart.h"
#include <vector>

using std::vector;

class CPPEModel
{
private:
	CXhChar500 m_sFolderPath;	//文件夹路径
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
		CProcessPlate destPlateOfComposite;	//复合模式
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
	CXhChar50 m_sVersion;		//版本号
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaAlias;		//代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar16 m_sOperator;		//操作员（制表人）
	CXhChar16 m_sAuditor;		//审核人
	CXhChar16 m_sCritic;		//评审人
	//文件输出路径
	CXhChar500 m_sOutputPath;	//文件夹路径
	//文件输出格式定制
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
			//文件格式末尾带特殊分隔符
			if(nKeySize >0 && nSplit == nKeySize && m_sSplitters[nSplit - 1].GetLength() > 0)
				sText.Append(m_sSplitters[nSplit - 1]);
			return sText;
		}
		bool IsValidFormat() { return m_sKeyMarkArr.size() > 0; }
	}file_format;
#ifdef __PNC_
	//PNC的功能权限控制
	static const BYTE FUNC_IDENTITY_FLAME_CUT	  = 0x01;	//火焰切割
	static const BYTE FUNC_IDENTITY_PLASMA_CUT	  = 0x02;	//等离子切割
	static const BYTE FUNC_IDENTITY_PUNCH_PROCESS = 0x04;	//冲床加工
	static const BYTE FUNC_IDENTITY_DRILL_PROCESS = 0x08;	//钻床加工
	static const BYTE FUNC_IDENTITY_LASER_PROCESS = 0x10;	//激光复合机
	static const BYTE FUNC_IDENTITY_HOLE_SORT	  = 0X11;	//螺栓孔排序
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
	static ILog2File* Log2File();//永远会返回一个有效指针,但只有log2file!=NULL时,才会真正记录错误日志
public:
	CPPEModel(void);
	~CPPEModel(void);
	//
	void Empty();
	BOOL FromBuffer(CBuffer &buffer);
	void ToBuffer(CBuffer &buffer,const char *file_version);
	BOOL InitModelByFolderPath(const char *folder_path);
	void InitPlateMcsAndCutPt(bool bSaveToFile=false);	//初始化钢板加工坐标系及切入点
	void InitPlateRelaNcPlate();
	//数据操作
	CProcessPart* AddPart(const char *sPartNo,BYTE cType,const char *sFilePath=NULL);
	BOOL DeletePart(const char *sPartNo);
	CProcessPart* FromPartNo(const char *sPartNo, int iNcType = 0);
	CProcessPart* EnumPartFirst(){return m_hashPartByPartNo.GetFirst();}
	CProcessPart* EnumPartNext(){return m_hashPartByPartNo.GetNext();}
	DWORD PartCount(){return m_hashPartByPartNo.GetNodeNum();}
	void ModifyPartNo(const char* sOldPartNo,const char* sNewPartNo);
	void SyncRelaPlateInfo(CProcessPlate* pWorkPlate);
	//关联文件路径
	CXhChar500 GetFolderPath();
	CXhChar500 GetPartFilePath(const char *sPartNo);
	bool SavePartToFile(CProcessPart *pPart);
	//得到有序集合(角钢集合和钢板集合)
	void GetSortedAngleSetAndPlateSet(CXhPtrSet<CProcessAngle> &angleSet,CXhPtrSet<CProcessPlate> &plateSet);
	void ReadPrjTowerInfoFromCfgFile(const char* cfg_file_path);
	//进度显示回调函数
	void(*DisplayProcess)(int percent, char *sTitle);
};
extern CPPEModel model;
