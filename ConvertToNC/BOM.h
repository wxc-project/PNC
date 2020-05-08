#pragma once

#include <stddef.h>
#include <WinDef.h>
#include "XhCharString.h"
#include "ArrayList.h"
#include "List.h"
#include "Buffer.h"

typedef struct BOMBOLT_RECORD
{
	static const DWORD TWO_CAP;		//双帽螺栓标识位
	static const DWORD ANTI_THEFT;	//防盗螺栓标识位
	static const DWORD ANTI_LOOSE;	//含防松垫圈标识位
	static const DWORD FOOT_NAIL;	//脚钉螺栓标识位
	static const DWORD REVERSE_NORM;//调转螺栓朝向标识位
	//
	static const BYTE FUNC_COMMON	= 0;//0.连接螺栓
	static const BYTE FUNC_FOOTNAIL	= 1;//1.脚钉，槽钢、角钢表示脚钉底座，钢板表示脚钉用途的钢板
	static const BYTE FUNC_WIREHOLE	= 2;//2.挂线孔;
	static const BYTE FUNC_EARTHHOLE= 3;//3.接地孔;
	static const BYTE FUNC_SETUPHOLE= 4;//4.装配孔;
	static const BYTE FUNC_BRANDHOLE= 5;//5.挂牌孔;
	static const BYTE FUNC_WATERHOLE= 6;//6.引流孔
	static const BYTE FUNC_FOOTNAIL2= 7;//7.代孔脚钉

	//HIBERID hiberId;			//不存储,使用+1级占位存储螺栓id
	float x,y;
	short d;
	float hole_d_increment;		//螺栓孔直径增量
	short waistLen;				// 腰圆孔腰长
	BYTE  cFuncType;			// 挂线孔
	BYTE  cFlag;				//螺栓孔加工工艺等特殊信息的标识细节，如低->高第1位为0x00表示冲孔，为0x01表示钻孔
	//COORD3D waistVec;			// 腰圆孔方向
	//DWORD dwRayNo;				//螺栓孔所在射线号
	DWORD m_dwFlag;			// 有特殊要求的标识位，如ANTITHEFT_BOLT(1),ANTILOOSE_BOLT(2),FOOTNAIL_BOLT(4)

	BOMBOLT_RECORD(){ memset(this,0,sizeof(BOMBOLT_RECORD)); }
	//void SetKey(DWORD key){hiberId.SetHiberarchy(0,0,2,key);}
	bool IsFootNail(){ return (m_dwFlag&FOOT_NAIL)!=0;}
	bool IsAntiTheft(){ return (m_dwFlag&ANTI_THEFT)!=0;}
	bool IsTwoCap(){ return (m_dwFlag&TWO_CAP)!=0;}
	bool IsAntiLoose(){ return (m_dwFlag&ANTI_LOOSE)!=0;}
	BOOL CloneBolt(BOMBOLT_RECORD *pNewBoltInfo);
	void FromBuffer(CBuffer &buffer,long version=NULL);
	void ToBuffer(CBuffer &buffer,long version=NULL);

}*BOMBOLT_RECORDPTR;
typedef struct BOMBOLT
{
	bool bFootNail;	//脚钉
	bool bTwoCap;	//双帽螺栓
	bool bAntiTheft;//防盗螺栓
	bool bAnitLoose;//防松螺栓
	bool bUFootNail;//U型脚钉
	short d;		//螺栓直径
	int nNoThreadlen;//无扣长
	short L;		//螺栓有效长或垫圈厚度,mm
	short L0;		//通过厚度
	short hFamily;			//螺栓规格从属系列标识，>=0表示螺栓，<0表示垫圈
	float grade;			//螺栓级别
	long iSeg;				//螺栓所从属的段号
	float fUnitWeight;		//单螺栓重
	CXhChar16 sMaterial;	//材质
	//
	BYTE nCommonNut;	//普通螺母
	BYTE nFastenNut;	//扣紧螺母
	BYTE nAntiTheftNut;	//防盗螺母
	BYTE nThinNut;		//薄螺母
	BYTE nCommonGasket;	//平垫圈
	BYTE nSpringGasket;	//弹簧垫圈
	BYTE nLockClasp;	//防松扣
private:
	long nPart;				//单基总数量（包括各种螺栓）
public:
	//构件默认值修改为0，否则统计特殊类型螺栓时个数可能少1 wht 12-08-24
	BOMBOLT(){memset(this,0,sizeof(BOMBOLT));nPart=0;}
	CXhChar50 CombinedKey(bool bIncLsSegI=false);
	long AddPart(int add_num=1){nPart+=add_num;return nPart;}
	long GetPartNum(){return nPart;}
	void FromBuffer(CBuffer &buffer,long version=NULL);
	void ToBuffer(CBuffer &buffer,long version=NULL);
	BOOL IsDianQuan() const{return hFamily<0;}
}*BOMBOLT_PTR;

