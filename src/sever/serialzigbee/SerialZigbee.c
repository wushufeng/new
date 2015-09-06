/*
 * SerialZigbee.c
 *
 *  Created on: 2015年4月13日
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

#include "../../A11_sysAttr/a11sysattr.h"
#include "../../database/database.h"
#include "../../modbus/modbus-private.h"
#include "../sysdatetime/getsysdatetime.h"
#include "../../log/rtulog.h"
#include "../../def.h"
#ifdef ARM_32
#define 	ZIGBEEDEVICE			"/dev/ttyS3"
#else
#define 	ZIGBEEDEVICE			"/dev/ttyUSB0"
#endif
modbus_t *ctx_zigbee;
int use_backend_zigbee;
unsigned short int *tab_rp_registers;
int nb_points;
pthread_t zigbee_thread;
ZB_explicit_RX_indicator *ZB_91_normal;
extern oil_well *poilwell[17];

//load_displacement *ptempbuf[17];									// 存放功图电参得临时buffer
extern exchangebuffer *pexbuffer[17];											// 存放功图电参的临时buffer


unsigned char instrument_group;									// 定义全局仪器组号，用次判断数据存放位置

unsigned short int dg_dot;
unsigned char dg_group;
unsigned char dg_num;
unsigned char dg_remainder;
unsigned short int elec_dot;
unsigned char elec_group;
unsigned char elec_num;
unsigned char elec_remainder;

//int dgOk2elecFlag = 0;
//unsigned char dg_mode = 0x10;					// 功图模式
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
//unsigned char AT_ND[8] = {0x7E,0x00,0x04,0x08,0x01,0x4E,0x44,0x64};  //寻找和报告网络模块
//uint8_t rsp[64] = {0x7E,0x00,0x20,0x11,0x00,0x00,0x13,0xA2,0x00,0x40,0xA7,0x62,0xFE,0xFF,0xFE,0xE8,0xE8,0x00,0x11,0x18,0x57,0x00,0x60,0x00,0x00,0x00,0x22,0x00,0x03,0x01,0x01,0x01,0x00,0x00,0x0A,0x07};
unsigned char rsp_data[260];
unsigned char req_data[260];


/////////////////debug//////////////////////
//unsigned char electR[35] = {0x7E, 0x00, 0x1F, 0x11, 0x00, 0x00, 0x13, 0xA2, 0x00, 0x40, 0xA8, 0x98, 0xA1, 0xE0, 0x9E, 0xE8, 0xE8, 0x00, 0x11, 0x18, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1F, 0x10, 0x01, 0x01, 0x02, 0x11, 0x0C, 0xF6};
//time_t d_tm_now;							// 读出当前时间秒数
//time_t d_tm_start;
// tcsetattr
//struct timeval t_start, t_end;

//long cost_time = 0;
/////////////////debug//////////////////////

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

static int zigbeeThreadFunc(void *arg);
static int receive_msg_zigbee(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type);
static int _sleep_and_flush_zigbee(modbus_t *ctx);
//static int compute_data_length_after_meta_zigbee(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type);
//static uint8_t compute_meta_length_after_function_zigbee(int function, msg_type_t msg_type);
static int _zigbee_check_integrity(modbus_t *ctx, uint8_t *msg, const int msg_length);
static int zigbee_reply(modbus_t *ctx, uint8_t *req, int req_length);
static int send_zigbee_msg(modbus_t *ctx, uint8_t *msg, int msg_length);
static uint8_t zigbeeCheck(uint8_t *buf, uint16_t start, uint16_t cnt);
inline int conventionalDataRespone(unsigned short int sleeptime);
static int collectDynagraphRespone(data_exchange *datex);
static int collectElecRespone(data_exchange *datex);
inline int dataGroupRespone(unsigned short int data_type);
inline int dataGroupResponeElec(unsigned short int data_type);
static int readElecRespone(data_exchange *datex);
static int ZBnetinResponse();


static int zigbeeThreadFunc(void *arg)
{
	int rc;
	int res;
	int n;

	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "Zigbee线程pthread_setcancelstate失败");
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "Zigbee线程pthread_setcanceltype失败");
		exit(EXIT_FAILURE);
	}
	for(n = 0; n < 17; n ++)
	{
		data_ex[n].dynagraph_mode = poilwell[0]->fuction_param.custom.dynagraph_mode;
	}

    rc = modbus_connect(ctx_zigbee);
    if (rc == -1) {
        zlog_error(c, "ZigBee接口不能连接! %s", modbus_strerror(errno));
//        modbus_free(ctx_zigbee);
        pthread_exit(0);
//        return -1;
    }
    for(;;)
    {
    	rc = receive_msg_zigbee(ctx_zigbee, req_data, MSG_INDICATION);


        if (rc == -1) {
            /* Connection closed by the client or error */
        	zlog_warn(c, "Zigbee接收数据错误 = %d", rc);
        	continue;
        }
        if (ctx_zigbee->debug) {
            int i;
            if(rc)
            	printf(">> ");
            for (i=0; i < rc; i++)
//                printf("<%.2X>", msg[msg_length + i]);
            	printf("%.2X ", req_data[i]);
            if(rc)
            	printf("\n");
        }
        rc = zigbee_reply(ctx_zigbee, req_data, rc);//, mb_mapping);
        if (rc == -1) {
            /* Connection closed by the client or error */
        	zlog_warn(c, "Zigbee发送数据错误 = %d", rc);
        	continue;
        }
	//sleep(1);//此处休眠时间过长，不能有
	}
