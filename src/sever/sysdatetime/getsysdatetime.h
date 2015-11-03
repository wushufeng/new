/*
 * getsysdatetime.h
 *
 *  Created on: 2015年3月9日
 *      Author: wsf
 */

#ifndef SRC_SYSDATETIME_GETSYSDATETIME_H_
#define SRC_SYSDATETIME_GETSYSDATETIME_H_

/*
 * 根据A11要求系统日期占用3个地址，总共6个字节，而且数据格式为BCD码
 * 所以每个变量占用两个字节
 * 如当前日期为2015年3月6日
 * local_year = 0x2015;local_month = 0x0003;local_day = 0x0006
 */
typedef struct
{
    short int local_year;
    short int local_month;
    short int local_day;
}sys_local_date;
/*
 * 根据A11要求系统时间占用3个地址，总共6个字节，而且数据格式为BCD码
 * 所以每个变量占用两个字节
 * 如当前时间为12:52:24
 * local_year = 0x0012;local_month = 0x0052;local_day = 0x0024
 */
typedef struct
{
	short int local_hour;
	short int local_minitue;
	short int local_second;
}sys_local_time;
typedef struct
{
//	sys_local_time time;
//	sys_local_date date;
	unsigned char hour[2];
	unsigned char min[2];
	unsigned char sec[2];
	unsigned char year[2];
	unsigned char mon[2];
	unsigned char day[2];
}sys_date_time;

sys_local_date* getSysLocalDate(time_t syncTime);
//int setSyslocaldate(void *obj);
sys_local_time* getSysLocalTime(time_t syncTime);

int getSysLocalDateTime(void *obj);

int getDynagraphDateTime(void *obj, time_t syncTime);
int setSystemTime(void *dt);
unsigned char DEC2BCD(const unsigned char decDat);
unsigned char BCD2DEC(unsigned char bcdDat);
int tmStringToFourUint16(const char *p_tm, unsigned short int *p_bcd);

#endif /* SRC_SYSDATETIME_GETSYSDATETIME_H_ */
