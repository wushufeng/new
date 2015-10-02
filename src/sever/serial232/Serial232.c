/*
 * Serial232.c
 *
 *  Created on: 2015年4月13日
 *      Author: wsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "Serial232.h"

#include "../../A11_sysAttr/a11sysattr.h"
#include "../../modbus/modbus.h"
#include "../../database/database.h"
#include "../../modbus/modbus-private.h"
#include "../../log/rtulog.h"
//#include "../../port/portserial.h"
//#include "../../myMB/myMB.h"
#include "../../database/database.h"

#ifdef ARM_32
#define 	SERIAL232			"/dev/ttyS2"
#else
#define 	SERIAL232			"/dev/ttyUSB1"
#endif

extern int mbWriteSigleRegister(uint16_t address, int data);
modbus_t *ctx_serial232;
//comm_t *ctx_232;
pthread_t serial232_thread;
int rc;
int use_backend_serial232;
unsigned char *query_serial232;
int header_length;
unsigned char ttt[] = {0x80, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xDB, 0xDC};
unsigned char snd[260];
//enum {
//    TCP,
//    UDP,
//    RTU
//};

static int serial232ThreadFunc(void *arg);
static int mb_reply(modbus_t *ctx, const uint8_t *req, int req_length, modbus_mapping_t *mb_mapping);
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
//    rc = comm_connect(ctx_232);
    if (rc == -1) {

    	zlog_error(c, "Serial232接口不能连接! %d", errno);
    	modbus_free(ctx_serial232);
    	pthread_exit(0);
//        fprintf(stderr, "[错误]Serial232接口不能连接! %s\n", modbus_strerror(errno));

//        return -1;
    }
    for (;;) {

    	/*
    	 * 函数modbus_receive,判断指示或回应两模式下不同功能码接受的数据
    	 * 返回正确接受的数据,并且该数据通过CRC校验
    	 * 正确返回接收到数据的总长度,所读出的数据放入query_serial232所指向的地址中
    	 * 错误返回-1
    	 */
        rc = modbus_receive(ctx_serial232, query_serial232, 22);
//        rc = mb_read(ctx_232, query_serial232, 0);
        if (rc == -1) {
            /* Connection closed by the client or error */
        	continue;
//            break;
        }
        serial_offset = ctx_serial232->backend->header_length;			// 取出数据头长度默认为1
//        serial_offset = 1;			// 取出数据头长度默认为1
        serial_slave = query_serial232[serial_offset - 1];
        if((serial_slave < 1) ||((serial_slave > 16) && (serial_slave != 128)))
        {
        	zlog_warn(c,"Modbus串口设备ID = %d, 不在范围内ID = 128 || 1 <= ID <= 16 \n", serial_slave);
        	continue;
//        	break;
        }
        if(serial_slave == 128)
        	serial_slave = 0;
//        rc = modbus_reply(ctx_serial232, ttt, 8, (modbus_mapping_t *)mb_mapping[serial_slave]);
//        sleep(2);
//        rc = modbus_reply(ctx_serial232, query_serial232, rc, (modbus_mapping_t *)mb_mapping[serial_slave]);
        rc = mb_reply(ctx_serial232, query_serial232, rc, (modbus_mapping_t *)mb_mapping[serial_slave]);
