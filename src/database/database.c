/*
 * database.c
 *
 *  Created on: 2015年3月26日
 *      Author: wsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sqlite3.h>

#include "database.h"
//#include "../A11_sysAttr/a11sysattr.h"
#include "../sever/sysdatetime/getsysdatetime.h"
#include "../A11_sysAttr/a11sysattr.h"
#include "../inifile/inifile.h"
#include "../managerDB/ManagerDB.h"
#include "../sever/sysdatetime/getsysdatetime.h"
#include "../log/rtulog.h"
#define ALLWELL	17
exchangebuffer *pexbuffer[ALLWELL];											// 存放功图电参的临时buffer
extern oil_well *poilwell[17];


const uint16_t UT_BITS_ADDRESS = 0x13;
const uint16_t UT_BITS_NB = 0x25;
const uint8_t UT_BITS_TAB[] = { 0xCD, 0x6B, 0xB2, 0x0E, 0x1B };

const uint16_t UT_INPUT_BITS_ADDRESS = 0xC4;
const uint16_t UT_INPUT_BITS_NB = 0x16;
const uint8_t UT_INPUT_BITS_TAB[] = { 0xAC, 0xDB, 0x35 };

const uint16_t UT_REGISTERS_ADDRESS = 0x00;
/* Raise a manual exception when this adress is used for the first byte */
const uint16_t UT_REGISTERS_ADDRESS_SPECIAL = 0x6C;
const uint16_t UT_REGISTERS_NB = 20000;
const uint16_t UT_REGISTERS_TAB[] = { 0x022B, 0x0001, 0x0064 };
/* If the following value is used, a bad response is sent.
   It's better to test with a lower value than
   UT_REGISTERS_NB_POINTS to try to raise a segfault. */
const uint16_t UT_REGISTERS_NB_SPECIAL = 0x2;

//const uint16_t UT_INPUT_REGISTERS_ADDRESS = 0x08;
//const uint16_t UT_INPUT_REGISTERS_NB = 0x1;
const uint16_t UT_INPUT_REGISTERS_ADDRESS = 0x00;
const uint16_t UT_INPUT_REGISTERS_NB = 0xD;
const uint16_t UT_INPUT_REGISTERS_TAB[] = { 0x000A };

const float UT_REAL = 916.540649;
const uint32_t UT_IREAL = 0x4465229a;

modbus_mapping_t *mb_mapping[ALLWELL];
pthread_t database_hread;
extern E1_sys_attribute *psysattr;

//extern pthread_mutex_t holdingReg_mutex;

static int databaseThreadFunc(void *arg);
static int syncElecData(int group);
static int syncDgData(int group);

int databaseThreadCancel(void)
{
	int res;
	void * thread_result;

	int kill_rc = pthread_kill(database_hread,0);		// 使用pthread_kill函数发送信号0判断线程是否还在
	zlog_info(c, "正在取消数据同步线程...");
	if(kill_rc == ESRCH)					// 线程不存在：ESRCH
		zlog_warn(c, "数据同步线程不存在或者已经退出");
	else if(kill_rc == EINVAL)		// 信号不合法：EINVAL
		zlog_warn(c, "非法信号");
	else
	{
		res = pthread_cancel(database_hread);
		if(res != 0)	{
			zlog_error(c, "取消数据同步线程失败-%d", res);
			exit(EXIT_FAILURE);
		}
	}

	zlog_info(c, "正在等待数据同步线程结束...");
	res = pthread_join(database_hread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待数据同步线程结束失败-%d", res);
		exit(EXIT_FAILURE);
	}
	return 0;
}


