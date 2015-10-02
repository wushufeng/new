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
#include	<time.h>
#include	"ManagerDB.h"
#include	"../database/database.h"
#include "../log/rtulog.h"

//#include	"../database/database.h"
//void outputItem(sqlite3_stmt* stat, int nColumn, char* out_pic_path);
const char *dbfile = "./DataBase/RTU.db";
const short int post[200] = {
0x0000, 0x0001, 0x0004, 0x000A, 0x0012, 0x001D, 0x002B, 0x003B, 0x004E, 0x0064, 0x007C, 0x0097, 0x00B4, 0x00D3, 0x00F5,
0x0118, 0x013E, 0x0166, 0x018F, 0x01BB, 0x01E9, 0x0219, 0x024A, 0x027D, 0x02B2, 0x02E9, 0x0322, 0x035C, 0x0398, 0x03D6,
0x0414, 0x0455, 0x0496, 0x04D8, 0x051A, 0x055D, 0x05A0, 0x05E2, 0x0626, 0x0669, 0x06AD, 0x06F2, 0x0736, 0x077B, 0x07C1,
0x0807, 0x084D, 0x0894, 0x08DA, 0x0920, 0x0965, 0x09AA, 0x09EE, 0x0A32, 0x0A75, 0x0AB7, 0x0AF7, 0x0B37, 0x0B77, 0x0BB5,
0x0BF3, 0x0C30, 0x0C6C, 0x0CA8, 0x0CE3, 0x0D1D, 0x0D56, 0x0D8E, 0x0DC5, 0x0DFB, 0x0E2F, 0x0E62, 0x0E93, 0x0EC3, 0x0EF1,
0x0F1E, 0x0F49, 0x0F73, 0x0F9C, 0x0FC4, 0x0FEA, 0x100F, 0x1033, 0x1056, 0x1077, 0x1096, 0x10B4, 0x10D0, 0x10EA, 0x1102,
0x1119, 0x112D, 0x1140, 0x1151, 0x1160, 0x116D, 0x1179, 0x1182, 0x118A, 0x118F, 0x1193, 0x1195, 0x1194, 0x1191, 0x118D,
0x1186, 0x117D, 0x1172, 0x1165, 0x1156, 0x1144, 0x1130, 0x111B, 0x1103, 0x10E9, 0x10CD, 0x10AF, 0x108F, 0x106D, 0x1049,
0x1023, 0x0FFB, 0x0FD2, 0x0FA7, 0x0F7A, 0x0F4C, 0x0F1C, 0x0EEB, 0x0EB8, 0x0E83, 0x0E4D, 0x0E14, 0x0DDB, 0x0D9F, 0x0D63,
0x0D26, 0x0CE7, 0x0CA8, 0x0C68, 0x0C28, 0x0BE7, 0x0BA6, 0x0B64, 0x0B22, 0x0ADF, 0x0A9B, 0x0A57, 0x0A12, 0x09CC, 0x0986,
0x0940, 0x08F9, 0x08B3, 0x086D, 0x0827, 0x07E2, 0x079D, 0x0758, 0x0714, 0x06D0, 0x068C, 0x0649, 0x0606, 0x05C3, 0x0581,
0x053F, 0x04FF, 0x04BF, 0x0480, 0x0442, 0x0405, 0x03CA, 0x0390, 0x0358, 0x0320, 0x02EA, 0x02B5, 0x0282, 0x0250, 0x021F,
0x01F0, 0x01C3, 0x0197, 0x016E, 0x0146, 0x0120, 0x00FD, 0x00DB, 0x00BC, 0x00A0, 0x0085, 0x006D, 0x0057, 0x0043, 0x0032,
0x0023, 0x0017, 0x000D, 0x0006, 0x0000
};
const short int constload[200] = {
		0x1274, 0x12A3, 0x12F5, 0x1372, 0x13F6, 0x146E, 0x14DD, 0x153C, 0x158E, 0x15D9, 0x161A, 0x1674, 0x1703, 0x17C5, 0x18A5,
		0x1985, 0x1A4D, 0x1AFF, 0x1B9B, 0x1C29, 0x1CB5, 0x1D50, 0x1E01, 0x1E72, 0x1E7E, 0x1E1B, 0x1D6A, 0x1CB4, 0x1C11, 0x1B90,
		0x1B14, 0x1A9C, 0x1A4A, 0x1A24, 0x1A57, 0x1AD2, 0x1B60, 0x1BFC, 0x1C76, 0x1CD1, 0x1D22, 0x1D48, 0x1D4B, 0x1D22, 0x1CC6,
		0x1C50, 0x1BAE, 0x1AFB, 0x1A66, 0x19F0, 0x19AB, 0x19A2, 0x19BA, 0x1A0F, 0x1A81, 0x1B03, 0x1BAB, 0x1C2B, 0x1C84, 0x1CC3,
		0x1CC3, 0x1CA6, 0x1C65, 0x1BE5, 0x1B57, 0x1AAF, 0x1A14, 0x19BE, 0x1985, 0x197E, 0x199E, 0x19DA, 0x1A47, 0x1ACF, 0x1B62,
		0x1BE8, 0x1C49, 0x1C76, 0x1C82, 0x1C64, 0x1C27, 0x1BCC, 0x1B4C, 0x1ACC, 0x1A4A ,0x19DF, 0x1999, 0x1979, 0x1994, 0x19DD,
		0x1A4D, 0x1AD0, 0x1B45, 0x1B99, 0x1BD7, 0x1BF7, 0x1BFB, 0x1BF5, 0x1BE3, 0x1BB4, 0x1B73, 0x1B21, 0x1AD4, 0x1A95, 0x1A42,
		0x19D8, 0x194B, 0x18B1, 0x181F, 0x178E, 0x16EF, 0x163B, 0x1581, 0x14CE, 0x1443, 0x13DD, 0x1394, 0x1362, 0x131B, 0x12C6,
		0x1243, 0x1189, 0x10B5, 0x0FCE, 0x0EFB, 0x0E54, 0x0DDA, 0x0DA0, 0x0DB5, 0x0E25, 0x0EEC, 0x0FD1, 0x10A7, 0x114C, 0x11AD,
		0x11D7, 0x11CC, 0x1178, 0x10EB, 0x1028, 0x0F58, 0x0EB8, 0x0E54, 0x0E37, 0x0E58, 0x0EA6, 0x0F21, 0x0FBF, 0x1068, 0x10FD,
		0x115E, 0x1187, 0x1174, 0x112E, 0x10C8, 0x1052, 0x0FDB, 0x0F6E, 0x0F1F, 0x0EF3, 0x0EF3, 0x0F2A, 0x0F94, 0x1013, 0x1095,
		0x10FF, 0x113F, 0x1162, 0x1166, 0x1151, 0x111F, 0x10CD, 0x106F, 0x1014, 0x0FCC, 0x0FAB, 0x0FAB, 0x0FCD, 0x101D, 0x1081,
		0x10FB, 0x1170, 0x11C4, 0x11FA, 0x1204, 0x11EA, 0x11B1, 0x115B, 0x10F6, 0x1099, 0x104F, 0x102B, 0x102B, 0x104C, 0x109E,
		0x1104, 0x1171, 0x11E1, 0x122B, 0x1251
};
short int readd[200];
const char *SQLCode = "create table if not exists 'carddata' ( \
			'ID'  INTEGER PRIMARY KEY AUTOINCREMENT, \
			'Upload'	TEXT,	\
			'Wellname'  TEXT, \
			'Interval'  INTEGER, \
			'Manual' INTEGER ,\
			'SetDot'  INTEGER, \
			'ActualDot'  INTEGER, \
			'CollectDateTime'  TEXT, \
			'Speed' INTEGER, \
			'Stroke'  INTEGER, \
			'Displacement' TEXT, \
			'Load' TEXT,	\
			'Current' TEXT, \
			'Power'  varchar(2000) ); ";
