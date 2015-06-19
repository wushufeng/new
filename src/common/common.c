/*
 * common.c
 *
 *  Created on: 2015年6月15日
 *      Author: wsf
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "common.h"


void setSysTime(struct tm Currenttime)
{
	struct timeval *tv;//当前时间
	struct timezone *tz;//当前时区
	tv=(struct timeval *)malloc(sizeof(struct timeval));
	tz=(struct timezone *)malloc(sizeof(struct timezone));
	tv->tv_sec = mktime((struct tm*) &Currenttime);
	tz =NULL;
	settimeofday(tv,tz);
	free(tv);
	free(tz);
}

//最快的字符串倒置方法
void rever(char s[])
{
    int len=strlen(s);
    int i=0;
    int j=len-1;
    char c;
    while (i<j)
    {
        c=s[i];
        s[i]=s[j];
        s[j]=c;
        i++;
        j--;
    }
}

void myitoa(int n, char s[],int b)
{
    int i=0;
    int sign=0;
    if (b==0) b=10;
    //判断符号
    if((sign=n)<0)
       n=-n;

    //分解生成逆序字符串
    do
    {
    	s[i++]=n%b+'0';
    } while ((n/=b)>0);
    if(sign<0) s[i++]='-';
    //结尾注意添加\0
    s[i]='\0';
    rever(s);
}

char *itoa(int n, char *s, int b)  /* b按什么进制转换二进制 2 八进制 8 十进制 10 十六进制16 */
{
    int c;
    switch(b)
    {
	case 10: /* decimal */
		sprintf(s, "%d", n);
		break;
	case 16: /* hexadecimal */
		sprintf(s, "%#x", n);
		break;
	case  8: /* octal */
		sprintf(s, "%#o", n);
		break;
	case  2: /* binary */
		for (c = 16 - 1; c >= 0; c--, n >>= 1)
	s[c] = (01 & n) + '0';
		s[16] = '\0';
		break;
	default:
		printf("Run Time Error: itoa() base argument not allowed\n");
		//exit(2);
    }
    return s;
}

//将单字节BCD码转化成十进制值
char BCDToDec(char BCD)
{
	char Dec;
	Dec = (BCD / 0x10)*10 + BCD % 0x10;
	return Dec;
}

/*
 * 获取当前应用程序的完整路径
 *
 */
int getAppPathFileName(char * AppPathFileName)
{
	const int MAXBUFSIZE=512;
	int count;
	count = readlink( "/proc/self/exe", AppPathFileName, MAXBUFSIZE );
	if ( count < 0 || count >= MAXBUFSIZE )
	{
		printf( "[错误]获取路径失败\n" );
		return( EXIT_FAILURE );
	}

	AppPathFileName[ count ] = '\0';
	printf( "[提示]当前应用软件完整路径 -> [%s]\n", AppPathFileName );
	return( EXIT_SUCCESS );
}
/*
 * 获取当前应用程序的绝对路径不含文件名
 *
 */
int getAppPath(char * AppPath)
{
	const int MAXBUFSIZE=512;
	int count,i;
	count = readlink( "/proc/self/exe", AppPath, MAXBUFSIZE );
	if ( count < 0 || count >= MAXBUFSIZE )
	{
		printf( "[错误]获取路径失败\n" );
		return( EXIT_FAILURE );
	}

	for (i=count-1;i>0;i--)
	{
		if (AppPath[i]=='/')
		{
			AppPath[i] = '\0';
			break;
		}
		AppPath[i] = '\0';
	}
	printf( "[提示]当前应用软件路径 -> [%s]\n", AppPath );
	return( EXIT_SUCCESS );
}
/*
 * 获取当前应用程序的文件名
 *
 */
int getAppName(char * AppFileName)
{
	const int MAXBUFSIZE=512;
	int i,s;
	char AppPath[MAXBUFSIZE];

	int count;
	count = readlink( "/proc/self/exe", AppPath, MAXBUFSIZE );
	if ( count < 0 || count >= MAXBUFSIZE )
	{
		printf( "[错误]获取路径失败\n" );
		return( EXIT_FAILURE );
	}

	for (i=count-1;i>0;i--)
	{
		if (AppPath[i]=='/')
		{
			s=i+1;
			break;
		}
		//AppPath[i] = '\0';
	}
	for(i=s;i<count;i++)
	{
		AppFileName[i-s] =AppPath[i];
	}
	AppPath[i] = '\0';
	printf( "[提示]当前应用软件名 -> [%s]\n", AppFileName );
	return( EXIT_SUCCESS );
}


/*
 * 入口参数：sec:查询状态等待超时时间 秒
 * 入口参数：flag 读写标记，1为发送；0为接收
 * 入口参数：sockfd 设备描述符
 */
