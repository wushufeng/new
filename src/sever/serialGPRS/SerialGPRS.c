/*
 * SerialGPRS.c
 *
 *  Created on: 2015年6月8日
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

#include "SerialGPRS.h"
#include "../../A11_sysAttr/a11sysattr.h"
#include "../../database/database.h"
#include "../../modbus/modbus-private.h"
#include "../sysdatetime/getsysdatetime.h"
#include "../../log/rtulog.h"

#define 	GPRSDEVICE			"/dev/ttyS4"
//#define 	GPRSDEVICE			"/dev/ttyUSB0"
modbus_t *ctx_gprs;
int use_backend_gprs;
unsigned short int *tab_rp_registers;
int nb_points;
pthread_t gprs_thread;
extern oil_well *poilwell[17];

unsigned char gprs_rsp_data[260];
unsigned char gprs_req_data[260];

typedef enum {
    _STEP_FUNCTION,
    _STEP_META,
    _STEP_DATA
} _step_t;
typedef enum {
    /* Request message on the server side */
    MSG_INDICATION,
    /* Request message on the client side */
    MSG_CONFIRMATION
} msg_type_t;

static int gprsThreadFunc(void *arg);
static int receive_msg_gprs(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type);
static int gprs_reply(modbus_t *ctx,  uint8_t *req, int req_length);


int serialGPRSThreadCancel(void)
{
	int res;
	void * thread_result;
	zlog_info(c, "正在取消SerialGPRS线程");
	res = pthread_cancel(gprs_thread);
	if(res != 0)	{
		zlog_error(c, "SerialGPRS线程取消失败");
		exit(EXIT_FAILURE);
	}
	zlog_info(c, "正在joinSerialGPRS线程");
	res = pthread_join(gprs_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "SerialGPRS线程join失败");
		exit(EXIT_FAILURE);
	}
	return 0;
}

/* @brief
 * wsf
 * gprs初始化
 */
int serialGprsInit(void *obj)
{
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	int baud;
	char parity;
	int data_bit;
	int stop_bit;

	use_backend_gprs = GPRS;

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
	ctx_gprs = modbus_new_rtu(GPRSDEVICE, baud, parity, data_bit, stop_bit);
	if (ctx_gprs == NULL) {
		fprintf(stderr, "[错误]GPRS不能动态分配一个内存环境\n");
		return -1;
	}
	modbus_set_debug(ctx_gprs, TRUE);
	modbus_set_error_recovery(ctx_gprs,
	                              MODBUS_ERROR_RECOVERY_LINK |
	                              MODBUS_ERROR_RECOVERY_PROTOCOL);
    if (use_backend_gprs == GPRS) {
          modbus_set_slave(ctx_gprs, SERVER_ID);
    }

//    nb_points = 300;
//    tab_rp_registers = (uint16_t *) malloc(300 * sizeof(uint16_t));
//    memset(tab_rp_registers, 0, 300 * sizeof(uint16_t));

	return 0;
}
void serialGprsFree()
{
	 modbus_close(ctx_gprs);
//	if(query_serial232 !=NULL)
//		free(query_serial232);
	if(ctx_gprs != NULL)
		modbus_free(ctx_gprs);
}
int createGprsThread(void)
{
	int res;
	res = pthread_create(&gprs_thread, NULL, (void*)&gprsThreadFunc, NULL);
    if(res != 0)
    {
    	fprintf(stderr, "[错误]GPRS线程创建失败: %s\n", modbus_strerror(errno));
//        perror("Thread creation failed");
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}
static int gprsThreadFunc(void *arg)
{
	int rc;
	int res;
	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "GPRS线程pthread_setcancelstate失败");
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "GPRS线程pthread_setcanceltype失败");
		exit(EXIT_FAILURE);
	}
    rc = modbus_connect(ctx_gprs);
    if (rc == -1) {
    	zlog_error(c, "GPRS不能连接! %s\n", modbus_strerror(errno));
//        modbus_free(ctx_gprs);
//        return -1;
    	pthread_exit(0);
    }
    for(;;)
    {
    	rc = receive_msg_gprs(ctx_gprs, gprs_req_data, MSG_INDICATION);

        if (rc == -1) {
            /* Connection closed by the client or error */
        	goto sl;
        }
        if (ctx_gprs->debug) {
            int i;
            printf(">> ");
            for (i=0; i < rc; i++)
            	printf("%.2X ", gprs_req_data[i]);
            printf("\n");
        }
        rc =gprs_reply(ctx_gprs, gprs_req_data, rc);//, mb_mapping);
        if (rc == -1) {
            /* Connection closed by the client or error */
//            break;
        	goto sl;
        }
sl:
	sleep(1);
	}
	pthread_exit(0);
}
static int receive_msg_gprs(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type)
{
	return 0;
}
static int gprs_reply(modbus_t *ctx,  uint8_t *req, int req_length)
{
	return 0;
}