class BOMPART
{
protected:
	long nPart;		//一个塔型中具有的当前编号单段（基）构件数量
public:
	static const BYTE OTHER = 0;//其余非常规构件
	static const BYTE BOLT  = 1;//螺栓
	static const BYTE ANGLE = 2;//角钢
	static const BYTE PLATE = 3;//钢板
	static const BYTE TUBE  = 4;//钢管
	static const BYTE FLAT  = 5;//扁钢
	static const BYTE SLOT  = 6;//槽钢
	static const BYTE ROUND = 7;//圆钢
	static const BYTE ACCESSORY = 8;//附件
	//构件子类型
	const static BYTE SUB_TYPE_TUBE_MAIN	= 1;	//主管
	const static BYTE SUB_TYPE_TUBE_BRANCH	= 2;	//支管
	const static BYTE SUB_TYPE_PLATE_C		= 3;	//槽型插板
	const static BYTE SUB_TYPE_PLATE_U		= 4;	//U型插板
	const static BYTE SUB_TYPE_PLATE_X		= 5;	//十字插板
	const static BYTE SUB_TYPE_PLATE_FL		= 6;	//法兰
	const static BYTE SUB_TYPE_PLATE_WATER	= 7;	//遮水板
	const static BYTE SUB_TYPE_FOOT_FL		= 8;	//底脚法兰
	const static BYTE SUB_TYPE_ROD_Z		= 9;	//主材
	const static BYTE SUB_TYPE_ROD_F		= 10;	//辅材
	const static BYTE SUB_TYPE_ANGLE_SHORT	= 11;	//短角钢
	const static BYTE SUB_TYPE_PLATE_FLD	= 12;	//对焊法兰
	const static BYTE SUB_TYPE_PLATE_FLP	= 13;	//平焊法兰
	const static BYTE SUB_TYPE_PLATE_FLG	= 14;	//刚性法兰
	const static BYTE SUB_TYPE_PLATE_FLR	= 15;	//柔性法兰
	const static BYTE SUB_TYPE_BOLT_PAD		= 16;	//螺栓垫板
	const static BYTE SUB_TYPE_TUBE_WIRE	= 17;	//套管
	const static BYTE SUB_TYPE_STEEL_GRATING= 18;	//钢格栅
	const static BYTE SUB_TYPE_STAY_ROPE	= 19;	//拉索构件
	const static BYTE SUB_TYPE_LADDER		= 20;	//爬梯
	const static BYTE SUB_TYPE_PLATE_ANGLERIB = 21;	//角钢肋板

