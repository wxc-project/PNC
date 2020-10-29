#ifndef  __DEF_CARD_
#define  __DEF_CARD_

enum CARD_ITEM_TYPE{
	ITEM_TYPE_CODE_NO=0,		//����
	ITEM_TYPE_TA_TYPE=1,		//����
	ITEM_TYPE_LENGTH=2,			//����
	ITEM_TYPE_PART_NO=3,		//����
	ITEM_TYPE_MAP_NO=4,			//ͼ��
	ITEM_TYPE_TA_NUM=5,			//����
	ITEM_TYPE_CUT_ROOT=6,		//�ٸ�
	ITEM_TYPE_CUT_BER=7,		//����
	ITEM_TYPE_MAKE_BEND=8,		//����
	ITEM_TYPE_PUSH_FLAT=9,		//ѹ��
	ITEM_TYPE_CUT_EDGE=10,		//����
	ITEM_TYPE_WRAP_EDGE=11,		//���
	ITEM_TYPE_WELD=12,			//����
	ITEM_TYPE_WELD_STICK=69,	//��˿�ͺ�
	ITEM_TYPE_NOT_WELD_PART = 72,//ɢ����ָ���ú��ӵ���� wht 20-08-21 ����
	ITEM_TYPE_KAIJIAO=13,		//����
	ITEM_TYPE_HEJIAO=14,		//�Ͻ�
	ITEM_TYPE_KAIHE_JIAO = 73,	//���Ͻ�	wht 20-08-21 ����
	ITEM_TYPE_SLOT_CUT = 81,	//���ۣ��Ǹָ�ڣ��ֹܿ��ۣ�
	ITEM_TYPE_PART_NUM=15,		//������
	ITEM_TYPE_PIECE_WEIGHT=16,	//���� 
	ITEM_TYPE_SUM_PART_NUM=17,	//�ӹ���
	ITEM_TYPE_SUM_WEIGHT=18,	//����
	ITEM_TYPE_REPL_MAT=19,		//���ò���
	ITEM_TYPE_DES_MAT=20,		//��Ʋ���
	ITEM_TYPE_DES_MAT_BRIEF	=70,	//��Ʋ��ʼ��ַ� wht 20-08-21 ����
	ITEM_TYPE_REPL_MAT_BRIEF = 71,	//���ò��ʼ��ַ� wht 20-08-21 ����
	ITEM_TYPE_REPL_GUIGE=21,	//���ù��
	ITEM_TYPE_DES_GUIGE=22,		//��ƹ��
	ITEM_TYPE_PRJ_NAME=23,		//��������
	ITEM_TYPE_HUOQU_FST=24,		//�״λ���
	ITEM_TYPE_HUOQU_SEC=25,		//���λ���
	ITEM_TYPE_MAP_SCALE=26,		//ͼ�����
	ITEM_TYPE_LSM12_NUM=27,		//M12����
	ITEM_TYPE_LSM16_NUM=28,		//M16����
	ITEM_TYPE_LSM20_NUM=29,		//M20����
	ITEM_TYPE_LSM24_NUM=30,		//M24����
	ITEM_TYPE_LSSUM_NUM=31,		//�ܿ���
	ITEM_TYPE_TAB_MAKER=32,		//�Ʊ���
	ITEM_TYPE_CUT_ANGLE_S_X=33,	//ʼ��X�н�/֫
	ITEM_TYPE_CUT_ANGLE_S_Y=34,	//ʼ��Y�н�/֫
	ITEM_TYPE_CUT_ANGLE_E_X=35,	//�ն�X�н�/֫
	ITEM_TYPE_CUT_ANGLE_E_Y=36,	//�ն�Y�н�/֫
	ITEM_TYPE_PART_NOTES=37,	//��ע
	ITEM_TYPE_DATE=38,			//����
	ITEM_TYPE_CRITIC=39,		//������
	ITEM_TYPE_AUDITOR=40,		//�����
	ITEM_TYPE_CUTBERICON=41,	//����ͼ��
	ITEM_TYPE_CUTROOTICON=42,	//�ٸ�ͼ��
	ITEM_TYPE_BENDICON=43,		//����ͼ��
	ITEM_TYPE_WELDICON=44,		//����ͼ��
	ITEM_TYPE_SKETCH_MAP=45,	//ʾ��ͼ
	ITEM_TYPE_PROCESS_NAME=46,	//��������
	ITEM_TYPE_PAGENUM=47,		//��  ҳ
	ITEM_TYPE_PAGEINDEX=48,		//��  ҳ
	ITEM_TYPE_LSM18_NUM=49,		//M18����
	ITEM_TYPE_LSM22_NUM=50,		//M22����
	ITEM_TYPE_FOOTNAIL_PLATE=51,//�Ŷ��������ϱ�
	ITEM_TYPE_WING_ANGLE=52,	//�Ǹ���֫�н�
	ITEM_TYPE_CUT_ANGLE=53,		//�н�
	ITEM_TYPE_KAIHE_ANGLE_ICON=54,//���Ͻ�ͼ��
	ITEM_TYPE_PUSH_FLAT_S_X=55,	//ʼ��Xѹ��
	ITEM_TYPE_PUSH_FLAT_S_Y=56,	//ʼ��Yѹ��
	ITEM_TYPE_PUSH_FLAT_E_X=57,	//�ն�Xѹ��
	ITEM_TYPE_PUSH_FLAT_E_Y=58,	//�ն�Yѹ��
	ITEM_TYPE_PUSH_FLAT_M_X=59,	//�м�Xѹ��
	ITEM_TYPE_PUSH_FLAT_M_Y=60, //�м�Yѹ��
	ITEM_TYPE_LSMX_NUM=61,		//��������
	ITEM_TYPE_CUT_ARC=62,		//�л�
	ITEM_TYPE_FILLET=63,		//�¿�
	ITEM_TYPE_ZUAN_KONG=64,		//���
	ITEM_TYPE_CHONG_KONG=65,	//���
	ITEM_TYPE_PUNCH_SHEAR=66,	//���
	ITEM_TYPE_GAS_CUTTING=67,	//����
	ITEM_TYPE_ONCE_OVER=68,		//һ�����
	//ITEM_TYPE_WELD_STICK=69,	//��˿�ͺ�
	//Ϊ�繤�������ƽǸֹ��գ��¼���������,�������԰����ల�ŵ��������Ը��� wht 20-08-21
	//ITEM_TYPE_DES_MAT_BRIEF	=70,//��Ʋ��ʼ��ַ�
	//ITEM_TYPE_REPL_MAT_BRIEF = 71,//���ò��ʼ��ַ�
	//ITEM_TYPE_NOT_WELD_PART	=72,//ɢ����ָ���ú��ӵ����
	//ITEM_TYPE_KAIHE_JIAO = 73,	//���Ͻ�
	ITEM_TYPE_TASK_NO = 74,			//���񵥺�
	ITEM_TYPE_TOLERANCE = 75,		//���Ϲ����׼(���ꡢ������������)
	ITEM_TYPE_TRIAL_ASSEMBLY = 76,	//����װ\����
	ITEM_TYPE_TUBE_TYPE = 77,		//�񻡺��ܡ���Ƶ�ܡ��޷��
	ITEM_TYPE_TECH_REQ = 78,		//����Ҫ��
	ITEM_TYPE_ONLY_PAGENUM = 79,	//��  ҳ����ҳ�룬ֻ������
	ITEM_TYPE_ONLY_PAGEINDEX = 80,	//��  ҳ��ҳ��������ֻ������
	//ITEM_TYPE_SLOT_CUT = 81,		//���ۣ��Ǹָ�ڣ��ֹܿ��ۣ�
	
};

#endif