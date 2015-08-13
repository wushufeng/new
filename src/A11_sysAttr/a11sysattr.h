/*
 * a11sysattr.h
 *
 *  Created on: 2015年3月26日
 *      Author: wsf
 */

#ifndef SRC_A11_SYSATTR_A11SYSATTR_H_
#define SRC_A11_SYSATTR_A11SYSATTR_H_
#define BUF_SIZE (256)
/*@brief
 * wsf
 * 应用得井占类型(well_station_type) 参数预定义
 */
#define		WST_SINGLE_WELL						000		// 单井0XX
#define		WST_VALVE_VAULT						100		// 阀室
#define		WST_OIL_WELL						001		// 油井（生产井）
#define		WST_GASS_WELL						002		// 气井
#define		WST_WATER_SOURCE_WELL				003		// 水源井
#define		WST_INJECTION_WELL					004		// 注水井
#define		WST_GAS_INJECTION_WELL				005		// 注气井
#define		WST_OBSERVING_WELL					006		// 观察井
#define		WST_METERING_STATION				101		// 计量站
#define 	WST_BOOSTER_STATION					102		// 增压站
#define		WST_GAS_GATHERING_STATION			103		// 集气站
#define		WST_GAS_TRANSMISSION_STATION		104		// 输气站
#define		WST_DISTRIBUTION_STATION			105		// 配气站
#define		WST_RE_INJECTION_STATION			106		// 水处理（回注）站配气站
#define		WST_DEHYDRATION_STATION				107		// 脱水站

///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.1 远程终端单元系统属性数据存储地址
// 地址范围40001-40299
// 一,RTU 基本信息
// 二, 通信类参数
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * 表 E.1 远程终端单元系统属性数据存储地址
 * 一,RTU 基本信息
 * 地址范围40001-40030
 */
typedef struct
{
	unsigned short int well_station_type;						// 井站类型
	unsigned short int dev_company;								// 设备厂家
	unsigned short int type_version;							// 型号版本
	unsigned short int password;								// 口令
	unsigned short int sys_time[3];								// 系统时间
	unsigned short int sys_date[3];								// 系统日期
	unsigned short int ram_vol;									// RAM电池电压
	unsigned short int cabinet_temperature;						// 机柜温度
	unsigned short int day_of_start_time[3];					// 日时间起点
	unsigned short int system_data_bakend[15];					// 系统保留
}__attribute__((packed)) RTU_baseinfo;

/*@brief
 * wsf
 * 通讯方式（communication_mode）预定义
 */
#define		CM_DATA_RADIO							0		// 数传电台
#define		CM_GPRS_CDMA							1		// GPRS/CDMA
#define		CM_RS_485								2		// RS485
#define		CM_WIRELESS_BRIDGES						3		// 无线网桥
#define		CM_MICWILL								4		//	MicWill
#define		CM_LTE									5		// LTE
/*@brief
 * wsf
 * 通讯协议（communication_protocols）预定义
 */
#define		CP_MB_RTU			0								// MODBUS RTU
#define		CP_MB_TCPIP			1								// MODBUS TCP/IP
#define		CP_MB_DNP			2								// DNP 3.0
#define		CP_MB_UDP			3								// 大庆GRM协议
/*@brief
 * wsf
 * 串口波特率（comm_baudrate）预定义
 */
#define		C_BAUD_1200				0	// 1200
#define		C_BAUD_2400				1	// 2400
#define		C_BAUD_4800				2	// 4800
#define		C_BAUD_9600				3	// 9600
#define		C_BAUD_19200			4	// 19200
#define		C_BAUD_38400			5	// 38400
#define		C_BAUD_57600			6	// 57600
#define		C_BAUD_OTHER			7	// 其他波特率 115200
/*@brief
 * wsf
 * 串口数据位（comm_databit）预定义
 */
#define		C_DATA_BIT_7				0	// 数据位7
#define		C_DATA_BIT_8				1	// 数据位8
/*@brief
 * wsf
 * 串口停止位（comm_stopbit）预定义
 */
#define		C_STOP_BIT_1				0	// 停止位1
#define		C_STOP_BIT_2				1	// 停止位2
/*@brief
 * wsf
 * 串口奇偶校验位（comm_paritybit）预定义
 */