	char cPartType;
	CXhChar16 sMappingDrwLabel;	//进行数据比对时，表示当前件号与原图纸明细中的匹配对应的构件编号
public:
	CXhChar16 sPartNo;	//构件编号
	int iSeg;			//段号
	short siSubType;	//
	char cMaterial;		//材质简化字符，如：'S','H','G'
	CXhChar16 sMaterial;//设置材质字符串用于处理新加材质,如：Q355 wht 19-08-25
	char cQualityLevel;	//材料质量等级
	double wide;		//构件宽度参数
	double thick;		//构件厚度参数
	double wingWideY;	//角钢Y肢宽度
	CXhChar16 sSpec;	//规格
	double length;		//构件长度参数
	double fPieceWeight;//单件重(kg)，对于钢板板为最小包容矩形重量
	double fSumWeight;	//总重
	double fMapSumWeight;	//图纸总重（IBOM导出材料汇总表时需要）wht 19-10-24
	short siZhiWan;		//>0：制弯次数，<=0不需要制弯
	BOOL bWeldPart;		//是否需要焊接
	BOOL bLegPart;		//是否为腿部构件
	short nM16Ls;	//M16螺栓孔数
	short nM20Ls;	//M20螺栓孔数
	short nM24Ls;	//M24螺栓孔数
	short nMXLs;	//其它直径螺栓孔数
	short nMSumLs;	//螺栓孔总数
	char sNotes[50];	//构件备注说明
	long feature1;		//特征属性1，临时使用
	long feature2;		//特征属性2，临时使用
	//螺栓信息
	ARRAY_LIST<BOMBOLT_RECORD> m_arrBoltRecs;
public:
	BOMPART();
	~BOMPART(){;}
	long AddPart(int add_num=1){nPart+=add_num;return nPart;}
	void SetPartNum(int nPartNum) { nPart = nPartNum; }
	long GetPartNum(){return nPart;}
	int GetBoltOrHoleNum(double min_d,double max_d=-1,BOOL bHoleD=TRUE);
	BOOL IsHasLinearHole();
	BOOL IsHasSpecFuncHole();
	BOOL IsDrillHole();
	BOOL IsPolyPart();
	BOOL IsExpandHole();
	bool get_bRevMappingPart(){return sMappingDrwLabel.At(0)!=0;}
	__declspec(property(get=get_bRevMappingPart)) bool bRevMappingPart;
	void set_szMappingDrwLabel(const char* _szMappingDrwLabel){sMappingDrwLabel=_szMappingDrwLabel;}
	const char* get_szMappingDrwLabel(){return bRevMappingPart?sMappingDrwLabel:sPartNo;}
	__declspec(property(put=set_szMappingDrwLabel,get=get_szMappingDrwLabel)) char* szMappingDrwLabel;
	virtual double CalZincCoatedArea(BOOL bSwitchToM=TRUE){return 0;}	//镀锌面积
	virtual short CutAngleN(){return 0;}
	virtual CXhChar16 GetSpec(){ return sSpec;}
	virtual short GetHuoquLineCount(BOOL bIncRollEdge=TRUE);
	virtual double GetHuoquDegree(int index){return 0;}
	virtual int GetLsHoleStr(char *hole_str,BOOL bIncUnit=TRUE);	//螺栓孔字符串
	virtual CXhChar16 GetPartTypeName(BOOL bSymbol=FALSE);
	virtual CXhChar16 GetPartSubTypeName();
	virtual CXhChar50 GetPartNo(int nMatCharPosType=0,char cMatSeparator=0,bool bIncQualityLevel=false);
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
	virtual double GetWeldLength(){return 0;}
};
struct BOMPROFILE_VERTEX{
	//HIBERID hiberId;				//不存储,使用+2级占位存储钢板轮廓边
	struct COORD2D{
		double x,y;
		COORD2D(double* coord=NULL, int dimension=2);
		operator double*(){return &x;}
		void Set(double coordX=0,double coordY=0){x=coordX; y=coordY;}
	};
	struct COORD3D{
		double x,y,z;
		COORD3D(double* coord=NULL, int dimension=3);
		operator double*(){return &x;}
		void Set(double coordX=0,double coordY=0,double coordZ=0){x=coordX; y=coordY; z=coordZ;}
	};
	BYTE type;						//1:普通直边 2:圆弧 3:椭圆弧
	bool m_bWeldEdge;				//是否焊缝边
	bool m_bRollEdge;				//此边需卷边
	short roll_edge_offset_dist;	//卷边偏移距离
	double radius;					//椭圆弧所需要的参数
	double sector_angle;			//标准圆弧所需要的参数
	short manu_space;				//焊缝加工预留间隙或卷边高度(mm)
	COORD2D vertex;					//顶点的位置坐标
	COORD3D center,column_norm;	//椭圆弧所需要的参数（永远输入绝对坐标),center如果表示圆弧圆心(拾取点)时，则输入相对坐标
	COORD3D work_norm;				//圆弧法线方向
	//double local_point_y;			//对活点位置，在该点两侧标注
	//int local_point_vec;			//0.表示无对活点，1.表示对活点两侧标注50-->100的方向与焊接父杆件的start-->end方向相同，2.表示相反
	CXhChar16 sWeldMotherPart;		//焊接缝所在的焊接母构件
	double edge_len;				//边长
public:	//焊缝参数
	static const char STATE_NEED_SLOPECUT  = 0x01;	//需要进行坡口
	static const char STATE_FSQUARE_WELD   = 0x02;	//正面平焊（无突出焊缝），否则默认为满焊
	static const char STATE_BSQUARE_WELD   = 0x04;	//背面平焊（无突出焊缝），否则默认为满焊
	char cStateFlag;	//0.坡口；2.正面平(满)焊；4.背面平(满)焊
	short height;		//焊缝高度,mm
	short length;		//焊缝长度,mm
	short lenpos;		//焊缝起点位置(自边始端点的偏移量), mm
public:
	BOMPROFILE_VERTEX(const double* coord2d=NULL, int dimension=2);
	BOMPROFILE_VERTEX(double x,double y);
	//void SetKey(DWORD key){hiberId.SetHiberarchy(0,0,0,key);}
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
};
class PART_PLATE : public BOMPART
{
public:
	BYTE m_cFaceN;			//面数(不含卷边)
	BOOL bNeedFillet;		//需要坡口
	static const int TYPE_TOWER_FOOT	  = 1;	//塔脚板
	static const int TYPE_FOLD_3FACE	  = 2;	//折叠板
	static const int TYPE_CUT_MOUTH_3FACE = 3;	//割口三面板
	int m_iPlateType;		//BOOL bTowerFootPlate;		//塔脚板
	double fPieceNetWeight;	//单件重(kg)（板为净截面重量）
	double min_area;		//连接板最小包容矩形面积(mm2)
	double real_area;		//净连板面积(mm2)
	double fPerimeter;		//钢板周长
	double fCutMouthLen;	//割口长度 wht 19-06-27
	double fWeldEdgeLen;	//焊接边长度
	double fWeldLineLen;	//焊缝线长度：焊接边长度 X 焊缝系数
	double fFlInnerD;		//法兰内圆直径
	double fFlOutterD;		//法兰外圆直径
	double fFlBoltLayoutD;	//法兰螺栓布置直径
	double huoQuAngleArr[2];	//faceAngleArr[0] 代表1号面和2面角度
							    //faceAngleArr[1] 代表2号面和3面角度
 	BOMPROFILE_VERTEX::COORD3D huoquFaceNormArr[2];
	//轮廓边信息
	CXhSimpleList<BOMPROFILE_VERTEX> listVertex;
public:
	BOOL IsTowerFootPlate() { return m_iPlateType == TYPE_TOWER_FOOT; }
	__declspec(property(get = IsTowerFootPlate)) BOOL bTowerFootPlate;
public:
	PART_PLATE();
	long GetEdgeCount();
	long GetArcEdgeCount();
	BOOL IsFL();
	BOOL IsRect();	
	virtual double CalZincCoatedArea(BOOL bSwitchToM=TRUE);
	virtual short CutAngleN();
	virtual short GetHuoquLineCount(BOOL bIncRollEdge=TRUE);
	virtual double GetHuoquDegree(int index);
	virtual CXhChar16 GetSpec();
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
	virtual double GetWeldLength(){return fWeldLineLen;}
	virtual double GetWeight();
};
class PART_TUBE : public BOMPART
{	//钢管加工工艺---相贯/开槽
public:
	struct TUBE_PROCESS
	{
		static const BYTE PROCESSTYPE_NONE		= 0;	//无特殊工艺
		static const BYTE PROCESSTYPE_TRANSECT	= 1;	//圆柱相贯
		static const BYTE PROCESSTYPE_PLANECUT	= 2;	//平面相贯
		static const BYTE PROCESSTYPE_CSLOT		= 3;	//一型开口槽
		static const BYTE PROCESSTYPE_USLOT		= 4;	//U型开口槽
		static const BYTE PROCESSTYPE_TSLOT		= 5;	//T型开口槽
		static const BYTE PROCESSTYPE_XSLOT		= 6;	//十字型开口槽
		static const BYTE PROCESSTYPE_FL		= 7;	//法兰
		static const BYTE PROCESSTYPE_FLD		= 8;	//对焊法兰
		static const BYTE PROCESSTYPE_FLP		= 9;	//平焊法兰
		static const BYTE PROCESSTYPE_FLG		= 10;	//刚性法兰
		static const BYTE PROCESSTYPE_FLR		= 11;	//柔性法兰
		BYTE type;				//钢管工艺类型
		CXhChar16 sPartNo;		//端头构件直径(法兰、插板、相关主管) wht 19-11-26
		CXhChar16 sSpec;		//相贯时主管规格
		double L;				//开口槽长度L或相贯时支管长度
		double A;				//开口槽宽度A或相贯时主管与支管之间的夹角或钢管心线与切割面之间的夹角
		double B;				//T型槽宽度B或U型槽内圆弧半径
		double fNormOffset;		//开口槽法向偏移量
		double fWeldEdge;		//端头焊缝长度 wht 19-11-26
		BOOL IsFL(){return PROCESSTYPE_FL==type||PROCESSTYPE_FLD==type||PROCESSTYPE_FLP==type||
						   PROCESSTYPE_FLR==type||PROCESSTYPE_FLG==type;}
		TUBE_PROCESS(){memset(this,0,sizeof(TUBE_PROCESS));}
	};
	TUBE_PROCESS startProcess;	//始端工艺类型
	TUBE_PROCESS endProcess;	//终端工艺类型
	BOOL bHasNodePlate;			//钢管中间连有节点板
	BOOL bHasBranchTube;		//钢管中间连有相贯的支管
	BOOL bHasFootNail;			//是否需要脚钉
	BOOL bHasWeldRoad;			//是否有焊缝线
public:
	PART_TUBE();
	int GetNotchType(char *type,int nRuleType);
	virtual double CalZincCoatedArea(BOOL bSwitchToM=TRUE);
	virtual CXhChar16 GetPartTypeName(BOOL bSymbol=FALSE);
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
};
#ifndef __DEF_KAIHEJIAO_STRUC__
#define __DEF_KAIHEJIAO_STRUC__
struct KAI_HE_JIAO
{
	float decWingAngle;		//两肢夹角
	short position;			//标定位置
	short startLength;		//始端开合长度
	short endLength;		//终端开合长度
	KAI_HE_JIAO() {memset(this,0,sizeof(KAI_HE_JIAO));}
};
#endif
#ifndef __DEF_ANGLE_PUSH_FLAT_STRUC__
#define __DEF_ANGLE_PUSH_FLAT_STRUC__
struct ANGLE_PUSH_FLAT
{
	short push_flat_x1_y2;	//X肢：1，Y肢：2
	short flat_start_pos;
	short flat_end_pos;
	ANGLE_PUSH_FLAT() { memset(this, 0, sizeof(ANGLE_PUSH_FLAT)); }
};
#endif
class PART_ANGLE : public BOMPART
{
public:
	float wing_angle;	//角钢两肢夹角
	BOOL bCutAngle;		//是否切角
	BOOL bCutBer;		//是否需要铲背
	BOOL bCutRoot;		//是否需要清根
	BOOL bKaiJiao;		//是否需要开角
	BOOL bHeJiao;		//是否需要合角
	BOOL bHasFootNail;	//是否需要脚钉
	int nPushFlat;		//0x01:始端压扁 0x02:中间压扁 0x04:终端压扁
	// --------- 切肢 -----------
	//标识所切角钢肢,0表示无切肢,1表示X肢,2表示Y肢
	int cut_wing[2];
	/*cut_wing_para[0]cut_wing_para[1]表示终端切角
	...[][0]表示角钢楞线切取长度
	...[][1]表示另一肢切取宽度
	...[][2]表示肢棱线切取长度
	*/
	int cut_wing_para[2][3];
    // --------- 切角 -----------
	/*
	cut_angle[0]表示起点X肢
	cut_angle[1]表示起点Y肢
	cut_angle[2]表示终点X肢
	cut_angle[3]表示终点Y肢
	...[][0]表示平行楞线方向切取量  ...[][1]表示垂直楞线方向切取量
	*/
	int cut_angle[4][2];
	double fWeldLineLen;	//焊缝长度
	CXhSimpleList<KAI_HE_JIAO> listKaihe;		//开合角
	CXhSimpleList<ANGLE_PUSH_FLAT> listPushFlat;//压扁
	struct POLY_ANGLE_INFO
	{
		int nIndex;				//构件索引
		BOMPROFILE_VERTEX::COORD3D norm_x_wing;	//X肢展开方向
		BOMPROFILE_VERTEX::COORD3D norm_y_wing;	//Y肢展开方向
		BOMPROFILE_VERTEX::COORD3D len_vec;		//延伸方向
	};
	CXhSimpleList<POLY_ANGLE_INFO> listPolyAngle;	//制弯角钢子角钢
public:
	PART_ANGLE();
	bool IsCutAngleOrWing(); 
	bool IsStartCutAngleOrWing();
	bool IsEndCutAngleOrWing();
	BYTE GetHuoquType(int *huoqu_wing_x0_y1_all2=NULL);	//0.无制弯 1.内曲 2.外曲 3.内曲筋 4.外曲筋
	virtual double GetHuoquDegree(int index);
	virtual double CalZincCoatedArea(BOOL bSwitchToM=TRUE);
	virtual short CutAngleN();
	virtual short KaiHeJiaoN();
	virtual double SuttleCount();
	virtual CXhChar16 GetSpec();
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
	virtual double GetWeldLength(){return fWeldLineLen;}
};
class PART_SLOT : public BOMPART
{
public:
	PART_SLOT();
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);
};
class BOM_PARTREF
{
public:
	CXhChar16 sPartNo;
	int nPartNum;
	BOM_PARTREF(){nPartNum=0;}
	void ToBuffer(CBuffer &buffer,long version=NULL);
	void FromBuffer(CBuffer &buffer,long version=NULL);
};
class BOM_WELDPART
{
public:
	static const BYTE TYPE_COMMON		= 0;	//普通组焊件
	static const BYTE TYPE_ANGLEFOOT	= 1;	//角钢塔脚
	static const BYTE TYPE_TUBEFOOT		= 2;	//钢管塔脚
	static const BYTE TYPE_PLATES		= 3;	//纯板焊件
	static const BYTE TYPE_ANGLERIBPLATE= 4;	//角钢肋板
	static const BYTE TYPE_ANGLELJB		= 5;	//角钢连板
	BYTE cWeldPartType;
	int iSeg;
	CXhChar16 sName;
	CXhChar16 sPartNo;
	int nWeldPartNum;
	CXhSimpleList<BOM_PARTREF> listPartRef;
	BOM_WELDPART(){iSeg=0;nWeldPartNum=0;cWeldPartType=0;}
	void ToBuffer(CBuffer &buffer,long version=NULL);
	void FromBuffer(CBuffer &buffer,long version=NULL);
};
class CModelBOM
{
public:
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar50 m_sPrjName;		//工程名称
	CXhChar100 m_sTowerTypeName;//塔型名
	CXhChar50 m_sTaAlias;		//代号（分区号）
	CXhChar50 m_sTaStampNo;		//钢印号(分区名称)
	CXhChar50 m_sOperator;		//操作员（制表人）
	CXhChar50 m_sAuditor;		//审核人
	CXhChar50 m_sCritic;		//评审人
	int m_nTaNum;				//塔基数
	int m_iCirclePlankAreaType;	//1.表示最小包容矩形统计;2.表示净面积统计(去掉内环)
	int m_iPlankWeightStatType;	//0.表示正交矩形统计;1.表示最小包容矩形统计;2.表示净面积统计
	BYTE m_biMatCharPosType;	//构件编号的材质字符位置类型
	char m_cMatCharSeparator;	//构件编号的材质字符分隔符: 无0  空格' '
	//
	CXhChar50 m_sSegStr;		//段号
	CXhChar100 m_sInstanceName;	//临时存储（杆塔号）
	CXhChar16 m_sInstanceNo;	//临时存储（序号）
	//
	int m_nPartsN,m_nBoltsN,m_nWeldPartsN;
	CXhSuperList<BOMPART> listParts;	//构件记录集合
	BOMBOLT* m_pBoltRecsArr;			//螺栓记录集合
	CXhSimpleList<BOM_WELDPART> listWeldParts;	//组焊件
public:
	CModelBOM();
	~CModelBOM();
	bool ToBuffer(CBuffer &buffer,long version=NULL);
	bool FromBuffer(CBuffer &buffer,long version=NULL);
};