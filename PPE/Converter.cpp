#include "stdafx.h"
#include "Converter.h"
#ifndef _LEGACY_LICENSE
#include "XhLdsLm.h"
#else
#include "Lic.h"
#endif

CConverter::CConverter(void)
{
}

CConverter::~CConverter(void)
{
}

CXhChar200 CConverter::ToString(int v)
{
	return CXhChar200("%d",v); 
}
CXhChar200 CConverter::ToString(double v)
{
	CXhChar200 sValue("%f",v);
	SimplifiedNumString(sValue);
	return sValue;
}
CXhChar200 CConverter::ToString(int enumId,char* itemArr)
{
	CXhChar200 sEnum;
	int i=0,i2=0,iCur=-1;
	CString sItemArr=itemArr;
	while ((i2=sItemArr.Find('|',i)) != -1)
	{
		CString sItem=sItemArr.Mid(i,i2-i);
		i=i2+1;
		iCur++;
		if(iCur==enumId)
			sEnum.Printf("%s",sItem);
	}
	return sEnum;
}
CXhChar200 CConverter::ToString(bool v)
{
	if(v)
		return CXhChar200("ÊÇ");
	else 
		return CXhChar200("·ñ");
}
CXhChar200 CConverter::ToString(char* v)
{
	return CXhChar200("%s",v);
}

int CConverter::ToInt(char* v)
{
	return atoi(v);
}
double CConverter::ToDouble(char* v)
{
	return atof(v);
}
int CConverter::ToEnumId(char* v,char* itemArr)
{
	int i=0,i2=0,enumId=0;
	CString sItemArr=itemArr;
	while ((i2=sItemArr.Find('|',i)) != -1)
	{
		CString sItem=sItemArr.Mid(i,i2-i);
		i=i2+1;
		enumId++;
		if(sItem.CompareNoCase(v))
			return enumId;
	}
	return -1;
}
BOOL CConverter::ToBoolean(char* v)
{
	if(stricmp(v,"ÊÇ")==0)
		return TRUE;
	else 
		return FALSE;
}