/*
 * SerialZigbee.h
 *
 *  Created on: 2015年4月13日
 *      Author: wsf
 */

#ifndef SRC_SEVER_SERIALZIGBEE_SERIALZIGBEE_H_
#define SRC_SEVER_SERIALZIGBEE_SERIALZIGBEE_H_

int serialZigbeeInit(void *obj);
int createZigbeeThread(void);
void serialZigbeeFree();



/* @brief
 * wsf
 * zigbee 0x91 定义一个ZigBee接收数据帧的帧头
 */
typedef struct
{
	unsigned char start_elimiter;											// 起始界定符
	unsigned short int  length;												// 长度
	unsigned char farme_type;												// 帧类型
	unsigned char mac_addr[8];											// 64位源地址
	unsigned short int network_addr;									// 16位网络地址
	unsigned char source_endpoint;									// 源端
	unsigned char destination_endpoint;								// 目的端
	unsigned short int cluster_ID;										// 簇ID
	unsigned short int profile_ID;										// 规范ID
	unsigned char rev_opt;													// 接收可选项
}__attribute__((packed)) ZB91_revdata_framehead;			//ZigBee Explicit Rx Indicator

/* @brief
 *  wsf
 *  zigbee 0x11  数据发送请求的帧结构的帧头
 */
typedef struct
{
	unsigned char start_elimiter;											// 起始界定符
	unsigned short int  length;												// 长度
	unsigned char frame_type;												// 帧类型
	unsigned char frame_ID;													// 帧ID ++
	unsigned char mac_addr[8];											// 64位源地址
	unsigned short int network_addr;									// 16位网络地址
	unsigned char source_endpoint;									// 源端
	unsigned char destination_endpoint;								// 目的端
	unsigned short int cluster_ID;										// 簇ID
	unsigned short int profile_ID;										// 规范ID
	unsigned char radius;														// 广播半径 ++
	unsigned char send_opt;													// 发送选项
}__attribute__((packed)) ZB11_snddata_framehead;			//ZB_explicit_cmd_frame_head;

/* @brief
 * wsf
 * zigbee 接收到的入网帧格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;
	unsigned char bak[24];
	unsigned char check_sum;
}__attribute__((packed)) ZB91_netin_framehead;			//ZigBee Explicit Rx Indicator
/* @brief
 * wsf
 * zigbee 入网帧的返回数据格式
 * 发送
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;							// ZB_11帧头
	unsigned short int dat;
	unsigned char check_sum;
}__attribute__((packed)) ZB11_netin_framehead;

/* @brief
 * wsf
 * zigbee终端上电帧
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;
	unsigned char bak[12];
	unsigned char check_sum;
}__attribute__((packed)) ZB91_poweron_framehead;

/* @brief
 *  wsf
 * A11规范 表 G.1 数据帧头定义
 */
typedef struct
{
	unsigned short int protocol_type;							// 协议类型
	unsigned short int company_code;						// 厂商代码
	unsigned short int instrument_type;						// 仪表类型
	unsigned char instrument_group;							// 仪表组号
	unsigned char instument_num;								// 仪表编号
	unsigned short int data_type;									// 数据类型
//	unsigned char orther_data[0];
} __attribute__((packed)) A11_data_framehead;

/* @breif
 * wsf
 * 定义一个功图采集和电参采集间数据交换的数组
 */
typedef struct
{
	unsigned char dynagraph_mode;									// 功图模式（与功图数据）
	unsigned char algorithms;												// 算法（与功图数据）这个不知道怎么得到的
	unsigned short int synchronization_time;						// 同步时间 10ms
	unsigned short int time_mark;										// 时间标记（ms下死点时间点由功图参数得到）
	unsigned short int cycle;												// 周期（无符号整数，10ms，由功图参数得到）
	unsigned short int dot;													// 点数（无符号整数型，由功图参数得到）
	unsigned short int time_interval;									// 时间间隔
	unsigned char flag;															// 是否有该电参
	ZB91_revdata_framehead ZB91_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;							// A11帧头
}__attribute__((packed)) data_exchange	;

