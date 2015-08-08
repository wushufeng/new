/*
 * a11sysattr.c
 *
 *  Created on: 2015年3月26日
 *      Author: wsf
 */


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "a11sysattr.h"
#include "../log/rtulog.h"
#include "../myfunction/myfun.h"
#include "../sever/sysdatetime/getsysdatetime.h"
#include "../inifile/inifile.h"
//#include "../sever/sysdatetime//getsysdatetime.h"
#include "../modbus/modbus.h"

extern modbus_mapping_t *mb_mapping[17];

const char *A11filename = "./A11config.ini";
int basesInfoInit(RTU_baseinfo * p_baseinfo);
int commParamInit(communicatios_parameters * p_commparam);
int createA11Configfile(E1_sys_attribute * psysattr,const char *filename);
int getA11Configure(E1_sys_attribute *psysattr);
static int manufacturersParamInit(manufacturers_of_custom * pobj);
oil_well *poilwell[17];

E1_sys_attribute* LoadConfigA11(void)
{
	int res;
	int n;

	E1_sys_attribute *psysattr = (E1_sys_attribute *)mb_mapping[0]->tab_registers;
	res = 0;
	if (IsFileExist(A11filename)) {
		zlog_warn(c, "A11配置文件不存在，创建新文件并写入默认配置");
		if(createA11Configfile(psysattr,A11filename) == 0)
        	zlog_info(c, "创建A11配置成功，并读入默认配置");
        else
        	zlog_error(c, "创建A11配置失败，从程序读入默认配置");
    }
	else
	{
		res = getA11Configure(psysattr);
		if(res  == 0)
			zlog_info(c, "成功从配置文件读入默认配置");
	}
	if(res == 0)
	{
		switch (psysattr->baseinfo.well_station_type)
		{
			case WST_SINGLE_WELL:											// 单井0XX
				break;
			case WST_VALVE_VAULT:											// 阀室
				break;
			case WST_OIL_WELL:													// 油井（生产井）
				zlog_info(c, "当前配置为得应用井站类型为\"油井\"(001)");
				zlog_info(c, "初始化数据格式为油井格式");
				for(n = 0; n < 17; n ++)
					poilwell[n] = (oil_well *)(mb_mapping[n]->tab_registers + sizeof(E1_sys_attribute) / 2);

				manufacturersParamInit(&poilwell[0]->fuction_param.custom);
//				poilwell[0]->fuction_param.custom.oilwell_ID[0] = 0x01;
//				poilwell[0]->fuction_param.custom.oilwell_ID[1] = 0x02;
				break;
			case WST_GASS_WELL:												// 气井
				break;
			case WST_WATER_SOURCE_WELL:							// 水源井
				break;
			case WST_INJECTION_WELL:										// 注水井
				break;
			case WST_GAS_INJECTION_WELL:								// 注气井
				break;
			case WST_OBSERVING_WELL:									// 观察井
				break;
			case WST_METERING_STATION:								// 计量站
				break;
			case WST_BOOSTER_STATION:									// 增压站
				break;
			case WST_GAS_GATHERING_STATION:						// 集气站
				break;
			case WST_GAS_TRANSMISSION_STATION:				//输气站
				break;
			case WST_DISTRIBUTION_STATION:						// 配气站
				break;
			case WST_RE_INJECTION_STATION:							// 水处理（回注）站配气站
				break;
			case WST_DEHYDRATION_STATION:						// 脱水站
				break;
			default:
				break;
		}
	}
	return psysattr;
}
int getA11Configure(E1_sys_attribute *psysattr)
{
	const int str_size = 32;
	char  str[str_size];
	psysattr->baseinfo.password = read_profile_int("rtuBaseInfo","password",-1,A11filename);
	if(psysattr->baseinfo.password == -1)
			return -1;
	psysattr->baseinfo.type_version = read_profile_int("rtuBaseInfo","type_version",-1,A11filename);
	if(psysattr->baseinfo.type_version == -1)
			return -1;
	psysattr->baseinfo.dev_company = read_profile_int("rtuBaseInfo","dev_company",-1,A11filename);
	if(psysattr->baseinfo.dev_company == -1)
			return -1;
	psysattr->baseinfo.well_station_type = read_profile_int("rtuBaseInfo","well_station_type",-1,A11filename);
	if(psysattr->baseinfo.well_station_type == -1)
			return -1;
	if(getSysLocalDateTime((void *)psysattr) != 0)
		return -1;
	psysattr->commparam.downlink_resend_num =read_profile_int("commParam","downlink_resend_num",-1,A11filename);
	if(psysattr->commparam.downlink_resend_num == -1)
			return -1;
	psysattr->commparam.downlink_recv_timeout =read_profile_int("commParam","downlink_recv_timeout",-1,A11filename);
	if(psysattr->commparam.downlink_recv_timeout == -1)
			return -1;
	psysattr->commparam.downlink_comm_interface =read_profile_int("commParam","downlink_comm_interface",-1,A11filename);
	if(psysattr->commparam.downlink_comm_interface == -1)
			return -1;
	psysattr->commparam.master_comm_mode =read_profile_int("commParam","master_comm_mode",-1,A11filename);
	if(psysattr->commparam.master_comm_mode == -1)
			return -1;
	psysattr->commparam.master_port =read_profile_int("commParam","master_port",-1,A11filename);
	if(psysattr->commparam.master_port == -1)
			return -1;

	read_profile_string("commParam","master_ip_address",str,str_size,"",A11filename);
	myIPtoa(".",str,psysattr->commparam.master_ip_address);

	psysattr->commparam.tcp_port =read_profile_int("commParam","tcp_port",-1,A11filename);
	if(psysattr->commparam.tcp_port == -1)
			return -1;
	psysattr->commparam.upd_port =read_profile_int("commParam","upd_port",-1,A11filename);
	if(psysattr->commparam.upd_port == -1)
			return -1;
	psysattr->commparam.tcp_udp_identity =read_profile_int("commParam","tcp_udp_identity",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	if(!read_profile_string("commParam","mac_address",str,str_size,NULL,A11filename))
		return -1;
	else
		myIPtoa(":",str,psysattr->commparam.mac_address);

	if(!read_profile_string("commParam","gateway",str,str_size,"",A11filename))
		return -1;
	else
		myIPtoa(".",str,psysattr->commparam.gateway);

	if(!read_profile_string("commParam","subnet_mask",str,str_size,"",A11filename))
			return -1;
		else
			myIPtoa(".",str,psysattr->commparam.subnet_mask);

	if(!read_profile_string("commParam","ip_address",str,str_size,"",A11filename))
		return -1;
	else
		myIPtoa(".",str,psysattr->commparam.ip_address);

	psysattr->commparam.comm_duplex =read_profile_int("commParam","comm_duplex",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	psysattr->commparam.comm_paritybit =read_profile_int("commParam","comm_paritybit",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	psysattr->commparam.comm_stopbit =read_profile_int("commParam","comm_stopbit",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	psysattr->commparam.comm_databit =read_profile_int("commParam","comm_databit",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	psysattr->commparam.comm_baudrate =read_profile_int("commParam","comm_baudrate",-1,A11filename);
	if(psysattr->commparam.tcp_udp_identity == -1)
			return -1;
	psysattr->commparam.terminal_comm_address =read_profile_int("commParam","terminal_comm_address",-1,A11filename);
	if(psysattr->commparam.terminal_comm_address == -1)
				return -1;
	psysattr->commparam.communication_protocols =read_profile_int("commParam","communication_protocols",-1,A11filename);
	if(psysattr->commparam.communication_protocols == -1)
				return -1;
	psysattr->commparam.communication_mode =read_profile_int("commParam","communication_mode",-1,A11filename);
	if(psysattr->commparam.communication_mode == -1)
				return -1;
	return 0;
}
int createA11Configfile(E1_sys_attribute * psysattr,const char *filename)
{
	char str[32];
	// 用默认值初始化baseinfo
	basesInfoInit(&psysattr->baseinfo);
	// 用默认值初始化commparam
	commParamInit(&psysattr->commparam);
	// 将内从中的baseinfo参数写入配置文件
	sprintf(str,"%d",psysattr->baseinfo.password);
	if (!write_profile_string("rtuBaseInfo","password",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->baseinfo.type_version);
	if (!write_profile_string("rtuBaseInfo","type_version",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->baseinfo.dev_company);
	if (!write_profile_string("rtuBaseInfo","dev_company",str,A11filename)) return -1;
	sprintf(str,"%03d",psysattr->baseinfo.well_station_type);
	if (!write_profile_string("rtuBaseInfo","well_station_type",str,A11filename)) return -1;
	if(getSysLocalDateTime((void *)psysattr) != 0)
		return -1;
	// 将内存中的commparam参数写入配置文件
	sprintf(str,"%d",psysattr->commparam.downlink_resend_num);
	if (!write_profile_string("commParam","downlink_resend_num",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.downlink_recv_timeout);
	if (!write_profile_string("commParam","downlink_recv_timeout",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.downlink_comm_interface);
	if (!write_profile_string("commParam","downlink_comm_interface",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.master_comm_mode);
	if (!write_profile_string("commParam","master_comm_mode",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.master_port);
	if (!write_profile_string("commParam","master_port",str,A11filename)) return -1;

	sprintf(str,"%d.%d.%d.%d",psysattr->commparam.master_ip_address[0],
			psysattr->commparam.master_ip_address[1],
			psysattr->commparam.master_ip_address[2],
			psysattr->commparam.master_ip_address[3]);
	if (!write_profile_string("commParam","master_ip_address",str,A11filename)) return -1;

	sprintf(str,"%d",psysattr->commparam.tcp_port);
	if (!write_profile_string("commParam","tcp_port",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.upd_port);
	if (!write_profile_string("commParam","upd_port",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.tcp_udp_identity);
	if (!write_profile_string("commParam","tcp_udp_identity",str,A11filename)) return -1;

	sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",psysattr->commparam.mac_address[0],
			psysattr->commparam.mac_address[1],
			psysattr->commparam.mac_address[2],
			psysattr->commparam.mac_address[3],
			psysattr->commparam.mac_address[4],
			psysattr->commparam.mac_address[5]);
	if (!write_profile_string("commParam","mac_address",str,A11filename)) return -1;

	sprintf(str,"%d.%d.%d.%d",psysattr->commparam.gateway[0],
			psysattr->commparam.gateway[1],
			psysattr->commparam.gateway[2],
			psysattr->commparam.gateway[3]);
	if (!write_profile_string("commParam","gateway",str,A11filename)) return -1;

	sprintf(str,"%d.%d.%d.%d",psysattr->commparam.subnet_mask[0],
			psysattr->commparam.subnet_mask[1],
			psysattr->commparam.subnet_mask[2],
			psysattr->commparam.subnet_mask[3]);
	if (!write_profile_string("commParam","subnet_mask",str,A11filename)) return -1;

	sprintf(str,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
			psysattr->commparam.ip_address[1],
			psysattr->commparam.ip_address[2],
			psysattr->commparam.ip_address[3]);
	if (!write_profile_string("commParam","ip_address",str,A11filename)) return -1;

	sprintf(str,"%d",psysattr->commparam.comm_duplex);
	if (!write_profile_string("commParam","comm_duplex",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.comm_paritybit);
	if (!write_profile_string("commParam","comm_paritybit",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.comm_stopbit);
	if (!write_profile_string("commParam","comm_stopbit",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.comm_databit);
	if (!write_profile_string("commParam","comm_databit",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.comm_baudrate);
	if (!write_profile_string("commParam","comm_baudrate",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.terminal_comm_address);
	if (!write_profile_string("commParam","terminal_comm_address",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.communication_protocols);
	if (!write_profile_string("commParam","communication_protocols",str,A11filename)) return -1;
	sprintf(str,"%d",psysattr->commparam.communication_mode);
	if (!write_profile_string("commParam","communication_mode",str,A11filename)) return -1;

	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.1 远程终端单元系统属性数据存储地址
// 地址范围40001-40299
// 一,RTU 基本信息
// 二, 通信类参数
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * E.1 远程终端单元系统属性数据存储地址
 * 一,RTU基本信息
 */
int basesInfoInit(RTU_baseinfo * p_baseinfo)
{
	p_baseinfo->well_station_type = WST_OIL_WELL;
	p_baseinfo->dev_company = 34;
	p_baseinfo->type_version = 17;
	p_baseinfo->password	= 4660;
//	p_sys_time = getSysLocalTime();
//	p_baseinfo->day_of_start_time[0] = p_baseinfo->sys_time[0] = p_sys_time->local_hour;
//	p_baseinfo->day_of_start_time[1] = p_baseinfo->sys_time[1] = p_sys_time->local_minitue;
//	p_baseinfo->day_of_start_time[2] = p_baseinfo->sys_time[2] = p_sys_time->local_second;
//	free(p_sys_time);
//	p_sys_date = getSysLocalDate();
//	p_baseinfo->sys_date[0] = p_sys_date->local_year;
//	p_baseinfo->sys_date[1] = p_sys_date->local_month;
//	p_baseinfo->sys_date[2] = p_sys_date->local_day;
//	free(p_sys_date);

//	p_baseinfo->ram_vol = getRamVoltage();
	p_baseinfo->ram_vol = 0.01 * 100;
//	p_baseinfo->cabinet_temperature = getCabinetTemperature();
	p_baseinfo->cabinet_temperature = 30;
	return 0;
}
/*@brief
 * wsf
 * 表 E.1 远程终端单元系统属性数据存储地址
 * 二, 通信类参数
 * 地址范围40031-40299
 */
int commParamInit(communicatios_parameters * p_commparam)
{
	p_commparam->communication_mode = CM_WIRELESS_BRIDGES;
	p_commparam->communication_protocols = CP_MB_TCPIP;
	p_commparam->terminal_comm_address = 0;
	p_commparam->comm_baudrate = C_BAUD_57600;
	p_commparam->comm_databit = C_DATA_BIT_8;
	p_commparam->comm_stopbit = C_STOP_BIT_1;
	p_commparam->comm_paritybit = C_NONE;
	p_commparam->comm_duplex = CE_FULL_DUPLEX;
	p_commparam->ip_address[0] = 192;
	p_commparam->ip_address[1] = 168;
	p_commparam->ip_address[2] = 93;
	p_commparam->ip_address[3] = 128;
	p_commparam->subnet_mask[0] = 255;
	p_commparam->subnet_mask[1] = 255;
	p_commparam->subnet_mask[2] = 255;
	p_commparam->subnet_mask[3] = 0;
	p_commparam->gateway[0] = 192;
	p_commparam->gateway[1] = 168;
	p_commparam->gateway[2] = 93;
	p_commparam->gateway[3] = 1;
	p_commparam->mac_address[0] = 0xE8;
	p_commparam->mac_address[1] = 0x2A;
	p_commparam->mac_address[2] = 0xEA;
	p_commparam->mac_address[3] = 0x09;
	p_commparam->mac_address[4] = 0x79;
	p_commparam->mac_address[5] = 0xAF;
	p_commparam->tcp_udp_identity = TUI_TCP;
	p_commparam->upd_port = 1503;
	p_commparam->tcp_port = 1502;
	p_commparam->master_ip_address[0] = 192;
	p_commparam->master_ip_address[1] = 168;
	p_commparam->master_ip_address[2] = 93;
	p_commparam->master_ip_address[3] = 1;
	p_commparam->master_port = 1506;
	p_commparam->master_comm_mode = MCM_TCP_SERVER;
	p_commparam->downlink_comm_interface = DCI_ZIGBEE;
	p_commparam->downlink_recv_timeout = 5;
	p_commparam->downlink_resend_num = 3;
	return 0;
}
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
 * E2油井运行采集控制数据存储地址表
 * 一,8路AI, 4路DI
 * 地址范围40300-40350
 */
int AI_DIInit(AI_DI_parameter * obj)
{
	obj->d1_pe_mt_at = 0;
	obj->d2_pe_running = 0;
	obj->pe_start_stop = 1;
	obj->pe_alarm_delay = 5;
	obj->auto_wake_up = 0;
	return 0;
}
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 二, 电参
 * 地址范围40351-40399
 */
int electricalParamInit(electrical_parameter * obj)
{
	return 0;
}
/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 三, 变频参数
 * 地址范围40400-40419
 */

/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 四, 抽油机特有参数
 * 地址范围40420-40429
 */

/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 五, 螺杆泵特有参数
 * 地址范围40430-40439
 */

/*@brief
 * wsf
 * E2油井运行采集控制数据存储地址表
 * 六, 报警故障诊断
 * 地址范围40440-40980
 */
///////////////////////////////////////////////////////////////////////////////////////////////
// 表 E.3 油井示功图采集数据存储地址表
// 地址范围40981-43000
///////////////////////////////////////////////////////////////////////////////////////////////
/*@brief
 * wsf
 * E.3 油井示功图采集数据存储地址表
 *
 */
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
 */

/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 二,节能控制控制参数
 */

/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 三,自定义（ 599 个地址）
 */

/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 四,仪表基本信息
 */

/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 五,标准预留
 */

/*@brief
 * wsf
 * E.4 油井功能参数控制指令存储地址表
 * 六,厂家自定义
 */
static int manufacturersParamInit(manufacturers_of_custom * pobj)
{
	E1_sys_attribute *psysattr = (E1_sys_attribute *)mb_mapping[0]->tab_registers;
//	// 49000
//	pobj->oilwell_ID[0] = 0x0102;
//	pobj->oilwell_ID[1] = 0x0304;
//	// 49020
//	pobj->communication_protocols = 0x0506;
//	// 49100
//	pobj->instrument[0].type = 0x01;
//	pobj->instrument[0].group = 0x02;
//	pobj->instrument[0].num = 0x03;
//	pobj->instrument[0].addr = 0x04;
//
//	// 49257
//	pobj->MUX[0] = 0x9876;
//	// 49308
//	pobj->update_info[0] = 0x4578;
//	// 49576 Set an incorrect number of values
//	pobj->MUX2[0] = 0x5678;
//	// 49659
//	pobj->IP_extern.mac[0] = 0x4567;

	memcpy(&pobj->communication_protocols, &psysattr->commparam.communication_protocols, 2);
	pobj->data_transfer_mode = 1;
	pobj->heartbeat_sta = 0;
	pobj->non_heartbeat_interval = 0;
	pobj->heartbeat_interval = 0;
	pobj->patrol_num = 8;
	pobj->dynagraph_mode = 0x10;
	pobj->dynagraph_dot = 200;
	// IPv4
	memcpy(pobj->IP_extern.mac, psysattr->commparam.mac_address, 12);
	memcpy(&pobj->IP_extern.comm_duplex, &psysattr->commparam.comm_duplex, 2);
	memcpy(pobj->IP_extern.IPv4_ip, psysattr->commparam.ip_address, 8);
	memcpy(pobj->IP_extern.IPv4_subnet_mask, psysattr->commparam.subnet_mask, 8);
	memcpy(pobj->IP_extern.IPv4_gateway, psysattr->commparam.gateway, 8);
	pobj->IP_extern.IPv4_tcp_udp_identity = psysattr->commparam.tcp_udp_identity;
	pobj->IP_extern.IPv4_access = 0;
	pobj->IP_extern.IPv4_upd_port = psysattr->commparam.upd_port;
	pobj->IP_extern.IPv4_tcp_port = psysattr->commparam.tcp_port;
	memcpy(pobj->IP_extern.IPv4_master_ip, psysattr->commparam.master_ip_address, 8);
	pobj->IP_extern.IPv4_master_port = psysattr->commparam.master_port;
	// IPv6 fe80::20c:29ff:fedc:6888
	pobj->IP_extern.IPv6_ip[0] = 0xfe80;
	pobj->IP_extern.IPv6_ip[1] = 0x0000;
	pobj->IP_extern.IPv6_ip[2] = 0x0000;
	pobj->IP_extern.IPv6_ip[3] = 0x0000;
	pobj->IP_extern.IPv6_ip[4] = 0x020c;
	pobj->IP_extern.IPv6_ip[5] = 0x29ff;
	pobj->IP_extern.IPv6_ip[6] = 0xfedc;
	pobj->IP_extern.IPv6_ip[7] = 0x6888;
	pobj->IP_extern.IPv6_prefix_len = 0;
	pobj->IP_extern.IPv6_gateway[0] = 0;
	pobj->IP_extern.IPv6_gateway[1] = 0;
	pobj->IP_extern.IPv6_gateway[2] = 0;
	pobj->IP_extern.IPv6_gateway[3] = 0;
	pobj->IP_extern.IPv6_gateway[4] = 0;
	pobj->IP_extern.IPv6_gateway[5] = 0;
	pobj->IP_extern.IPv6_gateway[6] = 0;
	pobj->IP_extern.IPv6_gateway[7] = 0;
	pobj->IP_extern.IPv6_tcp_udp_identity = psysattr->commparam.tcp_udp_identity;
	pobj->IP_extern.IPv6_access = 0;
	pobj->IP_extern.IPv6_upd_port = psysattr->commparam.upd_port;
	pobj->IP_extern.IPv6_tcp_port = psysattr->commparam.tcp_port;
	pobj->IP_extern.IPv6_obtain_ip[0] = 0;
	pobj->IP_extern.IPv6_obtain_ip[1] = 0;
	pobj->IP_extern.IPv6_obtain_ip[2] = 0;
	pobj->IP_extern.IPv6_obtain_ip[3] = 0;
	pobj->IP_extern.IPv6_obtain_ip[4] = 0;
	pobj->IP_extern.IPv6_obtain_ip[5] = 0;
	pobj->IP_extern.IPv6_obtain_ip[6] = 0;
	pobj->IP_extern.IPv6_obtain_ip[7] = 0;
//	get_float
	return 0;
}
//RTU_baseinfo* baseinfoInit(void)
//{
//	RTU_baseinfo *p_baseinfo;
////	sys_local_date *p_sys_date;
////	sys_local_time *p_sys_time;
//
//	p_baseinfo = (RTU_baseinfo *)malloc(sizeof(RTU_baseinfo));
//	if(p_baseinfo == NULL)
//	{
//		return NULL;
//	}
//	p_baseinfo->well_station_type = WST_OIL_WELL;
//	p_baseinfo->dev_company = 0x4754;	// "GT"
//	p_baseinfo->type_version = 0x0001;
//	p_baseinfo->password	= 0xFFFF;
////	p_sys_time = getSysLocalTime();
////	p_baseinfo->day_of_start_time[0] = p_baseinfo->sys_time[0] = p_sys_time->local_hour;
////	p_baseinfo->day_of_start_time[1] = p_baseinfo->sys_time[1] = p_sys_time->local_minitue;
////	p_baseinfo->day_of_start_time[2] = p_baseinfo->sys_time[2] = p_sys_time->local_second;
////	free(p_sys_time);
////	p_sys_date = getSysLocalDate();
////	p_baseinfo->sys_date[0] = p_sys_date->local_year;
////	p_baseinfo->sys_date[1] = p_sys_date->local_month;
////	p_baseinfo->sys_date[2] = p_sys_date->local_day;
////	free(p_sys_date);
//
////	p_baseinfo->ram_vol = getRamVoltage();
//	p_baseinfo->ram_vol = 0.01 * 100;
////	p_baseinfo->cabinet_temperature = getCabinetTemperature();
//	p_baseinfo->cabinet_temperature = 30;
//
//	return p_baseinfo;
//}
///* @brief
// * 初始化communcnications_parameters
// */
//communicatios_parameters* commParametersInit(void)
//{
//	communicatios_parameters *p_commparam;
//	p_commparam = (communicatios_parameters *)malloc(sizeof(communicatios_parameters));
//	if(p_commparam == NULL)
//	{
//		return NULL;
//	}
//	p_commparam->communication_mode = CM_WIRELESS_BRIDGES;
//	p_commparam->communication_protocols = CP_MB_TCPIP;
//	p_commparam->terminal_comm_address = 0;
//	p_commparam->comm_baudrate = C_BAUD_57600;
//	p_commparam->comm_databit = C_DATA_BIT_8;
//	p_commparam->comm_stopbit = C_STOP_BIT_1;
//	p_commparam->comm_paritybit = C_ENEN;
//	p_commparam->comm_duplex = CE_FULL_DUPLEX;
//	p_commparam->ip_address[0] = 192;
//	p_commparam->ip_address[1] = 168;
//	p_commparam->ip_address[2] = 248;
//	p_commparam->ip_address[3] = 128;
//	p_commparam->subnet_mask[0] = 255;
//	p_commparam->subnet_mask[1] = 255;
//	p_commparam->subnet_mask[2] = 255;
//	p_commparam->subnet_mask[3] = 0;
//	p_commparam->gateway[0] = 192;
//	p_commparam->gateway[1] = 168;
//	p_commparam->gateway[2] = 248;
//	p_commparam->gateway[3] = 1;
//	p_commparam->mac_address[0] = 0xE8;
//	p_commparam->mac_address[1] = 0x2A;
//	p_commparam->mac_address[2] = 0xEA;
//	p_commparam->mac_address[3] = 0x09;
//	p_commparam->mac_address[4] = 0x79;
//	p_commparam->mac_address[5] = 0xAF;
//	p_commparam->tcp_udp_identity = TUI_UDP;
//	p_commparam->upd_port = 1503;
//	p_commparam->tcp_port = 1502;
//	p_commparam->master_ip_address[0] = 192;
//	p_commparam->master_ip_address[1] = 117;
//	p_commparam->master_ip_address[2] = 117;
//	p_commparam->master_ip_address[3] = 117;
//	p_commparam->master_port = 1506;
//	p_commparam->master_comm_mode = MCM_TCP_SERVER;
//	p_commparam->downlink_comm_interface = DCI_ZIGBEE;
//	p_commparam->downlink_recv_timeout = 5;
//	p_commparam->downlink_resend_num = 3;
//
//
//	return p_commparam;
//}

