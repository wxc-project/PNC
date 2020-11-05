#include "stdafx.h"
#include "SysPara.h"
#include "Expression.h"
#include "NcPart.h"
#include "ParseAdaptNo.h"
#ifndef _LEGACY_LICENSE
#include "XhLdsLm.h"
#else
#include "Lic.h"
#endif

//////////////////////////////////////////////////////////////////////////
//FILTER_MK_PARA
FILTER_MK_PARA::FILTER_MK_PARA()
{
	m_bFileterS = FALSE;
	m_bFileterH = FALSE;
	m_bFileterh = FALSE;
	m_bFileterG = FALSE;
	m_bFileterP = FALSE;
	m_bFileterT = FALSE;
}
CString FILTER_MK_PARA::GetParaDesc()
{
	CString sDesc = m_sThickRange;
	sDesc += ";";
	sDesc += m_bFileterS ? "1" : "0";
	sDesc += ";";
	sDesc += m_bFileterH ? "1" : "0";
	sDesc += ";";
	sDesc += m_bFileterh ? "1" : "0";
	sDesc += ";";
	sDesc += m_bFileterG ? "1" : "0";
	sDesc += ";";
	sDesc += m_bFileterP ? "1" : "0";
	sDesc += ";";
	sDesc += m_bFileterT ? "1" : "0";
	return sDesc;
}
void FILTER_MK_PARA::ParseParaDesc(CString sDesc)
{
	CXhChar100 sTxt(sDesc);
	char *skey = strtok((char*)sTxt, ";");
	m_sThickRange = skey;
	m_sThickRange.Replace(" ", "");
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterS = atoi(skey);
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterH = atoi(skey);
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterh = atoi(skey);
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterG = atoi(skey);
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterP = atoi(skey);
	skey = strtok(NULL, ";");
	if (skey)
		m_bFileterT = atoi(skey);
}
//////////////////////////////////////////////////////////////////////////
//FILE_INFO_PARA
NC_INFO_PARA::NC_INFO_PARA()
{
	m_sThick.Copy("*"); 
	m_dwFileFlag = CNCPart::PLATE_DXF_FILE;
	m_bCutSpecialHole = TRUE;
	m_wEnlargedSpace = 0;
	m_bGrindingArc = FALSE;
	m_bReserveBigSH = FALSE;
	m_bReduceSmallSH = TRUE;
	m_ciHoldSortType = 0;
	m_bSortHasBigSH = FALSE;
	m_bOutputBendLine = m_bOutputBendType = FALSE;
	m_bExplodeText = FALSE;
}
DWORD NC_INFO_PARA::AddFileFlag(DWORD dwFlag)
{
	m_dwFileFlag |= dwFlag;
	return m_dwFileFlag;
}
bool NC_INFO_PARA::IsValidFile(DWORD dwFlag) {
	if ((dwFlag&m_dwFileFlag) > 0)
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
//CSysPara
IMPLEMENT_PROP_FUNC(CSysPara);

const int HASHTABLESIZE=500;
const int STATUSHASHTABLESIZE=500;
int CSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//
	AddPropItem("Model",PROPLIST_ITEM(id++,"�����Ϣ"));
	AddPropItem("model.m_sCompanyName",PROPLIST_ITEM(id++,"��Ƶ�λ"));
	AddPropItem("model.m_sPrjCode",PROPLIST_ITEM(id++,"���̱��"));
	AddPropItem("model.m_sPrjName",PROPLIST_ITEM(id++,"��������"));
	AddPropItem("model.m_sTaType",PROPLIST_ITEM(id++,"����"));
	AddPropItem("model.m_sTaAlias",PROPLIST_ITEM(id++,"����"));
	AddPropItem("model.m_sTaStampNo",PROPLIST_ITEM(id++,"��ӡ��"));
	AddPropItem("model.m_sOperator",PROPLIST_ITEM(id++,"����Ա"));
	AddPropItem("model.m_sAuditor",PROPLIST_ITEM(id++,"�����"));
	AddPropItem("model.m_sCritic",PROPLIST_ITEM(id++,"������"));
	//
	AddPropItem("NC",PROPLIST_ITEM(id++,"NC����"));
	AddPropItem("nc.m_fMKHoleD",PROPLIST_ITEM(id++,"��λ��ֱ��"));
	AddPropItem("nc.m_bDispMkRect",PROPLIST_ITEM(id++,"��ʾ�ֺ�","��ʾ�ֺ�","��|��"));
	AddPropItem("nc.m_ciMkVect", PROPLIST_ITEM(id++, "�ֺг���", "�ֺг���", "0.����ԭ����|1.����ˮƽ"));
	AddPropItem("nc.m_fMKRectW",PROPLIST_ITEM(id++,"�ֺп��","�ֺо��ο��"));
	AddPropItem("nc.m_fMKRectL",PROPLIST_ITEM(id++,"�ֺг���","�ֺо��γ���"));
	AddPropItem("nc.m_fBaffleHigh",PROPLIST_ITEM(id++,"����߶�"));
	AddPropItem("nc.m_iNcMode",PROPLIST_ITEM(id++,"�ְ�NCģʽ","���ɸְ�DXF�ļ���ģʽ","�и�|�崲|����|�и�+�崲|�и�+�崲+����"));
	AddPropItem("nc.m_fLimitSH",PROPLIST_ITEM(id++,"����׽���ֵ","�����һ��ͨ���и���мӹ�"));
	AddPropItem("CutLimitSH", PROPLIST_ITEM(id++, "�и�ʽ�����(���)", "ͨ���и�ս��мӹ��������"));
	AddPropItem("ProLimitSH", PROPLIST_ITEM(id++, "�崲ʽ�����(С��)", "ͨ���崲���ս��мӹ��������"));
	AddPropItem("nc.m_sNcDriverPath",PROPLIST_ITEM(id++,"�Ǹ�NC����"));
	AddPropItem("nc.m_ciDisplayType", PROPLIST_ITEM(id++, "��ǰ��ʾģʽ", "��ǰ��ʾ�ļӹ�ģʽ", "ԭʼ|����|������|�崲|�괲|����|����ģʽ"));
	AddPropItem("FileterMkSet", PROPLIST_ITEM(id++, "��ӡ��������"));
	//�ļ�����
	AddPropItem("FileSet", PROPLIST_ITEM(id++, "�ļ�����"));
	AddPropItem("FileFormat", PROPLIST_ITEM(id++, "����ļ���ʽ"));
	AddPropItem("OutputPath", PROPLIST_ITEM(id++, "����ļ�·��"));
	AddPropItem("PbjPara",PROPLIST_ITEM(id++,"PBJ����"));
	AddPropItem("pbj.m_bIncVertex",PROPLIST_ITEM(id++,"�������","����PBJʱ���Ƿ��������","��|��"));
	AddPropItem("pbj.m_bAutoSplitFile",PROPLIST_ITEM(id++,"�Զ����","��˨ֱ���������3ʱ���Զ����Ϊ���pbj�ļ�","��|��"));
	AddPropItem("pbj.m_bMergeHole", PROPLIST_ITEM(id++, "�ϲ��ӹ�", "�Ǳ�װ���׼�׼ӹ�(�磺19.5��17.5�̹�)��3�ֿ׾�ʱ�������ٵ�һ�ֺϲ������ڿ׾��ӹ�����������������", "��|��"));
	AddPropItem("PmzPara", PROPLIST_ITEM(id++, "PMZ����"));
	AddPropItem("pmz.m_iPmzMode", PROPLIST_ITEM(id++, "�ļ�ģʽ", "0.���ļ�,���п���һ���ļ��У�1.���ļ���һ������һ���ļ�", "0.���ļ�|1.���ļ�"));
	AddPropItem("pmz.m_bIncVertex", PROPLIST_ITEM(id++, "�������", "����PBJʱ���Ƿ��������", "��|��"));
	AddPropItem("pmz.m_bPmzCheck", PROPLIST_ITEM(id++, "���Ԥ��Pmz", "�������Ԥ���Pmz�ļ���һ���ְ���ȡ3~4������Ϊ��λ��,���ڼӹ�ǰԤ��", "��|��"));
	//�������и���ʾ
	AddPropItem("plasmaCut",PROPLIST_ITEM(id++,"�������и�����"));
	AddPropItem("plasmaCut.m_sOutLineLen",PROPLIST_ITEM(id++,"�����߳�","�����߳�","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("plasmaCut.m_sIntoLineLen",PROPLIST_ITEM(id++,"�����߳�","�����߳�","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("plasmaCut.m_bInitPosFarOrg",PROPLIST_ITEM(id++,"��ʼ��λ��","��ʼ��λ��","0.����ԭ��|1.Զ��ԭ��"));
	AddPropItem("plasmaCut.m_bCutPosInInitPos",PROPLIST_ITEM(id++,"����㶨λ","�����λ��","0.��ָ��������|1.ʼ���ڳ�ʼ��"));
	AddPropItem("plasmaCut.m_bCutSpecialHole", PROPLIST_ITEM(id++, "�и��ſ�", "", "��|��"));
	AddPropItem("plasmaCut.m_bGrindingArc", PROPLIST_ITEM(id++, "���Ǵ�ĥԲ��", "", "��|��"));
	//�����и���ʾ
	AddPropItem("flameCut",PROPLIST_ITEM(id++,"�����и�����"));
	AddPropItem("flameCut.m_sOutLineLen",PROPLIST_ITEM(id++,"�����߳�","�����߳�","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("flameCut.m_sIntoLineLen",PROPLIST_ITEM(id++,"�����߳�","�����߳�","(1/3)*T|(1/2)*T|(2/3)*T|T"));
	AddPropItem("flameCut.m_bInitPosFarOrg",PROPLIST_ITEM(id++,"��ʼ��λ��","��ʼ��","0.����ԭ��|1.Զ��ԭ��"));
	AddPropItem("flameCut.m_bCutPosInInitPos",PROPLIST_ITEM(id++,"����㶨λ","�����λ��","0.��ָ��������|1.ʼ���ڳ�ʼ��"));
	AddPropItem("flameCut.m_bCutSpecialHole", PROPLIST_ITEM(id++, "�и��ſ�", "����и�����ʱ���������и�·��", "��|��"));
	AddPropItem("flameCut.m_bGrindingArc", PROPLIST_ITEM(id++, "���Ǵ�ĥԲ��", "", "��|��"));
	//�������
	AddPropItem("OutPutSet", PROPLIST_ITEM(id++, "�������"));
	AddPropItem("nc.m_iDxfMode", PROPLIST_ITEM(id++, "����ȷ���", "����DXF�Ƿ���з���", "��|��"));
	//�и�����
	AddPropItem("nc.bFlameCut", PROPLIST_ITEM(id++, "�����и�", "", "����|����"));
	AddPropItem("nc.FlamePara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "����������ֵ", "����������ֵ"));
	AddPropItem("nc.FlamePara.m_xHoleIncrement", PROPLIST_ITEM(id++, "�׾�����ֵ", "", ""));
	AddPropItem("nc.FlamePara.m_sThick", PROPLIST_ITEM(id++, "���Χ", "*���к��,a�������,b-c�������"));
	AddPropItem("nc.FlamePara.m_dwFileFlag", PROPLIST_ITEM(id++, "�����ļ�", "", "DXF|TXT|CNC|DXF+TXT|DXF+CNC|DXF+TXT+CNC"));
	AddPropItem("nc.bPlasmaCut", PROPLIST_ITEM(id++, "�������и�", "", "����|����"));
	AddPropItem("nc.PlasmaPara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "����������ֵ", "����������ֵ"));
	AddPropItem("nc.PlasmaPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "�׾�����ֵ", "", ""));
	AddPropItem("nc.PlasmaPara.m_sThick", PROPLIST_ITEM(id++, "���Χ", "*���к��,a�������,b-c�������"));
	AddPropItem("nc.PlasmaPara.m_dwFileFlag", PROPLIST_ITEM(id++, "�����ļ�", "", "DXF|NC|DXF+NC"));
	//�崲�ӹ�
	AddPropItem("nc.bPunchPress", PROPLIST_ITEM(id++, "�崲�ӹ�", "", "����|����"));
	AddPropItem("nc.PunchPara.m_bReserveBigSH", PROPLIST_ITEM(id++, "����и��ſ�", "�Ƿ�����������", "��|��"));
	AddPropItem("nc.PunchPara.m_bReduceSmallSH", PROPLIST_ITEM(id++, "��������С�ſ�", "��������С�ſ�", "��|��"));
	AddPropItem("nc.PunchPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "�׾�����ֵ", "", ""));
	AddPropItem("nc.PunchPara.m_ciHoldSortType", PROPLIST_ITEM(id++, "Բ�����򷽰�", "", "0.����|1.����+����|2.����+�׾�"));
	AddPropItem("nc.PunchPara.m_bSortHasBigSH", PROPLIST_ITEM(id++, "�и�ײ�������", "Բ������ʱ�Ƿ����и��������", "��|��"));
	AddPropItem("nc.PunchPara.m_sThick", PROPLIST_ITEM(id++, "���Χ","*���к��,a�������,b-c�������"));
	AddPropItem("nc.PunchPara.m_dwFileFlag", PROPLIST_ITEM(id++, "�����ļ�", "", "DXF|PBJ|WKF|TTP|DXF+PBJ|DXF+WKF|DXF+TTP|DXF+PBJ+WKF"));
	AddPropItem("nc.bDrillPress", PROPLIST_ITEM(id++, "�괲�ӹ�", "", "����|����"));
	AddPropItem("nc.DrillPara.m_bReserveBigSH", PROPLIST_ITEM(id++, "����и������", "�Ƿ��������", "��|��"));
	AddPropItem("nc.DrillPara.m_bReduceSmallSH", PROPLIST_ITEM(id++, "��������С�ſ�", "��������С�ſ�", "��|��"));
	AddPropItem("nc.DrillPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "�׾�����ֵ", "", ""));
	AddPropItem("nc.DrillPara.m_ciHoldSortType", PROPLIST_ITEM(id++, "Բ�����򷽰�", "", "0.����|1.����+����|2.����+�׾�"));
	AddPropItem("nc.DrillPara.m_bSortHasBigSH", PROPLIST_ITEM(id++, "�и�ײ�������", "Բ������ʱ�Ƿ����и��������", "��|��"));
	AddPropItem("nc.DrillPara.m_sThick", PROPLIST_ITEM(id++, "���Χ", "*���к��,a�������,b-c�������"));
	AddPropItem("nc.DrillPara.m_dwFileFlag", PROPLIST_ITEM(id++, "�����ļ�", "", "DXF|PMZ|DXF+PMZ"));
	//����ӹ�
	AddPropItem("nc.bLaser", PROPLIST_ITEM(id++, "���⸴�ϻ�", "", "����|����"));
	AddPropItem("nc.LaserPara.m_wEnlargedSpace", PROPLIST_ITEM(id++, "����������ֵ", "����������ֵ"));
	AddPropItem("nc.LaserPara.m_xHoleIncrement", PROPLIST_ITEM(id++, "�׾�����ֵ", "", ""));
	AddPropItem("nc.LaserPara.m_sThick", PROPLIST_ITEM(id++, "���Χ", "*���к��,a�������,b-c�������"));
	AddPropItem("nc.LaserPara.Config", PROPLIST_ITEM(id++, "������ϸ"));
	AddPropItem("nc.LaserPara.m_dwFileFlag", PROPLIST_ITEM(id++, "�����ļ�"));
	AddPropItem("nc.LaserPara.m_bOutputBendLine", PROPLIST_ITEM(id++, "���������", "", "��|��"));
	AddPropItem("nc.LaserPara.m_bOutputBendType", PROPLIST_ITEM(id++, "�����������", "", "��|��"));
	AddPropItem("nc.LaserPara.m_bExplodeText", PROPLIST_ITEM(id++, "�������", "", "��|��"));
	//
	AddPropItem("holeIncrement.m_fDatum",PROPLIST_ITEM(id++,"�׾�����ֵ"));
	AddPropItem("holeIncrement.m_fM12",PROPLIST_ITEM(id++,"M12��׼����"));
	AddPropItem("holeIncrement.m_fM16",PROPLIST_ITEM(id++,"M16��׼����"));
	AddPropItem("holeIncrement.m_fM20",PROPLIST_ITEM(id++,"M20��׼����"));
	AddPropItem("holeIncrement.m_fM24",PROPLIST_ITEM(id++,"M24��׼����"));
	AddPropItem("holeIncrement.m_fCutSH", PROPLIST_ITEM(id++, "�и����������"));
	AddPropItem("holeIncrement.m_fProSH", PROPLIST_ITEM(id++, "�崲���������"));
	//��ɫ���÷���
	AddPropItem("CRMODE",PROPLIST_ITEM(id++,"��ɫ����"));
	AddPropItem("crMode.crLS12",PROPLIST_ITEM(id++,"��˨M12","M12�׾���ɫ"));
	AddPropItem("crMode.crLS16",PROPLIST_ITEM(id++,"��˨M16","M16�׾���ɫ"));
	AddPropItem("crMode.crLS20",PROPLIST_ITEM(id++,"��˨M20","M20�׾���ɫ"));
	AddPropItem("crMode.crLS24",PROPLIST_ITEM(id++,"��˨M24","M24�׾���ɫ"));
	AddPropItem("crMode.crOtherLS",PROPLIST_ITEM(id++,"������˨","�����׾���ɫ"));
	AddPropItem("crMode.crMarK",PROPLIST_ITEM(id++,"��ӡ��","��ӡ����ɫ"));
	AddPropItem("crMode.crEdge", PROPLIST_ITEM(id++, "������", "�����ߵ���ɫ"));
	AddPropItem("crMode.crHuoQu", PROPLIST_ITEM(id++, "������", "�����ߵ���ɫ"));
	AddPropItem("crMode.crText", PROPLIST_ITEM(id++, "��ʾ�ı�", "�ı���ɫ"));
	//
	AddPropItem("JgDrawing",PROPLIST_ITEM(id++,"���տ�"));
	AddPropItem("jgDrawing.fDimTextSize",PROPLIST_ITEM(id++,"����߶�"));
	AddPropItem("jgDrawing.iDimPrecision",PROPLIST_ITEM(id++,"��ȷ��","","1.0mm|0.5mm|0.1mm|"));
	AddPropItem("jgDrawing.fRealToDraw",PROPLIST_ITEM(id++,"��ͼ����"));	
	AddPropItem("jgDrawing.fDimArrowSize",PROPLIST_ITEM(id++,"��ע��ͷ�ߴ�"));
	AddPropItem("jgDrawing.fTextXFactor",PROPLIST_ITEM(id++,"�����߱�"));
	AddPropItem("jgDrawing.fPartNoTextSize",PROPLIST_ITEM(id++,"�����ָ�"));
	AddPropItem("jgDrawing.iPartNoFrameStyle",PROPLIST_ITEM(id++,"���ű߿�����",
		"ѡ���Զ��ж�ʱ����ų����Զ��жϱ߿�����,��ѡΪԲȦ,��Ź���ʱʹ����Բ����","0.ԲȦ|1.��Բ����|2.��ͨ����|3.�Զ��ж�"));
	AddPropItem("jgDrawing.fPartNoMargin",PROPLIST_ITEM(id++,"���ż�϶"));
	AddPropItem("jgDrawing.fPartNoCirD",PROPLIST_ITEM(id++,"����Ȧֱ��"));
	AddPropItem("jgDrawing.fPartGuigeTextSize",PROPLIST_ITEM(id++,"����ָ�"));
	AddPropItem("jgDrawing.iMatCharPosType",PROPLIST_ITEM(id++,"�����ַ�λ��",
		"������Ų����ַ�λ��","0.����Ҫ�����ַ�|1.�������ǰ|2.������ź�"));
	//�Ǹֹ���ͼ����
	AddPropItem("jgDrawing.fLsDistThreshold",PROPLIST_ITEM(id++,"�Ǹֳ����Զ�������˨�����ֵ"));
	AddPropItem("jgDrawing.fLsDistZoomCoef",PROPLIST_ITEM(id++,"��˨�������ϵ��"));
	AddPropItem("jgDrawing.bOneCardMultiPart",PROPLIST_ITEM(id++,"һ�����","","��|��"));
	AddPropItem("jgDrawing.bModulateLongJg",PROPLIST_ITEM(id++,"�����Ǹֳ���","","��|��"));	
	AddPropItem("jgDrawing.iJgZoomSchema",PROPLIST_ITEM(id++,"�Ǹ����ŷ���",
		"2.����ͬ�����ż�3.����ֱ����ž�Ϊ�����տ���ͼ�����Զ��������ű���,����ֱ�����ʱ�������������ͼ����",
		"0.������1:1����|1.ʹ�ù���ͼ����|2.����ͬ������|3.����ֱ�����"));
	AddPropItem("jgDrawing.bMaxExtendAngleLength",PROPLIST_ITEM(id++,"����޶�����Ǹֻ��Ƴ���",
		"�Ƿ��ڽǸֹ��տ���ͼ��������޶�����Ǹֻ��Ƴ��ȡ�","��|��"));
	AddPropItem("jgDrawing.iJgGDimStyle",PROPLIST_ITEM(id++,"�Ǹ��ľ��עλ��","","0.ʼ��|1.�м�|2.�Զ��ж�"));
	AddPropItem("jgDrawing.nMaxBoltNumStartDimG",PROPLIST_ITEM(id++,"�����˨��"));	
	//����ߴ���ѡ��Ϊ���콭���������Ҫԭ���Ƿ����պ��ʼ���Աä�죩 wjh-2017.11.28
	AddPropItem("jgDrawing.iLsSpaceDimStyle",PROPLIST_ITEM(id++,"��˨����ע��ʽ","","0.��X�᷽��|1.��Y�᷽��|2.�Զ��ж�|3.����ע���|4.����ߴ���"));
	AddPropItem("jgDrawing.nMaxBoltNumAlongX",PROPLIST_ITEM(id++,"�����˨��"));
	AddPropItem("jgDrawing.bDimCutAngle",PROPLIST_ITEM(id++,"��ע�н�","","��|��"));
	AddPropItem("jgDrawing.bDimCutAngleMap",PROPLIST_ITEM(id++,"��ע�н�ʾ��ͼ","","��|��"));
	AddPropItem("jgDrawing.bDimPushFlatMap",PROPLIST_ITEM(id++,"��עѹ��ʾ��ͼ","","��|��"));
	AddPropItem("jgDrawing.bDimKaiHe",PROPLIST_ITEM(id++,"��ע�Ǹֿ��Ͻ�","","��|��"));
	AddPropItem("jgDrawing.bDimKaiheAngleMap",PROPLIST_ITEM(id++,"��ע�Ǹֿ��Ͻ�ʾ��ͼ","","��|��"));
	AddPropItem("jgDrawing.bDimKaiheSumLen",PROPLIST_ITEM(id++,"��ע�Ǹֿ��������ܳ�","","��|��"));
	AddPropItem("jgDrawing.bDimKaiheAngle",PROPLIST_ITEM(id++,"��ע�Ǹֿ��ϽǶ���","","��|��"));	
	AddPropItem("jgDrawing.bDimKaiheSegLen",PROPLIST_ITEM(id++,"��ע�Ǹֿ�������ֶγ�","","��|��"));
	AddPropItem("jgDrawing.bDimKaiheScopeMap",PROPLIST_ITEM(id++,"��ע���������ʶ��","","��|��"));
	AddPropItem("jgDrawing.bJgUseSimpleLsMap",PROPLIST_ITEM(id++,"�Ǹ�ʹ�ü���˨ͼ��","","��|��"));
	AddPropItem("jgDrawing.bDimLsAbsoluteDist",PROPLIST_ITEM(id++,"��ע��˨���Գߴ�","","��|��"));
	AddPropItem("jgDrawing.bMergeLsAbsoluteDist",PROPLIST_ITEM(id++,"�ϲ����ڵȾ���˨���Գߴ�","","��|��"));
	AddPropItem("jgDrawing.bDimRibPlatePartNo",PROPLIST_ITEM(id++,"��ע�Ǹ��߰���","","��|��"));
	AddPropItem("jgDrawing.bDimRibPlateSetUpPos",PROPLIST_ITEM(id++,"��ע�Ǹ��߰尲װλ��","","��|��"));	
	AddPropItem("jgDrawing.iCutAngleDimType",PROPLIST_ITEM(id++,"�Ǹ��нǱ�ע��ʽ",
		"B:��ͷ�ߴ� L:֫�߳ߴ� C:��߳ߴ�\r\n��ʽһ:�н�=>CT LXB,��֫=>BC LXB,����֫=>CF L;\r\n��ʽ��:�н�=>BXL ��֫=>CXL �н�=>BXC(�д��=�н�+��֫)",
		"0.��ʽһ|1.��ʽ��"));
	AddPropItem("jgDrawing.fKaiHeJiaoThreshold",PROPLIST_ITEM(id++,"���ϽǱ�ע��ֵ(��)",
		"�Ǹ���֫�н���90���ƫ��ֵ���ڸ���ֵʱ��Ϊ��Ҫ���п��ϽǱ�ע��"));
	AddPropItem("jgDrawing.sAngleCardPath",PROPLIST_ITEM(id++,"�Ǹֹ��տ�"));
	AddPropItem("Font",PROPLIST_ITEM(id++,"��������"));
	AddPropItem("font.fTextHeight",PROPLIST_ITEM(id++,"��ʾ�����ָ�"));
	AddPropItem("font.fDxfTextSize", PROPLIST_ITEM(id++, "DXF�����ָ�"));
	AddPropItem("font.fDimTextSize",PROPLIST_ITEM(id++,"�ߴ��ע�ָ�"));
	AddPropItem("font.fPartNoTextSize",PROPLIST_ITEM(id++,"��������ָ�"));
	return id;
}
CSysPara::CSysPara(void)
{
	dock.pagePartList.bDisplay=TRUE;
	dock.pagePartList.uDockBarID=AFX_IDW_DOCKBAR_RIGHT;
	dock.pageProp.bDisplay=TRUE;
	dock.pageProp.uDockBarID=AFX_IDW_DOCKBAR_LEFT;
	dock.m_nLeftPageWidth=270;
	dock.m_nRightPageWidth=250;

	nc.m_bDispMkRect=FALSE;
	nc.m_fMKHoleD = 10;
	nc.m_fMKRectW = 30;
	nc.m_fMKRectL = 60;
	nc.m_fBaffleHigh = 260;
	nc.m_iNcMode = CNCPart::FLAME_MODE;
	nc.m_fLimitSH=25;
	nc.m_iDxfMode=0;
	nc.m_sNcDriverPath.Empty();
	nc.m_ciDisplayType=CNCPart::PUNCH_MODE;
	//
	filterMKParaList.Empty();
	//
	pbj.m_bAutoSplitFile=FALSE;
	pbj.m_bIncVertex=TRUE;
	pbj.m_bMergeHole = FALSE;
	//
	pmz.m_iPmzMode = 1;
	pmz.m_bIncVertex = FALSE;
	pmz.m_bPmzCheck = TRUE;
	//�������и�
	plasmaCut.m_sOutLineLen.Copy("4");
	plasmaCut.m_sIntoLineLen.Copy("4");
	plasmaCut.m_bCutPosInInitPos=TRUE;
	plasmaCut.m_bInitPosFarOrg=TRUE;
	//�����и�
	flameCut.m_sOutLineLen.Copy("4");
	flameCut.m_sIntoLineLen.Copy("4");
	flameCut.m_bCutPosInInitPos=TRUE;
	flameCut.m_bInitPosFarOrg=TRUE;
	//
	holeIncrement.m_fDatum=1.5;
	holeIncrement.m_fM12=1.5;
	holeIncrement.m_fM16=1.5;
	holeIncrement.m_fM20=1.5;
	holeIncrement.m_fM24=1.5;
	holeIncrement.m_fCutSH=0;
	holeIncrement.m_fProSH = 0;
	holeIncrement.m_fWaist = 0;
	nc.m_xFlamePara.m_xHoleIncrement = holeIncrement;
	nc.m_xPlasmaPara.m_xHoleIncrement = holeIncrement;
	nc.m_xPunchPara.m_xHoleIncrement = holeIncrement;
	nc.m_xDrillPara.m_xHoleIncrement = holeIncrement;
	nc.m_xLaserPara.m_xHoleIncrement = holeIncrement;
	//��ɫ����
	crMode.crLS12 = RGB(0, 255, 255);
	crMode.crLS16 = RGB(0, 255, 0);
	crMode.crLS20 = RGB(255, 0, 255);
	crMode.crLS24 = RGB(255, 165, 79);
	crMode.crOtherLS = RGB(128, 0, 64);
	crMode.crEdge = RGB(0, 0, 0);
	crMode.crHuoQu = RGB(128, 0, 255);
	crMode.crText = RGB(255, 0, 0);
	crMode.crMark = RGB(0, 0, 255);
	//�Ǹֹ���ͼ		
	jgDrawing.iDimPrecision =0;			
	jgDrawing.fRealToDraw=10;			
	jgDrawing.fDimArrowSize=1.5;		
	jgDrawing.fTextXFactor=0.7; 
	jgDrawing.iPartNoFrameStyle=3;	
	jgDrawing.fPartNoMargin=0.3;
	jgDrawing.fPartNoCirD=8.0;
	jgDrawing.fPartGuigeTextSize=3.0;
	jgDrawing.iMatCharPosType=0;

	jgDrawing.iJgZoomSchema=3;				//0.1:1���� 1.ʹ�ù���ͼ���� 2.����ͬ������ 3.����ֱ�����
	jgDrawing.bModulateLongJg=TRUE;			//�����Ǹֳ���
	jgDrawing.bOneCardMultiPart=FALSE;		//�Ǹ�����һ��������
	jgDrawing.iJgGDimStyle=2;				//0.ʼ�˱�ע  1.�м��ע 2.�Զ��ж�
	jgDrawing.bMaxExtendAngleLength=TRUE;
	jgDrawing.nMaxBoltNumStartDimG=100;		//������ʼ�˱�ע׼��֧�ֵ������˨��
	jgDrawing.iLsSpaceDimStyle=2;			//0.X�᷽��	  1.Y�᷽��  2.�Զ��ж� 3.����ע
	jgDrawing.nMaxBoltNumAlongX=50;			//��X�᷽���ע֧�ֵ������˨����
	jgDrawing.bDimCutAngle=TRUE;			//��ע�Ǹ��н�
	jgDrawing.bDimCutAngleMap=TRUE;			//��ע�Ǹ��н�ʾ��ͼ
	jgDrawing.bDimPushFlatMap=TRUE;			//��עѹ��ʾ��ͼ
	jgDrawing.bJgUseSimpleLsMap=FALSE;		//�Ǹ�ʹ�ü���˨ͼ��
	jgDrawing.bDimLsAbsoluteDist=TRUE;		//��ע��˨���Գߴ�
	jgDrawing.bMergeLsAbsoluteDist=TRUE;	//�ϲ����ڵȾ���˨���Գߴ�
	jgDrawing.bDimRibPlatePartNo=TRUE;		//��ע�Ǹ��߰���
	jgDrawing.bDimRibPlateSetUpPos=TRUE;	//��ע�Ǹ��߰尲װλ��
	jgDrawing.iCutAngleDimType=0;			//�Ǹ��нǱ�ע��ʽ 
	//���ϽǱ�ע
	jgDrawing.bDimKaiHe=TRUE;				//��ע�Ǹֿ��Ͻ�
	jgDrawing.bDimKaiheAngleMap=TRUE;		//��ע�Ǹֿ��Ͻ�ʾ��ͼ
	jgDrawing.fKaiHeJiaoThreshold=2;		//Ĭ�Ͽ��ϽǱ�ע��ֵΪ2��
	jgDrawing.bDimKaiheSumLen=TRUE;
	jgDrawing.bDimKaiheAngle=TRUE;
	jgDrawing.bDimKaiheSegLen=TRUE;
	jgDrawing.bDimKaiheScopeMap=FALSE;
	//
	jgDrawing.sAngleCardPath.Empty();
	//
	font.fTextHeight = 30;
	font.fDxfTextSize = 2;
	font.fDimTextSize=2.5;
	font.fPartNoTextSize=3.0;
}

CSysPara::~CSysPara(void)
{
}

BOOL CSysPara::Write(CString file_path)	//д�����ļ�
{
	CString version("3.0");
	if(file_path.IsEmpty())
		return FALSE;
	CFile file;
	if(!file.Open(file_path, CFile::modeCreate|CFile::modeWrite))
		return FALSE;
	BYTE byte=0;
	CArchive ar(&file,CArchive::store);
	ar<<version;
	ar<<dock.m_nLeftPageWidth;
	ar<<dock.m_nRightPageWidth;
	ar<<(WORD)dock.pagePartList.bDisplay;
	ar<<dock.pagePartList.uDockBarID;
	ar<<(WORD)dock.pageProp.bDisplay;
	ar<<dock.pageProp.uDockBarID;

	
	ar<<font.fTextHeight;
	ar<<nc.m_fBaffleHigh;
	ar<<byte;//nc.m_iMKMode;
	ar<<nc.m_fMKRectW;
	ar<<nc.m_fMKRectL;
	ar<<nc.m_fMKHoleD;
	ar<<nc.m_sNcDriverPath;
	ar<<nc.m_iNcMode;
	//
	ar<<holeIncrement.m_fDatum;
	ar<<holeIncrement.m_fM12;
	ar<<holeIncrement.m_fM16;
	ar<<holeIncrement.m_fM20;
	ar<<holeIncrement.m_fM24;
	ar<<holeIncrement.m_fCutSH;
	ar<<holeIncrement.m_fProSH;
	ar<<holeIncrement.m_fWaist;
	//�������и�
	ar<<nc.m_ciDisplayType;
	ar<<CString(plasmaCut.m_sOutLineLen);
	ar<<CString(plasmaCut.m_sIntoLineLen);
	ar<<(WORD)plasmaCut.m_bInitPosFarOrg;
	ar<<(WORD)plasmaCut.m_bCutPosInInitPos;
	//�����и�
	ar<<CString(flameCut.m_sOutLineLen);
	ar<<CString(flameCut.m_sIntoLineLen);
	ar<<(WORD)flameCut.m_bInitPosFarOrg;
	ar<<(WORD)flameCut.m_bCutPosInInitPos;
	ar<<font.fDimTextSize;
	ar<<jgDrawing.iDimPrecision;
	ar<<jgDrawing.fRealToDraw;
	ar<<jgDrawing.fDimArrowSize;
	ar<<jgDrawing.fTextXFactor;
	ar<<font.fPartNoTextSize;
	ar<<jgDrawing.iPartNoFrameStyle;
	ar<<jgDrawing.fPartNoMargin;
	ar<<jgDrawing.fPartNoCirD;
	ar<<jgDrawing.fPartGuigeTextSize;
	ar<<jgDrawing.iMatCharPosType;

	ar<<jgDrawing.iJgZoomSchema;
	ar<<(WORD)jgDrawing.bModulateLongJg;
	ar<<(WORD)jgDrawing.bOneCardMultiPart;
	ar<<jgDrawing.iJgGDimStyle;
	ar<<(WORD)jgDrawing.bMaxExtendAngleLength;
	ar<<jgDrawing.nMaxBoltNumStartDimG;
	ar<<jgDrawing.iLsSpaceDimStyle;
	ar<<jgDrawing.nMaxBoltNumAlongX;
	ar<<(WORD)jgDrawing.bDimCutAngle;
	ar<<(WORD)jgDrawing.bDimCutAngleMap;
	ar<<(WORD)jgDrawing.bDimPushFlatMap;
	ar<<(WORD)jgDrawing.bJgUseSimpleLsMap;
	ar<<(WORD)jgDrawing.bDimLsAbsoluteDist;
	ar<<(WORD)jgDrawing.bMergeLsAbsoluteDist;
	ar<<(WORD)jgDrawing.bDimRibPlatePartNo;
	ar<<(WORD)jgDrawing.bDimRibPlateSetUpPos;
	ar<<jgDrawing.iCutAngleDimType;
	//���ϽǱ�ע
	ar<<(WORD)jgDrawing.bDimKaiHe;
	ar<<(WORD)jgDrawing.bDimKaiheAngleMap;
	ar<<jgDrawing.fKaiHeJiaoThreshold;
	ar<<(WORD)jgDrawing.bDimKaiheSumLen;
	ar<<(WORD)jgDrawing.bDimKaiheAngle;
	ar<<(WORD)jgDrawing.bDimKaiheSegLen;
	ar<<(WORD)jgDrawing.bDimKaiheScopeMap;
	//�Ǹֹ��տ�
	ar<<jgDrawing.sAngleCardPath;
	//PBJ����
	ar<<(WORD)pbj.m_bIncVertex;
	ar<<(WORD)pbj.m_bAutoSplitFile;
	ar << (WORD)pbj.m_bMergeHole;
	//PMZ���� wht 19-07-02
	ar << (WORD)pmz.m_iPmzMode;
	ar << (WORD)pmz.m_bIncVertex;
	ar << (WORD)pmz.m_bPmzCheck;
	//�ְ�NC�������
	ar << CString(nc.m_xFlamePara.m_sThick);
	ar << (DWORD)nc.m_xFlamePara.m_dwFileFlag;
	ar << CString(nc.m_xPlasmaPara.m_sThick);
	ar << (DWORD)nc.m_xPlasmaPara.m_dwFileFlag;
	ar << CString(nc.m_xPunchPara.m_sThick);
	ar << (DWORD)nc.m_xPunchPara.m_dwFileFlag;
	ar << CString(nc.m_xDrillPara.m_sThick);
	ar << (DWORD)nc.m_xDrillPara.m_dwFileFlag;
	ar << CString(nc.m_xLaserPara.m_sThick);
	for (int i = 0; i < 5; i++)
	{
		NC_INFO_PARA* pNcPare = NULL;
		if (i == 0)
			pNcPare = &nc.m_xFlamePara;
		else if (i == 1)
			pNcPare = &nc.m_xPlasmaPara;
		else if (i == 2)
			pNcPare = &nc.m_xPunchPara;
		else if (i == 3)
			pNcPare = &nc.m_xDrillPara;
		else //if (i == 4)
			pNcPare = &nc.m_xLaserPara;
		ar << pNcPare->m_xHoleIncrement.m_fDatum;
		ar << pNcPare->m_xHoleIncrement.m_fM12;
		ar << pNcPare->m_xHoleIncrement.m_fM16;
		ar << pNcPare->m_xHoleIncrement.m_fM20;
		ar << pNcPare->m_xHoleIncrement.m_fM24;
		ar << pNcPare->m_xHoleIncrement.m_fCutSH;
		ar << pNcPare->m_xHoleIncrement.m_fProSH;
		ar << pNcPare->m_xHoleIncrement.m_fWaist;
	}
	//������Ϣ
	ar << CString(model.m_sCompanyName);
	ar << CString(model.m_sPrjCode);
	ar << CString(model.m_sPrjName);
	ar << CString(model.m_sTaType);
	ar << CString(model.m_sTaAlias);
	ar << CString(model.m_sTaStampNo);
	ar << CString(model.m_sOperator);
	ar << CString(model.m_sAuditor);
	ar << CString(model.m_sCritic);
	ar << CString(model.m_sOutputPath);
	ar << model.file_format.m_sSplitters.size();
	for (size_t i = 0; i < model.file_format.m_sSplitters.size(); i++)
		ar << CString(model.file_format.m_sSplitters[i]);
	ar << model.file_format.m_sKeyMarkArr.size();
	for (size_t i = 0; i < model.file_format.m_sKeyMarkArr.size(); i++)
		ar << CString(model.file_format.m_sKeyMarkArr[i]);
	//��ӡ��������
	ar << filterMKParaList.GetNodeNum();
	for (FILTER_MK_PARA* pPara = filterMKParaList.GetFirst(); pPara; pPara = filterMKParaList.GetNext())
		ar << pPara->GetParaDesc();
	//����ע������� wxc 16-11-21
	WriteSysParaToReg("MKRectL");
	WriteSysParaToReg("MKRectW");
	WriteSysParaToReg("MKHoleD");
	WriteSysParaToReg("SideBaffleHigh");
	WriteSysParaToReg("M12Color");
	WriteSysParaToReg("M16Color");
	WriteSysParaToReg("M20Color");
	WriteSysParaToReg("M24Color");
	WriteSysParaToReg("OtherColor");
	WriteSysParaToReg("MarkColor");
	WriteSysParaToReg("EdgeColor");
	WriteSysParaToReg("HuoQuColor");
	WriteSysParaToReg("TextColor");
	WriteSysParaToReg("TextHeight");
	WriteSysParaToReg("DxfTextSize");
	WriteSysParaToReg("LimitSH");
	WriteSysParaToReg("DxfMode");	
	WriteSysParaToReg("DispMkRect");
	WriteSysParaToReg("MKVectType");
	WriteSysParaToReg("m_ciDisplayType");
	WriteSysParaToReg("DrillNeedSH");
	WriteSysParaToReg("DrillReduceSmallSH");
	WriteSysParaToReg("DrillHoldSortType");
	WriteSysParaToReg("DrillSortHasBigSH");
	WriteSysParaToReg("PunchNeedSH");
	WriteSysParaToReg("PunchReduceSmallSH");
	WriteSysParaToReg("PunchHoldSortType");
	WriteSysParaToReg("PunchSortHasBigSH");
	WriteSysParaToReg("flameCut.m_bCutPosInInitPos");
	WriteSysParaToReg("flameCut.m_bInitPosFarOrg");
	WriteSysParaToReg("flameCut.m_sIntoLineLen");
	WriteSysParaToReg("flameCut.m_sOutLineLen");
	WriteSysParaToReg("flameCut.m_wEnlargedSpace");
	WriteSysParaToReg("flameCut.m_bCutSpecialHole");
	WriteSysParaToReg("flameCut.m_bGrindingArc");
	WriteSysParaToReg("plasmaCut.m_bCutPosInInitPos");
	WriteSysParaToReg("plasmaCut.m_bInitPosFarOrg");
	WriteSysParaToReg("plasmaCut.m_sIntoLineLen");
	WriteSysParaToReg("plasmaCut.m_sOutLineLen");
	WriteSysParaToReg("plasmaCut.m_wEnlargedSpace");
	WriteSysParaToReg("plasmaCut.m_bGrindingArc");
	WriteSysParaToReg("plasmaCut.m_bCutSpecialHole");
	WriteSysParaToReg("laserPara.m_bOutputBendLine");
	WriteSysParaToReg("laserPara.m_bOutputBendType");
	WriteSysParaToReg("laserPara.m_wEnlargedSpace");
	WriteSysParaToReg("laserPara.m_bExplodeText");
	WriteSysParaToReg("PbjIncVertex");
	WriteSysParaToReg("PbjAutoSplitFile");
	WriteSysParaToReg("PbjMergeHole");
	WriteSysParaToReg("PmzMode");
	WriteSysParaToReg("PmzIncVertex");
	WriteSysParaToReg("PmzCheck");
	return TRUE;
}
//From FileVersion.cpp
BOOL CSysPara::Read(CString file_path)	//�������ļ�
{
	CString version;
	WORD w;BYTE byte;
	if(file_path.IsEmpty())
		return FALSE;
	CFile file;
	if(!file.Open(file_path, CFile::modeRead))
		return FALSE;
	int nValue = 0;
	double fValue;
	double fVersion;
	CArchive ar(&file,CArchive::load);

	ar>>version;
	fVersion=atof(version);
	ar>>dock.m_nLeftPageWidth;
	ar>>dock.m_nRightPageWidth;
	ar>>w;	dock.pagePartList.bDisplay=w;
	ar>>dock.pagePartList.uDockBarID;
	ar>>w;	dock.pageProp.bDisplay=w;
	ar>>dock.pageProp.uDockBarID;

	if (fVersion < 2.8)
		ar >> fValue;
	if(fVersion>=1.5)
		ar>>font.fTextHeight;
	if (fVersion >= 1.4 && fVersion <2.8)
		ar >> nValue;
		
	ar>>nc.m_fBaffleHigh;
	if(fVersion>=1.3)
	{
		ar>>byte;//nc.m_iMKMode;
		ar>>nc.m_fMKRectW;
		ar>>nc.m_fMKRectL;
	}
	ar>>nc.m_fMKHoleD;
	ar>>nc.m_sNcDriverPath;
	if(fVersion>=1.2)
		ar>>nc.m_iNcMode;
	if(fVersion>=1.6)
	{
		ar>>holeIncrement.m_fDatum;
		ar>>holeIncrement.m_fM12;
		ar>>holeIncrement.m_fM16;
		ar>>holeIncrement.m_fM20;
		ar>>holeIncrement.m_fM24;
		ar>>holeIncrement.m_fCutSH;
		if (fVersion >= 2.7)
			ar >> holeIncrement.m_fProSH;
		if (fVersion >= 3.0)
			ar >> holeIncrement.m_fWaist;
	}
	if(fVersion>=1.8)
	{	
		ar>>nc.m_ciDisplayType;
		//�������и�
		CString sValue;
		ar>>sValue;plasmaCut.m_sOutLineLen.Copy(sValue);
		ar>>sValue;plasmaCut.m_sIntoLineLen.Copy(sValue);
		ar>>w;plasmaCut.m_bInitPosFarOrg=w;
		ar>>w;plasmaCut.m_bCutPosInInitPos=w;
		if (fVersion < 2.8)
			ar >> w;
		if (fVersion >= 2.4 && fVersion<2.8)
			ar >> w;
		//�����и�
		ar>>sValue;flameCut.m_sOutLineLen.Copy(sValue);
		ar>>sValue;flameCut.m_sIntoLineLen.Copy(sValue);
		ar>>w;flameCut.m_bInitPosFarOrg=w;
		ar>>w; flameCut.m_bCutPosInInitPos=w;
		if (fVersion < 2.8)
			ar >> w;
		if (fVersion >= 2.4 && fVersion < 2.8)
			ar >> w;
	}
	else if(fVersion>=1.7)
	{	//�������и�
		ar>>fValue;plasmaCut.m_sOutLineLen.Printf("%f",fValue);
		ar>>fValue;plasmaCut.m_sIntoLineLen.Printf("%f",fValue);
		ar>>w;plasmaCut.m_bInitPosFarOrg=w;
		ar>>w;plasmaCut.m_bCutPosInInitPos=w;
		//�����и�
		ar>>fValue;flameCut.m_sOutLineLen.Printf("%f",fValue);
		ar>>fValue;flameCut.m_sIntoLineLen.Printf("%f",fValue);
		ar>>w;flameCut.m_bInitPosFarOrg=w;
		ar>>w; flameCut.m_bCutPosInInitPos=w;
	}
	ar>>font.fDimTextSize;
	ar>>jgDrawing.iDimPrecision;
	ar>>jgDrawing.fRealToDraw;
	ar>>jgDrawing.fDimArrowSize;
	ar>>jgDrawing.fTextXFactor;
	ar>>font.fPartNoTextSize;
	ar>>jgDrawing.iPartNoFrameStyle;
	ar>>jgDrawing.fPartNoMargin;
	ar>>jgDrawing.fPartNoCirD;
	ar>>jgDrawing.fPartGuigeTextSize;
	ar>>jgDrawing.iMatCharPosType;

	ar>>jgDrawing.iJgZoomSchema;
	ar>>w;	jgDrawing.bModulateLongJg=w;
	ar>>w;	jgDrawing.bOneCardMultiPart=w;
	ar>>jgDrawing.iJgGDimStyle;
	ar>>w;	jgDrawing.bMaxExtendAngleLength=w;
	ar>>jgDrawing.nMaxBoltNumStartDimG;
	ar>>jgDrawing.iLsSpaceDimStyle;
	ar>>jgDrawing.nMaxBoltNumAlongX;
	ar>>w;	jgDrawing.bDimCutAngle=w;
	ar>>w;	jgDrawing.bDimCutAngleMap=w;
	ar>>w;	jgDrawing.bDimPushFlatMap=w;
	ar>>w;	jgDrawing.bJgUseSimpleLsMap=w;
	ar>>w;	jgDrawing.bDimLsAbsoluteDist=w;
	ar>>w;	jgDrawing.bMergeLsAbsoluteDist=w;
	ar>>w;	jgDrawing.bDimRibPlatePartNo=w;
	ar>>w;	jgDrawing.bDimRibPlateSetUpPos=w;
	ar>>jgDrawing.iCutAngleDimType;
	//���ϽǱ�ע
	ar>>w;	jgDrawing.bDimKaiHe=w;
	ar>>w;	jgDrawing.bDimKaiheAngleMap=w;
	ar>>jgDrawing.fKaiHeJiaoThreshold;
	ar>>w;	jgDrawing.bDimKaiheSumLen=w;
	ar>>w;	jgDrawing.bDimKaiheAngle=w;
	ar>>w;	jgDrawing.bDimKaiheSegLen=w;
	ar>>w;	jgDrawing.bDimKaiheScopeMap=w;
	//�Ǹֹ��տ�·��
	if(compareVersion(version,"1.1")>=0)
		ar>>jgDrawing.sAngleCardPath;
	//PBJ����
	if(compareVersion(version,"1.9")>=0)
	{
		if (fVersion < 2.8)
			ar >> w;
		ar>>w;	pbj.m_bIncVertex=w;
		ar>>w;	pbj.m_bAutoSplitFile=w;
	}
	if (compareVersion(version, "2.3") >= 0)
	{
		ar >> w;	pbj.m_bMergeHole = w;
	}
	//PMZ����
	if (compareVersion(version, "2.1") >= 0)
	{
		ar >> w;	pmz.m_iPmzMode = w;
		ar >> w;	pmz.m_bIncVertex = w;
		ar >> w;	pmz.m_bPmzCheck = w;
	}
	//�ְ�NC�������
	if (compareVersion(version, "2.0") >= 0)
	{
		DWORD dw;
		CString sValue;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xFlamePara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xFlamePara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xPlasmaPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xPlasmaPara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xPunchPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xPunchPara.m_dwFileFlag = dw;
		if (fVersion < 2.8)
			ar >> w;
		ar >> sValue; nc.m_xDrillPara.m_sThick.Copy(sValue);
		ar >> dw; nc.m_xDrillPara.m_dwFileFlag = dw;
		ar >> sValue; nc.m_xLaserPara.m_sThick.Copy(sValue);
	}
	if (compareVersion(version, "2.8") >= 0)
	{
		for (int i = 0; i < 5; i++)
		{
			NC_INFO_PARA* pNcPare = NULL;
			if (i == 0)
				pNcPare = &nc.m_xFlamePara;
			else if (i == 1)
				pNcPare = &nc.m_xPlasmaPara;
			else if (i == 2)
				pNcPare = &nc.m_xPunchPara;
			else if (i == 3)
				pNcPare = &nc.m_xDrillPara;
			else //if (i == 4)
				pNcPare = &nc.m_xLaserPara;
			ar >> pNcPare->m_xHoleIncrement.m_fDatum;
			ar >> pNcPare->m_xHoleIncrement.m_fM12;
			ar >> pNcPare->m_xHoleIncrement.m_fM16;
			ar >> pNcPare->m_xHoleIncrement.m_fM20;
			ar >> pNcPare->m_xHoleIncrement.m_fM24;
			ar >> pNcPare->m_xHoleIncrement.m_fCutSH;
			ar >> pNcPare->m_xHoleIncrement.m_fProSH;
			if (compareVersion(version, "3.0") >= 0)
				ar >> pNcPare->m_xHoleIncrement.m_fWaist;
		}
	}
	//
	if (compareVersion(version, "2.2") >= 0)
	{
		CString sValue;
		ar >> sValue; model.m_sCompanyName.Copy(sValue);
		ar >> sValue; model.m_sPrjCode.Copy(sValue);
		ar >> sValue; model.m_sPrjName.Copy(sValue);
		ar >> sValue; model.m_sTaType.Copy(sValue);
		ar >> sValue; model.m_sTaAlias.Copy(sValue);
		ar >> sValue; model.m_sTaStampNo.Copy(sValue);
		ar >> sValue; model.m_sOperator.Copy(sValue);
		ar >> sValue; model.m_sAuditor.Copy(sValue);
		ar >> sValue; model.m_sCritic.Copy(sValue);
	}
	if (compareVersion(version, "2.6") >= 0)
	{
		CString sValue;
		ar >> sValue;
		model.m_sOutputPath.Copy(sValue);
	}
	if (compareVersion(version, "2.5") >= 0)
	{
		int nSize = 0;
		ar >> nSize;
		CString sValue;
		for (int i = 0; i < nSize; i++)
		{
			ar >> sValue;
			model.file_format.m_sSplitters.push_back(CXhChar16(sValue));
		}
		ar >> nSize;
		for (int i = 0; i < nSize; i++)
		{
			ar >> sValue;
			model.file_format.m_sKeyMarkArr.push_back(CXhChar16(sValue));
		}
	}
	if (compareVersion(version, "2.9") >= 0)
	{
		//��ӡ��������
		int nSize = 0;
		ar >> nSize;
		CString sValue;
		for (int i = 0; i < nSize; i++)
		{
			ar >> sValue;
			FILTER_MK_PARA* pPara = filterMKParaList.append();
			pPara->ParseParaDesc(sValue);
		}
	}
	//��ȡע�������
	ReadSysParaFromReg("MKRectL");
	ReadSysParaFromReg("MKRectW");
	ReadSysParaFromReg("MKHoleD");
	ReadSysParaFromReg("SideBaffleHigh");
	ReadSysParaFromReg("M12Color");
	ReadSysParaFromReg("M16Color");
	ReadSysParaFromReg("M20Color");
	ReadSysParaFromReg("M24Color");
	ReadSysParaFromReg("OtherColor");
	ReadSysParaFromReg("MarkColor");
	ReadSysParaFromReg("EdgeColor");
	ReadSysParaFromReg("HuoQuColor");
	ReadSysParaFromReg("TextColor");
	ReadSysParaFromReg("TextHeight");
	ReadSysParaFromReg("DxfTextSize");
	ReadSysParaFromReg("LimitSH");
	ReadSysParaFromReg("DxfMode");
	ReadSysParaFromReg("DispMkRect");
	ReadSysParaFromReg("MKVectType");
	ReadSysParaFromReg("m_ciDisplayType");
	ReadSysParaFromReg("DrillNeedSH");
	ReadSysParaFromReg("DrillReduceSmallSH");
	ReadSysParaFromReg("DrillHoldSortType");
	ReadSysParaFromReg("DrillSortHasBigSH");
	ReadSysParaFromReg("PunchNeedSH");
	ReadSysParaFromReg("PunchReduceSmallSH");
	ReadSysParaFromReg("PunchHoldSortType");
	ReadSysParaFromReg("PunchSortHasBigSH");
	ReadSysParaFromReg("flameCut.m_bCutPosInInitPos");
	ReadSysParaFromReg("flameCut.flameCut.m_bInitPosFarOrg");
	ReadSysParaFromReg("flameCut.m_sIntoLineLen");
	ReadSysParaFromReg("flameCut.m_sOutLineLen");
	ReadSysParaFromReg("flameCut.m_wEnlargedSpace");
	ReadSysParaFromReg("flameCut.m_bCutSpecialHole");
	ReadSysParaFromReg("flameCut.m_bGrindingArc");
	ReadSysParaFromReg("plasmaCut.m_bCutPosInInitPos");
	ReadSysParaFromReg("plasmaCut.m_bInitPosFarOrg");
	ReadSysParaFromReg("plasmaCut.m_sIntoLineLen");
	ReadSysParaFromReg("plasmaCut.m_sOutLineLen");
	ReadSysParaFromReg("plasmaCut.m_wEnlargedSpace");
	ReadSysParaFromReg("plasmaCut.m_bCutSpecialHole");
	ReadSysParaFromReg("plasmaCut.m_bGrindingArc");
	ReadSysParaFromReg("laserPara.m_bOutputBendLine");
	ReadSysParaFromReg("laserPara.m_bOutputBendType");
	ReadSysParaFromReg("laserPara.m_wEnlargedSpace");
	ReadSysParaFromReg("laserPara.m_bExplodeText");
	ReadSysParaFromReg("PbjIncVertex");
	ReadSysParaFromReg("PbjAutoSplitFile");
	ReadSysParaFromReg("PbjMergeHole");
	ReadSysParaFromReg("PmzMode");
	ReadSysParaFromReg("PmzIncVertex");
	ReadSysParaFromReg("PmzCheck");
	return TRUE;
}
//���湲�ò�����ע���
void CSysPara::WriteSysParaToReg(LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH]="";
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	DWORD dwLength=0;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS && hKey)
	{
		if (stricmp(lpszEntry, "MKRectL") == 0)
			sprintf(sValue, "%f", nc.m_fMKRectL);
		else if (stricmp(lpszEntry, "MKRectW") == 0)
			sprintf(sValue, "%f", nc.m_fMKRectW);
		else if (stricmp(lpszEntry, "MKHoleD") == 0)
			sprintf(sValue, "%f", nc.m_fMKHoleD);
		else if (stricmp(lpszEntry, "SideBaffleHigh") == 0)
			sprintf(sValue, "%f", nc.m_fBaffleHigh);
		else if (stricmp(lpszEntry, "M12Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS12);
		else if (stricmp(lpszEntry, "M16Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS16);
		else if (stricmp(lpszEntry, "M20Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS20);
		else if (stricmp(lpszEntry, "M24Color") == 0)
			sprintf(sValue, "RGB%X", crMode.crLS24);
		else if (stricmp(lpszEntry, "OtherColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crOtherLS);
		else if (stricmp(lpszEntry, "MarkColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crMark);
		else if (stricmp(lpszEntry, "EdgeColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crEdge);
		else if (stricmp(lpszEntry, "HuoQuColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crHuoQu);
		else if (stricmp(lpszEntry, "TextColor") == 0)
			sprintf(sValue, "RGB%X", crMode.crText);
		else if (stricmp(lpszEntry, "TextHeight") == 0)
			sprintf(sValue, "%f", font.fTextHeight);
		else if (stricmp(lpszEntry, "DxfTextSize") == 0)
			sprintf(sValue, "%f", font.fDxfTextSize);
		else if (stricmp(lpszEntry, "LimitSH") == 0)
			sprintf(sValue, "%f", nc.m_fLimitSH);
		else if (stricmp(lpszEntry, "DxfMode") == 0)
			sprintf(sValue, "%d", nc.m_iDxfMode);
		else if (stricmp(lpszEntry, "DrillNeedSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bReserveBigSH);
		else if (stricmp(lpszEntry, "DrillReduceSmallSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bReduceSmallSH);
		else if (stricmp(lpszEntry, "DrillHoldSortType") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_ciHoldSortType);
		else if (stricmp(lpszEntry, "DrillSortHasBigSH") == 0)
			sprintf(sValue, "%d", nc.m_xDrillPara.m_bSortHasBigSH);
		else if (stricmp(lpszEntry, "PunchNeedSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bReserveBigSH);
		else if (stricmp(lpszEntry, "PunchReduceSmallSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bReduceSmallSH);
		else if (stricmp(lpszEntry, "PunchHoldSortType") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_ciHoldSortType);
		else if (stricmp(lpszEntry, "PunchSortHasBigSH") == 0)
			sprintf(sValue, "%d", nc.m_xPunchPara.m_bSortHasBigSH);
		else if (stricmp(lpszEntry, "DispMkRect") == 0)
			sprintf(sValue, "%d", nc.m_bDispMkRect);
		else if (stricmp(lpszEntry, "MKVectType") == 0)
			sprintf(sValue, "%d", nc.m_ciMkVect);
		else if (stricmp(lpszEntry, "m_ciDisplayType") == 0)
			sprintf(sValue, "%d", nc.m_ciDisplayType);

		//
		else if (stricmp(lpszEntry, "PbjIncVertex") == 0)
			sprintf(sValue, "%d", pbj.m_bIncVertex);
		else if (stricmp(lpszEntry, "PbjAutoSplitFile") == 0)
			sprintf(sValue, "%d", pbj.m_bAutoSplitFile);
		else if (stricmp(lpszEntry, "PbjMergeHole") == 0)
			sprintf(sValue, "%d", pbj.m_bMergeHole);
		//PMZ����
		else if (stricmp(lpszEntry, "PmzMode") == 0)
			sprintf(sValue, "%d", pmz.m_iPmzMode);
		else if (stricmp(lpszEntry, "PmzIncVertex") == 0)
			sprintf(sValue, "%d", pmz.m_bIncVertex);
		else if (stricmp(lpszEntry, "PmzCheck") == 0)
			sprintf(sValue, "%d", pmz.m_bPmzCheck);
		//
		else if (stricmp(lpszEntry, "flameCut.m_bCutPosInInitPos") == 0)
			sprintf(sValue, "%d", flameCut.m_bCutPosInInitPos);
		else if (stricmp(lpszEntry, "flameCut.m_bInitPosFarOrg") == 0)
			sprintf(sValue, "%d", flameCut.m_bInitPosFarOrg);
		else if (stricmp(lpszEntry, "flameCut.m_sIntoLineLen") == 0)
			strcpy(sValue, flameCut.m_sIntoLineLen);
		else if (stricmp(lpszEntry, "flameCut.m_sOutLineLen") == 0)
			sprintf(sValue, flameCut.m_sOutLineLen);
		else if (stricmp(lpszEntry, "flameCut.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xFlamePara.m_wEnlargedSpace);
		else if (stricmp(lpszEntry, "flameCut.m_bCutSpecialHole") == 0)
			sprintf(sValue, "%d", nc.m_xFlamePara.m_bCutSpecialHole);
		else if (stricmp(lpszEntry, "flameCut.m_bGrindingArc") == 0)
			sprintf(sValue, "%d", nc.m_xFlamePara.m_bGrindingArc);
		//
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutPosInInitPos") == 0)
			sprintf(sValue, "%d", plasmaCut.m_bCutPosInInitPos);
		else if (stricmp(lpszEntry, "plasmaCut.m_bInitPosFarOrg") == 0)
			sprintf(sValue, "%d", plasmaCut.m_bInitPosFarOrg);
		else if (stricmp(lpszEntry, "plasmaCut.m_sIntoLineLen") == 0)
			strcpy(sValue, plasmaCut.m_sIntoLineLen);
		else if (stricmp(lpszEntry, "plasmaCut.m_sOutLineLen") == 0)
			strcpy(sValue, plasmaCut.m_sOutLineLen);
		else if (stricmp(lpszEntry, "plasmaCut.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xPlasmaPara.m_wEnlargedSpace);
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutSpecialHole") == 0)
			sprintf(sValue, "%d", nc.m_xPlasmaPara.m_bCutSpecialHole);
		else if (stricmp(lpszEntry, "plasmaCut.m_bGrindingArc") == 0)
			sprintf(sValue, "%d", nc.m_xPlasmaPara.m_bGrindingArc);
		//
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendLine") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_bOutputBendLine);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendType") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_bOutputBendType);
		else if (stricmp(lpszEntry, "laserPara.m_wEnlargedSpace") == 0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_wEnlargedSpace);
		else if(stricmp(lpszEntry, "laserPara.m_bExplodeText")==0)
			sprintf(sValue, "%d", nc.m_xLaserPara.m_bExplodeText);
		dwLength = strlen(sValue);
		RegSetValueEx(hKey, lpszEntry, NULL, REG_SZ, (BYTE*)&sValue[0], dwLength);
		RegCloseKey(hKey);
	}
}
//
void CSysPara::ReadSysParaFromReg(LPCTSTR lpszEntry)
{
	char sValue[MAX_PATH],tem_str[100]="";;
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	HKEY hKey;
	DWORD dwDataType,dwLength=MAX_PATH;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS && hKey &&
		RegQueryValueEx(hKey, lpszEntry, NULL, &dwDataType, (BYTE*)&sValue[0], &dwLength) == ERROR_SUCCESS)
	{
		if (stricmp(lpszEntry, "MKRectL") == 0)
			nc.m_fMKRectL = atof(sValue);
		else if (stricmp(lpszEntry, "MKRectW") == 0)
			nc.m_fMKRectW = atof(sValue);
		else if (stricmp(lpszEntry, "MKHoleD") == 0)
			nc.m_fMKHoleD = atof(sValue);
		else if (stricmp(lpszEntry, "SideBaffleHigh") == 0)
			nc.m_fBaffleHigh = atof(sValue);
		else if (stricmp(lpszEntry, "TextHeight") == 0)
			font.fTextHeight = atof(sValue);
		else if (stricmp(lpszEntry, "DxfTextSize") == 0)
			font.fDxfTextSize = atof(sValue);
		else if (stricmp(lpszEntry, "LimitSH") == 0)
			nc.m_fLimitSH = atof(sValue);
		else if (stricmp(lpszEntry, "DxfMode") == 0)
			nc.m_iDxfMode = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillNeedSH") == 0)
			nc.m_xDrillPara.m_bReserveBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillReduceSmallSH") == 0)
			nc.m_xDrillPara.m_bReduceSmallSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillHoldSortType") == 0)
			nc.m_xDrillPara.m_ciHoldSortType = atoi(sValue);
		else if (stricmp(lpszEntry, "DrillSortHasBigSH") == 0)
			nc.m_xDrillPara.m_bSortHasBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchNeedSH") == 0)
			nc.m_xPunchPara.m_bReserveBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchReduceSmallSH") == 0)
			nc.m_xPunchPara.m_bReduceSmallSH = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchHoldSortType") == 0)
			nc.m_xPunchPara.m_ciHoldSortType = atoi(sValue);
		else if (stricmp(lpszEntry, "PunchSortHasBigSH") == 0)
			nc.m_xPunchPara.m_bSortHasBigSH = atoi(sValue);
		else if (stricmp(lpszEntry, "DispMkRect") == 0)
			nc.m_bDispMkRect = atoi(sValue);
		else if (stricmp(lpszEntry, "MKVectType") == 0)
			nc.m_ciMkVect = atoi(sValue);
		else if (stricmp(lpszEntry, "m_ciDisplayType") == 0)
			nc.m_ciDisplayType = atoi(sValue);
		//
		else if (stricmp(lpszEntry, "PbjIncVertex") == 0)
			pbj.m_bIncVertex = atoi(sValue);
		else if (stricmp(lpszEntry, "PbjAutoSplitFile") == 0)
			pbj.m_bAutoSplitFile = atoi(sValue);
		else if (stricmp(lpszEntry, "PbjMergeHole") == 0)
			pbj.m_bMergeHole = atoi(sValue);
		//
		//PMZ����
		else if (stricmp(lpszEntry, "PmzMode") == 0)
			pmz.m_iPmzMode = atoi(sValue);
		else if (stricmp(lpszEntry, "PmzIncVertex") == 0)
			pmz.m_bIncVertex = atoi(sValue);
		else if (stricmp(lpszEntry, "PmzCheck") == 0)
			pmz.m_bPmzCheck = atoi(sValue);
		//
		else if (stricmp(lpszEntry, "M12Color") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crLS12);
		}
		else if (stricmp(lpszEntry, "M16Color") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crLS16);
		}
		else if (stricmp(lpszEntry, "M20Color") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crLS20);
		}
		else if (stricmp(lpszEntry, "M24Color") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crLS24);
		}
		else if (stricmp(lpszEntry, "OtherColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crOtherLS);
		}
		else if (stricmp(lpszEntry, "MarkColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crMark);
		}
		else if (stricmp(lpszEntry, "EdgeColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crEdge);
		}
		else if (stricmp(lpszEntry, "HuoQuColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crHuoQu);
		}
		else if (stricmp(lpszEntry, "TextColor") == 0)
		{
			sprintf(tem_str, "%s", sValue);
			memmove(tem_str, tem_str + 3, 97);
			sscanf(tem_str, "%X", &crMode.crText);
		}
		else if (stricmp(lpszEntry, "flameCut.m_bInitPosFarOrg") == 0)
			flameCut.m_bInitPosFarOrg = atoi(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_bCutPosInInitPos") == 0)
			flameCut.m_bCutPosInInitPos = atoi(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_sIntoLineLen") == 0)
			flameCut.m_sIntoLineLen.Copy(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_sOutLineLen") == 0)
			flameCut.m_sOutLineLen.Copy(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_wEnlargedSpace") == 0)
			nc.m_xFlamePara.m_wEnlargedSpace = atoi(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_bCutSpecialHole") == 0)
			nc.m_xFlamePara.m_bCutSpecialHole = atoi(sValue);
		else if (stricmp(lpszEntry, "flameCut.m_bGrindingArc") == 0)
			nc.m_xFlamePara.m_bGrindingArc = atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_bInitPosFarOrg") == 0)
			plasmaCut.m_bInitPosFarOrg = atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutPosInInitPos") == 0)
			plasmaCut.m_bCutPosInInitPos = atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_sIntoLineLen") == 0)
			plasmaCut.m_sIntoLineLen.Copy(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_sOutLineLen") == 0)
			plasmaCut.m_sOutLineLen.Copy(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_wEnlargedSpace") == 0)
			nc.m_xPlasmaPara.m_wEnlargedSpace = atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_bCutSpecialHole") == 0)
			nc.m_xPlasmaPara.m_bCutSpecialHole = atoi(sValue);
		else if (stricmp(lpszEntry, "plasmaCut.m_bGrindingArc") == 0)
			nc.m_xPlasmaPara.m_bGrindingArc = atoi(sValue);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendLine") == 0)
			nc.m_xLaserPara.m_bOutputBendLine = atoi(sValue);
		else if (stricmp(lpszEntry, "laserPara.m_bOutputBendType") == 0)
			nc.m_xLaserPara.m_bOutputBendType = atoi(sValue);
		else if (stricmp(lpszEntry, "laserPara.m_wEnlargedSpace") == 0)
			nc.m_xLaserPara.m_wEnlargedSpace = atoi(sValue);
		else if(stricmp(lpszEntry, "laserPara.m_bExplodeText")==0)
			nc.m_xLaserPara.m_bExplodeText = atoi(sValue);
		RegCloseKey(hKey);
	}
}
void CSysPara::UpdateHoleIncrement(double fHoleInc)
{
	if (fabs(holeIncrement.m_fM12 - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fM12 = fHoleInc;
	if (fabs(holeIncrement.m_fM16 - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fM16 = fHoleInc;
	if (fabs(holeIncrement.m_fM20 - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fM20 = fHoleInc;
	if (fabs(holeIncrement.m_fM24 - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fM24 = fHoleInc;
	if (fabs(holeIncrement.m_fCutSH - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fCutSH = fHoleInc;
	if (fabs(holeIncrement.m_fProSH - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fProSH = fHoleInc;
	if (fabs(holeIncrement.m_fWaist - holeIncrement.m_fDatum) <= EPS)
		holeIncrement.m_fWaist = fHoleInc;
	holeIncrement.m_fDatum = fHoleInc;
}
BOOL CSysPara::IsFilterMK(int nThick, char cMat)
{
	FILTER_MK_PARA* pPara = NULL;
	for (pPara = filterMKParaList.GetFirst(); pPara; pPara = filterMKParaList.GetNext())
	{
		CHashList<SEGI> thickHash;
		GetSegNoHashTblBySegStr(pPara->m_sThickRange, thickHash);
		if (thickHash.GetValue(nThick))
			break;
	}
	if (pPara)
	{
		if (cMat == 'S')
			return pPara->m_bFileterS;
		else if (cMat == 'H')
			return pPara->m_bFileterH;
		else if (cMat == 'h')
			return pPara->m_bFileterh;
		else if (cMat == 'G')
			return pPara->m_bFileterG;
		else if (cMat == 'P')
			return pPara->m_bFileterP;
		else if (cMat == 'T')
			return pPara->m_bFileterT;
	}
	return FALSE;
}
int CSysPara::GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen/*=100*/)
{
	BOOL bContinueJustify = FALSE;
	CXhChar200 sText;
	if(GetPropID("font.fTextHeight")==id)
	{
		sText.Printf("%f",font.fTextHeight);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("font.fDxfTextSize") == id)
	{
		sText.Printf("%f", font.fDxfTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_bDispMkRect")==id)
	{
		if(nc.m_bDispMkRect)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.m_ciMkVect") == id)
	{
		if (nc.m_ciMkVect == 0)
			sText.Copy("0.����ԭ����");
		else
			sText.Copy("1.����ˮƽ");
	}
	else if(GetPropID("nc.m_fMKRectW")==id)
	{
		sText.Printf("%f",nc.m_fMKRectW);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_fMKRectL")==id)
	{
		sText.Printf("%f",nc.m_fMKRectL);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_fMKHoleD")==id)
	{
		sText.Printf("%f",nc.m_fMKHoleD);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_iNcMode")==id)
	{
		if(IsValidNcFlag(CNCPart::FLAME_MODE))
			sText.Append("����", '+');
		if(IsValidNcFlag(CNCPart::PLASMA_MODE))
			sText.Append("������",'+');
		if(IsValidNcFlag(CNCPart::PUNCH_MODE))
			sText.Append("�崲",'+');
		if (IsValidNcFlag(CNCPart::DRILL_MODE))
			sText.Append("�괲", '+');
		if (IsValidNcFlag(CNCPart::LASER_MODE))
			sText.Append("����", '+');
	}
	else if(GetPropID("nc.m_fLimitSH")==id)
	{
		sText.Printf("%f",nc.m_fLimitSH);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("CutLimitSH") == id)
		sText.Printf(">= %g", nc.m_fLimitSH);
	else if (GetPropID("ProLimitSH") == id)
		sText.Printf("< %g", nc.m_fLimitSH);
	else if(GetPropID("nc.DrillPara.m_bReserveBigSH")==id)
	{
		if(nc.m_xDrillPara.m_bReserveBigSH)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.DrillPara.m_bSortHasBigSH") == id)
	{
		if (nc.m_xDrillPara.m_bSortHasBigSH)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.DrillPara.m_bReduceSmallSH") == id)
	{
		if (nc.m_xDrillPara.m_bReduceSmallSH)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.DrillPara.m_ciHoldSortType") == id)
	{
		if (nc.m_xDrillPara.m_ciHoldSortType == 0)
			sText.Copy("0.����");
		else if (nc.m_xDrillPara.m_ciHoldSortType == 1)
			sText.Copy("1.����+����");
		else if (nc.m_xDrillPara.m_ciHoldSortType == 2)
			sText.Copy("2.����+�׾�");
	}
	else if(GetPropID("nc.m_fBaffleHigh")==id)
	{
		sText.Printf("%f",nc.m_fBaffleHigh);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("nc.m_iDxfMode")==id)
	{
		if(nc.m_iDxfMode==0)
			sText.Copy("��");
		else if(nc.m_iDxfMode==1)
			sText.Copy("��");
	}
	else if(GetPropID("nc.m_sNcDriverPath")==id)
		sText.Copy(nc.m_sNcDriverPath);

	else if (GetPropID("nc.bFlameCut") == id)
	{
		if (IsValidNcFlag(CNCPart::FLAME_MODE))
			strcpy(sText, "����");
		else
			strcpy(sText, "����");
	}
	else if (GetPropID("nc.FlamePara.m_sThick") == id)
		strcpy(sText, nc.m_xFlamePara.m_sThick);
	else if (GetPropID("nc.FlamePara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_TXT_FILE))
			sValue.Append("TXT", '+');
		if (nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_CNC_FILE))
			sValue.Append("CNC", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bPlasmaCut") == id)
	{
		if (IsValidNcFlag(CNCPart::PLASMA_MODE))
			strcpy(sText, "����");
		else
			strcpy(sText, "����");
	}
	else if (GetPropID("nc.PlasmaPara.m_sThick") == id)
		strcpy(sText, nc.m_xPlasmaPara.m_sThick);
	else if (GetPropID("nc.PlasmaPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_NC_FILE))
			sValue.Append("NC", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bPunchPress") == id)
	{
		if (IsValidNcFlag(CNCPart::PUNCH_MODE))
			strcpy(sText, "����");
		else
			strcpy(sText, "����");
	}
	else if (GetPropID("nc.PunchPara.m_sThick") == id)
		strcpy(sText, nc.m_xPunchPara.m_sThick);
	else if (GetPropID("nc.PunchPara.m_bReserveBigSH") == id)
	{
		if (nc.m_xPunchPara.m_bReserveBigSH)
			strcpy(sText, "��");
		else
			strcpy(sText, "��");
	}
	else if (GetPropID("nc.PunchPara.m_bSortHasBigSH") == id)
	{
		if (nc.m_xPunchPara.m_bSortHasBigSH)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.PunchPara.m_bReduceSmallSH") == id)
	{
		if (nc.m_xPunchPara.m_bReduceSmallSH)
			strcpy(sText, "��");
		else
			strcpy(sText, "��");
	}
	else if (GetPropID("nc.PunchPara.m_ciHoldSortType") == id)
	{
		if (nc.m_xPunchPara.m_ciHoldSortType == 0)
			sText.Copy("0.����");
		else if (nc.m_xPunchPara.m_ciHoldSortType == 1)
			sText.Copy("1.����+����");
		else if (nc.m_xPunchPara.m_ciHoldSortType == 2)
			sText.Copy("2.����+�׾�");
	}
	else if (GetPropID("nc.PunchPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_PBJ_FILE))
			sValue.Append("PBJ", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_WKF_FILE))
			sValue.Append("WKF", '+');
		if (nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_TTP_FILE))
			sValue.Append("TTP", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bDrillPress") == id)
	{
		if (IsValidNcFlag(CNCPart::DRILL_MODE))
			strcpy(sText, "����");
		else
			strcpy(sText, "����");
	}
	else if (GetPropID("nc.DrillPara.m_sThick") == id)
		strcpy(sText, nc.m_xDrillPara.m_sThick);
	else if (GetPropID("nc.DrillPara.m_dwFileFlag") == id)
	{
		CXhChar100 sValue;
		if (nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
			sValue.Append("DXF", '+');
		if (nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_PMZ_FILE))
			sValue.Append("PMZ", '+');
		strcpy(sText, sValue);
	}
	else if (GetPropID("nc.bLaser") == id)
	{
		if (IsValidNcFlag(CNCPart::LASER_MODE))
			strcpy(sText, "����");
		else
			strcpy(sText, "����");
	}
	else if(GetPropID("nc.LaserPara.m_wEnlargedSpace")==id)
		sText.Printf("%d", nc.m_xLaserPara.m_wEnlargedSpace);
	else if (GetPropID("nc.LaserPara.m_sThick") == id)
		strcpy(sText, nc.m_xLaserPara.m_sThick);
	else if (GetPropID("nc.LaserPara.m_dwFileFlag") == id)
		strcpy(sText, "DXF");
	else if (GetPropID("nc.LaserPara.m_bOutputBendLine") == id)
	{
		if (nc.m_xLaserPara.m_bOutputBendLine)
			sText.Copy("��");
		else //if (nc.m_xLaserPara.m_bOutputBendLine)
			sText.Copy("��");
	}
	else if (GetPropID("nc.LaserPara.m_bExplodeText") == id)
	{
		if (nc.m_xLaserPara.m_bExplodeText)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("nc.LaserPara.m_bOutputBendType") == id)
	{
		if (nc.m_xLaserPara.m_bOutputBendType)
			sText.Copy("��");
		else //if (nc.m_xLaserPara.m_bOutputBendType)
			sText.Copy("��");
	}
	else if (GetPropID("FileFormat") == id)
		sText.Copy(model.file_format.GetFileFormatStr());
	else if(GetPropID("OutputPath")==id)
		sText.Copy(model.m_sOutputPath);
	else if(GetPropID("pbj.m_bIncVertex")==id)
	{
		if(pbj.m_bIncVertex)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("pbj.m_bAutoSplitFile")==id)
	{
		if(pbj.m_bAutoSplitFile)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("pbj.m_bMergeHole") == id)
	{
	if (pbj.m_bMergeHole)
		sText.Copy("��");
	else
		sText.Copy("��");
	}
	else if (GetPropID("pmz.m_iPmzMode") == id)
	{
		if (pmz.m_iPmzMode == 1)
			sText.Copy("1.���ļ�");
		else //if(pmz.m_iPmzMode == 0)
			sText.Copy("0.���ļ�");
	}
	else if (GetPropID("pmz.m_bIncVertex") == id)
	{
		if (pmz.m_bIncVertex)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("pmz.m_bPmzCheck") == id)
	{
		if (pmz.m_bPmzCheck)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
#ifdef __PNC_
	else if(GetPropID("nc.m_ciDisplayType")==id)
	{
		if (g_sysPara.nc.m_ciDisplayType == CNCPart::FLAME_MODE)
			sText.Copy("����");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::PLASMA_MODE)
			sText.Copy("������");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::PUNCH_MODE)
			sText.Copy("�崲");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::DRILL_MODE)
			sText.Copy("�괲");
		else if (g_sysPara.nc.m_ciDisplayType == CNCPart::LASER_MODE)
			sText.Copy("����");
		else if (g_sysPara.nc.m_ciDisplayType > 0)
			sText.Copy("����ģʽ");
		else
			sText.Copy("ԭʼ");
	}
	//�������и�
	else if(GetPropID("plasmaCut.m_sOutLineLen")==id)
		sText.Copy(plasmaCut.m_sOutLineLen);
	else if(GetPropID("plasmaCut.m_sIntoLineLen")==id)
		sText.Copy(plasmaCut.m_sIntoLineLen);
	else if(GetPropID("plasmaCut.m_bInitPosFarOrg")==id)
	{
		if(!g_sysPara.plasmaCut.m_bInitPosFarOrg)
			sText.Copy("0.����ԭ��");
		else
			sText.Copy("1.Զ��ԭ��");
	}
	else if(GetPropID("plasmaCut.m_bCutPosInInitPos")==id)
	{
		if(plasmaCut.m_bCutPosInInitPos)
			sText.Copy("1.ʼ���ڳ�ʼ��");
		else
			sText.Copy("0.��ָ��������");
	}
	else if(GetPropID("nc.PlasmaPara.m_wEnlargedSpace")==id)
		sText.Printf("%d",nc.m_xPlasmaPara.m_wEnlargedSpace);
	else if (GetPropID("plasmaCut.m_bGrindingArc") == id)
	{
		if (nc.m_xPlasmaPara.m_bGrindingArc)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("plasmaCut.m_bCutSpecialHole") == id)
	{
		if (nc.m_xPlasmaPara.m_bCutSpecialHole)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	//�����и�
	else if(GetPropID("flameCut.m_sOutLineLen")==id)
		sText.Copy(flameCut.m_sOutLineLen);
	else if(GetPropID("flameCut.m_sIntoLineLen")==id)
		sText.Copy(flameCut.m_sIntoLineLen);
	else if(GetPropID("flameCut.m_bInitPosFarOrg")==id)
	{
		if(!g_sysPara.flameCut.m_bInitPosFarOrg)
			sText.Copy("0.����ԭ��");
		else
			sText.Copy("1.Զ��ԭ��");
	}
	else if(GetPropID("flameCut.m_bCutPosInInitPos")==id)
	{
		if(flameCut.m_bCutPosInInitPos)
			sText.Copy("1.ʼ���ڳ�ʼ��");
		else
			sText.Copy("0.��ָ��������");
	}
	else if(GetPropID("nc.FlamePara.m_wEnlargedSpace")==id)
		sText.Printf("%d", nc.m_xFlamePara.m_wEnlargedSpace);
	else if (GetPropID("flameCut.m_bGrindingArc") == id)
	{
		if (nc.m_xFlamePara.m_bGrindingArc)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("flameCut.m_bCutSpecialHole") == id)
	{
		if(nc.m_xFlamePara.m_bCutSpecialHole)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
#endif
	else if(GetPropID("holeIncrement.m_fDatum")==id)
	{
		sText.Printf("%f",holeIncrement.m_fDatum);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM12")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM12);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM16")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM16);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM20")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM20);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fM24")==id)
	{
		sText.Printf("%f",holeIncrement.m_fM24);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("holeIncrement.m_fCutSH")==id)
	{
		sText.Printf("%f",holeIncrement.m_fCutSH);
		SimplifiedNumString(sText);
	}
	else if (GetPropID("holeIncrement.m_fProSH") == id)
	{
		sText.Printf("%f", holeIncrement.m_fProSH);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("crMode.crLS12")==id)
		sText.Printf("RGB%X",crMode.crLS12);
	else if(GetPropID("crMode.crLS16")==id)
		sText.Printf("RGB%X",crMode.crLS16);
	else if(GetPropID("crMode.crLS20")==id)
		sText.Printf("RGB%X",crMode.crLS20);
	else if(GetPropID("crMode.crLS24")==id)
		sText.Printf("RGB%X",crMode.crLS24);
	else if(GetPropID("crMode.crOtherLS")==id)
		sText.Printf("RGB%X",crMode.crOtherLS);
	else if(GetPropID("crMode.crMarK")==id)
		sText.Printf("RGB%X",crMode.crMark);
	else if(GetPropID("crMode.crEdge")==id)
		sText.Printf("RGB%X",crMode.crEdge);
	else if(GetPropID("crMode.crHuoQu")==id)
		sText.Printf("RGB%X", crMode.crHuoQu);
	else if (GetPropID("crMode.crText") == id)
		sText.Printf("RGB%X", crMode.crText);
	else if(GetPropID("font.fDimTextSize")==id)
	{
		sText.Printf("%f",font.fDimTextSize);
		SimplifiedNumString(sText);
	}
	else
		bContinueJustify = TRUE;
	if (!bContinueJustify)
	{	//if-else�ж�̫�����ͨ����ȥ������ֻ�����м�Ͽ�
		if (valueStr)
			StrCopy(valueStr, sText, nMaxStrBufLen);
		return strlen(sText);
	}
	bContinueJustify = FALSE;
	if(GetPropID("jgDrawing.iDimPrecision")==id)
	{
		if(jgDrawing.iDimPrecision==0)
			sText.Copy("1.0mm");
		else if(jgDrawing.iDimPrecision==1)
			sText.Copy("0.5mm");
		else //if(jgDrawing.iDimPrecision==2)
			sText.Copy("0.1mm");
	}
	else if(GetPropID("jgDrawing.fRealToDraw")==id)
	{
		sText.Printf("%f",jgDrawing.fRealToDraw);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fDimArrowSize")==id)
	{
		sText.Printf("%f",jgDrawing.fDimArrowSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fTextXFactor")==id)
	{
		sText.Printf("%f",jgDrawing.fTextXFactor);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("font.fPartNoTextSize")==id)
	{
		sText.Printf("%f",font.fPartNoTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.iPartNoFrameStyle")==id)
	{
		if(jgDrawing.iPartNoFrameStyle==0)
			sText.Printf("%s","0.ԲȦ");
		else if(jgDrawing.iPartNoFrameStyle==1)
			sText.Printf("%s","1.��Բ����");
		else if(jgDrawing.iPartNoFrameStyle==2)
			sText.Printf("%s","2.��ͨ����");
		else 
			sText.Printf("%s","3.�Զ��ж�");
	}
	else if(GetPropID("jgDrawing.fPartNoMargin")==id)
	{
		sText.Printf("%f",jgDrawing.fPartNoMargin);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fPartNoCirD")==id)
	{
		sText.Printf("%f",jgDrawing.fPartNoCirD);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fPartGuigeTextSize")==id)
	{
		sText.Printf("%f",jgDrawing.fPartGuigeTextSize);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.iMatCharPosType")==id)
	{
		if(jgDrawing.iMatCharPosType==0)
			sText.Printf("%s","0.����Ҫ�����ַ�");
		else if(jgDrawing.iMatCharPosType==1)
			sText.Printf("%s","1.�������ǰ");
		else if(jgDrawing.iMatCharPosType==2)
			sText.Printf("%s","2.������ź�");
	}
	//�Ǹֹ���ͼ����
	else if(GetPropID("jgDrawing.fLsDistThreshold")==id)
	{
		sText.Printf("%f",jgDrawing.fLsDistThreshold);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.fLsDistZoomCoef")==id)
	{
		sText.Printf("%f",jgDrawing.fLsDistZoomCoef);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.bOneCardMultiPart")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bModulateLongJg")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.iJgZoomSchema")==id)
	{
		if(jgDrawing.iJgZoomSchema==1)
			sText.Printf("1.ʹ�ù���ͼ����");
		else if(jgDrawing.iJgZoomSchema==2)
			sText.Printf("2.����ͬ������");
		else if(jgDrawing.iJgZoomSchema==3)
			sText.Printf("3.����ֱ�����");
		else 
			sText.Printf("0.������1:1����");
	}
	else if(GetPropID("jgDrawing.bMaxExtendAngleLength")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.iJgGDimStyle")==id)
	{
		if(jgDrawing.iJgGDimStyle==0)
			sText.Printf("%s","0.ʼ��");
		else if(jgDrawing.iJgGDimStyle==1)
			sText.Printf("%s","1.�м�");
		else 
			sText.Printf("%s","2.�Զ��ж�");
	}
	else if(GetPropID("jgDrawing.nMaxBoltNumStartDimG")==id)
		sText.Printf("%d",jgDrawing.nMaxBoltNumStartDimG);
	else if(GetPropID("jgDrawing.iLsSpaceDimStyle")==id)
	{
		if(jgDrawing.iLsSpaceDimStyle==0)
			sText.Printf("%s","0.��X�᷽��");
		else if(jgDrawing.iLsSpaceDimStyle==1)
			sText.Printf("%s","1.��Y�᷽��");
		else if(jgDrawing.iLsSpaceDimStyle==3)
			sText.Printf("%s","3.����ע���");
		else if(jgDrawing.iLsSpaceDimStyle==4)
			sText.Printf("%s","4.����ߴ���");
		else
			sText.Printf("%s","2.�Զ��ж�");
	}
	else if(GetPropID("jgDrawing.nMaxBoltNumAlongX")==id)
		sText.Printf("%d",jgDrawing.nMaxBoltNumAlongX);
	else if(GetPropID("jgDrawing.bDimCutAngle")==id)
	{
		if(jgDrawing.bOneCardMultiPart)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimCutAngleMap")==id)
	{
		if(jgDrawing.bDimCutAngleMap)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimPushFlatMap")==id)
	{
		if(jgDrawing.bDimPushFlatMap)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiHe")==id)
	{
		if(jgDrawing.bDimKaiHe)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiheAngleMap")==id)
	{
		if(jgDrawing.bDimKaiheAngleMap)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiheSumLen")==id)
	{
		if(jgDrawing.bDimKaiheSumLen)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiheAngle")==id)
	{
		if(jgDrawing.bDimKaiheAngle)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiheSegLen")==id)
	{
		if(jgDrawing.bDimKaiheSegLen)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimKaiheScopeMap")==id)
	{
		if(jgDrawing.bDimKaiheScopeMap)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bJgUseSimpleLsMap")==id)
	{
		if(jgDrawing.bJgUseSimpleLsMap)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimLsAbsoluteDist")==id)
	{
		if(jgDrawing.bDimLsAbsoluteDist)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bMergeLsAbsoluteDist")==id)
	{
		if(jgDrawing.bMergeLsAbsoluteDist)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimRibPlatePartNo")==id)
	{
		if(jgDrawing.bDimRibPlatePartNo)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.bDimRibPlateSetUpPos")==id)
	{
		if(jgDrawing.bDimRibPlateSetUpPos)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("jgDrawing.iCutAngleDimType")==id)
	{
		if(jgDrawing.iCutAngleDimType==1)
			sText.Printf("%s","1.��ʽ��");
		else
			sText.Printf("%s","0.��ʽһ");
	}
	else if(GetPropID("jgDrawing.fKaiHeJiaoThreshold")==id)
	{
		sText.Printf("%f",jgDrawing.fKaiHeJiaoThreshold);
		SimplifiedNumString(sText);
	}
	else if(GetPropID("jgDrawing.sAngleCardPath")==id)
		sText.Copy(jgDrawing.sAngleCardPath);
	else if(GetPropID("model.m_sCompanyName")==id)
		sText.Copy(model.m_sCompanyName);
	else if(GetPropID("model.m_sPrjCode")==id)
		sText.Copy(model.m_sPrjCode);
	else if(GetPropID("model.m_sPrjName")==id)
		sText.Copy(model.m_sPrjName);
	else if(GetPropID("model.m_sTaType")==id)
		sText.Copy(model.m_sTaType);
	else if(GetPropID("model.m_sTaAlias")==id)
		sText.Copy(model.m_sTaAlias);
	else if(GetPropID("model.m_sTaStampNo")==id)
		sText.Copy(model.m_sTaStampNo);
	else if(GetPropID("model.m_sOperator")==id)
		sText.Copy(model.m_sOperator);
	else if(GetPropID("model.m_sAuditor")==id)
		sText.Copy(model.m_sAuditor);
	else if(GetPropID("model.m_sCritic")==id)
		sText.Copy(model.m_sCritic);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return sText.Length();
}
void CSysPara::AngleDrawingParaToBuffer(CBuffer &buffer)
{
	buffer.WriteDouble(font.fDimTextSize);
	buffer.WriteInteger(jgDrawing.iDimPrecision);
	buffer.WriteDouble(jgDrawing.fRealToDraw);
	buffer.WriteDouble(jgDrawing.fDimArrowSize);
	buffer.WriteDouble(jgDrawing.fTextXFactor);
	buffer.WriteDouble(font.fPartNoTextSize);
	buffer.WriteInteger(jgDrawing.iPartNoFrameStyle);
	buffer.WriteDouble(jgDrawing.fPartNoMargin);
	buffer.WriteDouble(jgDrawing.fPartNoCirD);
	buffer.WriteDouble(jgDrawing.fPartGuigeTextSize);
	buffer.WriteInteger(jgDrawing.iMatCharPosType);
	buffer.WriteInteger(jgDrawing.bModulateLongJg);
	buffer.WriteInteger(jgDrawing.iJgZoomSchema);
	buffer.WriteInteger(jgDrawing.bMaxExtendAngleLength);
	//buffer.WriteDouble(jgDrawing.fLsDistThreshold);		//�Ǹֳ����Զ�������˨�����ֵ(���ڴ˼��ʱ��Ҫ���е���);
	//buffer.WriteDouble(jgDrawing.fLsDistZoomCoef);		//��˨�������ϵ��
	buffer.WriteInteger(jgDrawing.bOneCardMultiPart);
	buffer.WriteInteger(jgDrawing.iJgGDimStyle);
	buffer.WriteInteger(jgDrawing.nMaxBoltNumStartDimG);
	buffer.WriteInteger(jgDrawing.iLsSpaceDimStyle);
	buffer.WriteInteger(jgDrawing.nMaxBoltNumAlongX);
	buffer.WriteInteger(jgDrawing.bDimCutAngle);
	buffer.WriteInteger(jgDrawing.bDimCutAngleMap);
	buffer.WriteInteger(jgDrawing.bDimPushFlatMap);
	buffer.WriteInteger(jgDrawing.bJgUseSimpleLsMap);
	buffer.WriteInteger(jgDrawing.bDimLsAbsoluteDist);
	buffer.WriteInteger(jgDrawing.bMergeLsAbsoluteDist);
	buffer.WriteInteger(jgDrawing.bDimRibPlatePartNo);
	buffer.WriteInteger(jgDrawing.bDimRibPlateSetUpPos);
	buffer.WriteInteger(jgDrawing.iCutAngleDimType);
	buffer.WriteInteger(jgDrawing.bDimKaiHe);
	buffer.WriteInteger(jgDrawing.bDimKaiheAngleMap);
	buffer.WriteDouble(jgDrawing.fKaiHeJiaoThreshold);
	buffer.WriteInteger(jgDrawing.bDimKaiheSumLen);
	buffer.WriteInteger(jgDrawing.bDimKaiheAngle);
	buffer.WriteInteger(jgDrawing.bDimKaiheSegLen);
	buffer.WriteInteger(jgDrawing.bDimKaiheScopeMap);
}
//////////////////////////////////////////////////////////////////////////
//PPE_LOCALE
//////////////////////////////////////////////////////////////////////////
PPE_LOCALE::PPE_LOCALE()
{

}
void PPE_LOCALE::InitCustomerSerial(UINT uiCustomizeSerial)
{
	XHLOCALE::InitCustomerSerial(uiCustomizeSerial);
	if (uiCustomizeSerial == XHLOCALE::CID_QingDao_HuiJinTong)
	{
		//���˸ְ�ĸ�ӡ��Ϣ
		AddLocaleItemBool("FilterPlateMK", true);
	}
}

PPE_LOCALE gxLocalizer;
CSysPara g_sysPara;