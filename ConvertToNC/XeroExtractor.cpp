#include "stdafx.h"
#include "f_alg_fun.h"
#include "ArrayList.h"
#include "XeroExtractor.h"
#include "CadToolFunc.h"
#include "LogFile.h"
#include "DefCard.h"
#include "ProcessPart.h"
#include "PNCModel.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
//////////////////////////////////////////////////////////////////////////
//
void CSymbolRecoginzer::AppendSymbolEnt(AcDbSpline* pSpline)
{
	if (pSpline == NULL)
		return;
	SYMBOL_ENTITY* pSymbolEnt = listSymbols.AttachObject();
	pSymbolEnt->ciSymbolType = 1;	//������S�ͷ���
	for (int i = 0; i < pSpline->numFitPoints() - 1; i++)
	{
		AcGePoint3d ptS, ptE;
		pSpline->getFitPointAt(i, ptS);
		pSpline->getFitPointAt(i + 1, ptE);
		//����߶�
		GELINE* pLine = pSymbolEnt->listSonlines.AttachObject();
		Cpy_Pnt(pLine->start, ptS);
		Cpy_Pnt(pLine->end, ptE);
	}
}
bool CSymbolRecoginzer::IsHuoquLine(GELINE* pCurveLine,DWORD cbFilterFlag/*=0*/)
{	//ͨ����Ƿ���ʶ�������
	f3dPoint inters;
	for(SYMBOL_ENTITY* pSymbol=listSymbols.EnumObjectFirst();pSymbol;pSymbol=listSymbols.EnumObjectNext())
	{
		if(pSymbol->ciSymbolType!=cbFilterFlag)
			continue;
		int nInters=0;
		for(GELINE* pSonLine=pSymbol->listSonlines.EnumObjectFirst();pSonLine;pSonLine=pSymbol->listSonlines.EnumObjectNext())
		{
			if(Int3dll(f3dLine(pCurveLine->start,pCurveLine->end),f3dLine(pSonLine->start,pSonLine->end),inters)==1)
				nInters++;
		}
		if(nInters>=3)
			return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
//CPlateExtractor
CPlateExtractor::CPlateExtractor()
{
	Init();
}
CPlateExtractor::~CPlateExtractor()
{
	EmptyBoltBlockRecog();
	m_recogSchemaList.Empty();
}
void CPlateExtractor::Init()
{	//���ű�ע����
	m_iDimStyle=0;
	m_sPnKey.Copy("#");
	m_sThickKey.Copy("-");
	m_sMatKey.Copy("Q");
	//Ĭ�ϼ�������ʶ������
	m_recogSchemaList.Empty();
	RECOG_SCHEMA *pSchema = InsertRecogSchema("����1", 0, "#", "Q", "-");
	if (pSchema)
		pSchema->m_bEnable = TRUE;
	InsertRecogSchema("����2", 0, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����1", 1, "#", "Q", "-");
	InsertRecogSchema("����2", 1, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����5", 1, "����:", "����:", "���:");
	InsertRecogSchema("����6", 1, "����:", "����:", "���:", "����:");
	InsertRecogSchema("����7", 1, "����:", "����:", "���:", "����:", "����", "����");
	InsertRecogSchema("����8", 1, "����:", "����:", "���:", "����:", "����", "����");
	InsertRecogSchema("����6", 1, "����:", "����:", "���:", "����");
	InsertRecogSchema("����7", 1, "����:", "����:", "���:", "����", "����", "����");
	InsertRecogSchema("����8", 1, "����:", "����:", "���:", "����", "����", "����");
	//��˨ֱ������
	m_xBoltBlockRecog.InitPlateBoltBlock();
}
RECOG_SCHEMA* CPlateExtractor::InsertRecogSchema(char* name, int dimStyle, char* partNoKey,
	char* matKey, char* thickKey, char* partCountKey /*= NULL*/,
	char* frontBendKey /*= NULL*/, char* reverseBendKey /*= NULL*/)
{
	RECOG_SCHEMA *pSchema1 = m_recogSchemaList.append();
	pSchema1->m_bEditable = FALSE;
	pSchema1->m_bEnable = FALSE;
	pSchema1->m_iDimStyle = dimStyle;
	if (name != NULL)
		pSchema1->m_sSchemaName.Copy(name);
	if (partCountKey != NULL)
		pSchema1->m_sPnKey.Copy(partNoKey);
	if (thickKey != NULL)
		pSchema1->m_sThickKey.Copy(thickKey);
	if (matKey != NULL)
		pSchema1->m_sMatKey.Copy(matKey);
	if (partCountKey != NULL)
		pSchema1->m_sPnNumKey.Copy(partCountKey);
	if (frontBendKey != NULL)
		pSchema1->m_sFrontBendKey.Copy(frontBendKey);
	if (reverseBendKey != NULL)
		pSchema1->m_sReverseBendKey.Copy(reverseBendKey);
	return pSchema1;
}
void CPlateExtractor::ActiveRecogSchema(RECOG_SCHEMA *pSchema)
{
	if (pSchema != NULL)
	{
		m_sPnKey.Copy(pSchema->m_sPnKey);
		m_sMatKey.Copy(pSchema->m_sMatKey);
		m_sThickKey.Copy(pSchema->m_sThickKey);
		m_sPnNumKey.Copy(pSchema->m_sPnNumKey);
		m_sFrontBendKey.Copy(pSchema->m_sFrontBendKey);
		m_sReverseBendKey.Copy(pSchema->m_sReverseBendKey);
		m_iDimStyle = pSchema->m_iDimStyle;
	}
}
int CPlateExtractor::GetKeyMemberNum()
{
	int nNum=0;
	if(m_sPnKey.GetLength()>0)
		nNum++;
	if(m_sThickKey.GetLength()>0)
		nNum++;
	if(m_sMatKey.GetLength()>0)
		nNum++;
	if(m_sPnNumKey.GetLength()>0)
		nNum++;
	return nNum;
}
int CPlateExtractor::GetPnKeyNum(const char* dim_str)
{
	if(strlen(dim_str)<=0)
		return 0;
	if(strstr(dim_str,m_sPnKey)==NULL)
		return 0;
	CXhChar100 sValue(dim_str);
	sValue.Replace(" ","");	//ȡ���ո�
	int n=0;
	if(m_sPnKey.GetLength()==1)
	{
		for(int i=0;i<(int)strlen(dim_str);i++)
		{
			if(dim_str[i]==m_sPnKey[0])
				n++;
		}
	}
	else
	{
		CString ss(sValue);
		if(ss.Find(m_sPnKey)>=0)
			n=1;
	}
	return n;
}
BOOL CPlateExtractor::IsMatchPNRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sPnKey)<=0)
		return FALSE;
	if(strstr(sText,m_sPnKey)==NULL)
		return FALSE;
	if(m_iDimStyle==1&&GetPnKeyNum(sText)!=1)	//���б�ע����������ֻ����һ��#�ؼ���
		return FALSE;
	if(m_iDimStyle==0)	//���б�ע��������������
	{
		if(strlen(m_sThickKey)>0 && strstr(sText,m_sThickKey)==NULL)
			return FALSE;
		if(strlen(m_sMatKey)>0 && strstr(sText,m_sMatKey)==NULL)
			return FALSE;
		if (strlen(m_sPnNumKey) > 0)
		{
			if (strstr(m_sPnNumKey, "|"))
			{	//�����ؼ���֧�ֶ������"��|��"
				CXhChar50 sNumKey = m_sPnNumKey;
				char* sKey = NULL;
				for (sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
				{
					if(strstr(sText, sKey))
						break;
				}
				if (sKey == NULL)
					return FALSE;
			}
			else if(strstr(sText, m_sPnNumKey) == NULL)
				return FALSE;
		}
	}
	else	//���б�ע��������
	{	//���б�ע�и����ض����ֽ����ų�(�������ڴ˲�����)
		//if(strstr(sText,"��")||strstr(sText,"��")||strstr(sText,"��"))
			//return FALSE;
	}
	return TRUE;
}
BOOL CPlateExtractor::IsMatchThickRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sThickKey)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	if(strstr(sText,m_sThickKey))
		return TRUE;
	else
		return FALSE;
}
//���ʹؼ��ֿ��Բ����ã�Ĭ��Ϊ"Q2"|"Q3"|"Q4" wxc-2020.11.3
BOOL CPlateExtractor::IsMatchMatRule(const char* sText)
{
	if(strlen(sText)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	if((strlen(m_sMatKey)>0 && strstr(sText,m_sMatKey))||
		(strstr(sText, "Q2")||strstr(sText,"Q3")||strstr(sText,"Q4")))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsMatchNumRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sPnNumKey)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	CXhChar50 sNumKey = m_sPnNumKey;
	for (char* sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
	{
		if (strstr(sText, sKey))
			return TRUE;
	}
	return FALSE;
}
BOOL CPlateExtractor::IsMatchRollEdgeRule(const char* sText)
{
	if (strstr(sText, "�������90") || strstr(sText, "�������90"))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsMatchBendRule(const char* sText)
{
	if(strlen(sText)<=0||(strlen(m_sFrontBendKey)<=0&&strlen(m_sReverseBendKey)<=0))
		return FALSE;
	else if(strstr(sText,m_sFrontBendKey)||strstr(sText,m_sReverseBendKey))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsBriefMatMark(char cMat)
{
	if ('H' == cMat || 'h' == cMat || 'G' == cMat ||
		'P' == cMat || 'T' == cMat || 'S' == cMat)
		return TRUE;
	else
		return FALSE;
}
BYTE CPlateExtractor::ParsePartNoText(const char* sText,CXhChar16& sPartNo)
{
	CString ss(sText);
	ss.Replace("��"," ");
	if (strstr(m_sPnKey, "#"))
	{	//#��ʶ���ڼ��ź���
		int iPos = ss.Find(m_sPnKey);
		if (iPos > 1 && ss[iPos - 1] == ' ')
			ss.Delete(iPos - 1);
		ss.Replace(m_sPnKey, "| ");
	}
	else
		ss.Replace(m_sPnKey,"|");
	CXhChar100 sPrevStr, str, sValue(ss.GetBuffer());
	BOOL bHasWeldFlag = FALSE;
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if (strstr(sKey, "|"))
		{	//��ȡ����
			str.Copy(sKey);
			str.Replace("|", "");
			str.Remove(' ');
			//���ݼ����д��ո����������磺101 H,��ȡΪ101H �� H 101,��ȡΪH101�� wht 19-07-22
			if (str.GetLength() == 1 && IsBriefMatMark(str[0]) && sPrevStr.GetLength() > 0)
				sPartNo.Printf("%s%s", (char*)sPrevStr, (char*)str);
			else if (sPrevStr.GetLength() == 1 && IsBriefMatMark(str[0]) && str.GetLength() > 0)
				sPartNo.Printf("%s%s", (char*)sPrevStr, (char*)str);
			else
				sPartNo.Copy(str);
			if (sPartNo.GetLength() > 0)
			{
				if(bHasWeldFlag)
					return PART_LABEL_WELD;
				else
					return PART_LABEL_VALID;
			}
			else
				return PART_LABEL_EMPTY;
		}
		else
		{
			sPrevStr.Copy(sKey);
			if (strstr(sKey, "��"))
				bHasWeldFlag = TRUE;
		}
	}
	return PART_LABEL_EMPTY;
}
void CPlateExtractor::ParseThickText(const char* sText,int& nThick)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sThickKey)==NULL||strstr(sKey,"|"))
			continue;
		//�鿴��ȱ�ʶ����λ��
		UINT i,index=0;
		for(i=0;i<strlen(sKey);i++)
		{
			if(sKey[i]==m_sThickKey[0])
			{
				index=i;
				break;
			}
		}
		if(index>0 && sKey[index-1]!=' ')
		{	//��ȱ�ʶ�����ַ����м�
			if(strstr(sKey,"Q")==NULL)
				continue;	//
			CXhChar50 sGroupStr(sKey),sMat,sThick;
			sGroupStr.Replace(m_sThickKey," ");
			sscanf(sGroupStr,"%s%s",(char*)sMat,(char*)sThick);
			sprintf(sKey,"%s%s",(char*)m_sThickKey,(char*)sThick);
		}
		//�����ַ���
		str.Copy(sKey);
		if(strstr(str,"mm"))
			str.Replace("mm","");
		str.Replace(m_sThickKey,"| ");
		int nValue = 0;
		sscanf(str,"%s%d",(char*)sValue,&nValue);
		if (nValue > 0)
			nThick = nValue;
	}
}
void CPlateExtractor::ParseMatText(const char* sText,char& cMat,char& cQuality)
{
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	CString str(sValue);
	if (strstr(str, "Q") == NULL)	//���ʱ�ʶ��û��Qʱ����Ҫ���ؼ����滻ΪQ wht 19-10-22
		str.Replace(m_sMatKey, " Q");
	sValue.Copy(str);
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if (cMat == 0 && strstr(sKey, "Q") && strstr(sKey, "|") == NULL)
		{
			char cMark = CProcessPart::QueryBriefMatMark(sKey);
			cQuality = CProcessPart::QueryBriefQuality(sKey);
			cMat = (cMark == 'A') ? 0 : cMark;
		}
	}
}
void CPlateExtractor::ParseNumText(const char* sText,int& nNum)
{
	//��ȡ���������ɸ��ؼ���
	CXhChar50 sNumKey = m_sPnNumKey;
	std::vector<CXhChar16> numKeyArr;
	for (char* sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
		numKeyArr.push_back(CXhChar16(sKey));
	//�����ַ�������ȡ����ֵ
	CString ss(sText);
	if (strstr(m_sPnNumKey, "��")|| strstr(m_sPnNumKey, "��"))
	{	//������ʶǰ�����пո�
		for (size_t i = 0; i < numKeyArr.size(); i++)
		{
			if (strstr(ss, numKeyArr[i]) == NULL)
				continue;
			int iPos = ss.Find(numKeyArr[i]);
			if (iPos > 1 && ss[iPos - 1] == ' ')
				ss.Delete(iPos - 1);
		}
	}
	CXhChar100 str, sValue(ss.GetBuffer());
	sValue.Replace("��"," ");
	if(m_iDimStyle==0 || strstr(sValue,m_sPnKey))
		sValue.Replace(m_sPnKey,"| ");
	for(char* sSubStr=strtok(sValue," \t"); sSubStr; sSubStr =strtok(NULL," \t"))
	{
		for (size_t i = 0; i < numKeyArr.size(); i++)
		{
			if (strstr(sSubStr, numKeyArr[i]))
			{
				str.Copy(sSubStr);
				str.Replace(numKeyArr[i], "");
				nNum = atoi(str);
				return;
			}
		}
	}
}
bool CPlateExtractor::ParseBendText(const char* sText,double &degree,BOOL &bFrontBend)
{
	if (strstr(sText, m_sFrontBendKey))
		bFrontBend = TRUE;
	else if (strstr(sText, m_sReverseBendKey))
		bFrontBend = FALSE;
	else
		return false;
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	sValue.Replace(m_sFrontBendKey,"");
	sValue.Replace(m_sReverseBendKey,"");
	sValue.Replace("���", "");
	sValue.Replace("��"," ");
	sValue.Replace("��"," ");
	char* sKey = strtok(sValue, " ");
	if (strlen(sKey) > 0)
	{
		degree = atof(sKey);
		return true;
	}
	else
		degree=0;
	return false;
}
bool CPlateExtractor::ParseRollEdgeText(const char* sText, double& degree, BOOL& bFrontBend)
{
	if (strstr(sText, "���90") == NULL)
		return false;
	else
		return ParseBendText(sText, degree, bFrontBend);
}
BOOL CPlateExtractor::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=FALSE;
	if(pRecognizer!=NULL)
	{	//����ֱ����SPLINE�ֶ����󽻽����ж�
		AcGePoint3d startPt,endPt;
		pAcDbLine->getStartPoint(startPt);
		pAcDbLine->getEndPoint(endPt);
		GELINE line;
		Cpy_Pnt(line.start, startPt);
		Cpy_Pnt(line.end, endPt);
		bRet=pRecognizer->IsHuoquLine(&line,0x01);
	}
	if(!bRet)
	{	//TODO:����Ӧ����ͨ��ͼ���������ʶ������� wjh-2017.6.3
	
	}
	return bRet;
}
BOOL CPlateExtractor::IsSlopeLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=FALSE;
	AcDbObjectId lineTypeId = GetEntLineTypeId(pAcDbLine);
	CXhChar50 sLineType = GetLineTypeName(lineTypeId);
	//������ʣ�����������ΪTRACKS wht 20-09-24
	if (sLineType.GetLength() > 0 && sLineType.EqualNoCase("TRACKS"))
		return TRUE;
	if(pRecognizer!=NULL)
	{
		
	}
	return bRet;
}
BOOL CPlateExtractor::ParsePartNoText(AcDbEntity *pAcadText, CXhChar16& sPartNo)
{
	if (pAcadText == NULL)
		return FALSE;
	CXhChar500 sText;
	if (pAcadText->isKindOf(AcDbText::desc()))
		sText = GetCadTextContent(pAcadText);
	else if (pAcadText->isKindOf(AcDbMText::desc()))
	{
		sText = GetCadTextContent(pAcadText);
		if (sText.GetLength() <= 0)
			return FALSE;
		//�˴�ʹ��\P�ָ���ܻ���2310P�еĲ����ַ�Ĩȥ����Ҫ���⴦��\P�滻\W wht 19-09-09
		CXhChar200 sNewText;
		char cPreChar = sText.At(0);
		sNewText.Append(cPreChar);
		for (int i = 1; i < sText.GetLength(); i++)
		{
			char cCurChar = sText.At(i);
			if (cPreChar == '\\'&&cCurChar == 'P')
				sNewText.Append('W');
			else
				sNewText.Append(cCurChar);
			cPreChar = cCurChar;
		}
		sText.Copy(sNewText);
		//
		vector<CString> lineTextArr;
		for (char* sKey = strtok(sText, "\\W"); sKey; sKey = strtok(NULL, "\\W"))
		{
			CString sTemp(sKey);
			sTemp.Replace("\\W", "");
			lineTextArr.push_back(sTemp);
		}
		if (lineTextArr.size() > 0)
		{
			for(size_t i=0;i<lineTextArr.size();i++)
			{
				CString ss = lineTextArr.at(i);
				if (IsMatchPNRule(ss))
				{
					sText.Copy(ss);
					break;
				}
			}
		}
	}
	if (!IsMatchPNRule(sText))
		return FALSE;
	BYTE ciRetCode = ParsePartNoText(sText, sPartNo);
	if (ciRetCode == CPlateExtractor::PART_LABEL_WELD)
		return FALSE;	//��ǰ����Ϊ�����Ӽ����� wht 19-07-22
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//CJgCardExtractor
#ifdef __UBOM_ONLY_
CJgCardExtractor::CJgCardExtractor()
{
	fTextHigh = 0;
	fPnDistX = 0;
	fPnDistY = 0;
}
CJgCardExtractor::~CJgCardExtractor()
{

}
BYTE CJgCardExtractor::InitJgCardInfo(const char* sJgCardPath)
{
	if (strlen(sJgCardPath) <= 0)
		return CARD_READ_FAIL;
	CLockDocumentLife lockCurDocment;
	PRESET_ARRLIST(CXhChar100, partNoPosArr, 10);
	AcDbDatabase blkDb(Adesk::kFalse);//����յ����ݿ�
	Acad::ErrorStatus retCode;
#ifdef _ARX_2007
	if ((retCode=blkDb.readDwgFile((ACHAR*)_bstr_t(sJgCardPath), _SH_DENYRW, true)) == Acad::eOk)
#else
	if ((retCode=blkDb.readDwgFile(sJgCardPath, _SH_DENYRW, true)) == Acad::eOk)
#endif
	{
		AcDbBlockTable *pTempBlockTable = NULL;
		blkDb.getBlockTable(pTempBlockTable, AcDb::kForRead);
		AcDbBlockTableRecord *pTempBlockTableRecord = NULL;
		pTempBlockTable->getAt(ACDB_MODEL_SPACE, pTempBlockTableRecord, AcDb::kForRead);
		pTempBlockTable->close();
		if (pTempBlockTableRecord == NULL)
			return CARD_READ_FAIL;
		SCOPE_STRU scope;
		AcDbBlockTableRecordIterator *pIterator = NULL;
		pTempBlockTableRecord->newIterator(pIterator);
		for (; !pIterator->done(); pIterator->step())
		{
			AcDbEntity *pEnt = NULL;
			pIterator->getEntity(pEnt, AcDb::kForRead);
			if(pEnt==NULL)
				continue;
			pEnt->close();
			if (pEnt->isKindOf(AcDbLine::desc()))
			{
				AcDbLine* pLine = (AcDbLine*)pEnt;
				scope.VerifyVertex(GEPOINT(pLine->startPoint().x,pLine->startPoint().y));
				scope.VerifyVertex(GEPOINT(pLine->endPoint().x, pLine->endPoint().y));
				continue;
			}
			else if (pEnt->isKindOf(AcDbPolyline::desc()))
			{
				AcDbPolyline *pPline = (AcDbPolyline*)pEnt;
				int nNum = pPline->numVerts();
				for (int indx = 0; indx < nNum; indx++)
				{
					AcGePoint3d location;
					pPline->getPointAt(indx, location);
					scope.VerifyVertex(GEPOINT(location.x, location.y));
				}
				continue;
			}
			//��¼���տ�ģ���еļ���λ��(��Ϊ���տ��ı�ǵ�)�����ڴ����ͼ�����нǸֹ��տ���������
			if (pEnt->isKindOf(AcDbMText::desc()) || pEnt->isKindOf(AcDbText::desc()))
			{
				CXhChar100 sText=GetCadTextContent(pEnt);
				if (g_xUbomModel.IsPartLabelTitle(sText))
				{
					GEPOINT pt = GetCadTextDimPos(pEnt);
					if (fPnDistX<pt.x || fPnDistY>pt.y)
					{	//
						fPnDistX = pt.x;
						fPnDistY = pt.y;
					}
					partNoPosArr.Append(CXhChar100("%s λ��(%.1f,%.1f)", (char*)sText, pt.x, pt.y));
				}
				continue;
			}
			if (!pEnt->isKindOf(AcDbPoint::desc()))
				continue;
			GRID_DATA_STRU grid_data;
			if (!GetGridKey((AcDbPoint*)pEnt, &grid_data))
				continue;
			if (grid_data.data_type == 3)
			{	//��ͼ����
				draw_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				draw_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else
			{	//���ݵ�����
				mapJgCardRect[grid_data.type_id].topLeft.Set(grid_data.min_x, grid_data.max_y);
				mapJgCardRect[grid_data.type_id].bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
		}
		pTempBlockTableRecord->close();
		//���տ���������
		frame_rect.topLeft.Set(scope.fMinX, scope.fMaxY);
		frame_rect.bottomRight.Set(scope.fMaxX, scope.fMinY);
		//��ʾ��Ϣ
		if (partNoPosArr.GetSize() > 1)
		{
			logerr.LogString(CXhChar100("���տ�ͼ��{%s}�д��ڶ���ļ�,��ȷ�ϣ�", (char*)sJgCardPath));
			for (CXhChar100 *pPartPos = partNoPosArr.GetFirst(); pPartPos; pPartPos = partNoPosArr.GetNext())
				logerr.LogString(*pPartPos);
			logerr.ShowToScreen();
			return CARD_READ_ERROR_PARTNO;
		}
		else
			return CARD_READ_SUCCEED;
	}
	return CARD_READ_FAIL;
}
f3dPoint CJgCardExtractor::GetJgCardOrigin(f3dPoint partNo_pt)
{
	return f3dPoint(partNo_pt.x - fPnDistX, partNo_pt.y - fPnDistY, 0);
}
#endif
