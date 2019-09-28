#include "stdafx.h"

void splitVersion(CString& sVersion, UINT& nMainVer,
							 UINT& nVice1Ver, UINT& nVice2Ver, UINT& nVice3Ver)
{
	int N;
	CString version,str;
	nMainVer = nVice1Ver = nVice2Ver = nVice3Ver = 0;
	version = sVersion;//sVersion.SpanExcluding("/");
	str = version.SpanExcluding(".");
	nMainVer = atoi(str);
	N = version.GetLength() - str.GetLength()-1;
	if(N<=0)
		return;

	version = version.Right(N);
	str = version.SpanExcluding(".");
	nVice1Ver = atoi(str);
	N = version.GetLength() - str.GetLength()-1;
	if(N<=0)
		return;

	version = version.Right(N);
	str = version.SpanExcluding(".");
	nVice2Ver = atoi(str);
	N = version.GetLength() - str.GetLength()-1;
	if(N<=0)
		return;
	version = version.Right(N);
	str = version.SpanExcluding(".");
	nVice3Ver = atoi(str);
	N = version.GetLength() - str.GetLength()-1;
}
//大于零表示版本号1高,等于零表示同版本，否则表示版本号1低
int compareVersion(CString sVersion1,CString sVersion2)
{
	UINT nMainVer[2], nVice1Ver[2], nVice2Ver[2], nVice3Ver[2];
	if(sVersion1.GetLength()==0)
		return 0;	//空版本号默认返回值为0
	splitVersion(sVersion1,nMainVer[0], nVice1Ver[0],
		nVice2Ver[0], nVice3Ver[0]);
	splitVersion(sVersion2, nMainVer[1], nVice1Ver[1],
		nVice2Ver[1], nVice3Ver[1]);	//当前塔的版本号
	if(nMainVer[0]>nMainVer[1])//0.比较发行版本号
		return 1;
	else if(nMainVer[0]<nMainVer[1])
		return -1;
	else
	{//1.0
		if(nVice1Ver[0]>nVice1Ver[1])
			return 1;
		else if(nVice1Ver[0]<nVice1Ver[1])
			return -1;
		else
		{//1.1
			if(nVice2Ver[0]>nVice2Ver[1])
				return 1;
			else if(nVice2Ver[0]<nVice2Ver[1])
				return -1;
			else
			{
				if(nVice3Ver[0]>nVice3Ver[1])
					return 1;
				else if(nVice3Ver[0]<nVice3Ver[1])
					return -1;
				else
					return 0;
			}
		}
	}
}
