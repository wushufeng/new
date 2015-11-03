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
extern oil_well *poilwell[ALLWELL];
instrument_timeout instimeout[63];

const uint16_t UT_BITS_ADDRESS = 0x13;
const uint16_t UT_BITS_NB = 0x25;
const uint8_t UT_BITS_TAB[] = { 0xCD, 0x6B, 0xB2, 0x0E, 0x1B };

const uint16_t UT_INPUT_BITS_ADDRESS = 0xC4;
const uint16_t UT_INPUT_BITS_NB = 0x16;
const uint8_t UT_INPUT_BITS_TAB[] = { 0xAC, 0xDB, 0x35 };

const uint16_t UT_REGISTERS_ADDRESS = 0x00;
/* Raise a manual exception when this adress is used for the first byte */
const uint16_t UT_REGISTERS_ADDRESS_SPECIAL = 0x6C;
const uint16_t UT_REGISTERS_NB = 10000;
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
static int kickInstrumentData();
static int syncDeviceInfo(int group);

int databaseThreadCancel(void)
{
	int res;
	void * thread_result;
	int n;

	// free动态得数据交换寄存器内存
	for(n = 0; n < ALLWELL; n ++)
	{
		if (pexbuffer[n] != NULL) {
			free(pexbuffer[n]);
		}
	}
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
		// 维护仪表信息表
		kickInstrumentData();
		// 删除数据库中的过期数据
		delOvertimeData();
		// 查找最老一条数据并更新A11中的时间
		searchOldestData();
		// TODO 判断是否有数据要更新MBbuffer  并将要保存得数据存入数据库
		for(n = 0; n < ALLWELL; n ++)
		{
			if(n == 3)
			{
				printf("dg_online = %d; dg_OK = %d; elec_Online = %d; elec_OK = %d \n",
						pexbuffer[n]->dg_online,
						pexbuffer[n]->dg_OK,
						pexbuffer[n]->elec_online,
						pexbuffer[n]->elec_OK);
			}
			if((pexbuffer[n]->elec_online == 0x3C) && (pexbuffer[n]->dg_online == 0x3C))
			{
				// 功图和电参都在线
				if((pexbuffer[n]->dg_OK == 0x3C) && (pexbuffer[n]->elec_OK == 0x3C))
				{
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
					pexbuffer[n]->dg_OK = 0x00;
					pexbuffer[n]->dg_online = 0x00;
					pexbuffer[n]->elec_OK = 0x00;
					pexbuffer[n]->elec_online = 0x00;
					pexbuffer[n]->dg_time = 0x00;
					bzero(&pexbuffer[n]->loaddata, sizeof(load_displacement));
				}
			}
			else if((pexbuffer[n]->dg_online == 0x3C) && (pexbuffer[n]->dg_OK == 0x3C))
			{
				syncDgData(n);
				zlog_info(c, "功图数据完成同步");
				databaseInsert(&pexbuffer[n]->loaddata, n, pexbuffer[n]->dg_time, 1,  0);
				zlog_info(c, "功图数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->dg_OK = 0x00;
				pexbuffer[n]->dg_online = 0x00;
				pexbuffer[n]->elec_OK = 0x00;
				pexbuffer[n]->elec_online = 0x00;
				pexbuffer[n]->dg_time = 0x00;
				bzero(&pexbuffer[n]->loaddata, sizeof(load_displacement));
			}
			else if((pexbuffer[n]->elec_online == 0x3C) && (pexbuffer[n]->elec_OK == 0x3C))
			{
				// 只有电参在线 并获得完整电参数据
				syncElecData(n);
				zlog_info(c, "电参数据完成同步");
				databaseInsert(&pexbuffer[n]->loaddata, n, pexbuffer[n]->dg_time, 0, 1);
				zlog_info(c, "电参数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->dg_OK = 0x00;
				pexbuffer[n]->dg_online = 0x00;
				pexbuffer[n]->elec_OK = 0x00;
				pexbuffer[n]->elec_online = 0x00;
				pexbuffer[n]->dg_time = 0x00;
				bzero(&pexbuffer[n]->loaddata, sizeof(load_displacement));
			}
			else
			{

			}
			if(pexbuffer[n]->elec2_OK == 0x3C)
			{
				// 只有电参在线 并获得完整电参数据
				syncElecData(n);
				zlog_info(c, "电参+数据完成同步");
				databaseInsert(&pexbuffer[n]->loaddata, n, 0, 0, 1);
				zlog_info(c, "电参+数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->elec2_OK = 0x00;
				pexbuffer[n]->elec2_online = 0x00;
				bzero(&pexbuffer[n]->loaddata.current, 500);
				bzero(&pexbuffer[n]->loaddata.power, 500);
			}
			if(pexbuffer[n]->oil_pressure == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "油压数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.oil_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "油压数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->oil_pressure = 0x00;
				bzero(&pexbuffer[n]->device_infor.oil_pressure, sizeof(device_base_information));
			}
			if(pexbuffer[n]->casing_pressure == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "套压数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.oil_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "套压数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->casing_pressure = 0x00;
				bzero(&pexbuffer[n]->device_infor.casing_pressure, sizeof(device_base_information));
			}
			if(pexbuffer[n]->back_pressure == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "回压数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.back_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "回压数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->back_pressure = 0x00;
				bzero(&pexbuffer[n]->device_infor.back_pressure, sizeof(device_base_information));
			}
			if(pexbuffer[n]->wellhead_oil_temp == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "井口油温数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.oil_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "井口油温数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->wellhead_oil_temp = 0x00;
				bzero(&pexbuffer[n]->device_infor.wellhead_oil_temp, sizeof(device_base_information));
			}
			if(pexbuffer[n]->load == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "载荷数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.oil_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "载荷数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->load = 0x00;
				bzero(&pexbuffer[n]->device_infor.load, sizeof(device_base_information));
			}
			if(pexbuffer[n]->displacement == 0x3C)
			{
				syncDeviceInfo(n);
				zlog_info(c, "位移数据完成同步");
				deviceInfoInsert(&pexbuffer[n]->device_infor.oil_pressure, n, pexbuffer[n]->device_time);
				zlog_info(c, "位移数据存入数据库");
				// 清零数据交换区
				pexbuffer[n]->device_time = 0;
				pexbuffer[n]->displacement = 0x00;
				bzero(&pexbuffer[n]->device_infor.displacement, sizeof(device_base_information));
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
/**
 * @brief
 * 维护大庆厂家定义的仪表信息
 */
static int kickInstrumentData()
{
	int n;
	time_t now_time = time(NULL);
	for(n = 0;n < 63;n ++)
	{
		if(instimeout[n].start_time != 0)
		{
			if((instimeout[n].start_time + instimeout[n].interval) < now_time)
			{
//				printf("kick info\n");
				bzero(&poilwell[0]->fuction_param.custom.instrument[n], sizeof(instrument_parameter));
				bzero(&instimeout[n].start_time, sizeof(instrument_timeout));
			}
		}
	}
	return 0;
}
/**
 * @brief
 * 同步仪器信息
 */
static int syncDeviceInfo(int group)
{
	device_base_information *ptr;
	device_base_information *ptr1;
	ptr = &poilwell[group]->fuction_param.device_infor.oil_pressure;
	ptr1 = &pexbuffer[group]->device_infor.oil_pressure;

	memcpy(ptr, ptr1, sizeof(device_base_information));
//	// 厂家代码
//	ptr->company_code = ptr1->company_code;
//	// 仪表类型
//	ptr->device_type = ptr1->device_type;
//	// 仪表组号
//	ptr->device_group = ptr1->device_group;
//	// 仪表编号
//	ptr->device_no = ptr1->device_no;
//	// 通信效率
//	ptr->comm_efficiency = ptr1->comm_efficiency;
//	// 电池电压
//	ptr->bat_vol = ptr1->bat_vol;
//	// 休眠时间
//	ptr->sleep_time = ptr1->sleep_time;
//	// 仪表状态
//	ptr->device_sta = ptr1->device_sta;
//	// 工作温度
//	ptr->work_temp[0] = ptr1->work_temp[0];
//	ptr->work_temp[1] = ptr1->work_temp[1];
//	// 实时数据
//	ptr->realtime_data[0] = ptr1->realtime_data[0] ;
//	ptr->realtime_data[1] = ptr1->realtime_data[1] ;
//	zlog_info(c, "油压数据完成同步");
	return 0;
}
