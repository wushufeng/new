/*
 * new_rtu.c
 *
 *  Created on: 2015年3月20日
 *      Author: wsf
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <mach/gpio.h>
//#include <mach/at91_pio.h>
//#include <assert.h>

#include "./log/rtulog.h"
#include "def.h"
#include "database/database.h"
#include "./A11_sysAttr/a11sysattr.h"
#include "./sever/serial232/Serial232.h"
#include "./sever/net1000/Net1000.h"
#include "./sever/serialzigbee/SerialZigbee.h"
#include "./sever/serialGPRS/SerialGPRS.h"
#include "./sever/AI8/a5d35ai.h"
#include "common/common.h"
#include "./managerDB/ManagerDB.h"
#include "./sever/sysdatetime/getsysdatetime.h"

#define MAXBUFSIZE 512
char AppPathFileName[MAXBUFSIZE];
char AppPath[MAXBUFSIZE];
char AppFileName[32];
char AppOptinFile[MAXBUFSIZE];//配置文件路径
char SysIniFile[MAXBUFSIZE];//系统ini文件

E1_sys_attribute *psysattr;

typedef struct
{
	int gprs_sta;
	int net_sta;
	int zigbee_sta;
	int rs232_sta;
	int rs485_sta;
	int aidi_sta;
}ser_sta_t;
//void mb_mapping_free(mb_mapping_t *mb_mapping);
//int testCopy(char * file);

int main(int argc, char *argv[])
{

	int res;
	char cCh;
	char bDoExit ;
	int n;
	ser_sta_t ser_sta = {0, 0, 0, 0, 0, 0};
	int zigbee_en = 1;
	int gprs_en = 0;
	int net_en = 1;
	int s232_en = 0;

//    char hname[128];
//    struct hostent *hent;
//    int i;

//	testCopy(NULL);

	// zlog
	if((res = Zlog_init()) != 0)
		return res;
	if((res = Zlog_get_category()) != 0)
		return res;


//	printf("[提示]创建sqlite库!\n");
////	testCreateTables();
////	databaseInsert(pexbuffer[0]);
//
//	searchOldestData();
//	printf("[提示] 数据库调试完成!\n");

//	unsigned short int ttt[] = {0x0019, 0x0059, 0x0009, 0x2005, 0x0008, 0x0015};
//	res = setSystemTime(ttt);
//	if(res == 0)
//		printf("设置时间成功\n");
//	else
//		printf("设置时间失败\n");

//	gethostname(hname, sizeof(hname));
//    //hent = gethostent();
//    hent = gethostbyname(hname);
//
//    printf("hostname: %s/naddress list: ", hent->h_name);
//    for(i = 0; hent->h_addr_list[i]; i++) {
//        printf("%s/t", inet_ntoa(*(struct in_addr*)(hent->h_addr_list[i])));
//    }

////	 创建holdingreg全局锁
//    res = pthread_mutex_init(&holdingReg_mutex,NULL);
//    if(res != 0)
//    {
//        perror("holdingReg_mutex initialization failed");
//        exit(EXIT_FAILURE);
//    }
////     开辟20000个空间,用于mb保持寄存器
//	u2_printf(tt, "测试测试!!!!%s%s%s%d", "1", "2", "3",4);printf("%s\n", tt);
	zlog_info(c, "******************************************");
	zlog_info(c, "**************RTU程序启动*****************");
	zlog_info(c, "******************************************");
	zlog_info(c, "RTU日志模块初始化完毕");
//	zlog_debug(c, "MBMapping数据空间错误!");
//	zlog_warn(c, "A11配置文件不存在，创建新文件并写入默认配置");
//	zlog_error(c, "MBMapping数据空间错误!");
//	getAppPathFileName((char*)AppPathFileName);
//	getAppPath((char*)AppPath);
//	getAppName((char*)AppFileName);
	if(mbMappingNew() == 0)
		zlog_info(c, "MBMapping数据空间准备就绪");
	else
		zlog_error(c, "MBMapping数据空间错误");

	// 加载A11数据,该指针指向mb保持寄存器
	psysattr = LoadConfigA11();
	zlog_info(c, "启动数据同步线程");
	res = createDatabaseThread();
	if(res != 0)
	{
		zlog_error(c, "启动数据同步失败");
		exit(EXIT_FAILURE);
	}
//	int sss = 0;
//	while (1)
//	{
////		switch (sss) {
////		case 0:
////		case 1:
////			sss  = 1;
////			break;
////		}
////		sss = 5;
//		if(sss == 0)
//			sss = 1;
//		else if(sss == 1)
//			sss = 3;
//	}
//	sss = 9;

	/* @brief
	 * wsf
	 * 创建aidi线程
	 */
