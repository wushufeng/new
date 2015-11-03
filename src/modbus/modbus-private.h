/*
 * Copyright © 2010-2012 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _MODBUS_PRIVATE_H_
#define _MODBUS_PRIVATE_H_

#ifndef _MSC_VER
# include <stdint.h>
# include <sys/time.h>
#else
# include "stdint.h"
# include <time.h>
typedef int ssize_t;
#endif
#include <sys/types.h>
//#include "./config.h"

#include "modbus.h"

MODBUS_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
 */
#define _MIN_REQ_LENGTH 12

#define _REPORT_SLAVE_ID 180

#define _MODBUS_EXCEPTION_RSP_LENGTH 5

/* Timeouts in microsecond (0.5 s) */
#define _RESPONSE_TIMEOUT    500000
#define _BYTE_TIMEOUT        500000

/* Function codes */
#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_DIAGNOSTICS				  0x08
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_AND_READ_REGISTERS  0x17

typedef enum {
    _MODBUS_BACKEND_TYPE_RTU=0,
    _MODBUS_BACKEND_TYPE_TCP
} modbus_bakend_type_t;

/* This structure reduces the number of params in functions and so
 * optimizes the speed of execution (~ 37%). */
typedef struct _sft {
    int slave;
    int function;
    int t_id;
} sft_t;

typedef struct _modbus_backend {
    unsigned int backend_type;																					// 后台类型 0:RTU;1:TCP
    unsigned int header_length;																					// 标头长度 取默认值 1
    unsigned int checksum_length;																				// 校验和长度 这里是CRC
    unsigned int max_adu_length;																				// modbus协议最大长度 默认值256
    int (*set_slave) (modbus_t *ctx, int slave);																// 设定从机ID
    int (*build_request_basis) (modbus_t *ctx, int function, int addr,						// 建立一个请求基础,返回长度值6
                                int nb, uint8_t *req);
    int (*build_response_basis) (sft_t *sft, uint8_t *rsp);												//建立一个回应基础,返回长度值2
    int (*prepare_response_tid) (const uint8_t *req, int *req_length);						// 准备一个回应数量,即数据长度减去2字节CRC校验
    int (*send_msg_pre) (uint8_t *req, int req_length);												// 将要发送的数据加上CRC校验(打包),返回总长度
    ssize_t (*send) (modbus_t *ctx, const uint8_t *req, int req_length);						// 将数据写入串口,返回写入数据的字节个数
    ssize_t (*recv) (modbus_t *ctx, uint8_t *rsp, int rsp_length);									// 从串口读出指定长度数据,返回读入字节个数
    int (*check_integrity) (modbus_t *ctx, uint8_t *msg,												// CRC有效,返回数据长度msg_length
                            const int msg_length);
    int (*pre_check_confirmation) (modbus_t *ctx, const uint8_t *req,						// RTU模式指向NULL,TCP模式有效
                                   const uint8_t *rsp, int rsp_length);
    int (*connect) (modbus_t *ctx);																				// 为RTU通讯建立一个串口
    void (*close) (modbus_t *ctx);																					// 关闭RTU模式中的文件描述符
    int (*flush) (modbus_t *ctx);																						// 清空终端IO未完成的输入/输出请求及数据
    int (*select) (modbus_t *ctx, fd_set *rfds, struct timeval *tv, int msg_length);	// 用select检查串口文件描述符是否可读
    int (*filter_request) (modbus_t *ctx, int slave);														// 检查从机地址是否正确,或是否是广播地址,正确返回0错误返回1
} modbus_backend_t;

struct _modbus {
    /* Slave address */
    int slave;															// 从机地址
    /* Socket or file descriptor */
    int s;																// 文件描述符或者是套接字
    int debug;															// 是否启用调试模式
    int error_recovery;													// 错误恢复
    struct timeval response_timeout;									// 回应超时
    struct timeval byte_timeout;										// 断包超时
    const modbus_backend_t *backend;									// 后台结构体指针
    void *backend_data;
};

void _modbus_init_common(modbus_t *ctx);
void _error_print(modbus_t *ctx, const char *context);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_size);
#endif

MODBUS_END_DECLS

#endif  /* _MODBUS_PRIVATE_H_ */
