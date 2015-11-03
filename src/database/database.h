/*
 * database.h
 *
 *  Created on: 2015年3月26日
 *      Author: wsf
 */

#ifndef SRC_DATABASE_DATABASE_H_
#define SRC_DATABASE_DATABASE_H_
#include "../def.h"
#include "../modbus/modbus.h"
#include "../A11_sysAttr/a11sysattr.h"
enum {
    TCP,
    UDP,
    RTU,
	ZIGBEE,
	GPRS
};

extern const uint16_t UT_BITS_ADDRESS;
extern const uint16_t UT_BITS_NB;
extern const uint8_t UT_BITS_TAB[];

extern const uint16_t UT_INPUT_BITS_ADDRESS;
extern const uint16_t UT_INPUT_BITS_NB;
extern const uint8_t UT_INPUT_BITS_TAB[];

extern const uint16_t UT_REGISTERS_ADDRESS;
/* Raise a manual exception when this adress is used for the first byte */
extern const uint16_t UT_REGISTERS_ADDRESS_SPECIAL;
extern const uint16_t UT_REGISTERS_NB;
extern const uint16_t UT_REGISTERS_TAB[];
/* If the following value is used, a bad response is sent.
   It's better to test with a lower value than
   UT_REGISTERS_NB_POINTS to try to raise a segfault. */
extern const uint16_t UT_REGISTERS_NB_SPECIAL;

//extern const uint16_t UT_INPUT_REGISTERS_ADDRESS = 0x08;
//extern const uint16_t UT_INPUT_REGISTERS_NB = 0x1;
extern const uint16_t UT_INPUT_REGISTERS_ADDRESS ;
extern const uint16_t UT_INPUT_REGISTERS_NB;
extern const uint16_t UT_INPUT_REGISTERS_TAB[];

extern const float UT_REAL;
extern const uint32_t UT_IREAL;

extern modbus_mapping_t *mb_mapping[17];
extern const char *A11filename ;



typedef struct
{
	unsigned char dg_online;														// 是否功图在线
	unsigned char dg_OK;															// 功图是否采集完毕
	time_t dg_time;																	// 功图采集时间
	unsigned char dg_group;															// 功图组号
	unsigned char elec_online;														// 电参是否在线
	unsigned char elec_OK;															// 电参数据读取是否完毕
	unsigned char elec2_online;														// 电参是否在线
	unsigned char elec2_OK;															// 电参数据读取是否完毕
	unsigned char elec_group;														// 电参组号
	unsigned char saved_flag;														// 是否保存过
	unsigned char upload_flag;														// 是否上传过
	load_displacement loaddata;
	time_t device_time;																// 其他仪器采集时间
	unsigned char oil_pressure;														// 油压仪表
	unsigned char casing_pressure;													// 套压仪表
	unsigned char back_pressure;													// 回压仪表
	unsigned char wellhead_oil_temp;												// 井口油温仪表
	unsigned char load;																// 载荷仪表
	unsigned char displacement;														// 位移仪表
	device_information device_infor;												// 仪表基本信息
}__attribute__((packed)) exchangebuffer;
/**
 * @brief
 * 用于仪器信息表的维护
 */
typedef struct {
instrument_parameter instrument;
time_t start_time;
time_t interval;
} instrument_update;
typedef struct {
time_t start_time;
time_t interval;
} instrument_timeout;

int mbMappingNew(void);

int databaseThreadCancel(void);
int createDatabaseThread(void);

#endif /* SRC_DATABASE_DATABASE_H_ */
