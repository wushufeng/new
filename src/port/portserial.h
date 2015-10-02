/*
 * portserial.h
 *
 *  Created on: Sep 2, 2015
 *      Author: wsf
 */

#ifndef SRC_PORT_PORTSERIAL_H_
#define SRC_PORT_PORTSERIAL_H_

#include <time.h>
#include <termios.h>


typedef struct _comm_opt {
    char device[16];
    /* Bauds: 9600, 19200, 57600, 115200, etc */
    int baud;
    /* Data bit */
    unsigned char data_bit;
    /* Stop bit */
    unsigned char stop_bit;
    /* Parity: 'N', 'O', 'E' */
    char parity;
    /* Save old termios settings */
    struct termios old_tios;
//    char ip[16];
//    int port;
} comm_opt;

typedef struct _comm_t {
    /* Slave address */
    int slave;												// 从机地址
    /* Socket or file descriptor */
    int s;													// 文件描述符或者是套接字
    int debug;												// 是否启用调试模式
    int error_recovery;										// 错误恢复
    struct timeval response_timeout;						// 回应超时
    struct timeval byte_timeout;							// 断包超时
//    const modbus_backend_t *backend;						// 后台结构体指针
    void *backend_data;
}comm_t;


//comm_t* new_comm_t(const char *device, int baud, char parity, int data_bit, int stop_bit, const char *ip, int port);
comm_t* new_comm_t(const char *device, int baud, char parity, int data_bit, int stop_bit);
void init_comm_t(comm_t *ctx);
int comm_connect(comm_t *ctx);
void comm_close(comm_t *ctx);
void comm_free(comm_t *ctx);
void comm_set_debug(comm_t *ctx, int boolean);
int comm_set_slave(comm_t *ctx, int slave);
inline int comm_send(comm_t *ctx, unsigned char *req, int req_length);
inline int comm_read(comm_t *ctx, unsigned char *rsp, int rsp_len, int timeout);
//int SerialRead(comm_t *ctx, unsigned char * pucBuffer, unsigned short int usNBytes, unsigned short int *usNBytesRead, int timeout);
inline int serialSend(comm_t *ctx, char *fmt,...);
int mbRead(comm_t *ctx, unsigned char *msg, int msg_type, int length_to_read);
int gprsRead(comm_t *ctx, unsigned char * pucBuffer, unsigned short int usNBytes, unsigned short int *usNBytesRead, int tm_ms );
#endif /* SRC_PORT_PORTSERIAL_H_ */
