/*
 * Serial232.c
 *
 *  Created on: 2015年4月13日
 *      Author: wsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
# include <errno.h>

#include "Serial232.h"
#include "../../A11_sysAttr/a11sysattr.h"
//#include "../../modbus/modbus.h"
#include "../../database/database.h"
#include "../../modbus/modbus-private.h"
#include "../../log/rtulog.h"


//#define 	SERIAL232			"/dev/ttyS2"
#define 	SERIAL232			"/dev/ttyUSB1"

modbus_t *ctx_serial232;
pthread_t serial232_thread;
int rc;
int use_backend_serial232;
uint8_t *query_serial232;
int header_length;


//enum {
//    TCP,
//    UDP,
//    RTU
//};

static int serial232ThreadFunc(void *arg);

static int serial232ThreadFunc(void *arg)
{
	int serial_slave;
	int serial_offset;
	int res;

	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "Serial232线程pthread_setcancelstate失败");
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "Serial232线程pthread_setcanceltype失败");
		exit(EXIT_FAILURE);
	}

    rc = modbus_connect(ctx_serial232);
    if (rc == -1) {
        fprintf(stderr, "[错误]Serial232接口不能连接! %s\n", modbus_strerror(errno));
        modbus_free(ctx_serial232);
        return -1;
    }
    for (;;) {
    	/*
    	 * 函数modbus_receive,判断指示或回应两模式下不同功能码接受的数据
    	 * 返回正确接受的数据,并且该数据通过CRC校验
    	 * 正确返回接收到数据的总长度,所读出的数据放入query_serial232所指向的地址中
    	 * 错误返回-1
    	 */
        rc = modbus_receive(ctx_serial232, query_serial232, 22);
        if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
        serial_offset = ctx_serial232->backend->header_length;			// 取出数据头长度默认为1
        serial_slave = query_serial232[serial_offset - 1];
        if((serial_slave < 1) ||((serial_slave > 16) && (serial_slave != 128)))
        {
        	printf("[提示]Modbus Serial device ID = %d, 不在范围内ID = 128 || 1 <= ID <= 16 \n", serial_slave);
        	break;
        }
        if(serial_slave == 128)
        	serial_slave = 0;
        rc = modbus_reply(ctx_serial232, query_serial232, rc, (modbus_mapping_t *)mb_mapping[serial_slave]);
        if (rc == -1) {
            break;
        }
    }
	pthread_exit(0);
}
int createSerial232Thread(void)
{
	int res;
	res = pthread_create(&serial232_thread, NULL, (void*)&serial232ThreadFunc, NULL);
    if(res != 0)
    {
    	fprintf(stderr, "Failed to allocate the query: %s\n", modbus_strerror(errno));
//        perror("Thread creation failed");
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}

int serial232Init(void *obj)
{
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	int baud;
	char parity;
	int data_bit;
	int stop_bit;

	use_backend_serial232 = RTU;

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

    ctx_serial232 = modbus_new_rtu(SERIAL232, baud, parity, data_bit, stop_bit);		//linux 初始化参数
    modbus_set_slave(ctx_serial232, SERVER_ID);												// 设置本机地址
    query_serial232 = malloc(MODBUS_RTU_MAX_ADU_LENGTH);				//开辟256的动态内存空间
    if(query_serial232 == NULL)
    {
    	fprintf(stderr, "Failed to allocate the query: %s\n", modbus_strerror(errno));
//        	perror("Malloc query failed");
    	return -1;
    }

    header_length = modbus_get_header_length(ctx_serial232);
    modbus_set_debug(ctx_serial232, TRUE);
	return 0;
}
int serial232ThreadCancel(void)
{
	int res;
	void * thread_result;
	zlog_info(c, "正在取消Serial232线程");
	res = pthread_cancel(serial232_thread);
	if(res != 0)	{
		zlog_error(c, "Serial232线程取消失败");
		exit(EXIT_FAILURE);
	}
	zlog_info(c, "正在joinSerial232线程");
	res = pthread_join(serial232_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "Serial232线程join失败");
		exit(EXIT_FAILURE);
	}
	return 0;
}
void serial232Free()
{
//	if(mb_mapping !=NULL)
//		modbus_mapping_free(mb_mapping);
	 modbus_close(ctx_serial232);
	if(query_serial232 !=NULL)
		free(query_serial232);
	if(ctx_serial232 != NULL)
		modbus_free(ctx_serial232);
}
