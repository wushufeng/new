/*
 * common.h
 *
 *  Created on: 2015年6月15日
 *      Author: wsf
 */

#ifndef SRC_COMMON_COMMON_H_
#define SRC_COMMON_COMMON_H_

void setSysTime(struct tm Currenttime);
char *itoa(int n, char *s, int b);
char BCDToDec(char BCD);
int getAppPathFileName(char * AppPathFileName);
int getAppPath(char * AppPath);
int getAppName(char * AppFileName);
int mySelect(int sockfd,int sec, int flag);
int getNoReportHisCount();
char * insertString (char * string, const char * source, const char * destination );
char * replaceString (char * string, const char * source, const char * destination );
char * strCutLen(char * str,int startindex,int length);
//void calculateKB(struct ParameterInfo * Parameter);
void split( char **arr, char *str, const char *del);
int strSplitnum(char *str, const char*del)  ;

#endif /* SRC_COMMON_COMMON_H_ */
