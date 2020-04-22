#ifndef  __DEF_CARD_
#define  __DEF_CARD_

enum CARD_ITEM_TYPE{
	ITEM_TYPE_CODE_NO=0,		//代号
	ITEM_TYPE_TA_TYPE=1,		//塔型
	ITEM_TYPE_LENGTH=2,			//长度
	ITEM_TYPE_PART_NO=3,		//件号
	ITEM_TYPE_MAP_NO=4,			//图号
	ITEM_TYPE_TA_NUM=5,			//基数
	ITEM_TYPE_CUT_ROOT=6,		//刨根
	ITEM_TYPE_CUT_BER=7,		//铲背
	ITEM_TYPE_MAKE_BEND=8,		//制弯
	ITEM_TYPE_PUSH_FLAT=9,		//压扁
	ITEM_TYPE_CUT_EDGE=10,		//铲边
	ITEM_TYPE_WRAP_EDGE=11,		//卷边
	ITEM_TYPE_WELD=12,			//焊接
	ITEM_TYPE_KAIJIAO=13,		//开角
	ITEM_TYPE_HEJIAO=14,		//合角
	ITEM_TYPE_PART_NUM=15,		//单基数
	ITEM_TYPE_PIECE_WEIGHT=16,	//单重 
	ITEM_TYPE_SUM_PART_NUM=17,	//加工数
	ITEM_TYPE_SUM_WEIGHT=18,	//总重
	ITEM_TYPE_REPL_MAT=19,		//代用材质
	ITEM_TYPE_DES_MAT=20,		//设计材质
	ITEM_TYPE_REPL_GUIGE=21,	//代用规格
	ITEM_TYPE_DES_GUIGE=22,		//设计规格
	ITEM_TYPE_PRJ_NAME=23,		//工程名称
	ITEM_TYPE_HUOQU_FST=24,		//首次火曲
	ITEM_TYPE_HUOQU_SEC=25,		//二次火曲
	ITEM_TYPE_MAP_SCALE=26,		//图面比例
	ITEM_TYPE_LSM12_NUM=27,		//M12孔数
	ITEM_TYPE_LSM16_NUM=28,		//M16孔数
	ITEM_TYPE_LSM20_NUM=29,		//M20孔数
	ITEM_TYPE_LSM24_NUM=30,		//M24孔数
	ITEM_TYPE_LSSUM_NUM=31,		//总孔数
	ITEM_TYPE_TAB_MAKER=32,		//制表人
	ITEM_TYPE_CUT_ANGLE_S_X=33,	//始端X切角/肢
	ITEM_TYPE_CUT_ANGLE_S_Y=34,	//始端Y切角/肢
	ITEM_TYPE_CUT_ANGLE_E_X=35,	//终端X切角/肢
	ITEM_TYPE_CUT_ANGLE_E_Y=36,	//终端Y切角/肢
	ITEM_TYPE_PART_NOTES=37,	//备注
	ITEM_TYPE_DATE=38,			//日期
	ITEM_TYPE_CRITIC=39,		//评审人
	ITEM_TYPE_AUDITOR=40,		//审核人
	ITEM_TYPE_CUTBERICON=41,	//铲背图标
	ITEM_TYPE_CUTROOTICON=42,	//刨根图标
	ITEM_TYPE_BENDICON=43,		//制弯图标
	ITEM_TYPE_WELDICON=44,		//焊接图标
	ITEM_TYPE_SKETCH_MAP=45,	//示意图
	ITEM_TYPE_PROCESS_NAME=46,	//工艺名称
	ITEM_TYPE_PAGENUM=47,		//共  页
	ITEM_TYPE_PAGEINDEX=48,		//第  页
	ITEM_TYPE_LSM18_NUM=49,		//M18孔数
	ITEM_TYPE_LSM22_NUM=50,		//M22孔数
	ITEM_TYPE_FOOTNAIL_PLATE=51,//脚钉底座材料表
	ITEM_TYPE_WING_ANGLE=52,	//角钢两肢夹角
	ITEM_TYPE_CUT_ANGLE=53,		//切角
	ITEM_TYPE_KAIHE_ANGLE_ICON=54,//开合角图标
	ITEM_TYPE_PUSH_FLAT_S_X=55,	//始端X压扁
	ITEM_TYPE_PUSH_FLAT_S_Y=56,	//始端Y压扁
	ITEM_TYPE_PUSH_FLAT_E_X=57,	//终端X压扁
	ITEM_TYPE_PUSH_FLAT_E_Y=58,	//终端Y压扁
};

#endif