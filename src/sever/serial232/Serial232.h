/*
 * Serial232.h
 *
 *  Created on: 2015年4月13日
 *      Author: wsf
 */

#ifndef SRC_SEVER_SERIAL232_SERIAL232_H_
#define SRC_SEVER_SERIAL232_SERIAL232_H_

int serial232Init(void *obj);
int createSerial232Thread(void);
void serial232Free();

#endif /* SRC_SEVER_SERIAL232_SERIAL232_H_ */
