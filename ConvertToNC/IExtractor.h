#pragma once
#include "f_ent.h"
#include "XhCharString.h"

class AcDbLine;
class AcDbEntity;

struct BASIC_INFO{
	char m_cMat;
	char m_cQuality;			//质量等级
	int m_nThick;
	int m_nNum;
	long m_idCadEntNum;
	CXhChar16 m_sPartNo;
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaAlias;		//代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar200 m_sBoltStr;		//螺栓字符串
	CXhChar100 m_sTaskNo;		//任务号
	BASIC_INFO() { m_nThick = m_nNum = 0; m_cMat = 0; m_cQuality = 0; m_idCadEntNum = 0; }
};
struct BOLT_HOLE{
	double d;			//螺栓名称直径如M20螺栓时d=20
	BYTE ciSymbolType;	//0.图块;1.圆孔
	float increment;	//螺栓孔比螺栓名义直径的增大间隙值
	float posX,posY;	//
	BOLT_HOLE() {
		posX = posY = increment = ciSymbolType = 0; d = 0;
	}
};
struct ICurveLine{
	static const BYTE STRAIGHT		=1;
	static const BYTE ARCLINE		=2;
	static const BYTE ELLIPSE		=3;
	BYTE ciCurveType;	//1.直线;2.圆弧;3.椭圆弧
	GEPOINT start,end;//const double* start,const double* end
	ICurveLine(){ciCurveType=1;}
};
struct ISymbolRecognizer{
	virtual bool IsIntersWith(ICurveLine* pCurveLine,DWORD cbFilterFlag=0)=0;
	virtual bool IsWeldingAlongSide(ICurveLine* pCurveLine)=0;
};