//close:
//    /* Free the memory */
////    free(tab_rp_bits);
//    free(tab_rp_registers);
//    /* Close the connection */
//    modbus_close(ctx_zigbee);
//    modbus_free(ctx_zigbee);
	pthread_exit(0);
}
int createZigbeeThread(void)
{
	int res;
	res = pthread_create(&zigbee_thread, NULL, (void*)&zigbeeThreadFunc, NULL);
    if(res != 0)
    {
    	zlog_error(c, "创建SerialZigbee线程失败:%s", modbus_strerror(errno));
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}
int serialZigbeeInit(void *obj)
{
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	int baud;
	char parity;
	int data_bit;
	int stop_bit;

	use_backend_zigbee = ZIGBEE;

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
	ctx_zigbee = modbus_new_rtu(ZIGBEEDEVICE, baud, parity, data_bit, stop_bit);
	if (ctx_zigbee == NULL) {
		zlog_error(c, "Unable to allocate libmodbus context");
		return -1;
	}
//	modbus_set_debug(ctx_zigbee, TRUE);
	modbus_set_debug(ctx_zigbee, FALSE);
	modbus_set_error_recovery(ctx_zigbee,
	                              MODBUS_ERROR_RECOVERY_LINK |
	                              MODBUS_ERROR_RECOVERY_PROTOCOL);
    if (use_backend_zigbee == ZIGBEE) {
          modbus_set_slave(ctx_zigbee, SERVER_ID);
    }

//    nb_points = (UT_REGISTERS_NB > UT_INPUT_REGISTERS_NB) ?
//    UT_REGISTERS_NB : UT_INPUT_REGISTERS_NB;
//    tab_rp_registers = (uint16_t *) malloc(nb_points * sizeof(uint16_t));
//    memset(tab_rp_registers, 0, nb_points * sizeof(uint16_t));
//    nb_points = 300;
//    tab_rp_registers = (uint16_t *) malloc(300 * sizeof(uint16_t));
//    memset(tab_rp_registers, 0, 300 * sizeof(uint16_t));

	return 0;
}
int serialZigbeeCancel(void)
{
	int res;
	void * thread_result;

	int kill_rc = pthread_kill(zigbee_thread,0);		// 使用pthread_kill函数发送信号0判断线程是否还在
	zlog_info(c, "正在取消SerialZigbee线程...");
	if(kill_rc == ESRCH)					// 线程不存在：ESRCH
		zlog_warn(c, "SerialZigbee线程不存在或者已经退出");
	else if(kill_rc == EINVAL)		// 信号不合法：EINVAL
		zlog_warn(c, "非法信号");
	else
	{
		res = pthread_cancel(zigbee_thread);
		if(res != 0)	{
			zlog_error(c, "取消SerialZigbee线程失败-%d", res);
			exit(EXIT_FAILURE);
		}
	}

	zlog_info(c, "正在等待SerialZigbee线程结束...");
	res = pthread_join(zigbee_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待SerialZigbee线程结束失败-%d", res);
		exit(EXIT_FAILURE);
	}
	return 0;
}
void serialZigbeeFree()
{

	 modbus_close(ctx_zigbee);

	if((ctx_zigbee != NULL) &&(ctx_zigbee->backend != NULL))
	{
		if(ctx_zigbee->s != -1)
		{
			free(ctx_zigbee->backend_data);
			free(ctx_zigbee);
		}
	}
//	for(n = 0; n < 17; n ++)
//	{
//		if(ptempbuf[n] != NULL)
//			free(ptempbuf[n]);
//	}
}
static int receive_msg_zigbee(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type)
/*
 * 指针msg用于保存接收到的数据首地址
 */
{
    int rc;
    fd_set rfds;							// 申请一组文件描述符集合
    struct timeval tv;						// 延时时间结构体
    struct timeval *p_tv;					// 延时时间结构体指针,用于指向此类结构体
    int length_to_read;
    int msg_length = 0;						// 此变量中保存已经读入数组msg中的字节个数
    _step_t step;

    if (ctx->debug) {						// 如果是debug模式显示当前状态
        if (msg_type == MSG_INDICATION) {
            zlog_debug(c, "等待ZigBee设备发送指令...");
        } else {
            zlog_debug(c, "Waiting for a confirmation...");
        }
    }

    /* Add a file descriptor to the set */
    FD_ZERO(&rfds);							// 将新建的指定文件描述符集清空
    FD_SET(ctx->s, &rfds);				// 将你感兴趣的文件描述符加入该集合,这里的ctx->s对应串口的文件描述符,对于tcp是套接字

    /* We need to analyse the message step by step.  At the first step, we want
     * to reach the function code because all packets contain this
     * information. */
    step = _STEP_FUNCTION;
    length_to_read = ctx->backend->header_length + 2;		// header_length初始值为1,即lenth_to_read当前为2
    																							//

    if (msg_type == MSG_INDICATION) {
        /* Wait for a message, we don't know when the message will be
         * received */
    	/*
    	 * 等待message,即作为从机等待主机的指示,这里不需要等待时间所以将时间指针p_tv = NULL
    	 * p_tv = NULL,是告诉select程序将一直阻塞某个文件描述符改变,这里指串口有数据可读
    	 */
        p_tv = NULL;
//        tv.tv_sec = ctx->response_timeout.tv_sec;
//        tv.tv_usec = ctx->response_timeout.tv_usec;
//        p_tv = &tv;
    } else {
    	/*
    	 * 如果是确认模式时将response回应时间付给tv这个时间变量结构体
    	 * 即在规定的时间内未接收到回应的数据
    	 */
        tv.tv_sec = ctx->response_timeout.tv_sec;
        tv.tv_usec = ctx->response_timeout.tv_usec;
        p_tv = &tv;
    }

    while (length_to_read != 0) {
    	/*ctx->backend->select的函数原型为
    	 * int _modbus_rtu_select(modbus_t *ctx, fd_set *rfds,struct timeval *tv, int length_to_read)
    	 *rc返回值为-1代表其他错误,0代表超时
    	 *rc正常返回相应文件描述符的可读写性
    	 */
        rc = ctx->backend->select(ctx, &rfds, p_tv, length_to_read);
        if (rc == -1) {
            _error_print(ctx, "select");
            if (ctx->error_recovery & MODBUS_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if (errno == ETIMEDOUT) {
                	_sleep_and_flush_zigbee(ctx);
                } else if (errno == EBADF) {				//EBADF文件描述符无效错误
                    modbus_close(ctx);
                    modbus_connect(ctx);
                }
                errno = saved_errno;
            }
            return -1;
        }
        /*
         * 当select()的时间参数设置为0.5秒时,如果在次时间内无可读数据则返回0
         * 在MSG_INDICATION模式下检测返回值是0的话,说明超时,返回0
         * 使用该方式是为了避免时间参数为NULL,即阻塞式
         */
//        if ((msg_type == MSG_INDICATION)&&(rc == 0)) {
//        	return 0;
//        }
        /*
         * 此处可能文件描述符集中只加入了一个文件描述符所以未判断是否该文件描述符是否在集合中
         */
//        if(!FD_ISSET(ctx->s,&rfds))
//        	return -1;
	/*运行到此处时,说明串口文件描述符有可读数据
	 * 通过ctx->backend->recvd读出数据保存到msg指针中,其长度为length_to_read (2)
	 * ctx->backend->recvd函数原型为_modbus_rtu_recv返回值为读到数据的个数
	 * 读出数据应该是每次成功读取一个字节的数据
	 * */
        rc = ctx->backend->recv(ctx, msg + msg_length, length_to_read);
        if (rc == 0) {
            errno = ECONNRESET;
            rc = -1;
        }

        if (rc == -1) {
            _error_print(ctx, "read");
            if ((ctx->error_recovery & MODBUS_ERROR_RECOVERY_LINK) &&
                (errno == ECONNRESET || errno == ECONNREFUSED ||
                 errno == EBADF)) {
                int saved_errno = errno;
                modbus_close(ctx);
                modbus_connect(ctx);
                /* Could be removed by previous calls */
                errno = saved_errno;
            }
            return -1;
        }

        /* Display the hex code of each character received */
//        if (ctx->debug) {
//            int i;
//            printf(">>");
//            for (i=0; i < rc; i++)
////                printf("<%.2X>", msg[msg_length + i]);
//            	printf("%.2X ", msg[msg_length + i]);
//        }

        /* Sums bytes received */
        msg_length += rc;					// msg_length 初始值为0,如果成功接收到了2个字节,即rc = 2 ,就加上rc
        /* Computes remaining bytes */
        length_to_read -= rc;			// 将已经读出数据从length_to_read中减去

        if (length_to_read == 0) {		//length_to_read为0表示成功读出ID和功能码这两个字节
            switch (step) {
            case _STEP_FUNCTION:
                /* Function code position */
            	/*函数compute_meta_length_after_function的作用
            	 * 根据不同模式(msg_type)指示或者确认这两种情况分别对不同功能码(msg[ctx->backend->header_length])
            	 * 所需要继续读出字节数的判定
            	 * */
            	length_to_read = msg[1] * 256 + msg[2];
//                length_to_read = compute_meta_length_after_function_zigbee(
//                    msg[ctx->backend->header_length],					// ctx->backend->header_length值为1 对应msg数组中的功能码
//                    msg_type);
                if (length_to_read != 0) {												// 通过功能码的判定得到length_to_read的值,若非零进入meta元模式
                    step = _STEP_META;													// 继续通过select函数判定文件描述符是否可读
                    break;
                } /* else switches straight to the next step */
            case _STEP_META:															// 程序运行至此说明meta元数据读取完毕,再根据元计算还要读出的数据
            		length_to_read = 1;													// 校验和反码
//                length_to_read = compute_data_length_after_meta_zigbee(
//                    ctx, msg, msg_type);
                /*
                 * 判断已经读出的msg_length+length_to_read的值是否大于设定值ctx->backend->max_adu_length(256)
                 * 若大于最大数据长度256报错,返回-1
                 */
                if ((msg_length + length_to_read) > ctx->backend->max_adu_length) {
                    errno = EMBBADDATA;
                    _error_print(ctx, "too many data");
                    return -1;
                }
                /*
                 * 若已有长度msg_length+再要读出的长度length_to_read未到最大值256,则进入STEP_DATA数据模式
                 * 继续通过select函数判定文件描述符是否可读,并完成所有数据的读出
                 * 此时还 while (length_to_read != 0) 循环中,待数据全部读出length_to_read变为0
                 * 或者报错时退出while循环
                 */
                step = _STEP_DATA;
                break;
            default:
                break;
            }
        }

        if (length_to_read > 0 && ctx->byte_timeout.tv_sec != -1) {
            /* If there is no character in the buffer, the allowed timeout
               interval between two consecutive bytes is defined by
               byte_timeout */
        	/*
        	 * 完成第一次select后,说明接收到到了数据,这是要把select的时间参数tv改为接受每个字节间所需最大时间
        	 * 即为modbus中的分包时间,3.5个字节长
        	 */
            tv.tv_sec = ctx->byte_timeout.tv_sec;
            tv.tv_usec = ctx->byte_timeout.tv_usec;
            p_tv = &tv;
//            p_tv = NULL;
        }
    }

    if (ctx->debug)
        printf("\n");
    /*
     * 程序运行到此说明接受到完整数据包
     * 用ctx->backend->check_integrity函数指针指向函数_modbus_rtu_check_integrity
     * 检查CRC校验和是否正确并将其返回值用return返回
     * 校验正确返回接收到数据的总长度
     * 错误返回-1
     */
//    return ctx->backend->check_integrity(ctx, msg, msg_length);
     return _zigbee_check_integrity(ctx, msg, msg_length);
}
static int _sleep_and_flush_zigbee(modbus_t *ctx)
{
    /* usleep source code */
    struct timespec request, remaining;
    request.tv_sec = ctx->response_timeout.tv_sec;
    request.tv_nsec = ((long int)ctx->response_timeout.tv_usec % 1000000)
        * 1000;
    while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
        request = remaining;
    return modbus_flush(ctx);
}

static uint8_t zigbeeCheck(uint8_t *buf, uint16_t start, uint16_t cnt)
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

static int _zigbee_check_integrity(modbus_t *ctx, uint8_t *msg, const int msg_length)
{
	uint8_t check_calculated;
	uint8_t check_received;

    check_calculated = zigbeeCheck(msg, 3, msg_length - 1);
//    check_received = (msg[msg_length - 2] << 8) | msg[msg_length - 1];
    check_received = msg[msg_length - 1];

    /* Check CRC of msg */
    if (check_calculated == check_received) {
        return msg_length;
    } else {
        if (ctx->debug) {
        	zlog_warn(c, "CRC received %0X != CRC calculated %0X",
            		check_received, check_calculated);
        }
        if (ctx->error_recovery & MODBUS_ERROR_RECOVERY_PROTOCOL) {
//            _modbus_rtu_flush(ctx);
        	tcflush(ctx->s, TCIOFLUSH);
        }
        errno = EMBBADCRC;
        return -1;
    }
}
static int zigbee_reply(modbus_t *ctx,  uint8_t *req, int req_length)//, modbus_mapping_t *mb_mapping)
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
//				res = send_zigbee_msg(ctx, rsp_data, rsp_length);
//				if(res == -1)
//				{
//					printf("[提示]发送请求路由表命令失败!\n");
//					break;
//				}
//				memcpy(rsp_data, AT_ND, sizeof(AT_ND));
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
										res = send_zigbee_msg(ctx, rsp_data, rsp_length);
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

									res = send_zigbee_msg(ctx, rsp_data, rsp_length);
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

	return send_zigbee_msg(ctx, rsp_data, rsp_length);
}
/* Sends a request/response */
static int send_zigbee_msg(modbus_t *ctx, uint8_t *msg, int msg_length)
{
    int rc;
    int i;

//    msg_length = ctx->backend->send_msg_pre(msg, msg_length);			// 计算校验和返回带校验和的长度
    if ((ctx->debug) && (msg_length))
    	printf("<< ");
    if (ctx->debug) {
        for (i = 0; i < msg_length; i++)
        	 printf("%.2X ", msg[i]);
//            printf("[%.2X]", msg[i]);
        printf("\n");
    }

    /* In recovery mode, the write command will be issued until to be
       successful! Disabled by default. */
    do {
        rc = ctx->backend->send(ctx, msg, msg_length);
        if (rc == -1) {
            _error_print(ctx, NULL);
            if (ctx->error_recovery & MODBUS_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if ((errno == EBADF || errno == ECONNRESET || errno == EPIPE)) {
                    modbus_close(ctx);
                    modbus_connect(ctx);
                } else {
                	_sleep_and_flush_zigbee(ctx);
                }
                errno = saved_errno;
            }
        }
    } while ((ctx->error_recovery & MODBUS_ERROR_RECOVERY_LINK) &&
             rc == -1);

    if (rc > 0 && rc != msg_length) {
        errno = EMBBADDATA;
        return -1;
    }

    return rc;
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
//	memset(rsp_data, 0, sizeof(rsp_data));
//	ZB_11 = (ZB_explicit_cmd_frame *)rsp_data;
//	ZB_91 = (ZB_explicit_RX_indicator *)req_data;
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
//	ZB_11->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
//
//	return data_length;
//}
/*@brief
 * wsf
 * 常规数据应答函数
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 */
inline int conventionalDataRespone(unsigned short int sleeptime)
{
	ZB_explicit_RX_indicator *ZB_91;
	ZB_explicit_cmd_frame *ZB_11;
	int data_length = 0;
	memset(rsp_data, 0, sizeof(rsp_data));
	ZB_11 = (ZB_explicit_cmd_frame *)rsp_data;
	ZB_91 = (ZB_explicit_RX_indicator *)req_data;
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
	ZB_11->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
//				rsp_length -= 2;																						// 参数应答
//				rsp_data[33] = zigbeeCheck(rsp_data,3,(rsp_length - 1));					// 参数应答
//				usleep(200);
	return data_length;
}
/*@brief
 * wsf
 * G10一体化功图参数写应答帧 开始采集命令
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 */
static int collectDynagraphRespone(data_exchange *datex)
{
	ZB_explicit_RX_indicator *ZB_91;
	A11_rsp_collect_dynagraph *collect;
	int data_length = 0;
	memset(rsp_data, 0, sizeof(rsp_data));
	ZB_91 = (ZB_explicit_RX_indicator *)req_data;
	collect = (A11_rsp_collect_dynagraph *)rsp_data;

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
	collect->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));

	return data_length;
}