int mySelect(int sockfd,int sec, int flag)
{
	struct timeval tv;
	fd_set readfds, writefds, exceptfds;
	int Result = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	if(flag == 0)
	{
		FD_SET(sockfd, &readfds);
		FD_SET(sockfd, &exceptfds);
		Result = select(sockfd+1, &readfds, NULL, &exceptfds, &tv);
		if(Result == -1)
		{
			perror("查询接收数据包发生错误，需要重新绑定端口！");
			close(sockfd);
			//exit(0);
			return 255;//需要关闭udp 服务端口，多一段时间重新打开端口
		}
		else if(Result == 0)
		{
			printf("接收数据超时。\n");
			return -1;
		}
		else if(Result > 0 && FD_ISSET(sockfd, &readfds) > 0)
			return 0;
		else
			return -2;
	}
	else if(flag == 1)
	{
		FD_SET(sockfd, &writefds);
		FD_SET(sockfd, &exceptfds);
		Result = select(sockfd+1, NULL, &writefds, &exceptfds, &tv);
		if(Result == -1)
		{
			perror("查询接收数据包发生错误，需要重新绑定端口！");
			close(sockfd);
			return 255;//需要关闭udp 服务端口，多一段时间重新打开端口
			//exit(0);
		}
		else if(Result == 0)
		{
			printf("发送数据包超时！\n");
			return -1;
		}
		else if(Result > 0 && FD_ISSET(sockfd, &writefds) > 0)
		{
			return 0;
		}
		else
			return -2;
	}
	return 1;
}

/*
 * 获取未上传历史数据记录数量
 */
int GetNoReportHisCount()
{
	return 0;
}


//字符串替换函数，在字符串 string 中查找 source， 找到则替换为destination，找不到则返回NULL
char * replaceString (char * string, const char * source, const char * destination )
{
    char* sk = strstr (string, source);
    if (sk == NULL) return NULL;

    char* tmp;
    size_t size = strlen(string)+strlen(destination);

    char* newstr = (char*)calloc (1, size);
    if (newstr == NULL) return NULL;

    char* retstr = (char*)calloc (1, size);
    if (retstr == NULL)
    {
        free (newstr);
        return NULL;
    }

    snprintf (newstr, size, "%s", string);
    sk = strstr (newstr, source);

    while (sk != NULL)
    {
        int pos = 0;
        memcpy (retstr+pos, newstr, sk - newstr);
        pos += sk - newstr;
        sk += strlen(source);
        memcpy (retstr+pos, destination, strlen(destination));
        pos += strlen(destination);
        memcpy (retstr+pos, sk, strlen(sk));

        tmp = newstr;
        newstr = retstr;
        retstr = tmp;

        memset (retstr, 0, size);
        sk = strstr (newstr, source);
    }
    free (retstr);
    return newstr;

}

//字符串插入函数  在字符串string 中查找 source ， 找到后再 source之后插入字符串 destination， 找不到则返回 NULL
char * insertString (char * string, const char * source, const char * destination )
{
    char* sk = strstr (string, source);
    if (sk == NULL) return NULL;
    //char* tmp;
    size_t size = strlen(string)+strlen(destination);
    char* newstr = (char*)calloc (1, size);
    if (newstr == NULL) return NULL;

    char* retstr = (char*)calloc (1, size);
    if (retstr == NULL)
    {
        free (newstr);
        return NULL;
    }

    snprintf (newstr, size, "%s", string);

    sk = strstr (newstr, source);

    int pos = 0;
    sk += strlen(source);

    memcpy (retstr+pos, newstr, sk - newstr);
    pos += sk - newstr;

    memcpy (retstr+pos, destination, strlen(destination));
    pos += strlen(destination);
    memcpy (retstr+pos, sk, strlen(sk));

    free (newstr);
    return retstr;

}

char * strCutLen(char * str,int startindex,int length)
{
    int i;
    char * subStr;
    if (strlen(str)<(startindex+length)) length=strlen(str)-startindex;
    subStr =(char *)malloc(sizeof(char)* length);
    for (i=0;i<length;i++)
    {
        subStr[i]=str[startindex+i];
    }
    return subStr;
}

////计算参数线性转换系数及常数
//void calculateKB(struct ParameterInfo *pParameter)
//{
//	if (pParameter->ADFullValue!=pParameter->ADZeroValue)
//	{
//		pParameter->K=(pParameter->RGMax-pParameter->RGMin)/(pParameter->ADFullValue-pParameter->ADZeroValue);
//		pParameter->B= ((pParameter->RGMax+pParameter->RGMin)- pParameter->K *(pParameter->ADFullValue+pParameter->ADZeroValue))/2.0;
//	}
//	else
//	{
//		pParameter->K=1;
//		pParameter->B=0;
//	}
//}

//字符分割函数的简单定义和实现
void split( char **arr, char *str, const char *del)
{
	char *s =NULL;
	s=strtok(str,del);
	while(s != NULL)
	{
		*arr++ = s;
		s = strtok(NULL,del);
	}
}
//判断总共有多少个分隔符，目的是在main函数中构造相应的arr指针数组
int strSplitnum(char *str, const char* del)
{
	char *first = NULL;
	char *second = NULL;
	int num = 0;
	first = strstr(str,del);
	while(first != NULL)
	{
		second = first+1;
		num++;
		first = strstr(second,del);
	}
	return num;
}