static int databaseThreadFunc(void *arg)
{
	int n;
	int res;
	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "数据同步线程pthread_setcancelstate失败-%d", res);
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "数据同步线程pthread_setcanceltype失败-%d", res);
		exit(EXIT_FAILURE);
	}
	for(;;)
	{
		// 读取当前系统时间
		if(getSysLocalDateTime(arg) != 0)
		{
//			printf("[提示]系统时间读取有误!\n");
			zlog_error(c, "系统时间读取有误");
			return -1;
		}

		// TODO 判断是否有数据要更新MBbuffer
		for(n = 0; n < ALLWELL; n ++)
		{
			if((pexbuffer[n]->elec_online == 0x3C) && (pexbuffer[n]->dg_online == 0x3C))
			{
				// 功图和电参都在线
				if((pexbuffer[n]->dg_OK == 0x3C) && (pexbuffer[n]->elec_OK == 0x3C))
				{
					// 功图和电参都测试完毕
//					pexbuffer[n]->dg_OK = 0x00;
//					pexbuffer[n]->dg_online = 0x00;
//					pexbuffer[n]->elec_OK = 0x00;
//					pexbuffer[n]->elec_online = 0x00;
					// 更新功图
					syncDgData(n);
					// 更新电参
					syncElecData(n);
					// 更新测试时间
					getDynagraphDateTime((void *)poilwell[n], pexbuffer[n]->dg_time);
					zlog_info(c, "功图和电参数据完成同步");
					//TODO 同步后将该数据转入数据库中
					databaseInsert(&pexbuffer[n]->loaddata, n, pexbuffer[n]->dg_time, 1 , 1);
					zlog_info(c, "功图和电参数据存入数据库");
					// 清零数据交换区
					bzero(pexbuffer[n], sizeof(exchangebuffer));
				}
			}
			else if((pexbuffer[n]->dg_online == 0x3C) && (pexbuffer[n]->dg_OK == 0x3C))
			{
				// 只有功图在线 并获得完整功图数据
//				pexbuffer[n]->dg_OK = 0x00;
//				pexbuffer[n]->dg_online = 0x00;
//				pexbuffer[n]->elec_OK = 0x00;
//				pexbuffer[n]->elec_online = 0x00;
				syncDgData(n);
				zlog_info(c, "功图数据完成同步");
				databaseInsert(&pexbuffer[n]->loaddata, n, pexbuffer[n]->dg_time, 1,  0);
				zlog_info(c, "功图数据存入数据库");
				// 清零数据交换区
				bzero(pexbuffer[n], sizeof(exchangebuffer));
			}
			else if((pexbuffer[n]->elec_online == 0x3C) && (pexbuffer[n]->elec_OK == 0x3C))
			{
				// 只有电参在线 并获得完整电参数据
//				pexbuffer[n]->dg_OK = 0x00;
//				pexbuffer[n]->dg_online = 0x00;
//				pexbuffer[n]->elec_OK = 0x00;
//				pexbuffer[n]->elec_online = 0x00;
				syncElecData(n);
				zlog_info(c, "电参数据完成同步");
				databaseInsert(&pexbuffer[n]->loaddata, n, pexbuffer[n]->dg_time, 0, 1);
				zlog_info(c, "电参数据存入数据库");
				// 清零数据交换区
				bzero(pexbuffer[n], sizeof(exchangebuffer));
			}
			else
			{

			}
		}

		sleep(5);
	}
	pthread_exit((void *)0);
