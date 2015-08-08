/*
 * rtulog.h
 *
 *  Created on: 2015年7月29日
 *      Author: wsf
 */

#ifndef SRC_LOG_RTULOG_H_
#define SRC_LOG_RTULOG_H_

#include "zlog.h"

extern zlog_category_t *c;

int Zlog_init(void);
int Zlog_get_category(void);

#endif /* SRC_LOG_RTULOG_H_ */
