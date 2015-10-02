/*
 * zigbee.c
 *
 *  Created on: Oct 2, 2015
 *      Author: wsf
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <string.h>
//#include <typeinfo>
#include <arpa/inet.h>
#include <time.h>

#include "SerialZigbee.h"

#include "../../def.h"
#include "../../myMB/myMB.h"
#include "../../log/rtulog.h"
#include "../../port/portserial.h"
#include "../../myfunction/myfun.h"
#include "../../database/database.h"
#include "../../A11_sysAttr/a11sysattr.h"
#include "../sysdatetime/getsysdatetime.h"

#define BUFFER_SIZE		1024
#ifdef ARM_32
#define 	ZBDEVICE			"/dev/ttyS3"
#else
#define 	ZBDEVICE			"/dev/ttyUSB0"
#endif

comm_t *zb_fd;
pthread_t zb_thread;

uint8_t *zb_rev_buf = NULL;		// zigbee接收数据buffer
uint8_t *zb_snd_buf = NULL;		// zigbee发送数据buffer

int zbONOFF = 0;

extern oil_well *poilwell[17];
extern exchangebuffer *pexbuffer[17];											// 存放功图电参的临时buffer
extern instrument_timeout instimeout[63];
ZB_explicit_RX_indicator *ZB_91_normal;

unsigned char instrument_group;									// 定义全局仪器组号，用次判断数据存放位置
unsigned short int dg_dot;
unsigned char dg_group;
unsigned char dg_num;
unsigned char dg_remainder;
unsigned short int elec_dot;
unsigned char elec_group;
unsigned char elec_num;
unsigned char elec_remainder;
data_exchange data_ex[17] = {{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00},
		{0x10, 0x00, 0xF79C, 0x0000, 0x0000, 0x00C8, 0x04CC, 0x00, 0x00, 0x00}};

static uint8_t zbCheck(uint8_t *buf, uint16_t start, uint16_t cnt);
static int zbCheckIntegrity(comm_t *ctx, uint8_t *msg, const int msg_length);
static int zbReply(comm_t *ctx,  uint8_t *req, int req_length,uint8_t *snd);
static int conventionalDataRespone(unsigned short int sleeptime);
static int collectDynagraphRespone(data_exchange *datex);
static int collectElecRespone(data_exchange *datex);
static int dataGroupRespone(unsigned short int data_type);
static int dataGroupResponeElec(unsigned short int data_type);
static int readElecRespone(data_exchange *datex);
static int ZBnetinResponse();
static int updateInstrument();
/**
 * @breif
 * zigbee线程实例
 */
static int zbThreadFunc(void *arg)
{
	int res;
	int data_len;
	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "ZigBee线程pthread_setcancelstate失败");
		pthread_exit("1");
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "ZigBee线程pthread_setcanceltype失败");
		pthread_exit("2");
	}

	res = comm_connect(zb_fd);
    if (res == -1) {
        zlog_error(c, "ZigBee接口建立连接失败! %s", strerror(errno));
        comm_free(zb_fd);
        pthread_exit("3");
    }
    else
    	zbONOFF = 1;
    int rc = tcflush(zb_fd->s, TCIOFLUSH);
    if (rc != -1 && zb_fd->debug) {
        printf("%d bytes flushed\n", rc);
    }

    for(;;)
    {
    	res = mbRead(zb_fd, zb_rev_buf, 0, 3);
    	if(res > 0) {
     		data_len = res;
     		if(zbCheckIntegrity(zb_fd, zb_rev_buf, data_len) == -1)
     			continue;
    	}
    	else {
    		zlog_warn(c, "ZigBee接收数据错误 = %d:%s", res, strerror(errno));
    		continue;
    	}
    	// 显示接收到得数据
        if (zb_fd->debug) {
            int i;
            if(data_len)
            	printf(">> ");
            for (i=0; i < data_len; i++)
            	printf("%.2X ", zb_rev_buf[i]);
            if(data_len)
            	printf("\n");
        }

        res = zbReply(zb_fd, zb_rev_buf, rc, zb_snd_buf);
        if (res == -1) {
            /* Connection closed by the client or error */
        	zlog_warn(c, "ZigBee发送数据错误 = %d", res);
        	continue;
        }
    }
	pthread_exit(0);
}
/**
 * @brief
 * zigbee初始化
 */
int zbInit(void *obj)
{
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	int baud;
	char parity;
	int data_bit;
	int stop_bit;
//	use_backend_zigbee = ZIGBEE;
	switch (psysattr->commparam.comm_baudrate)
	{
		case C_BAUD_1200:
			baud = 1200;
			break;
		case C_BAUD_2400:
			baud = 2400;
			break;
		case C_BAUD_4800:
			baud = 4800;
			break;
		case C_BAUD_9600:
			baud = 9600;
			break;
		case C_BAUD_19200:
			baud = 19200;
			break;
		case C_BAUD_38400:
			baud = 38400;
			break;
		case C_BAUD_57600:
			baud = 57600;
			break;
		case C_BAUD_OTHER:
			baud = 115200;
			break;
		default:
			baud = 57600;
			break;
	}
	switch (psysattr->commparam.comm_paritybit)
	{
		case C_NONE:
			parity = 'N';
			break;
		case C_ENEN:
			parity = 'E';
			break;
		case C_ODD:
			parity = 'O';
			break;
		default:
			parity = 'N';
			break;
	}
	switch (psysattr->commparam.comm_databit)
	{
		case C_DATA_BIT_7:
			data_bit = 7;
			break;
		case C_DATA_BIT_8:
			data_bit = 8;
			break;
		default:
			data_bit = 8;
			break;
	}
	switch (psysattr->commparam.comm_stopbit)
	{
		case C_STOP_BIT_1:
			stop_bit = 1;
			break;
		case C_STOP_BIT_2:
			stop_bit = 2;
			break;
		default:
			stop_bit = 'N';
			break;
	}
	zb_fd = new_comm_t(ZBDEVICE, baud, parity, data_bit, stop_bit);
	if (zb_fd == NULL)
	{
		zlog_error(c, "不能动态分配ZigBee环境");
		return -1;
	}
	comm_set_debug(zb_fd, 0);
   	comm_set_slave(zb_fd, SERVER_ID);
   	zb_fd->error_recovery = MB_ERROR_RECOVERY_LINK | MB_ERROR_RECOVERY_PROTOCOL;
	// 分配用于接收和发送数据的动态内存
	zb_rev_buf = (uint8_t *)malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (zb_rev_buf == NULL) {
    	zlog_error(c, "为GPRS分配接收动态内存失败:,%d", errno);
        return -1;
    }
	zb_snd_buf = (uint8_t *)malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (zb_snd_buf == NULL) {
    	zlog_error(c, "为GPRS分配接收动态内存失败:,%d", errno);
        return -1;
    }
    bzero(zb_rev_buf,(BUFFER_SIZE * sizeof(uint8_t)));
    bzero(zb_snd_buf,(BUFFER_SIZE * sizeof(uint8_t)));
	return 0;
}
/**
 * @brief
 * 创建zigbee线程
 */
