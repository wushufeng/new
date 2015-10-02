/*
 * Net1000.c
 *
 *  Created on: 2015年3月25日
 *      Author: wsf
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include<netdb.h>

//#include	<sys/socket.h>
//#include	<sys/un.h>
//#include	<unistd.h>

//#include "../../def.h"
//#include "../../modbus/modbus.h"
//#include "../../modbus/modbus-tcp.h"
#include "../../modbus/modbus-tcp-private.h"
#include "../../modbus/modbus-private.h"
#include "../../database/database.h"
#include "../../A11_sysAttr/a11sysattr.h"
#include "Net1000.h"
#include "../../log/rtulog.h"

int sockfd;
modbus_t *ctx_net1000;
int rc_net1000;
int use_backend_net1000;
uint8_t *query_net1000;
int header_length;
pthread_t net1000_thread;
int comm_mode;					//
//enum {
//    TCP,
//    UDP,
//    RTU
//};

static int net1000ThreadFunc(void *arg);
static int modbus_tcp_client_socket(modbus_t *ctx);
static int modbus_tcp_connect(modbus_t *ctx);
static int tcpClientConnect(modbus_t *ctx);
static char *Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

static int net1000ThreadFunc(void *arg)
{
	int res;
	int net_slave;
	int net_offset;



//	modbus_tcp_t *ctx_tcp = ctx_net1000->backend_data;
	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "Net1000线程pthread_setcancelstate失败-%d", res);
		exit(EXIT_FAILURE);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "Net1000线程pthread_setcanceltype失败-%d", res);
		exit(EXIT_FAILURE);
	}
	 if (use_backend_net1000 == TCP)
	 {
			switch(comm_mode)					// 主站通信方式
			{
				case MCM_INVALID:																		// 无效

					break;
				case MCM_GPRS_CDMA:																// GPRS/CDMA

					break;
				case MCM_TCP_SERVER:
				{
					modbus_tcp_t *ctx_tcp = ctx_net1000->backend_data;
					zlog_info(c, "Net1000正在尝试连接服务器:%s端口:%d...", ctx_tcp->ip, ctx_tcp->port);
					res = tcpClientConnect(ctx_net1000);
//					sockfd = modbus_tcp_client_socket(ctx_net1000);									// TCP client
//					res = modbus_tcp_connect(ctx_net1000);											// TCP client
					if(res == -1)
					{
						printf("Net1000无法连接主站");
					}

					break;
				}
				case MCM_TCP_CLIENT:
					zlog_info(c, "Net1000正在监听本地服务端口...");
					sockfd = modbus_tcp_listen(ctx_net1000, 1);										// TCP server
					break;
				case MCM_UDP_SERVER:

					break;
				case MCM_UDP_CLIENT:

					break;
				default:
					break;
			}
	 }

    for (;;)
    {
		if(comm_mode == MCM_TCP_SERVER)
		{
			for(;;)
			{

			}
		}
    	if(comm_mode == MCM_TCP_CLIENT)
		{
    		printf("modbus_tcp_accept\n");
			res = modbus_tcp_accept(ctx_net1000, &sockfd);							// TCP server
    	/*
    	 * 函数modbus_receive,判断指示或回应两模式下不同功能码接受的数据
    	 * 返回正确接受的数据,并且该数据通过CRC校验
    	 * 正确返回接收到数据的总长度,所读出的数据放入query_net1000所指向的地址中
    	 * 错误返回-1
    	 */

	    for(;;){
//	    	if(res == -1)
//	    		goto disconnect;
			rc_net1000 = modbus_receive(ctx_net1000, query_net1000,11);
			if (rc_net1000 == -1) {
				/* Connection closed by the client or error */
				break;
//	        	goto disconnect;
//				continue;
			}
			net_offset = ctx_net1000->backend->header_length;			// 取出数据头长度默认为1
		    net_slave = query_net1000[net_offset - 1];
	        if((net_slave < 1) ||((net_slave > 16) && (net_slave != 128)))
	        {
	        	zlog_warn(c, "Modbus TCP device ID = %d, 不在范围内ID = 128|| 1 <= ID <= 16", net_slave);
	        	continue;
//	        	goto disconnect;
	        }
	        if(net_slave == 128)
	        	net_slave = 0;
			rc_net1000 = modbus_reply(ctx_net1000, query_net1000, rc_net1000, (modbus_mapping_t *)mb_mapping[net_slave]);
			if (rc_net1000 == -1) {
				break;
//				goto disconnect;
			}
	    }