//        comm_send(ctx_232, snd, rc);
        if (rc == -1) {
        	continue;
//            break;
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
    	zlog_error(c, "创建Serial232线程失败:%d", errno);
//    	fprintf(stderr, "Failed to allocate the query: %s\n", modbus_strerror(errno));
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

//	use_backend_serial232 = RTU;

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
//	ctx_232 = new_comm_t(SERIAL232, baud, parity, data_bit, stop_bit);
    modbus_set_slave(ctx_serial232, SERVER_ID);												// 设置本机地址
//    mb_set_slave(ctx_232, 128);// 128
    query_serial232 = malloc(256);// 开辟256的动态内存空间
    if(query_serial232 == NULL)
    {
    	zlog_warn(c, "Serial232开辟动态内存失败: %d\n",errno);
//    	fprintf(stderr, "Failed to allocate the query: %s\n", modbus_strerror(errno));
//        	perror("Malloc query failed");
    	return -1;
    }

    header_length = modbus_get_header_length(ctx_serial232);
//    header_length = 1;
    modbus_set_debug(ctx_serial232, TRUE);
//    mb_set_debug(ctx_232, 1);
	return 0;
}
int serial232ThreadCancel(void)
{
	int res;
	void *thread_result;
	zlog_info(c, "正在取消Serial232线程");
	res = pthread_cancel(serial232_thread);
	if(res != 0)	{
		zlog_error(c, "Serial232线程取消失败-%d", res);
		exit(EXIT_FAILURE);
	}
	zlog_info(c, "正在等待Serial232线程");
	res = pthread_join(serial232_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待Serial232线程结束失败-%d", res);
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
	{
		free(query_serial232);
		query_serial232 = NULL;
	}
	if(ctx_serial232 != NULL)
		modbus_free(ctx_serial232);
}
/////////////////////////////////////////////////////////////////////////////////////
/// modbus库中得函数//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
static int _modbus_rtu_build_response_basis(sft_t *sft, uint8_t *rsp)
{
    /* In this case, the slave is certainly valid because a check is already
     * done in _modbus_rtu_listen */
    rsp[0] = sft->slave;
    rsp[1] = sft->function;

    return 2;
}
/* Build the exception response */
static int response_exception(modbus_t *ctx, sft_t *sft,
                              int exception_code, uint8_t *rsp)
{
    int rsp_length;

    sft->function = sft->function + 0x80;
    rsp_length = _modbus_rtu_build_response_basis(sft, rsp);

    /* Positive exception code */
    rsp[rsp_length++] = exception_code;

    return rsp_length;
}
static int mb_exception_rsp(modbus_t *ctx, sft_t *sft, int exception_code, uint8_t *rsp)
{
    int rsp_length;

    sft->function = sft->function + 0x80;
    rsp_length = _modbus_rtu_build_response_basis(sft, rsp);

    /* Positive exception code */
    rsp[rsp_length++] = exception_code;
	return rsp_length;
}
static int response_io_status(int address, int nb,
                              uint8_t *tab_io_status,
                              uint8_t *rsp, int offset)
{
    int shift = 0;
    int byte = 0;
    int i;

    for (i = address; i < address+nb; i++) {
        byte |= tab_io_status[i] << shift;
        if (shift == 7) {
            /* Byte is full */
            rsp[offset++] = byte;
            byte = shift = 0;
        } else {
            shift++;
        }
    }

    if (shift != 0)
        rsp[offset++] = byte;

    return offset;
}
static int sleep_and_flush(modbus_t *ctx)
{
    /* usleep source code */
    struct timespec request, remaining;
    request.tv_sec = ctx->response_timeout.tv_sec;
    request.tv_nsec = ((long int)ctx->response_timeout.tv_usec % 1000000)
        * 1000;
    while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
        request = remaining;
    //    return modbus_flush(ctx);
    int rc = ctx->backend->flush(ctx);
    if (rc != -1 && ctx->debug) {
        printf("%d bytes flushed\n", rc);
    }
    return rc;

}
/* Sends a request/response */
static int send_msg(modbus_t *ctx, uint8_t *msg, int msg_length)
{
    int rc;
    int i;

    msg_length = ctx->backend->send_msg_pre(msg, msg_length);			// 计算校验和返回带校验和的长度

    if (ctx->debug) {
        for (i = 0; i < msg_length; i++)
            printf("[%.2X]", msg[i]);
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
                    sleep_and_flush(ctx);
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
static int mb_reply(modbus_t *ctx, const uint8_t *req,
                 int req_length, modbus_mapping_t *mb_mapping)
{
    int offset = ctx->backend->header_length;			// 取出数据头长度默认为1
    int slave = req[offset - 1];										// 从输入参数指针req中取出SlaveID
    int function = req[offset];										// 取出功能码
    uint16_t address = (req[offset + 1] << 8) + req[offset + 2];	// 计算出两字节起始地址
    uint8_t rsp[260];				// 申请长度为260的用于返回数据的数组rsp
    int rsp_length = 0;													// 设定数组rsp游标rsp_length为0
    sft_t sft;
    /*
     * ctx->backend->filter_request指向函数_modbus_rtu_filter_request
     */
//    if (ctx->backend->filter_request(ctx, slave) == 1) {
//        /* Filtered */
//        return 0;
//    }
    /* @brief
     * Wusf
     * 若salveID不是本机返回0
     */
//    if(slave != ctx->slave )
//    	return 0;
    sft.slave = slave;
    sft.function = function;
    sft.t_id = ctx->backend->prepare_response_tid(req, &req_length);

    switch (function) {
//    case _FC_READ_COILS: {
//        int nb = (req[offset + 3] << 8) + req[offset + 4];
//
//        if (nb < 1 || MODBUS_MAX_READ_BITS < nb) {
//            if (ctx->debug) {
//            	zlog_debug(c, "非法得读取数据个数%d最大%d", nb, MODBUS_MAX_READ_BITS);
////                fprintf(stderr,
////                        "Illegal nb of values %d in read_bits (max %d)\n",
////                        nb, MODBUS_MAX_READ_BITS);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
//        } else if ((address + nb) > mb_mapping->nb_bits) {
//            if (ctx->debug) {
//            	zlog_debug(c, "读bits位非法地址%0x", address + nb);
////                fprintf(stderr, "Illegal data address %0X in read_bits\n",
////                        address + nb);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
//        } else {
//            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
//            rsp[rsp_length++] = (nb / 8) + ((nb % 8) ? 1 : 0);
//            rsp_length = response_io_status(address, nb,
//                                            mb_mapping->tab_bits,
//                                            rsp, rsp_length);
//        }
//    }
//        break;
//    case _FC_READ_DISCRETE_INPUTS: {
//        /* Similar to coil status (but too many arguments to use a
//         * function) */
//        int nb = (req[offset + 3] << 8) + req[offset + 4];
//
//        if (nb < 1 || MODBUS_MAX_READ_BITS < nb) {
//            if (ctx->debug) {
//            	zlog_debug(c, "读输入bits非法数量%d(最大数量 %d)", nb, MODBUS_MAX_READ_BITS);
////                fprintf(stderr,
////                        "Illegal nb of values %d in read_input_bits (max %d)\n",
////                        nb, MODBUS_MAX_READ_BITS);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
//        } else if ((address + nb) > mb_mapping->nb_input_bits) {
//            if (ctx->debug) {
//            	zlog_debug(c, "读输入bits非法地址 %0X", address + nb);
////                fprintf(stderr, "Illegal data address %0X in read_input_bits\n",
////                        address + nb);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
//        } else {
//            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
//            rsp[rsp_length++] = (nb / 8) + ((nb % 8) ? 1 : 0);
//            rsp_length = response_io_status(address, nb,
//                                            mb_mapping->tab_input_bits,
//                                            rsp, rsp_length);
//        }
//    }
//        break;
    case _FC_READ_HOLDING_REGISTERS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MODBUS_MAX_READ_REGISTERS < nb) {
            if (ctx->debug) {
            	zlog_debug(c, "读保持寄存器非法数量 %d(最大 %d)", nb ,MODBUS_MAX_READ_REGISTERS);
//                fprintf(stderr,
//                        "Illegal nb of values %d in read_holding_registers (max %d)\n",
//                        nb, MODBUS_MAX_READ_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_registers) {
            if (ctx->debug) {
            	zlog_debug(c, "非法的读寄存器地址 %0X", address + nb);
//                fprintf(stderr, "Illegal data address %0X in read_registers\n",
//                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i;

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = nb << 1;
            for (i = address; i < address + nb; i++) {
                rsp[rsp_length++] = mb_mapping->tab_registers[i] >> 8;
                rsp[rsp_length++] = mb_mapping->tab_registers[i] & 0xFF;
            }
        }
    }
        break;
    /*
    case _FC_DIAGNOSTICS:
//    	int nb = (req[offset + 3] << 8) + req[offset + 4];

        rsp_length = ctx->backend->build_response_basis(&sft, rsp);
        rsp[rsp_length++] = req[offset + 0] ;
        rsp[rsp_length++] = req[offset + 1] ;
 //       for (i = address; i < address + nb; i++) {
            rsp[rsp_length++] = req[offset + 2] ;
            rsp[rsp_length++] = req[offset + 3] ;
		break;
		*/
    case _FC_DIAGNOSTICS:{																// wsf 20150114 添加0x08功能
    	if(address != 0x0000){
    		if(ctx->debug){
    			zlog_debug(c, "非法得诊断功能地址%0X", address);
//                fprintf(stderr, "Illegal data address %0X in diagnostics\n",
//                        address);
    		}
    		 rsp_length = response_exception(
    		                ctx, &sft,
    		                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            memcpy(rsp, req, req_length);
            rsp_length = req_length;
            }
    }
    	break;
    case _FC_READ_INPUT_REGISTERS: {
        /* Similar to holding registers (but too many arguments to use a
         * function) */
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MODBUS_MAX_READ_REGISTERS < nb) {
            if (ctx->debug) {
            	zlog_debug(c, "非法得读输入寄存数量%d(最大%d)", nb , MODBUS_MAX_READ_REGISTERS);
//                fprintf(stderr,
//                        "Illegal number of values %d in read_input_registers (max %d)\n",
//                        nb, MODBUS_MAX_READ_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_input_registers) {
            if (ctx->debug) {
            	zlog_debug(c, "非法得读输入寄存器地址%0X", address + nb);
//                fprintf(stderr, "Illegal data address %0X in read_input_registers\n",
//                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i;

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = nb << 1;
            for (i = address; i < address + nb; i++) {
                rsp[rsp_length++] = mb_mapping->tab_input_registers[i] >> 8;
                rsp[rsp_length++] = mb_mapping->tab_input_registers[i] & 0xFF;
            }
        }
    }
        break;
//    case _FC_WRITE_SINGLE_COIL:
//        if (address >= mb_mapping->nb_bits) {
//            if (ctx->debug) {
//            	zlog_debug(c, "非法的写单线圈地址%0X", address);
////                fprintf(stderr, "Illegal data address %0X in write_bit\n",
////                        address);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
//        } else {
//            int data = (req[offset + 3] << 8) + req[offset + 4];
//
//            if (data == 0xFF00 || data == 0x0) {
//                mb_mapping->tab_bits[address] = (data) ? ON : OFF;
//                memcpy(rsp, req, req_length);
//                rsp_length = req_length;
//            } else {
//                if (ctx->debug) {
//                	zlog_debug(c, "写地址是%0X的bit位得非法数值%0X", address, data);
////                    fprintf(stderr,
////                            "Illegal data value %0X in write_bit request at address %0X\n",
////                            data, address);
//                }
//                rsp_length = response_exception(
//                    ctx, &sft,
//                    MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
//            }
//        }
//        break;
    case _FC_WRITE_SINGLE_REGISTER:
    {
    	int data = (req[offset + 3] << 8) + req[offset + 4];
    	switch(mbWriteSigleRegister(address, data))
    	{
    		case 0x02:	// 地址错误
    			rsp_length = mb_exception_rsp(ctx, &sft,0x02, rsp);
    			if (ctx->debug) {
    				zlog_debug(c, "0x06功能码不支持该地址%0X的写入", address);
    			}
    			break;
    		case 0x03:	//寄存器值错误
    			if (ctx->debug) {
    				zlog_debug(c, "0x06功能码写入地址%0X的值非法", address);
    			}
    			rsp_length = mb_exception_rsp(ctx, &sft,0x03, rsp);
    			break;
    		case 4:
    			zlog_warn(c, "0x06功能码写入地址%0X的值%d存入配置文件失败",address, data);
    			break;
    		default:
                mb_mapping->tab_registers[address] = data;
    			break;
      	}
		memcpy(rsp, req, req_length);
		rsp_length = req_length;
        break;
    }
//    case _FC_WRITE_MULTIPLE_COILS: {
//        int nb = (req[offset + 3] << 8) + req[offset + 4];
//
//        if (nb < 1 || MODBUS_MAX_WRITE_BITS < nb) {
//            if (ctx->debug) {
//            	zlog_debug(c, "写非法得写bit位数量%d(最大%d)", nb, MODBUS_MAX_WRITE_BITS);
////                fprintf(stderr,
////                        "Illegal number of values %d in write_bits (max %d)\n",
////                        nb, MODBUS_MAX_WRITE_BITS);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
//        } else if ((address + nb) > mb_mapping->nb_bits) {
//            if (ctx->debug) {
//            	zlog_debug(c, "非法的写bit位地址%0X", address + nb);
////                fprintf(stderr, "Illegal data address %0X in write_bits\n",
////                        address + nb);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
//        } else {
//            /* 6 = byte count */
//            modbus_set_bits_from_bytes(mb_mapping->tab_bits, address, nb, &req[offset + 6]);
//
//            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
//            /* 4 to copy the bit address (2) and the quantity of bits */
//            memcpy(rsp + rsp_length, req + rsp_length, 4);
//            rsp_length += 4;
//        }
//    }
//        break;
    case _FC_WRITE_MULTIPLE_REGISTERS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MODBUS_MAX_WRITE_REGISTERS < nb) {
            if (ctx->debug) {
            	zlog_debug(c, "非法的写寄存器数量在%d(最大%d)", nb, MODBUS_MAX_WRITE_REGISTERS);
//                fprintf(stderr,
//                        "Illegal number of values %d in write_registers (max %d)\n",
//                        nb, MODBUS_MAX_WRITE_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_registers) {
            if (ctx->debug) {
            	zlog_debug(c, "非法的写寄存器地址%0X", address + nb);
//                fprintf(stderr, "Illegal data address %0X in write_registers\n",
//                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
        	/*
        	 * TODO
        	 * 增加对10功能码支持
        	 */
            int i, j;
            for (i = address, j = 6; i < address + nb; i++, j += 2) {
                /* 6 and 7 = first value */
                mb_mapping->tab_registers[i] =
                    (req[offset + j] << 8) + req[offset + j + 1];
            }

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            /* 4 to copy the address (2) and the no. of registers */
            memcpy(rsp + rsp_length, req + rsp_length, 4);
            rsp_length += 4;
        }
    }
        break;
    case _FC_REPORT_SLAVE_ID: {
        int str_len;
        int byte_count_pos;

        rsp_length = ctx->backend->build_response_basis(&sft, rsp);
        /* Skip byte count for now */
        byte_count_pos = rsp_length++;
        rsp[rsp_length++] = _REPORT_SLAVE_ID;
        /* Run indicator status to ON */
        rsp[rsp_length++] = 0xFF;
        /* LMB + length of LIBMODBUS_VERSION_STRING */
        str_len = 3 + strlen(LIBMODBUS_VERSION_STRING);
        memcpy(rsp + rsp_length, "LMB" LIBMODBUS_VERSION_STRING, str_len);
        rsp_length += str_len;
        rsp[byte_count_pos] = rsp_length - byte_count_pos - 1;
    }
        break;
//    case _FC_READ_EXCEPTION_STATUS:
//        if (ctx->debug) {
//            fprintf(stderr, "FIXME Not implemented\n");
//        }
//        errno = ENOPROTOOPT;
//        return -1;
//        break;
//
//    case _FC_WRITE_AND_READ_REGISTERS: {
//        int nb = (req[offset + 3] << 8) + req[offset + 4];
//        uint16_t address_write = (req[offset + 5] << 8) + req[offset + 6];
//        int nb_write = (req[offset + 7] << 8) + req[offset + 8];
//        int nb_write_bytes = req[offset + 9];
//
//        if (nb_write < 1 || MODBUS_MAX_RW_WRITE_REGISTERS < nb_write ||
//            nb < 1 || MODBUS_MAX_READ_REGISTERS < nb ||
//            nb_write_bytes != nb_write * 2) {
//            if (ctx->debug) {
//                fprintf(stderr,
//                        "Illegal nb of values (W%d, R%d) in write_and_read_registers (max W%d, R%d)\n",
//                        nb_write, nb,
//                        MODBUS_MAX_RW_WRITE_REGISTERS, MODBUS_MAX_READ_REGISTERS);
//            }
//            rsp_length = response_exception(
//                ctx, &sft,
//                MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
//        } else if ((address + nb) > mb_mapping->nb_registers ||
//                   (address_write + nb_write) > mb_mapping->nb_registers) {
//            if (ctx->debug) {
//                fprintf(stderr,
//                        "Illegal data read address %0X or write address %0X write_and_read_registers\n",
//                        address + nb, address_write + nb_write);
//            }
//            rsp_length = response_exception(ctx, &sft,
//                                            MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
//        } else {
//            int i, j;
//            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
//            rsp[rsp_length++] = nb << 1;
//
//            /* Write first.
//               10 and 11 are the offset of the first values to write */
//            for (i = address_write, j = 10; i < address_write + nb_write; i++, j += 2) {
//                mb_mapping->tab_registers[i] =
//                    (req[offset + j] << 8) + req[offset + j + 1];
//            }
//
//            /* and read the data for the response */
//            for (i = address; i < address + nb; i++) {
//                rsp[rsp_length++] = mb_mapping->tab_registers[i] >> 8;
//                rsp[rsp_length++] = mb_mapping->tab_registers[i] & 0xFF;
//            }
//        }
//    }
//        break;

    default:
        rsp_length = response_exception(ctx, &sft,
                                        MODBUS_EXCEPTION_ILLEGAL_FUNCTION,
                                        rsp);
        break;
    }

    return send_msg(ctx, rsp, rsp_length);
}
