#pragma once
#include "CadToolFunc.h"

class CShiftDocSDIPara
{
public:
	CShiftDocSDIPara() {
#ifdef _ARX_2007
		SendCommandToCad(L"SDI 1\n");
#else
		SendCommandToCad("SDI 1\n");
#endif
	}
	~CShiftDocSDIPara() {
#ifdef _ARX_2007
		SendCommandToCad(L"SDI 0\n");
#else
		SendCommandToCad("SDI 0\n");
#endif
	}
};

class CCAdGlobalParaShift
{
	CXhChar100 m_sParamName;
	int m_nSrcValue;
public:
	CCAdGlobalParaShift(const char *paramName, int value) {
		if (paramName == NULL)
			return;
		m_sParamName.Copy(paramName);
		if (value == 0)
			m_nSrcValue = 1;
		else
			m_nSrcValue = 0;
#ifdef _ARX_2007
		SendCommandToCad((ACHAR*)_bstr_t(CXhChar100("%s %d\n", paramName, value)));
#else
		SendCommandToCad(CXhChar100("%s %d\n", paramName, value));
#endif
	}
	~CCAdGlobalParaShift() {
		if (m_sParamName.GetLength() > 0)
#ifdef _ARX_2007
			SendCommandToCad((ACHAR*)_bstr_t(CXhChar100("%s %d\n", (char*)m_sParamName, m_nSrcValue)));
#else
			SendCommandToCad(CXhChar100("%s %d\n", (char*)m_sParamName, m_nSrcValue));
#endif
	}
};

class CAcadSysVarLife
{
	CXhChar100 m_sVarName;
	int m_nVarValue;
	int m_nVarOldValue;
	BOOL m_bRetCode;
	short restype;
public:
	CAcadSysVarLife(char* var, int value);
	~CAcadSysVarLife();
};

