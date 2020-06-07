#include "StdAfx.h"
#include "CadObjLife.h"


//////////////////////////////////////////////////////////////////////////
// CAcadSysVarLife
CAcadSysVarLife::CAcadSysVarLife(char* var, int value)
{
	if (var == NULL)
		return;
	m_nVarOldValue = m_nVarValue = -1;
	m_bRetCode = FALSE;
	struct resbuf result;
	int retCode = 0;
#ifdef _ARX_2007
	retCode = acedGetVar(_bstr_t(var), &result);	//取得当前图层
#else
	retCode = acedGetVar(var, &result);
#endif
	if (RTNORM == retCode)
	{
		m_bRetCode = TRUE;
		m_sVarName.Copy(var);
		m_nVarValue = value;
		restype = result.restype;
		if (result.restype == RTSHORT)
		{
			m_nVarOldValue = result.resval.rint;
			result.resval.rint = m_nVarValue;
		}
		else if (result.restype == RTLONG)
		{
			m_nVarOldValue = result.resval.rlong;
			result.resval.rint = m_nVarValue;
		}
		else
			return;
#ifdef _ARX_2007
		retCode = acedSetVar(_bstr_t(m_sVarName), &result);
#else
		retCode = acedSetVar(m_sVarName, &result);
#endif
		m_bRetCode = (retCode == RTNORM);
	}
}
CAcadSysVarLife::~CAcadSysVarLife()
{
	if (m_bRetCode && m_nVarValue != m_nVarOldValue)
	{
		struct resbuf result;
		result.restype = restype;
		if (restype == RTLONG)
			result.resval.rlong = m_nVarOldValue;
		else if (restype == RTSHORT)
			result.resval.rint = m_nVarOldValue;
		else
			return;
#ifdef _ARX_2007
		acedSetVar(_bstr_t(m_sVarName), &result);
#else
		acedSetVar(m_sVarName, &result);
#endif
	}
}