int createZbThread(void)
{
	int res;
	res = pthread_create(&zb_thread, NULL, (void*)&zbThreadFunc, NULL);
    if(res != 0)
    {
    	zlog_error(c, "创建SerialZigBee线程失败:%s", modbus_strerror(errno));
        return (-1);
    }
	return 0;
}
/**
 * @breif
 * 取消zigbee线程
 */
int cancelZbThread(void)
{
	int res;
	void * thread_result;

	int kill_rc = pthread_kill(zb_thread,0);		// 使用pthread_kill函数发送信号0判断线程是否还在
	zlog_info(c, "正在取消SerialZigBee线程...");
	if(kill_rc == ESRCH)					// 线程不存在：ESRCH
		zlog_warn(c, "SerialZigBee线程不存在或者已经退出");
	else if(kill_rc == EINVAL)		// 信号不合法：EINVAL
		zlog_warn(c, "非法信号");
	else
	{
		res = pthread_cancel(zb_thread);
		if(res != 0)	{
			zlog_error(c, "取消SerialZigBee线程失败-%d", res);
			return -1;
		}
	}
	zlog_info(c, "正在等待SerialZigBee线程结束...");
	res = pthread_join(zb_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待SerialZigBee线程结束失败-%d", res);
		return -1;
	}
	return 0;
}
/**
 * @breif
 * zigbee所用内存释放
 */
