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
//static size_t strlcpy(char *dest, const char *src, size_t dest_size)
//{
//    register char *d = dest;
//    register const char *s = src;
//    register size_t n = dest_size;
//
//    /* Copy as many bytes as will fit */
//    if (n != 0 && --n != 0) {
//        do {
//            if ((*d++ = *s++) == 0)
//                break;
//        } while (--n != 0);
//    }
//
//    /* Not enough room in dest, add NUL and traverse rest of src */
//    if (n == 0) {
//        if (dest_size != 0)
//            *d = '\0'; /* NUL-terminate dest */
//        while (*s++)
//            ;
//    }
//
//    return (s - src - 1); /* count does not include NUL */
//}
