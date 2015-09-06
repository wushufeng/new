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
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#include "SerialGPRS.h"

#include "../../A11_sysAttr/a11sysattr.h"
#include "../../database/database.h"
#include "../sysdatetime/getsysdatetime.h"
#include "../../log/rtulog.h"
#include "../../myfunction/myfun.h"
#include "../../port/portserial.h"
#include "../../def.h"
#include "myMB.h"

#ifdef ARM_32
#define 	GPRSDEVICE			"/dev/ttyS4"
#else
#define 	GPRSDEVICE			"/dev/ttyUSB0"
#endif

int use_backend_gprs;
pthread_t gprs_thread;
//extern oil_well *poilwell[17];

char tempdata[] = "GTP_testGTP_testGTP_testGTP_testGTP_testGTP_testGTP_testGTP_testGTP_testGTP_test\n";
char gprs_rsp_data[1024];
//char gprs_req_data[1024];
int jsq;

typedef struct _gprs_t
{
	comm_t *comm;
	char ip[16];
	char mode[4];
	int port;
}gprs_t;
gprs_t gprs_fd;
//comm_t *ctx_gprs;
//*******************************
char	AT[]="AT"; //
char	ATH[]="ATH"; //
char	ATE0[]="ATE0"; //关闭回显
char	ATV1[]="ATV1\r\n"; //
char	AT_CPOWD[]="AT+CPOWD=1\r\n"; //AT+CPOWD=1	断电(POWER OFF)
char	AT_IPR[]="AT+IPR?\r";//查询波特率
char	AT_IPR_9600[]="AT+IPR=9600;&W\r\n";//写波特率
char	AT_CGREG[]="AT+CGREG?\r\n";//网络注册状态
char	AT_CSQ[]="AT+CSQ\r\n"; //读取信号强度
char	AT_CCLK_R[]="AT+CCLK?\r\n"; //读取日期时间
char	AT_CMTE[]="AT+CMTE?\r\n";  //查询模块温度
char	AT_CIPSTATUS[]="AT+CIPSTATUS\r\n";//查询模块gprs状态
char	AT_CIPHEAD[]="AT+CIPHEAD=1\r\n"; //数据前面就会自动加上一个标志
char	AT_CPIN[] = "AT+CPIN?"; //检测SIM卡状态

char   btlv9600[]="9600";//波特率
char   CONNECT_OK[]="CONNECT OK";//联网成功
char   IP_INITIAL[]="IP INITIAL";
char   power_off[]="NORMAL POWER DOWN";
char   CIPHEAD[]="+IPD31:";//暂定接收31个数据
char   SMS_tishi[]="+CMTI: \"SM\",";
char   RING_CALL[]="RING";
char   SMS_neirong_xin_head[]="+CMGR: \"REC UNREAD\",\"+86";//

//**********域名解析********************
char   AT_CDNSGIP[]="AT+CDNSGIP=\"xajhgprs.gicp.net\"\r\n";//域名解析

//*****wei****** 域名访问*****
char   AT_CIPMUX[]="AT+CIPMUX=0\r\n";//这条命令是用来设置SIM900模块工作在单链接方式的，多连接方式应用起来比较麻烦，我们这里先以单链接的方式来演示。
char   AT_CIPRXGET[]="AT+CIPRXGET=1\r\n";// 这条命令是用来设置获取数据的方式的，参数为：1是用来设置以手动的方式来提取接收到的数据的。
char   AT_CIPQRCLOSE[]="AT+CIPQRCLOSE=1";//这条命令是用来设置加速远程断开连接用的，不必细究这条命令，照此设置就行了。
char   AT_CIPMODE[]="AT+CIPMODE=0\r\n";//这条命令是用来选择TCPIP应用模式的，如果参数为0，那么以非透明的方式来应，如果参数为1，那么以透明方式来应用。我们这里是以非透明的方式来用
char   AT_CIPSTART_DNS[]="AT+CIPSTART=\"TCP\",\"jslhzd.xicp.net\",\"80\"\r\n" ;//连接到服务器
//*************TCP----IP*******
char   AT_CSTT[]="AT+CSTT\r\n";
char   AT_CIICR[]="AT+CIICR\r\n";
char   AT_CIFSR[]="AT+CIFSR\r\n";
char   AT_CLPORT[]="AT+CLPORT=\"TCP\",\"2020\"\r\n";//指定本机端口号
char   AT_CIPSTART[]="AT+CIPSTART=\"TCP\",\"58.30.16.206\",\"1236\"\r\n";  //7226
//*************UDP----IP*******
char   AT_CLPORT_UDP[]="AT+CLPORT=\"UDP\",\"2020\"\r\n";//指定本机端口号
char   AT_CIPSTART_UDP[]="AT+CIPSTART=\"UDP\",\"58.30.16.206\",\"1236\"\r\n"; //7226
//-------------发送数据------------------------------
char   AT_CIPSEND[]="AT+CIPSEND\r\n";	   //16代表发送16个字符
//char   AT_CIPSEND[]="AT+CIPSEND=len\r\n";
char   AT_CIPSEND_dat[16];
unsigned char   AT_text_end[]={0x1a,0x0d,0x0a,0x0};	   //16代表发送16个字符
//-------------关闭连接------------------------------
char   AT_CIPCLOSE[]  ="AT+CIPCLOSE\r\n";  //断开连接
char   AT_CIPSHUT[]  ="AT+CIPSHUT";    //关闭移动场景

