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
#include <stdint.h>

#include "portserial.h"

#include "../log/rtulog.h"
#include "../myMB/myMB.h"

static int _mb_select(comm_t *ctx, fd_set *rfds, struct timeval *tv, int length_to_read);
int mb_flush(comm_t *ctx);
static int _sleep_and_flush(comm_t *ctx);
static ssize_t mb_recv(comm_t *ctx, uint8_t *rsp, int rsp_length);
static uint8_t compute_meta_length_after_function(int function, int msg_type);
static int compute_data_length_after_meta(comm_t *ctx, uint8_t *msg, int msg_type);

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

    struct termios tios;
    speed_t speed;

    comm_opt *ctx_opt = ctx->backend_data;

//    if (ctx->debug)
    {
//        printf("Opening %s at %d bauds (%c, %d, %d)\n",
//        		ctx_gprs->device, ctx_gprs->baud, ctx_gprs->parity,
//				ctx_gprs->data_bit, ctx_gprs->stop_bit);
        zlog_info(c, "打开串口 %s 工作在 %d 波特率 (%c, %d, %d)",
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
        zlog_error(c, "不能打开设备端口 %s (%s)",
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
    comm_opt *ctx_opt = ctx->backend_data;
    zlog_info(c, "正在关闭串口:%s", ctx_opt->device);
	tcsetattr(ctx->s, TCSANOW, &(ctx_opt->old_tios));
	close(ctx->s);
//	printf(%d:%s\n",  errno, strerror(errno));
}

void comm_free(comm_t *ctx)
{
    if (ctx == NULL)
        return;
    //FIXME 此处free错误,为解决
    free(ctx->backend_data);
    ctx->backend_data = NULL;
    free(ctx);
    ctx = NULL;
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

inline int comm_send(comm_t *ctx, unsigned char *req, int req_length)
{
	int rc;
	int i;
    if (ctx->debug) {
    	printf("<< ");
        for (i = 0; i < req_length; i++)
            printf("%.2X", req[i]);
        printf("\n");
    }
    /* In recovery mode, the write command will be issued until to be
       successful! Disabled by default. */
    do {
        rc = write(ctx->s, req, req_length);
        if (rc == -1) {
            zlog_warn(c, "write错误-%d:%s", errno, strerror(errno));
            if (ctx->error_recovery & MB_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if ((errno == EBADF || errno == ECONNRESET || errno == EPIPE)) {
                    comm_close(ctx);
                    comm_connect(ctx);
                } else {
                	_sleep_and_flush(ctx);
                }
                errno = saved_errno;
            }
        }
    } while ((ctx->error_recovery & MB_ERROR_RECOVERY_LINK) && rc == -1);

    if (rc > 0 && rc != req_length) {
//        errno = EMBBADDATA;
        return -1;
    }
    else
    	return rc;
//	int ret;
////	tcflush(ctx->s,TCOFLUSH);
//	ret = write(ctx->s,req,req_length);
//	if (req_length == ret ){
////		tcflush(ctx->s, TCIFLUSH);	// 清串口输入缓存
//		return ret;
//	} else {
//		tcflush(ctx->s,TCOFLUSH);
//		return -1;
//	}
}

inline int comm_read(comm_t *ctx, unsigned char *rev, int rev_len, int tm_ms)
{
    int rc;
    fd_set rfds;									// 申请一组文件描述符集合
    struct timeval tv;						// 延时时间结构体
    struct timeval *p_tv;				// 延时时间结构体指针,用于指向此类结构体
//    int length_to_read;
//    int msg_length = 0;					// 此变量中保存已经读入数组msg中的字节个数

    if(tm_ms > 0)	{
    	tm_ms *= 10;
		tv.tv_sec = tm_ms / 1000;
		tv.tv_usec = (tm_ms % 1000) * 1000;
		p_tv = &tv;
    }
    else
    	p_tv = NULL;

    if(ctx->debug) {
    	printf(">[提示]串口等待一个命令...\n");
    }
    /* Add a file descriptor to the set */
    FD_ZERO(&rfds);						// 将新建的指定文件描述符集清空
    FD_SET(ctx->s, &rfds);				// 将你感兴趣的文件描述符加入该集合,这里的ctx->s对应串口的文件描述符,对于tcp是套接字

    do
    {
        while ((rc = select(ctx->s+1, &rfds, NULL, NULL, p_tv)) == -1) {		// 如果tv = NULL为阻塞：select将一直被阻塞
            if (errno == EINTR) {																		// EINTR:read 由于信号中断,没读到任何数据
//                if (ctx->debug) {																			//如果为debug模式即输出调试信息
                	zlog_warn(c, "select()一个非阻塞信号被捕获");
//                }
                /* Necessary after an error */
                FD_ZERO(&rfds);												// 如过出现error应重新将文件描述符集合清零
                FD_SET(ctx->s, &rfds);										// 再把要查询的文件描述符加入集合
            } else {
            	// FIXME可添加对于errno为ETIMEDOUT或EBADF的处理
            	zlog_error(c, "select错误:%d-%s", errno, strerror(errno));
            	// 判断是否需要从错误恢复连接
                if (ctx->error_recovery & MB_ERROR_RECOVERY_LINK) {
                    int saved_errno = errno;

                    if (errno == ETIMEDOUT) {
                        _sleep_and_flush(ctx);
                    } else if (errno == EBADF) {				//EBADF文件描述符无效错误
                    	comm_close(ctx);
                    	comm_connect(ctx);
                    }
                    errno = saved_errno;
                }
                return -1;
            }
        }
        if (rc == 0)	//select超时并返回
        {
//        	printf("%d\n",errno);
        	return 0;
        }
        if( FD_ISSET( ctx->s, &rfds ) )			// 判断是否是ctx->s可读
        {
			if((rc = read(ctx->s, rev, rev_len)) == -1 )
			{
				zlog_error(c, "read错误:%d-%s", errno, strerror(errno));
				// 判断是否需要从错误恢复连接
	            if ((ctx->error_recovery & MB_ERROR_RECOVERY_LINK) &&
	                (errno == ECONNRESET || errno == ECONNREFUSED ||
	                 errno == EBADF)) {
	                int saved_errno = errno;
	                comm_close(ctx);
	                comm_connect(ctx);
	                /* Could be removed by previous calls */
	                errno = saved_errno;
	            }
				return -1;
			}
			else
				return rc;
        }
    }while(1);
//	return rc;
}

int mbRead(comm_t *ctx, unsigned char *msg, int msg_type, int length_to_read)
/*
 * 指针msg用于保存接收到的数据首地址
 */
{
    int rc = 0;
    fd_set rfds;									// 申请一组文件描述符集合
    struct timeval tv;						// 延时时间结构体
    struct timeval *p_tv;				// 延时时间结构体指针,用于指向此类结构体
    int msg_length = 0;					// 此变量中保存已经读入数组msg中的字节个数
    int step;
    comm_opt *opt = (comm_opt *)ctx->backend_data;

    if (ctx->debug) {						// 如果是debug模式显示当前状态
        if (msg_type == 0) {
            printf(">[提示]串口%s等待一个命令...\n", opt->device);
        } else {
            printf(">[提示]串口%s等待一个确认...\n", opt->device);
        }
    }
    /* Add a file descriptor to the set */
    FD_ZERO(&rfds);						// 将新建的指定文件描述符集清空
    FD_SET(ctx->s, &rfds);				// 将你感兴趣的文件描述符加入该集合,这里的ctx->s对应串口的文件描述符,对于tcp是套接字

    /* We need to analyse the message step by step.  At the first step, we want
     * to reach the function code because all packets contain this
     * information. */
    step = 1;

    if (msg_type == 0) {
        /* Wait for a message, we don't know when the message will be
         * received */
    	/*
    	 * 等待message,即作为从机等待主机的指示,这里不需要等待时间所以将时间指针p_tv = NULL
    	 * p_tv = NULL,是告诉select程序将一直阻塞某个文件描述符改变,这里指串口有数据可读
    	 */
        p_tv = NULL;

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
        rc = _mb_select(ctx, &rfds, p_tv, length_to_read);
        if (rc == -1) {
//            _error_print(ctx, "select");
            zlog_warn(c, "select错误:%d", errno);
            if (ctx->error_recovery & MB_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if (errno == ETIMEDOUT) {
                    _sleep_and_flush(ctx);
                } else if (errno == EBADF) {				//EBADF文件描述符无效错误
                    comm_close(ctx);
                    comm_connect(ctx);
                }
                errno = saved_errno;
            }
            return -1;
        }
        rc = mb_recv(ctx, msg + msg_length, length_to_read);
// 20121002 注销此处,否则老师出现104错误(导致次原因是由于select阻塞不住,虽然时间为NULL,select总是返回)
//        if (rc == 0) {
//            errno = ECONNRESET;			// 如果错误为EINTR说明读是由中断引起 的, 如果是ECONNREST表示网络连接出了问题
//            rc = -1;
//        }
        if (rc == -1) {
            //            _error_print(ctx, "read");
            zlog_warn(c, "read错误:%d-%s", errno, strerror(errno));
            if ((ctx->error_recovery & MB_ERROR_RECOVERY_LINK) &&
                (errno == ECONNRESET || errno == ECONNREFUSED ||
                 errno == EBADF)) {
                int saved_errno = errno;
                comm_close(ctx);
                comm_connect(ctx);
                /* Could be removed by previous calls */
                errno = saved_errno;
            }

            return -1;
        }
        /* Sums bytes received */
        msg_length += rc;					// msg_length 初始值为0,如果成功接收到了2个字节,即rc = 2 ,就加上rc
        /* Computes remaining bytes */
        length_to_read -= rc;			// 将已经读出数据从length_to_read中减去

        if (length_to_read == 0) {		//length_to_read为0表示成功读出ID和功能码这两个字节
			if(step == 1)
			{
				length_to_read = msg[1] * 256 + msg[2];
				if (length_to_read != 0) {												// 通过功能码的判定得到length_to_read的值,若非零进入meta元模式
					step = 2;													// 继续通过select函数判定文件描述符是否可读
				}
				else
				{
					length_to_read = 1;
	                if ((msg_length + length_to_read) > 256) {
	//                    errno = EMBBADDATA;
	//                    _error_print(ctx, "too many data");
	                    zlog_warn(c, "数据长度超过256");
	                    return -1;
	                }
	                step = 3;
				}

			}
			else if(step == 2)
			{
				length_to_read = 1;
                if ((msg_length + length_to_read) > 256) {
//                    errno = EMBBADDATA;
//                    _error_print(ctx, "too many data");
                    zlog_warn(c, "数据长度超过256");
                    return -1;
                }
                step = 3;
			}
        }

        if (length_to_read > 0 && ctx->byte_timeout.tv_sec != -1) {
            /* If there is no character in the buffer, the allowed timeout
               interval between two consecutive bytes is defined by
               byte_timeout */
        	/*
        	 * 完成第一次select后,说明接受到了数据,这是要把select的时间参数tv改为接受每个字节间所需最大时间
        	 * 即为modbus中的分包时间,3.5个字节长
        	 */
            tv.tv_sec = ctx->byte_timeout.tv_sec;
            tv.tv_usec = ctx->byte_timeout.tv_usec;
            p_tv = &tv;
        }
    }
    if (ctx->debug) {
        int i;
        if(msg_length)
        	printf(">> ");
        for (i=0; i < msg_length; i++)
        	printf("%.2X", msg[i]);
        if(msg_length)
        	printf("\n");
    }
    return msg_length;
}
static int _mb_select(comm_t *ctx, fd_set *rfds, struct timeval *tv, int length_to_read)
{
	int s_rc;
	while ((s_rc = select(ctx->s+1, rfds, NULL, NULL, tv)) == -1)
	{
		if (errno == EINTR)
		{
			if (ctx->debug)
			{
				zlog_warn(c, "一个系统终端被捕获");
			}
			/* Necessary after an error */
			FD_ZERO(rfds);
			FD_SET(ctx->s, rfds);
		} else {
			return -1;
		}
	}
//    if (s_rc == 0)
//    {															//select返回值为0,即为超时
//        /* Timeout */
//        errno = ETIMEDOUT;
//        return -1;
//    }
	return s_rc;															//最后返回s_rc值,如果为非0或-1即检测到有数据可读
}
static int _sleep_and_flush(comm_t *ctx)
{
    /* usleep source code */
    struct timespec request, remaining;
    request.tv_sec = ctx->response_timeout.tv_sec;
    request.tv_nsec = ((long int)ctx->response_timeout.tv_usec % 1000000)
        * 1000;
    while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
        request = remaining;
    return mb_flush(ctx);
}
int mb_flush(comm_t *ctx)
{
    int rc = tcflush(ctx->s, TCIOFLUSH);
    if (rc != -1 && ctx->debug) {
        printf("%d bytes flushed\n", rc);
    }
    return rc;
}
static ssize_t mb_recv(comm_t *ctx, uint8_t *rsp, int rsp_length)
{
    return read(ctx->s, rsp, rsp_length);
}
static uint8_t compute_meta_length_after_function(int function, int msg_type)
{
    int length;

    if (msg_type == 0) {							//指示模式为本机作为从机接收主机发来的指令
        if ((function <= 0x06) ||
        			(function == 0x08)){							//wsf20150113
            length = 4;
        } else if (function == 0x0F ||
                   function == 0x10) {		//0x10写多个寄存器,起始地址高低+寄存器数量高低+字节数 = 5
            length = 5;
        } else if (function == 0x17) {
            length = 9;
        } else {
            /* _FC_READ_EXCEPTION_STATUS, _FC_REPORT_SLAVE_ID */
            length = 0;
        }
    } else {
        /* MSG_CONFIRMATION */										//确认模式为本机作为主机接收到从机发来的确认指令
        switch (function) {
        case 0x05:								//0x05写单个线圈起始地址高低+输出数量高低共4个字节
        case 0x06:						//0x06写单个寄存器,起始地址高低+寄存器数量高低共4个字节
        case 0x0f:						//0x0F写多个线圈,起始地址高低+输出值高低共4个字节
        case 0x10:				//0x10写多个寄存器,起始地址高低+寄存器数量高低共4个字节
        case 0x08:											//0x08诊断命令 wsf20150114
            length = 4;
            break;
        default:
            length = 1;
        }
    }

    return length;
}
static int compute_data_length_after_meta(comm_t *ctx, uint8_t *msg, int msg_type)
{
    int function = msg[1];					// 从接收数组msg中提取function功能码
    int length;
    /*
     * 此处针对写多字节命令,根据之a元数据分析后面还有多少各字节为要从串口读出的
     * 其他未在case中显示的命令返回length值为0
     */
    if (msg_type == 0) {
        switch (function) {
        case 0x0F:
        case 0x10:
            length = msg[1 + 5];
            break;
        case 0x17:
            length = msg[1 + 9];
            break;
        default:
            length = 0;
        }
    } else {
        /* MSG_CONFIRMATION */
        if (function <= 0x04 ||
            function == 0x11 ||
            function == 0x17) {
            length = msg[1 + 1];
        } else {
            length = 0;
        }
    }
    /*
     * 程序最后将上面得到的长度再加上默认的CRC校验长度(2),最终得到全部还要读出数据的中长度
     */
    length += 2;

    return length;
}