//		disconnect:
//			close(ctx_net1000->s);
//		sleep(5);
//		 if(comm_mode == MCM_TCP_SERVER)
//		 {
//			zlog_info(c, "Net1000正在尝试连接服务器...");
//			res = tcpClientConnect(ctx_net1000);
//
////					sockfd = modbus_tcp_client_socket(ctx_net1000);									// TCP client
////					res = modbus_tcp_connect(ctx_net1000);											// TCP client
//			 if(res == -1)
//			 {
//				 printf("Net1000无法连接主站");
//			 }
//			if (getpeername(ctx_net1000->s, (struct sockaddr *)&ss, &len) < 0)
//			{
//				printf("getpeername 错误\n");
//			}
//			printf("Step4:连接到： %s\n", Sock_ntop_host( (struct sockaddr *)&ss, len));
//		 }
    	}
    }
    pthread_exit(0);
}
int createNet1000Thread(void)
{
	int res;
	res = pthread_create(&net1000_thread, NULL, (void*)&net1000ThreadFunc, NULL);
    if(res != 0)
    {
    	zlog_error(c, "创建Net1000线程失败:%s", modbus_strerror(errno));
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}

int net1000Init(void *obj)
{
	char ip[16];
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	int port;

	comm_mode = psysattr->commparam.master_comm_mode;
	switch(comm_mode)					// 主站通信方式
	{
		case MCM_INVALID:																		// 无效

			break;
		case MCM_GPRS_CDMA:																// GPRS/CDMA

			break;
		case MCM_TCP_SERVER:
			sprintf(ip,"%d.%d.%d.%d",psysattr->commparam.master_ip_address[0],
			    			psysattr->commparam.master_ip_address[1],
							psysattr->commparam.master_ip_address[2],
							psysattr->commparam.master_ip_address[3]);
			port = psysattr->commparam.master_port;
			zlog_info(c, "Net1000作为客户端工作在TCP模式下，连接主站：%s, 服务端口:%d", ip, port);
			break;
		case MCM_TCP_CLIENT:
	    	sprintf(ip,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
	    			psysattr->commparam.ip_address[1],
					psysattr->commparam.ip_address[2],
					psysattr->commparam.ip_address[3]);
	    	port = psysattr->commparam.tcp_port;
	    	zlog_info(c, "Net1000作为服务器工作在TCP模式下，本地IP：%s, 服务端口:%d", ip, port);
			break;
		case MCM_UDP_SERVER:

			break;
		case MCM_UDP_CLIENT:

			break;
		default:
			break;
	}

	switch(psysattr->commparam.tcp_udp_identity)
	{
		case 0:
			use_backend_net1000 = TCP;
		break;
		case 1:
			use_backend_net1000 = UDP;
		break;
		default:
			use_backend_net1000 = TCP;
	}
    if (use_backend_net1000 == TCP)
    {
//    	assert()
//    	sprintf(ip,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
//    			psysattr->commparam.ip_address[1],
//				psysattr->commparam.ip_address[2],
//				psysattr->commparam.ip_address[3]);
//    	sprintf(ip,"%s","192.168.3.240");
//    	sprintf(ip,"%s","192.168.93.240");
    	ctx_net1000 = modbus_new_tcp(ip, port);
        query_net1000 = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
        if(query_net1000 == NULL)
        {
        	zlog_error(c, "为Net1000分配内存失败: %s", modbus_strerror(errno));
//        	perror("Malloc query_net1000 failed");
        	return -1;
        }
    }
    else  if(use_backend_net1000 == UDP)
    {
    	;
    }
    modbus_set_slave(ctx_net1000, SERVER_ID);
    header_length = modbus_get_header_length(ctx_net1000);
    modbus_set_debug(ctx_net1000, TRUE);
//    modbus_set_debug(ctx_net1000, FALSE);
//    if (mb_mapping == NULL)
//    {
//        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
//        modbus_free(ctx_net1000);
//        return -1;
//    }
	return 0;
}
int net1000ThreadCancel(void)
{
	int res;
	void * thread_result;

	int kill_rc = pthread_kill(net1000_thread, 0);		// 使用pthread_kill函数发送信号0判断线程是否还在
	zlog_info(c, "正在取消Net1000线程...");
	if(kill_rc == ESRCH)					// 线程不存在：ESRCH
		zlog_warn(c, "Net1000线程不存在或者已经退出");
	else if(kill_rc == EINVAL)		// 信号不合法：EINVAL
		zlog_warn(c, "非法信号");
	else
	{
		res = pthread_cancel(net1000_thread);
		if(res != 0)	{
			zlog_error(c, "取消Net1000线程失败-%d", res);
			exit(EXIT_FAILURE);
		}
	}

	zlog_info(c, "正在等待Net1000线程结束...");
	res = pthread_join(net1000_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待Net1000线程结束失败-%d", res);
		exit(EXIT_FAILURE);
	}
	return 0;
}
void net1000Free()
{
	if (use_backend_net1000 == TCP) {
//		_modbus_tcp_close(ctx_net1000);
	    shutdown(sockfd, SHUT_RDWR);
	    close(sockfd);
	}
//	if(mb_mapping !=NULL)
//		modbus_mapping_free(mb_mapping);
	if(query_net1000 !=NULL)
	{
		free(query_net1000);
		query_net1000 = NULL;
	}
	if(ctx_net1000 != NULL)
		modbus_free(ctx_net1000);
}
/* @brief
 * wsf
 * 作为Client的TCP方式
 * connect
 */
int modbus_tcp_connect(modbus_t *ctx)
{
	struct sockaddr_in server;
	modbus_tcp_t *ctx_tcp = ctx->backend_data;
	int res;

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(ctx_tcp->port);
//	server.sin_port = htons(6000);
	if (inet_pton(AF_INET, ctx_tcp->ip, &server.sin_addr) <= 0) {
//		  printf("[错误]Net1000调用inet_pton() 错误\n");
		  zlog_error(c, "Net1000调用inet_pton() 错误");
		  return -1;
		}
	res = connect(ctx->s,(struct sockaddr *)&server,sizeof(server));
    if(res ==- 1){
//    	printf("[错误]Net1000无法连接主站%s，服务端口: %d\n",ctx_tcp->ip, ctx_tcp->port);
    	zlog_error(c, "Net1000无法连接主站%s，服务端口: %d\n",ctx_tcp->ip, ctx_tcp->port);
//    	printf("[错误]connect()error\n");
    	return -1;
    }
    return res;
}

/* @brief
 * wsf
 * 作为Client的TCP方式
 * socket
 */
int modbus_tcp_client_socket(modbus_t *ctx)
{
//	int sockfd;
//	struct sockaddr_in server;
//	modbus_tcp_t *ctx_tcp = ctx->backend_data;

	ctx->s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//	ctx->s = sockfd;
	if(ctx->s == -1){
		zlog_error(c, "Net1000执行socket 错误");
		return -1;
	}
//	bzero(&server, sizeof(server));
//	server.sin_family = AF_INET;
//	server.sin_port = htons(1502);
//	if (inet_pton(AF_INET, "192.168.93.1", &server.sin_addr) <= 0) {
//		  printf("[错误]inet_pton() error\n");
//		  return -1;
//		}
//    if(connect(sockfd,(struct sockaddr *)&server,sizeof(server))==-1){
//    	printf("[错误]connect()error\n");
//    	return -1;
//    }
//    ctx->s =
	return ctx->s;
}

static int tcpClientConnect(modbus_t *ctx)
{
	int	n;
	int	res_connect;//++++
	struct sockaddr_in t;
	struct addrinfo	hints, *res, *ressave;
	modbus_tcp_t *ctx_tcp = ctx->backend_data;
	char port[6];
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	sprintf(port, "%d", ctx_tcp->port);
	port[5] = '\0';
	if ( (n = getaddrinfo(ctx_tcp->ip, port, &hints, &res)) != 0)
	{
		printf("TCP连接错误：%s, %d: %s", ctx_tcp->ip, ctx_tcp->port, gai_strerror(n));
		exit(-1);
	}
	ressave = res;
	do
	{
	//		printf("Step2.3:socket入口参数为ai_family = %d, ai_socktype = %d, ai_protocol = %d\n", res->ai_family, res->ai_socktype, res->ai_protocol);
	//		printf("Step2.3:socket入口参数为ai_family = %d, ai_socktype = %d, ai_protocol = %d\n", PF_INET, SOCK_STREAM, IPPROTO_TCP);
		ctx->s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	//		sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ctx->s < 0)
		{
			printf("Step2.4:创建socket套接字失败！sockfd = %d\n", ctx->s);
			continue;	/* ignore this one */
		}

	//		t = (struct sockaddr_in *)&res->ai_addr;
	//		t = res->ai_addr;
	//		t.sin_addr.s_addr = inet_addr("192.168.93.128");
		t.sin_addr.s_addr = inet_addr(ctx_tcp->ip);
		t.sin_port = htons(ctx_tcp->port);
//		printf("port = %d\n", htons(t.sin_port));
		t.sin_family = AF_INET;  //设置地址家族
//		printf("%s\n",inet_ntoa(t.sin_addr));
	//		int n = 0;
	//		for(n = 0; n < 14; n ++)
	//			printf("%.2X",(unsigned char *)res->ai_addr->sa_data[n]);
	//		printf("\n");
	//		if ( (res_connect = connect(sockfd, res->ai_addr, res->ai_addrlen))== 0)
		if ( (res_connect = connect(ctx->s, (struct sockaddr *)&t, sizeof(struct sockaddr)))== 0)
		{
			zlog_info(c, "RTU成功连接服务器%s，服务端口: %d",ctx_tcp->ip, ctx_tcp->port);
			break;		/* success */
		}
		else //++++
		{
			zlog_info(c, "RTU未能连接服务器errno = %d\n",res_connect);
//			printf("Step2.5:connect服务器失败errno = %d\n",res_connect);
		}
	//		Close(sockfd);	/* ignore this one */

		if (close(ctx->s) == -1)
		{
	//			err_sys("close error");
			zlog_warn(c, "关闭Net1000sockfd错误:%d", errno);
			exit(-1);
		}
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
	{
	//		err_sys("tcp_connect error for %s, %s", host, serv);
		printf("TCP连接错误： %s, %d", ctx_tcp->ip, ctx_tcp->port);
		exit(-1);
	}

	freeaddrinfo(ressave);

//	return(ctx->s);
	return 0;
}
char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}
#endif

//#ifdef	AF_UNIX
//	case AF_UNIX: {
//		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;
//
//			/* OK to have no pathname bound to the socket: happens on
//			   every connect() unless client calls bind() first. */
//		if (unp->sun_path[0] == 0)
//			strcpy(str, "(no pathname bound)");
//		else
//			snprintf(str, sizeof(str), "%s", unp->sun_path);
//		return(str);
//	}
//#endif
//
//#ifdef	HAVE_SOCKADDR_DL_STRUCT
//	case AF_LINK: {
//		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;
//
//		if (sdl->sdl_nlen > 0)
//			snprintf(str, sizeof(str), "%*s",
//					 sdl->sdl_nlen, &sdl->sdl_data[0]);
//		else
//			snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
//		return(str);
//	}
//#endif
	default:
		snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

static char *Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
	{
//		err_sys("sock_ntop_host error");	/* inet_ntop() sets errno */
		printf("sock_ntop_host错误\n");
		exit(1);
	}
	return(ptr);
}