int  zbFree()
{
	if(zbONOFF)
		comm_close(zb_fd);

	if(zb_fd != NULL)
		comm_free(zb_fd);

	if(zb_rev_buf != NULL)
	{
		free(zb_rev_buf);
		zb_rev_buf = NULL;
	}
	if(zb_snd_buf != NULL)
	{
		free(zb_snd_buf);
		zb_snd_buf = NULL;
	}
	return 0;
}
static uint8_t zbCheck(uint8_t *buf, uint16_t start, uint16_t cnt)
{
	uint16_t i;
	uint8_t sum = 0;
	uint8_t temp;
   for(i=start; i<cnt; i++)
   {
      sum = sum+buf[i];
   }
   temp = ((0xff - sum)&0xff);
   return(temp);
}
static int zbCheckIntegrity(comm_t *ctx, uint8_t *msg, const int msg_length)
{
	uint8_t check_calculated;
	uint8_t check_received;

    check_calculated = zbCheck(msg, 3, msg_length - 1);
    check_received = msg[msg_length - 1];

    /* Check CRC of msg */
    if (check_calculated == check_received) {
        return msg_length;
    } else {
        if (ctx->debug) {
        	zlog_warn(c, "CRC received %0X != CRC calculated %0X",
            		check_received, check_calculated);
        }
        if (ctx->error_recovery & MB_ERROR_RECOVERY_LINK) {
//            _modbus_rtu_flush(ctx);
        	tcflush(ctx->s, TCIOFLUSH);
        }
        errno = EMBBADCRC;
        return -1;
    }
}
static int zbReply(comm_t *ctx,  uint8_t *req, int req_length, uint8_t *snd)
{

	unsigned char  ZB_state;
	unsigned char API_Check;
	int rsp_length = 0;
	int res;
	unsigned char n;

	ZB_91_normal = (ZB_explicit_RX_indicator *)req;
	switch (ZB_91_normal->ZB91_framehead.farme_type)
	{
	case 0x8A:
		ZB_state = req[4];
		switch(ZB_state)																// 00 复位 01 看门狗复位 02 连接网络 03 脱网 06 协调器启动
		{
			case 0x00:
				// zigbee模块上电复位

				zlog_info(c, "ZigBee模块上电复位");
				break;
			case 0x06:
				// zigbee模块协调器启动成功
				zlog_info(c, "ZigBee模块协调器启动成功");
				break;
			default:
				break;
		}
		break;
	case 0x8B:																				// 发出API帧数据时的MAC层ACK反馈
	{
		API_Check = req[8];
		switch(API_Check)																// 发送数据成功
		{
			case 0x00:
				break;
			default:
				break;
		}
		break;
	}
	case 0xA1:																				// 路由报表与NO值有关
	{
		break;
	}
	case 0x88:																				// AT命令返回
	{
		if((req[5] == 0x4E) && (req[6] == 0x43))
		{
			break;
		}
		break;
	}
	case 0x91:
		switch(htons(ZB_91_normal->ZB91_framehead.cluster_ID))
		{
			case 0x0013: // 上电
			{
				ZB91_poweron_framehead * zb_poweron;
				zb_poweron = (ZB91_poweron_framehead *)req;
				zb_poweron->ZB91_framehead.mac_addr[0]= 0;
				zlog_info(c, "ZigBee模块MAC = %.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X上电", \
						zb_poweron->ZB91_framehead.mac_addr[0], \
						zb_poweron->ZB91_framehead.mac_addr[1], \
						zb_poweron->ZB91_framehead.mac_addr[2], \
						zb_poweron->ZB91_framehead.mac_addr[3], \
						zb_poweron->ZB91_framehead.mac_addr[4], \
						zb_poweron->ZB91_framehead.mac_addr[5], \
						zb_poweron->ZB91_framehead.mac_addr[6], \
						zb_poweron->ZB91_framehead.mac_addr[7]);
				break;
			}
			case 0x0095: // 入网帧
			{
				ZB91_netin_framehead * zb_netin;
				zb_netin = (ZB91_netin_framehead *)req;
				zb_netin->ZB91_framehead.mac_addr[0]= 0;
				zlog_info(c, "ZigBee模块MAC = %.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X入网成功", \
						zb_netin->ZB91_framehead.mac_addr[0], \
						zb_netin->ZB91_framehead.mac_addr[1], \
						zb_netin->ZB91_framehead.mac_addr[2], \
						zb_netin->ZB91_framehead.mac_addr[3], \
						zb_netin->ZB91_framehead.mac_addr[4], \
						zb_netin->ZB91_framehead.mac_addr[5], \
						zb_netin->ZB91_framehead.mac_addr[6], \
						zb_netin->ZB91_framehead.mac_addr[7]);
				rsp_length = ZBnetinResponse();
//				res = comm_send(ctx, snd, rsp_length);
//				if(res == -1)
//				{
//					printf("[提示]发送请求路由表命令失败!\n");
//					break;
//				}
//				memcpy(snd, AT_ND, sizeof(AT_ND));
//				rsp_length = sizeof(AT_ND);
				break;
			}
			case 0x0011:	// 油田A11
			{
				if(ZB_91_normal->ZB91_framehead.source_endpoint != 0xE8 ||
						ZB_91_normal->ZB91_framehead.destination_endpoint != 0xE8)
				{
					zlog_info(c, "源端 = %d, 目的端 =  %d", ZB_91_normal->ZB91_framehead.source_endpoint, ZB_91_normal->ZB91_framehead.destination_endpoint);
					break;
				}
			// 判断簇ID是否是0x00 0x11
				if(htons(ZB_91_normal->ZB91_framehead.cluster_ID) != 0x0011)
				{
					zlog_info(c, "簇ID = %d,不等于0x0011(17)", htons(ZB_91_normal->ZB91_framehead.cluster_ID));
					break;
				}
			// 判断规范ID是否是A11规定得0x1857传输时使用的 profile ID（建议用中国石油股票代码的后 4 位）
				if(htons(ZB_91_normal->ZB91_framehead.profile_ID) != 0x1857)
				{
					zlog_info(c, "规范ID = %d,不等于0x1857(6231)", htons(ZB_91_normal->ZB91_framehead.cluster_ID));
					break;
				}
			// 读出收到数据得仪器对应的仪器组号
				instrument_group = ZB_91_normal->A11_framehead.instrument_group;
				// 更新仪表信息寄存器 49100~49227
				updateInstrument();

			// 判断仪表类型
				switch(htons(ZB_91_normal->A11_framehead.instrument_type))
				{
					case 0x0001:											// 无线一体化载荷位移
					{

						time_t now_time = time(NULL);							// 读出当前时间秒数
						static time_t last_time[17] = {0};
						// 功图巡检时间
						poilwell[instrument_group]->load_displacement.dynagraph.interval = 	\
								(short int)(poilwell[0]->fuction_param.custom.dynagraph_patroltime / 60);
						// 功图测试点数
						poilwell[instrument_group]->load_displacement.dynagraph.set_dot = \
								poilwell[0]->fuction_param.custom.dynagraph_dot;

						A11_req_revdata_dynagraph * dynagraph;
						dynagraph = (A11_req_revdata_dynagraph *)req;

						switch(htons(dynagraph->A11_framehead.data_type))
						{
							case 0x0000:				//	上传的常规数据
								// 说明功图在线
								// 判断是否发送功图采集命令
								if(((last_time[instrument_group] == 0) || (poilwell[instrument_group]->load_displacement.dynagraph.manul_collection_order != 0) \
										|| (now_time > (last_time[instrument_group] + poilwell[instrument_group]->load_displacement.dynagraph.interval * 60))) \
												&& (pexbuffer[instrument_group]->dg_online == 0x3C))
								{
									// 取出测试间隔
									pexbuffer[instrument_group]->loaddata.interval = poilwell[instrument_group]->load_displacement.dynagraph.interval;
									// 取出预设得测试点数
									pexbuffer[instrument_group]->loaddata.set_dot = poilwell[instrument_group]->load_displacement.dynagraph.set_dot;
									// 清零手动采集标识
									poilwell[instrument_group]->load_displacement.dynagraph.manul_collection_order = 0;
									last_time[instrument_group] = now_time;
									// 发送功图采集命令
//									usleep(100000);				// 发送采集命令前得延时
									zlog_info(c, "发送功图[组=%d]采集命令", instrument_group);
									rsp_length = collectDynagraphRespone(&data_ex[instrument_group]);

									// 判断对应组号得电参是否在线，是则，发送电参采集命令
									if(pexbuffer[instrument_group]->elec_online == 0x3C)
									{
										res = comm_send(ctx, snd, rsp_length);
										if(res == -1)
										{
											zlog_info(c, "提示]发送一体化功图采[组=%d]集命令失败!", instrument_group);
											break;
										}
										zlog_info(c, "发送电参[组=%d]采集命令", instrument_group);
										rsp_length = collectElecRespone(&data_ex[instrument_group]);
									}
								}
								else
								{
									pexbuffer[instrument_group]->dg_online = 0x3C;
									zlog_info(c, "接收到一体化功图[组=%d]常规数据帧", instrument_group);
									// 常规数据应答
									rsp_length = conventionalDataRespone(0x003C);

								}
								break;
							case 0x0010:				// 上传的常规参数

								break;
							case 0x0020:				// 上传的功图数据包
							{
								float tempdata = 0;
								A11_req_dynagraph_first *dynagraph_data;
								dynagraph_data = (A11_req_dynagraph_first *)req;
								if(dynagraph_data->data_serialnum[0] == 0)									// 说明该数据为第一组数据
								{
									dg_dot = htons(dynagraph_data->dot);
									if(dg_dot <=  250)
									{
										dg_group = (unsigned char)(dg_dot / 15);
										dg_remainder = (unsigned char)(dg_dot %15);
									}
									else
									{
										zlog_warn(c, "一体化功图[组=%d]采集总点数[%d] > 250",  instrument_group, dg_dot);
										dg_group = 0;
										dg_remainder = 0;
										break;
									}
									data_ex[instrument_group].cycle = htons(dynagraph_data->cycle);
									data_ex[instrument_group].time_mark = htons(dynagraph_data->time_mark);
									data_ex[instrument_group].dot = htons(dynagraph_data->dot);

									pexbuffer[instrument_group]->loaddata.actual_dot = htons(dynagraph_data->dot);
									tempdata  = (6000.0 / htons(dynagraph_data->cycle));																		// 冲次 10ms
									memcpy(&pexbuffer[instrument_group]->loaddata.pumping_speed, &tempdata, 4);
									tempdata = (htons(dynagraph_data->stroke) / 1000.0);
									memcpy(&pexbuffer[instrument_group]->loaddata.pumping_stroke, &tempdata,4);
//									zlog_info(c, "冲程 = %d   冲程 =  %f \n", htons(dynagraph_data->stroke), tempdata);
									zlog_info(c, "接收到一体化功图[组=%d]数据第 [%d] 包", instrument_group, dynagraph_data->data_serialnum[0]);
								}
								else
								{
									A11_req_dynagraph_others *dynagraph_other;

									dynagraph_other = (A11_req_dynagraph_others *)req;
									dg_num = dynagraph_other->data_serialnum;

									if(dg_num <= dg_group)
									{
										for(n = 0; n < 15; n ++)
										{
											pexbuffer[instrument_group]->loaddata.displacement[(dg_num - 1) * 15 + n] =  htons(dynagraph_other->dynagraph[n]);
											pexbuffer[instrument_group]->loaddata.load[(dg_num - 1) * 15 + n] = htons(dynagraph_other->dynagraph[(15 + n)]);
										}
										zlog_info(c, "接收到一体化功图[组=%d]数据第 [%d] 包", instrument_group, dg_num);
									}
									else
									{
										for(n = 0; n < dg_remainder; n ++)
										{
											pexbuffer[instrument_group]->loaddata.displacement[(dg_num - 1) * 15 + n] =  htons(dynagraph_other->dynagraph[n]);
											pexbuffer[instrument_group]->loaddata.load[(dg_num - 1) * 15 + n] = htons(dynagraph_other->dynagraph[(15 + n)]);
										}
										zlog_info(c, "接收到一体化功图[组=%d]数据第 [%d] 包", instrument_group, dynagraph_other->data_serialnum);
										pexbuffer[instrument_group]->dg_time = time(NULL);
										pexbuffer[instrument_group]->dg_OK = 0x3C;
									}
								}
								rsp_length = dataGroupRespone(0x0201);			// 准备一体化功图数据包应答数据
								if((pexbuffer[instrument_group]->dg_OK == 0x3C) && (pexbuffer[instrument_group]->elec_online == 0x3C))
								{

									res = comm_send(ctx, snd, rsp_length);
									if(res == -1)
									{
										zlog_warn(c, "一体化功图[组= %d]数据包有误", instrument_group);
										break;
									}

									rsp_length = readElecRespone(&data_ex[instrument_group]);
									zlog_info(c, "开始读取电流图[组=%d]数据包", instrument_group);
								}
								break;
							}
							default :
								break;
						}
						break;
					}
					case 0x0002:											// 无线压力
					{
						A11_revdata_press_tempreture * wireless_press;
						wireless_press = (A11_revdata_press_tempreture *)req;
						/* 根据仪器组号和编号判断数据放到哪里
						 *
						 */
						switch(htons(wireless_press->A11_framehead.data_type))
						{
							case 0x0000:																		//	上传的常规数据
								// 厂家代码
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.company_code
								= htons(wireless_press->A11_framehead.company_code);
								// 仪表类型
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.device_type
								= htons(wireless_press->A11_framehead.instrument_type);
								// 仪表组号
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.device_group
								= (wireless_press->A11_framehead.instrument_group);
								// 仪表编号
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.device_no
								= (wireless_press->A11_framehead.instument_num);
								// 通信效率
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.comm_efficiency
								= (uint16_t)(wireless_press->A11_frame_data.comm_efficiency);
								// 电池电压
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.bat_vol
								= (uint16_t)(wireless_press->A11_frame_data.bat_vol);
								// 休眠时间
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.sleep_time
								= htons(wireless_press->A11_frame_data.sleep_time);
								// 仪表状态
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.device_sta
								= htons(wireless_press->A11_frame_data.instument_sta);
								// 工作温度
//								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.work_temp[0]
//								= htons(wireless_press->instument_temp[0]) ;
//								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.work_temp[1]
//								= htons(wireless_press->instument_temp[1]) ;
								// 实时数据
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.realtime_data[0]
								= htons(wireless_press->realtime_data[0]) ;
								poilwell[instrument_group]->fuction_param.device_infor.oil_pressure.realtime_data[1]
								= htons(wireless_press->realtime_data[1]) ;
								zlog_info(c, "接收到无线压力[组=%d]常规数据帧", instrument_group);
								rsp_length = conventionalDataRespone(0x004C);
								break;
							case 0x0010:													// 上传的常规参数
								break;
							default:
								break;
						}
						break;
					}
					case 0x0003:											// 无线温度
					{
						A11_revdata_press_tempreture * wireless_tempreture;
						wireless_tempreture = (A11_revdata_press_tempreture *)req;
						switch(htons(wireless_tempreture->A11_framehead.data_type))
						{
							case 0x0000:
								// 厂家代码
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.company_code
								= htons(wireless_tempreture->A11_framehead.company_code);
								// 仪表类型
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.device_type
								= htons(wireless_tempreture->A11_framehead.instrument_type);
								// 仪表组号
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.device_group
								= (wireless_tempreture->A11_framehead.instrument_group);
								// 仪表编号
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.device_no
								= (wireless_tempreture->A11_framehead.instument_num);
								// 通信效率
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.comm_efficiency
								= (uint16_t)(wireless_tempreture->A11_frame_data.comm_efficiency);
								// 电池电压
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.bat_vol
								= (uint16_t)(wireless_tempreture->A11_frame_data.bat_vol);
								// 休眠时间
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.sleep_time
								= htons(wireless_tempreture->A11_frame_data.sleep_time);
								// 仪表状态
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.device_sta
								= htons(wireless_tempreture->A11_frame_data.instument_sta);
								// 工作温度
//								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.work_temp[0]
//								= htons(wireless_tempreture->instument_temp[0]) ;
//								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.work_temp[1]
//								= htons(wireless_tempreture->instument_temp[1]) ;
								// 实时数据
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.realtime_data[0]
								= htons(wireless_tempreture->realtime_data[0]) ;
								poilwell[instrument_group]->fuction_param.device_infor.wellhead_oil_temp.realtime_data[1]
								= htons(wireless_tempreture->realtime_data[1]) ;
								zlog_info(c, "接收到无线温度[组=%d]常规数据帧", instrument_group);
								rsp_length = conventionalDataRespone(0x004C);
								break;
							case 0x0010:

								break;
							default:
								break;
						}
						break;
					}
					case 0x0004:
					{// 无线电量
						A11_revdata_electrical_parameter *elec_param;
						elec_param = (A11_revdata_electrical_parameter *)req;

						switch(htons(elec_param->A11_framehead.data_type))
						{
							case 0x0000:				// 电参得常规应答
								// 说明电参在线
								pexbuffer[instrument_group]->elec_online = 0x3C;
								memcpy(&data_ex[instrument_group].ZB91_framehead, &elec_param->ZB91_framehead, sizeof(ZB91_revdata_framehead));
								memcpy(&data_ex[instrument_group].A11_framehead, &elec_param->A11_framehead, sizeof(A11_data_framehead));


//								if(elec_param->A11_framehead.company_code == htons(0x0004))	// kaishan
								{
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_a[0] = htons(elec_param->current_phase_a[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_a[1] = htons(elec_param->current_phase_a[1]);
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_b[0] = htons(elec_param->current_phase_b[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_b[1] = htons(elec_param->current_phase_b[1]);
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_c[0] = htons(elec_param->current_phase_c[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.current_phase_c[1] = htons(elec_param->current_phase_c[1]);
									// 电机工作电压
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[0] = htons(elec_param->voltage_phase_a[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[1] = htons(elec_param->voltage_phase_a[1]);
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[0] = htons(elec_param->voltage_phase_b[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[1] = htons(elec_param->voltage_phase_b[1]);
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[0] = htons(elec_param->voltage_phase_c[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.voltage_phase_a[1] = htons(elec_param->voltage_phase_c[1]);
									// 功率因数
									poilwell[instrument_group]->run_ctrl.elec_param.power_factor[0] = htons(elec_param->power_factor[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.power_factor[1] = htons(elec_param->power_factor[1]);
									// 有功功率
									poilwell[instrument_group]->run_ctrl.elec_param.moto_p[0] = htons(elec_param->moto_p[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.moto_p[1] = htons(elec_param->moto_p[1]);
									// 无功功率
									poilwell[instrument_group]->run_ctrl.elec_param.moto_q[0] = htons(elec_param->moto_q[0]);
									poilwell[instrument_group]->run_ctrl.elec_param.moto_q[1] = htons(elec_param->moto_q[1]);

									zlog_info(c, "接收到无线电参[组=%d]常规数据帧", instrument_group);

									rsp_length = conventionalDataRespone(0x00B8);
								}
								break;
							case 0x0010:
								break;
							case 0x0030:				// 上传的电参数据包
							{
								A11_req_elec_first *elec_data;
								elec_data = (A11_req_elec_first *)req;
								elec_num = elec_data->data_serialnum;
								if(elec_num == 0)									// 说明该数据为第一组数据
								{
									elec_dot = htons(elec_data->dot);
									if(elec_dot <=  250)
									{
										elec_group = (unsigned char)(elec_dot / 15);
										elec_remainder = (unsigned char)(elec_dot %15);
									}
									else
									{
										zlog_warn(c, "电参[组=%d]采集总点数[%d] > 250", instrument_group, elec_dot);
										elec_group = 0;
										elec_remainder = 0;
										break;
									}
									zlog_info(c, "接收到电流图[组=%d]数据第 [%d] 包", instrument_group,elec_num);
								}
								else
								{
									A11_req_elec_others *elec_other;

									elec_other = (A11_req_elec_others *)req;

									if(elec_num <= elec_group)
									{
										for(n = 0; n < 15; n ++)
										{
											pexbuffer[instrument_group]->loaddata.current[(elec_num - 1) * 15 + n] = htons(elec_other->current_chart[n]);
											pexbuffer[instrument_group]->loaddata.power[(elec_num - 1) * 15 + n] = htons(elec_other->current_chart[(15 + n)]);
										}

										zlog_info(c, "接收到电流图[组=%d]数据第 [%d] 包", instrument_group, elec_num);
									}
									else
									{
										for(n = 0; n < dg_remainder; n ++)
										{
											pexbuffer[instrument_group]->loaddata.current[(elec_num - 1) * 15 + n] = htons(elec_other->current_chart[n]);
											pexbuffer[instrument_group]->loaddata.power[(elec_num - 1) * 15 + n] = htons(elec_other->current_chart[(15 + n)]);
										}
										zlog_info(c, "接收到电流图[组=%d]数据第 [%d] 包", instrument_group, elec_num);
										pexbuffer[instrument_group]->elec_OK = 0x3C;
									}

								}
								rsp_length = dataGroupResponeElec(0x0211);			// 电流图
								break;
							}
							default:
								break;
						}
						break;
					}
					case 0x0005:											// 无线角位移
						break;
					case 0x0006:											// 无线载荷
						break;
					case 0x0007:											// 无线扭矩
						break;
					case 0x0008:											// 无线动液面
						break;
					case 0x0009:											// 无线一体化转速扭矩
						break;
					case 0x1F10:											// 控制器(RTU)设备
						break;
					case 0x1F11:											// 控制器(RTU)设备
						break;
					default:													// 手操器
						break;
				}
				break;
			}
			default:
				break;
		}
		break;
	default:
		break;
	}

	return comm_send(ctx, snd, rsp_length);
}

/*@brief
 * wsf
 * zigbee入网后得应答
 *
 */
//inline int ZBnetRespone()
//{
//	ZB_explicit_RX_indicator *ZB_91;
//	ZB_explicit_cmd_frame *ZB_11;
//	int data_length = 0;
//	memset(zb_snd_buf, 0, sizeof(zb_snd_buf));
//	ZB_11 = (ZB_explicit_cmd_frame *)zb_snd_buf;
//	ZB_91 = (ZB_explicit_RX_indicator *)zb_rev_buf;
//	ZB_11->ZB11_framehead.start_elimiter = 0x7E;
////				ZB_11->length = htons(sizeof(ZB_explicit_cmd_frame) - 4 - 2);				// 参数应答
//	ZB_11->ZB11_framehead.length = htons(sizeof(ZB_explicit_cmd_frame) - 4);					// 常规数据应答
//	ZB_11->ZB11_framehead.frame_type = 0x11;
//	ZB_11->ZB11_framehead.frame_ID = 0x00;									//00无ACK  01有ACK
//	memcpy(ZB_11->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr,22);
//	ZB_11->ZB11_framehead.radius = 0x00;
//	ZB_11->ZB11_framehead.send_opt = 0x60;
//	memcpy(&ZB_11->A11_framehead, &ZB_91->A11_framehead, sizeof(A11_data_framehead));
//	ZB_11->A11_framehead.instrument_type = htons(0x1F10);						//根据标准规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
//	ZB_11->A11_framehead.data_type = htons(0x0100);									// 常规数据应答
//	ZB_11->sleep_time = htons(0x7600);		//htons(0x000A);
//	data_length = sizeof(ZB_explicit_cmd_frame);
//	ZB_11->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
//
//	return data_length;
//}
/*@brief
 * wsf
 * 常规数据应答函数
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 */
static int conventionalDataRespone(unsigned short int sleeptime)
{
	ZB_explicit_RX_indicator *ZB_91;
	ZB_explicit_cmd_frame *ZB_11;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	ZB_11 = (ZB_explicit_cmd_frame *)zb_snd_buf;
	ZB_91 = (ZB_explicit_RX_indicator *)zb_rev_buf;
	ZB_11->ZB11_framehead.start_elimiter = 0x7E;
//				ZB_11->length = htons(sizeof(ZB_explicit_cmd_frame) - 4 - 2);				// 参数应答
	ZB_11->ZB11_framehead.length = htons(sizeof(ZB_explicit_cmd_frame) - 4);					// 常规数据应答
	ZB_11->ZB11_framehead.frame_type = 0x11;
	ZB_11->ZB11_framehead.frame_ID = 0x00;									//00无ACK  01有ACK
	memcpy(ZB_11->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr,22);
	ZB_11->ZB11_framehead.radius = 0x00;
	ZB_11->ZB11_framehead.send_opt = 0x00;	// 0x60;
	memcpy(&ZB_11->A11_framehead, &ZB_91->A11_framehead, sizeof(A11_data_framehead));
	ZB_11->A11_framehead.instrument_type = htons(0x1F10);						//根据标准规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
//				ZB_11->fram_head.data_type = htons(0x0101);									// 参数应答
	ZB_11->A11_framehead.data_type = htons(0x0100);									// 常规数据应答
	ZB_11->sleep_time = htons(sleeptime);		//htons(0x000A);
	data_length = sizeof(ZB_explicit_cmd_frame);
	ZB_11->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
//				rsp_length -= 2;																						// 参数应答
//				zb_snd_buf[33] = zbCheck(zb_snd_buf,3,(rsp_length - 1));					// 参数应答
//				usleep(200);
	return data_length;
}
/*@brief
 * wsf
 * G10一体化功图参数写应答帧 开始采集命令
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 */
static int collectDynagraphRespone(data_exchange *datex)
{
	ZB_explicit_RX_indicator *ZB_91;
	A11_rsp_collect_dynagraph *collect;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	ZB_91 = (ZB_explicit_RX_indicator *)zb_rev_buf;
	collect = (A11_rsp_collect_dynagraph *)zb_snd_buf;

	collect->ZB11_framehead.start_elimiter = 0x7E;
	collect->ZB11_framehead.length = htons(sizeof(A11_rsp_collect_dynagraph) - 4);
	collect->ZB11_framehead.frame_type = 0x11;
	collect->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(collect->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr, 18);
	collect->ZB11_framehead.radius = 0x00;
	collect->ZB11_framehead.send_opt = 0x00;	// 0x60;

	memcpy(&collect->A11_framehead, &ZB_91->A11_framehead, sizeof(A11_data_framehead));
	collect->A11_framehead.instrument_type =  htons(0x1F10);						// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	collect->A11_framehead.data_type = htons(0x0200);									// 根据A11规范，RTU应答一体化功图开始测试该数据类型为0x0200

	collect->mode = datex->dynagraph_mode;
	collect->dot = htons(datex->dot);
	collect->synchronization_time = htons(datex->synchronization_time);
	collect->time_interval = htons(datex->time_interval);
//							collect->company_func = ;

	data_length = sizeof(A11_rsp_collect_dynagraph);
	collect->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));

	return data_length;
}

/*@brief
 * wsf
 * G13 电参参数写应答帧 开始采集命令
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 */
static int collectElecRespone(data_exchange *datex)
{
	A11_rsp_collect_elec *collect;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	collect = (A11_rsp_collect_elec *)zb_snd_buf;

	collect->ZB11_framehead.start_elimiter = 0x7E;
	collect->ZB11_framehead.length = htons(sizeof(A11_rsp_collect_elec) - 4);
	collect->ZB11_framehead.frame_type = 0x11;
	collect->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(collect->ZB11_framehead.mac_addr, datex->ZB91_framehead.mac_addr, 18);
	collect->ZB11_framehead.radius = 0x00;
	collect->ZB11_framehead.send_opt = 0x00;	// 0x60;

	memcpy(&collect->A11_framehead, &datex->A11_framehead, sizeof(A11_data_framehead));
	collect->A11_framehead.instrument_type = htons(0x1F10);								// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	collect->A11_framehead.data_type = htons(0x0210);									// 根据A11规范，RTU应答电参开始测试该数据类型为0x0210

	collect->mode = datex->dynagraph_mode;//0x10;//
	collect->dot = htons(datex->dot);//0xC800;//
	collect->synchronization_time = htons(datex->synchronization_time);//0x9cF7;//
	collect->time_interval = htons(datex->time_interval);//0xCC04;//
	collect->algorithms = datex->algorithms;//0x00;//

	data_length = sizeof(A11_rsp_collect_elec);
	collect->check_sum = zbCheck(zb_snd_buf, 3, (data_length - 1));
	return data_length;
}
/*@brief
 * wsf
 * G18 一体化功图应答数据帧格式
 * G21 无线载荷应答数据帧格式
 * G24 无线位移应答数据正格式
 * G28 电参应答数据帧格式
 * G32 专项数据应答数据帧格式
 * 发送
 * 数据类型：功图0x0201，无线载荷0x0205，无线位移0x0208，电参0x0211，专项数据0x0231
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 */
static int dataGroupRespone(unsigned short int data_type)
{
	A11_req_dynagraph_first *ZB_91;
	A11_rsp_datagroup *rsp_dynagraph;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	ZB_91 = (A11_req_dynagraph_first *)zb_rev_buf;
	rsp_dynagraph = (A11_rsp_datagroup *)zb_snd_buf;

	rsp_dynagraph->ZB11_framehead.start_elimiter = 0x7E;
	rsp_dynagraph->ZB11_framehead.length = htons(sizeof(A11_rsp_datagroup) - 4);
	rsp_dynagraph->ZB11_framehead.frame_type = 0x11;
	rsp_dynagraph->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(rsp_dynagraph->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr, 18);
	rsp_dynagraph->ZB11_framehead.radius = 0x00;
	rsp_dynagraph->ZB11_framehead.send_opt = 0x00;	// 0x60;

	memcpy(&rsp_dynagraph->A11_framehead, &ZB_91->A11_framehead, sizeof(A11_data_framehead));
	rsp_dynagraph->A11_framehead.instrument_type =  htons(0x1F10);						// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	rsp_dynagraph->A11_framehead.data_type = htons(data_type);									// 根据A11规范，RTU应答一体化功图应答功图数据类型为0x0201

	rsp_dynagraph->data_serialnum = ZB_91->data_serialnum[0];

	data_length = sizeof(A11_rsp_datagroup);
	rsp_dynagraph->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
	return data_length;
}
/*@brief
 * wsf
 * G28 电参应答数据帧格式
 * 发送
 * 数据类型：功图0x0201，无线载荷0x0205，无线位移0x0208，电参0x0211，专项数据0x0231
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 */
static int dataGroupResponeElec(unsigned short int data_type)
{
	A11_req_elec_first *ZB_91;
	A11_rsp_datagroup *rsp_dynagraph;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	ZB_91 = (A11_req_elec_first *)zb_rev_buf;
	rsp_dynagraph = (A11_rsp_datagroup *)zb_snd_buf;

	rsp_dynagraph->ZB11_framehead.start_elimiter = 0x7E;
	rsp_dynagraph->ZB11_framehead.length = htons(sizeof(A11_rsp_datagroup) - 4);
	rsp_dynagraph->ZB11_framehead.frame_type = 0x11;
	rsp_dynagraph->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(rsp_dynagraph->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr, 18);
	rsp_dynagraph->ZB11_framehead.radius = 0x00;
	rsp_dynagraph->ZB11_framehead.send_opt = 0x00;//0x60;

	memcpy(&rsp_dynagraph->A11_framehead, &ZB_91->A11_framehead, sizeof(A11_data_framehead));
	rsp_dynagraph->A11_framehead.instrument_type =  htons(0x1F10);						// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	rsp_dynagraph->A11_framehead.data_type = htons(data_type);									// 根据A11规范，RTU应答一体化功图应答功图数据类型为0x0201

	rsp_dynagraph->data_serialnum = ZB_91->data_serialnum;

	data_length = sizeof(A11_rsp_datagroup);
	rsp_dynagraph->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
	return data_length;
}
/*@brief
 * wsf
 * G25 读电流图数据命令帧格式
 * 取出zb_rev_buf中的相关数据以ZB11格式放入zb_snd_buf中
 * 发送
 */
static int readElecRespone(data_exchange *datex)
{
	A11_rsp_currentchart *readcurrent;
	int data_length = 0;
	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	readcurrent = (A11_rsp_currentchart *)zb_snd_buf;
	readcurrent->ZB11_framehead.start_elimiter = 0x7E;
	readcurrent->ZB11_framehead.length = htons(sizeof(A11_rsp_currentchart) - 4);
	readcurrent->ZB11_framehead.frame_type = 0x11;
	readcurrent->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(readcurrent->ZB11_framehead.mac_addr, datex->ZB91_framehead.mac_addr, 18);
	readcurrent->ZB11_framehead.radius = 0x00;
	readcurrent->ZB11_framehead.send_opt = 0x00;	// 0x60;

	memcpy(&readcurrent->A11_framehead, &datex->A11_framehead, sizeof(A11_data_framehead));
	readcurrent->A11_framehead.instrument_type =  htons(0x1F10);						// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	readcurrent->A11_framehead.data_type = htons(0x0212);									// 根据A11规范，RTU应答电参获取电流图数据类型为 0x0212

	readcurrent->dynagraph_mode = datex->dynagraph_mode;//0x00;//
	readcurrent->algorithms = datex->algorithms;//0x00;//
	readcurrent->synchronization_time = htons(datex->synchronization_time);//0x9cF7;//
	readcurrent->time_mark = htons(datex->time_mark);//0x00;//
	readcurrent->cycle = htons(datex->cycle);//0xe803;//
	readcurrent->dot = htons(datex->dot);//0xC800;//


	data_length = sizeof(A11_rsp_currentchart);
	readcurrent->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
	return data_length;
}
/* @brief
 * wsf
 * ZigBee 入网帧的回应
 *
 */
static int ZBnetinResponse()
{
	ZB91_netin_framehead *ZB_91;
	ZB11_netin_framehead *ZB_11;
	int data_length = 0;

	bzero(zb_snd_buf, BUFFER_SIZE *sizeof(uint8_t));
	ZB_91 = (ZB91_netin_framehead *)zb_rev_buf;
	ZB_11 = (ZB11_netin_framehead *)zb_snd_buf;

	ZB_11->ZB11_framehead.start_elimiter = 0x7E;
	ZB_11->ZB11_framehead.length = htons(sizeof(ZB11_netin_framehead) - 4);					// 常规数据应答
	ZB_11->ZB11_framehead.frame_type = 0x00;
	ZB_11->ZB11_framehead.frame_ID = 0x13;									//00无ACK  01有ACK
	memcpy(ZB_11->ZB11_framehead.mac_addr, ZB_91->ZB91_framehead.mac_addr,8);
	ZB_11->ZB11_framehead.network_addr = htons(0xFFFE);
	ZB_11->ZB11_framehead.source_endpoint = 0x00;
	ZB_11->ZB11_framehead.destination_endpoint = 0x00;
	ZB_11->ZB11_framehead.cluster_ID = htons(0x0032);
	ZB_11->ZB11_framehead.profile_ID = 0x0000;
	ZB_11->ZB11_framehead.radius = 0x00;
	ZB_11->ZB11_framehead.send_opt = 0x00;
	ZB_11->dat = htons(0x7600);

	data_length = sizeof(ZB11_netin_framehead);
	ZB_11->check_sum = zbCheck(zb_snd_buf,3,(data_length - 1));
	return data_length;
}
// 更新仪表信息寄存器 49100~49227
static int updateInstrument()
{
	int n;

	for(n = 0; n < 63; n++)
	{
		// 查找当前仪表是否在列表中
		if((poilwell[0]->fuction_param.custom.instrument[n].type)	\
			&& (poilwell[0]->fuction_param.custom.instrument[n].type == ZB_91_normal->A11_framehead.instrument_type >> 8)	\
			&& (poilwell[0]->fuction_param.custom.instrument[n].group == ZB_91_normal->A11_framehead.instrument_group)	\
			&& (poilwell[0]->fuction_param.custom.instrument[n].num == ZB_91_normal->A11_framehead.instument_num)	\
			&& (poilwell[0]->fuction_param.custom.instrument[n].addr == 0))
		{
			// 更新在线时间
			instimeout[n].start_time = time(NULL);
			return 0;
		}
	}
	if(n > 63)
	{
		zlog_info(c, "仪表信息寄存器已满(最大63个信息)");
		return 1;
	}
	else		// 插入仪表信息
	{
		for(n = 0; n < 63; n++)
		{
			// 查找列表中的空闲位置
			if(poilwell[0]->fuction_param.custom.instrument[n].type == 0)
			{
				poilwell[0]->fuction_param.custom.instrument[n].type = ZB_91_normal->A11_framehead.instrument_type >> 8;
				poilwell[0]->fuction_param.custom.instrument[n].group = ZB_91_normal->A11_framehead.instrument_group;
				poilwell[0]->fuction_param.custom.instrument[n].num = ZB_91_normal->A11_framehead.instument_num;
				if(ZB_91_normal->A11_framehead.instrument_type > 0x4000)
					poilwell[0]->fuction_param.custom.instrument[n].addr = 1;	// 有线
				else
					poilwell[0]->fuction_param.custom.instrument[n].addr = 0;	// 无线
				instimeout[n].start_time = time(NULL);
				instimeout[n].interval = 120;
				break;
			}
		}
	}
	return 0;
}
