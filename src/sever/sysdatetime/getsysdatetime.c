/*
 * getsysdatetime.c
 *
 *  Created on: 2015年3月9日
 *      Author: wsf
 */


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "getsysdatetime.h"
#include "../../A11_sysAttr/a11sysattr.h"

//int main()
//{
//    char sys_datetime[11];
//    sys_local_date *p_sys_date;
//    sys_local_time *p_sys_time;
//    p_sys_date = getSysLocalDate();
//    sprintf(sys_datetime, "%04x_%02x_%02x",p_sys_date->local_year,p_sys_date->local_month,p_sys_date->local_day);
//    printf("%s\n",sys_datetime);
//
//    p_sys_time = getSysLocalTime();
//    sprintf(sys_datetime, "%02x:%02x:%02x",p_sys_time->local_hour,p_sys_time->local_minitue,p_sys_time->local_second);
//    printf("%s\n",sys_datetime);
///*
//    char sys_date[11];
//    char sys_time[9];
//    time_t nowtime = time(NULL);
//    struct tm *now = localtime(&nowtime);
////    printf("now: %04d-%02d-%02d %02d:%02d:%02d\nunix time: %ld\n",
////    now->tm_year+1900, now->tm_mon+1, now->tm_mday,
////    now->tm_hour, now->tm_min, now->tm_sec, (long)nowtime);
//    sprintf(sys_date,"%04d_%02d_%02d",now->tm_year+1900, now->tm_mon+1, now->tm_mday);
//    sprintf(sys_time,"%02d:%02d:%02d",now->tm_hour, now->tm_min, now->tm_sec);
//    sprintf(sys_date, "%02x_%02x",0x20,0x15);
//    printf("%s\n",sys_date);
//    printf("%s\n",sys_time);
//    */
//
//    free(p_sys_date);
//    free(p_sys_time);
//    return 0;
//}

sys_local_date* getSysLocalDate(void)
{
    sys_local_date *p_sys_date;

    time_t nowtime = time(NULL);
    struct tm *now = localtime(&nowtime);
    char temp = 0;

    p_sys_date = (sys_local_date *)malloc(sizeof(sys_local_date));
    if (p_sys_date == NULL) {
        return NULL;
    }
/*    sprintf(sys_date,"%04d%02d%02d",now->tm_year+1900, now->tm_mon+1, now->tm_mday);

    temp = (sys_date[0] & 0x0F) << 4;
	temp += sys_date[1] & 0x0F;
	p_local_date->local_year = temp;
	p_local_date->local_year = p_local_date->local_year << 8;
	temp = (sys_date[2] & 0x0F) << 4;
	temp += sys_date[3] & 0x0F;
	p_local_date->local_year += temp;

	temp = (sys_date[4] & 0x0F) << 4;
	temp += sys_date[5] & 0x0F;
	p_local_date->local_month = temp;

	temp = (sys_date[6] & 0x0F) << 4;
	temp += sys_date[7] & 0x0f;
	p_local_date->local_day = temp;*/

    temp = (now->tm_year+1900) / 100;
    p_sys_date->local_year = DEC2BCD(temp);
    p_sys_date->local_year = p_sys_date->local_year << 8;
    temp = (now->tm_year+1900) % 100;
    p_sys_date->local_year += DEC2BCD(temp);

    temp = now->tm_mon + 1;
    p_sys_date->local_month = DEC2BCD(temp);

    temp = now->tm_mday;
    p_sys_date->local_day = DEC2BCD(temp);

	return p_sys_date;
}
///*
// *  设置本地日期
// */
//int setSyslocaldate(void *obj)
//{
//	time_t timer;
//	struct tm *t_tm;
//    struct timeval tv;
////    struct rtc_time tm;
//    time_t timep;
//    int res;
////	unsigned char tempH,tempL;
////	E1_sys_attribute *p_sysattr = (E1_sys_attribute *)obj;
//
//	time(&timer);
//	t_tm = localtime(&timer);
//
////	tempL = BCD2DEC(*(unsigned char*)&p_sysattr->baseinfo.sys_date[0]);					// 内存中高字节在低地址
////	tempH = BCD2DEC(*((unsigned char*)&p_sysattr->baseinfo.sys_date[0] +1));
//////	tempH = BCD2DEC(p_sysattr->baseinfo.sys_date[0] >> 8);
//////	tempL = BCD2DEC(p_sysattr->baseinfo.sys_date[0] & 0x00F0);
////	t_tm->tm_year = tempH * 100 + tempL - 190;											// 年
////
////	tempL = p_sysattr->baseinfo.sys_date[1] & 0x00FF ;									// 月
////	t_tm->tm_mon = BCD2DEC(tempL) - 1;
////
////	tempL = p_sysattr->baseinfo.sys_date[2] & 0x00FF ;									// 日
////	t_tm->tm_mday = BCD2DEC(tempL) - 1;
////	t_tm->tm_min --;
//	t_tm->tm_min = 1;
//    timep = mktime(t_tm);
//    tv.tv_sec = time(NULL);
////    tv.tv_sec = timep;
//    tv.tv_usec = 0;
////    if(settimeofday (&tv, (struct timezone *) 0) < 0)
//    res = settimeofday(&tv,NULL);
//    if(res < 0)
//    {
//    printf("Set system datatime error! \n");
//    return -1;
//    }
//    else
//    	printf("Set system date time OK! \n");
//
//	return 0;
//}
sys_local_time* getSysLocalTime(void)
{
	sys_local_time *p_sys_time;

//    char sys_time[9];

    time_t nowtime = time(NULL);
    struct tm *now = localtime(&nowtime);
    char temp = 0;

    p_sys_time = (sys_local_time *)malloc(sizeof(sys_local_time));
    if (p_sys_time == NULL) {
        return NULL;
    }
/*    sprintf(sys_time,"%02d%02d%02d",now->tm_hour, now->tm_min, now->tm_sec);

    temp = (sys_time[0] & 0x0F) << 4;
	temp += sys_time[1] & 0x0F;
	p_local_time->local_hour = temp;

	temp = (sys_time[2] & 0x0F) << 4;
	temp += sys_time[3] & 0x0F;
	p_local_time->local_minitue = temp;

	temp = (sys_time[4] & 0x0F) << 4;
	temp += sys_time[5] & 0x0F;
	p_local_time->local_second = temp;*/

    temp = now->tm_hour;
    p_sys_time->local_hour = DEC2BCD(temp);

    temp = now->tm_min;
    p_sys_time->local_minitue = DEC2BCD(temp);

    temp = now->tm_sec;
    p_sys_time->local_second = DEC2BCD(temp);

	return p_sys_time;
}

