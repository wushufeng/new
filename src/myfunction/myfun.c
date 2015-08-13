/*
 * myfun.c
 *
 *  Created on: 2015年3月27日
 *      Author: wsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int myIPtoa(char *obj,char* ip,unsigned short int *a)
{
	char ipp[64] = {0};
	char *q;
	char *token;
//	char *temp = ":";

	strcpy(ipp,ip);
//	printf("%s \n",ipp);
	q = ipp;
	while((token = strsep (&q,obj))!=NULL)
	{
//		if(!memcmp(obj, temp, 1))
//		{
//			*a ++ = strtod(token,NULL);
//		}
//		else
		*a ++ = atoi(token);
	}
	return	 1;
}