//	if(0)
	{
		zlog_info(c, "启动AIDI线程");
		res = createAidiThread();
		if(res != 0)
		{
			zlog_error(c, "AIDI线程创建失败");
			exit(EXIT_FAILURE);
		}
	}
	/*@brief
	 * wsf
	 * TCP
	 */
	//////////////////////////////////////////////////////////
//    if(0)
	if((psysattr->commparam.communication_mode == CM_GPRS_CDMA) && (gprs_en))
	{
		zlog_info(c, "GPRS正在初始化...");
		res = serialGprsInit((void *) psysattr);
		if(res == 0)
		{
			zlog_info(c, "GPRS初始化完成");
			zlog_info(c, "启动GPRS线程");
			ser_sta.gprs_sta = 1;
			res = createGprsThread();
		    if(res != 0)
		    {
		    	zlog_error(c, "GPRS线程创建失败");
		        exit(EXIT_FAILURE);
		    }
		}
		else
			zlog_error(c, "GPRS初始化失败");
	}
	/////////////////////////////////////////////////////////////
	/*@brief
	 * wsf
	 * TCP
	 */
		//////////////////////////////////////////////////////////
//    if(0)
	if((psysattr->commparam.communication_mode == CM_WIRELESS_BRIDGES) || (net_en))
	{
		zlog_info(c, "Net1000正在初始化...");
		res = net1000Init((void *) psysattr);
		if(res == 0)
		{
			zlog_info(c, "Net1000初始化完成");
			zlog_info(c, "启动Net1000线程");
			ser_sta.net_sta = 1;
			res = createNet1000Thread(argv[1]);
		    if(res != 0)
		    {
		    	zlog_error(c, "Net1000线程创建失败");
		        exit(EXIT_FAILURE);
		    }
		}
	}
	/////////////////////////////////////////////////////////////
	/*@brief
	 * wsf
	 * Serial232
	 */
