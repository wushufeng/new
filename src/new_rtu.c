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


E1_sys_attribute *psysattr;
//extern oil_well *poilwell[16];
//pthread_mutex_t holdingReg_mutex;
int main(int argc, char *argv[])
{

	int res;
	char cCh;
	char bDoExit ;
	int n;

////	 创建holdingreg全局锁
//    res = pthread_mutex_init(&holdingReg_mutex,NULL);
//    if(res != 0)
//    {
//        perror("holdingReg_mutex initialization failed");
//        exit(EXIT_FAILURE);
//    }
////     开辟20000个空间,用于mb保持寄存器

	getAppPathFileName((char*)AppPathFileName);
	getAppPath((char*)AppPath);
	getAppName((char*)AppFileName);

	if(mbMappingNew() == 0)
		printf("[提示]MBMapping数据空间准备就绪!\n");
	else
		printf("[错误]MBMapping数据空间错误!\n");
	// 加载A11数据,该指针指向mb保持寄存器
	psysattr = LoadConfigA11();
//	psysattr->baseinfo.ram_vol = 0x1234;
	printf("[提示]启动Database线程!\n");
	res = createDatabaseThread();
	if(res != 0)
	{
		printf("[错误]启动Database失败!\n");
		exit(EXIT_FAILURE);
	}
	printf("[提示]创建sqlite库!\n");
	openDatabase("./RTU.db");
	createDatabase(DBfd);
	closeDatabase(DBfd);
	printf("[提示] 完成!\n");
	/* @brief
	 * wsf
	 * 创建aidi线程
	 */
	printf("[提示]启动AIDI线程!\n");
	res = createAidiThread();
    if(res != 0)
    {
        perror("[错误]AIDI线程创建失败!\n");
        exit(EXIT_FAILURE);
    }
	/*@brief
	 * wsf
	 * TCP
	 */
		//////////////////////////////////////////////////////////
	if(psysattr->commparam.communication_protocols == CP_MB_TCPIP)
	{
		printf("[提示]Net1000正在初始化...\n");
		res = net1000Init((void *) psysattr);
		if(res == 0)
		{
			printf("[提示]Net1000初始化完成!\n");
			printf("[提示]启动Net1000线程!\n");
			res = createNet1000Thread();
		    if(res != 0)
		    {
		        perror("[错误]Net1000线程创建失败!\n");
		        exit(EXIT_FAILURE);
		    }
		}
	}
	/////////////////////////////////////////////////////////////
	/*@brief
	 * wsf
	 * Serial232
	 */
	if(psysattr->commparam.communication_protocols == CP_MB_RTU)
	{
		printf("[提示]Serial232正在初始化...\n");
		res = serial232Init((void *)psysattr);
		if(res == 0)
		{
			printf("[提示]Serial232初始化完成!\n");
			printf("[提示]启动Serial232线程!\n");
			res = createSerial232Thread();
			if(res != 0)
			{
				perror("Serial232线程创建失败!\n");
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
		printf("[提示]ZigBee正在初始化...\n");
		res = serialZigbeeInit((void *) psysattr);
		if(res == 0)
		{
			printf("[提示]ZigBee初始化完成!\n");
			printf("[提示]启动ZigBee线程!\n");
			res = createZigbeeThread();
		    if(res != 0)
		    {
		        perror("ZigBee线程创建失败!\n");
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
            printf("[提示]正在释放内存...\n");
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
//	if(psysattr != NULL)
//		free(psysattr);
	for(n = 0; n < 16; n ++)
	{
		if(mb_mapping[n] !=NULL)
			modbus_mapping_free(mb_mapping[n]);
	}
	net1000Free();
	serial232Free();
	serialZigbeeFree();
	serialGprsFree();
	printf("[提示]释放完毕,程序退出!\n");
	return 1;
}
