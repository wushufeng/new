/*
 * SerialGPRS.h
 *
 *  Created on: 2015年6月8日
 *      Author: wsf
 */

#ifndef SRC_SEVER_SERIALGPRS_SERIALGPRS_H_
#define SRC_SEVER_SERIALGPRS_SERIALGPRS_H_



int serialGprsInit(void *obj);
int createGprsThread(void);
void serialGprsFree();
int serialGPRSThreadCancel(void);
#endif /* SRC_SEVER_SERIALGPRS_SERIALGPRS_H_ */