#define		C_NONE						0	// 无校验
#define 	C_ENEN						1	// 偶校验
#define 	C_ODD							2	// 奇校验
/*@brief
 * wsf
 * 半/全双工（comm_duplex）预定义
 */
#define		CD_HALF_DUPLEX		0	// 半双工
#define		CE_FULL_DUPLEX		1	// 全双工
/*@brief
 * wsf
 * 半/全双工（tcp_udp_identity）预定义
 */
#define		TUI_TCP						0	// TCP
#define		TUI_UDP						1	// UDP
/*@brief
 * wsf
 * 主站通讯方式（master_comm_mode）预定义
 */
#define		MCM_INVALID			0xFF		// 无效
#define		MCM_GPRS_CDMA		0x01		// GPRS/CDMA
#define		MCM_TCP_SERVER		0x02		// Ethernet TCP Server
#define		MCM_TCP_CLIENT		0x03		// Ethernet TCP Client
#define		MCM_UDP_SERVER		0x04		// Ethernet UPD Server
#define		MCM_UDP_CLIENT		0x05		// Ethernet UDP Client
/*@brief
 * wsf
 * 下行通讯接口（downlink_comm_interface）预定义
 */
#define		DCI_INVALID				0xFF		// 无效
#define		DCI_ETHERNET			0x01		// 以太网
#define		DCI_WIRELESS			0x02		// 无线433 （TTL）
#define		DCI_ZIGBEE				0x03		// ZigBee （TTL）
#define		DCI_RS232A				0x04		// RS232A
#define		DCI_RS232B				0x05		// RS232B
#define		DCI_RS485A				0x06		// RS485A
#define		DCI_RS485B				0x07		// RS485B


/*@brief
 * wsf
 * 表 E.1 远程终端单元系统属性数据存储地址
 * 二, 通信类参数
 * 地址范围40031-40299
 */
 typedef struct //communicatios_parameters
{
	unsigned short int communication_mode;						// 通讯方式
	unsigned short int communication_protocols;					// 通讯协议
	unsigned short int terminal_comm_address;					// 终端通讯地址
	unsigned short int comm_baudrate;							// 波特率
	unsigned short int comm_databit;							// 数据位
	unsigned short int comm_stopbit;							// 停止位
	unsigned short int comm_paritybit;							// 奇偶校验位
	unsigned short int comm_duplex;								// 半/全双工
	unsigned short int ip_address[4];							// 本机IP地址
	unsigned short int subnet_mask[4];							// 子网掩码
	unsigned short int gateway[4];								// 网关
	unsigned short int mac_address[6];							// MAC地址
	unsigned short int tcp_udp_identity;						// TCP/UPD标识 0：TCP；1：UDP
	unsigned short int upd_port;								// UDP端口号
	unsigned short int tcp_port ;								// TCP端口号
	unsigned short int master_ip_address[4];					// 主站IP地址
	unsigned short int master_port;								// 主站端口号
	unsigned short int master_comm_mode;						// 主站通讯方式
	unsigned short int downlink_comm_interface;					// 下行通讯接口
	unsigned short int downlink_recv_timeout;					// 下行通信接收超时时间 单位：s 默认 5s
	unsigned short int downlink_resend_num;						// 下行通讯失败重发次数 单位：次 默认 3次
	unsigned short int reserved_address[231];					// 预留地址
} __attribute__((packed)) communicatios_parameters ;

//typedef struct
//{
//	RTU_baseinfo baseinfo;
//	communicatios_parameters commparam;
//	AI_DI_parameter aidi_param;
//}A11_sysattr;

///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.2 油井运行采集控制数据存储地址表
// 地址范围40300-40980
// 一, 8 路 AI， 4 路 DI
// 二, 电参
// 三, 变频参数
// 四, 抽油机特有参数
// 五, 螺杆泵特有参数
// 六, 报警故障诊断
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * DI1， 0=软件自动， 1=手动
 */
#define		PE_SOFT_AUTO				0	// 半双工
#define		PE_SOFT_MANUAL			1	// 全双工
/*@brief
 * wsf
 * DI2， 0=停止,1=运行，其它数值自定义
 */
