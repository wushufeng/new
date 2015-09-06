/*
 * portserial.c
 *
 *  Created on: Sep 2, 2015
 *      Author: wsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "portserial.h"

#include "../log/rtulog.h"


// new一个comm_t结构体
comm_t* new_comm_t(const char *device,
        int baud, char parity, int data_bit,
        int stop_bit)
{
	comm_t *ctx;
	comm_opt *ctx_opt;
	size_t dest_size;
	size_t ret_size;

	ctx = (comm_t *) malloc(sizeof(comm_t));
	init_comm_t(ctx);

//	ctx->backend = &_modbus_rtu_backend;							// backend 包含数据操作函数
	ctx->backend_data = (comm_opt *) malloc(sizeof(comm_opt));		// backend_data 包含串口配置结构体
	ctx_opt = (comm_opt *)ctx->backend_data;

	// 将设备放入ctx->device中
	dest_size = sizeof(ctx_opt->device);

//	ret_size = strlcpy(ctx_gprs->device, device, dest_size);
	strncpy(ctx_opt->device, device, sizeof(ctx_opt->device));
	ret_size = strlen(ctx_opt->device);

	if (ret_size == 0) {
		zlog_error(c, "设备路径字符串为空");
		comm_free(ctx);
		errno = EINVAL;
		return NULL;
	}

	if (ret_size >= dest_size) {
		zlog_error(c, "设备路径字符串被截断");
		comm_free(ctx);
		errno = EINVAL;
		return NULL;
	}

	ctx_opt->baud = baud;
	if (parity == 'N' || parity == 'E' || parity == 'O') {
		ctx_opt->parity = parity;
	} else {
		comm_free(ctx);
		errno = EINVAL;
		return NULL;
	}
	ctx_opt->data_bit = data_bit;
	ctx_opt->stop_bit = stop_bit;
//	ctx_opt->port = port;
//	strncpy(ctx_opt->ip, ip, sizeof(ctx_opt->ip));
//	ret_size = strlen(ctx_opt->device);
//
//	if (ret_size == 0) {
//		zlog_error(c, "主站ip字符串为空");
//		comm_free(ctx);
//		errno = EINVAL;
//		return NULL;
//	}
//	if (ret_size >= dest_size) {
//		zlog_error(c, "主站ip字符串被截断");
//		comm_free(ctx);
//		errno = EINVAL;
//		return NULL;
//	}
	return ctx;
}
// 初始化comm_t结构体
void init_comm_t(comm_t *ctx)
{
    /* Slave and socket are initialized to -1 */
    ctx->slave = -1;
    ctx->s = -1;

    ctx->debug = 0;
    ctx->error_recovery = 0;

    ctx->response_timeout.tv_sec = 0;
    ctx->response_timeout.tv_usec = 500000;	// (0.5s)

    ctx->byte_timeout.tv_sec = 0;
    ctx->byte_timeout.tv_usec = 500000;	// (0.5s)
}

