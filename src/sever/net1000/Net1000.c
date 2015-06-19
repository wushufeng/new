/*
 * Net1000.c
 *
 *  Created on: 2015年3月25日
 *      Author: wsf
 */
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <arpa/inet.h>

//#include "../../def.h"
//#include "../../modbus/modbus.h"
//#include "../../modbus/modbus-tcp.h"
#include "../../modbus/modbus-private.h"
#include "../../database/database.h"
#include "../../A11_sysAttr/a11sysattr.h"
#include "Net1000.h"

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


static int net1000ThreadFunc(void *arg)
{
	int res;
	int net_slave;
	int net_offset;
//	modbus_tcp_t *ctx_tcp = ctx_net1000->backend_data;

	 if (use_backend_net1000 == TCP)
	 {
			switch(comm_mode)					// 主站通信方式
			{
				case MCM_INVALID:																		// 无效

					break;
				case MCM_GPRS_CDMA:																// GPRS/CDMA

					break;
				case MCM_TCP_SERVER:
					printf("[提示]Net1000正在尝试连接服务器...\n");
					 sockfd = modbus_tcp_client_socket(ctx_net1000);								// TCP client
				 	 res = modbus_tcp_connect(ctx_net1000);											// TCP client
//				 	 if(res == -1)
//				 	 {
//				 		 printf("[错误]Net1000无法连接主站%s，服务端口: %d\n",ctx_tcp->ip, ctx_tcp->port);
//				 	 }
					break;
				case MCM_TCP_CLIENT:
					printf("[提示]Net1000正在监听本地服务端口...\n");
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

    for (;;) {
	    if (use_backend_net1000 == TCP)
	    {
	    	if(comm_mode == MCM_TCP_CLIENT)
	    		res = modbus_tcp_accept(ctx_net1000, &sockfd);							// TCP server
	    }
	    else if (use_backend_net1000 == UDP)
	    {
	        ;
	    }
    	/*
    	 * 函数modbus_receive,判断指示或回应两模式下不同功能码接受的数据
    	 * 返回正确接受的数据,并且该数据通过CRC校验
    	 * 正确返回接收到数据的总长度,所读出的数据放入query_net1000所指向的地址中
    	 * 错误返回-1
    	 */
    	printf("11111...\n");
	    for(;;){
	    	if(res == -1)
	    		goto disconnect;
			rc_net1000 = modbus_receive(ctx_net1000, query_net1000,11);
			if (rc_net1000 == -1) {
				/* Connection closed by the client or error */
//				break;
	        	goto disconnect;
			}

	/*        if (rc_net1000 == 0) {
	//        	if(modbus_test_debug(ctx))
	//        		printf("\nThere is %d times out select\n",num++);
				a5d3ad_get_ad(mb_mapping->tab_input_registers,
						UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);
				continue;
			}*/
	//        printf("122344...\n");
			/* Read holding registers */
//			if (query_net1000[header_length] == 0x06)
//			{
//				printf("  pppppp \n");
//			}
//			if (query_net1000[header_length] == 0x03)
//			{
//			   // 如果接受到的指示数组中的第4,5字节对应的读出数量为2,就将数量改为1
//				if (MODBUS_GET_INT16_FROM_INT8(query_net1000, header_length + 3) == UT_REGISTERS_NB_SPECIAL)
//				{
//					printf("Set an incorrect number of values\n");
//					MODBUS_SET_INT16_TO_INT8(query_net1000, header_length + 3, UT_REGISTERS_NB_SPECIAL - 1);
//				}
//				else if (MODBUS_GET_INT16_FROM_INT8(query_net1000, header_length + 1) == UT_REGISTERS_ADDRESS_SPECIAL)
//				{
//					printf("Reply to this special register address by an exception\n");
//					modbus_reply_exception(ctx_net1000, query_net1000, MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY);
//					continue;
//				}
//			}
			net_offset = ctx_net1000->backend->header_length;			// 取出数据头长度默认为1
		    net_slave = query_net1000[net_offset - 1];
	        if((net_slave < 1) ||((net_slave > 16) && (net_slave != 128)))
	        {
	        	printf("[提示]Modbus TCP device ID = %d, 不在范围内ID = 128|| 1 <= ID <= 16\n", net_slave);
	        	goto disconnect;//break;
	        }
	        if(net_slave == 128)
	        	net_slave = 0;
			rc_net1000 = modbus_reply(ctx_net1000, query_net1000, rc_net1000, mb_mapping[net_slave]);
			if (rc_net1000 == -1) {
//				break;
				goto disconnect;
			}
	    }
		disconnect:
		 close(ctx_net1000->s);
		        sleep(5);
    }
    return 1;
}
int createNet1000Thread(void)
{
	int res;
	res = pthread_create(&net1000_thread, NULL, (void*)&net1000ThreadFunc, NULL);
    if(res != 0)
    {
    	fprintf(stderr, "Thread creation failed: %s\n", modbus_strerror(errno));
//        perror("Thread creation failed");
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
			printf("[提示]Net1000作为客户端工作在TCP模式下，连接主站：%s, 服务端口:%d\n", ip, port);
			break;
		case MCM_TCP_CLIENT:
	    	sprintf(ip,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
	    			psysattr->commparam.ip_address[1],
					psysattr->commparam.ip_address[2],
					psysattr->commparam.ip_address[3]);
	    	port = psysattr->commparam.tcp_port;
	    	printf("[提示]Net1000作为服务器工作在TCP模式下，本地IP：%s, 服务端口:%d\n", ip, port);
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
        	fprintf(stderr, "Failed to allocate the query_net1000: %s\n", modbus_strerror(errno));
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
    modbus_set_debug(ctx_net1000, FALSE);
//    if (mb_mapping == NULL)
//    {
//        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
//        modbus_free(ctx_net1000);
//        return -1;
//    }
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
		free(query_net1000);
	if(ctx_net1000 != NULL)
		modbus_free(ctx_net1000);
}
//int modbus_receive_write(uint8_t *req , int head_length)
//{
//
//}