/*
 * 连接数据库
 * 入口：数据库文件路径
 * 返回：成功返回0，失败返回1，并打印错误信息
 */
int openDatabase(const char *dbfile, sqlite3 *db)
{
	int res;

//	res = sqlite3_open(dbfile, &DBfd);
	res = sqlite3_open_v2(dbfile, \
			&db, \
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, \
			NULL);
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "无法打开数据库，错误号[%d]: %s", res, sqlite3_errmsg(db));
//		printf("[错误]无法打开数据库，错误号[%d]: %s\n", res, sqlite3_errmsg(db));
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
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "无法关闭数据库，错误号[%d]: %s", res, sqlite3_errmsg(db));
//		printf("[错误]无法关闭数据库，错误号[%d]: %s\n", res, sqlite3_errmsg(db));
	}
	return res;
}
/*
 * 建立数据库数据表
 * 参数入口：数据库操作符
 * 返回：成功返回0，失败返回1，并打印错误信息
 */

int createSqlTable(void)
{
	sqlite3 *pdb = NULL;
	int res;
	char *errmsg = NULL;

	// 打开数据库
	res = sqlite3_open_v2(dbfile, &pdb, SQLITE_OPEN_READWRITE |SQLITE_OPEN_CREATE, NULL);
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "sqlite3_open_v2失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
//		printf("[错误]sqlite3_open_v2失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
		return res;
	}
	res = sqlite3_exec( pdb, SQLCode, 0, 0, &errmsg );
	if(res != SQLITE_OK)
	{
//		char * tempinfo = NULL;
//		sprintf(tempinfo,"[错误]创建数据表失败：%d-%s \n", res, errmsg);
//		zlog_info(c, tempinfo);
		zlog_warn(c, "创建数据表失败：%d-%s", res, errmsg);
//		printf("[错误]创建数据表失败：%d-%s \n", res, errmsg);
		sqlite3_free(errmsg);								// 释放内部分配的空间
		sqlite3_close(pdb);							// 来用关闭打开的数据库
		return 1;
	}
	sqlite3_close(pdb);							// 来用关闭打开的数据库
	return res;
}
/*
 * 向数据库插入数据
 * 参数入口：被写入的数据（包含该数据数据完整标识），测试时间，所属仪器号（0-16）
 */