/* Sets up a serial port for GPRS communications */
int comm_connect(comm_t *ctx)
{
//	int bStatus = 0;
//	comm_opt *ctx_opt = ctx->backend_data;
//	struct termios xNewTIO;
//	speed_t xNewSpeed;
//
//	if((ctx->s = open(ctx_opt->device, O_RDWR | O_NOCTTY)) < 0)
//		zlog_error(c, "Can't open serial port %s: %s",ctx_opt->device, strerror(errno));
//	else if (tcgetattr(ctx->s, &ctx_opt->old_tios) != 0)
//		zlog_error(c, "Can't get settings from port %s: %s", ctx_opt->device, strerror(errno));
//	else
//	{
//		bzero(&xNewTIO, sizeof(struct termios));
//
//        switch ( ctx_opt->parity )
//        {
//        case 'N':
//        	xNewTIO.c_cflag &=~ PARENB;
//            break;
//        case 'E':
//            xNewTIO.c_cflag |= PARENB;
//            break;
//        case 'O':
//            xNewTIO.c_cflag |= PARENB | PARODD;
//            break;
//        default:
//            bStatus = 1;
//        }
//        switch ( ctx_opt->data_bit )
//        {
//        case 8:
//            xNewTIO.c_cflag |= CS8;
//            break;
//        case 7:
//            xNewTIO.c_cflag |= CS7;
//            break;
//        default:
//            bStatus = 1;
//        }
//        switch ( ctx_opt->baud )
//        {
//        case 9600:
//            xNewSpeed = B9600;
//            break;
//        case 19200:
//            xNewSpeed = B19200;
//            break;
//        case 38400:
//            xNewSpeed = B38400;
//            break;
//        case 57600:
//            xNewSpeed = B57600;
//            break;
//        case 115200:
//            xNewSpeed = B115200;
//            break;
//        default:
//            bStatus = 1;
//        }
//        if( bStatus )
//        {
//            if( cfsetispeed( &xNewTIO, xNewSpeed ) != 0 )
//            {
//            	zlog_error(c, "Can't set baud rate %d for port %s: %s\n",
//            			ctx_opt->baud, ctx_opt->device, strerror( errno ) );
//            }
//            else if( cfsetospeed( &xNewTIO, xNewSpeed ) != 0 )
//            {
//            	zlog_error(c, "Can't set baud rate %d for port %s: %s\n",
//                		ctx_opt->baud, ctx_opt->device, strerror( errno ) );
//            }
//            else if( tcsetattr( ctx->s, TCSANOW, &xNewTIO ) != 0 )
//            {
//            	zlog_error(c, "Can't set settings for port %s: %s\n",
//            			ctx_opt->device, strerror( errno ) );
//            }
//            else
//            {
//
//                bStatus = 0;
//            }
//        }
//	}


    struct termios tios;
    speed_t speed;

    comm_opt *ctx_opt = ctx->backend_data;

//    if (ctx->debug)
    {
//        printf("Opening %s at %d bauds (%c, %d, %d)\n",
//        		ctx_gprs->device, ctx_gprs->baud, ctx_gprs->parity,
//				ctx_gprs->data_bit, ctx_gprs->stop_bit);
        zlog_debug(c, "打开串口 %s 工作在 %d 波特率 (%c, %d, %d)",
        		ctx_opt->device, ctx_opt->baud, ctx_opt->parity,
				ctx_opt->data_bit, ctx_opt->stop_bit);
    }

    /* The O_NOCTTY flag tells UNIX that this program doesn't want
       to be the "controlling terminal" for that port. If you
       don't specify this then any input (such as keyboard abort
       signals and so forth) will affect your process

       Timeouts are ignored in canonical input mode or when the
       NDELAY option is set on the file via open or fcntl */
    ctx->s = open(ctx_opt->device, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
//    ctx->s = open(ctx_gprs->device, O_RDWR | O_NOCTTY);
    if (ctx->s == -1) {
//        fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
//        		ctx_gprs->device, strerror(errno));
        zlog_error(c, "GPRS不能打开设备端口 %s (%s)",
        		ctx_opt->device, strerror(errno));
        return -1;
    }

    /* Save */
    tcgetattr(ctx->s, &(ctx_opt->old_tios));

    bzero(&tios, sizeof(struct termios));

    /* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    switch (ctx_opt->baud) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        if (ctx->debug) {
//            fprintf(stderr,
//                    "WARNING Unknown baud rate %d for %s (B9600 used)\n",
//					ctx_gprs->baud, ctx_gprs->device);
            zlog_warn(c, "未知的波特率 %d 在 %s (B9600 被使用)\n",
            		ctx_opt->baud, ctx_opt->device);
        }
    }

    /* Set the baud rate */
    if ((cfsetispeed(&tios, speed) < 0) ||
        (cfsetospeed(&tios, speed) < 0)) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }

    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (ctx_opt->data_bit) {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (ctx_opt->stop_bit == 1)
        tios.c_cflag &=~ CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (ctx_opt->parity == 'N') {
        /* None */
        tios.c_cflag &=~ PARENB;
    } else if (ctx_opt->parity == 'E') {
        /* Even */
        tios.c_cflag |= PARENB;
        tios.c_cflag &=~ PARODD;
    } else {
        /* Odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (ctx_opt->parity == 'N') {
        /* None */
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &=~ OPOST;

    /* C_CC         Control characters
       VMIN         Minimum number of characters to read
       VTIME        Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
    */
    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->s, TCSANOW, &tios) < 0) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }
    tcflush(ctx->s, TCIFLUSH);	// 清串口输入缓存
    tcflush(ctx->s, TCOFLUSH);	// 清串口输出缓存
    return 0;
}
void comm_close(comm_t *ctx)
{
    if (ctx == NULL)
        return;
	/* Closes the file descriptor in RTU mode */
    comm_opt *ctx_gprs = ctx->backend_data;

	tcsetattr(ctx->s, TCSANOW, &(ctx_gprs->old_tios));
	close(ctx->s);
}

