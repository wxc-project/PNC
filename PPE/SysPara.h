#pragma once
#include "XhCharString.h"
#include "PropListItem.h"
#include "PPEModel.h"
#include "XhLocale.h"

//�׾�����ֵ
struct HOLE_INCREMENT {
	double m_fDatum;	//����
	double m_fM12;		//M12
	double m_fM16;		//M16
	double m_fM20;		//M20
	double m_fM24;		//M24
	double m_fCutSH;	//�и�ʽ����׾�����ֵ
	double m_fProSH;	//�崲ʽ����׾�����ֵ
};
struct FILTER_MK_PARA
{
	CXhChar50 m_sThickRange;
	BOOL m_bFileterS;	//Q235
	BOOL m_bFileterH;	//Q345
	BOOL m_bFileterh;	//Q355
	BOOL m_bFileterG;	//Q390
	BOOL m_bFileterP;	//Q420
	BOOL m_bFileterT;	//Q460
	//
	FILTER_MK_PARA();
	//
	CString GetParaDesc();
	void ParseParaDesc(CString sDesc);
};
struct NC_INFO_PARA
{
	CXhChar100 m_sThick;
	DWORD m_dwFileFlag;
	HOLE_INCREMENT m_xHoleIncrement;
	//���²���Ŀǰֻ���ڿ��Ƽ��⸴�ϻ�DXF�ļ���� wht 19-10-22
	BOOL m_bOutputBendLine;
	BOOL m_bOutputBendType;
	BOOL m_bExplodeText;
	//�и�ӹ�����
	BOOL m_bCutSpecialHole;
	WORD m_wEnlargedSpace;
	BOOL m_bGrindingArc;	//���Ǵ�ĥ����
	//�崲�ӹ�����
	BOOL m_bReserveBigSH;	//����и��
	BOOL m_bReduceSmallSH;	//��������崲��
	BYTE m_ciHoldSortType;	//��˨������0.����|1.����+����|2.����+�׾�
	BOOL m_bSortHasBigSH;	//�и����Ƿ��������
public:
	NC_INFO_PARA();
	//
	DWORD AddFileFlag(DWORD dwFlag);
	bool IsValidFile(DWORD dwFlag);
};
class CSysPara
{
public:
	//DockPageͣ������
	struct DOCKPAGEPOS
	{
		BOOL bDisplay;
		UINT uDockBarID;
	};
	struct DOCK_ENV{
		DOCKPAGEPOS pagePartList,pageProp;
		short m_nLeftPageWidth,m_nRightPageWidth;
	}dock;
	//
	HOLE_INCREMENT holeIncrement;
	//��ӡ���˲���
	ATOM_LIST<FILTER_MK_PARA> filterMKParaList;
	//NC���ݲ���
	struct NC_PARA{
		//�ְ�NC����
		double m_fBaffleHigh;	//����߶�
		BOOL m_bDispMkRect;		//��ʾ�ֺ�
		BYTE m_ciMkVect;		//�ֺг��� 0.����ԭ����|1.����ˮƽ
		double m_fMKHoleD,m_fMKRectW,m_fMKRectL;
		BYTE m_iNcMode;			//����|������|�崲|�괲|����ӹ�
		double m_fLimitSH;		//����׾�����ֵ
		BOOL m_iDxfMode;		//DXF�ļ����� 0.������ 1.�����
		BYTE m_ciDisplayType;	//��ǰ��ʾ�ӹ�ģʽ
		//�ְ��������
		NC_INFO_PARA m_xFlamePara;
		NC_INFO_PARA m_xPlasmaPara;
		NC_INFO_PARA m_xPunchPara;
		NC_INFO_PARA m_xDrillPara;
		NC_INFO_PARA m_xLaserPara;
		//�Ǹ�NC����
		CString m_sNcDriverPath;
		//�ְ������ļ�·��
		CString m_sPlateConfigFilePath;
	}nc;
	struct PBJ_PARA{
		BOOL m_bIncVertex;	//���������
		BOOL m_bAutoSplitFile;	//��˨�����೬�����ֺ��Զ�������ɶ���ļ�
		BOOL m_bMergeHole;	//���������ٵı�׼�׺ϲ�ʹ��ͬһ��ͷ�ӹ������Ǳ�װ���׼�׼ӹ� wht 19-07-25
	}pbj;
	struct PMZ_PARA {
		int m_iPmzMode;		//�ļ�ģʽ��0.���ļ� 1.���ļ�
		BOOL m_bIncVertex;	//���������
		BOOL m_bPmzCheck;	//���Ԥ��PMZ��ʽ
	}pmz;
	//�����и�
	struct FLAME_CUT_PARA{
		CXhChar16 m_sOutLineLen;
		CXhChar16 m_sIntoLineLen;
		BOOL m_bInitPosFarOrg;
		BOOL m_bCutPosInInitPos;
	}flameCut,plasmaCut;
	//��ɫ����
	struct COLOR_MODE{
		COLORREF crLS12;
		COLORREF crLS16;
		COLORREF crLS20;
		COLORREF crLS24;
		COLORREF crOtherLS;
		COLORREF crMark;
		COLORREF crEdge;
		COLORREF crHuoQu;
		COLORREF crText;
	}crMode;
	//�Ǹֹ��տ�����
	struct JGDRAWING_PARA{
		int iDimPrecision;			//�ߴ羫ȷ��
		double fRealToDraw;			//������ͼ�����ߣ�ʵ�ʳߴ�/��ͼ�ߴ磬��1:20ʱ��fRealToDraw=20
		double	fDimArrowSize;		//�ߴ��ע��ͷ��
		double fTextXFactor;
		int		iPartNoFrameStyle;	//��ſ����� //0.ԲȦ 1.��Բ���ľ��ο� 2.���ο�	3.�Զ��ж�
		double	fPartNoMargin;		//����������ſ�֮��ļ�϶ֵ 
		double	fPartNoCirD;		//�������Ȧֱ��
		double fPartGuigeTextSize;	//����������ָ�
		int iMatCharPosType;
		BOOL bModulateLongJg;		//�����Ǹֳ��� ��δʹ�ã���iJgZoomSchema����ñ��� wht 11-05-07
		int iJgZoomSchema;			//�Ǹֻ��Ʒ�����0.1:1���� 1.ʹ�ù���ͼ���� 2.����ͬ������ 3.����ֱ�����
		BOOL bMaxExtendAngleLength;	//����޶�����Ǹֻ��Ƴ���
		double fLsDistThreshold;	//�Ǹֳ����Զ�������˨�����ֵ(���ڴ˼��ʱ��Ҫ���е���);
		double fLsDistZoomCoef;		//��˨�������ϵ��
		BOOL bOneCardMultiPart;		//�Ǹ�����һ��������
		int  iJgGDimStyle;			//0.ʼ�˱�ע  1.�м��ע 2.�Զ��ж�
		int  nMaxBoltNumStartDimG;	//������ʼ�˱�ע׼��֧�ֵ������˨��
		int  iLsSpaceDimStyle;		//0.X�᷽��	  1.Y�᷽��  2.�Զ��ж� 3.����ע  4.�ޱ�ע����(X�᷽��)  4.����ߴ��ߣ��ޱ�ע����(X�᷽��)��Ҫ���ڽ���(��)�����ʼ���ä������
		int  nMaxBoltNumAlongX;		//��X�᷽���ע֧�ֵ������˨����
		BOOL bDimCutAngle;			//��ע�Ǹ��н�
		BOOL bDimCutAngleMap;		//��ע�Ǹ��н�ʾ��ͼ
		BOOL bDimPushFlatMap;		//��עѹ��ʾ��ͼ
		BOOL bJgUseSimpleLsMap;		//�Ǹ�ʹ�ü���˨ͼ��
		BOOL bDimLsAbsoluteDist;	//��ע��˨���Գߴ�
		BOOL bMergeLsAbsoluteDist;	//�ϲ����ڵȾ���˨���Գߴ� �����������:��ʱҲ��Ҫ�� wjh-2014.6.9
		BOOL bDimRibPlatePartNo;	//��ע�Ǹ��߰���
		BOOL bDimRibPlateSetUpPos;	//��ע�Ǹ��߰尲װλ��
		//�нǱ�ע��ʽһ
		//�нǱ�ע��ʽ�� B:��ͷ�ߴ� L:֫�߳ߴ� C:��߳ߴ� 
		//BXL �н�  CXL ��֫ BXC �н�  �д��=�н�+��֫
		int	 iCutAngleDimType;		//�нǱ�ע��ʽ 0.��ʽһ  1.��ʽ�� wht 10-11-01
		//
		BOOL bDimKaiHe;				//��ע�Ǹֿ��Ͻ�
		BOOL bDimKaiheAngleMap;		//��ע�Ǹֿ��Ͻ�ʾ��ͼ
		double fKaiHeJiaoThreshold; //���ϽǱ�ע��ֵ(��) wht 11-05-06
		//�������ϽǱ�ע���� wht 12-03-13
		BOOL bDimKaiheSumLen;		//��ע���������ܳ�
		BOOL bDimKaiheAngle;		//��ע���϶���	
		BOOL bDimKaiheSegLen;		//��ע��������ֶγ�
		BOOL bDimKaiheScopeMap;		//��ע���������ʶ��
		//
		CString sAngleCardPath;
	}jgDrawing;
	struct FONT{
		double  fTextHeight;		//��ͨ��������߶�
		double	fDimTextSize;		//���ȳߴ��ע�ı���
		double	fPartNoTextSize;	//����������ָ�
		double  fDxfTextSize;		//DXF�ļ�����������߶�
	}font;
public:
	CSysPara(void);
	~CSysPara(void);
	//
	BOOL Read(CString file_path);	//�������ļ�
	BOOL Write(CString file_path);	//д�����ļ�
	void WriteSysParaToReg(LPCTSTR lpszEntry);	//���湲�ò�����ע���
	void ReadSysParaFromReg(LPCTSTR lpszEntry);	//��ȡ���ò�����ע���
	void UpdateHoleIncrement(double fHoleInc);
	BOOL IsFilterMK(int nThick, char cMat);
	//
	void AngleDrawingParaToBuffer(CBuffer &buffer);
	//�ְ�NCģʽ����
	bool IsValidNcFlag(BYTE ciNcFlag) {
		if ((ciNcFlag&nc.m_iNcMode) > 0)
			return true;
		else
			return false;
	}
	DWORD AddNcFlag(BYTE ciNcFlag) {
		nc.m_iNcMode |= ciNcFlag;
		return nc.m_iNcMode;
	}
	DWORD DelNcFlag(BYTE ciNcFlag) {
		BYTE ciCode = 0xff - ciNcFlag;
		nc.m_iNcMode &= ciCode;
		return nc.m_iNcMode;
	}
	//�ְ����ʾģʽ����
	bool IsValidDisplayFlag(BYTE ciNcFlag){
		if (ciNcFlag&nc.m_ciDisplayType)
			return true;
		else
			return false;
	}
	//����������
	DECLARE_PROP_FUNC(CSysPara);
	int GetPropValueStr(long id, char *valueStr, UINT nMaxStrBufLen = 100);
};
//
struct PPE_LOCALE : public XHLOCALE
{
	PPE_LOCALE();
public:
	virtual void InitCustomerSerial(UINT uiSerial);
};

extern PPE_LOCALE gxLocalizer;
extern CSysPara g_sysPara;
