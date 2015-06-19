/*
 * a5d35-ai.c
 *
 *  Created on: 2015年6月9日
 *      Author: wsf
 */

/*
 * Software triggers are an ADC operating mode where the software starts the conversion.
This feature is exposed by IIO through the following files:
in_voltageX_raw: raw value of the channel X of the ADC
in_voltage_scale: value you have to multiply in_voltageX_raw with to have a value in microvolts
Reading into in_voltageX_raw will perform a software trigger on the ADC, then block until the conversion is completed, and finally return the value of this conversion.
Please note that conversions are done one channel at a time.
Here is the output on the AT91SAM9 console that shows an ADC measure when a 3V DC power supply is connected between analog ground AGND and ADC input 0 AD0 pins:
root@at91sam9g20ek:~# cat /sys/bus/iio/devices/iio\:device0/in_voltage0_raw
948
root@at91sam9g20ek:~# cat /sys/bus/iio/devices/iio\:device0/in_voltage_scale
3222.000000
We can calculate the result: 948 x 3222 = 3.0V
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include<sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <string.h>




//#include "../../modbus/modbus.h"
#include "../../A11_sysAttr/a11sysattr.h"
#include "a5d35ai.h"


extern E1_sys_attribute *psysattr;
unsigned short int ai_data[10];
pthread_t aidi_thread;

static int aidiThreadFunc(void *arg);
static void modbus_set_float_swapped(float f, unsigned short int *dest);
static int  a5d3ad_get_ad(unsigned short int *dest, int index, unsigned short int nb_bits);



int createAidiThread(void)
{
	int res;
//	res = pthread_create(&aidi_thread, NULL, (void*)&aidiThreadFunc, NULL);
	res = pthread_create(&aidi_thread, NULL, (void *)&aidiThreadFunc, NULL);
    if(res != 0)
    {
//    	fprintf(stderr, "[错误]AIDI线程创建失败: %s\n", modbus_strerror(errno));
    	printf("[错误]AIDI线程创建失败!\n");
//        perror("Thread creation failed");
        return (EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}
static int aidiThreadFunc(void *arg)
{
	float tempdata = 0;
	unsigned short int voltage_scale;
	unsigned short int tempun16 = 0;
//	int n;
	for(;;)
	{
//		a5d3ad_get_ad(ai_data, 0, 10);

		voltage_scale = ai_data[0];

//		printf("voltage_scale = %d ", voltage_scale);
//		for(n =1; n < 10; n ++)
//			printf("ai_data[ %d] = %d ", n, ai_data[n]);
//		printf("\n");

		tempdata = (float)(ai_data[1] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a1_oil_pressure);
		tempdata =(float)(ai_data[2] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a2_casing_pessure);
		tempdata = (float)(ai_data[3] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a3_back_pressure);
		tempdata = (float)(ai_data[4] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a4_wellhead_oil_temperature);
		tempdata =(float)(ai_data[5] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a5_now_polished_rod_load);
		tempdata = (float)(ai_data[6] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a6_displacement);
		tempdata = (float)(ai_data[7] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a7_custom);
		tempdata = (float)(ai_data[8] * voltage_scale / 1000000.0);
		modbus_set_float_swapped(tempdata, psysattr->aidi_param.a8_custom);

		tempdata = (float)(ai_data[9] * voltage_scale / 1000000.0 * 2.0);
		tempun16 = (unsigned short int) (tempdata * 100);
		psysattr->baseinfo.ram_vol = tempun16;

		usleep(100);				// 100ms 延时
//		sleep(10);
	}
	return 0;
}




//void itoa(int i,char*string)
//{
//	int power,j;
//	j=i;
//	for(power=1;j>=10;j/=10)
//		power*=10;
//
//	for(;power>0;power/=10)
//	{
//		*string++='0'+i/power;
//		i%=power;
//	}
//	*string='\0';
//}
static int  a5d3ad_get_ad(unsigned short int *dest, int index, unsigned short int nb_bits)
{
//	char *htmldata;
//	char delims[] = "&";
//	char *result= NULL;
//	int slen,adchannel;
	char *adc[12]={"0","1","2","3","4","5","6","7","8","9","10","11"};

	char ad[128]="cat /sys/devices/ahb.0/apb.1/f8018000.adc/iio:device0/in_voltage";
	char adt[128];
	char ad1[]="_raw";
//	char adchannels[20];


	FILE *fp;
	char tmp[1024];
//	float valscale;
	int val,valtmp;
	int i;



	fp=popen("cat /sys/devices/ahb.0/apb.1/f8018000.adc/iio:device0/in_voltage_scale","r");
	if(!fp)
	{
		return -1;
	}
	while (fgets(tmp, sizeof(tmp), fp) != NULL)
	{
		if (tmp[strlen(tmp) - 1] == '\n')
		{
			tmp[strlen(tmp) - 1] = '\0'; //去除换行符
		}

	}

	val = atoi(tmp);
	dest[0] = val;

//	printf("in_voltage_scale=%d\n",val);
	pclose(fp);


	for(i=0;i<(nb_bits - 1);i++)
	{
		//拼接 "cat /sys/devices/ahb.0/apb.1/f8018000.adc/iio:device0/in_voltage" + adchannel + "_raw"
		memset(adt,0,sizeof(adt));
		strncpy(adt,ad,128);
		strcat(adt,adc[i]);
		strcat(adt,ad1);

//		printf("--------------------------------\n");
		fp=popen(adt,"r");
		if(!fp)
		{
			return -1;
		}
		while (fgets(tmp, sizeof(tmp), fp) != NULL)
		{
			if (tmp[strlen(tmp) - 1] == '\n')
			{
				tmp[strlen(tmp) - 1] = '\0'; //去除换行符
			}

		}
		valtmp = atoi(tmp);
		//printf("in_voltage0_raw=%d\n",valtmp);
		//valad = val * valtmp;
		dest[i + 1] = valtmp;
//		printf("ad channel %d voltage is %d\n",i,valtmp);
		pclose(fp);


	}
//	if(fp != NULL)
//		close(fp);
//	close(fp);
//	printf("end\n");

	return 0;
}
/* Set a float to 4 bytes in Modbus format */
/*@brief
 * 将4bytes的float数据写入modbus寄存器
 * 浮点数顺序为：交换swapped
 */
static void modbus_set_float_swapped(float f, unsigned short int *dest)
{
    uint32_t i = 0;

    memcpy(&i, &f, sizeof(unsigned int));
    dest[1] = (unsigned short int)i;
    dest[0] = (unsigned short int)(i >> 16);
}