#define		PE_STOP				0	// 停止
#define		PE_RUN				1	// 运行
/*@brief
 * wsf
 * 动力设备PE， 1=开机， 2=关机
 */
#define		PE_POWERON					1	// 开机
#define		PE_POWEROFF				2	// 关机
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 一,8路AI, 4路DI
 * 地址范围40300-40350
 */
typedef struct
{
	unsigned short int a1_oil_pressure[2];						// 油压
	unsigned short int a2_casing_pessure[2];					// 套压
	unsigned short int a3_back_pressure[2];						// 回压
	unsigned short int a4_wellhead_oil_temperature[2];			// 井口油温
	unsigned short int a5_now_polished_rod_load[2];				// 当前悬点载荷(抽油机)
	unsigned short int a6_displacement[2];						// 位移(抽油机)
	unsigned short int a7_custom[2];							// 自定义
	unsigned short int a8_custom[2];							// 自定义
	unsigned short int d1_pe_mt_at;								// 动力设备(power equipment)手/自动状态(0=软件自动， 1=手动)
	unsigned short int d2_pe_running;							// 动力设备运行状态(0=停止,1=运行，其它数值自定义)
	unsigned short int d3_state;								// DI3状态
	unsigned short int d4_state;								// DI4状态
	unsigned short int pe_start_stop;							// 动力设备启停控制(1=开机， 2=关机)
	unsigned short int pe_alarm_delay;							// 动力设备启动报警延时时间(启动报警（响铃）所需时间)
	unsigned short int auto_wake_up;							// 主动唤醒标识
	unsigned short int reserved_address[28];					// 预留地址
}__attribute__((packed)) AI_DI_parameter;
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 二, 电参
 * 地址范围40351-40399
 */
typedef struct
{
	unsigned short int current_phase_a[2];						// 电机工作电流 A 相
	unsigned short int current_phase_b[2];						// 电机工作电流 B 相
	unsigned short int current_phase_c[2];						// 电机工作电流 C 相
	unsigned short int voltage_phase_a[2];						// 电机工作电压 A 相
	unsigned short int voltage_phase_b[2];						// 电机工作电压 B 相
	unsigned short int voltage_phase_c[2];						// 电机工作电压 C 相
	unsigned short int moto_pw[2];								// 电机有功功耗
	unsigned short int moto_qw[2];								// 电机无功功耗
	unsigned short int moto_p[2];								// 电机有功功率
	unsigned short int moto_q[2];								// 电机无功功率
	unsigned short int reverse_power[2];						// 电机反向功率
	unsigned short int power_factor[2];							// 电机功率因数
	unsigned short int reserved_address[25];					// 预留地址
}__attribute__((packed)) electrical_parameter;
/*@brief
 * wsf
 * 电机运行模式(0 - 工频， 1 - 变频)
 */
#define		IP_POWER						1	// 工频
#define		IP_CONVERTER				2	// 变频
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 三, 变频参数
 * 地址范围40400-40419
 */
typedef struct
{
	unsigned short int inverter_mode;							// 电机运行模式(0 - 工频， 1 - 变频)
	unsigned short int inverter_ctrl;							// 频率控制方式
	unsigned short int set_frequency[2];						// 变频设置频率(Hz)
	unsigned short int run_frequency[2];						// 变频运行频率(Hz)
	unsigned short int inverter_fault;							// 变频故障状态
	unsigned short int reserved_address[13];					// 预留地址
}__attribute__((packed)) inverter_parameter;
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 四, 抽油机特有参数
 * 地址范围40420-40429
 */
typedef struct
{
	unsigned short int pumping_speed[2];						// 冲次
	unsigned short int pumping_stroke[2];						// 冲程
	unsigned short int reserved_address[6];						// 预留地址
}__attribute__((packed)) pumping_parameter;
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 五, 螺杆泵特有参数
 * 地址范围40430-40439
 */
typedef struct
{
	unsigned short int stp_speed[2];							// 转速
	unsigned short int stp_torque[2];							// 扭矩
	unsigned short int reserved_address[6];						// 预留地址
}__attribute__((packed)) screw_type_pump_parameter;
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 六, 报警故障诊断
 * 地址范围40440-40980
 */