static int gprsThreadFunc(void *arg);
//static int receive_msg_gprs(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type);
//static int gprs_reply(modbus_t *ctx,  uint8_t *req, int req_length);


static inline int sim900a_sd_cmd(char *cmd, char *rsp, int tm_ms);
static int sim900a_init(void);
static inline char* sim900a_chk_cmd(char *str, char *dest);

int serialGPRSThreadCancel(void)
{
	int res;
	void * thread_result;

	int kill_rc = pthread_kill(gprs_thread,0);		// 使用pthread_kill函数发送信号0判断线程是否还在
	zlog_info(c, "正在取消SerialGPRS线程...");
	if(kill_rc == ESRCH)					// 线程不存在：ESRCH
		zlog_warn(c, "SerialGPRS线程不存在或者已经退出");
	else if(kill_rc == EINVAL)		// 信号不合法：EINVAL
		zlog_warn(c, "signal is invalid/n");
	else
	{
		res = pthread_cancel(gprs_thread);
		if(res != 0)	{
			zlog_error(c, "取消SerialGPRS线程失败-%d", res);
			exit(EXIT_FAILURE);
		}
	}

	zlog_info(c, "正在等待SerialGPRS线程结束...");
	res = pthread_join(gprs_thread, &thread_result);
	if(res != 0)	{
		zlog_error(c, "等待SerialGPRS线程结束失败-%d", res);
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
	char ip[16];
	int port;

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
			baud = 115200;
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


	gprs_fd.comm = new_comm_t(GPRSDEVICE, baud, parity, data_bit, stop_bit);

//	ctx_gprs = new_comm_t(GPRSDEVICE, baud, parity, data_bit, stop_bit, ip, port);
	if (gprs_fd.comm == NULL) {
		zlog_error(c, "GPRS不能动态分配一个内存环境");
		return -1;
	}
	comm_set_debug(gprs_fd.comm, 0);

    if (use_backend_gprs == GPRS) {
    	comm_set_slave(gprs_fd.comm, SERVER_ID);
    }
	// gprs
	sprintf(ip,"%d.%d.%d.%d",psysattr->commparam.master_ip_address[0],
					psysattr->commparam.master_ip_address[1],
					psysattr->commparam.master_ip_address[2],
					psysattr->commparam.master_ip_address[3]);
//	sprintf(gprs_fd.ip, "%s", ip);
	strncpy(gprs_fd.ip, ip, strlen(ip));
	port = psysattr->commparam.master_port;
	gprs_fd.port = port;
	if(psysattr->commparam.tcp_udp_identity == 1)
		strncpy(gprs_fd.mode, "UDP", 3);
	else
		strncpy(gprs_fd.mode, "TCP", 3);
	return 0;
}
void serialGprsFree()
{
	comm_close(gprs_fd.comm);

	if(gprs_fd.comm != NULL)
		comm_free(gprs_fd.comm);
}
int createGprsThread(void)
{
	int res;
	res = pthread_create(&gprs_thread, NULL, (void*)&gprsThreadFunc, NULL);
    if(res != 0)
    {
    	fprintf(stderr, "[错误]GPRS线程创建失败: %d\n", res);
//        perror("Thread creation failed");
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}
static int gprsThreadFunc(void *arg)
{
	int rc;
	int res;
	int connectsta=0;			// 0,正在连接;1,连接成功;2,连接失败
	char *ptr, *ptr1, *ptr2, *ptr3;
	int timex = 0;
	int count = 0;
	int hbeaterrcnt = 0;
	char dymanicIP[16];

	unsigned short int llll = 0;

	ptr = (char *)malloc(100);
	ptr1 = (char *)malloc(100);
	res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(res != 0)	{
		zlog_error(c, "GPRS线程pthread_setcancelstate失败");
//		exit(EXIT_FAILURE);
		pthread_exit(0);
	}
	res = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if(res != 0)	{
		zlog_error(c, "GPRS线程pthread_setcanceltype失败");
//		exit(EXIT_FAILURE);
		pthread_exit(0);
	}
    rc = comm_connect(gprs_fd.comm);
    if (rc == -1) {
    	zlog_error(c, "GPRS不能连接! %s", modbus_strerror(errno));
    	if(gprs_fd.comm != NULL)
    		comm_free(gprs_fd.comm);
    	pthread_exit(0);
    }


    sim_init:
    zlog_info(c, "初始化sim900a模块");
	if(sim900a_init() == 0)
		zlog_info(c, "sim900a初始化完成");
	else
	{
		zlog_warn(c, "sim900a初始化失败");
		goto sim_init;
	}

	zlog_info(c, "以%s方式连接服务器:%s端口号:%d", gprs_fd.mode, gprs_fd.ip, gprs_fd.port);
	sprintf(ptr, "AT+CIPSTART=\"%s\",\"%s\",\"%d\"", gprs_fd.mode, gprs_fd.ip, gprs_fd.port);
	if(sim900a_sd_cmd(ptr,"OK",500) != 0)
	{
		goto sim_init;
	}


    for(;;)
    {
    	if((timex % 20) == 0)
    	{
    		count ++;
    		if((connectsta == 2) || (hbeaterrcnt > 8))	// 连接终端了,或者连续8次心跳没有正确发送成功,则重新连接
    		{
    			sim900a_sd_cmd("AT+CIPCLOSE=1","CLOSE OK",500);
				sim900a_sd_cmd("AT+CIPSHUT","SHUT OK",500);		//
				zlog_info(c, "网络中断,以%s重新连接服务器:%s端口号:%d", gprs_fd.mode, gprs_fd.ip, gprs_fd.port);
				sim900a_sd_cmd(ptr,"OK",500);
				connectsta = 0;//
				hbeaterrcnt = 0;
    		}
    	}
    	if((connectsta == 0) && ((timex % 200) == 0))
    	{
			zlog_info(c, "查询sim900a模块网络连接状态");
			if(sim900a_sd_cmd("AT+CIPSTATUS","OK",500) == 0)
			{
				;
//				sim900a_sd_cmd("AT+CIFSR", 0, 500);
//				do	{
//					ptr1 = strstr(gprs_rsp_data, "\r\n");
//					if(ptr1 != NULL)
//					{
//						ptr1[0] = '_';ptr1[1] = '_';
//					}
//				}while(ptr1 != NULL);
//				zlog_info(c, "sim900a模块状态:%s", gprs_rsp_data);
			}
			if(strstr((const char*)gprs_rsp_data,"IP INITIAL"))	connectsta=2;	// 初始化
			else if(strstr((const char*)gprs_rsp_data,"PDP DEACT"))	connectsta=2;	// 连接断开(你应该检查一下你的天线了）
			else if(strstr((const char*)gprs_rsp_data,"CLOSED"))	connectsta=2;		// 对方掉线或断开连接了
			else if(strstr((const char*)gprs_rsp_data,"CONNECT OK"))
			{
				connectsta = 1;
				do	{
					ptr1 = strstr(gprs_rsp_data, "\r\n");
					if(ptr1 != NULL)
					{
						ptr1[0] = '_';ptr1[1] = '_';
					}
				}while(ptr1 != NULL);
				zlog_info(c, "sim900a模块状态:%s", gprs_rsp_data);
				bzero(gprs_rsp_data,strlen(gprs_rsp_data));
				if(sim900a_sd_cmd("AT+CIFSR", "ERROR", 500) != 0)
				{
					bzero(dymanicIP, sizeof(dymanicIP));
					strncpy(dymanicIP, gprs_rsp_data, strlen(gprs_rsp_data));
					printf("获取到本机动态IP:%s", dymanicIP);
				}

			}
			else
			{
				do	{
					ptr1 = strstr(gprs_rsp_data, "\r\n");
					if(ptr1 != NULL)
					{
						ptr1[0] = '_';ptr1[1] = '_';
					}
				}while(ptr1 != NULL);
				zlog_info(c, "sim900a模块状态:%s", gprs_rsp_data);
//				printf("%s\n", gprs_rsp_data);
			}
    	}
		if((connectsta == 1) && (timex >= 300))// 连接正常得时候每6秒发送一次心跳
		{
			printf("发送心跳包");
			timex=0;
			if(sim900a_sd_cmd("AT+CIPSEND",">",200)==0)// 发送数据
			{
				sim900a_sd_cmd((char *)0X00,0,0);	// 发送数据 0x00
				usleep(20000);						// 必须加延时
				sim900a_sd_cmd((char *)0X1A,0,0);	// ctrl+z,结束数据发送,启动一次传输
			}else
			{
				sim900a_sd_cmd((char *)0X1B,0,0);	// ESC,取消发送
				printf("取消发送");
			}
			hbeaterrcnt++;
			printf("hbeaterrcnt:%d\r\n",hbeaterrcnt);//
		}
		usleep(10000);
    	if(1)
    	{
			if(hbeaterrcnt)							// 需要检测心跳包应答
			{
				if(strstr((const char*)gprs_rsp_data,"SEND OK"))hbeaterrcnt=0;// 心跳正常
			}
    		llll = 0;
    		bzero(gprs_rsp_data, strlen(gprs_rsp_data));
    		SerialRead(gprs_fd.comm, gprs_rsp_data, sizeof(gprs_rsp_data), &llll, 1);
    		ptr2 = strstr((const char*)gprs_rsp_data, "+IPD");
    		if(ptr2 != NULL)
    		{
    			printf("%s\n", ptr2);
    			ptr3 = strstr((const char*)ptr2,",");
    			ptr2 = strstr((const char*)ptr2,":");
    			ptr2[0] = 0;// 加入结束符
    			printf("接收到%s字节,内容如下:\n",ptr3 + 1);
//    			ptr2[0] = ':';// 加入结束符
//    			ptr1 = strstr((const char*)ptr2,":");
    			printf("%s\n", ptr2 + 1);
    			if(gprs_fd.comm->debug)
    			{
					int i;
					if(llll)
						printf(">> ");
					for (i=0; i < llll; i++)
						printf("%.2X ", gprs_rsp_data[i]);
					if(llll)
						printf("\n");
    			}

	            // 发送测试
				if(sim900a_sd_cmd("AT+CIPSEND",">",200)==0)// 发送数据
				{
					sim900a_sd_cmd((ptr2 + 1),"SEND OK",40);	// 发送数据 0x00
					usleep(20000);						// 必须加延时
					sim900a_sd_cmd((char *)0x1A,0,0);	// ctrl+z,结束数据发送,启动一次传输
				}else
				{
					sim900a_sd_cmd((char *)0x1B,0,0);	// ESC,取消发送
					printf("取消发送");
				}
    		}
    	}
    	timex++;
	}
    free(ptr);
    free(ptr1);
    serialGprsFree();
	pthread_exit(0);
}
static int sim900a_init(void)
{
	int re_try = 0;
	char *ptr, *ptr2;
	while(sim900a_sd_cmd("AT","OK",100) != 0)						//AT检测模块
	{
		zlog_warn(c, "未检测到sim900a模块");
		usleep(800000);
		zlog_info(c, "尝试连接sim900a模块...");
		usleep(400000);
		if(re_try ++ > 5)
		{
			zlog_warn(c, "请检查sim900a模块是否安装好");
			return - 1;
		}
	}
	if(sim900a_sd_cmd("ATE0","OK",200) != 0)	return 2;

	while(sim900a_sd_cmd("AT+CPIN?","OK",200) != 0)						//AT检测模块
	{
		zlog_warn(c, "未检测到sim卡");
		usleep(800000);
		zlog_info(c, "尝试连接sim卡...(%d)", ++re_try);
		usleep(400000);
		if(re_try > 5)
		{
			zlog_warn(c, "请检查sim卡是否安装好");
			return - 2;
		}
	}
//	if(sim900a_sd_cmd(AT_CPIN,"OK",200) != 0)	return 3;      						//检测SIM卡状态
//	if(sim900a_sd_cmd(AT_CIPQRCLOSE,"CLOSE OK",2) != 0)	return -1;	        		//关闭连接
	sim900a_sd_cmd("AT+CIPCLOSE=1", "CLOSE OK", 100);	        			//关闭连接
	sim900a_sd_cmd("AT+CIPSHUT", "SHUT OK", 100);						//关闭移动场景
	if(sim900a_sd_cmd("AT+CGCLASS=\"B\"", "OK", 1000) != 0)	return 4;	      		//设置GPRS移动台类别为B,支持包交换和数据交换
	if(sim900a_sd_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"", "OK", 1000) != 0)	return 5;  //设置PDP上下文,互联网接协议,接入点等信息
	if(sim900a_sd_cmd("AT+CGATT=1", "OK", 500) != 0)	return 6;		            //附着GPRS业务
	if(sim900a_sd_cmd("AT+CIPCSGP=1,\"CMNET\"", "OK", 500) != 0)	return 7;	    //设置为GPRS连接模式
	if(sim900a_sd_cmd("AT+CIPHEAD=1", "OK", 500) != 0 )	return 8;		//设置接收数据显示IP头(方便判断数据来源)
	if(sim900a_sd_cmd("AT+CNUM", "+CNUM", 200) == 0)
	{
		ptr = strstr((const char*)gprs_rsp_data,"\"+");
		ptr2 = strstr((const char *)ptr,"\",");
		ptr2[0] = 0;	// 加入结束符
		printf("本机号码为:%s\n", ptr + 1);
		ptr2[0] = '\\';	// 加入结束符
	}
	bzero(gprs_rsp_data, strlen(gprs_rsp_data));
	return 0;
}
static inline int sim900a_sd_cmd(char *cmd, char *rsp, int tm_ms)
{
	int res;
	int r_len;
	unsigned short int llll = 0;
	char *strx = 0;
	char cmd_enter[128];
	char *ptr1;
//	int retry_num = 0;
//retry:
//	if(gprs_fd.comm->debug)
//		printf("<<cmd:%s\n", cmd);
//	if(((cmd[0] == 0x1A) && (cmd[1] == 0x1A)) || ((cmd[0] == 0x1B) && (cmd[1] == 0x1B)) )
	if((unsigned int)cmd <= 0xFF)
	{
		if(gprs_fd.comm->debug)
			printf("<<cmd:%s\n",(char *)&cmd);
		res = comm_send(gprs_fd.comm, (char *)&cmd, 1);
//		res = serialSend(gprs_fd.comm, "%d", (int)cmd);
	}
	else
	{
		if(gprs_fd.comm->debug)
			printf("<<cmd:%s\n", cmd);
		bzero(cmd_enter,sizeof(cmd_enter));
		strncpy(cmd_enter, cmd, strlen(cmd));
		strcat(cmd_enter, "\r\n");
//		res = comm_send(gprs_fd.comm, cmd_enter, strlen(cmd_enter));
		res = serialSend(gprs_fd.comm,"%s\r\n", cmd);
	}
	if(res == -1)
	{
		if((unsigned int)cmd <= 0xFF)
			zlog_error(c, "写串口错误(%d)", (int)cmd);
		else
			zlog_error(c, "写串口错误(%s)", cmd);
		return -1;
	}
	usleep(20000);
	bzero(gprs_rsp_data,strlen(gprs_rsp_data));
	if(rsp)
	{
		r_len = 0;
		SerialRead(gprs_fd.comm, gprs_rsp_data, sizeof(gprs_rsp_data), &llll, tm_ms);
		r_len = llll;
	//	r_len = comm_read(gprs_fd.comm, gprs_rsp_data, sizeof(gprs_rsp_data), timeout);
	//	if(r_len == 0)
	//	{
	//		printf("读取sim900a数据超时(%s)\n", cmd);
	//		res = -1;
	//	}
		if(gprs_fd.comm->debug)
		{
			do	{
				ptr1 = strstr(gprs_rsp_data, "\r\n");
				if(ptr1 != NULL)
				{
					ptr1[0] = '_';ptr1[1] = '_';
				}
			}while(ptr1 != NULL);
//			printf("%s", gprs_rsp_data);
			printf(">>rsp(%d):%s\n", r_len, gprs_rsp_data);
		}
	//	if(r_len > 0)
		{
	//		sim900a_chk_cmd(gprs_rsp_data, rsp);
			strx = strstr((const char*)gprs_rsp_data, rsp);
			if(strx != NULL)
			{
				res = 0;
				if(gprs_fd.comm->debug)
					printf("配置GPRS成功(%s)\n", cmd);
			}
			else
			{
				if(gprs_fd.comm->debug)
					printf("配置GPRS失败(%s)\n", cmd);
				res = -1;
			}
		}
	}
//	else
//	{
//		res = -1;
//		printf("读串口错误(%s)\n", cmd);
//	}
//	bzero(gprs_rsp_data,sizeof(gprs_rsp_data));
//	if((res <= 0) && (retry_num < 3))
//	{
//		retry_num ++;
//		goto retry;
//	}
//	if(retry_num >= 3)
//		res = -1;
	return res;
}
// sim900a发送命令后,检测接收到的应答
// str:期待的应答结果
// 返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
static inline char* sim900a_chk_cmd(char *dest, char *str)
{
	char *strx = NULL;

	strx=strstr((const char*)dest,(const char*)str);

	return (char* )strx;
}