//	return 0;
}
int createDatabaseThread(void)
{
	int res;
	int n;
	// 创建数据表
	createSqlTable();
	for(n = 0; n < ALLWELL; n ++)
	{
		pexbuffer[n] = (exchangebuffer *)malloc(sizeof(exchangebuffer));
		if (pexbuffer[n] == NULL) {
			zlog_error(c, "申请数据交换空间%d失败", n + 1);
			return -1;
		}
		bzero(pexbuffer[n], sizeof(exchangebuffer));
	}
	res = pthread_create(&database_hread, NULL, (void *)&databaseThreadFunc, (void *)psysattr);
    if(res != 0)
    {
        zlog_error(c, "创建数据同步线程失败-%d", res);
        return (EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}
int mbMappingNew(void)
{
	int i;

	for(i = 0; i < 17; i ++)
	{
		mb_mapping[i] = modbus_mapping_new(
			UT_BITS_ADDRESS + UT_BITS_NB,
			UT_INPUT_BITS_ADDRESS + UT_INPUT_BITS_NB,
			UT_REGISTERS_ADDRESS + UT_REGISTERS_NB,
			UT_INPUT_REGISTERS_ADDRESS + UT_INPUT_REGISTERS_NB);
		if (mb_mapping[i] == NULL) {
//			fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
			zlog_error(c, "分配数据交换动态内存失败: %s", modbus_strerror(errno));
	//        modbus_free(ctx);
			return -1;
	//        return NULL;
		}
	}
    /* Examples from PI_MODBUS_300.pdf.
       Only the read-only input values are assigned. */

    /** INPUT STATUS **/
    modbus_set_bits_from_bytes(mb_mapping[0]->tab_input_bits,
                               UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
                               UT_INPUT_BITS_TAB);

    /** INPUT REGISTERS **/
    /*
    a5d3ad_get_ad(mb_mapping->tab_input_registers,
    		UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);
*/

    for (i=0; i < UT_INPUT_REGISTERS_NB; i++) {
        mb_mapping[0]->tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+i] =
            UT_INPUT_REGISTERS_TAB[i];
    }
    /*@brief_wsf
     * 将RTU基本信息传入holding_registers
     */
//	    mb_mapping->tab_registers = &(p_base_info->well_station_type);
//    for(i = 0; i < UT_REGISTERS_NB; i ++)
//    {
//    	mb_mapping->tab_registers[UT_REGISTERS_ADDRESS + i] =
//    			i ;
//    }
    return 0;
//    return mb_mapping;
}

/* @brief
 * wsf
 * 通过0x06功能码修改配置文件
 */
int mbWriteSigleRegister(uint16_t address, int data)
{
	char str[32];
//	static char data_flag_ip = 0;
	switch(address)
	{
		case 0:		// 应用得井站类型
			sprintf(str,"%03d",psysattr->baseinfo.well_station_type);
			if (!write_profile_string("rtuBaseInfo","well_station_type",str,A11filename))
				return -1;
			break;
		case 1:		// 设备厂家
			sprintf(str,"%d",psysattr->baseinfo.dev_company);
			if (!write_profile_string("rtuBaseInfo","dev_company",str,A11filename))
				return -1;
			break;
		case 2:		// 型号版本
			sprintf(str,"%d",psysattr->baseinfo.type_version);
			if (!write_profile_string("rtuBaseInfo","type_version",str,A11filename))
				return -1;
			break;
		case 3:		// 口令
			sprintf(str,"%d",psysattr->baseinfo.password);
			if (!write_profile_string("rtuBaseInfo","password",str,A11filename))
				return -1;
			break;
		case 6:		// 系统时间
		case 7:
		case 8:

			break;
		case 9:		// 系统日期
		case 10:
		case 11:

			break;
		case 12:		// 日时间起点
		case 13:
		case 14:

			break;
		case 30:		// 通信方式
		{
			psysattr->commparam.communication_mode = data;
			sprintf(str,"%d",psysattr->commparam.communication_mode);
			if (!write_profile_string("commParam","communication_mode",str,A11filename))
				return -1;
			break;
		}
		case 31:		// 通讯协议
			psysattr->commparam.communication_protocols = data;
			sprintf(str,"%d",psysattr->commparam.communication_protocols);
			if (!write_profile_string("commParam","communication_protocols",str,A11filename))
				return -1;
			break;
		case 32:		// 终端通信地址
			psysattr->commparam.terminal_comm_address = data;
			sprintf(str,"%d",psysattr->commparam.terminal_comm_address);
			if (!write_profile_string("commParam","terminal_comm_address",str,A11filename))
				return -1;
			break;
		case 33:		// 波特率
			psysattr->commparam.comm_baudrate = data;
			sprintf(str,"%d",psysattr->commparam.comm_baudrate);
			if (!write_profile_string("commParam","comm_baudrate",str,A11filename))
				return -1;
			break;
		case 34:		// 数据位
			psysattr->commparam.comm_databit = data;
			sprintf(str,"%d",psysattr->commparam.comm_databit);
			if (!write_profile_string("commParam","comm_databit",str,A11filename))
				return -1;
			break;
		case 35:		// 停止位
			psysattr->commparam.comm_stopbit = data;
			sprintf(str,"%d",psysattr->commparam.comm_stopbit);
			if (!write_profile_string("commParam","comm_stopbit",str,A11filename))
				return -1;
			break;
		case 36:		//奇偶校验
			psysattr->commparam.comm_paritybit = data;
			sprintf(str,"%d",psysattr->commparam.comm_paritybit);
			if (!write_profile_string("commParam","comm_paritybit",str,A11filename))
				return -1;
			break;
		case 37:		// 半/全双工
			psysattr->commparam.comm_duplex = data;
			sprintf(str,"%d",psysattr->commparam.comm_duplex);
			if (!write_profile_string("commParam","comm_duplex",str,A11filename))
				return -1;
			break;
		case 38:		// 本地IP地址
		case 39:
		case 40:
		case 41:
//			psysattr->commparam.ip_address[0] = d?ata;
//			data_flag_ip |= 0x01;
//			if(data_flag_ip == 0x0F)
//			{
//				data_flag_ip = 0x00;
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
					psysattr->commparam.ip_address[1],
					psysattr->commparam.ip_address[2],
					psysattr->commparam.ip_address[3]);
			if (!write_profile_string("commParam","ip_address",str,A11filename))
				return -1;
//			}
			break;
		case 42:		// 子网掩码
		case 43:
		case 44:
		case 45:
//			psysattr->commparam.subnet_mask[0] = data;
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.subnet_mask[0],
						psysattr->commparam.subnet_mask[1],
						psysattr->commparam.subnet_mask[2],
						psysattr->commparam.subnet_mask[3]);
				if (!write_profile_string("commParam","subnet_mask",str,A11filename))
					return -1;
			break;
		case 46:		// 网关
		case 47:
		case 48:
		case 49:
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.gateway[0],
						psysattr->commparam.gateway[1],
						psysattr->commparam.gateway[2],
						psysattr->commparam.gateway[3]);
				if (!write_profile_string("commParam","gateway",str,A11filename))
					return -1;
			break;
		case 50:		// MAC地址
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
			sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",psysattr->commparam.mac_address[0],
					psysattr->commparam.mac_address[1],
					psysattr->commparam.mac_address[2],
					psysattr->commparam.mac_address[3],
					psysattr->commparam.mac_address[4],
					psysattr->commparam.mac_address[5]);
			if (!write_profile_string("commParam","mac_address",str,A11filename))
				return -1;
			break;
		case 56:		// TCP/UDP标识
			sprintf(str,"%d",psysattr->commparam.tcp_udp_identity);
			if (!write_profile_string("commParam","tcp_udp_identity",str,A11filename))
				return -1;
			break;
		case 57:		// 本地UDP端口号
			sprintf(str,"%d",psysattr->commparam.upd_port);
			if (!write_profile_string("commParam","upd_port",str,A11filename))
				return -1;
			break;
		case 58:		// 本地TCP端口号
			sprintf(str,"%d",psysattr->commparam.tcp_port);
			if (!write_profile_string("commParam","tcp_port",str,A11filename))
				return -1;
			break;
		case 59:		// 主站IP地址
		case 60:
		case 61:
		case 62:
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.master_ip_address[0],
						psysattr->commparam.master_ip_address[1],
						psysattr->commparam.master_ip_address[2],
						psysattr->commparam.master_ip_address[3]);
			if (!write_profile_string("commParam","master_ip_address",str,A11filename))
				return -1;
			break;
		case 63:		// 主站端口号
			psysattr->commparam.master_port = data;
			sprintf(str,"%d",psysattr->commparam.master_port);
			if (!write_profile_string("commParam","master_port",str,A11filename))
				return -1;
			break;
		case 64:		// 主站通讯方式
			psysattr->commparam.master_comm_mode = data;
			sprintf(str,"%d",psysattr->commparam.master_comm_mode);
			if (!write_profile_string("commParam","master_comm_mode",str,A11filename))
				return -1;
			break;
		case 65:		// 下行通信接口
			psysattr->commparam.downlink_comm_interface = data;
			sprintf(str,"%d",psysattr->commparam.downlink_comm_interface);
			if (!write_profile_string("commParam","downlink_comm_interface",str,A11filename))
				return -1;
			break;
		case 66:		// 下行通信接收超时时间
			psysattr->commparam.downlink_recv_timeout = data;
			sprintf(str,"%d",psysattr->commparam.downlink_recv_timeout);
			if (!write_profile_string("commParam","downlink_recv_timeout",str,A11filename))
				return -1;
			break;
		case 67:		// 下行通讯失败重发次数
			psysattr->commparam.downlink_resend_num = data;
			sprintf(str,"%d",psysattr->commparam.downlink_resend_num);
			if (!write_profile_string("commParam","downlink_resend_num",str,A11filename))
				return -1;
			break;

		default:
			break;
	}

	return 0;
}
/*
 * 同步功图电参数据
 */
static int syncElecData(int group)
{
	memcpy(&poilwell[group]->load_displacement.dynagraph.current, &pexbuffer[group]->loaddata.current, 250);
	memcpy(&poilwell[group]->load_displacement.dynagraph.power, &pexbuffer[group]->loaddata.power, 250);
	return 0;
}
/*
 * 同步功图数据
 */
static int syncDgData(int group)
{
	poilwell[group]->load_displacement.dynagraph.actual_dot = pexbuffer[group]->loaddata.actual_dot;
	memcpy(&poilwell[group]->load_displacement.dynagraph.pumping_speed, &pexbuffer[group]->loaddata.pumping_speed, 4);
	memcpy(&poilwell[group]->load_displacement.dynagraph.pumping_stroke, &pexbuffer[group]->loaddata.pumping_stroke, 4);
	memcpy(&poilwell[group]->load_displacement.dynagraph.load, &pexbuffer[group]->loaddata.load, 250);
	memcpy(&poilwell[group]->load_displacement.dynagraph.displacement, &pexbuffer[group]->loaddata.displacement, 250);
	return 0;
}