void comm_free(comm_t *ctx)
{
    if (ctx == NULL)
        return;

    free(ctx->backend_data);
    free(ctx);
}

void comm_set_debug(comm_t *ctx, int boolean)
{
    ctx->debug = boolean;
}
int comm_set_slave(comm_t *ctx, int slave)
{
    /* Broadcast address is 0 (MODBUS_BROADCAST_ADDRESS) */
    if (slave >= 0 && slave <= 247) {
        ctx->slave = slave;
    } else {
        errno = EINVAL;
        return -1;
    }
    return 0;
}
inline int serialSend(comm_t *ctx, char *fmt,...)
{
	char *buffer;
	int res = 0;
	va_list ap;
	buffer = (char *)malloc(4096);
	bzero(buffer, 4096);
	va_start(ap,fmt);
	vsprintf(buffer,fmt,ap);
	va_end(ap);

	res = write(ctx->s,buffer,strlen(buffer));
	if (res != strlen(buffer))
	{
		tcflush(ctx->s,TCOFLUSH);
		res = -1;
	}

	if(buffer != NULL)
		free(buffer);
	return res;
}

inline int comm_send(comm_t *ctx, char *req, int req_length)
{
	int ret;
//	tcflush(ctx->s,TCOFLUSH);
	ret = write(ctx->s,req,req_length);
	if (req_length == ret ){
//		tcflush(ctx->s, TCIFLUSH);	// 清串口输入缓存
		return ret;
	} else {
		tcflush(ctx->s,TCOFLUSH);
		return -1;
	}
}