int databaseInsert(void *obj, int group, time_t dgtime, int dgflag, int elecflag)
{
	int res;
	//int n;
	char *buffer;
	sqlite3 *pdb = NULL;
//	sqlite3_stmt* pStmt = NULL;//sqlite3内部定义的一种"SQL语句
	char *errmsg = NULL;
	time_t nowtime;
	load_displacement *pobj = (load_displacement *)obj;
	// 打开数据库
	res = sqlite3_open_v2(dbfile, &pdb, SQLITE_OPEN_READWRITE |SQLITE_OPEN_CREATE, NULL);
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "sqlite3_open_v2失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
//		printf("[错误]sqlite3_open_v2失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
		return res;
	}
	// 准备待插入得数据
	nowtime = time(NULL);
	if(dgtime)
		nowtime = dgtime;
	struct tm *now = localtime(&nowtime);
	char *timebuffer;
	timebuffer = (char *)malloc(64);
	bzero(timebuffer, 64);
	snprintf(timebuffer, 64, "%d-%02d-%02d %02d:%02d:%02d", \
			now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, \
			now->tm_hour, now->tm_min, now->tm_sec);
	int speed;
	speed = (int)(pobj->pumping_speed[0] * 256 + pobj->pumping_speed[1]);
	int stroke;
	stroke = (int)(pobj->pumping_stroke[0] * 256 + pobj->pumping_stroke[1]);

	// 插入基本数据insert into
	buffer = (char *)malloc(2048);
	bzero(buffer, 2048);
	snprintf(buffer, 2048, "insert into carddata (Wellname, Upload, Interval, Manual, SetDot, ActualDot, CollectDateTime,Speed, Stroke) \
			values (%d, \'%s\', %d, %d, %d, %d, \'%s\', %d, %d);", \
			group, "NO", pobj->interval, pobj->manul_collection_order, pobj->set_dot, pobj->actual_dot, timebuffer, speed, stroke);
	res = sqlite3_exec(pdb, buffer, 0, 0, &errmsg);
	if(res){
//		char * tempinfo = NULL;
//		sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//		zlog_info(c, tempinfo);
		fprintf(stderr, "[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
	}
	// 读出最新序列,总序列数即为最后一个序列得数列号。
    char** pResult;
    int nRow;
    int nCol;
    res = sqlite3_get_table(pdb, "select * from carddata;",&pResult, &nRow, &nCol, &errmsg);
    if (res != SQLITE_OK)
	{
		sqlite3_close(pdb);
		zlog_warn(c, "sqlite3_get_table失败,错误号=%d:%s",res, sqlite3_errmsg(pdb));
//		fprintf(stderr, "[错误]sqlite3_get_table失败,错误号=%d:%s\n",res, sqlite3_errmsg(pdb));
		sqlite3_free(errmsg);
		return 1;
	}
    sqlite3_free_table(pResult);
    printf("nRow = %d, nCol = %d\n", nRow, nCol);
    // 将位移数据字段更新最后一条数据
	// 准备位移数据，转换为字符串
	char *tbuf;
	int L, i;
	tbuf = (char*)malloc(2000);
	if(dgflag)
	{
		bzero(tbuf, 2000);
		for (i = 0; i < 200; i ++) {
			L = strlen(tbuf);
			if  (i < 199) sprintf(tbuf+L,"%d, ",pobj->displacement[i]);
			else sprintf(tbuf+L,"%d", pobj->displacement[i]);
		};
		snprintf(buffer, 2048, "update carddata set Displacement = \'%s\' where ID = %d;", tbuf, nRow);
		res = sqlite3_exec(pdb, buffer, 0, 0, &errmsg);
		if(res){
//			char * tempinfo = NULL;
//			sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//			zlog_info(c, tempinfo);
			zlog_warn(c, "sqlite_exec失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
//			fprintf(stderr, "[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
		}
		// 将功图数据字段更新最后一条数据
		bzero(tbuf, 2000);
		for (i = 0; i < 200; i ++) {
			L = strlen(tbuf);
			if  (i < 199) sprintf(tbuf+L,"%d, ",pobj->load[i]);
			else sprintf(tbuf+L,"%d", pobj->load[i]);
		};
		bzero(buffer, 2048);
		snprintf(buffer, 2048, "update carddata set Load = \'%s\' where ID = %d;", tbuf, nRow);
		res = sqlite3_exec(pdb, buffer, 0, 0, &errmsg);
		if(res){
//			char * tempinfo = NULL;
//			sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//			zlog_info(c, tempinfo);
			zlog_warn(c, "sqlite_exec失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
		}
	}
	// 将电流图更新到最后一条数据
	if(elecflag)
	{
		bzero(tbuf, 2000);
		for (i = 0; i < 200; i ++) {
			L = strlen(tbuf);
			if  (i < 199) sprintf(tbuf+L,"%d, ",pobj->current[i]);
			else sprintf(tbuf+L,"%d", pobj->current[i]);
		};
		bzero(buffer, 2048);
		snprintf(buffer, 2048, "update carddata set Current = \'%s\' where ID = %d;", tbuf, nRow);
		res = sqlite3_exec(pdb, buffer, 0, 0, &errmsg);
		if(res){
//			char * tempinfo = NULL;
//			sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//			zlog_info(c, tempinfo);
			zlog_warn(c, "sqlite_exec失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
		}
		// 将功率更新到最后一条数据
		bzero(tbuf, 2000);
		for (i = 0; i < 200; i ++) {
			L = strlen(tbuf);
			if  (i < 199) sprintf(tbuf+L,"%d, ",pobj->power[i]);
			else sprintf(tbuf+L,"%d", pobj->power[i]);
		};
		bzero(buffer, 2048);
		snprintf(buffer, 2048, "update carddata set Power = \'%s\' where ID = %d;", tbuf, nRow);
		res = sqlite3_exec(pdb, buffer, 0, 0, &errmsg);
		if(res){
//			char * tempinfo = NULL;
//			sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//			zlog_info(c, tempinfo);
			zlog_warn(c, "sqlite_exec失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
		}
	}
	// 释放动态内存
	free(buffer);
	// 关闭数据库
//	closeDatabase(pdb);
	res = sqlite3_close(pdb);
	if(res)
	{
		zlog_warn(c, "sqlite3_close失败，错误号=[%d]:%s", res, sqlite3_errmsg(pdb));
		return res;
	}
	return 0;
}
/*
 * 从数据库查看一条记录
 */
int showRecordsByTime(time_t time)
{
	int res;
	sqlite3 *pdb = NULL;
	char *sqlcode;
	sqlite3_stmt* pStmt = NULL;//sqlite3内部定义的一种"SQL语句

	// 打开数据库
	res = sqlite3_open_v2(dbfile, &pdb, SQLITE_OPEN_READWRITE |SQLITE_OPEN_CREATE, NULL);
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "sqlite3_open_v2失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
		return res;
	}
	// 转换时间为字符串
	struct tm *now = localtime(&time);
	char *timebuffer;
	timebuffer = (char *)malloc(64);
	bzero(timebuffer, 64);
	snprintf(timebuffer, 64, "%d-%d-%d %d:%d:%d", \
			now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, \
			now->tm_hour, now->tm_min, now->tm_sec);
	//
	sqlcode = (char *)malloc(128);
	bzero(sqlcode, 128);
	snprintf(sqlcode, 128, "select * from carddata where CollectDateTime >= \'%s\'", timebuffer);

	sqlite3_prepare_v2(pdb, sqlcode, -1, &pStmt, NULL);
	res = sqlite3_step(pStmt);//读取某一条记录
	while (res == SQLITE_ROW) /* sqlite3_step() has another row ready */
	{
	   int id;
		id =  sqlite3_column_int(pStmt,0);
			if(!id)
				continue;
		printf("ID = %d\n",id);
		char *name;
		name = (char *)sqlite3_column_text(pStmt, 1);
		printf("井名 = %s\n", name);
		char *tt;
		tt = (char *)sqlite3_column_text(pStmt,6);
		printf("时间 = %s\n", tt);
		res = sqlite3_step(pStmt);//读取某一条记录
   }
   sqlite3_finalize(pStmt);
   free(timebuffer);
   free(sqlcode);
	// 关闭数据库
	res = sqlite3_close(pdb);
	if(res)
	{
		zlog_warn(c, "sqlite3_close失败，错误号=[%d]:%s", res, sqlite3_errmsg(pdb));
		return res;
	}
	return 0;
}
//获取类型
char * GetType(int t)
{
    char * s;
    switch (t)
    {
    case 1: s = "SQLITE_INTEGER";break;
    case 2: s = "SQLITE_FLOATE"; break;
    case 3: s = "SQLITE_TEXT"; break;
    case 4: s = "SQLITE_BLOB"; break;
    case 5: s = "SQLITE_NULL"; break;
    }
    return s;
}
int testCreateTables(void)
{
	int res;
	sqlite3 *pdb = NULL;
	char *errmsg = NULL;

	// 打开数据库没有则创建
	res = sqlite3_open_v2(dbfile, &pdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if(res != SQLITE_OK)
	{
		zlog_warn(c, "sqlite3_open_v2失败,错误号=%d:%s", res, sqlite3_errmsg(pdb));
		return res;
	}
	// 创建表
	res = sqlite3_exec( pdb, SQLCode, 0, 0, &errmsg );
	if(res != SQLITE_OK)
	{
//		sprintf(errmsg,"%s", sqlite3_errmsg(pdb));
//		if(strcmp(errmsg, "table 'carddata' already exists"))
//		char * tempinfo = NULL;
//		sprintf(tempinfo,"[错误]sqlite_exec失败,错误号=%d:%s\n", res, sqlite3_errmsg(pdb));
//		zlog_info(c, tempinfo);
		zlog_warn(c, "创建数据表失败，错误号=%d:%s", res, sqlite3_errmsg(pdb));
//		return res;
	}
	// 关闭数据库
	res = sqlite3_close(pdb);
	if(res)
	{
		zlog_warn(c, "无法关闭数据库，错误号=[%d]:%s", res, sqlite3_errmsg(pdb));
		return res;
	}
	return 0;
}
