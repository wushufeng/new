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
int mbMappingNew(void);

int A11toHoldingReg(void *p_sysattr);

int createDatabaseThread(void);

int mbWriteSigleRegister(uint16_t address, int data);
#endif /* SRC_DATABASE_DATABASE_H_ */