typedef struct
{
	unsigned short int terminal_reg1;							// 终端自身状态寄存 1
	unsigned short int terminal_reg2;							// 终端自身状态寄存 2
	unsigned short int AI_troublecode1;							// AI 仪表故障码 1
	unsigned short int AI_troublecode2;							// AI 仪表故障码 2
	unsigned short int AI_alarmcode1;							// AI 报警码 1
	unsigned short int AI_alarmcode2;							// AI 报警码 2
	unsigned short int a7_custom;								// DI 报警码
	unsigned short int a8_custom;								// 动力设备停机原因
	unsigned short int d1_pe_mt_at;								// 动力设备开机原因
	unsigned short int reserved_address[532];					// 预留地址
}__attribute__((packed)) alarm_fault_diagnosis;
///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.3 油井示功图采集数据存储地址表
// 地址范围40981-43000
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * E.3 油井示功图采集数据存储地址表
 * 地址范围40981-43000
 */
typedef struct
{
	unsigned short int interval;								// 功图自动测量间隔
	unsigned short int manul_collection_order;					// 手工采集功图指令
	unsigned short int set_dot;									// 功图采集设置点数
	unsigned short int actual_dot;								// 功图实际点数
	unsigned short int collection_datetime[6];					// 功图采集时间
	unsigned short int pumping_speed[2];						// 冲次
	unsigned short int pumping_stroke[2];						// 冲程
	unsigned short int reserved_address1[6];					// 预留地址
	unsigned short int displacement[250];						// 当前示功图数据-位移值250点
	unsigned short int load[250];								// 位移点对应载荷值 250 点
	unsigned short int current[250];							// 位移点对应电流值 250 点
	unsigned short int power[250];								// 位移点对应有功功率值 250点
//	unsigned short int reserved_address2[1000];					// 预留地址
}__attribute__((packed)) load_displacement;
///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.4 油井功能参数控制指令存储地址表
// 地址范围43001-49999
// 一, 报警限值参数设置
// 二, 节能控制控制参数
// 三, 自定义（ 599 个地址）
// 四, 仪表基本信息
// 五, 标准预留
// 六, 厂家自定义
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 一,报警限值参数设置
 * 地址范围43001-43360
 */
typedef struct
{
	unsigned short int sig_type;								// AI 仪表信号形式 (1-有线， 11-无线， 21-485 表)
	unsigned short int lrv[2];									// AI 仪表量程下限
	unsigned short int urv[2];									// AI 仪表量程上限
	unsigned short int low_alarm_limit[2];						// AI 仪表报警下限
	unsigned short int upper_alarm_limit[2];					// AI 仪表报警上限
	unsigned short int low_act;									// AI 仪表报警下限动作
	unsigned short int upper_act;								// AI 仪表报警上限动作
	unsigned short int reserved_address[9];						// AI 预留地址
}__attribute__((packed)) ai_alramparam;

typedef struct
{
	unsigned short int sig_type;								// DI 信号形式
	unsigned short int sig_on_off;								// DI 信号开-关动作
	unsigned short int sig_off_on;								// DI 信号关-开动作
	unsigned short int reserved_address[7];						// DI 预留地址
}__attribute__((packed)) di_alramparam;
typedef struct
{
//	ai_alramparam ai1_alarm_param;								// AI1 报警限值参数设置
//	ai_alramparam ai2_alarm_param;								// AI2 报警限值参数设置
//	ai_alramparam ai3_alarm_param;								// AI3 报警限值参数设置
//	ai_alramparam ai4_alarm_param;								// AI4 报警限值参数设置
//	ai_alramparam ai5_alarm_param;								// AI5 报警限值参数设置
//	ai_alramparam ai6_alarm_param;								// AI6 报警限值参数设置
//	ai_alramparam ai7_alarm_param;								// AI7 报警限值参数设置
//	ai_alramparam ai8_alarm_param;								// AI8 报警限值参数设置
	ai_alramparam ai_alarm_param[16];							// AI   报警限值参数设置
	di_alramparam di_alarm_param[4];								// DI   报警限值参数设置

}__attribute__((packed)) alarm_parameter;
/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 二,节能控制控制参数
 * 地址范围43361-43400
 */