//	if(0)
	if((psysattr->commparam.communication_protocols == CP_MB_RTU) && (s232_en))
	{
		zlog_info(c, "Serial232正在初始化...");
		res = serial232Init((void *)psysattr);
		if(res == 0)
		{
			zlog_info(c, "Serial232初始化完成");
			zlog_info(c, "启动Serial232线程!");
			ser_sta.rs232_sta = 1;
			res = createSerial232Thread();
			if(res != 0)
			{
				zlog_error(c, "Serial232线程创建失败");
				exit(EXIT_FAILURE);
			}
		}
	}
	//////////////////////////////////////////////////////////
	/*@brief
	 * wsf
	 * Zigbee
	 */
	if((psysattr->commparam.downlink_comm_interface == DCI_ZIGBEE) && (zigbee_en))
	{
		zlog_info(c, "ZigBee正在初始化...");
		res = zbInit((void *) psysattr);
		if(res == 0)
		{
			zlog_info(c, "ZigBee初始化完成");
			zlog_info(c, "启动ZigBee线程");
			ser_sta.zigbee_sta = 1;
			res = createZbThread(argv[1]);
		    if(res != 0)
		    {
		    	zlog_error(c, "ZigBee线程创建失败");
		        exit(EXIT_FAILURE);
		    }
		}
	}


	//////////////////////////////////////////////////////////
    printf( "输入 'q' 退出 或者输入 'h'获取帮助!\n" );
    bDoExit = 0;
	do
    {
        printf( "> " );
        cCh = getchar(  );
        switch ( cCh )
        {
        case 's':
        	printf("[提示]new_RTU基本信息：\n");
//        	printf("%s\n",);
//        	printf("%s\n",paramConfig->mainParamconfig.StorePath);
//        	printf("%s\n",paramConfig->mainParamconfig.ProgramName);
//        	printf("%s\n",paramConfig->mainParamconfig.StationName);
//        	printf("The device ID = %d\n",paramConfig->mainParamconfig.DeviceId);
        	break;
        case 'q':
            bDoExit = 1;
            break;
        case 'h':
            printf( "金时P90A_newRTU帮助:\n" );
            printf( "  'd' ... disable protocol stack.\n" );
            printf( "  'e' ... enabled the protocol stack.\n" );
            printf( "  's' ... 显示new_RTU基本信息：\n" );
            printf( "  'q' ... quit application.\n" );
            printf( "  'h' ... this information.\n" );
            printf( "\n" );
            printf( "Copyright 2015 GTP <gtpwsf@sina.com>\n" );
            break;
        default:
            if( !bDoExit && ( cCh != '\n' ) )
            {
                printf( "输入了非法命令 '%c'，请重新输入!\n", cCh );
            }
            break;
        }

        /* eat up everything untill return character. */
        while( !bDoExit && ( cCh != '\n' ) )
        {
            cCh = getchar(  );
        }
    }
    while( !bDoExit );
	// 取消并join数据库线程
	databaseThreadCancel();
	if(ser_sta.aidi_sta)
		AIDIThreadCancel();
	if(ser_sta.net_sta)
		net1000ThreadCancel();
	if(ser_sta.zigbee_sta)
//		serialZigbeeCancel();
		cancelZbThread();
	if(ser_sta.rs232_sta)
		serial232ThreadCancel();
	if(ser_sta.gprs_sta)
		serialGPRSThreadCancel();


	zlog_info(c, "正在释放内存...");
	for(n = 0; n < 17; n ++)
	{
		if(mb_mapping[n] !=NULL)
			modbus_mapping_free(mb_mapping[n]); // modbus_mapping_t
//		if(pexbuffer[n] != NULL)
//		{
//			free(pexbuffer[n]);
//			pexbuffer[n] = NULL;
//		}
//			mb_mapping_free(mb_mapping[n]);
	}
	if(ser_sta.net_sta)
		net1000Free();
	if(ser_sta.rs232_sta)
		serial232Free();
	if(ser_sta.zigbee_sta)
		zbFree();
	if(ser_sta.gprs_sta)
		serialGprsFree();
	zlog_info(c, "*********************************************");
	zlog_info(c, "**************RTU程序正常退出****************");
	zlog_info(c, "*********************************************");
	zlog_fini();
	exit(EXIT_SUCCESS);
}

//int testCopy(char * file)
//{
//	char block[1024];
//	int in ,out;
//	int nread, uu;
//	uu = 0;
//	in = open("newRTU_Bottle_x86", O_RDONLY);
//	out = open("newRTU_Bottle_x86_bak", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH);
//	while((nread = read(in, block, 1024)) > 0)
//	{
////		printf("0x%02X ", c);
//		if(uu == 0)
//		{
//			int n = 0;
////			block[n] = 0x01;
//			for(n = 0; n < nread; n ++)
//				printf("0x%02X ", block[n]);
//			printf("\n");
//			uu = 1;
//		}
//		write(out, block, nread);
//	}
//	printf("\n");
//	return 1;
//}
