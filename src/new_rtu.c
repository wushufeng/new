/*
 * new_rtu.c
 *
 *  Created on: 2015年3月20日
 *      Author: wsf
 */
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>


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


#define MAXBUFSIZE 512
char AppPathFileName[MAXBUFSIZE];
char AppPath[MAXBUFSIZE];
char AppFileName[32];
char AppOptinFile[MAXBUFSIZE];//配置文件路径
char SysIniFile[MAXBUFSIZE];//系统ini文件


//extern exchangebuffer *pexbuffer[17];
E1_sys_attribute *psysattr;
//extern oil_well *poilwell[16];
//pthread_mutex_t holdingReg_mutex;
int main(int argc, char *argv[])
{

	int res;
	char cCh;
	char bDoExit ;
	int n;
	// zlog
	if((res = Zlog_init()) != 0)
		return res;
	if((res = Zlog_get_category()) != 0)
		return res;
//	res = zlog_init("rtulog.conf");
//	if (res) {
//		printf("RTU日志初始化失败\n");
//		return -1;
//	}
//	c = zlog_get_category("my_cat");
//	if (!c) {
//		printf("获取RTU日志类型失败\n");
//		zlog_fini();
//		return -2;
//	}
////	 创建holdingreg全局锁
//    res = pthread_mutex_init(&holdingReg_mutex,NULL);
//    if(res != 0)
//    {
//        perror("holdingReg_mutex initialization failed");
//        exit(EXIT_FAILURE);
//    }
////     开辟20000个空间,用于mb保持寄存器
	zlog_info(c, "**************RTU程序启动*****************");
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
		zlog_error(c, "MBMapping数据空间错误!");

	// 加载A11数据,该指针指向mb保持寄存器
	psysattr = LoadConfigA11();
	zlog_info(c, "启动数据同步线程!");
	res = createDatabaseThread();
	if(res != 0)
	{
		zlog_error(c, "启动数据同步失败!");
		exit(EXIT_FAILURE);
	}
//	printf("[提示]创建sqlite库!\n");
//	testCreateTables();
//	databaseInsert(pexbuffer[0]);
//	showRecordsByTime(0);
//	printf("[提示] 数据库调试完成!\n");
	/* @brief
	 * wsf
	 * 创建aidi线程
	 */
//	if(0)
	{
		zlog_info(c, "启动AIDI线程!");
		res = createAidiThread();
		if(res != 0)
		{
			zlog_error(c, "AIDI线程创建失败!");
			exit(EXIT_FAILURE);
		}
	}
	/*@brief
	 * wsf
	 * TCP
	 */
		//////////////////////////////////////////////////////////
//    if(0)
	if(psysattr->commparam.communication_protocols == CP_MB_TCPIP)
	{
		zlog_info(c, "Net1000正在初始化...");
		res = net1000Init((void *) psysattr);
		if(res == 0)
		{
			zlog_info(c, "Net1000初始化完成!");
			zlog_info(c, "启动Net1000线程!");
			res = createNet1000Thread();
		    if(res != 0)
		    {
		    	zlog_error(c, "Net1000线程创建失败!");
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
	if(psysattr->commparam.communication_protocols == CP_MB_RTU)
	{
		zlog_info(c, "Serial232正在初始化...");
		res = serial232Init((void *)psysattr);
		if(res == 0)
		{
			zlog_info(c, "Serial232初始化完成!");
			zlog_info(c, "启动Serial232线程!\n");
			res = createSerial232Thread();
			if(res != 0)
			{
				zlog_error(c, "Serial232线程创建失败!");
				exit(EXIT_FAILURE);
			}
		}
	}
	//////////////////////////////////////////////////////////
	/*@brief
	 * wsf
	 * Zigbee
	 */
	if(psysattr->commparam.downlink_comm_interface == DCI_ZIGBEE)
	{
		zlog_info(c, "ZigBee正在初始化...");
		res = serialZigbeeInit((void *) psysattr);
		if(res == 0)
		{
			zlog_info(c, "ZigBee初始化完成!");
			zlog_info(c, "启动ZigBee线程!");
			res = createZigbeeThread();
		    if(res != 0)
		    {
		    	zlog_error(c, "ZigBee线程创建失败!");
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
	AIDIThreadCancel();
	net1000ThreadCancel();
	serialZigbeeCancel();
//	serial232ThreadCancel();
//	serialGPRSThreadCancel();


//	if(psysattr != NULL)
//		free(psysattr);
	for(n = 0; n < 16; n ++)
	{
		if(mb_mapping[n] !=NULL)
			modbus_mapping_free(mb_mapping[n]);
	}
	zlog_info(c, "正在释放内存...");
	net1000Free();
//	serial232Free();
	serialZigbeeFree();
	serialGprsFree();
	zlog_info(c, "**************RTU程序正常退出*****************");
	zlog_fini();
	exit(EXIT_SUCCESS);
}