/* @brief
 *  wsf
 *  表 G.5 常规数据帧格式
 *
 */
typedef struct
{
//	A11_data_framehead fram_head;
	unsigned char comm_efficiency;
	unsigned char bat_vol;
	unsigned short int sleep_time;
	unsigned short int instument_sta;
	unsigned short int realtime_data1[2];
	unsigned short int realtime_data2[2];
}__attribute__((packed)) conventional_data_frame_format;

/* @brief
 *  wsf
 *  zigbee 0x91  数据接收的帧结构
 * ZigBee Explicit Rx Indicator
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;							// ZigBee帧头
	A11_data_framehead A11_framehead;
	conventional_data_frame_format RF_Data;								// 接收到得数据 数据长度为（length - 18）
} __attribute__((packed)) ZB_explicit_RX_indicator;




/* @brief
 *  wsf
 *  zigbee 0x11  数据发送请求的帧结构
 * Explicit Addressing ZigBee Command Frame
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;							// ZB_11帧头
	A11_data_framehead A11_framehead;												//
	unsigned short int sleep_time;												//
	unsigned char check_sum;
} __attribute__((packed)) ZB_explicit_cmd_frame;


/* @brief
 * wsf
 * 定义一个A11数据帧得格式
 */
typedef struct
{
	unsigned char comm_efficiency;											// 通讯效率
	unsigned char bat_vol;															// 电池电压
	unsigned short int sleep_time;												// 休眠时间
	unsigned short int instument_sta;											// 仪器状态
}__attribute__((packed)) A11_revdata_frame;

/* @brief
 * wsf
 * 定义接收压变，温变数据的格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;							// ZigBee帧头
	A11_data_framehead A11_framehead;										// A11数据帧头 10B
	A11_revdata_frame A11_frame_data;										// A11数据 6B
	unsigned short int realtime_data[2];										// 存放实时数据
	unsigned short int instument_temp[2];									// 存放仪表温度
		}__attribute__((packed)) A11_revdata_press_tempreture;
/* @brief
 * wsf
 * 定义接收电参常规数据的格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;							// ZigBee帧头
	A11_data_framehead A11_framehead;										// A11数据帧头 10B
	A11_revdata_frame A11_frame_data;										// A11数据 6B
	unsigned short int current_phase_a[2];									// 电机工作电流 A 相 A
	unsigned short int current_phase_b[2];									// 电机工作电流 B 相 A
	unsigned short int current_phase_c[2];									// 电机工作电流 C 相 A
	unsigned short int voltage_phase_a[2];									// 电机工作电压 A 相 V
	unsigned short int voltage_phase_b[2];									// 电机工作电压 B 相 V
	unsigned short int voltage_phase_c[2];									// 电机工作电压 C 相 V
	unsigned short int power_factor[2];										// 电机功率因数
	unsigned short int moto_p[2];												// 电机有功功率 KW
	unsigned short int moto_q[2];												// 电机无功功率 KW
	unsigned short int total_power[2];											// 电机总耗电 KW`H
}__attribute__((packed)) A11_revdata_electrical_parameter;

/* @brief
 * wsf
 * 定义接收一体化功图常规数据的格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;							// ZigBee帧头
	A11_data_framehead A11_framehead;										// A11数据帧头 10B
	A11_revdata_frame A11_frame_data;										// A11数据 6B
	unsigned short int acceleration[2];										// 加速度 单位：g
	unsigned short int load[2];														// 载荷 单位：KN
}__attribute__((packed)) A11_req_revdata_dynagraph;

/* @brief
 * wsf
 * G10 定义一体化功图参数写应答帧格式
 * 即一体化功图开始测试命令
 * 发送
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char mode;														// 功图模式（实际功图 0x00；功图原始数据 0x01；仿真功图0x10）
	unsigned short int dot;													// 功图点数（整数型，无符号整数型，采集功图的点数）
	unsigned short int synchronization_time;						// 同步时间（10ms 发送采集功图命令得时间标签）
	unsigned short int time_interval;									// 时间间隔（ms 功图点采集间隔）
	unsigned char 	company_func[10];								// 厂家功能
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_rsp_collect_dynagraph;

/* @brief
 * wsf
 * G13 定义电参参数写应答帧格式
 * 即电参开始测试命令
 * 发送
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char mode;														// 功图模式（实际功图 0x00；功图原始数据 0x01；仿真功图0x10）
	unsigned short int dot;													// 功图点数（整数型，无符号整数型，采集功图的点数）
	unsigned short int synchronization_time;						// 同步时间（10ms 发送采集功图命令得时间标签）
	unsigned short int time_interval;									// 时间间隔（ms 功图点采集间隔）
	unsigned char algorithms;												// 算法
	unsigned char company_func[10];									// 厂家功能
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_rsp_collect_elec;

/* @brief
 * wsf
 * G16 功图第一组数据帧格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char data_serialnum[4];									// 数据组顺序号
	unsigned short int synchronization_time;						// 同步时间
	unsigned short int time_mark;										// 时间标记
	unsigned short int dot;													// 功图点数（整数型，无符号整数型，采集功图的实际点数）
	unsigned short int stroke;												// 冲程（无符号证书，隐含三维小数）
	unsigned short int cycle;												// 周期（无符号整数，10ms）
	unsigned char 	company_func[20];								// 厂家功能
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_req_dynagraph_first;

/* @brief
 * wsf
 * G17 功图其他组数据帧格式
 * 接收
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char data_serialnum;										// 数据组顺序号
	unsigned char reserved;													// 保留
	unsigned short int dynagraph[30];								// 功图数据
//	unsigned char dynagraph[60];											// 功图数据
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_req_dynagraph_others;
/* @brief
 * wsf
 * G18 一体化功图应答数据帧格式
 * G21 无线载荷应答数据帧格式
 * G24 无线位移应答数据正格式
 * G28 电参应答数据帧格式
 * G32 专项数据应答数据帧格式
 * 发送
 * 数据类型：功图0x0201，无线载荷0x0205，无线位移0x0208，电参0x0211，专项数据0x0231
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char data_serialnum;										// 数据组顺序号
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_rsp_datagroup;

/* @brief
 * wsf
 * G25 电流图数据命令帧格式，发送次命令后电参分组返回电流图
 * 发送
 * 数据类型为0x0212
 */
