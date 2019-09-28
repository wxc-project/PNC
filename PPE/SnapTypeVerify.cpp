#include "StdAfx.h"
#include "XhLicAgent.h"
#include "IXeroCad.h"
#include "SnapTypeVerify.h"

CSnapTypeVerify::CSnapTypeVerify(int provider/*=2*/,DWORD dwVerifyFlag/*=0xffffffff*/)
{
	m_dwDrawingSpaceFlag=m_dwSolidSpaceFlag=m_dwLineSpaceFlag=0;
	if(provider==1)
		m_dwDrawingSpaceFlag=dwVerifyFlag;	//provider=1
	else if(provider==2)
		m_dwSolidSpaceFlag=dwVerifyFlag;	//provider=2
	else if(provider==3)
		m_dwLineSpaceFlag=dwVerifyFlag;	//provider=3
}
DWORD CSnapTypeVerify::SetVerifyFlag(int provider,DWORD dwFlag/*=0*/)
{
	if(provider==1)
		return m_dwDrawingSpaceFlag=dwFlag;	//provider=1
	else if(provider==2)
		return m_dwSolidSpaceFlag=dwFlag;	//provider=2
	else if(provider==3)
		return m_dwLineSpaceFlag=dwFlag;	//provider=3
	return 0;
}
DWORD CSnapTypeVerify::AddVerifyFlag(int provider,DWORD dwFlag)
{
	if(provider==1)
	{
		m_dwDrawingSpaceFlag|=dwFlag;	//provider=1
		return m_dwDrawingSpaceFlag;
	}
	else if(provider==2)
	{
		m_dwSolidSpaceFlag|=dwFlag;	//provider=2
		return m_dwSolidSpaceFlag;
	}
	else if(provider==3)
	{
		m_dwLineSpaceFlag|=dwFlag;	//provider=3
		return m_dwLineSpaceFlag;
	}
	return 0;
}
DWORD CSnapTypeVerify::AddVerifyType(int provider,int iObjClsType)
{
	if(provider==1)
	{
		DWORD dwFlag=0;
		if(iObjClsType==IDbEntity::DbPoint)	// = 1;
			dwFlag=GetSingleWord(IDbEntity::DbPoint)>0;
		else if(iObjClsType==IDbEntity::DbLine)		// = 2;
			dwFlag=GetSingleWord(IDbEntity::DbLine)>0;
		else if(iObjClsType==IDbEntity::DbArcline)	// = 3;
			dwFlag=GetSingleWord(IDbEntity::DbArcline)>0;
		else if(iObjClsType==IDbEntity::DbRect)	// = 4;
			dwFlag=GetSingleWord(IDbEntity::DbRect)>0;
		else if(iObjClsType==IDbEntity::DbArrow)	// = 5;
			dwFlag=GetSingleWord(IDbEntity::DbArrow)>0;
		else if(iObjClsType==IDbEntity::DbMark)		// = 6;
			dwFlag=GetSingleWord(IDbEntity::DbMark)>0;
		else if(iObjClsType==IDbEntity::DbPolyline)	// = 7;
			dwFlag=GetSingleWord(IDbEntity::DbPolyline)>0;
		else if(iObjClsType==IDbEntity::DbCircle)	// = 8;
			dwFlag=GetSingleWord(IDbEntity::DbCircle)>0;
		else if(iObjClsType==IDbEntity::DbSpline)	// = 9;
			dwFlag=GetSingleWord(IDbEntity::DbSpline)>0;
		else if(iObjClsType==IDbEntity::DbText)		// =10;
			dwFlag=GetSingleWord(IDbEntity::DbText)>0;
		else if(iObjClsType==IDbEntity::DbMText)	// =11;
			dwFlag=GetSingleWord(IDbEntity::DbMText)>0;
		else if(iObjClsType==IDbEntity::DbDiametricDimension)	// =12;
			dwFlag=GetSingleWord(IDbEntity::DbDiametricDimension)>0;
		else if(iObjClsType==IDbEntity::DbRadialDimension)		// =13;
			dwFlag=GetSingleWord(IDbEntity::DbRadialDimension)>0;
		else if(iObjClsType==IDbEntity::DbAngularDimension)		// =14;
			dwFlag=GetSingleWord(IDbEntity::DbAngularDimension)>0;
		else if(iObjClsType==IDbEntity::DbAlignedDimension)		// =15;
			dwFlag=GetSingleWord(IDbEntity::DbAlignedDimension)>0;
		else if(iObjClsType==IDbEntity::DbRotatedDimension)		// =16;
			dwFlag=GetSingleWord(IDbEntity::DbRotatedDimension)>0;
		m_dwDrawingSpaceFlag|=dwFlag;	//provider=1
		return m_dwDrawingSpaceFlag;
	}
	/*else if(provider==2)
	{
		DWORD dwFlag=0;
		switch(iObjClsType)
		{
		case CLS_PART:
			dwFlag=SELECT_PART;
			break;
		case CLS_LINEPART:
			dwFlag=SELECT_LINEPART;
			break;
		case CLS_ARCPART:
			dwFlag=SELECT_ARCPART;
			break;	
		case CLS_LINEANGLE:
			dwFlag=GetSingleWord(SELECTINDEX_LINEANGLE);
			break;
		case CLS_LINETUBE:
			dwFlag=GetSingleWord(SELECTINDEX_LINETUBE);
			break;
		case CLS_LINEFLAT:
			dwFlag=GetSingleWord(SELECTINDEX_LINEFLAT);
			break;
		case CLS_LINESLOT:
			dwFlag=GetSingleWord(SELECTINDEX_LINESLOT);
			break;
		case CLS_PLATE:
			dwFlag=GetSingleWord(SELECTINDEX_PLATE);
			break;
		case CLS_PARAMPLATE:
			dwFlag=GetSingleWord(SELECTINDEX_PARAMPLATE);
			break;
		case CLS_BOLT:
			dwFlag=GetSingleWord(SELECTINDEX_BOLT);
			break;
		case CLS_SPHERE:
			dwFlag=GetSingleWord(SELECTINDEX_SPHERE);
			break;
		case CLS_ARCANGLE:
			dwFlag=GetSingleWord(SELECTINDEX_ARCANGLE);
			break;
		case CLS_ARCSLOT:
			dwFlag=GetSingleWord(SELECTINDEX_ARCSLOT);
			break;
		case CLS_ARCFLAT:
			dwFlag=GetSingleWord(SELECTINDEX_ARCFLAT);
			break;
		case CLS_ARCTUBE:
			dwFlag=GetSingleWord(SELECTINDEX_ARCTUBE);
			break;
		default:
			dwFlag=0;
			break;
		}
		m_dwSolidSpaceFlag|=dwFlag;	//provider=2
		return m_dwSolidSpaceFlag;
	}*/
	else if(provider==3)
	{
		DWORD dwFlag=0;
		switch(iObjClsType)
		{
		case AtomType::prPoint:	//0x00000001
			dwFlag=SNAP_POINT;
			break;
		case AtomType::prLine:	//0x00000002
			dwFlag=SNAP_LINE;
			break;
		case AtomType::prCircle://0x00000008
			dwFlag=SNAP_CIRCLE;
			break;
		case AtomType::prRect:	//0x00000010
			dwFlag=SNAP_RECTANGLE;
			break;
		case AtomType::prPolyFace:	//0x00000020
			dwFlag=SNAP_POLYGON;
			break;
		case AtomType::prArc:	//0x00000040
			dwFlag=SNAP_ARC;
			break;
		default:
			dwFlag=0;
			break;
		}
		m_dwLineSpaceFlag|=dwFlag;	//provider=3
		return m_dwLineSpaceFlag;
	}
	else
		return 0;
}
bool CSnapTypeVerify::IsValidObjType(char ciProvider,int iObjClsType)
{
	if(ciProvider==1)
		return IsValidEntityObjType(iObjClsType);
	else if(ciProvider==2)
		return IsValidSolidObjType(iObjClsType);
	else if(ciProvider==3)
		return IsValidGeObjType(iObjClsType);
	return false;
}
bool CSnapTypeVerify::IsValidEntityObjType(int iObjClsType){
	if(iObjClsType==IDbEntity::DbPoint)	// = 1;
		return (GetSingleWord(IDbEntity::DbPoint)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbLine)		// = 2;
		return (GetSingleWord(IDbEntity::DbLine)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbArcline)	// = 3;
		return (GetSingleWord(IDbEntity::DbArcline)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbRect)	// = 4;
		return (GetSingleWord(IDbEntity::DbRect)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbArrow)	// = 5;
		return (GetSingleWord(IDbEntity::DbArrow)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbMark)		// = 6;
		return (GetSingleWord(IDbEntity::DbMark)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbPolyline)	// = 7;
		return (GetSingleWord(IDbEntity::DbPolyline)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbCircle)	// = 8;
		return (GetSingleWord(IDbEntity::DbCircle)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbSpline)	// = 9;
		return (GetSingleWord(IDbEntity::DbSpline)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbText)		// =10;
		return (GetSingleWord(IDbEntity::DbText)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbMText)	// =11;
		return (GetSingleWord(IDbEntity::DbMText)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbDiametricDimension)	// =12;
		return (GetSingleWord(IDbEntity::DbDiametricDimension)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbRadialDimension)		// =13;
		return (GetSingleWord(IDbEntity::DbRadialDimension)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbAngularDimension)		// =14;
		return (GetSingleWord(IDbEntity::DbAngularDimension)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbAlignedDimension)		// =15;
		return (GetSingleWord(IDbEntity::DbAlignedDimension)&m_dwDrawingSpaceFlag)>0;
	else if(iObjClsType==IDbEntity::DbRotatedDimension)		// =16;
		return (GetSingleWord(IDbEntity::DbRotatedDimension)&m_dwDrawingSpaceFlag)>0;
	else
		return false;
}
bool CSnapTypeVerify::IsValidSolidObjType(int iObjClsType){
	/*switch(iObjClsType)
	{
	case CLS_LINEANGLE:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_LINEANGLE))>0;
	case CLS_LINETUBE:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_LINETUBE))>0;
	case CLS_LINEFLAT:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_LINEFLAT))>0;
	case CLS_LINESLOT:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_LINESLOT))>0;
	case CLS_PLATE:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_PLATE))>0;
	case CLS_PARAMPLATE:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_PARAMPLATE))>0;
	case CLS_BOLT:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_BOLT))>0;
	case CLS_SPHERE:
		return (m_dwSolidSpaceFlag&GetSingleWord(SELECTINDEX_SPHERE))>0;
		//case SELECTINDEX_ARCANGLE	12
		//case SELECTINDEX_ARCSLOT	13
		//case SELECTINDEX_ARCFLAT	14
		//case SELECTINDEX_ARCTUBE	15
		//case SELECTINDEX_STDPART	16
		//case SELECTINDEX_GROUPLINEANGLE 17
		//case SELECTINDEX_BLOCKREF	18
		//case SELECT_LINEPART			0x00000078
	default:
		return false;
	}*/
	return false;
}
bool CSnapTypeVerify::IsEnableSnapSpace(int providerSpace)	//1.DRAWINGSPACE;2.SOLIDSPACE;3.LINESPACE
{
	if(providerSpace==1)
		return m_dwDrawingSpaceFlag>0;
	else if(providerSpace==2)
		return m_dwSolidSpaceFlag>0;
	else if(providerSpace==3)
		return m_dwLineSpaceFlag>0;
	else
		return false;
}
bool CSnapTypeVerify::IsValidGeObjType(int iObjClsType){
	switch(iObjClsType)
	{
	case AtomType::prPoint:	//0x00000001
		return (m_dwLineSpaceFlag&SNAP_POINT)>0;
	case AtomType::prLine:	//0x00000002
		return (m_dwLineSpaceFlag&SNAP_LINE)>0;
	case AtomType::prCircle://0x00000008
		return (m_dwLineSpaceFlag&SNAP_CIRCLE)>0;
	case AtomType::prRect:	//0x00000010
		return (m_dwLineSpaceFlag&SNAP_RECTANGLE)>0;
	case AtomType::prPolyFace:	//0x00000020
		return (m_dwLineSpaceFlag&SNAP_POLYGON)>0;
	case AtomType::prArc:	//0x00000040
		return (m_dwLineSpaceFlag&SNAP_ARC)>0;
	default:
		return false;
	}
}
DWORD CSnapTypeVerify::GetSnapTypeFlag(char ciProvider){
	if(ciProvider==1)
		return m_dwDrawingSpaceFlag;
	else if(ciProvider==2)
		return m_dwSolidSpaceFlag;
	else if(ciProvider==3)
		return m_dwLineSpaceFlag;
	return 0;
}
#include "f_ent.h"
#include "ComplexId.h"
SELOBJ::SELOBJ(DWORD dwhObj,DWORD dwObjInfoFlag,IDrawing* pDrawing/*=NULL*/)
{
	InitObj(dwhObj, dwObjInfoFlag,pDrawing);
}
void SELOBJ::InitObj(DWORD dwhObj,DWORD dwObjInfoFlag,IDrawing* pDrawing/*=NULL*/)
{
	m_pDrawing=pDrawing;
	SUBID16 classify=dwObjInfoFlag;
	BYTE group=(BYTE)classify.Group();
	provider=group&0x0f;
	ciTriggerType=(group&0xf0)>>4;
	if(provider==1)
	{
		idEnt=dwhObj;
		iEntType=classify.Id();
	}
	else if(provider==2)
	{
		this->hObj=dwhObj;
		this->iObjType=classify.Id();
	}
	else if(provider==3)
	{
		this->pAtom=(fAtom*)dwhObj;
		this->iAtomType=classify.Id();
	}
	m_hRelaObj=GetRelaObjH();
}
long SELOBJ::GetRelaObjH()
{
	if(provider==1)
	{
		IDbEntity* pEnt=m_pDrawing!=NULL?m_pDrawing->GetDbEntity(idEnt):NULL;
		if(pEnt)
		{
			HIBERID hiberid=pEnt->GetHiberId();
			return hiberid.masterId;
		}
		return 0;
	}
	else if(provider==2)
		return hObj;
	else if(provider==3&&pAtom)
		return pAtom->ID;
	else
		return 0;
}