unsigned char DEC2BCD(const unsigned char decDat)
{
	return(((decDat/10)<<4)+(decDat%10));
}
unsigned char BCD2DEC(const unsigned char bcdDat)
{
	return((bcdDat >> 4) *10 + (bcdDat & 0x0F));
}
int getSysLocalDateTime(void *obj)
{
	static int flag_time = 0;
	E1_sys_attribute *psysattr = (E1_sys_attribute *)obj;
	sys_local_date *pLocalDate;
	sys_local_time *pLocalTime;
	pLocalDate = getSysLocalDate();
	if(pLocalDate == NULL)
		return -1;
	psysattr->baseinfo.sys_date [0] = pLocalDate->local_year;
	psysattr->baseinfo.sys_date [1] = pLocalDate->local_month;
	psysattr->baseinfo.sys_date [2] = pLocalDate->local_day;
	if(pLocalDate != NULL)
		free(pLocalDate);
	pLocalTime = getSysLocalTime();
	if(pLocalTime == NULL)
		return -1;
	psysattr->baseinfo.sys_time [0] = pLocalTime->local_hour;
	psysattr->baseinfo.sys_time [1] = pLocalTime->local_minitue;
	psysattr->baseinfo.sys_time [2] = pLocalTime->local_second;
	if(!flag_time)
	{
		psysattr->baseinfo.day_of_start_time[0] = psysattr->baseinfo.sys_time [0];
		psysattr->baseinfo.day_of_start_time[1] = psysattr->baseinfo.sys_time [1];
		psysattr->baseinfo.day_of_start_time[2] = psysattr->baseinfo.sys_time [2];
		flag_time = 1;
	}
	if(pLocalTime != NULL)
		free(pLocalTime);
	return 0;
}
int getDynagraphDateTime(void *obj)
{
	oil_well *ptr = (oil_well *)obj;
	sys_local_date *pLocalDate;
	sys_local_time *pLocalTime;
	pLocalDate = getSysLocalDate();
	if(pLocalDate == NULL)
		return -1;
	ptr->load_displacement.dynagraph.collection_datetime [0] = pLocalDate->local_year;
	ptr->load_displacement.dynagraph.collection_datetime [1] = pLocalDate->local_month;
	ptr->load_displacement.dynagraph.collection_datetime [2] = pLocalDate->local_day;
	if(pLocalDate != NULL)
		free(pLocalDate);
	pLocalTime = getSysLocalTime();
	if(pLocalTime == NULL)
		return -1;
	ptr->load_displacement.dynagraph.collection_datetime [3] = pLocalTime->local_hour;
	ptr->load_displacement.dynagraph.collection_datetime [4] = pLocalTime->local_minitue;
	ptr->load_displacement.dynagraph.collection_datetime [5] = pLocalTime->local_second;

	if(pLocalTime != NULL)
		free(pLocalTime);
	return 0;
}
