/*
 * Net1000.h
 *
 *  Created on: 2015年3月25日
 *      Author: wsf
 */

#ifndef SRC_SEVER_NET1000_NET1000_H_
#define SRC_SEVER_NET1000_NET1000_H_



//int net1000Init(ParamConfiguration *obj);
int createNet1000Thread(void);
int net1000Init(void *obj);
int net1000Start();
void net1000Free();
#endif /* SRC_SEVER_NET1000_NET1000_H_ */