typedef struct
{
	unsigned short int ep_enable;								// 空抽控制使能ep = empty_pumping
	unsigned short int ep_stoptime;								// 空抽停机时间	(空抽后停机的时间)
	unsigned short int reserved_address1[8];					// 空抽控制预留地址
	unsigned short int timer_ip_enable;							// 定时-间抽控制使能ip=intermissive pumping
	unsigned short int timer_ip_starttime[3];					// 定时-间抽开机时间
	unsigned short int timer_ip_stoptime[3];					// 定时-间抽停机时间
	unsigned short int reserved_address2[3];					// 定时-间抽控制预留地址
	unsigned short int regular_ip_enable;						// 定间隔-间抽控制使能
	unsigned short int regular_ip_starttime;					// 定间隔-间抽关机时间
	unsigned short int regular_ip_stoptime;						// 定间隔-间抽开机时间
	unsigned short int reserved_address4[7];					// 定间隔-间抽控制预留参数
	unsigned short int startup_enable;							// 上电自动启抽控制使能
	unsigned short int startup_delay;							// 上电自动启抽延时时间
	unsigned short int di_alarm_param[8];						// 上电自动启抽控制预留地址
}__attribute__((packed)) energy_conserve_parameter;
/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 三,自定义（ 599 个地址）
 * 地址范围43401-43999
 */
typedef struct
{
	unsigned short int u_def[599];
}__attribute__((packed)) user_define;
/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 四,仪表基本信息
 * 地址范围44000-44599
 */
typedef struct
{
	unsigned short int company_code;							// 厂商代码
	unsigned short int device_type;								// 仪表类型
	unsigned short int device_group;							// 仪表组号
	unsigned short int device_no;								// 仪表编号
	unsigned short int comm_efficiency;							// 通信效率
	unsigned short int bat_vol;									// 电池电压
	unsigned short int sleep_time;								// 休眠时间
	unsigned short int device_sta;								// 仪表状态
	unsigned short int work_temp[2];							// 工作温度
	unsigned short int realtime_data[2];						// 当前实时数据 该位为自行添加
	unsigned short int reserved_address[8];						// 预留
}__attribute__((packed)) device_base_information;
typedef struct
{
	device_base_information oil_pressure;						// 油压仪表
	device_base_information casing_pressure;					// 套压仪表
	device_base_information back_pressure;						// 回压仪表
	device_base_information wellhead_oil_temp;					// 井口油温仪表
	device_base_information load;								// 载荷仪表
	device_base_information displacement;						// 位移仪表
	device_base_information other_device[24];					// 其他仪表
}__attribute__((packed)) device_information;
/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 五,标准预留
 * 地址范围44600-48999
 */
typedef struct
{
	unsigned short int st_reserved[4400];						// 标准预留地址4400个地址
	// 大庆A11扩展了此表从4700地址开始
	// 表9-5 油井 RTU 补传历史数据存储地址表
//	unsigned short int st_reserved[2400];
//	unsigned short int history_start_time[5];					// 写要读取历史数据的起始时间
//	unsigned short int history_index;							// 历史数据条目索引号
	// 预留
	// RTU记录中，最早一条历史数据得时间
	// 预留
	// 历史数据条目查询成功后，此条历史数据记录得创建时间
	// 历史数据区
	// 油压值
	// 套压值
	// 回压值
	// 温度值
	// 油压上传的时间
	// 套压上传的时间
	// 回压上传的时间
	// 温度上传的时间
	// 电参的周期采集时间
	// 油套回温度的连接报警状态
	// 油套回温度上下限报警状态
	// 功图自动测量间隔
	// 手工采集功图指令
	// 功图采集设置点数
	// 功图实际点数
	// 功图采集时间
	// 冲次
	// 冲程
	// 预留地址
	// 250点位移量
	// 250点载荷量
	// 250点电流值
	// 250点功率值
	// 预留地址 920
}__attribute__((packed)) standard_resvered;

typedef enum {
    _ON = 0,
    _OFF
} A11_switch_type_t;

typedef enum {
    _TRANSPARENT = 0,
    _NETWORK
} A11_data_transfer_t;