typedef struct
{
	ZB11_snddata_framehead ZB11_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char dynagraph_mode;									// 功图模式（与功图数据）
	unsigned char algorithms;												// 算法（与功图数据）这个不知道怎么得到的
	unsigned short int synchronization_time;						// 同步时间 10ms
	unsigned short int time_mark;										// 时间标记（ms下死点时间点由功图参数得到）
	unsigned short int cycle;												// 周期（无符号整数，10ms，由功图参数得到）
	unsigned short int dot;													// 点数（无符号整数型，由功图参数得到）
	unsigned char 	company_func[10];								// 厂家功能
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_rsp_currentchart;

/* @brief
 * wsf
 * G26 电参第一组数据帧格式
 * 接收
 * 数据类型为0x0030
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char data_serialnum;										// 数据组顺序号
	unsigned char reserved;													// 保留
	unsigned char dynagraph_mode;									// 功图模式
	unsigned char algorithms;												// 算法
	unsigned short int synchronization_time;						// 同步时间
	unsigned short int time_mark;										// 时间标记
	unsigned short int dot;													// 点数（无符号整数型，与功图参数相同）
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_req_elec_first;

/* @brief
 * wsf
 * G27 电参后续组数据帧格式
 * 接收
 * 数据类型为0x0030
 */
typedef struct
{
	ZB91_revdata_framehead ZB91_framehead;  					// ZigBee帧头
	A11_data_framehead A11_framehead;								// A11数据帧头 10B
	unsigned char data_serialnum;										// 数据组顺序号
	unsigned char reserved;													// 保留
	unsigned short int current_chart[30];							// 电流图
	unsigned char check_sum;												// ZigBee校验和
}__attribute__((packed)) A11_req_elec_others;

#endif /* SRC_SEVER_SERIALZIGBEE_SERIALZIGBEE_H_ */
