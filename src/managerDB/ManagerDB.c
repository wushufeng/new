/*
 * ManagerDB.c
 *
 *  Created on: 2015年6月26日
 *      Author: wsf
 */


#include	<stdio.h>
#include	<sqlite3.h>
#include	<string.h>
#include	<stdlib.h>
#include	"ManagerDB.h"
#include	"../database/database.h"

const char *dbfile = ".DataBase/RTU.db";
sqlite3 *DBfd = NULL;

/*
 * 连接数据库
 * 入口：数据库文件路径
 * 返回：成功返回0，失败返回1，并打印错误信息
 */
int openDatabase(const char *dbfile)
{
	int res;

//	res = sqlite3_open(dbfile, &DBfd);
	res = sqlite3_open_v2(dbfile, \
			&DBfd, \
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, \
			NULL);
	if(res)
	{
		printf("[错误]无法打开数据库，错误号[%d]: %s\n", res, sqlite3_errmsg(DBfd));
	}
	return res;
}
/*
 * 断开数据库连接
 * 入口：数据库操作符
 * 返回：成功返回0，失败返回1，并打印错误信息
 */
int closeDatabase(sqlite3 *db)
{
	int res;

	res = sqlite3_close(db);
	if(res)
	{
		printf("[错误]无法打开数据库，错误号[%d]: %s\n", res, sqlite3_errmsg(db));
	}
	return res;
}
/*
 * 建立数据库数据表
 * 参数入口：数据库操作符
 * 返回：成功返回0，失败返回1，并打印错误信息
 */

int createDatabase(sqlite3 *db)
{
	int res;
	char *errmsg = NULL;
	const char *SQLCode = "CREATE TABLE 'carddata' ( \
			'ID'  INTEGER PRIMARY KEY, \
			'Wellname'  TEXT, \
			'Interval'  INTEGER, \
			'Manul' INTEGER ,\
			'Setdot'  INTEGER, \
			'ActualDot'  INTEGER, \
			'CollectDateTime'  INTEGER, \
			'Speed' INTEGER, \
			'Stoke'  INTEGER, \
			'Reserve'  INTEGER, \
			'Displacement' TEXT, \
			'Load' TEXT,	\
			'Current' TEXT, \
			'Power'  TEXT ) ";
	res = sqlite3_exec( db, SQLCode, 0, 0, &errmsg );
	if(res)
	{
		printf("[错误]创建数据表失败：%d-%s \n", res, errmsg);
	}
	return res;
}
/*
 * 向数据库插入数据
 * 参数入口：数据库操作符
 */
int databaseInsert(sqlite3 *db, exchangebuffer *mnode)
{
	int res;
	char *errmsg = NULL;
	char **tb;									// 用于保存查询结果字符串的地址
	int nrow;										// 用于保存查询结果字符串的
	int ncol;
	char *SQLCode;

	SQLCode = (char *)malloc(100);
	if(!mnode)
		return 1;


	if(SQLCode != NULL)
		free(SQLCode);
	return res;
}