typedef enum {
    _REALDATA = 0,
	_SIMULATE = 0x10
} A11_dynagraph_mode_t;
/* @brief
 * wsf
 * 仪表参数：类型、组号、编号、地址 (地址：用于有线仪表 地址为 0：无线仪表 地址非 0：有线仪表)
 */
typedef struct
{
	unsigned char type;											// 仪表类型
	unsigned char group;										// 仪表组号
	unsigned char num;											// 仪表编号
	unsigned char addr;											// 仪表地址
} __attribute__((packed)) instrument_parameter;
/* @brief
 * wsf
 * 大庆厂家自定义中得IP扩展
 */
typedef struct
{
	unsigned short int mac[6]; 									// MAC地址
	unsigned short int comm_duplex; 							// 半/全双工 (0-半双工，1=全双工)
	unsigned short int IPv4_ip[4]; 								// 本地IPv4地址
	unsigned short int IPv4_subnet_mask[4];						// IPv4 子网掩码
	unsigned short int IPv4_gateway[4];							// IPv4 网关
	unsigned short int IPv4_tcp_udp_identity;					// TCP/UPD标识 (0：TCP；1：UDP)
	unsigned short int IPv4_access;								// IPv4 IP获取方式 (0-固定，1-DHCP)
	unsigned short int IPv4_upd_port;							// IPv4 本地UPD端口
	unsigned short int IPv4_tcp_port;							// IPv4 本地TCP端口
	unsigned short int IPv4_master_ip[4];						// IPv4 主站IP地址
	unsigned short int IPv4_master_port;						// IPv4 主站端口号
	unsigned short int IPv4_obtain_ip[4];						// 获取的 IPv4 地址
	unsigned short int IPv6_ip[8];								// 本地 IPv6 地址
	unsigned short int IPv6_prefix_len;	 						// IPv6 子网长度
	unsigned short int IPv6_gateway[8];							// IPv6 网关
	unsigned short int IPv6_tcp_udp_identity;					// IPv6 TTCP/UDP标识
	unsigned short int IPv6_access;								// IPv6 IP获取方式
	unsigned short int IPv6_upd_port;							// IPv6 本地UPD端口
	unsigned short int IPv6_tcp_port;							// IPv6 本地TCP端口
	unsigned short int IPv6_master_ip[8];						// IPv6 主站IP地址
	unsigned short int IPv6_master_port;						// IPv6 主站端口号
	unsigned short int IPv6_obtain_ip[8];						// 获取的 IPv6 地址
} __attribute__((packed)) IPv4_IPv6_extend;


/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 六,厂家自定义
 * 地址范围49000-49999
 */