/*@brief
 * wsf
 * G13 电参参数写应答帧 开始采集命令
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 */
static int collectElecRespone(data_exchange *datex)
{
	A11_rsp_collect_elec *collect;
	int data_length = 0;
	bzero(rsp_data, sizeof(rsp_data));
	collect = (A11_rsp_collect_elec *)rsp_data;

	collect->ZB11_framehead.start_elimiter = 0x7E;
	collect->ZB11_framehead.length = htons(sizeof(A11_rsp_collect_elec) - 4);
	collect->ZB11_framehead.frame_type = 0x11;
	collect->ZB11_framehead.frame_ID = 0x00;																	// 00无ACK  01有ACK
	memcpy(collect->ZB11_framehead.mac_addr, datex->ZB91_framehead.mac_addr, 18);
	collect->ZB11_framehead.radius = 0x00;
	collect->ZB11_framehead.send_opt = 0x00;	// 0x60;

	memcpy(&collect->A11_framehead, &datex->A11_framehead, sizeof(A11_data_framehead));
	collect->A11_framehead.instrument_type = htons(0x1F10);						// 根据A11规范，RTU 应答传感器的帧头中的仪表类型应为 0x1F10
	collect->A11_framehead.data_type = htons(0x0210);									// 根据A11规范，RTU应答电参开始测试该数据类型为0x0210

	collect->mode = datex->dynagraph_mode;//0x10;//
	collect->dot = htons(datex->dot);//0xC800;//
	collect->synchronization_time = htons(datex->synchronization_time);//0x9cF7;//
	collect->time_interval = htons(datex->time_interval);//0xCC04;//
	collect->algorithms = datex->algorithms;//0x00;//

	data_length = sizeof(A11_rsp_collect_elec);
	collect->check_sum = zigbeeCheck(rsp_data, 3, (data_length - 1));
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
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 */
inline int dataGroupRespone(unsigned short int data_type)
{
	A11_req_dynagraph_first *ZB_91;
	A11_rsp_datagroup *rsp_dynagraph;
	int data_length = 0;
	memset(rsp_data, 0, sizeof(rsp_data));
	ZB_91 = (A11_req_dynagraph_first *)req_data;
	rsp_dynagraph = (A11_rsp_datagroup *)rsp_data;

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
	rsp_dynagraph->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
	return data_length;
}
/*@brief
 * wsf
 * G28 电参应答数据帧格式
 * 发送
 * 数据类型：功图0x0201，无线载荷0x0205，无线位移0x0208，电参0x0211，专项数据0x0231
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 */
inline int dataGroupResponeElec(unsigned short int data_type)
{
	A11_req_elec_first *ZB_91;
	A11_rsp_datagroup *rsp_dynagraph;
	int data_length = 0;
	memset(rsp_data, 0, sizeof(rsp_data));
	ZB_91 = (A11_req_elec_first *)req_data;
	rsp_dynagraph = (A11_rsp_datagroup *)rsp_data;

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
	rsp_dynagraph->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
	return data_length;
}
/*@brief
 * wsf
 * G25 读电流图数据命令帧格式
 * 取出req_data中的相关数据以ZB11格式放入rsp_data中
 * 发送
 */
static int readElecRespone(data_exchange *datex)
{
	A11_rsp_currentchart *readcurrent;
	int data_length = 0;
	bzero(rsp_data, sizeof(rsp_data));
//	memset(rsp_data, 0, sizeof(rsp_data));
	readcurrent = (A11_rsp_currentchart *)rsp_data;
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
	readcurrent->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
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

	memset(rsp_data, 0, sizeof(rsp_data));
	ZB_91 = (ZB91_netin_framehead *)req_data;
	ZB_11 = (ZB11_netin_framehead *)rsp_data;

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
	ZB_11->check_sum = zigbeeCheck(rsp_data,3,(data_length - 1));
	return data_length;
}
