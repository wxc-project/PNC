#pragma once
#include "XhCharString.h"

class CConverter
{
public:
	CConverter(void);
	~CConverter(void);

	static CXhChar200 ToString(int v);
	static CXhChar200 ToString(double v);
	static CXhChar200 ToString(int enumId,char* itemArr);
	static CXhChar200 ToString(bool v);
	static CXhChar200 ToString(char* v);
	//
	static int ToInt(char* v);
	static double ToDouble(char* v);
	static int ToEnumId(char* v,char* itemArr);
	static BOOL ToBoolean(char* v);
};