inline int comm_read(comm_t *ctx, char *rsp, int rsp_len, int timeout)
{
    unsigned char   bResult = 0;
    ssize_t         res;
    fd_set          rfds;
    struct timeval  tv;
    int bytesread;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO( &rfds );
    FD_SET( ctx->s, &rfds );

    /* Wait until character received or timeout. Recover in case of an
     * interrupted read system call. */
    do
    {
//        while ((res = select(ctx->s+1, &rfds, NULL, NULL, &tv)) == -1) 	// 如果tv = NULL为阻塞：select将一直被阻塞
//        {
//            if (errno == EINTR)
//            {																		// EINTR:read 由于信号中断,没读到任何数据
//                FD_ZERO( &rfds );
//                FD_SET( ctx->s, &rfds );
//            }
//            else
//            {
//                break;
//            }
//        }
    	res = select( ctx->s + 1, &rfds, NULL, NULL, &tv );
    	if( res == -1 )
        {
            if( errno != EINTR )		// Interrupted system call
            {
                bResult = -1;
                bytesread = -1;
                zlog_error(c, "GPRS端口中select错误:%d", errno);
            }
        }
    	if(res == 0)
    	{
    		printf("select time out\n");
//    		tv.tv_sec = timeout;
            bResult = -1;
            bytesread = 0;
    	}
        else if( FD_ISSET( ctx->s, &rfds ) )
        {
            if( ( res = read( ctx->s, rsp, rsp_len ) ) == -1 )
            {
                bResult = -1;
                bytesread = -1;
            }
            else
            {
                bytesread = ( int ) res;
                break;
            }
        }
        else
        {
        	bytesread = 0;
            break;
        }
    }
	while( bResult == 0 );
	return bytesread;
//	fd_set fds;
//	FD_ZERO(&fds);
//	FD_SET(ctx->s, &fds);
//	struct timeval tv = {0, 1000*timeout};
//	int total = rsp_len;
//	jsq=0;
//	char* startRev = rsp;
//	while(total > 0)
//	{
//		int res = select(ctx->s+1, &fds, NULL, NULL, &tv);
//		tv.tv_usec = 1000*timeout;
//		if(res == -1)
//		{
//			if(errno == EINTR)		/* Interrupted system call */
//			continue;
//			else
//			{
//			printf("socket error %d with select", errno);
//			return -1;
//		  }
//	}
//	else if(res == 0)
//	{
//	   jsq=rsp_len - total;
//	   if(jsq>1024)
//	   {  jsq=0;}
//	   rsp[jsq] = '\0';
//	//	   memcpy(buff1,rsp,jsq); // copy param
//	   printf("res = 0, 读串口: %s\n", rsp);
//	  return rsp_len - total;
//	}
//
//	if(FD_ISSET(ctx->s, &fds))
//	{
//	  int rs = read(ctx->s, startRev, rsp_len);
//	  if(rs < 0)
//	  {
//		rsp[rsp_len - total] = '\0';
//	//		perror("readComm:");
//		printf("res = 1, 读串口: %s\n", rsp);
//		return -1;
//	  }
//	  total -= rs;
//	  startRev += rs;
//	}
//	}
//	return rsp_len - total;

//	fd_set fds;
//	FD_ZERO(&fds);
//	FD_SET(ctx->s, &fds);
//	struct timeval tv;
//	int read_len;
////	int total = rsp_len;
//	jsq=0;
////	char* startRev = rsp;
//
//	tv.tv_sec = 0;
//	tv.tv_usec = timeout * 1000;
////	while(total > 0)
////	{
//		int res = select(ctx->s+1, &fds, NULL, NULL, &tv);
//		if(res == -1)
//		{
////		  if(errno == EINTR)			// 系统中断
////			continue;
////		  else
////		  {
//			zlog_error(c, "select错误 %d", errno);
//			return -1;
////		  }
//		}
//		else if(res == 0)
//		{
//		   jsq=rsp_len - total;
//		   if(jsq>1024)
//		   {  jsq=0;}
//		   rsp[jsq] = '\0';
//		   memcpy(gprs_req_data,rsp,jsq); // copy param
//		   printf("readComm: %s\n", rsp);
//		  return rsp_len - total;
//		}
//
//		if(FD_ISSET(ctx->s, &fds))			// 判断描述符fd是否在给定的描述符集fdset中
//		{
//			read_len = read(ctx->s, rsp, rsp_len);
//			if(read_len < 0)
//			{
//				rsp[0] = '\0';
//			}
//
//		}
////	}
//	return read_len;
}
//inline int comm_read(comm_t *ctx, char *rsp, int rsp_len, int timeout)
int SerialRead(comm_t *ctx, char * pucBuffer, unsigned short int usNBytes, unsigned short int *usNBytesRead, int tm_ms )
{
    int            bResult = 0;
    size_t         res;
    fd_set          rfds;
    struct timeval  tv;
    tm_ms *= 10;
    tv.tv_sec = tm_ms / 1000;
    tv.tv_usec = (tm_ms % 1000) * 1000;
    FD_ZERO( &rfds );
    FD_SET( ctx->s, &rfds );

    /* Wait until character received or timeout. Recover in case of an
     * interrupted read system call. */
    do
    {
        if( select( ctx->s + 1, &rfds, NULL, NULL, &tv ) == -1 )
        {
            if( errno != EINTR )
            {
                bResult = 1;
            }
        }
        else if( FD_ISSET( ctx->s, &rfds ) )
        {
            if( ( res = read( ctx->s, pucBuffer, usNBytes ) ) == -1 )
            {
            	// FIXME 读串口错误室,是否关闭文件描述符
//            	comm_close(ctx->s);
            	bResult = 1;
            }
            else
            {
                *usNBytesRead = res;
                break;
            }
        }
        else
        {
            *usNBytesRead = 0;
            break;
        }
    }
    while( bResult == 0 );
    return bResult;
}