typedef struct
{
//	unsigned short int custom[1000];							// 厂家自定义区1000个地址
	// 2.2.3 大庆油田统一的厂家自定义寄存器规划表检查
	unsigned short int oilwell_ID[16];							// 16 口油井得油井ID
	unsigned short int RTU_ID[2];								// RTU_ID
	unsigned char reserved1[4];									// 预留
	unsigned short int communication_protocols;					// RTU与上位机协议类型(0：Modbus RTU	1：Modbus TCP/IP	2：DNP3.0	3：大庆 GRM 协议)
	unsigned short int data_transfer_mode;						// 数据传输模式(0：透传模式 1：网络识别模式)
	unsigned short int heartbeat_sta;							// 心跳状态 (0：关闭 1：开启)
	unsigned short int non_heartbeat_interval;					// 无应答心跳间隔时间 (单位：s（0～65535）)
	unsigned short int heartbeat_interval;						// 有应答心跳间隔时间 (单位：s（0～65535）)
	unsigned short int patrol_num;								// RTU巡检总井数 (1～8)
	unsigned short int dynagraph_mode;							// 功图测试类型 (0：实际功图	0x10：模拟功图)
	unsigned short int dynagraph_dot;							// 功图测试点数 (200～250)
	unsigned short int elec_switch;								// 电量图测试状态 (0：关闭，1：开启)
	unsigned short int offline_savedata_switch;					// 断网存储状态 (0：关闭，1：开启)
	unsigned short int A1alram_switch;							// A1报警 (0：关闭，1：开启)
	unsigned short int A1alram_upload_cycle;					// A1报警上传周期 (单位 s)
	unsigned short int update_packet_size;						// 更新单元单包发送字节数 (必须是 32 的整数倍)
	unsigned char reserved2[4];									// 预留
	unsigned short int dynagraph_patroltime;					// 功图巡检时间 (单位 s,0:关闭有线巡检)
	unsigned short int press_patroltime;						// 压力巡检时间 (同上)
	unsigned short int tempreture_patroltime;					// 温度巡检时间 (同上)
	unsigned short int elec_patroltime;							// 电参巡检时间 (同上)
	unsigned short int angledisplacement_patroltime;			// 角位移巡检时间 (同上)
	unsigned short int load_patroltime;							// 载荷巡检时间 (同上)
	unsigned short int touque_patroltime;						// 扭矩巡检时间 (同上)
	unsigned short int liquid_patroltime;						// 液面巡检时间 (同上)
	unsigned short int rpm_patroltime;							// 扭矩转速巡检时间 (同上)
	unsigned short int analog_patroltime;						// 模拟量巡检时间 (同上)
	unsigned short int switch_patroltime;						// 开关量巡检时间 (同上)
	unsigned char reserved3[108];								// 预留
	instrument_parameter instrument[63];						// 仪表1参数：类型、组号、编号、地址 (地址：用于有线仪表 地址为 0：无线仪表 地址非 0：有线仪表)
	unsigned char effective_instrument_num;						// 有效仪表数量
	unsigned char total_instrument;								// 全部仪表数量
	unsigned char reserved4[2];									// 保留
	unsigned char reserved5[58];								// 保留
	unsigned short int MUX[51];									// 复用寄存器49257~49307
	unsigned short int update_info[268];						// 在线更新信息
	unsigned short int MUX2[83];								// 复用寄存器
	IPv4_IPv6_extend IP_extern;									// IP扩展
	unsigned char reserved6[596];								// 保留
}__attribute__((packed)) manufacturers_of_custom;

// E1 远程终端单元系统属性数据存储地址
// 该表中汇集了需要存储到配置文件上得一些数据格式
typedef struct
{
	RTU_baseinfo baseinfo;
	communicatios_parameters commparam;
	AI_DI_parameter aidi_param;
//	load_displacement dynagraph;
}E1_sys_attribute;
/*
 * 当前使用方式为只使用一个RTU，所以将E2表中得关于AIDI的参数转移至地址中
 */

// E2 油井运行采集控制数据存储地址表
typedef struct
{
//	AI_DI_parameter aidi_param;									// 8 路 AI， 4 路 DI
	electrical_parameter elec_param;							// 电参
	inverter_parameter inverter_param;							// 变频参数
	pumping_parameter pumping_param;							// 抽油机特有参数
	screw_type_pump_parameter screw_patam;						// 螺杆泵特有参数
	alarm_fault_diagnosis alarm_fault;							// 报警故障诊断
}E2_run_ctrl;
// E3 油井示功图采集数据存储地址表
typedef struct
{
	load_displacement dynagraph;								// 油井示功图采集数据存储地址表
	unsigned short int reserved_address2[1000];					// 预留地址
}E3_load_displacement;
// E4 油井功能参数控制指令存储地址表
typedef struct
{
	alarm_parameter alarm_param;								// 报警限值参数设置
	energy_conserve_parameter ennerpy_conserver_param;			// 节能控制控制参数
	user_define user_def;										// 自定义（ 599 个地址）
	device_information device_infor;							// 仪表基本信息
	standard_resvered st_reserved;								// 标准预留
	manufacturers_of_custom custom;								// 厂家自定义
}E4_function_parameter;
/*@brief
 * wsf
 * E1 + E2 + E3 + E4
 * 001-油井(生产井)
 */
typedef struct
{
//	E1_sys_attribute sys_attr;
	E2_run_ctrl run_ctrl;
	E3_load_displacement load_displacement;
	E4_function_parameter fuction_param;
}oil_well;

// 初始化 baseinfo
RTU_baseinfo* baseinfoInit(void);
/*@brief
 * wsf
 * 初始化communcnications_parameters
 */
communicatios_parameters* commParametersInit(void);

E1_sys_attribute* LoadConfigA11();



#endif /* SRC_A11_SYSATTR_A11SYSATTR_H_ */
