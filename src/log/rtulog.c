/*
 * rtulog.c
 *
 *  Created on: 2015年7月29日
 *      Author: wsf
 */


#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<time.h>

#include	"rtulog.h"

zlog_category_t *c;

int Zlog_init(void)
{
	int rc;
	rc = zlog_init("rtulog.conf");
	if (rc) {
		printf("RTU日志初始化失败\n");
		return -1;
	}
	return rc;
}

int Zlog_get_category(void)
{
	c = zlog_get_category("my_cat");
	if (!c) {
		printf("获取RTU日志类型失败\n");
		zlog_fini();
		return -2;
	}
	return 0;
}
